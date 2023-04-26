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

#include "switched_power.h"
#include "hardware.h"
#include "logs.h"

void setupSwitchedPower()
{
  // enable pin on switchable integrated 3v3 boost converter
  pinMode(SWITCHED_POWER_ENABLE, OUTPUT);
  digitalWrite(SWITCHED_POWER_ENABLE, LOW);
}

void enableSwitchedPower()
{
  // debug(F("Enabling switched power"));
  digitalWrite(SWITCHED_POWER_ENABLE, HIGH);
}

void disableSwitchedPower()
{
  // debug(F("Disabling switched power"));
  digitalWrite(SWITCHED_POWER_ENABLE, LOW);
}

void cycleSwitchablePower()
{
  debug(F("Cycle switched power"));
  disableSwitchedPower();
  delay(500);
  enableSwitchedPower();
  delay(500);
}