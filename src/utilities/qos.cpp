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

#include "qos.h"
#include <Arduino.h>
#include <libmaple/libmaple.h>
#include "system/logs.h"
#include "utilities/utilities.h"

extern "C" char* _sbrk(int incr);
int freeMemory()
{
  char top;
  // debug((int) &top );
  // debug((int) reinterpret_cast<char*>(_sbrk(0)) );

  return &top - reinterpret_cast<char*>(_sbrk(0));
}

void intentionalMemoryLeak()
{
  // cause a memory leak
  char * mem = (char *) malloc(400); // intentional memory leak, big enough to get around buffering
  debug(mem); // use it so compiler doesn't remove the leak
}

void checkMemory()
{
  // calculate and print free memory
  // reset the system if we are running out of memory
  // char freeMemoryMessage[21];
  int freeMemoryAmount = freeMemory();
  // sprintf(freeMemoryMessage, reinterpretCharPtr(F("FreeMem: %d")), freeMemoryAmount);
  // debug(freeMemoryMessage);
  if(freeMemoryAmount < 500){
    //debug(F("Low mem, resetting!"));
    debug(F("LOWMEM"));
    nvic_sys_reset(); // software reset, takes us back to init
  }
}

void printFreeMemory()
{
  char freeMemoryMessage[21];
  int freeMemoryAmount = freeMemory();
  sprintf(freeMemoryMessage, reinterpretCharPtr(F("Free Memory: %d")), freeMemoryAmount);
  notify(freeMemoryAmount);
}