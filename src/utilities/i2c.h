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
#include <Wire_slave.h> // Communicate with I2C/TWI devices

void i2cSendTransmission(byte i2cAddress, byte registerAddress, const void * data, int numBytes);
void i2cError(int transmissionCode);
void scanI2C(TwoWire *wire);
bool scanI2C(TwoWire *wire, int searchAddress);
void enableI2C1();
void enableI2C2();

