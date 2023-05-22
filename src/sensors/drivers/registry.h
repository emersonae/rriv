#ifndef WATERBEAR_SENSOR_REGISTRY
#define WATERBEAR_SENSOR_REGISTRY

#include "generic_analog.h"
#include "atlas_ec.h"
#include "driver_template.h"
#include "adafruit_dht22.h"
<<<<<<< HEAD
#include "generic_actuator.h"
#include "air_pump.h"
#include "BME280_driver.h"
#include "adafruit_ahtx0.h"

=======
#include "atlas_co2_driver.h"

#include "adafruit_ahtx0.h"
>>>>>>> 6592eb8104ebdd496d50977e28bc35a3891c9c92

#define MAX_SENSOR_TYPE 0xFFFE

void buildDriverSensorMap();

#endif