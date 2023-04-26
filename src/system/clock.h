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

#ifndef WATERBEAR_CLOCK
#define WATERBEAR_CLOCK

#include "DS3231.h"

// The DS3231 RTC chip
extern DS3231 Clock;

#define ALRM1_MATCH_EVERY_SEC  0b1111  // once a second
#define ALRM1_MATCH_SEC        0b1110  // when seconds match
#define ALRM1_MATCH_MIN_SEC    0b1100  // when minutes and seconds match
#define ALRM1_MATCH_HR_MIN_SEC 0b1000  // when hours, minutes, and seconds match
//byte ALRM1_SET = ALRM1_MATCH_SEC;

#define ALRM2_ONCE_PER_MIN     0b111   // once per minute (00 seconds of every minute)
#define ALRM2_MATCH_MIN        0b110   // when minutes match
#define ALRM2_MATCH_HR_MIN     0b100   // when hours and minutes match
//byte ALRM2_SET = ALRM2_ONCE_PER_MIN;


void setNextAlarmInternalRTC(short interval);
void setNextAlarmInternalRTCSeconds(short seconds);
void setNextAlarmInternalRTCMilliseconds(int milliseconds);

#ifdef USES_DS3231_ALARM
void setNextAlarm(short interval);
#endif

void dateTime(uint16_t* date, uint16_t* time);
void clearAllAlarms();
time_t timestamp();
void setTime(time_t toSet);
void t_t2ts(time_t unixTS, uint32 diffMillis, char *humanTime);

#endif