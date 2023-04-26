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

#include "filesystem.h"
#include "clock.h"
#include "monitor.h"
#include "system/logs.h"

char dataDirectory[6] = "/Data";


WaterBear_FileSystem::WaterBear_FileSystem(char * loggingFolder, int chipSelectPin)
{
  strcpy(this->loggingFolder, loggingFolder);
  this->chipSelectPin = chipSelectPin;
  this->initializeSDCard();
 
  this->setLoggingFolder(loggingFolder);
  debug("logging folder set");
}

void WaterBear_FileSystem::initializeSDCard(){
   // initialize the SD card
  //Serial2.print(F("Initializing SD card..."));
  notify("Start SD card...");

  // Make sure chip select pin is set to output
  pinMode(this->chipSelectPin, OUTPUT);

  // see if the card is present and can be initialized:
  if(!this->sd.begin(chipSelectPin, SPI_CLOCK_DIV4))
  {
    notify(F("Card fail"));
    // one way to handle a failure:
    // flash the led in an alert like fashion
    // go to sleep for a short period of time
    // then wake up and reset
    //
    // another way to handle a failure:
    // somehow latch on an error led and power everything else down.
    // errored out mode
    //
    // another way to handle a failure:
    // just go to sleep and wait for the next cycle
    // also produce some kind of check engine light.
    // can we do a very low current blink LED or something.
    delay(6000);
    nvic_sys_reset();
  }
  else
  {
    notify(F("card initialized."));
  }
}

void WaterBear_FileSystem::writeString(const char * string)
{
  // notify("printing to log file");
  // notify((int)strlen(string));
  // notify(string);
  this->logfile.print(string);
}

void WaterBear_FileSystem::endOfLine()
{
  this->logfile.println();
  this->logfile.flush();
}


void WaterBear_FileSystem::writeDebugMessage(const char* message)
{
  this->logfile.print("debug,");
  this->logfile.print(message);
  this->logfile.println();
  this->logfile.flush();
}

void WaterBear_FileSystem::dumpLoggedDataToStream(Stream * myStream, char * lastFileNameSent)
{
  /*
    // Debug
    // printCurrentDirListing();

    if (!sd.chdir(dataDirectory)) {
      notify(F("fail: Data."));
      state = 0;
      return;
    }

    // Debug
    // printCurrentDirListing();
    // notify(lastDownloadDate);

    sd.vwd()->rewind();
    SdFile dirFile;
    char sdDirName[30];
    char sdFileName[30];
    short rootDirIndex = 0;
    while (dirFile.openNext(sd.vwd(), O_READ)) {
      dir_t d;
      if (!dirFile.dirEntry(&d)) {
        //notify(F("dirEntry failed"));
        state = 0;
        return;
      }
      if(!dirFile.isDir()){
        // Descend into all the deployment directories
        continue;
      }
      dirFile.getName(sdDirName, 30);
      //Serial2.write("Dir: ");
      //notify(sdDirName);

      if(! sd.chdir(sdDirName) ){
        Serial2.write("fail:");
        notify(sdDirName);
        state = 0;
        return;
      }
      dirFile.close();

      sd.vwd()->rewind();
      SdFile deploymentFile;
      while (deploymentFile.openNext(sd.vwd(), O_READ)) {
        dir_t d;
        if (!deploymentFile.dirEntry(&d)) {
          // notify(F("depl file fail"));
          state = 0;
          return;
        }
        deploymentFile.getName(sdFileName, 24);


        if(strncmp(sdFileName, lastDownloadDate, 10) > 0){
            //notify(sdFilename);

            File datafile = sd.open(sdFileName);
            // send size of transmission ?
            // notify(datafile.fileSize());
            while (datafile.available()) {
                Serial2.write(datafile.read());
            }
            datafile.close();
         }
         deploymentFile.close();
      }

      //char deploymentCompleteMessage[34] = ">WT_DEPLOYMENT_TRANSFERRED<";
      //Serial2.write(deploymentCompleteMessage);

      sd.chdir(dataDirectory);

      rootDirIndex = rootDirIndex + 1;

      // sd.vwd()->rewind();
      // printCurrentDirListing();
      // Advance to the last directoy we were at
      sd.vwd()->rewind();

      for(short i = 0; i < rootDirIndex; i = i + 1){
        dirFile.openNext(sd.vwd(), O_READ);
        dirFile.close();
      }
    }

    if (!sd.chdir("/")) {
      notify(F("fail /"));
      state = 0;
      return;
    }
*/
}

void WaterBear_FileSystem::setLoggingFolder(char *newLoggingFolder)
{
  strcpy(loggingFolder, newLoggingFolder);
}

bool WaterBear_FileSystem::openFile(char * filename)
{
  this->sd.chdir("/");
  printCurrentDirListing();
  if(!this->sd.exists(dataDirectory))
  {
    notify("mkdir");
    this->sd.mkdir(dataDirectory);
    delay(10);
  }

  if(!this->sd.chdir(dataDirectory))
  {
    notify(F("failed: /Data."));
  }
  else
  {
    //notify(F("cd /Data."));
  }

  if(!this->sd.exists(loggingFolder))
  {
    // printCurrentDirListing();
    notify("mkdir:");
    notify(loggingFolder);
    this->sd.mkdir(loggingFolder);
  }

  if(!this->sd.chdir(this->loggingFolder))
  {
    notify("failed:");
    notify(loggingFolder);
  }
  // else
  // {
  //   Serial2.print("cd:");
  //   notify(loggingFolder);
  // }


  notify("Opening file");
  notify(filename);
  this->logfile = this->sd.open(filename, FILE_WRITE); //O_CREAT | O_WRITE | O_APPEND);
  notify("Opened");

  //sd.chdir();
  if(!logfile)
  {
    notify(F(">not found<"));
    return false;
  }

  return true;

}

void WaterBear_FileSystem::setNewDataFile(long unixtime, char * header)
{

  char uniquename[11];
  sprintf(uniquename, "%lu", unixtime);
  char suffix[5] = ".CSV";
  strncpy(filename, uniquename, 10);
  strncpy(&filename[10], suffix, 5);

  notify("cd");
  this->sd.chdir("/");
  delay(1);

  notify(header);
  strcpy(this->header, header);

  bool success = this->openFile(filename);
  if( !success )
  {
    Serial2.print(F("filesystem open failure"));
    while(1);
  }

  // TODO: add datalogger/slot settings as formatted header, prefaced with #

  this->logfile.println(header); // write the headers to the new logfile
  // this->logfile.flush();
  //Serial2.print("wrote:");
  //notify(ret);
}

void WaterBear_FileSystem::printCurrentDirListing()
{
  debug("dir");

  this->sd.vwd()->rewind();
  SdFile dirFile;
  char sdFilename[30];
  while (dirFile.openNext(sd.vwd(), O_READ))
  {
    dirFile.getName(sdFilename, 30);
    debug(sdFilename);
    dirFile.close();
  }
}

void WaterBear_FileSystem::closeFileSystem()
{
  // Serial2.println(F("Close filesystem"));
  //this->logfile.sync();
  this->logfile.close(); // syncs then closes

  //this->sd.end // doesn't exist
}

void WaterBear_FileSystem::reopenFileSystem()
{
  initializeSDCard();
  bool success = this->openFile(filename);
  if( !success )
  {
    debug(F("Reopen file failed"));
    time_t setupTime = timestamp();
    char setupTS[21];
    sprintf(setupTS, "time: %lld", setupTime);
    debug(setupTS);
    setNewDataFile(setupTime, this->header); // open a new file via epoch timestamp
  }
  else 
  {
    debug(F("Reopened file"));
  }
}

// bool WaterBear_FileSystem::checkFileSize()
// {
//   return (this->logfile.fileSize() > MAX_FILE_SIZE_BYTES);
// }