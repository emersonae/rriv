#include "sensors/drivers/adafruit_dht22.h"
#include "system/logs.h" // for debug() and notify()
#include "system/hardware.h" // for pin names

short GPIO_PINS[7] = {
    GPIO_PIN_1, // CN7-30
    GPIO_PIN_2, // CN10-22
    GPIO_PIN_3, // CN10-24 // 5v booster
    GPIO_PIN_4, // CN10-16
    GPIO_PIN_5, // CN7-2
    GPIO_PIN_6, // CN7-3
    GPIO_PIN_7 // CN7-23
};

#define TEMPERATURE_VALUE_TAG "temperature"
#define HUMIDITY_VALUE_TAG "humidity"

AdaDHT22::AdaDHT22()
{
  //debug("allocating AdaDHT22")
}

AdaDHT22::~AdaDHT22(){}

const char * AdaDHT22::getSensorTypeString()
{
  return sensorTypeString;
}

configuration_bytes_partition AdaDHT22::getDriverSpecificConfigurationBytes()
{
  configuration_bytes_partition partition;
  memcpy(&partition, &configuration, sizeof(driver_configuration));
  return partition;
}

void AdaDHT22::configureSpecificConfigurationsFromBytes(configuration_bytes_partition configurationPartition)
{
  memcpy(&configuration, &configurationPartition, sizeof(driver_configuration));
}

void AdaDHT22::appendDriverSpecificConfigurationJSON(cJSON * json)
{
  // debug("getting AdaDHT22 json");
  cJSON_AddNumberToObject(json, "sensor_pin", configuration.sensor_pin + 1);

  //driver specific config, customize
  addCalibrationParametersToJSON(json);
}

void AdaDHT22::setup()
{
  // debug("setup AdaDHT22");
  short gpioPin = GPIO_PINS[configuration.sensor_pin];
  dht = new DHT_Unified(gpioPin, DHTTYPE);
  dht->begin();
  // notify("AdaDHT22 Initialized");
}

void AdaDHT22::stop()
{
  delete dht;
  pinMode(GPIO_PINS[configuration.sensor_pin], INPUT);
  digitalWrite(GPIO_PINS[configuration.sensor_pin], LOW);
  // notify("AdaDHT22 stopped");
}

bool AdaDHT22::takeMeasurement()
{
  // debug("taking measurement from AdaDHT22");
  sensors_event_t event;
  bool measurementTaken = false;

  dht->temperature().getEvent(&event);
  temperature = event.temperature;
  if(isnan(temperature))
  {
    notify("Read Error: temperature");
  }
  else
  {
    measurementTaken = true;
  }

  dht->humidity().getEvent(&event);
  humidity = event.relative_humidity;
  if(isnan(humidity))
  {
    notify("Read Error: humidity");
  }
  else
  {
    measurementTaken = true;
  }

  if(measurementTaken)
  {
    addValueToBurstSummaryMean(TEMPERATURE_VALUE_TAG, temperature);
    addValueToBurstSummaryMean(HUMIDITY_VALUE_TAG, humidity);
  }

  return measurementTaken;
}

const char *AdaDHT22::getRawDataString()
{
  // debug("configuring AdaDHT22 dataString");
  // process data string for .csv
  sprintf(dataString, "%.2f,%.2f", temperature, humidity);
  return dataString;
}

const char *AdaDHT22::getSummaryDataString()
{
  double temperatureBurstSummaryMean = getBurstSummaryMean(TEMPERATURE_VALUE_TAG);
  double humidityBurstSummaryMean = getBurstSummaryMean(HUMIDITY_VALUE_TAG);
  sprintf(dataString, "%0.3f,%0.3f", temperatureBurstSummaryMean, humidityBurstSummaryMean);
  return dataString;  
}

const char *AdaDHT22::getBaseColumnHeaders()
{
  // for debug column headers defined in the .h
  // debug("getting AdaDHT22 base column headers");
  return baseColumnHeaders;
}

void AdaDHT22::initCalibration()
{
  // debug("init AdaDHT22 calibration");
}

void AdaDHT22::calibrationStep(char *step, int arg_cnt, char ** args)
{
  // for intermediary steps of calibration process
  // debug("AdaDHT22 calibration steps");
}

void AdaDHT22::addCalibrationParametersToJSON(cJSON *json)
{
  // follows structure of calibration parameters in .h
  // debug("add AdaDHT22 calibration parameters to json");
  cJSON_AddNumberToObject(json, CALIBRATION_TIME_STRING, configuration.cal_timestamp);
}

bool AdaDHT22::configureDriverFromJSON(cJSON *json)
{
  const cJSON *sensorPinJSON = cJSON_GetObjectItemCaseSensitive(json, "sensor_pin");
  int gpioPinCount = 7;  
  if (sensorPinJSON->valueint > 0 && sensorPinJSON->valueint <= gpioPinCount)
  {
    configuration.sensor_pin = (byte)sensorPinJSON->valueint - 1;
  }
  else
  {
    notify("Invalid sensor pin");
    return false;
  }
  return true;
}

void AdaDHT22::setDriverDefaults()
{
  // debug("setting AdaDHT22 driver defaults");
  // set default values for driver struct specific values
  configuration.cal_timestamp = 0;
}

uint32 AdaDHT22::millisecondsUntilNextReadingAvailable()
{
  return 2000; // 1 reading per 2 seconds
}