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
#include <stdint.h>
#include <string.h>
#include <xc.h>

#include "adc_driver.h"

#define WIFIRE_POT_ADC_CH_INDEX (8)
#define WIFIRE_TEMPERATURE_ADC_CH_INDEX (44)

void AdcDriver_Initialise(void)
{
#ifdef __32MZ2048ECG100__

    /*  Potentiometer */
    TRISBbits.TRISB13 = 1;
    CNPUBbits.CNPUB13 = 0;
    CNPDBbits.CNPDB13 = 0;
    ANSELBbits.ANSB13 = 1;

    /* Set up the CAL registers */
    AD1CAL1 = DEVADC1;
    AD1CAL2 = DEVADC2;
    AD1CAL3 = DEVADC3;
    AD1CAL4 = DEVADC4;
    AD1CAL5 = DEVADC5;

    /* Configure AD1CON1 */
    AD1CON1 = 0;    // No AD1CON1 features are enabled including: Stop-in-Idle, early
    // interrupt, filter delay Fractional mode and scan trigger source.

    /* Configure AD1CON2 */
    AD1CON2 = 0;                // Boost, Low-power mode off, SAMC set to min, set up the ADC Clock
    AD1CON2bits.SAMC = 0xFF;// Sample time = 256 x Tad
    AD1CON2bits.ADCSEL = 1;// 1 = SYSCLK, 2 REFCLK03, 3 FRC
    AD1CON2bits.ADCDIV = 100;// TQ = 1/200 MHz; Tad = 100 * (TQ * 2) = 1 us; 1 MHz ADC clock

    AD1CON3 = 0;

    AD1IMOD = 0;
    AD1IMODbits.SH0MOD = 0x02;// Workaround for calibration problem
    AD1IMODbits.SH1MOD = 0x02;// Workaround for calibration problem
    AD1IMODbits.SH2MOD = 0x02;// Workaround for calibration problem
    AD1IMODbits.SH3MOD = 0x02;// Workaround for calibration problem
    AD1IMODbits.SH4MOD = 0x02;// Workaround for calibration problem
    AD1IMODbits.SH5MOD = 0x02;// Workaround for calibration problem

    AD1GIRQEN1 = 0;// No global interrupts are used.
    AD1GIRQEN2 = 0;

    AD1CSS1 = 0;// No channel scanning is used.
    AD1CSS2 = 0;

    /* Configure AD1CMPCONx */
    AD1CMPCON1 = 0; // No digital comparators are used.
    AD1CMPCON2 = 0;
    AD1CMPCON3 = 0;
    AD1CMPCON4 = 0;
    AD1CMPCON5 = 0;
    AD1CMPCON6 = 0;

    /* Configure AD1FLTRx */
    AD1FLTR1 = 0; // No oversampling filters are used.
    AD1FLTR2 = 0;
    AD1FLTR3 = 0;
    AD1FLTR4 = 0;
    AD1FLTR5 = 0;
    AD1FLTR6 = 0;

    /* Set up the trigger sources */
    AD1TRG1 = 0; // Initialize all sources to no trigger.
    AD1TRG2 = 0;
    AD1TRG3 = 0;

    /* Select AN8 for conversion */
    AD1CON3bits.ADINSEL = 8;

    /* Turn the ADC on, start calibration */
    /* Wait for calibration to complete */
    AD1CON1bits.ADCEN = 1;
    while (AD1CON2bits.ADCRDY == 0);
    AD1IMODbits.SH0MOD = 0x0;     // Workaround for calibration problem
    AD1IMODbits.SH1MOD = 0x0;//
    AD1IMODbits.SH2MOD = 0x0;//
    AD1IMODbits.SH3MOD = 0x0;//
    AD1IMODbits.SH4MOD = 0x0;//
    AD1IMODbits.SH5MOD = 0x0;//

#else
    /* Enable clock to analog circuit */
    ADCANCONbits.ANEN7 = 1;                 // Enable the clock to analog bias ADC7
    while (!ADCANCONbits.WKRDY7)
        ;            // Wait until ADC7 is ready

    /* Enable the ADC module */
    ADCCON3bits.DIGEN7 = 1;                 // Enable ADC7
#endif
}


uint32_t AdcDriver_GetPotentiometerLevel(void)
{
    unsigned int result = 0;

#ifdef __32MZ2048ECG100__

    AD1CON3bits.RQCONVRT = 1;
    while (AD1DSTAT1bits.ARDY8 == 0);
    result = AD1DATA8;

#else

    DRV_ADC_Start();
    while (!DRV_ADC_SamplesAvailable(WIFIRE_POT_ADC_CH_INDEX))
        ;
    result = DRV_ADC_SamplesRead(WIFIRE_POT_ADC_CH_INDEX);
    DRV_ADC_Stop();

#endif

    return result;
}

uint32_t AdcDriver_GetTemperatureLevel(void)
{
    unsigned int result = 0;

// TODO - FIX TEMPERATURE READ
#ifdef __32MZ2048ECG100__

    AD1CON3bits.RQCONVRT = 1;
    while (AD1DSTAT1bits.ARDY8 == 0);
    result = AD1DATA8;

#else

    DRV_ADC_Start();
    while (!DRV_ADC_SamplesAvailable(WIFIRE_TEMPERATURE_ADC_CH_INDEX))
        ;
    result = DRV_ADC_SamplesRead(WIFIRE_TEMPERATURE_ADC_CH_INDEX);
    DRV_ADC_Stop();

#endif

    return result;
}

float AdcDriver_GetPotentiometerVoltage(void)
{
    float result;
    uint32_t level = AdcDriver_GetPotentiometerLevel();
    
    // Voltage = level/(12bit scale 4095) * 3.3V
    result = ((float)level * 3.3) / 4095;
    return result;
}

float AdcDriver_GetTemperatureDegrees(bool isCelcius)
{
    float result;
    if (isCelcius)
        result = 19.5;
    else
        result = 60.9;      // Fahrenheit

    return result;
}

