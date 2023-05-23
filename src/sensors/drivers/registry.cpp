#include "registry.h"
#include "sensors/sensor_map.h"
#include "system/logs.h"
//
// Follow steps to add a new sensor driver
//

// Step 1: Include the header for your driver in sensor_map.h
//include in registry.h not sensor_map.h??


#define GENERIC_ANALOG_SENSOR 0x0000
#define ATLAS_EC_OEM_SENSOR 0x0001
#define ADAFRUIT_DHT22_SENSOR 0x0002
#define ATLAS_CO2_SENSOR 0x0003
<<<<<<< HEAD
#define GENERIC_ACTUATOR 0x0004
#define AIR_PUMP 0x0005
#define BME280 0x0006
=======

>>>>>>> 6592eb8104ebdd496d50977e28bc35a3891c9c92
#define ADAFRUIT_AHTX0_SENSOR 0x0007
// Step 2: Add a #define for the next available integer code

#define DRIVER_TEMPLATE 0xFFFE
#define NO_SENSOR 0xFFFF




void buildDriverSensorMap()
{  

  setupSensorMaps<GenericAnalogDriver>(GENERIC_ANALOG_SENSOR, F(GENERIC_ANALOG_DRIVER_TYPE_STRING));

<<<<<<< HEAD
  //setupSensorMaps<AtlasECDriver>(ATLAS_EC_OEM_SNSOR, F(ATLAS_EC_OEM_TYPE_STRING));
=======
  // setupSensorMaps<AtlasECDriver>(ATLAS_EC_OEM_SENSOR, F(ATLAS_EC_OEM_TYPE_STRING));
>>>>>>> 6592eb8104ebdd496d50977e28bc35a3891c9c92

  //setupSensorMaps<DriverTemplate>(DRIVER_TEMPLATE, F(DRIVER_TEMPLATE_TYPE_STRING));
 
  // setupSensorMaps<AdaDHT22>(ADAFRUIT_DHT22_SENSOR, F(ADAFRUIT_DHT22_TYPE_STRING));

  setupSensorMaps<AtlasCO2Driver>(ATLAS_CO2_SENSOR, F(ATLAS_CO2_DRIVER_TYPE_STRING)); // 4848 bytes

  setupSensorMaps<AdaAHTX0>(ADAFRUIT_AHTX0_SENSOR, F(ADAFRUIT_DHTX0_TYPE_STRING));

  // setupSensorMaps<GenericActuator>(GENERIC_ACTUATOR, F(GENERIC_ACTUATOR_TYPE_STRING));

  // setupSensorMaps<AirPump>(AIR_PUMP, F(AIR_PUMP_TYPE_STRING));

  //setupSensorMaps<BME280Driver>(BME280, F(BME280_TYPE_STRING));

  setupSensorMaps<AdaAHTX0>(ADAFRUIT_AHTX0_SENSOR, F(ADAFRUIT_DHTX0_TYPE_STRING));

  //Step 3: call setupSensorMaps with the class name, code, and type string for your sensor
  

}
