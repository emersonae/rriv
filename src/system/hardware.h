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

#ifndef WATERBEAR_HARDWARE
#define WATERBEAR_HARDWARE

#include <Arduino.h>
#include <Wire_slave.h> // Communicate with I2C/TWI devices

// For F103RB
#define Serial Serial2

// extern TwoWire Wire;
#define WireOne Wire 

// extern TwoWire WireTwo;
#define WireTwo Wire1

#define SWITCHED_POWER_ENABLE PC6
#define SD_ENABLE_PIN PC8

#define BLE_COMMAND_MODE_PIN PB5
#define INTERRUPT_LINE_7_PIN PC7
//pinMode(PB10, INPUT_PULLDOWN); // This WAS interrupt line 10, user interrupt. Needs to be reassigned.

#define BATTERY_INPUT PB0 // battery.V

#define ANALOG_INPUT_1_PIN PB1 // A2
#define ANALOG_INPUT_2_PIN PC0 // A3
#define ANALOG_INPUT_3_PIN PC1 // A4
#define ANALOG_INPUT_4_PIN PC2 // A5
#define ANALOG_INPUT_5_PIN PC3 // A6

#define ONBOARD_LED_PIN PA5

#define GPIO_PIN_1 PA1
#define GPIO_PIN_2 PB2
#define GPIO_PIN_3 PB8 // 5v booster
#define GPIO_PIN_4 PB12
#define GPIO_PIN_5 PC11 // DHT22
#define GPIO_PIN_6 PC12 // actuator tests
#define GPIO_PIN_7 PC13

#define EXADC_RESET PC5

// Bluefruit on SPI
#define BLUEFRUIT_SPI_SCK   PB13
#define BLUEFRUIT_SPI_MISO  PB14
#define BLUEFRUIT_SPI_MOSI  PB15
//#define BLUEFRUIT_SPI_CS    PB8  // Pullup
#define BLUEFRUIT_SPI_IRQ   PB9
#define BLUEFRUIT_SPI_RST   PC4

void gpioPinOff(uint8 pin);
void gpioPinOn(uint8 pin);
void startSerial2();
void setupInternalRTC();
void setupHardwarePins();
int getBatteryValue();

#endif