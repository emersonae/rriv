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

#include "low_power.h"

#include <Arduino.h>
#include <libmaple/pwr.h>
#include <libmaple/scb.h>
#include <libmaple/rcc.h>
#include <libmaple/systick.h>
#include <libmaple/usb.h>
#include <libmaple/timer.h>
#include <libmaple/adc.h>
#include <libmaple/dac.h>
#include <libmaple/usart.h>
#include "logs.h"

void enterStopMode()
{
  // Clear PDDS and LPDS bits
  PWR_BASE->CR &= PWR_CR_LPDS | PWR_CR_PDDS | PWR_CR_CWUF;

  // Set PDDS and LPDS bits for standby mode, and set Clear WUF flag (required per datasheet):
  PWR_BASE->CR |= PWR_CR_CWUF;
  //??PWR_BASE->CR |= PWR_CR_PDDS; // Enter stop/standby mode when cpu goes into deep sleep

  // PWR_BASE->CR |=  PWR_CSR_EWUP;   // Enable wakeup pin bit.  This is for wake from the WKUP pin specifically

  //Unset Power down deepsleep bit.
  PWR_BASE->CR &= ~PWR_CR_PDDS; // Also have to unset this to get into STOP mode
  // set Low-power deepsleep.
  PWR_BASE->CR |= PWR_CR_LPDS; // Puts voltage regulator in low power mode.  
  
  SCB_BASE->SCR |= SCB_SCR_SLEEPDEEP;
  //SCB_BASE->SCR &= ~SCB_SCR_SLEEPDEEP;

  SCB_BASE->SCR &= ~SCB_SCR_SLEEPONEXIT;

  rcc_switch_sysclk(RCC_CLKSRC_HSI);
  rcc_turn_off_clk(RCC_CLK_PLL);
  rcc_turn_off_clk(RCC_CLK_LSI);

  __asm__ volatile( "dsb" ); // assembly: data synchronization barrier
  systick_disable();
  __asm__ volatile( "wfi" ); // assembly: wait for interrupt
  // wake up on interrupt
  systick_enable();
  __asm__ volatile( "isb" ); // assembly: instruction synchronization barrier

  rcc_turn_on_clk(RCC_CLK_PLL);
  while(!rcc_is_clk_ready(RCC_CLK_PLL));

  //Finally, switch to the now-ready PLL as the main clock source.
  rcc_switch_sysclk(RCC_CLKSRC_PLL);


}

void enterSleepMode()
{
  rcc_switch_sysclk(RCC_CLKSRC_HSI); 
  rcc_turn_off_clk(RCC_CLK_PLL);

  __asm__ volatile( "dsb" );
  systick_disable();
  __asm__ volatile( "wfi" );
  systick_enable();
  __asm volatile( "isb" );

  rcc_turn_on_clk(RCC_CLK_PLL); 
  while(!rcc_is_clk_ready(RCC_CLK_PLL));
  rcc_switch_sysclk(RCC_CLKSRC_PLL);
}

void componentsAlwaysOff()
{

  rcc_turn_off_clk(RCC_CLK_LSI);  

  usb_power_off();

  usart_disable(Serial1.c_dev());

  usart_disable(Serial3.c_dev());

  spi_peripheral_disable(SPI2);  // this one is used by the BLE chip

  timer_disable(&timer2);
  timer_disable(&timer3);
  timer_disable(&timer4);

  // this might be redundant
  ADC2->regs->CR2 &= ~ADC_CR2_TSVREFE;

  // adc_disable(ADC1); // turn off when asleep, potentially recalibrate when waking
  adc_disable(ADC2); // should always be off
  // adc_disable_all();

  // digital to analog converter, could always be disabled
  DAC_BASE->CR &= ~DAC_CR_EN1;  // don't think this made a difference
  DAC_BASE->CR &= ~DAC_CR_EN2;

return;

  // rcc_clk_disable( RCC_ADC1);
  rcc_clk_disable( RCC_ADC2);
  rcc_clk_disable( RCC_ADC3);
    

  //rcc_clk_disable( RCC_AFIO);
  
  // rcc_clk_disable( RCC_BKP);
  // rcc_clk_disable( RCC_CRC);
  rcc_clk_disable( RCC_DAC);
  rcc_clk_disable( RCC_DMA1);
  rcc_clk_disable( RCC_DMA2);
  rcc_clk_disable( RCC_FLITF);
  rcc_clk_disable( RCC_FSMC);



  // rcc_clk_disable( RCC_GPIOA); // ??
  // rcc_clk_disable( RCC_GPIOB);
  // rcc_clk_disable( RCC_GPIOC);
  // rcc_clk_disable( RCC_GPIOD);
  // rcc_clk_disable( RCC_GPIOE);
  // rcc_clk_disable( RCC_GPIOF);
  // rcc_clk_disable( RCC_GPIOG);
  //rcc_clk_disable( RCC_I2C1); // these clocks are needed during normal run
  //rcc_clk_disable( RCC_I2C2);
  // rcc_clk_disable( RCC_PWR); // ??
  // rcc_clk_disable( RCC_SDIO); // ??
  // rcc_clk_disable( RCC_SPI1);
  // rcc_clk_disable( RCC_SPI2);
  rcc_clk_disable( RCC_SPI3);
  rcc_clk_disable( RCC_SRAM);
  // rcc_clk_disable( RCC_TIMER1); // this clock is needed by the custom watchdog
  rcc_clk_disable( RCC_TIMER2);
  rcc_clk_disable( RCC_TIMER3);
  rcc_clk_disable( RCC_TIMER4);
  rcc_clk_disable( RCC_TIMER5);
  rcc_clk_disable( RCC_TIMER6);
  rcc_clk_disable( RCC_TIMER7);
  rcc_clk_disable( RCC_TIMER8);
  rcc_clk_disable( RCC_TIMER9);
  rcc_clk_disable( RCC_TIMER10);
  rcc_clk_disable( RCC_TIMER11);
  rcc_clk_disable( RCC_TIMER12);
  rcc_clk_disable( RCC_TIMER13);
  rcc_clk_disable( RCC_TIMER14);
  rcc_clk_disable( RCC_USART1);
  //rcc_clk_disable( RCC_USART2); // needed for USART2 
  rcc_clk_disable( RCC_USART3);
  rcc_clk_disable( RCC_UART4);
  rcc_clk_disable( RCC_UART5);
  rcc_clk_disable( RCC_USB);
  
}

// void hardwarePinsAlwaysOff() // not currently used
// {
// // any pins changed need to be set back to the right modes when we wake
// // need to find out what pinModes are default or how to reset them

// //disable unused pins()
//   pinMode(PA0, INPUT); // PA0-WKUP/USART2_CTS/ADC12_IN0/TIM2_CH1_ETR
//   pinMode(PA1, INPUT); // INPUT_ANALOG was suggested or INPUT_PULLDOWN?

//   pinMode(PA4, INPUT);

//   pinMode(PA6, INPUT);
//   pinMode(PA7, INPUT);
//   pinMode(PA8, INPUT);
//   pinMode(PA9, INPUT);
//   pinMode(PA10, INPUT);
//   pinMode(PA11, INPUT);
//   pinMode(PA12, INPUT);
//   pinMode(PA13, INPUT);
//   pinMode(PA14, INPUT);
//   pinMode(PA15, INPUT);

//   pinMode(PB0, INPUT); // PB0 ADC12_IN8/TIM3_CH3

//   pinMode(PB2, INPUT);
//   pinMode(PB3, INPUT);
//   pinMode(PB4, INPUT);
//   pinMode(PB5, INPUT);
//   pinMode(PB6, INPUT);
//   pinMode(PB7, INPUT);
//   pinMode(PB8, INPUT);
//   pinMode(PB9, INPUT);
//   pinMode(PB10, INPUT);
//   pinMode(PB11, INPUT);
//   pinMode(PB12, INPUT);
//   pinMode(PB13, INPUT);
//   pinMode(PB14, INPUT);
//   pinMode(PB15, INPUT);

//   pinMode(PC4, INPUT);
//   // pinMode(PC5, INPUT); // external ADC reset
//   // pinMode(PC6, INPUT); // this is the switch power pin

//   pinMode(PC9, INPUT);
//   pinMode(PC10, INPUT);
//   pinMode(PC11, INPUT);
//   pinMode(PC12, INPUT);
//   pinMode(PC13, INPUT);
//   pinMode(PC14, INPUT);
//   pinMode(PC15, INPUT);
// }

void componentsStopMode()
{

  i2c_disable(I2C1);  // other chips on waterbear board
  i2c_disable(I2C2);  // Atlas EC chip, external chips
                      // this is a place where using a timer as a watchdog may be important
  rcc_clk_disable( RCC_I2C1); // these clocks are needed during normal run
  rcc_clk_disable( RCC_I2C2);

  pinMode(PC8, OUTPUT); // check order of operation, necessary to switch components off
  digitalWrite(PC8, LOW);
  spi_peripheral_disable(SPI1);  // this one is used by the SD card

  // this might be redundant
  ADC1->regs->CR2 &= ~ADC_CR2_TSVREFE;  // turning off the temperature sensor that's in the ADC
  //^ does this need to be re-enabled? - kc

  adc_disable(ADC1); // turn off when asleep, potentially recalibrate when waking
}

void hardwarePinsStopMode()
{
  return; // didn't make a big power difference
  // check pins again on new board //
  pinMode(PA5, INPUT);
  pinMode(PB1, INPUT);
  pinMode(PC0, INPUT); // wasn't originally listed
  pinMode(PC1, INPUT);
  pinMode(PC2, INPUT);
  pinMode(PC3, INPUT);
  pinMode(PC7, INPUT);
  pinMode(PC8, INPUT);
  pinMode(PA2, INPUT); // USART2_TX/ADC12_IN2/TIM2_CH3
  pinMode(PA3, INPUT); // USART2_RX/ADC12_IN3/TIM2_CH4
}

void componentsBurstMode()
{
  debug(F("turn on components"));

  //pinmode?
  rcc_clk_enable( RCC_I2C1); // these clocks are needed during normal run
  rcc_clk_enable( RCC_I2C2);
  // i2c_master_enable(I2C1, I2C_BUS_RESET);
  // i2c_master_enable(I2C2, I2C_BUS_RESET);
  // i2c_master_enable(I2C1, 0, 0);
  // i2c_master_enable(I2C2, 0, 0);
  
  pinMode(PC8, OUTPUT); // check order of operation, necessary to switch components off
  digitalWrite(PC8, HIGH);
  spi_peripheral_enable(SPI1);
  
  adc_enable(ADC1);
  ADC1->regs->CR2 |= ADC_CR2_TSVREFE; // temperature sensor inside ADC

  debug(F("turned on components"));

}

void disableSerialLog()
{
  Serial2.end();
  usart_disable(Serial2.c_dev()); // turn back on when awakened? or not needed when deployed
}

void enableSerialLog()
{
  usart_enable(Serial2.c_dev());
  Serial2.begin(SERIAL_BAUD);
}

// // Not Used
// void restorePinDefaults()
// {

//   // setup hardware pins and test if it works, we may not need to do anything other than that

//   pinMode(PA0, OUTPUT);// PA0-WKUP/USART2_CTS/ADC12_IN0/TIM2_CH1_ETR
//   pinMode(PA1, OUTPUT); // USART2_RTS/ADC12_IN1/TIM2_CH2
//   pinMode(PA2, OUTPUT); // USART2_TX/ADC12_IN2/TIM2_CH3
//   pinMode(PA3, OUTPUT); // USART2_RX/ADC12_IN3/TIM2_CH4
//   pinMode(PA4, OUTPUT); // SPI1_NSS/USART2_CK/ADC12_IN4
//   pinMode(PA5, OUTPUT); // SPI1_SCK/ADC12_IN5
//   pinMode(PA6, OUTPUT); // SPI1_MISO/ADC12_IN6/TIM3_CH1
//   pinMode(PA7, OUTPUT); // SPI1_MOSI/ADC12_IN7/TIM3_CH2
//   pinMode(PA8, OUTPUT); // USART1_CK/TIM1_CH1/MCO
//   pinMode(PA9, OUTPUT); // USART1_TX/TIM1_CH2
//   pinMode(PA10, OUTPUT); // USART1_RX/TIM1_CH3
//   pinMode(PA11, OUTPUT); // USART1_CTS/CANRX/USBDM/TIM1_CH4
//   pinMode(PA12, OUTPUT); // USART1_RTS/CANTX/USBDP/TIM1_ETR
//   pinMode(PA13, OUTPUT); // JTMS/SWDIO
//   pinMode(PA14, OUTPUT); // JTCK/SWCLK
//   pinMode(PA15, OUTPUT); // JTDI

//   pinMode(PB0, OUTPUT);// PB0 ADC12_IN8/TIM3_CH3
//   pinMode(PB1, OUTPUT); // ADC12_IN9/TIM3_CH4
//   pinMode(PB2, OUTPUT); // PB2/BOOT1
//   pinMode(PB3, OUTPUT); // JTDO
//   pinMode(PB4, OUTPUT); // JNTRST
//   pinMode(PB5, OUTPUT); // I2C1_SMBAI
//   pinMode(PB6, OUTPUT); // I2C1_SCL/TIM4_CH1
//   pinMode(PB7, OUTPUT); // I2C1_SDA/TIM4_CH2
//   pinMode(PB8, OUTPUT); // TIM4_CH3
//   pinMode(PB9, OUTPUT); // TIM4_CH4
//   pinMode(PB10, OUTPUT); // I2C2_SCL/USART3_TX
//   pinMode(PB11, OUTPUT); // I2C2_SDA/USART3_RX
//   pinMode(PB12, OUTPUT); // SPI2_NSS/I2C2_SMBAI/USART3_CK/TIM1_BKIN
//   pinMode(PB13, OUTPUT); // SPI2_SCK/USART3_CTS/TIM1_CH1N
//   pinMode(PB14, OUTPUT); // SPI2_MISO/USART3_RTS/TIM1_CH2N
//   pinMode(PB15, OUTPUT); // SPI2_MOSI/TIM1_CH3N

//   pinMode(PC0, OUTPUT); // ADC12_IN10
//   pinMode(PC1, OUTPUT); // ADC12_IN11
//   pinMode(PC2, OUTPUT); // ADC12_IN12
//   pinMode(PC3, OUTPUT); // ADC12_IN13
//   pinMode(PC4, OUTPUT); // ADC12_IN14
//   pinMode(PC5, OUTPUT); // External ADC Reset  (ADC_RESET_PC5)
//   pinMode(PC6, OUTPUT);
//   pinMode(PC7, OUTPUT);
//   pinMode(PC8, OUTPUT);
//   pinMode(PC9, OUTPUT);
//   pinMode(PC10, OUTPUT);
//   pinMode(PC11, OUTPUT);
//   pinMode(PC12, OUTPUT);
//   pinMode(PC13, OUTPUT); // PC13-TAMPER-RTC
//   pinMode(PC14, OUTPUT); // PC14-OSC32_IN
//   pinMode(PC15, OUTPUT); // PC15-OSC32_OUT

// }
