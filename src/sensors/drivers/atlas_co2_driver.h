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
#ifndef WATERBEAR_ATLAS_CO2_DRIVER
#define WATERBEAR_ATLAS_CO2_DRIVER

#include "sensors/sensor.h"
#include <sensors/AtlasScientificCO2.h>
#include <sensors/CampbellOBS3.h>

//#define any pins/static options used

#define ATLAS_CO2_DRIVER_TYPE_STRING "atlas_co2"


class AtlasCO2Driver : public I2CProtocolSensorDriver
{
  typedef struct
  {
    unsigned long long cal_timestamp; // 8 bytes for unix time of calibration (optional)
  } driver_config;

  public:
    // Constructor
    AtlasCO2Driver();
    ~AtlasCO2Driver();

    const char * getSensorTypeString();

    // Interface
    void configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurations);
    configuration_bytes_partition getDriverSpecificConfigurationBytes();
    void appendDriverSpecificConfigurationJSON(cJSON * json);
    void setup();
    void stop();
    bool takeMeasurement();
    const char * getRawDataString();
    const char * getSummaryDataString();
    const char * getBaseColumnHeaders();
    void initCalibration();
    void calibrationStep(char *step, int arg_cnt, char ** args);
    uint32 millisecondsUntilNextReadingAvailable();
    bool isWarmedUp();
    int millisecondsToWarmUp();
    // void factoryReset();

  protected:
    bool configureDriverFromJSON(cJSON *json);
    void setDriverDefaults();

  private:
    //sensor specific variables, functions, etc.
    AtlasScientificCO2 *modularSensorDriver;
    CampbellOBS3 * campbell;
    driver_config configuration;

    const char * sensorTypeString = ATLAS_CO2_DRIVER_TYPE_STRING;
    uint32 setupTime; // for unix time of setup to track when ready to take samples
    int timeDiff;

    /*value(s) to be placed in dataString, should correspond to number of 
    column headers and entries in dataString*/
    int value; // sensor raw return(s) to be added to dataString
    const char *baseColumnHeaders = "CO2_ppm"; // will be written to .csv
    // const char *baseColumnHeaders = "CO2_ppm,C"; // will be written to .csv
    char dataString[30]; // will be written to .csv

    void addCalibrationParametersToJSON(cJSON *json);
};

#endif