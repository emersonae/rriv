#include "datalogger.h"
#include <Cmd.h>
#include "system/measurement_components.h"
#include "system/monitor.h"
#include "system/watchdog.h"
#include "system/command.h"
#include "sensors/sensor.h"
#include "sensors/sensor_map.h"
#include "sensors/sensor_types.h"
#include "utilities/i2c.h"
#include "utilities/qos.h"
#include "utilities/STM32-UID.h"

// static method to read configuration from EEPROM
void Datalogger::readConfiguration(datalogger_settings_type * settings)
{
  byte* buffer = (byte *) malloc(sizeof(byte) * sizeof(datalogger_settings_type));
  for(unsigned short i=0; i < sizeof(datalogger_settings_type); i++)
  {
    short address = EEPROM_DATALOGGER_CONFIGURATION_START + i;
    buffer[i] = readEEPROM(&Wire, EEPROM_I2C_ADDRESS, address);
  }

  memcpy(settings, buffer, sizeof(datalogger_settings_type));

  // apply defaults
  if(settings->burstNumber == 0 || settings->burstNumber > 20)
  {
    settings->burstNumber = 1;
  }
  if(settings->interBurstDelay > 300)
  {
    settings->interBurstDelay = 0;
  }

  settings->debug_values = true;
}


Datalogger::Datalogger(datalogger_settings_type * settings)
{
  powerCycle = true;
  debug("creating datalogger");
  debug("got mode");
  debug(settings->mode);

  // defaults
  if(settings->interval < 1) 
  {
    debug("Setting interval to 1 by default");
    settings->interval = 1;
  }

  memcpy(&this->settings, settings, sizeof(datalogger_settings_type));

  switch(settings->mode)
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
      strcpy(loggingFolder, reinterpret_cast<const char *> F("NOT_DEPLOYED"));
      break;
  }
}


void Datalogger::setup()
{
  startCustomWatchDog();

  setupHardwarePins();
  setupSwitchedPower();
  powerUpSwitchableComponents();
  
  bool externalADCInstalled = scanIC2(&Wire, 0x2f);
  settings.externalADCEnabled = externalADCInstalled;

  setupManualWakeInterrupts();
  disableManualWakeInterrupt(); // don't respond to interrupt during setup
  clearManualWakeInterrupt();

  clearAllAlarms(); // don't respond to alarms during setup

  //initBLE();

  unsigned char uuid[UUID_LENGTH];
  readUniqueId(uuid);
  decodeUniqueId(uuid, uuidString, UUID_LENGTH);

  buildDriverSensorMap();
  loadSensorConfigurations();
  initializeFilesystem();
  setUpCLI();
}


void Datalogger::loop()
{

  if(inMode(deploy_on_trigger)){
    deploy();
    goto SLEEP;
    return;
  }

  if(inMode(logging))
  {
    if(powerCycle)
    {
      debug("Powercycle");
      deploy();
      goto SLEEP;
    }

    if(shouldExitLoggingMode())
    {
      notify("Should exit logging mode");
      changeMode(interactive);
      return;
    }

    if(shouldContinueBursting())
    {
      measureSensorValues();
      writeMeasurementToLogFile();
    } 
    else 
    {
      completedBursts++;
      if(completedBursts < settings.burstNumber)
      {
        debug(F("do another burst"));
        debug(settings.burstNumber);
        debug(completedBursts);
        delay(settings.interBurstDelay * 1000);
        initializeBurst();
        return;
      }

      // go to sleep
      fileSystemWriteCache->flushCache();
      SLEEP: stopAndAwaitTrigger();
      initializeMeasurementCycle();
      return;
    }
    return;
  }

  processCLI();

  // not currently used
  // TODO: do we want to cache dirty config to reduce writes to EEPROM?
  // if (configurationIsDirty())
  // {
  //   debug("Configuration is dirty");
  //   storeConfiguration();
  //   stopLogging();
  // }

  if (inMode(logging) || inMode(deploy_on_trigger))
  {
    // processCLI may have moved logger into a deployed mode
    goto SLEEP;
  }
  else if (inMode(interactive))
  {
    if(interactiveModeLogging){
      if(timestamp() > lastInteractiveLogTime + 5)
      {
        notify(F("interactive log"));
        measureSensorValues(false);
        writeMeasurementToLogFile();
        lastInteractiveLogTime = timestamp();
      }
    }
  }
  else if (inMode(debugging))
  {
    measureSensorValues(false);
    writeMeasurementToLogFile();
    delay(5000); // this value could be configurable, also a step / read from CLI is possible
  }
  else
  {
    // invalid mode!
    notify(F("Invalid Mode!"));
    notify(mode);
    mode = interactive;
    delay(1000);
  }

  powerCycle = false;
}

#define TOTAL_SLOTS 4

void Datalogger::loadSensorConfigurations()
{

  // load sensor configurations from EEPROM and count them
  sensorCount = 0;
  generic_config * configs[EEPROM_TOTAL_SENSOR_SLOTS];
  for(int i=0; i<EEPROM_TOTAL_SENSOR_SLOTS; i++)
  {
    debug("reading slot");
    generic_config * sensorConfig = (generic_config *) malloc(sizeof(generic_config));

    readEEPROMObject(EEPROM_DATALOGGER_SENSORS_START + i * EEPROM_DATALOGGER_SENSOR_SIZE, sensorConfig, EEPROM_DATALOGGER_SENSOR_SIZE);

    debug(sensorConfig->common.sensor_type);
    if(sensorConfig->common.sensor_type <= MAX_SENSOR_TYPE)
    {
      debug("found configured sensor");
      sensorCount++;
    }
    sensorConfig->common.slot = i;
    configs[i] = sensorConfig;
  }
  if(sensorCount == 0)
  {
    debug("no sensor configurations found");
  }

  // construct the drivers
  debug("construct drivers");
  drivers = (SensorDriver**) malloc(sizeof(SensorDriver*) * sensorCount);
  int j = 0;
  for(int i=0; i<EEPROM_TOTAL_SENSOR_SLOTS; i++)
  {
    if(configs[i]->common.sensor_type > MAX_SENSOR_TYPE)
    {
      debug("no sensor");
      continue;
    }

    debug("getting driver for sensor type");
    debug(configs[i]->common.sensor_type);
    SensorDriver * driver  = driverForSensorType(configs[i]->common.sensor_type);
    debug("got sensor driver");
    checkMemory();

    drivers[j] = driver;
    j++;
    switch(driver->getProtocol()){
      case analog:
        ((AnalogSensorDriver*) driver)->setup();
        break;
      case i2c:
        ((I2CSensorDriver*) driver)->setup(&WireTwo);
        break;
      default:
        break;
    }
    debug("configure sensor driver");
    driver->configure(configs[i]);  //pass configuration struct to the driver
    debug("configured sensor driver");
  }

  for(int i=0; i<EEPROM_TOTAL_SENSOR_SLOTS; i++)
  {
    free(configs[i]);
  }

  // set up bookkeeping for dirty configurations
  if(dirtyConfigurations != NULL)
  {
    free(dirtyConfigurations);
  }
  dirtyConfigurations = (bool *) malloc(sizeof(bool) * (sensorCount + 1));
}


void Datalogger::startLogging()
{
  interactiveModeLogging = true;
}


void Datalogger::stopLogging()
{
  interactiveModeLogging = false;
}


bool Datalogger::shouldExitLoggingMode()
{
  if( Serial2.peek() != -1){
    //attempt to process the command line
    for(int i=0; i<10; i++)
    {
      processCLI();
    }
    if(inMode(interactive))
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
  for(int i=0; i<sensorCount; i++)
  {
    notify(F("check sensor burst"));
    notify(i);
    if(!drivers[i]->burstCompleted())
    {
      return true;
    }
  }
  return false;
}


void Datalogger::initializeBurst()
{
  for(int i=0; i<sensorCount; i++)
  {
    drivers[i]->initializeBurst();
  }
}


void Datalogger::initializeMeasurementCycle()
{
  notify(F("setting base time"));
  currentEpoch = timestamp();
  offsetMillis = millis();

  initializeBurst();

  completedBursts = 0;

  notify(F("Waiting for start up delay"));
  delay(settings.startUpDelay);
  
}


void Datalogger::measureSensorValues(bool performingBurst)
{
  if(settings.externalADCEnabled)
  {
    // get readings from the external ADC
    debug("converting enabled channels call");
    externalADC->convertEnabledChannels();
    debug("converted enabled channels");
  }

  for(int i=0; i<sensorCount; i++){
    if(drivers[i]->takeMeasurement())
    {
      if(performingBurst) 
      {
        drivers[i]->incrementBurst();  // burst bookkeeping
      }
    }
  }
}


void Datalogger::writeStatusFieldsToLogFile()
{
  notify(F("Write status fields"));

  // Fetch and Log time from DS3231 RTC as epoch and human readable timestamps
  uint32 currentMillis = millis();
  double currentTime = (double)currentEpoch + (((double)currentMillis - offsetMillis) / 1000);

  char currentTimeString[20];
  char humanTimeString[20];
  sprintf(currentTimeString, "%10.3f", currentTime); // convert double value into string
  t_t2ts(currentTime, currentMillis-offsetMillis, humanTimeString);        // convert time_t value to human readable timestamp

  // TODO: only do this once
  char deploymentUUIDString[2*16 + 1];
  for (short i = 0; i < 16; i++)
  {
    sprintf(&deploymentUUIDString[2 * i], "%02X", (byte)settings.deploymentIdentifier[i]);
  }
  deploymentUUIDString[2*16] = '\0';

  fileSystemWriteCache->writeString(settings.siteName);
  fileSystemWriteCache->writeString((char *) ",");
  fileSystemWriteCache->writeString(deploymentUUIDString);
  fileSystemWriteCache->writeString((char *) ",");
  char buffer[10]; sprintf(buffer, "%ld,", settings.deploymentTimestamp); fileSystemWriteCache->writeString(buffer);  
  fileSystemWriteCache->writeString(uuidString);
  fileSystemWriteCache->writeString((char *)",");
  fileSystemWriteCache->writeString(currentTimeString);
  fileSystemWriteCache->writeString((char *)",");
  fileSystemWriteCache->writeString(humanTimeString);
  fileSystemWriteCache->writeString((char *)",");
  
}


void Datalogger::debugValues(char * buffer)
{
  if(settings.debug_values)
  {
    notify(buffer);
  }
}


bool Datalogger::writeMeasurementToLogFile()
{
  writeStatusFieldsToLogFile();

  // write out the raw battery reading
  int batteryValue = analogRead(PB0);
  char buffer[100];
  sprintf(buffer, "%d,", batteryValue);
  fileSystemWriteCache->writeString(buffer);   debugValues(buffer);

  // and write out the sensor data
  debug(F("Write out sensor data"));
  for(int i=0; i<sensorCount; i++)
  {
    // get values from the sensor
    char * dataString = drivers[i]->getDataString();
    fileSystemWriteCache->writeString(dataString);   debugValues(dataString);
    if(i < sensorCount - 1)
    {
      fileSystemWriteCache->writeString( (char *) reinterpretCharPtr(F(",")));   debugValues( (char * )reinterpretCharPtr(F(",")));
    }
  }
  sprintf(buffer, ",%s,", userNote);
  fileSystemWriteCache->writeString(buffer);   debugValues(buffer);
  if(userValue != INT_MIN){
    sprintf(buffer, "%d", userValue);
    fileSystemWriteCache->writeString(buffer);   debugValues(buffer);
  }
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


// not currently used
// bool Datalogger::configurationIsDirty()
// {
//   for(int i=0; i<sensorCount+1; i++)
//   {
//     if(dirtyConfigurations[i])
//     {
//       return true;
//     }
//   } 

//   return false;
// }

// not curretnly used
// void Datalogger::storeConfiguration()
// {
//   for(int i=0; i<sensorCount+1; i++)
//   {
//     if(dirtyConfigurations[i]){
//       //store this config block to EEPROM
//     }
//   }
// }

void Datalogger::getConfiguration(datalogger_settings_type * dataloggerSettings)
{
  memcpy(dataloggerSettings, &settings, sizeof(datalogger_settings_type));
}


void Datalogger::setSensorConfiguration(char * type, cJSON * json)
{
  if(strcmp(type, "generic_analog") == 0) // make generic for all types
  {
    SensorDriver * driver = new GenericAnalog();
    driver->configureFromJSON(json);
    generic_config configuration = driver->getConfiguration();
    storeSensorConfiguration(&configuration);

    notify(F("updating slots"));
    bool slotReplacement = false;
    notify(sensorCount);
    for(int i=0; i<sensorCount; i++){
      if(drivers[i]->getConfiguration().common.slot == driver->getConfiguration().common.slot)
      {
        slotReplacement = true;
        notify(F("slot replacement"));
        notify(i);
        SensorDriver * replacedDriver = drivers[i];
        drivers[i] = driver;
        notify("deleting");
        delete(replacedDriver);
        notify(F("OK"));
        break;
      }
    }
    if(!slotReplacement)
    {
      sensorCount = sensorCount + 1;
      SensorDriver ** driverAugmentation = (SensorDriver**) malloc(sizeof(SensorDriver*) * sensorCount);
      for(int i=0; i<sensorCount-2; i++)
      {
        driverAugmentation[i] = drivers[i];
      }
      driverAugmentation[sensorCount-1] = driver;
      free(drivers);
      drivers = driverAugmentation;
    }
    notify(F("OK"));

  }
}

void Datalogger::clearSlot(unsigned short slot)
{
  byte empty[SENSOR_CONFIGURATION_SIZE];
  for(int i=0; i<SENSOR_CONFIGURATION_SIZE; i++)
  {
    empty[i] = 0xFF;
  }
  writeSensorConfigurationToEEPROM(slot, empty);
  sensorCount--;

  SensorDriver ** updatedDrivers = (SensorDriver**) malloc(sizeof(SensorDriver*) * sensorCount);
  int j=0;
  for(int i=0; i<sensorCount+1; i++){
    generic_config configuration = this->drivers[i]->getConfiguration();
    if(configuration.common.slot != slot)
    {
      updatedDrivers[j] = this->drivers[i];
      j++;
    }
    else
    {
      delete(this->drivers[i]);
    }
  }
}


cJSON ** Datalogger::getSensorConfigurations() // returns unprotected **
{
  cJSON ** sensorConfigurationsJSON = (cJSON **) malloc(sizeof(cJSON *) * sensorCount);
  for(int i=0; i<sensorCount; i++)
  {
    sensorConfigurationsJSON[i] = drivers[i]->getConfigurationJSON();
  }
  return sensorConfigurationsJSON;
}


void Datalogger::setInterval(int interval)
{
  settings.interval = interval;
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


void Datalogger::setIntraBurstDelay(int delay)
{
  settings.interBurstDelay = delay;
  storeDataloggerConfiguration();
}


void Datalogger::setExternalADCEnabled(bool enabled)
{
  settings.externalADCEnabled = enabled;
}


void Datalogger::setUserNote(char * note)
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


SensorDriver * Datalogger::getDriver(unsigned short slot)
{
  for(int i=0; i<sensorCount; i++)
  {
    if(this->drivers[i]->getConfiguration().common.slot == slot)
    {
      return this->drivers[i];
    }
  }
  return NULL;
}


void Datalogger::calibrate(unsigned short slot, char * subcommand, int arg_cnt, char ** args)
{
  SensorDriver * driver = getDriver(slot);
  if(strcmp(subcommand, "init") == 0)
  {
    driver->initCalibration();
  }
  else
  {
    notify(args[0]);
    driver->calibrationStep(subcommand, atoi(args[0]));
  }
}


void Datalogger::storeMode(mode_type mode)
{
  char modeStorage = 'i';
  switch(mode){
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
  sprintf(message, reinterpret_cast<const char *> F("Moving to mode %d"), mode);
  notify(message);
  this->mode = mode;
}

bool Datalogger::inMode(mode_type mode)
{
  return this->mode == mode;
}


void Datalogger::deploy()
{
  notify(F("Deploying now!"));

  setDeploymentIdentifier();
  setDeploymentTimestamp(timestamp());
  strcpy(loggingFolder, settings.siteName);
  fileSystem->closeFileSystem(); 
  initializeFilesystem();
  changeMode(logging);
  storeMode(logging);
  powerCycle = false; // not a powercycle loop
}


void Datalogger::initializeFilesystem()
{
  SdFile::dateTimeCallback(dateTime);

  fileSystem = new WaterBear_FileSystem(loggingFolder, SD_ENABLE_PIN);
  Monitor::instance()->filesystem = fileSystem;
  debug(F("Filesystem started OK"));

  time_t setupTime = timestamp();
  char setupTS[21];
  sprintf(setupTS, "unixtime: %lld", setupTime);
  notify(setupTS);


  char header[200];
  const char * statusFields = "site,deployment,deployed_at,uuid,time.s,time.h,battery.V";
  strcpy(header, statusFields);
  debug(header);
  for(int i=0; i<sensorCount; i++){
    debug(i);
    debug(drivers[i]->getCSVColumnNames());
    strcat(header, ",");
    strcat(header, drivers[i]->getCSVColumnNames());
  }
  strcat(header, ",user_note,user_value");

  fileSystem->setNewDataFile(setupTime, header); // name file via epoch timestamps

  if(fileSystemWriteCache != NULL)
  {
    delete(fileSystemWriteCache);
  }
  debug("make a new write cache");
  fileSystemWriteCache = new WriteCache(fileSystem);
}


void Datalogger::powerUpSwitchableComponents()  
{
  cycleSwitchablePower();
  delay(500);
  enableI2C1();

  debug("resetting for exADC");
  delay(1); // delay > 50ns before applying ADC reset
  digitalWrite(PC5,LOW); // reset is active low
  delay(1); // delay > 10ns after starting ADC reset
  digitalWrite(PC5,HIGH);
  delay(100); // Wait for ADC to start up
  
  bool externalADCInstalled = scanIC2(&Wire, 0x2f); // use datalogger setting once method is moved to instance method
  if(externalADCInstalled)
  {
    debug(F("Set up external ADC"));
    externalADC = new AD7091R();
    externalADC->configure();
    externalADC->enableChannel(0);
    externalADC->enableChannel(1);
    externalADC->enableChannel(2);
    externalADC->enableChannel(3);
  } else {
    debug(F("external ADC not installed"));
  }
  
  debug(F("Switchable components powered up"));
}

void Datalogger::powerDownSwitchableComponents() // called in stopAndAwaitTrigger
{
  //TODO: hook for sensors that need to be powered down?
  i2c_disable(I2C2);
  debug(F("Switchable components powered down"));
}


void Datalogger::prepareForUserInteraction()
{
  char humanTime[26];
  time_t awakenedTime = timestamp();

  t_t2ts(awakenedTime, millis(), humanTime);
  debug(F("Awakened by user"));
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
  debug(F("Await measurement trigger"));

  if (Clock.checkIfAlarm(1))
  {
    debug(F("Alarm 1"));
  }

  printInterruptStatus(Serial2);
  debug(F("Going to sleep"));

  // save enabled interrupts
  int iser1, iser2, iser3;
  storeAllInterrupts(iser1, iser2, iser3);

  clearManualWakeInterrupt();
  setNextAlarmInternalRTC(settings.interval); 


  powerDownSwitchableComponents();
  fileSystem->closeFileSystem(); // close file, filesystem
  disableSwitchedPower();

  awakenedByUser = false; // Don't go into sleep mode with any interrupt state

  componentsStopMode();

  disableCustomWatchDog();
  debug(F("disabled watchdog"));
  disableSerialLog(); // TODO
  hardwarePinsStopMode(); // switch to input mode
  
  clearAllInterrupts();
  clearAllPendingInterrupts();

  enableManualWakeInterrupt(); // The button, which is not powered during stop mode on v0.2 hardware
  nvic_irq_enable(NVIC_RTCALARM); // enable our RTC alarm interrupt

  enterStopMode();
  //enterSleepMode();

  reenableAllInterrupts(iser1, iser2, iser3);
  disableManualWakeInterrupt();
  nvic_irq_disable(NVIC_RTCALARM);  

  enableSerialLog(); 
  enableSwitchedPower();
  setupHardwarePins(); // used from setup steps in datalogger

  debug(F("Awakened by interrupt"));

  startCustomWatchDog(); // could go earlier once working reliably
  // delay( (WATCHDOG_TIMEOUT_SECONDS + 5) * 1000); // to test the watchdog


  if(awakenedByUser == true)
  {
    debug(F("USER TRIGGERED INTERRUPT"));
  }

  // We have woken from the interrupt
  // printInterruptStatus(Serial2);

  powerUpSwitchableComponents();
  // turn components back on
  componentsBurstMode();
  fileSystem->reopenFileSystem();


  if(awakenedByUser == true)
  {
    awakeTime = timestamp();
  }

  // We need to check on which interrupt was triggered
  if (awakenedByUser)
  {
    prepareForUserInteraction();
  }
}

void Datalogger::storeDataloggerConfiguration()
{
  writeDataloggerSettingsToEEPROM(&this->settings);
}

void Datalogger::storeSensorConfiguration(generic_config * configuration){
  notify(F("Storing sensor configuration"));
  // notify(configuration->common.slot);
  // notify(configuration->common.sensor_type);
  writeSensorConfigurationToEEPROM(configuration->common.slot, configuration);
}

void Datalogger::setSiteName(char * siteName)
{
  strcpy(this->settings.siteName, siteName);
  storeDataloggerConfiguration();
}

void Datalogger::setDeploymentIdentifier()
{
  byte uuidNumber[16];
  // TODO need to generate this UUID number
  // https://www.cryptosys.net/pki/Uuid.c.html
  memcpy(this->settings.deploymentIdentifier, uuidNumber, 16);
  storeDataloggerConfiguration();
}

void Datalogger::setDeploymentTimestamp(int timestamp)
{
  this->settings.deploymentTimestamp = timestamp;
}
