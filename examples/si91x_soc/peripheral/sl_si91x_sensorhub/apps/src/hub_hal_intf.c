/*******************************************************************************
* @file  hub_hal_intf.c
* @brief 
*******************************************************************************
* # License
* <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
*******************************************************************************
*
* The licensor of this software is Silicon Laboratories Inc. Your use of this
* software is governed by the terms of Silicon Labs Master Software License
* Agreement (MSLA) available at
* www.silabs.com/about-us/legal/master-software-license-agreement. This
* software is distributed to you in Source Code format and is governed by the
* sections of the MSLA applicable to Source Code.
*
******************************************************************************/

/*
 * hub_hal_intf.c
 *
 *  Created on: Mar 14, 2023
 *      Author: kokuncha
 */
#include "hub_hal_intf.h"
#include "rsi_error.h"

uint32_t null_func(void)
{
  return RSI_OK;
}
int32_t gpio_create()
{
  return 0xff;
}
sl_sensor_impl_type sensor_impls[] = {
#ifdef CONFIG_SENSOR_INCLUDED_LIGHT
  {
    .type    = LIGHT_SENSOR_ID,
    .create  = light_sensor_create,
    .delete  = light_sensor_delete,
    .sample  = light_sensor_sample,
    .control = light_sensor_control,
  },
#endif
  {
    .type    = GPIO_SENSOR_ID,
    .create  = gpio_create,
    .delete  = null_func,
    .sample  = null_func,
    .control = null_func,
  },

};

uint32_t get_impl_size()
{
  return (sizeof(sensor_impls) / sizeof(sl_sensor_impl_type));
}
