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

#ifndef WATERBEAR_SENSOR_DRIVER
#define WATERBEAR_SENSOR_DRIVER

#include <Arduino.h>
#include <Wire_slave.h>
#include <cJSON.h>
#include <map>
#include <string>

#define CALIBRATION_TIME_STRING reinterpret_cast<const char*>(F("calibration_time"))

typedef enum protocol
{
  analog,
  i2c,
  gpio,
  drivertemplate,
  genericactuator,
} protocol_type;

#define SENSOR_CONFIGURATION_SIZE 64

typedef struct 
{ 
  byte common[32];
  byte specific[32];
} configuration_bytes;

typedef struct 
{
  byte partition[32];
} configuration_bytes_partition;


// common_sensor_driver_config
// configurations shared between all drivers
// needs to be 32 bytes total (one configuration_partition_bytes)
// 20 bytes currently usused
typedef struct
{
  // arrange from biggest type to smallest type
  char tag[6];                    // 6 bytes
  unsigned short int sensor_type; // 2 bytes
  unsigned short int warmup;      // 2 bytes - in seconds (65535 max value/60=1092 min)
  byte slot;                      // 1 byte
  byte burst_size;                // 1 byte // reading_count

} common_sensor_driver_config;


#define MAX_REQUESTED_READING_DELAY 3600000;

class SensorDriver
{

public:
  // Constructor
  SensorDriver();
  virtual ~SensorDriver();
  bool configureFromJSON(cJSON *json);
  void configureFromBytes(configuration_bytes configurationBytes); 
  const configuration_bytes getConfigurationBytes();
  const common_sensor_driver_config * getCommonConfigurations();
  void setDefaults();

  void initializeBurst();
  void incrementBurst();
  bool burstCompleted();

  // utility function for providing mean for burst summary value
  void addValueToBurstSummaryMean(std::string tag, double value);
  double getBurstSummaryMean(std::string tag);

  char *getCSVColumnHeaders();
  cJSON *getConfigurationJSON(); // returns unprotected pointer

  short getSlot();
  void setConfigurationNeedsSave();
  void clearConfigurationNeedsSave();
  bool getNeedsSave();

protected:
  common_sensor_driver_config commonConfigurations;
  void configureCSVColumns();

private:
  char csvColumnHeaders[200] = "column_header";
  short burstCount = 0;
  bool configurationNeedsSave = false;

  // Variables for computing burst summary values
  std::map<std::string, double> burstSummarySums;
  std::map<std::string, int> burstSummarySumCounts;

  //
  // Subclass Implementation Interface
  //
public:

  /*
  * Identifies to the datalogger which communication protocol object
  * to pass into the driver.
  *
  * @return a value from the protocol_type enum.  Currently analog, i2c, or gpio
  */
  virtual protocol_type getProtocol() = 0;

  /*
  *  Returns a unique string that identifies this sensor
  *
  */
  virtual const char * getSensorTypeString() = 0;


  /*
   *  Perform any sensor specific allocations, object creation, or commands.
   *  The sensor should be ready to read values after this method completes.
   *  This method is optional.
   */
  virtual void setup();
  virtual void stop();
  virtual void hibernate();
  virtual void wake();
  virtual void setDebugMode(bool debug);

  /*
   *  Retrieve a measurement from the sensor
   *  and store read measurement(s) privately for later output.
   * 
   *  @return true indicates a measurement was read and stored.
   *  @return false indicates that the sensor is not ready to read and store a
   *  new measurement.
   * 
   *  This method is required.
   */
  virtual bool takeMeasurement() = 0;




  /*
   * Returns a comma separated string that contains one or more
   * measurement values from the last reading the driver successfully
   * preformed.
   * 
   * @return comma separate string of measurement values
   */
  virtual const char *getRawDataString() = 0;


  virtual const char *getSummaryDataString() = 0;

  /*
   * Returns a comma separated string that contains header values
   * for columns of data corresponding to the values retured by
   * getRawDataString().
   * 
   * @return comma separated string of columen headers
   */
  virtual const char *getBaseColumnHeaders() = 0;

  // Warm Up time, which is only checked once at the start of the measurement cycle
  virtual bool isWarmedUp();
  
  virtual int millisecondsToWarmUp();

  // Calibration
  virtual void initCalibration() = 0;
  ;
  virtual void calibrationStep(char *step, int arg_cnt, char **args) = 0;

  // Timing
  virtual uint32 millisecondsUntilNextReadingAvailable();

  virtual uint32 millisecondsUntilNextRequestedReading();

  virtual void factoryReset();

protected:

  virtual void configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurations); 
  
  virtual configuration_bytes_partition getDriverSpecificConfigurationBytes() = 0;

  virtual bool configureDriverFromJSON(cJSON *json);
  
  virtual void appendDriverSpecificConfigurationJSON(cJSON * json) = 0;
  
  virtual void setDriverDefaults() = 0;



};

#include "base/analog_protocol_driver.h"

/*
*  Base class for sensor drivers using the I2C protocol
*/
class I2CProtocolSensorDriver : public SensorDriver
{
public:
  ~I2CProtocolSensorDriver();
  protocol_type getProtocol();
  void setWire(TwoWire *wire);

protected:
  TwoWire *wire;
};

/*
*  Base class for sensor drivers using the GPIO protocol
*/
class GPIOProtocolSensorDriver : public SensorDriver
{
public:
  ~GPIOProtocolSensorDriver();
  protocol_type getProtocol();
};

/*
*  Base class for the sensor driver template
*/
class DriverTemplateProtocolSensorDriver : public SensorDriver
{
public:
  ~DriverTemplateProtocolSensorDriver();
  protocol_type getProtocol();
};
/*
*  Base class for actuators (for now?) AE
*/

class GenericActuatorProtocolSensorDriver : public SensorDriver
{
public: 
  ~GenericActuatorProtocolSensorDriver();
  protocol_type getProtocol();
};

void getDefaultsCommon(common_sensor_driver_config *fillValues);
void readCommonConfigOnly(common_sensor_driver_config *readValues); // not made //TODO: what is this


#endif
