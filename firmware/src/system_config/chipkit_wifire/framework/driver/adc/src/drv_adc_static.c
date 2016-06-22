/*******************************************************************************
  ADC Driver Initialization File

  File Name:
    drv_adc_static_hs.c

  Summary:
    This file contains source code necessary to initialize the IC driver.

  Description:
    This file contains source code necessary to initialize the system.  It
    implements the "DRV_ADC_Initialize" function, configuration bits, and allocates
    any necessary global system resources, such as the systemObjects structure
    that contains the object handles to all the MPLAB Harmony module objects in
    the system.
 *******************************************************************************/

/*******************************************************************************
Copyright (c) 2013-2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Include Files
// *****************************************************************************
// *****************************************************************************
#include "framework/driver/adc/drv_adc_static.h"
 
// *****************************************************************************
// *****************************************************************************
// Section: ADC Static Driver Functions
// *****************************************************************************
// *****************************************************************************
void DRV_ADC_Initialize(void)
{
    /* Select Power Mode */
	PLIB_ADCHS_Setup(
	DRV_ADC_ID_1,
	ADCHS_VREF_AVDD_AVSS,
	ADCHS_CHARGEPUMP_DISABLE,
	ADCHS_OUTPUT_DATA_FORMAT_INTEGER,
	false,
	ADCHS_FAST_SYNC_SYSTEM_CLOCK_DISABLE,
	ADCHS_FAST_SYNC_PERIPHERAL_CLOCK_DISABLE,
	0,
	0,
	ADCHS_CLOCK_SOURCE_PBCLK,
	5,
	ADCHS_WARMUP_CLOCK_32768
	);




/* Setting for class-2 analog inputs */
	PLIB_ADCHS_AnalogInputModeSelect(
			DRV_ADC_ID_1,
			(ADCHS_AN_INPUT_ID)ADCHS_CLASS2_AN8,
			ADCHS_INPUT_MODE_SINGLE_ENDED_UNIPOLAR
			);


	PLIB_ADCHS_AnalogInputEdgeTriggerSet( DRV_ADC_ID_1, (ADCHS_CLASS12_AN_INPUT_ID)ADCHS_CLASS2_AN8 );

	PLIB_ADCHS_AnalogInputTriggerSourceSelect(
			DRV_ADC_ID_1,
			(ADCHS_CLASS12_AN_INPUT_ID)ADCHS_CLASS2_AN8,
			ADCHS_TRIGGER_SOURCE_GLOBAL_SOFTWARE_EDGE
	);


	PLIB_ADCHS_AnalogInputScanSetup
			(
				DRV_ADC_ID_1, 
				ADCHS_AN8, 
				ADCHS_SCAN_TRIGGER_SENSITIVE_EDGE,
				ADCHS_SCAN_TRIGGER_SOURCE_GLOBAL_SOFTWARE_EDGE
			);



	/* Include Analog input to Scan list */
	PLIB_ADCHS_AnalogInputScanSelect
	(
		DRV_ADC_ID_1,
		ADCHS_AN8
	);
	
	PLIB_ADCHS_AnalogInputModeSelect(
			DRV_ADC_ID_1,
			ADCHS_AN8,
			ADCHS_INPUT_MODE_SINGLE_ENDED_UNIPOLAR
			);	

			






    /* Enable ADC */
    PLIB_ADCHS_Enable(DRV_ADC_ID_1);
	
	/* Check Vref to be ready */
	while(!PLIB_ADCHS_VREFIsReady(DRV_ADC_ID_1));
	
	/* Check for Vref Fault */
	while(PLIB_ADCHS_VREFFaultHasOccurred(DRV_ADC_ID_1));	

	
	
	
	
}


inline void DRV_ADC_DeInitialize(void)
{
    /* Enable ADC */
    PLIB_ADCHS_Disable(DRV_ADC_ID_1);
}




inline void DRV_ADC_Start(void)
{
    /* Start ADC */
    //PLIB_ADCHS_SoftwareSamplingStart(DRV_ADC_ID_1);
	PLIB_ADCHS_GlobalSoftwareTriggerEnable(DRV_ADC_ID_1);
}

inline void DRV_ADC_Stop(void)
{
    /* Stop ADC */
    PLIB_ADCHS_SoftwareSamplingStop(DRV_ADC_ID_1);	
}

uint32_t DRV_ADC_SamplesRead(uint8_t bufIndex)
{
    /* Read Result */
    return PLIB_ADCHS_AnalogInputResultGet
	( 
		DRV_ADC_ID_1, 
		(ADCHS_AN0 + bufIndex)
	);
}

bool DRV_ADC_SamplesAvailable(uint8_t bufIndex)
{
    /* Check if data is available or not */
	return PLIB_ADCHS_AnalogInputDataIsReady
	(
		DRV_ADC_ID_1, 
		(ADCHS_AN0 + bufIndex)
	);
}





