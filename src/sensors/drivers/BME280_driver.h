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
#ifndef WATERBEAR_BME280
#define WATERBEAR_BME280

#include "sensors/sensor.h"
#include <Seeed_BME280.h>

#define BME280_TYPE_STRING "BME280"

class BME280Driver : public I2CProtocolSensorDriver
{
 
  typedef struct
  {
    unsigned long long cal_timestamp; // 8 bytes for epoch time of calibration
  } driver_config;

  public:
    // Constructor
    BME280Driver();
    ~BME280Driver();

    
    const char * getSensorTypeString();
    void setup();
    void stop();
    bool takeMeasurement();
    const char * getRawDataString();
    const char * getSummaryDataString();
    const char * getBaseColumnHeaders();
    void initCalibration();
    void calibrationStep(char *step, int arg_cnt, char ** args);
    
    configuration_bytes_partition getDriverSpecificConfigurationBytes();
    void configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurations);

  protected:
    
    bool configureDriverFromJSON(cJSON *json);
    void appendDriverSpecificConfigurationJSON(cJSON *json);
    void setDriverDefaults();

  private:
    //sensor specific variables, functions, etc.
    const char *sensorTypeString = BME280_TYPE_STRING;
    driver_config configuration;
    BME280 *BME280ptr;
    void addCalibrationParametersToJSON(cJSON *json);

    /*value(s) to be placed in dataString, should correspond to number of 
    column headers and entries in dataString*/
    float temperature; // sensor raw return(s) to be added to dataString
    float pressure;
    const char *baseColumnHeaders = "C,hPa "; // will be written to .csv
    char dataString[30]; // will be written to .csv

    
};

#endif