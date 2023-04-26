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

#ifndef WATERBEAR_FILESYSTEM
#define WATERBEAR_FILESYSTEM

#include "SdFat.h"
#include "DS3231.h"
#include "write_cache.h"

// #define MAX_FILE_SIZE_BYTES  100000 // basing off of when it resets while running (121.3kb)
// #define MAX_FILE_SIZE_BYTES  8000//

class WaterBear_FileSystem : public OutputDevice
{

private:
  // File system object.
  SdFat sd;
  File logfile;
  int chipSelectPin;
  char filename[15];
  char loggingFolder[29];
  char header[200];

  void printCurrentDirListing();
  bool openFile(char * filename);


public:
  WaterBear_FileSystem(char * loggingFolder, int chipSelectPin);
  void initializeSDCard();
  void writeDebugMessage(const char* message);
  void setLoggingFolder(char * loggingFolder);
  void setNewDataFile(long unixtime, char * header);
  void dumpLoggedDataToStream(Stream * myStream, char * lastFileNameSent);
  void closeFileSystem(); // close filesystem when sleeping
  void reopenFileSystem(); // reopen filesystem after wakeup
  //bool checkFileSize(); //return whether max file size has been exceeded or not
  void writeString(const char * string);
  void endOfLine();

};

#endif
