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
#ifndef WATERBEAR_ADAFRUIT_DHT22
#define WATERBEAR_ADAFRUIT_DHT22

#include "sensors/sensor.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE DHT22   // DHT 22 (AM2302)

#define ADAFRUIT_DHT22_TYPE_STRING "adafruit_dht22"

class AdaDHT22 : public GPIOProtocolSensorDriver
{

  typedef struct 
  {
    unsigned long long cal_timestamp;   // 8 bytes for epoch time of calibration (optional)
    short sensor_pin : 4; // 4 bits (16 states)
  } driver_configuration;

  public:
    // Constructor
    AdaDHT22();
    ~AdaDHT22();

    //
    // Interface Implementation
    //
    const char * getSensorTypeString();
    void setup();
    void stop();
    bool takeMeasurement();
    const char * getRawDataString();
    const char * getSummaryDataString();
    const char * getBaseColumnHeaders();
    void initCalibration();
    void calibrationStep(char *step, int arg_cnt, char ** args);
    
    uint32 millisecondsUntilNextReadingAvailable();

  protected:
    void configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurations);
    configuration_bytes_partition getDriverSpecificConfigurationBytes();
    bool configureDriverFromJSON(cJSON *json);
    void appendDriverSpecificConfigurationJSON(cJSON *json);
    void setDriverDefaults();

  private:
    const char *sensorTypeString = ADAFRUIT_DHT22_TYPE_STRING;
    driver_configuration configuration;
    DHT_Unified *dht;

    float temperature;
    float humidity;
    const char *baseColumnHeaders = "C,RH"; // will be written to .csv
    char dataString[16]; // will be written to .csv

    void addCalibrationParametersToJSON(cJSON *json);
};

#endif