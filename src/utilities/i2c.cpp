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

#include "i2c.h"
#include "system/hardware.h"
#include "system/logs.h"
#include "utilities/i2c.h"

void i2cError(int transmissionCode)
{
  switch(transmissionCode){
    case SUCCESS:
      debug(F("i2c success"));
      break;
    case EDATA:
      debug(F("i2c data error"));
      break;
    case ENACKADDR:
      debug(F("i2c address not ack'd"));
      break;
    case ENACKTRNS:
      debug(F("i2c transmission not ack;d"));
      break;
    case EOTHER:
      debug(F("i2c error: other"));
      break;
    default:
      debug(F("i2c: unknown response code"));
      break;
  }
}

void i2cSendTransmission(byte i2cAddress, byte registerAddress, const void * data, int numBytes)
{
  // char debugMessage[100];
  // sprintf(debugMessage, "i2c %X %X %d", i2cAddress, registerAddress, numBytes);
  // debug(debugMessage);

  short rval = -1;
  while (rval != 0)
  {
    Wire.beginTransmission(i2cAddress);
    Wire.write(registerAddress);

    for(int i=numBytes-1; i>=0; i--){ // correct order
      Wire.write( ((byte *) data) +i, 1);
    }

    rval = Wire.endTransmission();

    if(rval != 0){
      i2cError(rval);
      delay(1000); // give it a chance to fix itself
      i2c_bus_reset(I2C1);            // consider i2c reset?
      i2c_bus_reset(I2C2);
    }
  }
}

void scanI2C(TwoWire *wire)
{
  scanI2C(wire, -1);
}

bool scanI2C(TwoWire *wire, int searchAddress)
{
  // Serial.println("Scanning");
  byte error, address;
  int nDevices;
  nDevices = 0;
  bool found = false;
  for (address = 1; address < 127; address++)
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    wire->beginTransmission(address);
    error = wire->endTransmission(); 
    if (error == 0)
    {
      Serial.print(F("I2C 0x"));
      if (address < 16)
        Serial.print(F("0"));
      Serial.println(address, HEX);
      if(address == searchAddress)
      {
        found = true;
      }
      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print(F("error 0x"));
      if (address < 16)
        Serial.print(F("0"));
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println(F("No I2C devices found"));

  return found;
}

void enableI2C1()
{
  i2c_disable(I2C1);
  i2c_master_enable(I2C1, 0, 0);
  debug(F("Enabled I2C1"));

  // i2c_bus_reset(I2C1); // might not be necessary
  // delay(500);
  // debug(F("Reset I2C1"));s

  WireOne.begin();
  delay(250);
  // debug(F("Began TwoWire 1"));
  
  // debug(F("Scanning 1"));
  scanI2C(&Wire);
}

void enableI2C2()
{
  i2c_disable(I2C2);
  i2c_master_enable(I2C2, 0, 0);
  debug(F("Enabled I2C2"));

  // i2c_bus_reset(I2C2); // might not be necessary
  // delay(500);
  // debug(F("Reset I2C2"));

  WireTwo.begin();
  delay(250);
  // debug(F("Began TwoWire 2"));

  // debug(F("Scanning 2"));
  scanI2C(&WireTwo);
}