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

#include "datalogger.h"
#include <Cmd.h>
#include "system/measurement_components.h"
#include "system/monitor.h"
#include "system/watchdog.h"
#include "system/command.h"
#include "sensors/sensor_map.h"
#include "sensors/drivers/registry.h"
#include "utilities/i2c.h"
#include "utilities/qos.h"
#include "utilities/STM32-UID.h"
#include "scratch/dbgmcu.h"
#include "system/logs.h"

void Datalogger::sleepMCU(uint32 milliseconds)
{
  if(milliseconds < 5)
  {
    delay(milliseconds);
    return;
  }

  setNextAlarmInternalRTCMilliseconds(milliseconds);

  int iser1, iser2, iser3;
  storeAllInterrupts(iser1, iser2, iser3);

  disableCustomWatchDog();
  disableSerialLog();

  clearAllInterrupts();
  clearAllPendingInterrupts();

  nvic_irq_enable(NVIC_RTCALARM); // enable our RTC alarm interrupt

  enterSleepMode();

  nvic_irq_disable(NVIC_RTCALARM);

  reenableAllInterrupts(iser1, iser2, iser3);

  offsetMillis -= milliseconds; // account for millisecond count while systick was off

  enableSerialLog();

  startCustomWatchDog();
  reloadCustomWatchdog();
}

// static method to read configuration from EEPROM
void Datalogger::readConfiguration(datalogger_settings_type *settings)
{
  byte *buffer = (byte *)malloc(sizeof(byte) * sizeof(datalogger_settings_type));
  for (unsigned short i = 0; i < sizeof(datalogger_settings_type); i++)
  {
    short address = EEPROM_DATALOGGER_CONFIGURATION_START + i;
    buffer[i] = readEEPROM(&Wire, EEPROM_I2C_ADDRESS, address);
  }

  memcpy(settings, buffer, sizeof(datalogger_settings_type));

  // apply defaults
  if (settings->burstNumber == 0 || settings->burstNumber > 100)
  {
    settings->burstNumber = 1;
  }
  if (settings->interBurstDelay > 300)
  {
    settings->interBurstDelay = 0;
  }

  settings->debug_values = true;
  settings->log_raw_data = true;
  settings->debug_to_file = true;
}

Datalogger::Datalogger(datalogger_settings_type *settings)
{
  powerCycle = true;

  // defaults
  if (settings->wakeInterval < 1)
  {
    settings->wakeInterval = 1;
  }

  memcpy(&this->settings, settings, sizeof(datalogger_settings_type));

  switch (settings->mode)
  {
  case 'i':
    changeMode(interactive);
    strcpy(loggingFolder, reinterpret_cast<const char *> F("INTERACTIVE"));
    break;
  case 'l':
    changeMode(logging);
    strcpy(loggingFolder, settings->siteName);
    break;
  default:
    changeMode(interactive);
    strcpy(loggingFolder, reinterpret_cast<const char *> F("BENCH"));
    break;
  }
}

void Datalogger::setup()
{
  startCustomWatchDog();
  Monitor::instance()->debugToFile = settings.debug_to_file;

  setupHardwarePins();
  setupSwitchedPower();
  powerUpSwitchableComponents();

  bool externalADCInstalled = scanI2C(&Wire, 0x2f);
  settings.externalADCEnabled = externalADCInstalled;

  setupManualWakeInterrupts();
  disableManualWakeInterrupt(); // don't respond to interrupt during setup
  clearManualWakeInterrupt();

  clearAllAlarms(); // don't respond to alarms during setup

  //initBLE();

  unsigned char uuid[UUID_LENGTH];
  readUniqueId(uuid);

  decodeUniqueId(uuid, uuidString, UUID_LENGTH);

  checkMemory();
  buildDriverSensorMap();
  // debug("Built driver sensor map");
  loadSensorConfigurations();
  // debug("Loaded sensor configurations");
  initializeFilesystem();
  setUpCLI();
}

/*
*
* return: continue processing readings cycle bool
*/
bool Datalogger::processReadingsCycle()
{
  uint32 measureSensorDuration = millis();
  measureSensorValues();
  measureSensorDuration = millis() - measureSensorDuration;
  // notify(measureSensorDuration);

  if (settings.log_raw_data) // we are really talking about a burst summary
  {
    writeRawMeasurementToLogFile();
    if(settings.debug_to_file)
    {
      fileSystemWriteCache->flushCache();
    }
  }

  if (shouldContinueBursting())
  {
    uint32 waitBetweenReadings = minMillisecondsUntilNextReading() - measureSensorDuration;
    notify(waitBetweenReadings);
    if (waitBetweenReadings > 0)
    {
      burstCycleStartMillis -= waitBetweenReadings; // account for systick during sleep
      sleepMCU(waitBetweenReadings);
    }
    return true;
  }

  // otherwise burst cycle completed,
  // so output burst summary
  writeSummaryMeasurementToLogFile();    
  completedBursts++;
  if(settings.debug_to_file)
  {
    fileSystemWriteCache->flushCache();
  }

  if (completedBursts < settings.burstNumber)
  {
    // debug(F("do another burst"));
    if (settings.interBurstDelay > 0)
    {
      // notify(F("burstDelay"));
      int interBurstDelay = settings.interBurstDelay * 60; // convert to seconds
      // todo: we should sleep any sensors that can be slept without re-warming
      // this could be called 'standby' mode
      // placeSensorsInStandbyMode();
      
      // notify(interBurstDelay);
      uint32 millisElapsed = millis() - burstCycleStartMillis;
      notify(interBurstDelay - (millisElapsed/1000)); // convert to seconds
      sleepMCU(interBurstDelay * 1000 - millisElapsed); // convert second to millisecond, subtract time passed
      
      // wakeSensorsFromStandbyMode(); 
    }
    initializeBurst();
    return true;
  }
  return false;
}

void Datalogger::testMeasurementCycle()
{
  initializeMeasurementCycle();
  fileSystemWriteCache->setOutputToSerial(true); // another way to do this would be to set a special write cache
  while(processReadingsCycle()){
    fileSystemWriteCache->flushCache();
    outputLastMeasurement();
  }      
  fileSystemWriteCache->flushCache();            // instead of using a boolean in this particular write cache
  fileSystemWriteCache->setOutputToSerial(false);// and then set it back to the original writecache here
}

void Datalogger::loop()
{
  if (inMode(deploy_on_trigger))
  {
    deploy(); // if deploy returns false here, the trigger setup has a fatal coding defect not detecting invalid conditions for deployment
    goto SLEEP;
    return;
  }

  if (inMode(logging))
  {

    if (powerCycle)
    {
      debug("Powercycle");
      bool deployed = enterFieldLoggingMode();
      if (!deployed)
      {
        // what should we do here?
        // if we are in the field and the manual power cycle got skipped, the battery will quickly drain
        // perhaps just shut down the unit?
        powerDownSwitchableComponents();
        while (1)
          ;
      }
      powerCycle = false; // handled powercycle loop
      goto SLEEP;
    }

    if (shouldExitLoggingMode())
    {
      notify("Should exit logging mode");
      changeMode(interactive);
      return;
    }

    if(processReadingsCycle())
    {
      return;
    }

    // otherwise go to sleep
    fileSystemWriteCache->flushCache();
  SLEEP:
    stopAndAwaitTrigger(); // sleep and then wake
    initializeMeasurementCycle(); // once we wake up
    return;
  }

  processCLI();

  storeSensorConfigurationIfNeedsSave();

  if (inMode(logging) || inMode(deploy_on_trigger))
  {
    // processCLI may have moved logger into a deployed mode
    goto SLEEP;
  }
  else if (inMode(interactive))
  {
    if (interactiveModeLogging)
    {
      if (timestamp() > lastInteractiveLogTime + 1)
      {
        // notify(F("interactive log"));
        measureSensorValues(false);
        if(interactiveModeLogging)
        {
          outputLastMeasurement();
          Serial2.print(F("CMD >> "));
        }
        writeRawMeasurementToLogFile();
        fileSystemWriteCache->flushCache();
        lastInteractiveLogTime = timestamp();
      }
    }
    
  }
  else if (inMode(debugging))
  {
    measureSensorValues(false);
    writeRawMeasurementToLogFile();
    delay(5000); // this value could be configurable, also a step / read from CLI is possible
  }
  else
  {
    // invalid mode!
    notify(F("Bad Mode"));
    notify(mode);
    mode = interactive;
    delay(1000);
  }

  powerCycle = false;
}

void Datalogger::loadSensorConfigurations()
{

  // load sensor configurations from EEPROM and count them
  sensorCount = 0;
  configuration_bytes sensorConfigs[EEPROM_TOTAL_SENSOR_SLOTS];
  for (int i = 0; i < EEPROM_TOTAL_SENSOR_SLOTS; i++)
  {
    notify("reading slot");
    readSensorConfigurationFromEEPROM(i, &sensorConfigs[i]);

    common_sensor_driver_config * commonConfiguration = (common_sensor_driver_config *) &sensorConfigs[i].common;

    notify(commonConfiguration->sensor_type);
    if( sensorTypeCodeExists(commonConfiguration->sensor_type) )
    {
      // notify("found configured sensor");
      sensorCount++;
    }
    commonConfiguration->slot = i;
  }
  if (sensorCount == 0)
  {
    notify("no sensor config");
  }
  // notify("FREE MEM");
  // printFreeMemory();

  // construct the drivers
  // notify("construct drivers");
  drivers = (SensorDriver **)malloc(sizeof(SensorDriver *) * sensorCount);
  int j = 0;
  for (int i = 0; i < EEPROM_TOTAL_SENSOR_SLOTS; i++)
  {
    // notify("FREE MEM");
    // printFreeMemory();
    common_sensor_driver_config * commonConfiguration = (common_sensor_driver_config *) &sensorConfigs[i].common;

    if ( !sensorTypeCodeExists(commonConfiguration->sensor_type) )
    {
      notify("no sensor code");
      continue;
    }

    // debug("getting sensor driver");
    // debug(commonConfiguration->sensor_type);
    SensorDriver *driver = driverForSensorTypeCode(commonConfiguration->sensor_type);
    // debug("got sensor driver");
    checkMemory();

    drivers[j] = driver;
    j++;

    if (driver->getProtocol() == i2c)
    {
      // debug("got i2c sensor");
      ((I2CProtocolSensorDriver *)driver)->setWire(&WireTwo);
      // debug("set wire");
    }
    // debug("do setup");
    driver->setup();

    // debug("configure sensor driver");
    driver->configureFromBytes(sensorConfigs[i]); //pass configuration struct to the driver
    // debug("configured sensor driver");
  }

}

void Datalogger::reloadSensorConfigurations() // for dev & debug
{
  // calling this function does not deal with memory fragmentation
  // so it's not part of the main system, only for dev & debug
  // notify("FREE MEM reload");
  // printFreeMemory();
  // free sensor configs
  for(unsigned short i=0; i<sensorCount; i++)
  {
    delete(drivers[i]);
  }
  free(drivers);
  // notify("FREE MEM reload");
  // printFreeMemory();
  loadSensorConfigurations();
}

void Datalogger::startLogging()
{
  initializeMeasurementCycle();
  interactiveModeLogging = true;
}

void Datalogger::stopLogging()
{
  interactiveModeLogging = false;
}

bool Datalogger::shouldExitLoggingMode()
{
  if (Serial2.peek() != -1)
  {
    //attempt to process the command line
    for (int i = 0; i < 10; i++)
    {
      processCLI();
    }
    if (inMode(interactive))
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  return false;
}

bool Datalogger::shouldContinueBursting()
{
  for (unsigned short i = 0; i < sensorCount; i++)
  {
    if (!drivers[i]->burstCompleted())
    {
      return true;
    }
  }
  return false;
}

void Datalogger::initializeBurst()
{
  burstCycleStartMillis = millis();
  for (unsigned int i = 0; i < sensorCount; i++)
  {
    drivers[i]->initializeBurst();
  }
}

// setup phase for measurement cycle, happens once upon waking
void Datalogger::initializeMeasurementCycle()
{
  // notify(F("set base time"));
  currentEpoch = timestamp(); // only call time once per measurement cycle to preserve battery
  offsetMillis = millis(); // use systick to keep track of time instead, offset lets us start at 0

  measurementCycle++;
  initializeBurst();
  completedBursts = 0;

  if (settings.startUpDelay > 0)
  {
    notify(F("start up delay:"));
    int startUpDelay = settings.startUpDelay*60; // convert to seconds and print
    notify(startUpDelay);
    sleepMCU(startUpDelay * 1000); // convert seconds to milliseconds
  }

  // Check if each sensor is warmed up, if not, then store the max interval to sleep
  // before checking again until all report that they are warmed up
  // notify("warmup sensors");
  int warmUpTime = 0;
  for (unsigned short i = 0; i < sensorCount; i++)
  {
    // get the max warmup time between all sensors that are not warmed up
    if (drivers[i]->isWarmedUp() == false)
    {
      warmUpTime = max(warmUpTime, drivers[i]->millisecondsToWarmUp());
    }

    // sleep for that duration, then check if all sensors are now warmed up
    if (i == (sensorCount - 1) && warmUpTime > 0)
    {
      // notify(warmUpTime);
      sleepMCU(warmUpTime);
      i = 0;
      warmUpTime = 0;
    }
  }
  notify("warmed");
}

void Datalogger::measureSensorValues(bool performingBurst)
{
  if (settings.externalADCEnabled)
  {
    // get readings from the external ADC
    // debug("converting enabled channels call");
    externalADC->convertEnabledChannels();
    // debug("converted enabled channels");
  }

  for (unsigned int i = 0; i < sensorCount; i++)
  {
    // TODO: error checking if takeMeasurement returns false
    if (drivers[i]->takeMeasurement())
    {
      if (performingBurst)
      {
        drivers[i]->incrementBurst(); // burst bookkeeping
      }
    }
  }
}

void Datalogger::writeStatusFieldsToLogFile(const char * type)
{
  // debug(F("Write status fields"));

  fileSystemWriteCache->writeString(type);
  fileSystemWriteCache->writeString((char *)",");

  // Fetch and Log time from DS3231 RTC as epoch and human readable timestamps
  uint32 currentMillis = millis();

  double currentTime = (double) currentEpoch + ( (double) ( currentMillis - offsetMillis) ) / 1000;

  char currentTimeString[20];
  char humanTimeString[24]; // YYYY-MM-DD HH:MM:SS:sss
  sprintf(currentTimeString, "%10.3f", currentTime);                 // convert double value into string
  t_t2ts(currentTime, currentMillis - offsetMillis, humanTimeString); // convert time_t value to human readable timestamp

  fileSystemWriteCache->writeString(settings.siteName);
  fileSystemWriteCache->writeString((char *)",");
  fileSystemWriteCache->writeString(settings.loggerName);
  fileSystemWriteCache->writeString((char *)",");

  char buffer[100];
  if(settings.deploymentIdentifier[0] == 0xFF)
  {
    sprintf(buffer, "%s-%lu", uuidString, settings.deploymentTimestamp);
  }
  else
  {
    char deploymentIdentifier[16] = {0};
    strncpy(deploymentIdentifier, settings.deploymentIdentifier, 15);
    debug(deploymentIdentifier[0]);
    debug(deploymentIdentifier);
    debug(uuidString);
    debug(settings.deploymentTimestamp);
    sprintf(buffer, "%s-%s-%lu", deploymentIdentifier, uuidString, settings.deploymentTimestamp);
  }
  fileSystemWriteCache->writeString(buffer);
  fileSystemWriteCache->writeString((char *)",");
  sprintf(buffer, "%ld,", settings.deploymentTimestamp);
  fileSystemWriteCache->writeString(buffer);
  fileSystemWriteCache->writeString(uuidString);
  fileSystemWriteCache->writeString((char *)",");
  fileSystemWriteCache->writeString(currentTimeString);
  fileSystemWriteCache->writeString((char *)",");
  fileSystemWriteCache->writeString(humanTimeString);
  fileSystemWriteCache->writeString((char *)","); 

  // write out the raw battery reading
  // sprintf(buffer, "%d,", getBatteryValue()); // not working with v0.2 schematic

  // hardcoded to have battery into exADC port 3 along side 5v booster
  sprintf(buffer, "%d,", externalADC->getChannelValue(3)); 
  fileSystemWriteCache->writeString(buffer);

  // write measurement cycle number and burst number
  sprintf(buffer,"%d,%d,", measurementCycle, completedBursts+1);
  fileSystemWriteCache->writeString(buffer);
}

void Datalogger::writeUserFieldsToLogFile()
{
  char buffer[150];
  sprintf(buffer, ",%s,", userNote);
  fileSystemWriteCache->writeString(buffer);
  if (userValue != INT_MIN)
  {
    sprintf(buffer, "%d", userValue);
    fileSystemWriteCache->writeString(buffer);
  }
}

bool Datalogger::writeRawMeasurementToLogFile()
{
  writeStatusFieldsToLogFile("raw");

  // and write out the sensor data
  debug(F("Write data"));
  for (unsigned short i = 0; i < sensorCount; i++)
  {
    // get values from the sensors
    const char *dataString = drivers[i]->getRawDataString();
    fileSystemWriteCache->writeString(dataString);
    if (i < sensorCount - 1)
    {
      fileSystemWriteCache->writeString((char *)reinterpretCharPtr(F(",")));
    }
  }

  writeUserFieldsToLogFile();
  fileSystemWriteCache->endOfLine();
  return true;
}

bool Datalogger::writeSummaryMeasurementToLogFile()
{
  writeStatusFieldsToLogFile("summary");

  // and write out the sensor data
  for (unsigned short i = 0; i < sensorCount; i++)
  {
    // get values from the sensors
    const char *dataString = drivers[i]->getSummaryDataString();
    notify(dataString);
    fileSystemWriteCache->writeString(dataString);
    if (i < sensorCount - 1)
    {
      fileSystemWriteCache->writeString((char *)reinterpretCharPtr(F(",")));
    }
  }

  writeUserFieldsToLogFile();
  fileSystemWriteCache->endOfLine();
  return true;
}


void Datalogger::setUpCLI()
{
  cli = CommandInterface::create(Serial2, this);
  cli->setup();
}

void Datalogger::processCLI()
{
  cli->poll();
}

void Datalogger::storeSensorConfigurationIfNeedsSave()
{
  for(unsigned short i=0; i<sensorCount; i++)
  {
    if(drivers[i]->getNeedsSave())
    {
      storeSensorConfiguration(drivers[i]);
    }
  }
}

void Datalogger::notifyInvalid(){
  notify("Invalid:");
}

// void Datalogger::setRTCtimestamp(uint32 setTime){
//   settings.RTCsetTime = (unsigned long) setTime;
// }

void Datalogger::setConfiguration(cJSON *config)
{
  // TODO: use one command for all config set, rather than individual commands, only check errors if respective key found
  const cJSON* siteNameJSON = cJSON_GetObjectItemCaseSensitive(config, "siteName");
  if(siteNameJSON != NULL && cJSON_IsString(siteNameJSON) && strlen(siteNameJSON->valuestring) <= 7)
  {
    strcpy(settings.siteName, siteNameJSON->valuestring);
  } else {
    notifyInvalid();
    notify("siteName");
  }

  const cJSON* loggerNameJSON = cJSON_GetObjectItemCaseSensitive(config, "loggerName");
  if(loggerNameJSON != NULL && cJSON_IsString(loggerNameJSON) && strlen(loggerNameJSON->valuestring) <= 7)
  {
    strcpy(settings.loggerName, loggerNameJSON->valuestring);
  } else {
    notifyInvalid();
    notify("loggerName");
  }
  
  const cJSON* deploymentIdentifierJSON = cJSON_GetObjectItemCaseSensitive(config, "deploymentIdentifier");
  if(deploymentIdentifierJSON != NULL && cJSON_IsString(deploymentIdentifierJSON) && strlen(deploymentIdentifierJSON->valuestring) <= 15)
  {
    strcpy(settings.deploymentIdentifier, deploymentIdentifierJSON->valuestring);
  } else {
    notifyInvalid();
    notify("deploymentIdentifier");
  }

  const cJSON * intervalJson = cJSON_GetObjectItemCaseSensitive(config, "wakeInterval");
  if(intervalJson != NULL && cJSON_IsNumber(intervalJson) && intervalJson->valueint > 0)
  {
    settings.wakeInterval = (byte) intervalJson->valueint;
  } else {
    notifyInvalid();
    notify("wakeInterval");
  }

  const cJSON * burstNumberJson = cJSON_GetObjectItemCaseSensitive(config, "burstNumber");
  if(burstNumberJson != NULL && cJSON_IsNumber(burstNumberJson) && burstNumberJson->valueint > 0)
  {
    settings.burstNumber = (byte) burstNumberJson->valueint;
  } else {
    notifyInvalid();
    notify("burstNumber");
  }

  const cJSON * startUpDelayJson = cJSON_GetObjectItemCaseSensitive(config, "startUpDelay");
  if(startUpDelayJson != NULL && cJSON_IsNumber(startUpDelayJson) && startUpDelayJson->valueint >= 0)
  {
    settings.startUpDelay = (byte) startUpDelayJson->valueint;
  } else {
    notifyInvalid();
    notify("startUpDelay");
  }

  const cJSON * interBurstDelayJson = cJSON_GetObjectItemCaseSensitive(config, "interBurstDelay");
  if(interBurstDelayJson != NULL && cJSON_IsNumber(interBurstDelayJson) && interBurstDelayJson->valueint >= 0)
  {
    settings.interBurstDelay = (byte) interBurstDelayJson->valueint;
  } else {
    notifyInvalid();
    notify("interBurstDelay");
  }

  storeDataloggerConfiguration();
}

cJSON * Datalogger::getConfigurationJSON()
{
  cJSON* json = cJSON_CreateObject();
  cJSON_AddStringToObject(json, "UUID", getUUIDString());
  cJSON_AddStringToObject(json, "loggerName", settings.loggerName);
  cJSON_AddNumberToObject(json, "RTCsetTime", settings.RTCsetTime);
  cJSON_AddStringToObject(json, "siteName", settings.siteName);
  cJSON_AddStringToObject(json, "deploymentIdentifier", settings.deploymentIdentifier);
  cJSON_AddNumberToObject(json, "wakeInterval(min)", settings.wakeInterval);
  cJSON_AddNumberToObject(json, "startUpDelay(min)", settings.startUpDelay);
  cJSON_AddNumberToObject(json, "burstNumber", settings.burstNumber);
  cJSON_AddNumberToObject(json, "interBurstDelay(min)", settings.interBurstDelay);
  return json;
}

void Datalogger::setSensorConfiguration(char *type, cJSON *json)
{
  SensorDriver *driver = NULL;
  short typeCode = typeCodeForSensorTypeString(type);
  driver = driverForSensorTypeCode(typeCode);

  if (driver != NULL)
  {
    if (driver->configureFromJSON(json) == false)
    {
      return;
    }
    if (driver->getProtocol() == i2c)
    {
      ((I2CProtocolSensorDriver *)driver)->setWire(&WireTwo);
    }
    driver->setup();
    storeSensorConfiguration(driver);

    bool slotReplacement = false;
    for (unsigned short i = 0; i < sensorCount; i++)
    {
      if (drivers[i]->getCommonConfigurations()->slot == driver->getCommonConfigurations()->slot)
      {
        slotReplacement = true;
        SensorDriver *replacedDriver = drivers[i];
        drivers[i] = driver;
        delete (replacedDriver);
        break;
      }
    }
    if (!slotReplacement)
    {
      sensorCount = sensorCount + 1;
      SensorDriver **updatedDrivers = (SensorDriver **)malloc(sizeof(SensorDriver *) * sensorCount);
      unsigned short i = 0;
      // printFreeMemory();
      // if(sensorCount > 1)
      // {
      //   notify(drivers[i]->getCommonConfigurations()->slot);
      //   notify(driver->getCommonConfigurations()->slot);
      // }
      for (; i < sensorCount-1 && drivers[i]->getCommonConfigurations()->slot < driver->getCommonConfigurations()->slot ; i++)
      {
        updatedDrivers[i] = drivers[i];
      }

      updatedDrivers[i] = driver;
      i++;
      for (; i < sensorCount; i++)
      {
        updatedDrivers[i] = drivers[i-1];
      }
      free(drivers);
      drivers = updatedDrivers;
    }
  }
}

void Datalogger::clearSlot(unsigned short slot)
{
  bool slotConfigured = false;
  for (unsigned short i = 0; i < sensorCount; i++)
  {
    if (drivers[i]->getCommonConfigurations()->slot == slot)
    {
      slotConfigured = true;
      break;
    }
  }
  if (!slotConfigured)
  {
    notify("Slot no config");
    return;
  }

  byte empty[SENSOR_CONFIGURATION_SIZE];
  for (int i = 0; i < SENSOR_CONFIGURATION_SIZE; i++)
  {
    empty[i] = 0xFF;
  }
  writeSensorConfigurationToEEPROM(slot, empty);
  sensorCount--;

  SensorDriver **updatedDrivers = (SensorDriver **)malloc(sizeof(SensorDriver *) * sensorCount);
  int j = 0;
  for (unsigned short i = 0; i < sensorCount + 1; i++)
  {
    if (this->drivers[i]->getCommonConfigurations()->slot != slot)
    {
      updatedDrivers[j] = this->drivers[i];
      j++;
    } 
    else 
    {
      delete(this->drivers[i]);
    }
  }
  free(this->drivers);
  this->drivers = updatedDrivers;
}

cJSON *Datalogger::getSensorConfiguration(short index) // returns unprotected **
{
  return drivers[index]->getConfigurationJSON();
}

void Datalogger::setWakeInterval(int wakeInterval)
{
  settings.wakeInterval = wakeInterval;
  storeDataloggerConfiguration();
}

void Datalogger::setBurstNumber(int number)
{
  settings.burstNumber = number;
  storeDataloggerConfiguration();
}

void Datalogger::setStartUpDelay(int delay)
{
  settings.startUpDelay = delay;
  storeDataloggerConfiguration();
}

void Datalogger::setInterBurstDelay(int delay)
{
  settings.interBurstDelay = delay;
  storeDataloggerConfiguration();
}

void Datalogger::setExternalADCEnabled(bool enabled)
{
  settings.externalADCEnabled = enabled;
}

void Datalogger::setUserNote(char *note)
{
  strcpy(userNote, note);
}

void Datalogger::setUserValue(int value)
{
  userValue = value;
}

void Datalogger::toggleTraceValues()
{
  settings.debug_values = !settings.debug_values;
  storeConfiguration();
  Serial2.println(bool(settings.debug_values));
}

SensorDriver *Datalogger::getDriver(unsigned short slot)
{
  for (unsigned short i = 0; i < sensorCount; i++)
  {
    if (this->drivers[i]->getCommonConfigurations()->slot == slot)
    {
      return this->drivers[i];
    }
  }
  return NULL;
}

void Datalogger::calibrate(unsigned short slot, char *subcommand, int arg_cnt, char **args)
{

  SensorDriver *driver = getDriver(slot);
  if(driver == NULL)
  {
    notify(F("No driver"));
    return;
  }

  if (strcmp(subcommand, "init") == 0)
  {
    notify("calling init");
    driver->initCalibration();
  }
  else
  {

    driver->calibrationStep(subcommand, arg_cnt, args);
  }
}

void Datalogger::storeMode(mode_type mode)
{
  char modeStorage = 'i';
  switch (mode)
  {
  case logging:
    modeStorage = 'l';
    break;
  case deploy_on_trigger:
    modeStorage = 't';
    break;
  default:
    modeStorage = 'i';
    break;
  }
  settings.mode = modeStorage;
  storeDataloggerConfiguration();
}

void Datalogger::changeMode(mode_type mode)
{
  char message[50];
  sprintf(message, reinterpret_cast<const char *> F("->Mode %d"), mode);
  notify(message);
  this->mode = mode;
}

bool Datalogger::inMode(mode_type mode)
{
  return this->mode == mode;
}

const char * abortMessage = "**** ABORTING DEPLOYMENT *****";

bool Datalogger::deploy()
{
  // notify(F("Deploying now!"));
  notifyDebugStatus();
  if (checkDebugSystemDisabled() == false)
  {
    notify(abortMessage);
    return false;
  }

  setDeploymentTimestamp(timestamp());  // TODO: deployment should span across power cycles
  measurementCycle = 0; // reset measurement cycle count when we deploy
  // increment at start of cycle, or after?
  enterFieldLoggingMode();
  return true;
}

bool Datalogger::enterFieldLoggingMode()
{
  strcpy(loggingFolder, settings.siteName);
  fileSystem->closeFileSystem();
  initializeFilesystem();
  setSensorDebugModes(false);
  changeMode(logging);
  storeMode(logging);
  return true;
}

void Datalogger::initializeFilesystem()
{
  SdFile::dateTimeCallback(dateTime);

  fileSystem = new WaterBear_FileSystem(loggingFolder, SD_ENABLE_PIN);
  Monitor::instance()->filesystem = fileSystem;
  // debug(F("Filesystem started OK"));

  time_t setupTime = timestamp();
  char setupTS[21];
  sprintf(setupTS, "unixtime: %lld", setupTime);
  notify(setupTS);

  // example with co2, dht22, and methane sensor
  // "type,site,logger,deployment,deployed_at,uuid,time.s,time.h,battery.V,measurement,burst,dht_C,dht_RH,atlas_CO2_ppm,ch4rf_raw,ch4rf_cal,ch4_raw,ch4_cal,user_note,user_value"
  char header[200]; // adjust as necessary, statusFields + csv column headers from each driver
  const char *statusFields = "type,site,logger,deployment,deployed_at,uuid,time.s,time.h,battery.V,measurementCycle,burstCycle"; // 96 char + null
  
  strcpy(header, statusFields);
  debug(header);
  for (unsigned short i = 0; i < sensorCount; i++)
  {
    debug(i);
    debug(drivers[i]->getCSVColumnHeaders());
    strcat(header, ",");
    strcat(header, drivers[i]->getCSVColumnHeaders());
  }
  strcat(header, ",user_note,user_value");

  fileSystem->setNewDataFile(setupTime, header); // name file via epoch timestamps

  if (fileSystemWriteCache != NULL)
  {
    delete (fileSystemWriteCache);
  }
  fileSystemWriteCache = new WriteCache(fileSystem);
}

void Datalogger::powerUpSwitchableComponents()
{
  cycleSwitchablePower();

  // turn on 5v booster for exADC reference voltage, needs the delay
  // might be possible to turn off after exADC discovered, not certain.
  gpioPinOn(GPIO_PIN_3);
  
  delay(250);
  enableI2C1();
  enableI2C2();

  // debug("reset exADC");
  // Reset external ADC (if it's installed)
  delay(1); // delay > 50ns before applying ADC reset
  digitalWrite(EXADC_RESET,LOW); // reset is active low
  delay(1); // delay > 10ns after starting ADC reset
  digitalWrite(EXADC_RESET,HIGH);
  delay(100); // Wait for ADC to start up

  bool externalADCInstalled = scanI2C(&Wire, 0x2f); // use datalogger setting once method is moved to instance method
  if (externalADCInstalled)
  {
    debug(F("Setup exADC"));
    externalADC = new AD7091R();
    externalADC->configure();
    externalADC->enableChannel(0);
    externalADC->enableChannel(1);
    externalADC->enableChannel(2);
    externalADC->enableChannel(3);
  }
  else
  {
    debug(F("no exADC"));
  }

  // debug(F("Switchable components powered up"));
};

void Datalogger::powerDownSwitchableComponents() // called in stopAndAwaitTrigger
{
  //TODO: hook for sensors that need to be powered down? separate functions?
  //TODO: hook for actuators that need to be powered down?
  gpioPinOff(GPIO_PIN_3); //turn off 5v booster
  gpioPinOff(GPIO_PIN_6); //not in use currently
  i2c_disable(I2C2);
  digitalWrite(EXADC_RESET,LOW);
  // debug(F("Switchable components powered down"));
  delete externalADC;
}

void Datalogger::prepareForUserInteraction()
{
  char humanTime[26];
  time_t awakenedTime = timestamp();

  t_t2ts(awakenedTime, millis(), humanTime);
  // debug(F("Awakened by user"));
  debug(F(humanTime));

  awakenedByUser = false;
  awakeTime = awakenedTime;
}

// bool Datalogger::checkAwakeForUserInteraction(bool debugLoop)
// {
//   // Are we awake for user interaction?
//   bool awakeForUserInteraction = false;
//   if (timestamp() < awakeTime + USER_WAKE_TIMEOUT)
//   {
//     debug(F("Awake for user interaction"));
//     awakeForUserInteraction = true;
//   }
//   else
//   {
//     if (!debugLoop)
//     {
//       debug(F("Not awake for user interaction"));
//     }
//   }
//   if (!awakeForUserInteraction)
//   {
//     awakeForUserInteraction = debugLoop;
//   }
//   return awakeForUserInteraction;
// }

// bool checkTakeMeasurement(bool bursting, bool awakeForUserInteraction)
// {
//   // See if we should send a measurement to an interactive user
//   // or take a bursting measurement
//   bool takeMeasurement = false;
//   if (bursting)
//   {
//     takeMeasurement = true;
//   }
//   else if (awakeForUserInteraction)
//   {
//     unsigned long currentMillis = millis();
//     unsigned int interactiveMeasurementDelay = 1000;
//     if (currentMillis - lastMillis >= interactiveMeasurementDelay)
//     {
//       lastMillis = currentMillis;
//       takeMeasurement = true;
//     }
//   }
//   return takeMeasurement;
// }

void Datalogger::stopAndAwaitTrigger()
{
  // debug(F("Await measurement trigger"));

  // printInterruptStatus(Serial2);
  debug(F("GoingToSleep"));

  // save enabled interrupts
  int iser1, iser2, iser3;
  storeAllInterrupts(iser1, iser2, iser3);

  clearManualWakeInterrupt();
  setNextAlarmInternalRTC(settings.wakeInterval);

  // power down sensors -> function?
  for (unsigned int i = 0; i < sensorCount; i++)
  {
    drivers[i]->stop();
  }

  powerDownSwitchableComponents();
  fileSystem->closeFileSystem();
  //// filesize issue work around, maybe solved by addressing memory leak in drivers?
  // if(fileSystem->checkFileSize())
  // {
  //   notify("newfile");
  //   // not working, not sure how this is supposed to work
  //   // initializeFilesystem(); // if file size exceeded, make new file
  //   // fileSystem->closeFileSystem(); // then close it

  //   nvic_sys_reset(); // or just reset if this isn't working
  // }

  disableSwitchedPower();

  awakenedByUser = false; // Don't go into sleep mode with any interrupt state

  componentsStopMode();

  disableCustomWatchDog();
  // debug(F("disabled watchdog"));
  disableSerialLog();     // TODO
  hardwarePinsStopMode(); // switch to input mode

  clearAllInterrupts();
  clearAllPendingInterrupts();

  enableManualWakeInterrupt();    // The button, which is not powered during stop mode on v0.2 hardware
  nvic_irq_enable(NVIC_RTCALARM); // enable our RTC alarm interrupt

  enterStopMode();

  reenableAllInterrupts(iser1, iser2, iser3);
  disableManualWakeInterrupt();
  nvic_irq_disable(NVIC_RTCALARM);

  enableSerialLog();
  enableSwitchedPower();

  setupHardwarePins(); // used from setup steps in datalogger

  // debug(F("Awoke"));

  startCustomWatchDog(); // could go earlier once working reliably
  // delay( (DEFAULT_WATCHDOG_TIMEOUT_SECONDS + 5) * 1000); // to test the watchdog

  if (awakenedByUser == true)
  {
    notify(F("User interrupt"));
  }

  // We have woken from the interrupt
  // printInterruptStatus(Serial2);

  powerUpSwitchableComponents();
  
  // power up sensors -> function?
  // relocating so i2c is active for sensors requiring it
  for (unsigned int i = 0; i < sensorCount; i++)
  {
    drivers[i]->setup();
  }

  // turn components back on
  componentsBurstMode();
  fileSystem->reopenFileSystem();

  if (awakenedByUser == true)
  {
    awakeTime = timestamp();
  }

  // We need to check on which interrupt was triggered
  if (awakenedByUser)
  {
    prepareForUserInteraction();
  }
  // notify("awake");
}

void Datalogger::storeDataloggerConfiguration()
{
  writeDataloggerSettingsToEEPROM(&this->settings);
}

void Datalogger::storeSensorConfiguration(SensorDriver * driver)
{
  const configuration_bytes configurationBytes = driver->getConfigurationBytes();
  writeSensorConfigurationToEEPROM(driver->getSlot(), &configurationBytes);
}

void Datalogger::setSiteName(char *siteName)
{
  strncpy(this->settings.siteName, siteName, 8);
  storeDataloggerConfiguration();
}

void Datalogger::setDeploymentIdentifier(char *deploymentIdentifier)
{
  strncpy(this->settings.deploymentIdentifier, deploymentIdentifier, 16);
  storeDataloggerConfiguration();
}

void Datalogger::setLoggerName(char *loggerName)
{
  strncpy(this->settings.loggerName, loggerName, 8);
  storeDataloggerConfiguration();
}

void Datalogger::setDeploymentTimestamp(int timestamp)
{
  this->settings.deploymentTimestamp = timestamp;
}

const char *Datalogger::getUUIDString()
{
  return uuidString;
}

uint32 Datalogger::minMillisecondsUntilNextReading()
{
  /*
    TODO: split into two modes, asynchronous and synchronous logging
    consider that eventually a sensor may have a very long delay until ready
    or a user may want differing burst counts per sensor
    ie the DHT22 could be pinged once every minute, rather than once every 2s
  */

  uint32 minimumDelayUntilNextRequestedReading = MAX_REQUESTED_READING_DELAY; 
  uint32 maxRequestedReadingDelay = MAX_REQUESTED_READING_DELAY;
  uint32 maxDelayUntilNextAvailableReading = 0;
  // if logMode == "synchronous"
  for(int i=0; i<sensorCount; i++)
  {
    // retrieve the fastest time requested for sampling
    minimumDelayUntilNextRequestedReading = min(minimumDelayUntilNextRequestedReading, drivers[i]->millisecondsUntilNextRequestedReading());
    // retrieve the slowest response time for sampling
    maxDelayUntilNextAvailableReading = max(maxDelayUntilNextAvailableReading, drivers[i]->millisecondsUntilNextReadingAvailable());
  }
  // TODO: clean up logic for minimum delay, default is max, and compared to max, if no sensor wants to run faster, then it stays at max
  if(minimumDelayUntilNextRequestedReading == maxRequestedReadingDelay) // can't compare to #define?
  {
    minimumDelayUntilNextRequestedReading = 0;
  }

  // return max to prioritize sampling as soon as ALL sensors are ready to sample
  return max(minimumDelayUntilNextRequestedReading, maxDelayUntilNextAvailableReading);
  
  // if logMode == "asynchronous"{}
  // return min to prioritize sampling at desired speed for ONE specific sensor
  // if (maxDelayUntilNextAvailableReading == 0) // meaning all sensors have no delay between readings available
  // {
  //   return minimumDelayUntilNextRequestedReading;
  // }
  // return min(minimumDelayUntilNextRequestedReading, maxDelayUntilNextAvailableReading);
}

void Datalogger::setSensorDebugModes(bool debug)
{
  for(unsigned short i=0; i<sensorCount; i++)
  {
    drivers[i]->setDebugMode(debug);
  }
}

void Datalogger::outputLastMeasurement()
{
  Serial2.print("\n");
  for (unsigned short i = 0; i < sensorCount; i++)
  {
    Serial2.print(drivers[i]->getCSVColumnHeaders());
    Serial2.print(i < sensorCount - 1 ? "," : "\n");
  }

  for (unsigned short i = 0; i < sensorCount; i++)
  {
    Serial2.print(drivers[i]->getRawDataString());
    Serial2.print(i < sensorCount - 1 ? "," : "\n");
  }
}
