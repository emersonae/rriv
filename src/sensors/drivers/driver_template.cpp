#include "sensors/drivers/driver_template.h"
#include "system/logs.h" // for debug() and notify()
// #include "system/measurement_components.h" // if external adc is used

#define VAR_TAG "var"

DriverTemplate::DriverTemplate()
{
  // debug("allocating driver template");
}

DriverTemplate::~DriverTemplate(){}

const char * DriverTemplate::getSensorTypeString()
{
  return sensorTypeString;
}

configuration_bytes_partition DriverTemplate::getDriverSpecificConfigurationBytes()
{
  configuration_bytes_partition partition;
  memcpy(&partition, &configuration, sizeof(driver_configuration));
  return partition;
}

bool DriverTemplate::configureDriverFromJSON(cJSON *json)
{
  return true;
}


void DriverTemplate::configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurationPartition)
{
  memcpy(&configuration, &configurationPartition, sizeof(driver_configuration));
}

void DriverTemplate::appendDriverSpecificConfigurationJSON(cJSON * json)
{
  // debug("appeding driver specific driver template json");
  
  //driver specific config, customize
  addCalibrationParametersToJSON(json);
}

void DriverTemplate::setup()
{
  // debug("setup DriverTemplate");
}

void DriverTemplate::stop()
{
  // debug("stop/sleep and delete new DriverTemplate");
}

bool DriverTemplate::takeMeasurement()
{
  // debug("taking measurement from driver template");
  //return true if measurement taken store in class value(s), false if not
  bool measurementTaken = false;
  if(true)
  {
    value = 42;
    measurementTaken = true;
  }
  addValueToBurstSummaryMean(VAR_TAG, value); // use the default option for computing the burst summary value
  return measurementTaken;
}

const char *DriverTemplate::getRawDataString()
{
  // debug("configuring driver template dataString");
  // process data string for .csv
  sprintf(dataString, "%d,%0.3f",value,value*31.83);
  return dataString;
}

const char *DriverTemplate::getSummaryDataString()
{
  double burstSummaryMean = getBurstSummaryMean(VAR_TAG);
  sprintf(dataString, "%0.3f,%0.3f", burstSummaryMean, burstSummaryMean*31.83);
  return dataString;  
}

const char *DriverTemplate::getBaseColumnHeaders()
{
  // for debug column headers defined in the .h
  // debug("getting driver template base column headers");
  return baseColumnHeaders;
}

void DriverTemplate::initCalibration()
{
  // debug("init driver template sensor calibration");
}

void DriverTemplate::calibrationStep(char *step, int arg_cnt, char ** args)
{
  // for intermediary steps of calibration process
  // debug("driver template calibration steps");
}

void DriverTemplate::addCalibrationParametersToJSON(cJSON *json)
{
  // follows structure of calibration parameters in .h
  // debug("add driver template calibration parameters to json");
  cJSON_AddNumberToObject(json, CALIBRATION_TIME_STRING, configuration.cal_timestamp);
}

void DriverTemplate::setDriverDefaults()
{
  // debug("setting driver template driver defaults");
  // set default values for driver struct specific values
  configuration.cal_timestamp = 0;
}