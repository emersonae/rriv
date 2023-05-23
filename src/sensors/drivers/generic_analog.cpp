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

#include "generic_analog.h"
#include "system/logs.h"
#include "system/measurement_components.h"
#include "system/eeprom.h" // TODO: ideally not included in this scope
#include "system/clock.h"  // TODO: ideally not included in this scope
#include "sensors/sensor_map.h"
#include "system/hardware.h"
#include "utilities/rrivmath.h"

int ADC_PINS[5] = {
    ANALOG_INPUT_1_PIN,
    ANALOG_INPUT_2_PIN,
    ANALOG_INPUT_3_PIN,
    ANALOG_INPUT_4_PIN,
    ANALOG_INPUT_5_PIN
};

#define GENERIC_ANALOG_VALUE_TAG "value"

GenericAnalogDriver::GenericAnalogDriver() {}

GenericAnalogDriver::~GenericAnalogDriver() {}

const char * GenericAnalogDriver::getSensorTypeString()
{
  return sensorTypeString;
}

bool GenericAnalogDriver::configureDriverFromJSON(cJSON *json)
{
  const cJSON *adcSelectJSON = cJSON_GetObjectItemCaseSensitive(json, "adc_select");
  const char * errorMessage = reinterpret_cast<const char *>(F("Invalid adc select"));
  bool error = false;
  if (adcSelectJSON != NULL && cJSON_IsString(adcSelectJSON))
  {
    if (strcmp(adcSelectJSON->valuestring, "internal") == 0)
    {
      configurations.adc_select = ADC_SELECT_INTERNAL;
    }
    else if (strcmp(adcSelectJSON->valuestring, "external") == 0)
    {
      configurations.adc_select = ADC_SELECT_EXTERNAL;
    }
    else
    {
      error = true;
    }
  }
  else
  {
    error = true;
  }
  if(error)
  {
    notify(errorMessage);
    return false;
  }

  const cJSON *sensorPortJSON = cJSON_GetObjectItemCaseSensitive(json, "sensor_port");
  int maxSensorPorts = 4; // External has 4 channels
  if (configurations.adc_select == ADC_SELECT_INTERNAL)
  {
    maxSensorPorts++;
  }
  if (sensorPortJSON != NULL && cJSON_IsNumber(sensorPortJSON) && sensorPortJSON->valueint > 0 && sensorPortJSON->valueint <= maxSensorPorts)
  {
    configurations.sensor_port = (byte)sensorPortJSON->valueint - 1;
  }
  else
  {
    errorMessage = reinterpret_cast<const char *>(F("Invalid sensor port"));
    error = true;
  }
  if(error)
  {
    notify(errorMessage);
    return false;
  }
  return true;
}

void GenericAnalogDriver::setDriverDefaults()
{
  memset(&configurations, 0, sizeof(configurations));
  if (configurations.adc_select != ADC_SELECT_EXTERNAL && configurations.adc_select != ADC_SELECT_INTERNAL)
  {
    configurations.adc_select = ADC_SELECT_INTERNAL;
  }
}

configuration_bytes_partition GenericAnalogDriver::getDriverSpecificConfigurationBytes()
{
  configuration_bytes_partition partition;
  memcpy(&partition, &configurations, sizeof(generic_linear_analog_config));
  return partition;
}

void GenericAnalogDriver::configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurationPartition)
{
  memcpy(&configurations, &configurationPartition, sizeof(generic_linear_analog_config));
}


void GenericAnalogDriver::appendDriverSpecificConfigurationJSON(cJSON * json)
{
  cJSON_AddNumberToObject(json, "sensor_port", configurations.sensor_port + 1);
  switch (configurations.adc_select)
  {
  case ADC_SELECT_INTERNAL:
    cJSON_AddStringToObject(json, "adc_select", "internal");
    break;
  case ADC_SELECT_EXTERNAL:
    cJSON_AddStringToObject(json, "adc_select", "external");
    break;
  default:
    break;
  }
  addCalibrationParametersToJSON(json);
}

const char *GenericAnalogDriver::getBaseColumnHeaders()
{
  return baseColumnHeaders;
}

void GenericAnalogDriver::setup()
{
  if (configurations.adc_select == ADC_SELECT_INTERNAL)
  {
    pinMode(ADC_PINS[configurations.sensor_port], INPUT_ANALOG);
    // notify("Internal ADC Pin setup");
  }
}

void GenericAnalogDriver::stop()
{
  if (configurations.adc_select == ADC_SELECT_INTERNAL)
  {
    pinMode(ADC_PINS[configurations.sensor_port], INPUT);
    digitalWrite(ADC_PINS[configurations.sensor_port], LOW);
    // notify("ADC port stopped");
  }
}

bool GenericAnalogDriver::takeMeasurement()
{
  // take measurement and write to dataString member variable
  switch (configurations.adc_select)
  {
  case ADC_SELECT_INTERNAL:
  {
    int adcPin = ADC_PINS[configurations.sensor_port];
    this->value = analogRead(adcPin);
  }
  break;

  case ADC_SELECT_EXTERNAL:
  {
    // debug("get extADC value");
    this->value = externalADC->getChannelValue(configurations.sensor_port);
  }
  break;

  default:
  {
  } // bad configuration
  break;
  }

  // validate the value
  // store this->value for summary calculation
  addValueToBurstSummaryMean(GENERIC_ANALOG_VALUE_TAG, this->value);

  return true;
}

#define DEFAULT_CALIBRATION_BURST_LENGTH 20
#define MAX_CALIBRATION_BURST_LENGTH 200
void GenericAnalogDriver::takeCalibrationBurstMeasurement()
{
  if(configurations.calibrationBurstCount < 1 || configurations.calibrationBurstCount > MAX_CALIBRATION_BURST_LENGTH)
  {
    configurations.calibrationBurstCount = DEFAULT_CALIBRATION_BURST_LENGTH;
  }
  int x[MAX_CALIBRATION_BURST_LENGTH];
  int sum = 0;
  double sum1 = 0;
  notify("Calibration measurements:");
  for (int i = 0; i < configurations.calibrationBurstCount; i++)
  {
    if (configurations.adc_select == ADC_SELECT_EXTERNAL)
    {
      externalADC->convertEnabledChannels();
    }
    takeMeasurement();
    // TODO: check for 0 values, report them, and either skip them for calibration, or halt calibration and notify that board may have electrical issues?
    // Thoughts: reasons for 0 values? electrical issue, power failure, or are there scenarios where 0's are acceptable but still need to be skipped?
    notify(this->value);
    sum += this->value;
    x[i] = this->value;
    delay(100);
  }
  double average = (double) sum / configurations.calibrationBurstCount;
  this->value = average;

  /*  Compute  variance */
  for (int i = 0; i < configurations.calibrationBurstCount; i++)
  {
    sum1 = sum1 + rrivmath::power((x[i] - average), 2);
  }
  calibrationVariance = sum1 / (float)(configurations.calibrationBurstCount);
  char buffer[50];
  sprintf(buffer, "variance = %.2f\n", calibrationVariance);
  notify(buffer);
}

double GenericAnalogDriver::getCalibratedValue(double value)
{
  double calibratedValue = (configurations.m * value + configurations.b) * rrivmath::power(10, -getOrderOfMagnitudeToScale());
  return calibratedValue;
}

const char *GenericAnalogDriver::getRawDataString() //TODO: getRawDataString() ??
{
  sprintf(dataString, "%d,%0.3f", value, getCalibratedValue(value));
  return dataString;
}

const char *GenericAnalogDriver::getSummaryDataString()
{
  double burstSummaryMean = getBurstSummaryMean(GENERIC_ANALOG_VALUE_TAG);
  sprintf(dataString, "%0.3f,%0.3f", burstSummaryMean, getCalibratedValue(burstSummaryMean));
  return dataString;  
}

void GenericAnalogDriver::initCalibration()
{
  notify(F("Two point calibration"));
  notify(F("calibrate SLOT low VALUE"));
  notify(F("calibrate SLOT high VALUE"));
  notify(F("calibrate SLOT store"));
  calibrate_high_reading = calibrate_high_value = calibrate_low_reading = calibrate_low_value = 0;
}

void GenericAnalogDriver::printCalibrationStatus()
{
  notify(F("Calibration status:"));
  char buffer[50];
  sprintf(buffer, "high_reading: %d", calibrate_high_reading);
  notify(buffer);
  sprintf(buffer, "high_variance: %f", calibrate_high_variance);
  notify(buffer);
  sprintf(buffer, "high_value: %f", calibrate_high_value);
  notify(buffer);
  sprintf(buffer, "low_reading: %d", calibrate_low_reading);
  notify(buffer);
  sprintf(buffer, "low_variance: %f", calibrate_low_variance);
  notify(buffer);
  sprintf(buffer, "low_value: %f", calibrate_low_value);
  notify(buffer);
}

void missingArg()
{
  notify("Missing arg");
}

void GenericAnalogDriver::calibrationStep(char *step, int arg_cnt, char ** args)
{
  short testCalValues[3] = {1100, 2000, 3000};

  if (strcmp(step, "high") == 0)
  {    
    if(arg_cnt == 0)
    {
      missingArg();
      return;
    }
    // notify(args[0]);
    // notify(atof(args[0]));
    takeCalibrationBurstMeasurement();
    calibrate_high_reading = this->value;
    calibrate_high_value = atof(args[0]);
    calibrate_high_variance = this->calibrationVariance;
    printCalibrationStatus();
  }
  else if (strcmp(step, "low") == 0)
  {
    if(arg_cnt == 0)
    {
      missingArg();
      return;
    }
    takeCalibrationBurstMeasurement();

    calibrate_low_reading = this->value;
    calibrate_low_value = atof(args[0]);;
    calibrate_low_variance = this->calibrationVariance;

    printCalibrationStatus();
  }
  else if (strcmp(step, "store") == 0)
  {
    printCalibrationStatus();
    if (
        calibrate_high_reading == 0 || calibrate_high_value == 0 || calibrate_low_reading == 0 || calibrate_low_value == 0)
    {
      notify("Incomplete cal");
      return;
    }

    computeCalibratedCurve();
    setConfigurationNeedsSave();

    cJSON *json = cJSON_CreateObject();
    addCalibrationParametersToJSON(json);
    char *string = cJSON_Print(json);
    if (string == NULL)
    {
      notify("Print json fail");
    }
    notify(string);
    free(json);
  }
  else if(strcmp(step, "set-cal-burst-length") == 0)
  {
    configurations.calibrationBurstCount = atoi(args[0]);
  }
  else if(strcmp(step, "test-cal") == 0)
  {
    calibrate_high_reading = testCalValues[2];
    calibrate_high_value = .305;
    calibrate_low_reading = testCalValues[0];
    calibrate_low_value = .201;
  }
  else if(strcmp(step, "test-curve") == 0)
  {
    for(short i=0; i<3; i++)
    {
      value = testCalValues[i];
      notify(getRawDataString());
    }
  }
  else
  {
    notify("Invalid cal step");
  }
}

void GenericAnalogDriver::computeCalibratedCurve() // calibrate using linear slope equation, log time
{
  // y = mx+b    m = (y2-y1)/(x2-x1)    b = y - mx
  // all x and y are integers.  m and b are scale up and cast to int for storage

  // figure out orders of magnitude
  int orderOfMagnitude = rrivmath::floor(rrivmath::log10(calibrate_low_value)); // TODO this isn't enough to know OoM !
  // notify(orderOfMagnitude);
  int exponent = getOrderOfMagnitudeToScale();
  double scaledCalibrateHighValue = calibrate_high_value * rrivmath::power(10, exponent);
  double scaledCalibrateLowValue = calibrate_low_value * rrivmath::power(10, exponent);
  // notify(scaledCalibrateHighValue);
  // notify(scaledCalibrateLowValue);

  double m = (double)(scaledCalibrateHighValue - scaledCalibrateLowValue) / (double)(calibrate_high_reading - calibrate_low_reading);
  double b = scaledCalibrateHighValue - m * calibrate_high_reading;

  configurations.m = m;
  configurations.b = b;
  configurations.order_of_magnitude = orderOfMagnitude;
  configurations.x1 = calibrate_low_reading;
  configurations.x2 = calibrate_high_reading;
  configurations.y1 = scaledCalibrateLowValue; // TODO: larger storage for y values probably necessary
  configurations.y2 = scaledCalibrateHighValue;
  configurations.cal_timestamp = timestamp();
}

short GenericAnalogDriver::getOrderOfMagnitudeToScale()
{
  short exponent = (3 - configurations.order_of_magnitude);
  return exponent;
}

void GenericAnalogDriver::addCalibrationParametersToJSON(cJSON *json)
{
  if(configurations.order_of_magnitude > -6 && configurations.order_of_magnitude < 6)
  {  
    cJSON_AddNumberToObject(json, "m", configurations.m);
    cJSON_AddNumberToObject(json, "b", configurations.b);
    // cJSON_AddNumberToObject(json, "order_of_magnitude", configuration.order_of_magnitude);
    cJSON_AddNumberToObject(json, "x1", configurations.x1);
    cJSON_AddNumberToObject(json, "x1 var", configurations.x1Var);
    cJSON_AddNumberToObject(json, "x2", configurations.x2);
    cJSON_AddNumberToObject(json, "x2 var", configurations.x2Var);
    cJSON_AddNumberToObject(json, "cal burst length", configurations.calibrationBurstCount);
    cJSON_AddNumberToObject(json, "y1", configurations.y1 * rrivmath::power(10, -getOrderOfMagnitudeToScale()));
    cJSON_AddNumberToObject(json, "y2", configurations.y2 * rrivmath::power(10, -getOrderOfMagnitudeToScale()));
    cJSON_AddNumberToObject(json, CALIBRATION_TIME_STRING, configurations.cal_timestamp);
  }
  else
  {
    cJSON_AddStringToObject(json, "calibration", "not init");
  }
}

uint32 GenericAnalogDriver::millisecondsUntilNextRequestedReading()
{
  return 100; // analog can be read continuously, but would not be meaningful
}
