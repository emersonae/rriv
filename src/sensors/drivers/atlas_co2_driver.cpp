/* 
 *  RRIV - Open Source Environmental Data Logging Platform
 *  Copyright (C) 20202  Zaven Arra  zaven.arra@gmail.com
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 */
#include "sensors/drivers/atlas_co2_driver.h"
#include "system/logs.h" // for debug() and notify()

#define CO2_TAG "co2"

AtlasCO2Driver::AtlasCO2Driver()
{
  // debug("allocating driver template");
}

AtlasCO2Driver::~AtlasCO2Driver(){}

const char * AtlasCO2Driver::getSensorTypeString()
{
  return sensorTypeString;
}

configuration_bytes_partition AtlasCO2Driver::getDriverSpecificConfigurationBytes()
{
  configuration_bytes_partition partition;
  memcpy(&partition, &configuration, sizeof(driver_config));
  return partition;
}

void AtlasCO2Driver::configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurationPartition)
{
  memcpy(&configuration, &configurationPartition, sizeof(driver_config));
}

void AtlasCO2Driver::appendDriverSpecificConfigurationJSON(cJSON * json)
{
  // debug("appeding driver specific driver template json");
  
  //driver specific config, customize
  addCalibrationParametersToJSON(json);
}

// TODO: should setup be distinguished from wakeup?
void AtlasCO2Driver::setup()
{
  // debug("setup AtlasCO2Driver");
  modularSensorDriver = new AtlasScientificCO2(wire,-1);
  if(!modularSensorDriver->setup()){
    notify("CO2 setup failed");
  }
  modularSensorDriver->wake();
  setupTime = millis();
}

void AtlasCO2Driver::stop()
{
  modularSensorDriver->sleep();
  delete modularSensorDriver;
  // debug("stop/delete AtlasCO2Driver");
}

// void AtlasCO2Driver::factoryReset()
// {
//   notify("FactoryReset");
//   wire->beginTransmission(0x69); // default address
//   wire->write((const uint8_t*)"Factory",8);
// }

bool AtlasCO2Driver::takeMeasurement()
{
  // debug("taking measurement from driver template");
  //return true if measurement taken store in class value(s), false if not
  modularSensorDriver->startSingleMeasurement();
  modularSensorDriver->waitForMeasurementCompletion();
  bool measurementTaken = modularSensorDriver->addSingleMeasurementResult();
  if(measurementTaken)
  {
    value = modularSensorDriver->sensorValues[0];
    modularSensorDriver->clearValues();
    measurementTaken = true;
    addValueToBurstSummaryMean(CO2_TAG, value);
  }
  else
  {
    value = -1;
  }

  return measurementTaken;
}

const char *AtlasCO2Driver::getRawDataString()
{
  sprintf(dataString, "%d", value);
  return dataString;
}

const char * AtlasCO2Driver::getSummaryDataString()
{
  sprintf(dataString, "%0.2f", getBurstSummaryMean(CO2_TAG));
  return dataString;
}
// const char *AtlasCO2Driver::getRawDataString()
// {
//   // debug("configuring driver template dataString");
//   // process data string for .csv
//   sprintf(dataString, "%d,%d",value,0);
//   return dataString;
// }

const char *AtlasCO2Driver::getBaseColumnHeaders()
{
  // for debug column headers defined in the .h
  // debug("getting driver template base column headers");
  return baseColumnHeaders;
}

void AtlasCO2Driver::initCalibration()
{
  // debug("init driver template sensor calibration");
}

void AtlasCO2Driver::calibrationStep(char *step, int arg_cnt, char ** args)
{
  // for intermediary steps of calibration process
  // debug("driver template calibration steps");
}

void AtlasCO2Driver::addCalibrationParametersToJSON(cJSON *json)
{
  // follows structure of calibration parameters in .h
  // debug("add driver template calibration parameters to json");
  cJSON_AddNumberToObject(json, CALIBRATION_TIME_STRING, configuration.cal_timestamp);
}

bool AtlasCO2Driver::configureDriverFromJSON(cJSON *json)
{
  // debug("configuring driver template driver from json");
  return true;
}

void AtlasCO2Driver::setDriverDefaults()
{
  // debug("setting driver template driver defaults");
  // set default values for driver struct specific values
  configuration.cal_timestamp = 0;
}

uint32 AtlasCO2Driver::millisecondsUntilNextReadingAvailable()
{
  return 1000; // 1 reading per second
}

bool AtlasCO2Driver::isWarmedUp()
{
  timeDiff = millis() - setupTime;
  if (timeDiff < 10000) // need 10 seconds to warm up
  {
    return false;
  }
  return true;
}

int AtlasCO2Driver::millisecondsToWarmUp()
{
  int warmupTime = 10000 - timeDiff;
  setupTime -= warmupTime; // account for systick being off when sleeping
  return warmupTime;
}
