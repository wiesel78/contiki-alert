#ifndef DEVICE_UTILS_H_
#define DEVICE_UTILS_H_

#include "dev/button-sensor.h"
#include "dev/als-sensor.h"

#define TARGET_CC2538DK cc2538dk

#if TARGET == TARGET_CC2538DK
#include "dev/cc2538-sensors.h"
#define GET_VALUE_TEMPERATURE (cc2538_temp_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED))
#define GET_VALUE_LIGHT (als_sensor.value(0))
#define GET_POWER (vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED))
#endif /* TARGET == "cc2538dk" */

#endif /* DEVICE_UTILS_H_ */
