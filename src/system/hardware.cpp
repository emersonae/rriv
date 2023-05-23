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

#include "hardware.h"
#include <libmaple/libmaple.h>
#include <libmaple/pwr.h>
#include "configuration.h"
#include "system/logs.h"

// TODO: are these functions pointless? Or will people understand what it means to write low/high to pins?
void gpioPinOff(uint8 pin)
{
    digitalWrite(pin, LOW);
}

void gpioPinOn(uint8 pin)
{
    digitalWrite(pin, HIGH);
}

void startSerial2()
{
  // Start up Serial2
  // TODO: Need to do an if(Serial2) after an amount of time, just disable it
  Serial2.begin(SERIAL_BAUD);
  while (!Serial2)
  {
    delay(100);
  }
  notify(F("Begin Serial2"));
}

void setupInternalRTC()
{
  // Set up the internal RTC
  RCC_BASE->APB1ENR |= RCC_APB1ENR_PWREN;
  RCC_BASE->APB1ENR |= RCC_APB1ENR_BKPEN;
  PWR_BASE->CR |= PWR_CR_DBP; // Disable backup domain write protection, so we can write
}

void setupHardwarePins()
{
  // debug(F("setup pins"));
  //pinMode(BLE_COMMAND_MODE_PIN, OUTPUT); // Command Mode pin for BLE
  
  pinMode(INTERRUPT_LINE_7_PIN, INPUT_PULLUP); // This the interrupt line 7
  
  pinMode(ONBOARD_LED_PIN, OUTPUT); // This is the onboard LED ? Turns out this is also the SPI1 clock.  niiiiice.

  pinMode(BATTERY_INPUT, INPUT_ANALOG); // to monitor the battery pack

  pinMode(GPIO_PIN_6, OUTPUT); // GPIO pin available
  pinMode(24, OUTPUT); // in use for 5v booster

  // pinMode(PA4, INPUT_PULLDOWN); // mosfet for battery measurement - should be OUTPUT ??

  // redundant?
  //pinMode(PA2, OUTPUT); // USART2_TX/ADC12_IN2/TIM2_CH3
  //pinMode(PA3, INPUT); // USART2_RX/ADC12_IN3/TIM2_CH4

  pinMode(PC5, OUTPUT); // external ADC reset
  digitalWrite(PC5, HIGH);
}

int getBatteryValue()
{
  return analogRead(BATTERY_INPUT);
}