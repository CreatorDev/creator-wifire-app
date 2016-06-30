/***********************************************************************************************************************
 Copyright (c) 2016, Imagination Technologies Limited and/or its affiliated group companies.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
 following conditions are met:
     1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
        following disclaimer.
     2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
        following disclaimer in the documentation and/or other materials provided with the distribution.
     3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
        products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***********************************************************************************************************************/

#ifndef __IPSO_OBJECT__DEFINITIONS_H_
#define __IPSO_OBJECT__DEFINITIONS_H_

/* IPSO Smart-Object definitions
 * Defined in the Technical Guideline by the Internet Protocol for Smart Objects (IPSO) Alliance.
 * (IPSO-Smart-Objects-1.0.pdf - 2014). 
 */
#define IPSO_DIGITAL_INPUT_OBJECT       3200        // WiFire buttons
#define IPSO_DIGITAL_OUTPUT_OBJECT      3201
#define IPSO_ANALOGUE_INPUT_OBJECT      3202        // WiFire ADC
#define IPSO_ANALOGUE_OUTPUT_OBJECT     3203

#define IPSO_GENERIC_SENSOR_OBJECT      3300
#define IPSO_ILLUMINANCE_SENSOR_OBJECT  3301
#define IPSO_PRESENCE_SENSOR_OBJECT     3302
#define IPSO_TEMPERATURE_OBJECT         3303        // WiFire temperature
#define IPSO_HUMIDITY_SENSOR_OBJECT     3304
#define IPSO_POWER_MEASUREMENT_OBJECT   3305
#define IPSO_ACTUATION_OBJECT           3306
#define IPSO_SET_POINT_OBJECT           3308
#define IPSO_LOAD_CONTROL_OBJECT        3310
#define IPSO_LIGHT_CONTROL_OBJECT       3311        // WiFire LEDs
#define IPSO_POWER_CONTROL_OBJECT       3312
#define IPSO_ACCELEROMETER_OBJECT       3313
#define IPSO_MAGNETOMETER_OBJECT        3314
#define IPSO_BAROMETER_OBJECT           3315

/* IPSO reusable resource definitions (used to compose objects)
 * Note: only a sub-set of the defined resources have been included here.
 */
#define IPSO_DIGITAL_INPUT_STATE            5500
#define IPSO_DIGITAL_INPUT_COUNTER          5501
#define IPSO_DIGITAL_INPUT_POLARITY         5502
#define IPSO_DIGITAL_INPUT_DEBOUNCE_PERIOD  5503    // debounce in ms
#define IPSO_DIGITAL_INPUT_EDGE             5504    // edge selection
#define IPSO_DIGITAL_INPUT_COUNTER_RESET    5505    // execute command

#define IPSO_ANALOGUE_INPUT_CURRENT_VALUE   5600
#define IPSO_MIN_MEASURED_VALUE             5601
#define IPSO_MAX_MEASURED_VALUE             5602
#define IPSO_MIN_RANGE_VALUE                5603
#define IPSO_MAX_RANGE_VALUE                5604
#define IPSO_RESET_MIN_MAX_MEASURED_VALUES  5605

#define IPSO_SENSOR_VALUE                   5700
#define IPSO_UNITS                          5701
#define IPSO_APPLICATION_TYPE               5750
#define IPSO_SENSOR_TYPE                    5751

#define IPSO_ON_OFF                         5850
#define IPSO_DIMMER                         5851    // 0-100 (percentage)
#define IPSO_ON_TIME                        5852    // time since activated (seconds)

#endif // __IPSO_OBJECT__DEFINITIONS_H_


