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
#include "sensors/drivers/BME280_driver.h"
#include "system/logs.h" // for debug() and notify()
// #include "system/measurement_components.h" // if external adc is used

BME280Driver::BME280Driver()
{
  // debug("allocating driver template");
}

BME280Driver::~BME280Driver(){}

const char * BME280Driver::getSensorTypeString()
{
  return sensorTypeString;
}
configuration_bytes_partition BME280Driver::getDriverSpecificConfigurationBytes()
{
  configuration_bytes_partition partition;
  memcpy(&partition, &configuration, sizeof(driver_config));
  return partition;
}

void BME280Driver::configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurationPartition)
{
  memcpy(&configuration, &configurationPartition, sizeof(driver_config));
}

void BME280Driver::appendDriverSpecificConfigurationJSON(cJSON * json)
{
  // debug("appeding driver specific driver template json");
  
  //driver specific config, customize
  addCalibrationParametersToJSON(json);
}

void BME280Driver::setup()
{
  BME280ptr = new BME280();

}

void BME280Driver::stop() 
{
  // debug("stop/delete DriverTemplate");
}

bool BME280Driver::takeMeasurement()
{
  // debug("taking measurement from driver template");
  //return true if measurement taken store in class value(s), false if not
  
  bool measurementTaken = false; //temporarily setting to always true 
  temperature = BME280ptr->getTemperature();
  // if(isnan(temperature))
  // {
  //   notify("Error reading temperature)");
  // }
  // else
  // {
  //   measurementTaken = true;
  // }
  pressure = BME280ptr->getPressure();
  // if(isnan(pressure))
  // {
  //   notify("Error reading pressure");
  // }
  // else
  // {
  //   measurementTaken = true;
  // }
  measurementTaken = true;
  // if(measurementTaken)
  // {
  //   addValueToBurstSummaryMean("temperature", temperature);
  //   addValueToBurstSummaryMean("pressure", pressure);
  // }

  return measurementTaken;
}

const char *BME280Driver::getRawDataString()
{
  // debug("configuring driver template dataString");
  // process data string for .csv
  //sprintf(dataString, "%0.3f, %0.3f",temperature, pressure);
  sprintf(dataString, "%.2f,%.2f", temperature, pressure);
  return dataString;
}

const char *BME280Driver::getSummaryDataString()
{
  //double burstSummaryMean = getBurstSummaryMean("var");
  sprintf(dataString, "%.2f,%.2f", temperature, pressure);
  return dataString;
}

const char *BME280Driver::getBaseColumnHeaders()
{
  // for debug column headers defined in the .h
  // debug("getting driver template base column headers");
  return baseColumnHeaders;
}

void BME280Driver::initCalibration()
{
  // debug("init driver template sensor calibration");
}

void BME280Driver::calibrationStep(char *step, int arg_cnt, char ** args)
{
  // for intermediary steps of calibration process
  // debug("driver template calibration steps");
}




bool BME280Driver::configureDriverFromJSON(cJSON *json)
{
  return true;
}


void BME280Driver::addCalibrationParametersToJSON(cJSON *json)
{
  // follows structure of calibration parameters in .h
  // debug("add driver template calibration parameters to json");
  cJSON_AddNumberToObject(json, CALIBRATION_TIME_STRING, configuration.cal_timestamp);
}
