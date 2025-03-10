/***************************************************************************/ /**
 * @file sl_si91x_adc.c
 * @brief ADC API implementation
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#include "sl_si91x_adc.h"
#include "clock_update.h"
#include "rsi_chip.h"
#include "rsi_bod.h"

/*******************************************************************************
 ***************************  DEFINES / MACROS   *******************************
 ******************************************************************************/
#define ICACHE2_ADDR_TRANSLATE_1_REG  *(volatile uint32_t *)(0x20280000 + 0x24)
#define MISC_CFG_SRAM_REDUNDANCY_CTRL *(volatile uint32_t *)(0x46008000 + 0x18)
#define MISC_CONFIG_MISC_CTRL1        *(volatile uint32_t *)(0x46008000 + 0x44)
#define SOC_PLL_LIMIT                 120000000  // Limit for soc pll clock.
#define MAX_SAMPLE_RATE               10000000   // Maximum sampling rate 5 Mbps.
#define MINIMUM_NUMBER_OF_CHANNEL     1          // Minimum number of channel enable
#define MAXIMUM_NUMBER_OF_CHANNEL     16         // Maximum number of channel enable
#define MAXIMUM_CHANNEL_ID            16         // Maximum adc dma support channel id.
#define ADC_PING_ENABLE               1          // Enable the ADC pong.
#define ADC_PONG_ENABLE               1          // Enable the ADC pong.
#define MAXIMUM_PING_LENGTH           1024       // Maximum ping length
#define MAXIMUM_PONG_LENGTH           1024       // Maximum pong length
#define MAXIMUM_PING_ADDR             4294967296 // Maximum ping address
#define MAXIMUM_PONG_ADDR             4294967296 // Maximum pong address
#define MAX_THRS_VAL                  7          // Maximum threshold value
#define MAX_POS_IN_SEL                31         // Maximum positive input selection
#define MAX_NEG_IN_SEL                15         // Maximum negative input selection
#define MINIMUM_SAMPLING_LENGTH       1          // Minimum sampling length
#define MAXIMUM_SAMPLING_LENGTH       1023       // Maximum sampling length
#define MINIMUM_REF_VOLT              1.8        // Maximum reference voltage
#define MAXMIMUM_REF_VOLT             3.6        // Maximum reference voltage
#define INVALID_POS_INPUT_1           9          // Invalid positive input selection 1
#define INVALID_POS_INPUT_2           19         // Invalid positive input selection 2
#define INVALID_NEG_INPUT_1           9          // Invalid negative input selection 1
#define MAXIMUM_POS_INPUT             24         // Maximum value of positive input selection
#define MAXIMUM_NEG_INPUT             13         // Maximum value of negative input selection
#define ADC_RELEASE_VERSION           0          // ADC Release version
#define ADC_SQA_VERSION               0          // ADC SQA version
#define ADC_DEV_VERSION               1          // ADC Developer version

/*******************************************************************************
 *************************** LOCAL VARIABLES   *******************************
 ******************************************************************************/
static sl_adc_callback_t user_callback = NULL;

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/
static sl_status_t convert_rsi_to_sl_error_code(rsi_error_t error);
static sl_status_t validate_adc_parameters(sl_adc_config_t *adc_config);
static sl_status_t validate_adc_channel_parameters(sl_adc_channel_config_t *adc_channel_config);
static sl_status_t validate_adc_thrld_parameters(sl_adc_fifo_thrld_config_t *adc_fifo_threshold);
static sl_status_t validate_adc_ext_trigger_parameters(sl_adc_external_config_t *adc_external_trigger);
static sl_status_t validate_adc_internal_parameters(sl_adc_internal_config_t *adc_internal_config);
static void callback_event_handler(uint8_t channel_no, uint8_t event);

/*******************************************************************************
 * To get the driver version
 *
 * @param[in] None
 * @return
 * *         returns driver version
*******************************************************************************/
sl_adc_version_t sl_si91x_adc_get_version(void)
{
  sl_adc_version_t version;
  version.release = ADC_RELEASE_VERSION;
  version.major   = ADC_SQA_VERSION;
  version.minor   = ADC_DEV_VERSION;
  return version;
}

/*******************************************************************************
 * To set the clock for ADC peripheral
 * Validates the clock frequency via validate_clock_parameters function
 * If the clock frequency is in range, it activates the clock for ADC peripheral
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_configure_clock(sl_adc_clock_config_t *clock_configuration)
{
  boolean_t adc_clock     = ENABLE;
  boolean_t odd_divfactor = 0;
  boolean_t swallo_enable = 0;
  sl_status_t status;
  rsi_error_t error_status;
  do {
    if (clock_configuration == NULL) {
      status = SL_STATUS_NULL_POINTER;
      break;
    }
    /*Default keep M4 in reference clock*/
    error_status = RSI_CLK_M4SocClkConfig(M4CLK, M4_ULPREFCLK, clock_configuration->division_factor);
    status       = convert_rsi_to_sl_error_code(error_status);
    if (status != SL_STATUS_OK) {
      break;
    }
    if (clock_configuration->soc_pll_clock >= SOC_PLL_LIMIT) {
      /*Configure the prefetch and registering when SOC clock is more than 120Mhz*/
      /*Configure the SOC PLL to 220MHz*/
      ICACHE2_ADDR_TRANSLATE_1_REG = BIT(21); //Icache registering when clk freq more than 120
      /*When set, enables registering in M4-TA AHB2AHB. This will have performance penalty. This has to be set above 100MHz*/
      MISC_CFG_SRAM_REDUNDANCY_CTRL = BIT(4);
      MISC_CONFIG_MISC_CTRL1 |= BIT(4); //Enable Register ROM as clock frequency is 200 Mhz
    }
    /*Configure the PLL frequency*/
    error_status =
      RSI_CLK_SetSocPllFreq(M4CLK, clock_configuration->soc_pll_clock, clock_configuration->soc_pll_reference_clock);
    status = convert_rsi_to_sl_error_code(error_status);
    if (status != SL_STATUS_OK) {
      break;
    }
    /*Switch M4 clock to PLL clock for speed operations*/
    error_status = RSI_CLK_M4SocClkConfig(M4CLK, M4_SOCPLLCLK, clock_configuration->division_factor);
    status       = convert_rsi_to_sl_error_code(error_status);
    if (status != SL_STATUS_OK) {
      break;
    }
    ROMAPI_M4SS_CLK_API->clk_qspi_clk_config(M4CLK,
                                             QSPI_SOCPLLCLK,
                                             swallo_enable,
                                             odd_divfactor,
                                             clock_configuration->division_factor);
    clock_configuration->division_factor = 1;
    /* Switch ULP Pro clock to 90 MHZ */
    RSI_ULPSS_ClockConfig(M4CLK, adc_clock, clock_configuration->division_factor, odd_divfactor);
    clock_configuration->division_factor = 0;
    /* This API is used to configure the ULPSS processor clock source*/
    RSI_ULPSS_UlpProcClkConfig(ULPCLK, ULP_PROC_SOC_CLK, clock_configuration->division_factor, &rsi_delay_ms);
  } while (false);
  return status;
}

/*******************************************************************************
 * Initialization of ADC
 * This API configure ADC sampling rate, enable adc peripheral and set the
 * adc peripheral clock. And registering callback function that user passing as
 * event parameter.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_init(sl_adc_channel_config_t adc_channel_config, sl_adc_config_t adc_config)
{
  sl_status_t status;
  rsi_error_t error_status;
  uint8_t ch_num;
  do {
    // Validate ADC parameters, if the parameters incorrect
    // If the status is not equal to SL_STATUS_OK, returns error code.
    status = validate_adc_parameters(&adc_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    // Validate ADC channel parameters, if the parameters incorrect
    // If the status is not equal to SL_STATUS_OK, returns error code.
    status = validate_adc_channel_parameters(&adc_channel_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    for (ch_num = 0; ch_num < adc_config.num_of_channel_enable; ch_num++) {
      //Validate sampling length is proper or not
      if ((adc_config.operation_mode == SL_ADC_STATIC_MODE)
          && (adc_channel_config.num_of_samples[0] > MINIMUM_SAMPLING_LENGTH)) {
        status = SL_STATUS_INVALID_COUNT;
        break;
      }
      // validate sample length, if it above 1023 it will return error code.
      if ((adc_channel_config.num_of_samples[ch_num] > MAXIMUM_SAMPLING_LENGTH)
          || (adc_channel_config.num_of_samples[ch_num] < MINIMUM_SAMPLING_LENGTH)) {
        status = SL_STATUS_INVALID_COUNT;
        break;
      }
      // Verify the user given positive input selection
      if ((adc_channel_config.pos_inp_sel[ch_num] == INVALID_POS_INPUT_1)
          || (adc_channel_config.pos_inp_sel[ch_num] == INVALID_POS_INPUT_2)
          || (adc_channel_config.pos_inp_sel[ch_num] > MAXIMUM_POS_INPUT)) {
        status = SL_STATUS_INVALID_RANGE;
        break;
      }
      // Verify the user given negative input selection
      if ((adc_channel_config.neg_inp_sel[ch_num] == INVALID_NEG_INPUT_1)
          || (adc_channel_config.neg_inp_sel[ch_num] > MAXIMUM_NEG_INPUT)) {
        status = SL_STATUS_INVALID_RANGE;
        break;
      }
    }
    if (status != SL_STATUS_OK) {
      break;
    }
    error_status = ADC_Init(adc_channel_config, adc_config, callback_event_handler);
    status       = convert_rsi_to_sl_error_code(error_status);
  } while (false);
  return status;
}

/*******************************************************************************
 * This API read the input supply of chip
 * If input supply to chip is less than 2.4V then switch input voltage supply
 * from SCDC to HPLDO.
 * This function will return chip voltage.
 ******************************************************************************/
float sl_si91x_adc_get_chip_voltage(void)
{
  volatile float chip_volt = 0;
  float max_ip_volt_scdc   = (float)2.4;
  //Get the input voltage of chip.
  chip_volt = RSI_BOD_SoftTriggerGetBatteryStatus();
  //If input to chip less than 2.4V then switch input voltage supply from SCDC to HPLDO
  if (chip_volt < max_ip_volt_scdc) {
    RSI_IPMU_ProgramConfigData(hp_ldo_voltsel);
  }
  return chip_volt;
}

/*******************************************************************************
 * To change the power mode of ADC
 * The following two modes are supported
 * ADC_POWER_ON
 * ADC_POWER_OFF
 * In power off mode, debug is disabled, to enable the debug DEBUGINIT() needs
 * to be called.
 * This function will return "SL_STATUS_OK" on successful execution.
 ******************************************************************************/
sl_status_t sl_si91x_adc_set_power_mode(POWER_STATE state)
{
  //set the power mode to ADC.
  RSI_ADC_PowerControl(state);
  return SL_STATUS_OK;
}

/*******************************************************************************
 * This API used for configure reference voltage for ADC operation.
 * Chip voltage will be 3.2V or 1.8V, If chip voltage to chip is 1.8V then
 * chip voltage(bypass LDO) act as reference voltage to analog peripheral.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_configure_reference_voltage(float vref_value, float chip_voltage)
{
  sl_status_t status;
  rsi_error_t error_status;
  // Validating reference voltage range.
  if ((vref_value < (float)MINIMUM_REF_VOLT) || (vref_value > (float)MAXMIMUM_REF_VOLT)) {
    status = SL_STATUS_INVALID_PARAMETER;
  } else {
    // Set analog reference voltage
    error_status = RSI_AUX_RefVoltageConfig(vref_value, chip_voltage);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To control and Configure the ADC channels.
 * It will configure Input type, operation mode, sampling rate, sampling length,
 * ping & pong address, number of channel enable, positive & negative channel
 * input selection.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_channel_set_configuration(sl_adc_channel_config_t adc_channel_config,
                                                   sl_adc_config_t adc_config)
{
  sl_status_t status;
  rsi_error_t error_status;
  do {
    // Validate ADC parameters, if the parameters incorrect
    // If the status is not equal to SL_STATUS_OK, returns error code.
    status = validate_adc_parameters(&adc_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    // Validate ADC channel parameters, if the parameters incorrect
    // If the status is not equal to SL_STATUS_OK, returns error code.
    status = validate_adc_channel_parameters(&adc_channel_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    // set the configuration for ADC channel.
    error_status = ADC_ChannelConfig(adc_channel_config, adc_config);
    status       = convert_rsi_to_sl_error_code(error_status);
  } while (false);
  return status;
}

/*******************************************************************************
 * To register the user event callback
 * It registers the callback, i.e., stores the callback function address
 * and pass to the variable that is called in Interrupt Handler.
 * If another callback is registered without unregistering previous callback then, it
 * returns an error code, so it is mandatory to unregister the callback before registering
 * another callback.
 * It will returns error if any callback is already registered.
 ******************************************************************************/
sl_status_t sl_si91x_adc_register_event_callback(sl_adc_callback_t callback_event)
{
  sl_status_t status;
  do {
    // Validate instance, if the parameters is NULL, it returns an error code.
    if (callback_event == NULL) {
      status = SL_STATUS_NULL_POINTER;
      break;
    }
    // To validate the function pointer if the parameters is not NULL then, it
    // returns an error code
    if (user_callback != NULL) {
      status = SL_STATUS_BUSY;
      break;
    }
    // User callback address is passed to the static variable which is called
    // at the time of interrupt
    user_callback = callback_event;
    // Returns SL_STATUS_OK if callback is successfully registered
    status = SL_STATUS_OK;
  } while (false);
  return status;
}

/*******************************************************************************
 * To unregister the user event callback
 * It unregisters the callback, i.e., clear the callback function address
 * and pass NULL value to the variable
 ******************************************************************************/
void sl_si91x_adc_unregister_event_callback(void)
{
  // Pass the NULL value to the static variable which is called at the time of
  // interrupt.
  // It is further validated in register callback API.
  user_callback = NULL;
}

/*******************************************************************************
 * To configure external trigger as ULP_timer, ULP_gpio,M4_CT based on this
 * it will detect edge and channel trigger selection will be happen.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_configure_external_trigger(sl_adc_external_config_t adc_external_trigger)
{
  sl_status_t status;
  rsi_error_t error_status;
  // Validate adc_external_trigger parameters.
  status = validate_adc_ext_trigger_parameters(&adc_external_trigger);
  if (status == SL_STATUS_OK) {
    error_status = RSI_ADC_ExtTrigConfig(AUX_ADC_DAC_COMP, adc_external_trigger);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To configure the ADC sampling rate for ADC channels.
 * It will set channel offset value and channel frequency for each channel to set the
 * sampling rate.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_configure_channel_sampling_rate(sl_adc_internal_config_t adc_internal_config,
                                                         uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  do {
    // Validate channel number.
    if (channel_num >= MAXIMUM_CHANNEL_ID) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Validate ADC internal parameters
    status = validate_adc_internal_parameters(&adc_internal_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    error_status = RSI_ADC_ChannelSamplingRate(AUX_ADC_DAC_COMP,
                                               channel_num,
                                               adc_internal_config.ch_offset_val[channel_num],
                                               adc_internal_config.ch_sampling_factor[channel_num]);
    status       = convert_rsi_to_sl_error_code(error_status);
  } while (false);
  return status;
}

/*******************************************************************************
 * To configure Ping and pong memory location along with length of
 * ping memory and pong memory.
 * ADC channel to be configured as 0,1,2 ...15
 * And enable ADC ping and ADC pong.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_configure_ping_pong_memory_address(sl_adc_internal_config_t adc_internal_config,
                                                            uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  do {
    // Validate channel number.
    if (channel_num >= MAXIMUM_CHANNEL_ID) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Validate ADC internal parameters
    status = validate_adc_internal_parameters(&adc_internal_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    // Configure ADC ping and pong memory address.
    error_status = RSI_ADC_PingPongMemoryAdrConfig(AUX_ADC_DAC_COMP,
                                                   channel_num,
                                                   adc_internal_config.ping_addr[channel_num],
                                                   adc_internal_config.pong_addr[channel_num],
                                                   adc_internal_config.ping_length[channel_num],
                                                   adc_internal_config.pong_length[channel_num],
                                                   ADC_PING_ENABLE,
                                                   ADC_PONG_ENABLE);
    status       = convert_rsi_to_sl_error_code(error_status);
  } while (false);
  return status;
}

/*******************************************************************************
 * To configure the fifo threshold to ADC.
 * It will set the value of aempty and afull threshold to respective register,
 * and we can set internal dma enable or not. if internal dma is enable
 * it will reading ADC samples from ADC fifo and writing them to ULP SRAM Memories.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_fifo_threshold_configuration(sl_adc_config_t adc_config,
                                                      sl_adc_fifo_thrld_config_t adc_fifo_threshold)
{
  sl_status_t status;
  rsi_error_t error_status;
  do {
    // Validate ADC parameters, if the parameters are incorrect
    // If the status is not equal to SL_STATUS_OK, returns error code.
    status = validate_adc_parameters(&adc_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    // Validate ADC fifo threshold parameters, if the parameters are incorrect
    // If the status is not equal to SL_STATUS_OK, returns error code.
    status = validate_adc_thrld_parameters(&adc_fifo_threshold);
    if (status != SL_STATUS_OK) {
      break;
    }
    error_status = RSI_ADC_Config(AUX_ADC_DAC_COMP,
                                  adc_fifo_threshold.num_of_channel_en,
                                  adc_config.operation_mode,
                                  adc_fifo_threshold.a_empty_threshold,
                                  adc_fifo_threshold.a_full_threshold,
                                  adc_fifo_threshold.dma_type);
    status       = convert_rsi_to_sl_error_code(error_status);
  } while (false);
  return status;
}

/*******************************************************************************
 * To compare threshold value with adc data
 * Threshold value to be programmed, it will set threshold value condition as
 * THRESHOLD1_EQ
 * THRESHOLD1_GRT
 * THRESHOLD1_LESS
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_threshold_configuration(sl_adc_threshold_config_t adc_threshold)
{
  sl_status_t status;
  rsi_error_t error_status;
  if ((adc_threshold.threshold1 > MAX_THRS_VAL) || (adc_threshold.threshold2 > MAX_THRS_VAL)) {
    status = SL_STATUS_INVALID_PARAMETER;
  } else {
    error_status = RSI_ADC_ThresholdConfig(AUX_ADC_DAC_COMP,
                                           (int16_t)adc_threshold.threshold1,
                                           adc_threshold.threshold1_cond,
                                           (int16_t)adc_threshold.threshold2,
                                           adc_threshold.threshold2_cond,
                                           adc_threshold.range);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To read the ADC samples.
 * This API can use when operation mode is on fifo mode and ulp memories.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_read_data(sl_adc_channel_config_t adc_channel_config, uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  uint8_t data_process          = 0;
  uint8_t ping_pong_memory_read = 0;
  do {
    // Validate channel number.
    if (channel_num >= MAXIMUM_CHANNEL_ID) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Validate ADC parameters, if the parameters are incorrect
    // If the status is not equal to SL_STATUS_OK, returns error code.
    status = validate_adc_channel_parameters(&adc_channel_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    data_process = 1;
    error_status = RSI_ADC_ReadData(adc_channel_config.rx_buf[channel_num],
                                    ping_pong_memory_read,
                                    channel_num,
                                    data_process,
                                    adc_channel_config.input_type[channel_num]);
    status       = convert_rsi_to_sl_error_code(error_status);
  } while (false);
  return status;
}

/*******************************************************************************
 * To read the ADC samples.
 * This API can use when static mode is enable as on operation mode.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_read_data_static(sl_adc_channel_config_t adc_channel_config,
                                          sl_adc_config_t adc_config,
                                          uint16_t *adc_value)
{
  sl_status_t status;
  uint8_t data_process     = 1;
  uint8_t chnl_num         = 0;
  int16_t read_static_data = 0;
  // Validate ADC parameters, if the parameters are incorrect
  // If the status is not equal to SL_STATUS_OK, returns error code.
  status = validate_adc_channel_parameters(&adc_channel_config);
  if (status == SL_STATUS_OK) {
    // Enable the gain and calculation on output samples.
    read_static_data = RSI_ADC_ReadDataStatic(AUX_ADC_DAC_COMP, data_process, adc_channel_config.input_type[0]);
  }
  *adc_value = (uint16_t)read_static_data;
  sl_si91x_adc_channel_interrupt_clear(adc_config, chnl_num);
  return status;
}

/*******************************************************************************
 * To configure the ADC in Static Mode
 * it can be programmed for setting input type, positive and negative
 * input channel selection.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_configure_static_mode(sl_adc_channel_config_t adc_channel_config, uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  do {
    // Validate channel number.
    if (channel_num >= MAXIMUM_CHANNEL_ID) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Validate ADC parameters, if the parameters are incorrect
    // If the status is not equal to SL_STATUS_OK, returns error code.
    status = validate_adc_channel_parameters(&adc_channel_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    error_status = RSI_ADC_StaticMode(AUX_ADC_DAC_COMP,
                                      adc_channel_config.pos_inp_sel[channel_num],
                                      adc_channel_config.neg_inp_sel[channel_num],
                                      adc_channel_config.input_type[channel_num]);
    status       = convert_rsi_to_sl_error_code(error_status);
  } while (false);
  return status;
}

/*******************************************************************************
 * To configure the ADC in FIFO Mode
 * it can be programmed for setting input type, positive and negative
 * input channel selection.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_configure_fifo_mode(sl_adc_channel_config_t adc_channel_config, uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  do {
    // Validate channel number.
    if (channel_num >= MAXIMUM_CHANNEL_ID) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Validate ADC parameters, if the parameters are incorrect
    // If the status is not equal to SL_STATUS_OK, returns error code.
    status = validate_adc_channel_parameters(&adc_channel_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    error_status = RSI_ADC_FifoMode(AUX_ADC_DAC_COMP,
                                    channel_num,
                                    adc_channel_config.pos_inp_sel[channel_num],
                                    adc_channel_config.neg_inp_sel[channel_num],
                                    adc_channel_config.input_type[channel_num]);
    status       = convert_rsi_to_sl_error_code(error_status);
  } while (false);
  return status;
}

/*******************************************************************************
 * To Enable or Disable Noise averaging mode for ADC.
 * The parameter sate should be boolean type it will enable or disable.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_set_noise_average_mode(boolean_t state)
{
  sl_status_t status;
  rsi_error_t error_status;
  error_status = RSI_ADC_NoiseAvgMode(AUX_ADC_DAC_COMP, state);
  status       = convert_rsi_to_sl_error_code(error_status);
  return status;
}

/*******************************************************************************
 * To Enable temperature_sensor.
 * It will enable BJT based Temperature sensor.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_temperature_sensor_enable(void)
{
  sl_status_t status;
  rsi_error_t error_status;
  error_status = RSI_ADC_TempSensorEnable(AUX_ADC_DAC_COMP);
  status       = convert_rsi_to_sl_error_code(error_status);
  return status;
}

/*******************************************************************************
 * To get the channel sampling rate value which is configured to ADC.
 * It will return sampling rate as uint32 type.
 ******************************************************************************/
uint32_t sl_si91x_adc_get_sampling_rate(uint8_t channel_num)
{
  return ADC_GetSamplingRate(channel_num);
}

/*******************************************************************************
 * Start the ADC operation.
 * It will start the operation mode as either static or dynamic(fifo) mode.
 * If it is fifo mode it will Enable Aux ADC Configuration.
 * This API will activates the ADC path in Aux ADC controller.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_start(sl_adc_config_t adc_config)
{
  sl_status_t status;
  rsi_error_t error_status;
  // Validate ADC parameters, if the parameters are incorrect
  // If the status is not equal to SL_STATUS_OK, returns error code.
  status = validate_adc_parameters(&adc_config);
  if (status == SL_STATUS_OK) {
    error_status = ADC_Start(adc_config);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * Stop the ADC operation.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_stop(sl_adc_config_t adc_config)
{
  sl_status_t status;
  rsi_error_t error_status;
  // Validate ADC parameters, if the parameters are incorrect
  // If the status is not equal to SL_STATUS_OK, returns error code.
  status = validate_adc_parameters(&adc_config);
  if (status == SL_STATUS_OK) {
    error_status = ADC_Stop(adc_config);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To enable internal DMA for corresponding ADC channels.
 * Internal DMA will be used for reading ADC samples from ADC FIFO and  writing
 * them to ULP SRAM Memories.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_internal_per_channel_dma_enable(uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  // Validate channel number.
  if (channel_num >= MAXIMUM_CHANNEL_ID) {
    status = SL_STATUS_INVALID_PARAMETER;
  } else {
    error_status = RSI_ADC_InternalPerChnlDmaEnable(AUX_ADC_DAC_COMP, channel_num);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To disable internal DMA for corresponding ADC channels.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_internal_per_channel_dma_disable(uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  // Validate channel number.
  if (channel_num >= MAXIMUM_CHANNEL_ID) {
    status = SL_STATUS_INVALID_PARAMETER;
  } else {
    error_status = RSI_ADC_InternalPerChnlDmaDisable(AUX_ADC_DAC_COMP, channel_num);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To enable ping pong for corresponding ADC channels.
 * ADC channel to be configured as 0,1,2 ...15
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_enable_ping_pong(uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  // Validate channel number.
  if (channel_num >= MAXIMUM_CHANNEL_ID) {
    status = SL_STATUS_INVALID_PARAMETER;
  } else {
    error_status = RSI_ADC_PingpongEnable(AUX_ADC_DAC_COMP, channel_num);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To disable ping pong for corresponding ADC channels.
 * ADC channel to be configured as 0,1,2 ...15
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_disable_ping_pong(uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  // Validate channel number.
  if (channel_num >= MAXIMUM_CHANNEL_ID) {
    status = SL_STATUS_INVALID_PARAMETER;
  } else {
    error_status = RSI_ADC_PingpongDisable(AUX_ADC_DAC_COMP, channel_num);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To enable ADC channel.
 * ADC channel to be configured as 0,1,2 ...15
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_channel_enable(uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  // Validate channel number.
  if (channel_num >= MAXIMUM_CHANNEL_ID) {
    status = SL_STATUS_INVALID_PARAMETER;
  } else {
    error_status = RSI_ADC_ChnlEnable(AUX_ADC_DAC_COMP, channel_num);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To disable ADC channel.
 * ADC channel to be configured as 0,1,2 ...15
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_channel_disable(uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  // Validate channel number.
  if (channel_num >= MAXIMUM_CHANNEL_ID) {
    status = SL_STATUS_INVALID_PARAMETER;
  } else {
    error_status = RSI_ADC_ChnlDisable(AUX_ADC_DAC_COMP, channel_num);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To read the external trigger status of ADC.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_get_external_trigger_status(sl_adc_external_config_t adc_external_trigger,
                                                     uint8_t *ext_trigger)
{
  sl_status_t status;
  // Validate adc_external_trigger parameters.
  status = validate_adc_ext_trigger_parameters(&adc_external_trigger);
  if (status == SL_STATUS_OK) {
    *ext_trigger = RSI_ADC_ExtTrigStatusRead(AUX_ADC_DAC_COMP, adc_external_trigger);
    status       = SL_STATUS_OK;
  }
  return status;
}

/*******************************************************************************
 * To clear the external trigger status.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_clear_external_trigger(sl_adc_external_config_t adc_external_trigger)
{
  sl_status_t status;
  // Validate adc_external_trigger parameters.
  status = validate_adc_ext_trigger_parameters(&adc_external_trigger);
  if (status == SL_STATUS_OK) {
    RSI_ADC_ExtTrigStatusClear(AUX_ADC_DAC_COMP, adc_external_trigger);
    // In rsi not returning RSI_OK, so that we passing SL_STATUS_OK here.
    status = SL_STATUS_OK;
  }
  return status;
}

/*******************************************************************************
 * To Unmask the ADC channel.
 * The interrupt will clear when static mode interrupt will detect, threshold
 * detection interrupt enable, fifo full interrupt enable.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_channel_interrupt_clear(sl_adc_config_t adc_config, uint8_t channel_num)
{
  sl_status_t status;
  rsi_error_t error_status;
  do {
    // Validate channel number.
    if (channel_num >= MAXIMUM_CHANNEL_ID) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Validate ADC parameters, if the parameters are incorrect
    // If the status is not equal to SL_STATUS_OK, returns error code.
    status = validate_adc_parameters(&adc_config);
    if (status != SL_STATUS_OK) {
      break;
    }
    error_status = RSI_ADC_ChnlIntrUnMask(AUX_ADC_DAC_COMP, channel_num, adc_config.operation_mode);
    status       = convert_rsi_to_sl_error_code(error_status);
  } while (false);
  return status;
}

/*******************************************************************************
 * To De-initialize the ADC.
 * This API will power off the ADC power control and
 * stop the ADc operation mode.
 * RSI errors are converted to the SL errors via convert_rsi_to_sl_error_code
 * function.
 ******************************************************************************/
sl_status_t sl_si91x_adc_deinit(sl_adc_config_t adc_config)
{
  sl_status_t status;
  rsi_error_t error_status;

  user_callback = NULL;
  // Validate ADC parameters, if the parameters are incorrect
  // If the status is not equal to SL_STATUS_OK, returns error code.
  status = validate_adc_parameters(&adc_config);
  if (status == SL_STATUS_OK) {
    error_status = ADC_Deinit(adc_config);
    status       = convert_rsi_to_sl_error_code(error_status);
  }
  return status;
}

/*******************************************************************************
 * To validate the RSI error code
 * While calling the RSI APIs, it returns the RSI Error codes.
 * This function converts the RSI error codes into SL error codes.
 * It takes argument as RSI error type and returns the SL error type.
 * It has a single switch statement which is mapped with the SL error code and
 * after successful conversion it breaks the switch statement.
 * If the error code is not listed, by default is SL_STATUS_FAIL.
 ******************************************************************************/
static sl_status_t convert_rsi_to_sl_error_code(rsi_error_t error)
{
  sl_status_t status;
  switch (error) {
    case RSI_OK:
      status = SL_STATUS_OK;
      break;
    case INVALID_PARAMETERS:
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    case ERROR_INVALID_INPUT_FREQUENCY:
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    case ERROR_CLOCK_NOT_ENABLED:
      status = SL_STATUS_NOT_INITIALIZED;
      break;
    case INVALID_SAMPLING_RATE:
      status = SL_STATUS_INVALID_RANGE;
      break;
    case INVALID_SAMPLE_LENGTH:
      status = SL_STATUS_INVALID_COUNT;
      break;
    case ERROR_ADC_INVALID_ARG:
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    case ERROR_PS_INVALID_STATE:
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    default:
      status = SL_STATUS_FAIL;
      break;
  }
  return status;
}

/*******************************************************************************
 * To validate the parameters ADC configuration and build a 32 bit integer
 * It takes pointer to ADC configuration structure and pointer to the uint32_t
 * as argument and builds the integer using the validations and 'OR' operation.
 * According to the values in ADC configuration structure, it performs the
 * 'OR' operation of the values.
 ******************************************************************************/
static sl_status_t validate_adc_parameters(sl_adc_config_t *adc_config)
{
  sl_status_t status;
  do {
    // Validates the ADC operation mode, if it is not either static or fifo mode it returns error code.
    if (adc_config->operation_mode >= SL_ADC_OPERATION_MODE_LAST) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // If the number of channel is not in range i.e., between 1 and 19, returns the error code.
    if ((adc_config->num_of_channel_enable < MINIMUM_NUMBER_OF_CHANNEL)
        || (adc_config->num_of_channel_enable > MAXIMUM_NUMBER_OF_CHANNEL)) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Returns SL_STATUS_OK if the parameter are appropriate
    status = SL_STATUS_OK;
  } while (false);
  return status;
}

/*******************************************************************************
 * To validate the parameters of ADC channel and build a 32 bit integer
 * It takes pointer to ADC channel configuration structure and pointer to the
 * uint32_t as argument and builds the integer using the validations and 'OR'
 * operation.
 * According to the values in adc channel configuration structure, it performs
 * the 'OR' operation of the values.
 ******************************************************************************/
static sl_status_t validate_adc_channel_parameters(sl_adc_channel_config_t *adc_channel_config)
{
  sl_status_t status;
  do {
    // Validate input type
    if (adc_channel_config->input_type[0] >= SL_ADC_INPUT_TYPE_LAST) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Verify the user given sampling rate is proper or not
    if (adc_channel_config->sampling_rate[0] > MAX_SAMPLE_RATE) {
      status = SL_STATUS_INVALID_RANGE;
      break;
    }
    // Returns SL_STATUS_OK if the parameter are appropriate
    status = SL_STATUS_OK;
  } while (false);
  return status;
}

/*******************************************************************************
 * To validate the parameters of ADC external trigger and build a 32 bit integer
 * It takes pointer to ADC external trigger configuration structure and pointer
 * to the uint32_t as argument and builds the integer using the validations and
 * 'OR' operation.
 * According to the values in external trigger configuration structure, it
 * performs the 'OR' operation of the values.
 ******************************************************************************/
static sl_status_t validate_adc_ext_trigger_parameters(sl_adc_external_config_t *adc_external_trigger)
{
  sl_status_t status;
  do {
    //Validate ADC external trigger type.
    if (adc_external_trigger->trigger_type >= SL_ADC_EXT_TRIGGER_TYPE_LAST) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Validate ADC external trigger detection and edge trigger.
    if ((adc_external_trigger->trigger_num >= SL_ADC_EXT_TRIGGER_LAST)
        || (adc_external_trigger->detection_edge_sel >= SL_ADC_EXT_TRIGGER_EDGE_LAST)) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Validate ADC external trigger selection and channel_id.
    if ((adc_external_trigger->trigger_sel_val >= MAXIMUM_CHANNEL_ID)
        || (adc_external_trigger->trigger_sel >= SL_ADC_EXT_TRIGGER_SEL_LAST)) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Returns SL_STATUS_OK if the parameter are appropriate
    status = SL_STATUS_OK;
  } while (false);
  return status;
}

/*******************************************************************************
 * To validate the parameters of ADC internal parameters and build a 32 bit integer
 * It takes pointer to ADC internal parameters configuration structure and pointer
 * to the uint32_t as argument and builds the integer using the validations and
 * 'OR' operation.
 * According to the values in internal parameters configuration structure, it
 * performs the 'OR' operation of the values.
 ******************************************************************************/
static sl_status_t validate_adc_internal_parameters(sl_adc_internal_config_t *adc_internal_config)
{
  sl_status_t status;
  do {
    // Validate ADC input type.
    if (adc_internal_config->input_type[0] >= SL_ADC_INPUT_TYPE_LAST) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Verify ADC sampling length with maximum and minimum value.
    if ((adc_internal_config->num_of_samples[0] < MINIMUM_SAMPLING_LENGTH)
        || (adc_internal_config->num_of_samples[0] > MAXIMUM_SAMPLING_LENGTH)) {
      status = SL_STATUS_INVALID_COUNT;
      break;
    }
    // Validate ping and pong address maximum value.
    if ((adc_internal_config->ping_addr >= (uint32_t *)MAXIMUM_PING_ADDR)
        || (adc_internal_config->pong_addr >= (uint32_t *)MAXIMUM_PONG_ADDR)) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    //       Validate ping and pong length maximum value
    if ((adc_internal_config->ping_length[0] >= MAXIMUM_PING_LENGTH)
        || (adc_internal_config->pong_length[0] >= MAXIMUM_PONG_LENGTH)) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Returns SL_STATUS_OK if the parameter are appropriate
    status = SL_STATUS_OK;
  } while (false);
  return status;
}

/*******************************************************************************
 * To validate the parameters of ADC fifo threshold and build a 32 bit integer
 * It takes pointer to fifo threshold configuration structure and pointer to
 * the uint32_t as argument and builds the integer using the validations and
 * 'OR' operation.
 * According to the values in adc fifo threshold configuration structure, it
 * performs the 'OR' operation of the values.
 ******************************************************************************/
static sl_status_t validate_adc_thrld_parameters(sl_adc_fifo_thrld_config_t *adc_fifo_threshold)
{
  sl_status_t status;
  do {
    // Validate DMA type
    if (adc_fifo_threshold->dma_type >= SL_ADC_DMA_TYPE_LAST) {
      status = SL_STATUS_INVALID_PARAMETER;
      break;
    }
    // Verify the user given channel type
    if (adc_fifo_threshold->num_of_channel_en >= SL_ADC_CHANNEL_TYPE_LAST) {
      status = SL_STATUS_INVALID_RANGE;
      break;
    }
    // Returns SL_STATUS_OK if the parameter are appropriate
    status = SL_STATUS_OK;
  } while (false);
  return status;
}

/*******************************************************************************
 * Static callback function for handling the callback events.
 * When ADC operation mode in STATIC the ADC_STATIC_MODE_CALLBACK event
 * will trigger and unmask the adc_channel.
 ******************************************************************************/
static void callback_event_handler(uint8_t channel_no, uint8_t event)
{
  switch (event) {
    case SL_INTERNAL_DMA:
      user_callback(channel_no, SL_INTERNAL_DMA);
      break;
    case SL_ADC_STATIC_MODE_EVENT:
      user_callback(channel_no, SL_ADC_STATIC_MODE_EVENT);
      break;
  }
}
