/***************************************************************************/ /**
 * @file sl_si91x_button_config.h
 * @brief Button Driver Configuration
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************/

#ifndef SL_SI91X_BUTTON_CONFIG_H
#define SL_SI91X_BUTTON_CONFIG_H

#include "RTE_Device_917.h"

#define SL_SI91x_BUTTON_COUNT (2)

#define SL_BUTTON_BTN0_PIN    SL_SI91x_BUTTON0_PIN
#define SL_BUTTON_BTN0_PORT   SL_SI91x_BUTTON0_PORT
#define SL_BUTTON_BTN0_NUMBER SL_SI91x_BUTTON0_NUMBER

#define SL_BUTTON_BTN1_PIN    SL_SI91x_BUTTON1_PIN
#define SL_BUTTON_BTN1_PORT   SL_SI91x_BUTTON1_PORT
#define SL_BUTTON_BTN1_NUMBER SL_SI91x_BUTTON1_NUMBER
#define SL_BUTTON_BTN1_PAD    SL_SI91x_BUTTON1_PAD

#endif // SL_SI91X_BUTTON_CONFIG_H