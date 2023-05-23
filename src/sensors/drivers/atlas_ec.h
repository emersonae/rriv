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
#ifndef WATERBEAR_ATLAS_EC
#define WATERBEAR_ATLAS_EC

#include "sensors/sensor.h"
#include "EC_OEM.h"

#define ATLAS_EC_OEM_TYPE_STRING "atlas_ec"

class AtlasECDriver : public I2CProtocolSensorDriver
{

  typedef struct // 32 bytes max
  {
    unsigned long long cal_timestamp; // 8byte epoch timestamp at calibration // TODO: move to common
  } driver_configuration;

  public: 
    AtlasECDriver();
    ~AtlasECDriver();
  
  private:
    const char *sensorTypeString = ATLAS_EC_OEM_TYPE_STRING;
    driver_configuration configuration;
    EC_OEM *oem_ec; // A pointer to the I2C driver for the Atlas EC sensor
    
    int value;
    const char * baseColumnHeaders = "ec.mS";
    char dataString[20]; // local storage for data string

    // unsigned long long lastSuccessfulReadingMillis = 0;

  //
  // Interface Implementation
  //
  public:
    const char * getSensorTypeString();
    void setup();
    void stop();

    void wake();
    void hibernate();
    void setDebugMode(bool debug); // for setting internal debug parameters, such as LED on 

    bool takeMeasurement();
    const char * getRawDataString();
    const char * getSummaryDataString();
    const char * getBaseColumnHeaders();

    void initCalibration();
    void calibrationStep(char * step, int arg_cnt, char ** args);
    void addCalibrationParametersToJSON(cJSON * json);

    uint32 millisecondsUntilNextReadingAvailable();

  protected:
    void configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurations);
    configuration_bytes_partition getDriverSpecificConfigurationBytes();
    // bool configureDriverFromJSON(cJSON *json);
    void appendDriverSpecificConfigurationJSON(cJSON *json);
    void setDriverDefaults();
};

#endif