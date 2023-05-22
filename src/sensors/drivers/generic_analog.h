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
#ifndef WATERBEAR_GENERIC_ANALOG
#define WATERBEAR_GENERIC_ANALOG

#include "sensors/sensor.h"

#define GENERIC_ANALOG_DRIVER_TYPE_STRING "generic_analog"

class GenericAnalogDriver : public AnalogProtocolSensorDriver
{

  typedef struct // 32 bytes
  {
    // analog sensor that can be 2pt calibrated
    unsigned long long cal_timestamp; // 8 byte epoch timestamp at calibration  // TODO: move to common
    float m;                          // 4bytes, slope
    float b;                          // 4bytes, y-intercept
    short x1;                         // 2bytes for 2pt calibration
    short x1Var;                      // variance of x1
    short y1;                         // 2bytes for 2pt calibration
    short x2;                         // 2bytes for 2pt calibration
    short x2Var;                      // variance of x2
    short y2;                         // 2bytes for 2pt calibration
    short adc_select : 2;             // two bits, support hardware expansion (addnl adc chips)
    short sensor_port : 4;
    short calibrated : 1;
    byte order_of_magnitude; // for converting stored values back into entered values
    byte calibrationBurstCount;
  } generic_linear_analog_config;

public:
  // Constructor
  GenericAnalogDriver();
  ~GenericAnalogDriver();

private:
  const char *sensorTypeString = GENERIC_ANALOG_DRIVER_TYPE_STRING;
  generic_linear_analog_config configurations;

  int value;
  const char *baseColumnHeaders = "raw,cal";
  char dataString[16];
  float calibrationVariance = -1;

  short calibrate_high_reading = 0;
  float calibrate_high_variance = -1;
  double calibrate_high_value = 0;
  short calibrate_low_reading = 0;
  float calibrate_low_variance = -1;
  double calibrate_low_value = 0;

  void computeCalibratedCurve();
  void printCalibrationStatus();
  void takeCalibrationBurstMeasurement(); // for calibration
  double getCalibratedValue(double value);
  short getOrderOfMagnitudeToScale();


  //
  // Interface Implementation
  //
public:
  const char *getSensorTypeString();
  void setup();
  void stop();
  bool takeMeasurement();
  const char *getRawDataString();
  const char *getSummaryDataString();
  const char *getBaseColumnHeaders();

  void initCalibration();
  void calibrationStep(char *step, int arg_cnt, char **args);

  uint32 millisecondsUntilNextRequestedReading();

protected:
  void configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurations);
  configuration_bytes_partition getDriverSpecificConfigurationBytes();
  bool configureDriverFromJSON(cJSON *json);
  void appendDriverSpecificConfigurationJSON(cJSON *json);
  void setDriverDefaults();

private:
  void addCalibrationParametersToJSON(cJSON *json);
};

#endif