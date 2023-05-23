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

#ifndef WATERBEAR_DATALOGGER
#define WATERBEAR_DATALOGGER

#include <Wire_slave.h> // Communicate with I2C/TWI devices
#include <SPI.h>
#include "SdFat.h"

#include "configuration.h"
#include "utilities/utilities.h"

// #include "system/ble.h"
#include "system/clock.h"
#include "system/command.h"
#include "system/eeprom.h"
#include "system/filesystem.h"
#include "system/hardware.h"
#include "system/interrupts.h"
#include "system/low_power.h"
#include "system/monitor.h"
#include "system/switched_power.h"
#include "system/adc.h"
#include "system/write_cache.h"

#include "sensors/sensor.h"

#define DEPLOYMENT_IDENTIFIER_LENGTH 16

// 64 bytes max, one configuration_partition_bytes
// Currently there are 6 bytes unused
typedef struct datalogger_settings { 
    char deploymentIdentifier[16]; // 16 bytes
    char loggerName[8]; // 8 bytes
    char siteName[8]; // 8 bytes
    unsigned long RTCsetTime; // 8 bytes
    unsigned long deploymentTimestamp; // 8 bytes
    char unused[6]; // 6 bytes unused
    unsigned short wakeInterval;  // 2 bytes minutes
    unsigned short startUpDelay; // 2 bytes minutes
    unsigned short burstNumber; // 2 bytes
    unsigned short interBurstDelay; // 2 bytes minutes
    char mode;       // 1 byte i(interactive), d(debug), l(logging), t(deploy on trigger)
    byte externalADCEnabled : 1;
    byte debug_values : 1;
    byte withold_incomplete_readings : 1; // only publish complete readings, default to withold.
    byte log_raw_data : 1;
    byte debug_to_file: 1;
    byte continuous_power: 1;
    byte reserved2 : 2;
} datalogger_settings_type;

typedef enum mode { interactive, debugging, logging, deploy_on_trigger } mode_type;

// Forward declaration of class
class CommandInterface;

class Datalogger
{

public:
    
    // sensors
    unsigned short sensorCount = 0;
    bool * dirtyConfigurations = NULL;      // configuration change tracking
    short * sensorTypes = NULL;
    void ** sensorConfigurations = NULL;
    SensorDriver ** drivers = NULL;
    datalogger_settings_type settings;

    static void readConfiguration(datalogger_settings_type * settings);
    Datalogger(datalogger_settings_type * settings);

    void setup();
    void loop();

    void changeMode(mode_type mode);
    bool inMode(mode_type mode);
    void storeMode(mode_type mode);
    bool deploy();
    bool enterFieldLoggingMode();

    void processCLI();

    // settings
    void setSiteName(char * siteName);
    void setDeploymentIdentifier(char * deploymentIdentifier);
    void setLoggerName(char * loggerName);
    void setDeploymentTimestamp(int timestamp);

    void setWakeInterval(int interval);
    void setBurstSize(int size);
    void setBurstNumber(int number);
    void setStartUpDelay(int delay);
    void setInterBurstDelay(int delay);

    // void setRTCtimestamp(uint32 setTime);

    void setConfiguration(cJSON * config);
    cJSON * getConfigurationJSON();
    cJSON * getSensorConfiguration(short index);

    void setSensorConfiguration(char * type, cJSON * json);
    void clearSlot(unsigned short slot);
    void storeSensorConfigurationIfNeedsSave();

    void calibrate(unsigned short slot, char * subcommand, int arg_cnt, char ** args);
    void setExternalADCEnabled(bool enabled);

    void setUserNote(char * note);
    void setUserValue(int value);

    void toggleTraceValues();
    void stopLogging();
    void startLogging();
    void testMeasurementCycle();

    const char * getUUIDString();

    void reloadSensorConfigurations(); // for dev & debug
    void stopAndAwaitTrigger(); // public for dev & debug

private:
    // modules
    WaterBear_FileSystem *fileSystem;
    WriteCache * fileSystemWriteCache = NULL;

    // state
    char uuidString[25]; // 2 * UUID_LENGTH + 1
    mode_type mode = interactive;
    bool powerCycle = true;
    bool interactiveModeLogging = false;
    time_t currentEpoch;
    uint32 offsetMillis;
    char loggingFolder[26];
    unsigned short completedBursts;
    unsigned short measurementCycle; 
    int awakeTime;
    uint32 burstCycleStartMillis;

    // user
    char userNote[100] = "\0";
    int userValue = INT_MIN;
    int lastInteractiveLogTime = 0;

    void loadSensorConfigurations();
    bool shouldExitLoggingMode();
    void measureSensorValues(bool performingBurst = true);
    bool writeRawMeasurementToLogFile();
    bool writeSummaryMeasurementToLogFile();
    void writeDebugFieldsToLogFile();
    bool configurationIsDirty();
    void storeConfiguration();
    void initializeBurst();
    bool shouldContinueBursting();
    bool processReadingsCycle();

    // CLI
    CommandInterface * cli;
    
    void setUpCLI();

    // utility
    void writeStatusFieldsToLogFile(const char * type);
    void writeUserFieldsToLogFile();
    void initializeMeasurementCycle();
    void outputLastMeasurement();

    void storeDataloggerConfiguration();
    void storeSensorConfiguration(SensorDriver * driver);

    uint32 minMillisecondsUntilNextReading();
    void sleepMCU(uint32 milliseconds);
 
    // run loop
    void initializeFilesystem();
    // void stopAndAwaitTrigger();
    void writeStatusFields(const char * type);
    void prepareForUserInteraction();
    void powerUpSwitchableComponents();
    void powerDownSwitchableComponents();

    // utility
    SensorDriver * getDriver(unsigned short slot);

    // debugging
    void debugValues(char * buffer);
    void setSensorDebugModes(bool debug);
    void notifyInvalid();
};
#endif