/***************************************************************************/ /**
 * @file ulp_uart_example.c
 * @brief Ulp UART examples functions
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
#include "sl_si91x_usart.h"
#include "rsi_board.h"
#include "ulp_uart_example.h"
#include "rsi_egpio.h"
#include "rsi_rom_egpio.h"
/*******************************************************************************
 ***************************  Defines / Macros  ********************************
 ******************************************************************************/
#define RESERVED_IRQ_COUNT   16                                   // Reserved IRQ count
#define EXT_IRQ_COUNT        98                                   // External IRQ count
#define VECTOR_TABLE_ENTRIES (RESERVED_IRQ_COUNT + EXT_IRQ_COUNT) // Vector table entries
#define BUFFER_SIZE          1024                                 // Data send and receive length
#define USART_BAUDRATE       115200                               // Baud rate <9600-7372800>
#define PORT                 0
#define PIN                  10
#define SET                  1
#define CLR                  0
#define NON_UC_DEFAULT_CONFIG \
  0 //  Enable this macro to set the default configurations in non_uc case, this is useful when someone don't want to use UC configuration

/*******************************************************************************
 *************************** LOCAL VARIABLES   *******************************
 ******************************************************************************/
static uint8_t data_in[BUFFER_SIZE];
static uint8_t data_out[BUFFER_SIZE];

volatile boolean_t send_complete = false, transfer_complete = false, receive_complete = false;
static boolean_t begin_transmission = true;

/*******************************************************************************
 **********************  Local Function prototypes   ***************************
 ******************************************************************************/
void callback_event(uint32_t event);
static void compare_loop_back_data(void);
/*******************************************************************************
 **************************   GLOBAL VARIABLES   *******************************
 ******************************************************************************/
sl_usart_handle_t usart_handle;
usart_mode_enum_t current_mode = SL_SEND_DATA;

uint32_t ramVector[VECTOR_TABLE_ENTRIES] __attribute__((aligned(256)));
extern void hardware_setup(void);
/*******************************************************************************
 * USART Example Initialization function
 ******************************************************************************/
void usart_example_init(void)
{
  /* At this stage the micro-controller clock setting is already configured,
     * this is done through SystemInit() function which is called from startup
     * file (startup_rs1xxxx.s) before to branch to application main.
     * To reconfigure the default setting of SystemInit() function, refer to
     * startup_rs1xxxx.c file
     */
  //copying the vector table from flash to ram
  memcpy(ramVector, (uint32_t *)SCB->VTOR, sizeof(uint32_t) * VECTOR_TABLE_ENTRIES);
  // Assigning the ram vector address to VTOR register
  SCB->VTOR = (uint32_t)ramVector;
  // Switching MCU from PS4 to PS2 state(low power state)
  // In this mode, whatever be the timer clock source value, it will run with 20MHZ only
  // To use usart in high power mode, don't call hardware_setup()

  hardware_setup();
  sl_status_t status;
  sl_si91x_usart_control_config_t usart_config;
  sl_si91x_usart_control_config_t get_config;
#if NON_UC_DEFAULT_CONFIG
  usart_config.baudrate      = USART_BAUDRATE;
  usart_config.mode          = SL_USART_MODE_ASYNCHRONOUS;
  usart_config.parity        = SL_USART_NO_PARITY;
  usart_config.stopbits      = SL_USART_STOP_BITS_1;
  usart_config.hwflowcontrol = SL_USART_FLOW_CONTROL_NONE;
  usart_config.databits      = SL_USART_DATA_BITS_8;
  usart_config.misc_control  = SL_USART_MISC_CONTROL_NONE;
  usart_config.usart_module  = ULPUART;
  usart_config.config_enable = ENABLE;
  usart_config.synch_mode    = DISABLE;
#endif
  //Set pin 0 in GPIO mode
  RSI_EGPIO_SetPinMux(EGPIO1, PORT, PIN, EGPIO_PIN_MUX_MODE0);
  //Set output direction
  RSI_EGPIO_SetDir(EGPIO1, PORT, PIN, EGPIO_CONFIG_DIR_OUTPUT);
  do {
    // Initialize the UART
    status = sl_si91x_usart_init(ULPUART, &usart_handle);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_usart_initialize: Error Code : %lu \n", status);
      break;
    }
    DEBUGOUT("USART initialization is successful \n");
    // Configure the USART configurations
    status = sl_si91x_usart_set_configuration(usart_handle, &usart_config);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_usart_set_configuration: Error Code : %lu \n", status);
      break;
    }
    DEBUGOUT("USART configuration is successful \n");
    // Register user callback function
    status = sl_si91x_usart_register_event_callback(callback_event);
    if (status != SL_STATUS_OK) {
      DEBUGOUT("sl_si91x_usart_register_event_callback: Error Code : %lu \n", status);
      break;
    }
    DEBUGOUT("USART user event callback registered successfully \n");
    sl_si91x_usart_get_configurations(ULPUART, &get_config);
    DEBUGOUT("Baud Rate = %d \n", get_config.baudrate);
  } while (false);
}

/*******************************************************************************
 * Example ticking function
 ******************************************************************************/
void usart_example_process_action(void)
{
  sl_status_t status;
  // In this switch case, according to the macros enabled in header file, it starts to execute the APIs
  // Assuming all the macros are enabled, after transfer, receive will be executed and after receive
  // send will be executed.
  switch (current_mode) {
#if (SL_USART_SYNCH_MODE != ENABLE)
    case SL_SEND_DATA:
      // Validation for executing the API only once
      if (begin_transmission == true) {
        // Fill the data buffer to be send
        for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
          data_out[i] = (uint8_t)(i + 1);
        }
        status = sl_si91x_usart_send_data(usart_handle, data_out, sizeof(data_out));
        if (status != SL_STATUS_OK) {
          // If it fails to execute the API, it will not execute rest of the things
          DEBUGOUT("sl_si91x_usart_send_data: Error Code : %lu \n", status);
          current_mode = SL_TRANSMISSION_COMPLETED;
          break;
        }
        begin_transmission = false;
      }

      send_complete = false;

      if (USE_RECEIVE) {
        // If receive macro is enabled, current mode is set to receive
        current_mode       = SL_RECEIVE_DATA;
        begin_transmission = true;
        break;
      }
      DEBUGOUT("USART send completed successfully \n");
      // Current mode is set to complete
      current_mode = SL_TRANSMISSION_COMPLETED;
      break;

    case SL_RECEIVE_DATA:
      if (begin_transmission == true) {
        // Validation for executing the API only once
        status = sl_si91x_usart_receive_data(usart_handle, data_in, sizeof(data_in));
        if (status != SL_STATUS_OK) {
          // If it fails to execute the API, it will not execute rest of the things
          DEBUGOUT("sl_si91x_usart_receive_data: Error Code : %lu \n", status);
          current_mode = SL_TRANSMISSION_COMPLETED;
          break;
        }
        DEBUGOUT("USART receive begin successfully \n");
        begin_transmission = false;
      }
      //Waiting till the receive is completed
      if (receive_complete) {
        // Update the receive compelete flag with 0.
        receive_complete = false;
        if (USE_SEND) {
          // If send macro is enabled, current mode is set to send
          current_mode       = SL_SEND_DATA;
          begin_transmission = true;
          DEBUGOUT("USART receive completed \n");
          break;
        }

        DEBUGOUT("USART send completed successfully \n");
        DEBUGOUT("USART receive completed \n");
        compare_loop_back_data();
        // If send macro is not enabled, current mode is set to completed.
        current_mode = SL_TRANSMISSION_COMPLETED;
      }
      break;

    case SL_TRANSFER_DATA:
#else
    // Validation for executing the API only once
    if (begin_transmission) {
      // Fill the data buffer to be send
      for (int i = 0; i < BUFFER_SIZE; i++) {
        data_out[i] = (uint8_t)i + 1;
      }

      status = sl_si91x_usart_transfer_data(usart_handle, data_out, data_in, sizeof(data_out));
      if (status != SL_STATUS_OK) {
        // If it fails to execute the API, it will not execute rest of the things
        DEBUGOUT("sl_si91x_usart_transfer_data: Error Code : %lu \n", status);
        current_mode = completed;
        break;
      }
      DEBUGOUT("USART transfer begin successfully \n");
      begin_transmission = false;
    }

    //Waiting till the transfer is completed
    if (transfer_complete) {
      // Update transfer_complete flag with 0.
      transfer_complete = false;
      // At last current mode is set to completed.
      current_mode = SL_TRANSMISSION_COMPLETED;
    }
    DEBUGOUT("USART transfer completed \n");
#endif
      break;

    case SL_TRANSMISSION_COMPLETED:
      break;
  }
}

/*******************************************************************************
 * Compares data received buffer via USART with data transfer buffer and print
 * the loopback test passed or failed
 ******************************************************************************/
static void compare_loop_back_data(void)
{
  uint16_t data_index = 0;
  // Check for data in and data out are same, if same then comparision
  // will continue till end of the buffer
  for (data_index = 0; data_index < BUFFER_SIZE; data_index++) {
    if (data_in[data_index] != data_out[data_index]) {
      break;
    }
  }
  if (data_index == BUFFER_SIZE) {
    RSI_EGPIO_SetPin(EGPIO1, PORT, PIN, SET);
    RSI_EGPIO_SetPin(EGPIO1, PORT, PIN, CLR);
    DEBUGOUT("Data comparison successful, Loop Back Test Passed \n");
  } else {
    DEBUGOUT("Data comparison failed, Loop Back Test failed \n");
  }
}

/*******************************************************************************
 * Callback function triggered on data Transfer and reception
 ******************************************************************************/
void callback_event(uint32_t event)
{
  switch (event) {
    case SL_USART_EVENT_SEND_COMPLETE:
      send_complete = true;
      break;
    case SL_USART_EVENT_RECEIVE_COMPLETE:
      receive_complete = true;
      break;
    case SL_USART_EVENT_TRANSFER_COMPLETE:
      transfer_complete = true;
      break;
  }
}
