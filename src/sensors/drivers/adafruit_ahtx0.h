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

#ifndef WATERBEAR_ADAFRUIT_AHTX0
#define WATERBEAR_ADAFRUIT_AHTX0

#include "sensors/sensor.h"
#include <Adafruit_AHTX0.h>


#define ADAFRUIT_DHTX0_TYPE_STRING "adafruit_ahtx0"

class AdaAHTX0 : public I2CProtocolSensorDriver
{
  typedef struct // 32 bytes max
  {
    unsigned long long cal_timestamp; // 8byte epoch timestamp at calibration // TODO: move to common
  } driver_configuration;
  public:
  //Constrcutor
    AdaAHTX0();
    ~AdaAHTX0();
    const char * getSensorTypeString();

    bool takeMeasurement();
    const char * getRawDataString();
    const char * getSummaryDataString();
    const char * getBaseColumnHeaders();

    void initCalibration();
    void calibrationStep(char * step, int arg_cnt, char ** args);
    
    void setup();
    void stop();

    uint32 millisecondsUntilNextReadingAvailable();

  protected:
    void configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurations);
    configuration_bytes_partition getDriverSpecificConfigurationBytes();
    bool configureDriverFromJSON(cJSON *json);
    void appendDriverSpecificConfigurationJSON(cJSON *json);
    void setDriverDefaults();

  private:
    const char *sensorTypeString = ADAFRUIT_DHTX0_TYPE_STRING;
    driver_configuration configuration;
    Adafruit_AHTX0 *aht; //pointer to I2C driver for Adafruit_AHTX0
    
    float temperature;
    float humidity;
    const char * baseColumnHeaders = "C,RH";
    char dataString[16]; // local storage for data string

    unsigned long long lastSuccessfulReadingMillis = 0;

    void addCalibrationParametersToJSON(cJSON * json);
    

    

};

#endif