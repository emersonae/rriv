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

#include "watchdog.h"
#include <Arduino.h>
#include <libmaple/libmaple.h>
#include "logs.h"

void timerFired() // trigger a reset
{
  // Can only output 2 chars because interrupts can't handle more
  timer_pause(TIMER1);
  Serial2.println("TF");

  //notify("TF"); // calls flush leading to a crash as well
  //Serial2.flush(); // causes crash
  delay(5000);
  nvic_sys_reset();
}

void reloadCustomWatchdog()
{
  debug(F("reloadCustomWatchdog"));
  timer_generate_update(TIMER1);
}


/*
void extendCustomWatchdog(int seconds)
{
  while (seconds > 0)
  {
    debug(seconds);
    reloadCustomWatchdog();
    if (seconds > DEFAULT_WATCHDOG_TIMEOUT_SECONDS - 1)
    {
      delay((DEFAULT_WATCHDOG_TIMEOUT_SECONDS-1)*1000);
    }
    else
    {
      delay(seconds * 1000);
    }
    seconds = seconds - (DEFAULT_WATCHDOG_TIMEOUT_SECONDS - 1);
  }
}*/

void startCustomWatchDog()
{
  startCustomWatchDog(DEFAULT_WATCHDOG_TIMEOUT_SECONDS);
}

void startCustomWatchDog(int watchdogSeconds)
{
  // debug("Setup watchdog");

  timer_init(TIMER1);
  timer_set_prescaler(TIMER1, 65535); //  64000000 / 65536 = 976.5 Hz
  // int watchdogValue = 5 * 976.5 ; // 976.5 Hz * ( 5 * 976.5 counts) = 5s
  int watchdogValue = watchdogSeconds * 976.5; // 976.5 Hz * ( 5 * 976.5 counts) = 5s
  timer_set_compare(TIMER1, TIMER_CH1, watchdogValue);
  // timer_cc_enable(TIMER1, TIMER_CH1); // not necessary?

  timer_set_reload(TIMER1, watchdogValue);
  timer_generate_update(TIMER1);

  timer_resume(TIMER1);
  // debug(F("Resumed timer"));

  timer_attach_interrupt(TIMER1, TIMER_CC1_INTERRUPT, timerFired);
  // debug(F("Attached interrupt!"));
}

void disableCustomWatchDog()
{
  timer_detach_interrupt(TIMER1, TIMER_CC1_INTERRUPT);
  // timer_pause(TIMER1);
  // timer_cc_disable(TIMER1, TIMER_CH1);
  // timer_generate_update(TIMER1);
  // timer_disable(TIMER1);

  //timer_disable_irq ??
  // nvic_irq_disable()

  rcc_reset_dev(TIMER1->clk_id);
  rcc_clk_disable(TIMER1->clk_id);

  // timer_init(TIMER1); // not necessary
}

void printWatchDogStatus(){
  char message[50];
  int timerCount = timer_get_count(TIMER1);
  sprintf(message, reinterpret_cast<const char *> F("Timer Count: %d"), timerCount);
  debug(timerCount);
}