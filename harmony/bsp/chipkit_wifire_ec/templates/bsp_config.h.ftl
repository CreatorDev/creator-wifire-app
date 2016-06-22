<#--
/*******************************************************************************
  BSP Freemarker Template File

  Company:
    Microchip Technology Inc.

  File Name:
   bsp_config.h.ftl

  Summary:
    BSP Freemarker Template File

  Description:

*******************************************************************************/

/*******************************************************************************
Copyright (c) 2014 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS  WITHOUT  WARRANTY  OF  ANY  KIND,
EITHER EXPRESS  OR  IMPLIED,  INCLUDING  WITHOUT  LIMITATION,  ANY  WARRANTY  OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A  PARTICULAR  PURPOSE.
IN NO EVENT SHALL MICROCHIP OR  ITS  LICENSORS  BE  LIABLE  OR  OBLIGATED  UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION,  BREACH  OF  WARRANTY,  OR
OTHER LEGAL  EQUITABLE  THEORY  ANY  DIRECT  OR  INDIRECT  DAMAGES  OR  EXPENSES
INCLUDING BUT NOT LIMITED TO ANY  INCIDENTAL,  SPECIAL,  INDIRECT,  PUNITIVE  OR
CONSEQUENTIAL DAMAGES, LOST  PROFITS  OR  LOST  DATA,  COST  OF  PROCUREMENT  OF
SUBSTITUTE  GOODS,  TECHNOLOGY,  SERVICES,  OR  ANY  CLAIMS  BY  THIRD   PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE  THEREOF),  OR  OTHER  SIMILAR  COSTS.
*******************************************************************************/
-->
<#if CONFIG_BSP_CK_WIFIRE_EC_LED1 == true>
<#if CONFIG_BSP_CK_WIFIRE_EC_LED_LD1_NAME != "BSP_LED_1">
#define ${CONFIG_BSP_CK_WIFIRE_EC_LED_LD1_NAME} BSP_LED_1
</#if>
</#if>
<#if CONFIG_BSP_CK_WIFIRE_EC_LED2 == true>
<#if CONFIG_BSP_CK_WIFIRE_EC_LED_LD2_NAME != "BSP_LED_2">
#define ${CONFIG_BSP_CK_WIFIRE_EC_LED_LD2_NAME} BSP_LED_2
</#if>
</#if>
<#if CONFIG_BSP_CK_WIFIRE_EC_LED3 == true>
<#if CONFIG_BSP_CK_WIFIRE_EC_LED_LD3_NAME != "BSP_LED_3">
#define ${CONFIG_BSP_CK_WIFIRE_EC_LED_LD3_NAME} BSP_LED_3
</#if>
</#if>
<#if CONFIG_BSP_CK_WIFIRE_EC_LED4 == true>
<#if CONFIG_BSP_CK_WIFIRE_EC_LED_LD4_NAME != "BSP_LED_4">
#define ${CONFIG_BSP_CK_WIFIRE_EC_LED_LD4_NAME} BSP_LED_4
</#if>
</#if>
<#if CONFIG_BSP_CK_WIFIRE_EC_SW1 == true>
<#if CONFIG_BSP_CK_WIFIRE_EC_SW1_NAME != "BSP_SWITCH_1">
#define ${CONFIG_BSP_CK_WIFIRE_EC_SW1_NAME} BSP_SWITCH_1
</#if>
</#if>
<#if CONFIG_BSP_CK_WIFIRE_EC_SW2 == true>
<#if CONFIG_BSP_CK_WIFIRE_EC_SW2_NAME != "BSP_SWITCH_2">
#define ${CONFIG_BSP_CK_WIFIRE_EC_SW2_NAME} BSP_SWITCH_2
</#if>
</#if>
<#--
/*******************************************************************************
 End of File
*/
-->

