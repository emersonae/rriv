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

#include "clock.h"
#include "configuration.h"
#include <RTClock.h>
#include "filesystem.h"
#include "logs.h"


DS3231 Clock;

void handleInterrupt(){
  // just do nothing
  // Serial2.println("RTC interrupt!!");
}

void setNextAlarmInternalRTC(short interval){
  short minutes = Clock.getMinute();
  //Serial2.println("minutes");
  //Serial2.println(Clock.getMinute(), DEC);
  //Serial2.println(minutes);
  short seconds = Clock.getSecond();
  // Serial2.println("seconds");
  // Serial2.println(seconds);
  // an example of the math
  // time = 10:48:12 (current time)
  // minutes = 48 ( current minutes)
  // seconds = 12 ( current seconds)
  // interval = 15
  // nextMinutes = 48 + 15 - (3) = 60
  // minutesDiff = 60 - 48 = 12
  // minutesDiffSeconds = 12 * 60 = 720
  // secondsUntilWake = 720 - 12 = 708
  // Serial2.println("interval");
  // Serial2.println(interval);
  // Serial2.println("minutes");
  // Serial2.println(minutes);
  short nextMinutes = (minutes + interval - (minutes % interval));
  Serial2.println("next minutes");
  Serial2.println(nextMinutes);
  short minutesDiff = nextMinutes - minutes;
  short minutesDiffSeconds = minutesDiff * 60;
  short secondsUntilWake = minutesDiffSeconds - seconds; // -offset.  Offset would allow for some startup time.
  // Serial2.println("seconds until wake");
  // Serial2.println(secondsUntilWake);

  RTClock * clock = new RTClock(RTCSEL_LSE);
  // Serial2.println("made clock");  Serial2.flush();

  // char message[100];
  // sprintf(message, "Got clock value (current): %lli", clock->getTime());
  // debug(message);
    
  clock->setTime(0);
  // sprintf(message, "Got clock value (reset): %lli", clock->getTime());
  // debug(message);

  clock->removeAlarm();
  // secondsUntilWake = 10;
  clock->setAlarmTime(secondsUntilWake);
  clock->createAlarm(handleInterrupt, secondsUntilWake);
  delete clock;

  // sprintf(message, "set alarm time to wake: %i", secondsUntilWake);
  // debug(message);
}


void setNextAlarmInternalRTCSeconds(short seconds)
{
  RTClock * clock = new RTClock(RTCSEL_LSE);
  // Serial2.println("made clock");  Serial2.flush();

  // char message[100];
  // sprintf(message, "Got clock value (current): %lli", clock->getTime());
  // debug(message);
  
  clock->setTime(0);
  // sprintf(message, "Got clock value (reset): %lli", clock->getTime());
  // debug(message);

  clock->removeAlarm();
  clock->setAlarmTime(seconds);
  clock->createAlarm(handleInterrupt, seconds);
  delete clock;

  // sprintf(message, "set alarm seconds until wake: %i", seconds);
  // notify(message);
}

void setNextAlarmInternalRTCMilliseconds(int milliseconds)
{
  RTClock * clock = new RTClock(RTCSEL_LSE, 32); //according to sheet clock/(prescaler + 1) = Hz
  // Serial2.println("made clock");  Serial2.flush();

  // char message[100];
  // sprintf(message, "Got clock value (current): %lli", clock->getTime());
  // debug(message);
    
  clock->setTime(0);
  // sprintf(message, "Got clock value (reset): %lli", clock->getTime());
  // debug(message);

  clock->removeAlarm();
  clock->setAlarmTime(milliseconds);
  clock->createAlarm(handleInterrupt, milliseconds);
  delete clock;

  // sprintf(message, "set alarm milliseconds until wake: %i", milliseconds);
  // notify(message);
}

#ifdef USES_DS3231_ALARM
void setNextAlarm(short interval)
{
  Clock.turnOffAlarm(1); // Clear the Control Register
  Clock.turnOffAlarm(2);
  Clock.checkIfAlarm(1); // Clear the Status Register
  Clock.checkIfAlarm(2);
  
  //
  // Alarm every interval minutes
  //
  int AlarmBits = ALRM2_ONCE_PER_MIN;
  AlarmBits <<= 4;
  AlarmBits |= ALRM1_MATCH_MIN_SEC;
  short minutes = Clock.getMinute();
  short nextMinutes = (minutes + interval - (minutes % interval)) % 60;
  char message[100];
  sprintf(message, "Next Alarm, with minutes: %i", nextMinutes);
  debug(message);
  Clock.setA1Time(0b0, 0b0, nextMinutes, 0b0, AlarmBits, true, false, false);

  //
  // Alarm every 10 seconds for debugging
  //
    // debug(F("Using short sleep"));
    // int AlarmBits = ALRM2_ONCE_PER_MIN;
    // AlarmBits <<= 4;
    // AlarmBits |= ALRM1_MATCH_SEC;
    // short seconds = Clock.getSecond();
    // short debugSleepSeconds = 30;
    // short nextSeconds = (seconds + debugSleepSeconds - (seconds % debugSleepSeconds)) % 60;
    // char message[100];
    // sprintf(message, "Next Alarm, with seconds: %i, now seconds: %i", nextSeconds, seconds);
    // debug(message);
    // Clock.setA1Time(0b0, 0b0, 0b0, nextSeconds, AlarmBits, true, false, false);

  // set both alarms to :00 and :30 seconds, every minute
      // Format: .setA*Time(DoW|Date, Hour, Minute, Second, 0x0, DoW|Date, 12h|24h, am|pm)
      //                    |                                    |         |        |
      //                    |                                    |         |        +--> when set for 12h time, true for pm, false for am
      //                    |                                    |         +--> true if setting time based on 12 hour, false if based on 24 hour
      //                    |                                    +--> true if you're setting DoW, false for absolute date
      //                    +--> INTEGER representing day of the week, 1 to 7 (Monday to Sunday)
      //


  Clock.turnOnAlarm(1);
}
#endif

void dateTime(uint16_t* date, uint16_t* time)
{
  // Fetch time from DS3231 RTC
  bool century = false;
	bool h24Flag;
	bool pmFlag;
  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(Clock.getYear() + 1900, Clock.getMonth(century) + 1, Clock.getDate()); // year is since 1900, months range 0-11

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(Clock.getHour(h24Flag, pmFlag), Clock.getMinute(), Clock.getSecond());
}

void clearAllAlarms()
{
  Clock.turnOffAlarm(1);
  Clock.turnOffAlarm(2);
  Clock.checkIfAlarm(1); // Clear the Status Register
  Clock.checkIfAlarm(2);
}


time_t timestamp()
{
  struct tm ts;
  bool century = false;
	bool h24Flag;
	bool pmFlag;

  ts.tm_year = Clock.getYear();
  ts.tm_mon = Clock.getMonth(century)-1;
  ts.tm_mday = Clock.getDate();
  ts.tm_wday = Clock.getDoW()-1;
  ts.tm_hour = Clock.getHour(h24Flag, pmFlag);
  ts.tm_min = Clock.getMinute();
  ts.tm_sec = Clock.getSecond();
  ts.tm_isdst = -1; // Is DST on? 1 = yes, 0 = no, -1 = unknown

  // notify(ts.tm_year);
  // notify(ts.tm_mon);
  // notify(ts.tm_mday);
  // notify(ts.tm_wday);
  // notify(ts.tm_hour);
  // notify(ts.tm_min);
  // notify(ts.tm_sec);
  // notify(ts.tm_isdst);

  return (mktime(&ts)); // turn tm struct into time_t value
}

void setTime(time_t toSet)
{
  // Clock.setEpoch(toSet); //takes up too much space
  struct tm ts;

  ts = *gmtime(&toSet); // Convert time_t epoch timestamp to tm as UTC time
  Clock.setClockMode(false); //true for 12h false for 24h
  Clock.setYear(ts.tm_year);
  Clock.setMonth(ts.tm_mon+1);
  Clock.setDate(ts.tm_mday);
  Clock.setDoW(ts.tm_wday+1);
  Clock.setHour(ts.tm_hour);
  Clock.setMinute(ts.tm_min);
  Clock.setSecond(ts.tm_sec);
}

void t_t2ts(time_t unixTS, uint32 diffMillis, char *humanTime)
{
  struct tm ts;
  char buf[21] = "0";

  ts = *gmtime(&unixTS); // convert unix to tm structure

  // Format time, "yyyy-mm-dd hh:mm:ss.sss" = "%Y/%m/%d %H:%M:%S" = 23
  strftime(buf, 20, "%Y-%m-%d %H:%M:%S", &ts);
  sprintf(humanTime, "%s.%03i", buf, diffMillis % 1000);
}
