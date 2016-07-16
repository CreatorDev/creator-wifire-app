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

#ifndef __BSP_SRAND_SEED_H_
#define __BSP_SRAND_SEED_H_

#include <stdint.h>
#include <stdbool.h>

/* Application specific ADC init
 */
void AdcDriver_Initialise(void);

/* Read potentiometer level
 * Returns: raw 12bit ADC value
 */
uint32_t AdcDriver_GetPotentiometerLevel(void);

/* Read temperature level
 * Returns: raw 12bit ADC value
 */
uint32_t AdcDriver_GetTemperatureLevel(void);

/* Read potentiometer voltage
 * Returns: value in volts
 */
float AdcDriver_GetPotentiometerVoltage(void);

/* Read temperature
 * Returns: temperature in Celcius or Fahrenheit
 */
float AdcDriver_GetTemperatureDegrees(bool isCelcius);

/* Start next ADC scan
 * Note: use this to minimise wait for value ready on next read
 */
void AdcDriver_ScanStart(void);

#endif //__BSP_SRAND_SEED_H_
