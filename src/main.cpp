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

#include <Arduino.h>
#include <string.h>

#include "datalogger.h"
#include "system/watchdog.h"
#include "system/hardware.h"
#include "utilities/i2c.h"
#include "utilities/qos.h"
#include "scratch/dbgmcu.h"
#include "version.h"
#include "system/eeprom.h"
#include "system/logs.h"

// Setup and Loop
Datalogger *datalogger;
void printWelcomeMessage(datalogger_settings_type *dataloggerSettings);
void workspace();

void setup(void)
{
  startSerial2();
  Monitor::instance()->debugToSerial = true;

  workspace();

  startCustomWatchDog();
  printWatchDogStatus();

  // disable unused components and hardware pins
  componentsAlwaysOff();
  //hardwarePinsAlwaysOff(); // TODO are we turning off I2C pins still, which is wrong

  setupInternalRTC();

  // turn on switched power to read from EEPROM
  setupSwitchedPower();
  cycleSwitchablePower();
  enableI2C1();
  delay(500);

  // debug("creating datalogger");
  datalogger_settings_type *dataloggerSettings = (datalogger_settings_type *)malloc(sizeof(datalogger_settings_type));
  Datalogger::readConfiguration(dataloggerSettings);
  datalogger = new Datalogger(dataloggerSettings);
  // debug("created datalogger");
  datalogger->setup();

  /* We're ready to go! */
  debug(F("setup done"));
  notifyDebugStatus();

  startCustomWatchDog(); // printMCUDebugStatus delays with user message, don't want watchdog to trigger

  Monitor::instance()->debugToSerial = false;

  printWelcomeMessage(dataloggerSettings);

  // TODO: I think by default the logger should always be in interactive mode on flash, is that possible?
  // Otherwise we need to check that the unit has been power cycled for debug mode after flashing
  if (datalogger->inMode(logging))
  {
    notify("Device -> log mode in 5 sec");
    notify("Enter 'i' for interactive mode");
    Serial2.print("CMD >> ");
    int start = timestamp();
    int now = start;
    while (now < start + 5)
    {
      startCustomWatchDog();
      datalogger->processCLI();
      now = timestamp();
    }
  }
  else
  {
    Serial2.print("CMD >> ");
    Serial2.flush();
  }
}

void loop(void)
{
  startCustomWatchDog();
  // printWatchDogStatus();
  checkMemory();

  datalogger->loop();
}

void printWelcomeMessage(datalogger_settings_type *dataloggerSettings)
{
  // Welcome message

   const __FlashStringHelper *welcomeMessage = F("Welcome"); //F(R"RRIV(
// ____/\\\\\\\\\________/\\\\\\\\\______/\\\\\\\\\\\__/\\\________/\\\_        
//  __/\\\///////\\\____/\\\///////\\\___\/////\\\///__\/\\\_______\/\\\_       
//   _\/\\\_____\/\\\___\/\\\_____\/\\\_______\/\\\_____\//\\\______/\\\__      
//    _\/\\\\\\\\\\\/____\/\\\\\\\\\\\/________\/\\\______\//\\\____/\\\___     
//     _\/\\\//////\\\____\/\\\//////\\\________\/\\\_______\//\\\__/\\\____    
//      _\/\\\____\//\\\___\/\\\____\//\\\_______\/\\\________\//\\\/\\\_____   
//       _\/\\\_____\//\\\__\/\\\_____\//\\\______\/\\\_________\//\\\\\______  
//        _\/\\\______\//\\\_\/\\\______\//\\\__/\\\\\\\\\\\______\//\\\_______ 
//         _\///________\///__\///________\///__\///////////________\///________


// River Restoration Intelligence and Verification  https://rriv.org
// Copyright (C) 2020  Zaven Arra  zaven.arra@gmail.com
// This program comes with ABSOLUTELY NO WARRANTY; for details type `show-warranty'.
// This is free software, and you are welcome to redistribute it
// under certain conditions; type `show-conditions' for details.
// )RRIV");

  // You are connected to Little Peep
  // Site name: TRAY
  // Hardware version: WB2.1
  // Software version: v1.3.2

  // The river is at the center.
  // Type 'help' for command list.
  // CMD >>
  // )RRIV";

  notify(welcomeMessage);
  char buffer[100];
  sprintf(buffer, "Site name: %s", dataloggerSettings->siteName);
  notify(buffer);
  sprintf(buffer, "Software version: %s", WATERBEAR_FIRMWARE_VERSION);
  notify(buffer);

  const __FlashStringHelper *handoff = F(R"RRIV(
Type 'help' for command list.
)RRIV");
  notify(handoff);
}

// space to work our development details


void workspace()
{
  // notify(sizeof(long long));
  // notify(sizeof(sone));
  // exit(0);
}