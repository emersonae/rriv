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

#ifndef WATERBEAR_CONTROL
#define WATERBEAR_CONTROL

#include <Arduino.h>
// #include "Adafruit_BluefruitLE_UART.h"
// #include "Adafruit_BluefruitLE_SPI.h"
#include "DS3231.h"
#include "time.h"
#include "datalogger.h"


// Forward declaration of class
class Datalogger;

#define WT_CONTROL_NONE 0
#define WT_CONTROL_CONFIG 1000 // displays raw readings, add flags? rename "control"?
#define WT_CONTROL_CAL_DRY 1001
#define WT_CONTROL_CAL_LOW 1002
#define WT_CONTROL_CAL_HIGH 1003
#define WT_SET_RTC 1004
#define WT_DEPLOY 1005

#define WT_DEBUG_VAlUES 1006 // display all logged data ~1/s, add flags?
#define WT_CLEAR_MODES 1007

#define WT_CAL_TEMP 1008 // display temp readings ~1/s || include as flag for config/debug?
#define WT_TEMP_CAL_LOW 1009 // >WT_CAL_LOW:x.x< low C temp from external thermometer
#define WT_TEMP_CAL_HIGH 1010 // >WT_CAL_HIGH:xxx.x< high C temp from external thermometer

#define WT_USER_VALUE 1011 // serial input user value
#define WT_USER_NOTE 1012 // serial input user note
#define WT_USER_INPUT 1013 // serial input user value & note

// mode to report which sensors are calibrated?
// log when conductivity sensor was calibrated

extern short controlFlag; // for WT_CONTROL_CONFIG

class CommandInterface
{
  private:
    /*Stream *mySerial;
    HardwareSerial * hs;
    Adafruit_BluefruitLE_UART * ble;
    */

  public:

    // TODO: rewrite CommandInterface as OOP class
    static CommandInterface * create(HardwareSerial &port, Datalogger * datalogger);
    static CommandInterface * instance();

    int state;

    CommandInterface(HardwareSerial &port, Datalogger * datalogger);

    void setup();
    void poll();

    int processControlCommands(Stream * myStream, Datalogger * datalogger);
    int processControlCommands(HardwareSerial &port, Datalogger * datalogger);
    // int processControlCommands(Adafruit_BluefruitLE_UART &ble, Datalogger * datalogger);
    // int processControlCommands(Adafruit_BluefruitLE_SPI &ble, Datalogger * datalogger);

    bool ready(Stream * myStream);
    bool ready(HardwareSerial &port);
    // bool ready(Adafruit_BluefruitLE_UART &ble);
    // bool ready(Adafruit_BluefruitLE_SPI &ble);

    // time_t timestamp();
    // void setTime(time_t toSet);
    // void t_t2ts(time_t epochTS, char *humanTime); //Epoch TS to yyyy/mm/dd dow hh:mm:ss zzz

#if SOFTWARE_SERIAL_AVAILABLE
    void processControlCommands(SoftwareSerial &port);
#endif

    // cli functions
    void _setSiteName(char * siteName);
    void _setLoggerName(char * loggerName);
    void _setDeploymentIdentifier(char * deploymentIdentifier);
    void _setWakeInterval(int size);
    void _setBurstSize(int size);
    void _setBurstNumber(int number);
    void _setStartUpDelay(int number);
    void _setInterBurstDelay(int number);
  
    void _setUserNote(char * note);
    void _setUserValue(int value);

    void _getConfig();
    void _setConfig(char * config);
    void _setSlotConfig(char * config);
    void _clearSlot(int slot);

    void _getRTC();
    // void _getRTC(uint32 currentTimestamp);
    void _setRTC(uint32 setTimestamp);

    void _deployNow();
    void _switchToInteractiveMode();

    void _calibrate(int slot, char * subcommand, int arg_cnt, char ** args);

    void _toggleDebug();
    void _startLogging();
    void _stopLogging();
    void _testMeasurementCycle();
    void _go();
    void _reloadSensorConfigurations();
    void _enterStop();

    void _help();

    void _gpiotest(int pin); // toggles gpio pin state (low/high)

  private:
    Datalogger * datalogger;
    void * lastCommandPayload;
    bool lastCommandPayloadAllocated = false;
};

#endif