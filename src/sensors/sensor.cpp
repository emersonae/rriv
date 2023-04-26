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

#include "sensor.h"
#include "system/logs.h"
#include "sensors/sensor_map.h"
#include "system/eeprom.h"

SensorDriver::SensorDriver(){}
SensorDriver::~SensorDriver(){}

cJSON *SensorDriver::getConfigurationJSON() // returns unprotected pointer
{
  cJSON *json = cJSON_CreateObject();
  cJSON_AddNumberToObject(json, "slot", commonConfigurations.slot + 1);
  cJSON_AddStringToObject(json, "type", getSensorTypeString());
  cJSON_AddStringToObject(json, "tag", commonConfigurations.tag);
  cJSON_AddNumberToObject(json, "burst_size", commonConfigurations.burst_size);
  this->appendDriverSpecificConfigurationJSON(json);
  return json;
}

const configuration_bytes SensorDriver::getConfigurationBytes()
{
  configuration_bytes configurationBytes;
  memcpy(&configurationBytes.common, &commonConfigurations, sizeof(configuration_bytes_partition));
  configuration_bytes_partition driverSpecificPartition = getDriverSpecificConfigurationBytes();
  memcpy(&configurationBytes.specific, &driverSpecificPartition, sizeof(configuration_bytes_partition));
  return configurationBytes;
}

const common_sensor_driver_config * SensorDriver::getCommonConfigurations()
{
  return &commonConfigurations;
}


void SensorDriver::configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurations)
{
  // override to load driver specific configurations
} 

configuration_bytes_partition SensorDriver::getDriverSpecificConfigurationBytes()
{
  configuration_bytes_partition emptyPartition;
  return emptyPartition;
}

bool SensorDriver::configureDriverFromJSON(cJSON *json)
{
  // override to load driver specific configurations
  return true;
}
  
void SensorDriver::appendDriverSpecificConfigurationJSON(cJSON * json)
{
  // override to return driver specific configurations
}

void SensorDriver::initializeBurst()
{
  burstCount = 0;
  burstSummarySumCounts.clear();
  burstSummarySums.clear();
}

void SensorDriver::incrementBurst()
{
  burstCount++;
}

bool SensorDriver::burstCompleted()
{
  // notify(burstCount);
  // notify(commonConfigurations.burst_size);
  return burstCount >= commonConfigurations.burst_size;
}

void SensorDriver::addValueToBurstSummaryMean(std::string tag, double value)
{
  if(burstSummarySums.count(tag) == 0)
  {
    burstSummarySums[tag] = 0;
    burstSummarySumCounts[tag] = 0;
  }
  burstSummarySums[tag] += value;
  burstSummarySumCounts[tag] += 1;
}

double SensorDriver::getBurstSummaryMean(std::string tag)
{
  return burstSummarySums[tag] / burstSummarySumCounts[tag];
}

void SensorDriver::configureCSVColumns()
{
  // notify("config csv columns");
  char csvColumnHeaders[100] = "\0";
  char buffer[100];
  strcpy(buffer, this->getBaseColumnHeaders());
  // debug(buffer);
  char * token = strtok(buffer, ",");
  while(token != NULL)
  {
    // debug(token);
    strcat(csvColumnHeaders, this->commonConfigurations.tag);
    strcat(csvColumnHeaders, "_");
    strcat(csvColumnHeaders, token);
    token = strtok(NULL, ",");
    if(token != NULL)
    {
      strcat(csvColumnHeaders, ",");
    }
  }
  strcpy(this->csvColumnHeaders, csvColumnHeaders);
  // notify("done");
}

char *SensorDriver::getCSVColumnHeaders()
{
  return csvColumnHeaders;
}

void SensorDriver::setDefaults()
{
  if(commonConfigurations.burst_size <= 0 || commonConfigurations.burst_size > 100)
  {
    commonConfigurations.burst_size = 10;
  }
  this->setDriverDefaults();
}

void invalid(){
  notify("Invalid");
}

bool SensorDriver::configureFromJSON(cJSON * json)
{
  
#ifndef PRODUCTION_FIRMWARE_BUILD
  if( sizeof(commonConfigurations) > sizeof(configuration_bytes_partition) )
  { 
    // TODO: improve these messages to indicate how much overflow there is
    // TODO: tell which driver is causing the issue, along the lines of this->getType()
    // TODO: this is to help during driver dev, not production firmware
    notify("Invalid memory size for driver common configuration");
    exit(1);
  }
#endif

  memset(&commonConfigurations, 0, SENSOR_CONFIGURATION_SIZE);

  commonConfigurations.sensor_type = typeCodeForSensorTypeString(getSensorTypeString());

  const cJSON* slotJSON = cJSON_GetObjectItemCaseSensitive(json, "slot");
  if(slotJSON != NULL && cJSON_IsNumber(slotJSON) && slotJSON->valueint > 0 && slotJSON->valueint <= EEPROM_TOTAL_SENSOR_SLOTS)
  {
    commonConfigurations.slot = slotJSON->valueint - 1;
  }
  else
  {
    invalid();
    notify("slot");
    return false;
  }

  const cJSON * tagJSON = cJSON_GetObjectItemCaseSensitive(json, "tag");
  if(tagJSON != NULL && cJSON_IsString(tagJSON) && strlen(tagJSON->valuestring) <= 5)
  {
    strcpy(commonConfigurations.tag, tagJSON->valuestring);
  }
  else
  {
    invalid();
    notify("tag");
    return false;
  }

  const cJSON * burstSizeJson = cJSON_GetObjectItemCaseSensitive(json, "burst_size");
  if(burstSizeJson != NULL && cJSON_IsNumber(burstSizeJson) && burstSizeJson->valueint > 0)
  {
    commonConfigurations.burst_size = (byte) burstSizeJson->valueint;
  }
  else
  {
    invalid();
    notify("burst_size");
    return false;
  }

  this->setDefaults();
  if (this->configureDriverFromJSON(json) == false)
  {
    return false;
  }
  this->configureCSVColumns();
  return true;
}


void SensorDriver::configureFromBytes(configuration_bytes configurationBytes)
{
  configuration_bytes_partition partitions[2];
  memcpy(&partitions, &configurationBytes, sizeof(configuration_bytes));
  memcpy(&commonConfigurations, &partitions[0], sizeof(configuration_bytes_partition));
  this->configureSpecificConfigurationsFromBytes(partitions[1]);
  this->configureCSVColumns();
}

void SensorDriver::setup()
{
  // by default no setup
  return;
}

void SensorDriver::stop()
{

}

void SensorDriver::hibernate()
{

}

void SensorDriver::wake()
{

}

void SensorDriver::setDebugMode(bool debug) // for setting internal debug parameters, such as LED on
{

}

bool SensorDriver::isWarmedUp()
{
  return true;
}

short SensorDriver::getSlot()
{
  return commonConfigurations.slot;
}

void SensorDriver::setConfigurationNeedsSave()
{
  configurationNeedsSave = true;
}

void SensorDriver::clearConfigurationNeedsSave()
{
  configurationNeedsSave = false;
}

bool SensorDriver::getNeedsSave()
{
  return this->configurationNeedsSave;
}

int SensorDriver::millisecondsToWarmUp()
{
  return 0; // default is ready to read... we shouldn't ask this unless a sensor is not warmed up though, so the default should throw an error?
}

uint32 SensorDriver::millisecondsUntilNextReadingAvailable()
{
  return 0; // return min by default, a larger number in driver implementation causes correct delay
}

uint32 SensorDriver::millisecondsUntilNextRequestedReading()
{
  return MAX_REQUESTED_READING_DELAY; // as slow as possible by default, a smaller number in driver implementation forces faster read
}


AnalogProtocolSensorDriver::~AnalogProtocolSensorDriver(){}

protocol_type AnalogProtocolSensorDriver::getProtocol()
{
  return analog;
}


I2CProtocolSensorDriver::~I2CProtocolSensorDriver(){}

protocol_type I2CProtocolSensorDriver::getProtocol()
{
  return i2c;
}

void I2CProtocolSensorDriver::setWire(TwoWire * wire)
{
  this->wire = wire;
}


GPIOProtocolSensorDriver::~GPIOProtocolSensorDriver(){}

protocol_type GPIOProtocolSensorDriver::getProtocol()
{
  // debug("getting gpio protocol");
  return gpio;
}


DriverTemplateProtocolSensorDriver::~DriverTemplateProtocolSensorDriver(){}

protocol_type DriverTemplateProtocolSensorDriver::getProtocol()
{
  // debug("getting driver template protocol");
  return drivertemplate;
}


// function for atlas co2 sensor - KC
void SensorDriver::factoryReset(){}