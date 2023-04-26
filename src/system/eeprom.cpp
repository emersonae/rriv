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

#include "eeprom.h"
#include "monitor.h"
#include "utilities/STM32-UID.h"
#include "utilities/i2c.h"
#include "system/logs.h"

void writeEEPROM(TwoWire * wire, int deviceaddress, short eeaddress, byte data )
{
  i2cSendTransmission(deviceaddress, eeaddress, &data, 1);
  delay(5);
}

byte readEEPROM(TwoWire * wire, int deviceaddress, short eeaddress )
{
  byte rdata = 0xFF;
  i2cSendTransmission(deviceaddress, eeaddress, 0, 0);
  delay(5);

  wire->requestFrom(deviceaddress,1);
  // short numBytes = wire->requestFrom(deviceaddress,1);
  // char debugMessage[100];
  // sprintf(debugMessage, "ee got %i bytes", numBytes);
  // debug(debugMessage);

  while(!wire->available()){} 
  rdata = wire->read();

  return(rdata);
}

// void readDeploymentIdentifier(char * deploymentIdentifier)
// {
//   for(short i=0; i < DEPLOYMENT_IDENTIFIER_LENGTH; i++)
//   {
//     short address = EEPROM_DEPLOYMENT_IDENTIFIER_ADDRESS_START + i;
//     deploymentIdentifier[i] = readEEPROM(&Wire, EEPROM_I2C_ADDRESS, address);
//   }
//   deploymentIdentifier[DEPLOYMENT_IDENTIFIER_LENGTH] = '\0';
// }

// void writeDeploymentIdentifier(char * deploymentIdentifier)
// {
//   for(short i=0; i < DEPLOYMENT_IDENTIFIER_LENGTH; i++)
//   {
//     short address = EEPROM_DEPLOYMENT_IDENTIFIER_ADDRESS_START + i;
//     writeEEPROM(&Wire, EEPROM_I2C_ADDRESS, address, deploymentIdentifier[i]);
//   }
// }

void readObjectFromEEPROM(short i2cAddress, short address, void * data, uint8_t size)
{
  byte * buffer = (byte *) data;
  for (uint8_t i = 0; i < size; i++)
  {
    buffer[i] = readEEPROM(&Wire, i2cAddress, address+i);
  }
}


void readObjectFromEEPROM(short address, void * data, uint8_t size) // Little Endian
{
  readObjectFromEEPROM(EEPROM_I2C_ADDRESS, address, data, size);
}

void writeObjectToEEPROM(int i2cAddress, int baseAddress, void * source, int size)
{
  byte * bytes = (byte *) source;
  for(short i=0; i < size; i++)
  {
    short address = baseAddress + i;
    writeEEPROM(&Wire, i2cAddress, address, bytes[i]);
  }
}

void writeObjectToEEPROM(int baseAddress, void * source, int size)
{
  writeObjectToEEPROM(EEPROM_I2C_ADDRESS, baseAddress, source, size);
}

void writeDataloggerSettingsToEEPROM(void * dataloggerSettings)
{
  writeObjectToEEPROM(EEPROM_DATALOGGER_CONFIGURATION_START, dataloggerSettings, EEPROM_DATALOGGER_CONFIGURATION_SIZE);
}

void writeSensorConfigurationToEEPROM(short slot, const void * configuration)
{
  int block = slot / 4 + 1;
  int offset = slot % 4;
  int deviceAddress = EEPROM_I2C_ADDRESS + block;
  int memoryAddress = offset * EEPROM_DATALOGGER_SENSOR_SIZE;
  // notify(deviceAddress);
  // notify(memoryAddress);
  writeObjectToEEPROM(deviceAddress, memoryAddress, (void *) configuration, EEPROM_DATALOGGER_SENSOR_SIZE);
}

void readSensorConfigurationFromEEPROM(short slot, void * configuration)
{
  int block = slot / 4 + 1;
  int offset = slot % 4;
  int deviceAddress = EEPROM_I2C_ADDRESS + block;
  int memoryAddress = offset * EEPROM_DATALOGGER_SENSOR_SIZE;
  readObjectFromEEPROM(deviceAddress, memoryAddress, configuration, EEPROM_DATALOGGER_SENSOR_SIZE);

}

void readUniqueId(unsigned char * uuid)
{
  for(int i=0; i < UUID_LENGTH; i++)
  {
    unsigned int address = EEPROM_UUID_ADDRESS_START + i;
    uuid[i] = readEEPROM(&Wire, EEPROM_I2C_ADDRESS, address);
  }

  debug(F("UUID in EEPROM:")); // TODO: need to create another function and read from flash  

  unsigned char uninitializedEEPROM[16] = { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  char uuidString[2 * UUID_LENGTH + 1];

  if(memcmp(uuid, uninitializedEEPROM, UUID_LENGTH) == 0)
  {
    // debug(F("Retrieve UUID"));
    getSTM32UUID(uuid);

    // debug(F("UUID to Write:"));
    // decodeUniqueId(uuid, uuidString, UUID_LENGTH);
    // debug(uuidString);
    
    for(int i=0; i < UUID_LENGTH; i++)
    {
      unsigned int address = EEPROM_UUID_ADDRESS_START + i;
      writeEEPROM(&Wire, EEPROM_I2C_ADDRESS, address, uuid[i]);
    }

    for(int i=0; i < UUID_LENGTH; i++)
    {
      unsigned int address = EEPROM_UUID_ADDRESS_START + i;
      uuid[i] = readEEPROM(&Wire, EEPROM_I2C_ADDRESS, address);
    }

    // debug(F("UUID in EEPROM:"));
    for(short i=0; i < UUID_LENGTH; i++)
    {
        sprintf(&uuidString[2*i], "%02X", (byte) uuid[i]);
    }
    // debug(uuidString);
  } 
  else 
  {
    // TODO: this is confused.  each byte is 00-FF, which means 12 bytes = 24 chars in hex
    // char uuidString[2 * UUID_LENGTH + 1];
    // decodeUniqueId(uuid, uuidString, UUID_LENGTH);
    // debug(uuidString);
  }

}

void writeEEPROMBytes(short address, unsigned char * data, uint8_t size) // Little Endian
{
  for (uint8_t i = 0; i < size; i++)
  {
    writeEEPROM(&Wire, EEPROM_I2C_ADDRESS, address+i, data[i]);
  }
}




void readEEPROMBytes(short address, unsigned char * data, uint8_t size) // Little Endian
{
  for (uint8_t i = 0; i < size; i++)
  {
    data[i] = readEEPROM(&Wire, EEPROM_I2C_ADDRESS, address+i);
  }
}


// void readEEPROMBytesMem(short address, void * destination, uint8_t size) // Little Endian
// {
//   debug(F("readEEPROMBytesMem"));
//   char buffer[size];
//   for (uint8_t i = 0; i < size; i++)
//   {
//     // read everything from address into buffer
//     // Serial2.print(address+i);
//     // Serial2.print(",");
//     // Serial2.flush(); 
//     buffer[i] = readEEPROM(&Wire, EEPROM_I2C_ADDRESS, address+i);
//   }
//   memcpy(destination, &buffer, size); // copy buffer to destination
// }

// void writeEEPROMBytesMem(short address, void * source, uint8_t size)
// {
//   char buffer[size];
//   //read everything from source into buffer
//   memcpy(&buffer, source, size);
//   for (uint8_t i = 0; i < size; i++)
//   {
//     // write everything from buffer to address
//     // note: output will look weird because 
//     // serial print doesn't distinguish between char and ints
//     // Serial2.print(buffer[i]);
//     // Serial2.print(address+i);
//     // Serial2.print(",");
//     // Serial2.flush();
//     writeEEPROM(&Wire, EEPROM_I2C_ADDRESS, address+i, buffer[i]);
//   }
// }

void clearEEPROMAddress(short address, uint8_t length)
{
  for (uint8_t i = 0; i < length; i++)
  {
    writeEEPROM(&Wire, EEPROM_I2C_ADDRESS, address+i, EEPROM_RESET_VALUE);
  }
}