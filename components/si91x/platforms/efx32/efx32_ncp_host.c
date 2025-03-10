/*******************************************************************************
* @file  efx32_ncp_host.c
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

#include "sl_wifi_constants.h"
#include "sl_si91x_host_interface.h"
#include "sl_board_configuration.h"
#include "sl_status.h"
#include "os_tick.h"   // CMSIS RTOS2
#include "cmsis_os2.h" // CMSIS RTOS2
#include "em_gpio.h"
#include "em_cmu.h"
#include "gpiointerrupt.h"
#include "sl_si91x_status.h"
#include "sl_constants.h"
#include <stdbool.h>
#include <string.h>
#include "FreeRTOS.h"
#include <semphr.h>

#define VERIFY_STATUS(s)   \
  do {                     \
    if (s != SL_STATUS_OK) \
      return s;            \
  } while (0);

#ifndef SL_WIFI_BOARD_READY_WAIT_TIME
#define SL_WIFI_BOARD_READY_WAIT_TIME \
  40000 //some scenarios like after firmware upgrade, it will take 40 seconds to boad ready
#endif

typedef struct {
  sl_wifi_buffer_t *head;
  sl_wifi_buffer_t *tail;
  osMutexId_t mutex;
  uint32_t flag;
} si91x_packet_queue_t;

osEventFlagsId_t si91x_events       = 0;
osEventFlagsId_t si91x_bus_events   = 0;
osEventFlagsId_t si91x_async_events = 0;
osThreadId_t si91x_thread           = 0;
osMutexId_t si91x_bus_mutex         = 0;
osThreadId_t si91x_event_thread     = 0;
osMutexId_t malloc_free_mutex       = 0;

static si91x_packet_queue_t cmd_queues[SI91X_QUEUE_MAX];

extern void si91x_bus_thread(void *args);
extern void si91x_event_handler_thread(void *args);
static void gpio_interrupt(uint8_t interrupt_number);
sl_status_t sli_verify_device_boot(uint32_t *rom_version);
sl_status_t sli_wifi_select_option(const uint8_t configuration);

void sl_si91x_host_set_sleep_indicator(void)
{
  GPIO_PinOutSet(SLEEP_CONFIRM_PIN.port, SLEEP_CONFIRM_PIN.pin);
}

void sl_si91x_host_clear_sleep_indicator(void)
{
  GPIO_PinOutClear(SLEEP_CONFIRM_PIN.port, SLEEP_CONFIRM_PIN.pin);
}

uint32_t sl_si91x_host_get_wake_indicator(void)
{
  return GPIO_PinInGet(WAKE_INDICATOR_PIN.port, WAKE_INDICATOR_PIN.pin);
}

sl_status_t sl_si91x_host_init(void)
{
  int i;
  sl_status_t status = SL_STATUS_OK;

  status = sl_si91x_host_bus_init();
  VERIFY_STATUS(status);

  // Start reset line low
  GPIO_PinModeSet(RESET_PIN.port, RESET_PIN.pin, gpioModePushPull, 0);

  // configure packet pending interrupt priority
  NVIC_SetPriority(GPIO_ODD_IRQn, PACKET_PENDING_INT_PRI);

  // Configure interrupt, sleep and wake confirmation pins
  GPIOINT_CallbackRegister(INTERRUPT_PIN.pin, gpio_interrupt);
  GPIO_PinModeSet(INTERRUPT_PIN.port, INTERRUPT_PIN.pin, gpioModeInputPullFilter, 0);
  GPIO_ExtIntConfig(INTERRUPT_PIN.port, INTERRUPT_PIN.pin, INTERRUPT_PIN.pin, true, false, true);
  GPIO_PinModeSet(SLEEP_CONFIRM_PIN.port, SLEEP_CONFIRM_PIN.pin, gpioModeWiredOrPullDown, 1);
  GPIO_PinModeSet(WAKE_INDICATOR_PIN.port, WAKE_INDICATOR_PIN.pin, gpioModeWiredOrPullDown, 0);

  memset(cmd_queues, 0, sizeof(cmd_queues));

  if (NULL == si91x_events) {
    si91x_events = osEventFlagsNew(NULL);
  }

  if (NULL == si91x_bus_events) {
    si91x_bus_events = osEventFlagsNew(NULL);
  }

  if (NULL == si91x_async_events) {
    si91x_async_events = osEventFlagsNew(NULL);
  }

  if (NULL == si91x_thread) {
    const osThreadAttr_t attr = {

      .name       = "si91x_bus",
      .priority   = osPriorityRealtime,
      .stack_mem  = 0,
      .stack_size = 1536,
      .cb_mem     = 0,
      .cb_size    = 0,
      .attr_bits  = 0u,
      .tz_module  = 0u,
    };
    si91x_thread = osThreadNew(si91x_bus_thread, NULL, &attr);
  }

  if (NULL == si91x_event_thread) {
    const osThreadAttr_t attr = {
      .name       = "si91x_event",
      .priority   = osPriorityNormal,
      .stack_mem  = 0,
      .stack_size = 1536,
      .cb_mem     = 0,
      .cb_size    = 0,
      .attr_bits  = 0u,
      .tz_module  = 0u,
    };
    si91x_event_thread = osThreadNew(si91x_event_handler_thread, NULL, &attr);
  }

  if (NULL == si91x_bus_mutex) {
    si91x_bus_mutex = osMutexNew(NULL);
  }

  for (i = 0; i < SI91X_QUEUE_MAX; i++) {
    cmd_queues[i].head  = NULL;
    cmd_queues[i].tail  = NULL;
    cmd_queues[i].mutex = osMutexNew(NULL);
    // Initialize corresponding queue with its unique bit mask value based on queueid
    // to avoid performing left shift ooperation every time when queue status api is
    // called(Which happens very frequently in bus thread)
    cmd_queues[i].flag = (1 << i);
  }
  malloc_free_mutex = osMutexNew(NULL);
  return status;
}

sl_status_t sl_si91x_host_deinit(void)
{
  int i;

  // Deallocate all threads, mutexes and event handlers
  if (NULL != si91x_thread) {
    osThreadTerminate(si91x_thread);
    si91x_thread = NULL;
  }

  if (NULL != si91x_event_thread) {
    osThreadTerminate(si91x_event_thread);
    si91x_event_thread = NULL;
  }

  if (NULL != si91x_events) {
    osEventFlagsDelete(si91x_events);
    si91x_events = NULL;
  }

  if (NULL != si91x_bus_events) {
    osEventFlagsDelete(si91x_bus_events);
    si91x_bus_events = NULL;
  }

  if (NULL != si91x_async_events) {
    osEventFlagsDelete(si91x_async_events);
    si91x_async_events = NULL;
  }

  if (NULL != si91x_bus_mutex) {
    osMutexDelete(si91x_bus_mutex);
    si91x_bus_mutex = NULL;
  }

  for (i = 0; i < SI91X_QUEUE_MAX; i++) {
    osMutexDelete(cmd_queues[i].mutex);
    cmd_queues[i].mutex = NULL;
  }
  osMutexDelete(malloc_free_mutex);
  malloc_free_mutex = NULL;
  return SL_STATUS_OK;
}

sl_si91x_host_timestamp_t sl_si91x_host_get_timestamp(void)
{
  return osKernelGetTickCount();
}

sl_si91x_host_timestamp_t sl_si91x_host_elapsed_time(uint32_t starting_timestamp)
{
  return (osKernelGetTickCount() - starting_timestamp);
}

void sl_si91x_host_delay_ms(uint32_t delay_milliseconds)
{
  osDelay(delay_milliseconds);
}

void sl_si91x_host_set_event(uint32_t event_mask)
{
  osEventFlagsSet(si91x_events, event_mask);
}

void sl_si91x_host_set_bus_event(uint32_t event_mask)
{
  osEventFlagsSet(si91x_bus_events, event_mask);
}

void sl_si91x_host_set_async_event(uint32_t event_mask)
{
  osEventFlagsSet(si91x_async_events, event_mask);
}

/*Note: This function is only used to queue the responses in to corresponding response queues*/
sl_status_t sl_si91x_host_add_to_queue(sl_si91x_queue_type_t queue, sl_wifi_buffer_t *buffer)
{
  sl_wifi_buffer_t *packet = (sl_wifi_buffer_t *)buffer;
  osMutexAcquire(cmd_queues[queue].mutex, 0xFFFFFFFFUL);
  packet->node.node = NULL;

  if (cmd_queues[queue].tail == NULL) {
    cmd_queues[queue].head = packet;
    cmd_queues[queue].tail = packet;
  } else {
    cmd_queues[queue].tail->node.node = (sl_slist_node_t *)packet;
    cmd_queues[queue].tail            = packet;
  }

  osMutexRelease(cmd_queues[queue].mutex);
  return SL_STATUS_OK;
}

/*Note: This function is only used to queue the Commands in to corresponding command queues*/
sl_status_t sl_si91x_host_add_to_queue_with_atomic_action(sl_si91x_queue_type_t queue,
                                                          sl_wifi_buffer_t *buffer,
                                                          void *user_data,
                                                          sl_si91x_host_atomic_action_function_t handler)
{
  sl_wifi_buffer_t *packet = (sl_wifi_buffer_t *)buffer;

  osMutexAcquire(cmd_queues[queue].mutex, 0xFFFFFFFFUL);
  if (NULL != handler) {
    handler(user_data);
  }
  packet->node.node = NULL;

  if (cmd_queues[queue].tail == NULL) {
    cmd_queues[queue].head = packet;
    cmd_queues[queue].tail = packet;
  } else {
    cmd_queues[queue].tail->node.node = (sl_slist_node_t *)packet;
    cmd_queues[queue].tail            = packet;
  }

  osMutexRelease(cmd_queues[queue].mutex);
  return SL_STATUS_OK;
}

sl_status_t sl_si91x_semaphore_create(sl_si91x_semaphore_handle_t *semaphore, uint32_t count)
{
  UNUSED_PARAMETER(count); //This statement is added only to resolve compilation warning, value is unchanged
  SemaphoreHandle_t *p_semaphore = NULL;
  p_semaphore                    = (SemaphoreHandle_t *)semaphore;

  if (semaphore == NULL) {
    return RSI_ERROR_IN_OS_OPERATION;
  }
  *p_semaphore = xSemaphoreCreateBinary();

  if (*p_semaphore == NULL) {
    return RSI_ERROR_IN_OS_OPERATION;
  }
  return RSI_ERROR_NONE;
}

sl_status_t sl_si91x_semaphore_wait(sl_si91x_semaphore_handle_t *semaphore, uint32_t timeout_ms)
{
  SemaphoreHandle_t *p_semaphore = NULL;
  p_semaphore                    = (SemaphoreHandle_t *)semaphore;

  if (semaphore == NULL || *p_semaphore == NULL) //Note : FreeRTOS porting
  {
    return RSI_ERROR_INVALID_PARAM;
  }
  if (!timeout_ms) {
    timeout_ms = 0xffffffff;
  }
  if (xSemaphoreTake(*p_semaphore, timeout_ms) == 0) {
    return RSI_ERROR_NONE;
  }
  return RSI_ERROR_IN_OS_OPERATION;
}

sl_status_t sl_si91x_semaphore_post(sl_si91x_semaphore_handle_t *semaphore)
{
  SemaphoreHandle_t *p_semaphore = NULL;
  p_semaphore                    = (SemaphoreHandle_t *)semaphore;

  if (semaphore == NULL || *p_semaphore == NULL) //Note : FreeRTOS porting
  {
    return RSI_ERROR_INVALID_PARAM;
  }
  if (xSemaphoreGive(*p_semaphore) == 0) {
    return RSI_ERROR_NONE;
  }
  return RSI_ERROR_IN_OS_OPERATION;
}

/*Note: This function is only used to dequeue the responses from Async response queues*/
sl_status_t sl_si91x_host_remove_from_queue(sl_si91x_queue_type_t queue, sl_wifi_buffer_t **buffer)
{
  sl_wifi_buffer_t *packet = NULL;
  osMutexAcquire(cmd_queues[queue].mutex, 0xFFFFFFFFUL);

  if (cmd_queues[queue].tail == NULL) {
    osMutexRelease(cmd_queues[queue].mutex);
    return SL_STATUS_EMPTY;
  }

  packet                 = cmd_queues[queue].head;
  cmd_queues[queue].head = (sl_wifi_buffer_t *)packet->node.node;
  packet->node.node      = NULL;

  if (NULL == cmd_queues[queue].head) {
    cmd_queues[queue].tail = NULL;
  }

  *buffer = (sl_wifi_buffer_t *)packet;
  osMutexRelease(cmd_queues[queue].mutex);

  return SL_STATUS_OK;
}

/*Note: This function is only used to dequeue the responses from Synchronous response queues*/
sl_status_t sl_si91x_host_remove_node_from_queue(sl_si91x_queue_type_t queue,
                                                 sl_wifi_buffer_t **buffer,
                                                 void *user_data,
                                                 sl_si91x_compare_function_t compare_function)
{
  sl_wifi_buffer_t *packet   = NULL;
  sl_wifi_buffer_t *data     = NULL;
  sl_wifi_buffer_t *previous = NULL;
  sl_status_t status         = SL_STATUS_NOT_FOUND;

  osMutexAcquire(cmd_queues[queue].mutex, 0xFFFFFFFFUL);

  if (cmd_queues[queue].tail == NULL) {
    osMutexRelease(cmd_queues[queue].mutex);
    return SL_STATUS_EMPTY;
  }

  packet   = cmd_queues[queue].head;
  previous = NULL;

  while (NULL != packet) {
    if (true == compare_function((sl_wifi_buffer_t *)packet, user_data)) {
      if (NULL == previous) {
        cmd_queues[queue].head = (sl_wifi_buffer_t *)packet->node.node;
      } else {
        previous->node.node = packet->node.node;
      }

      if (cmd_queues[queue].tail == packet) {
        cmd_queues[queue].tail = previous;
      }

      packet->node.node = NULL;
      data              = packet;
      status            = SL_STATUS_OK;
      break;
    }

    previous = packet;
    packet   = (sl_wifi_buffer_t *)packet->node.node;
  }

  if (NULL == cmd_queues[queue].head) {
    cmd_queues[queue].tail = NULL;
  }

  *buffer = (sl_wifi_buffer_t *)data;
  osMutexRelease(cmd_queues[queue].mutex);

  return status;
}

/* This function is used to flush the pending TX packets from the specified queue */
sl_status_t sl_si91x_host_flush_nodes_from_queue(sl_si91x_queue_type_t queue,
                                                 void *user_data,
                                                 sl_si91x_compare_function_t compare_function,
                                                 sl_si91x_node_free_function_t node_free_function)
{
  UNUSED_PARAMETER(queue);
  UNUSED_PARAMETER(user_data);
  UNUSED_PARAMETER(compare_function);
  UNUSED_PARAMETER(node_free_function);

  return SL_STATUS_OK;
}

uint32_t sl_si91x_host_queue_status(sl_si91x_queue_type_t queue)
{
  uint32_t status = 0;

  osMutexAcquire(cmd_queues[queue].mutex, 0xFFFFFFFFUL);
  status = ((NULL == cmd_queues[queue].tail) ? 0 : cmd_queues[queue].flag);
  osMutexRelease(cmd_queues[queue].mutex);

  return status;
}

uint32_t si91x_host_wait_for_event(uint32_t event_mask, uint32_t timeout)
{
  uint32_t result = osEventFlagsWait(si91x_events, event_mask, (osFlagsWaitAny | osFlagsNoClear), timeout);
  osEventFlagsClear(si91x_events, event_mask);
  if (result == (uint32_t)osErrorTimeout || result == (uint32_t)osErrorResource) {
    return 0;
  }
  return result;
}

uint32_t si91x_host_wait_for_bus_event(uint32_t event_mask, uint32_t timeout)
{
  uint32_t result = osEventFlagsWait(si91x_bus_events, event_mask, (osFlagsWaitAny | osFlagsNoClear), timeout);
  osEventFlagsClear(si91x_bus_events, event_mask);
  if (result == (uint32_t)osErrorTimeout || result == (uint32_t)osErrorResource) {
    return 0;
  }
  return result;
}

uint32_t si91x_host_wait_for_async_event(uint32_t event_mask, uint32_t timeout)
{
  uint32_t result = osEventFlagsWait(si91x_async_events, event_mask, (osFlagsWaitAny | osFlagsNoClear), timeout);
  osEventFlagsClear(si91x_async_events, event_mask);
  if (result == (uint32_t)osErrorTimeout || result == (uint32_t)osErrorResource) {
    return 0;
  }
  return result;
}

uint32_t si91x_host_clear_events(uint32_t event_mask)
{
  uint32_t result = osEventFlagsClear(si91x_events, event_mask);
  if (result == (uint32_t)osErrorResource) {
    return 0;
  }
  return result;
}

uint32_t si91x_host_clear_bus_events(uint32_t event_mask)
{
  uint32_t result = osEventFlagsClear(si91x_bus_events, event_mask);
  if (result == (uint32_t)osErrorResource) {
    return 0;
  }
  return result;
}

uint32_t si91x_host_clear_async_events(uint32_t event_mask)
{
  uint32_t result = osEventFlagsClear(si91x_async_events, event_mask);
  if (result == (uint32_t)osErrorResource) {
    return 0;
  }
  return result;
}

void sl_si91x_host_hold_in_reset(void)
{
  GPIO_PinModeSet(RESET_PIN.port, RESET_PIN.pin, gpioModePushPull, 1);
  GPIO_PinOutClear(RESET_PIN.port, RESET_PIN.pin);
}

void sl_si91x_host_release_from_reset(void)
{
  GPIO_PinModeSet(RESET_PIN.port, RESET_PIN.pin, gpioModeWiredOrPullDown, 1);
}

void sl_si91x_host_enable_bus_interrupt(void)
{
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
}

void sl_si91x_host_disable_bus_interrupt(void)
{
  NVIC_DisableIRQ(GPIO_ODD_IRQn);
}

static void gpio_interrupt(uint8_t interrupt_number)
{
  UNUSED_PARAMETER(interrupt_number);
  sl_si91x_host_set_bus_event(NCP_HOST_BUS_RX_EVENT);
  //  GPIO_IntClear(0xAAAA);
}

sl_status_t si91x_bootup_firmware(const uint8_t select_option)
{
  uint32_t rom_version;
  uint32_t timestamp;
  sl_status_t status = SL_STATUS_FAIL;
  //const uint8_t select_option = config->boot_option; //LOAD_NWP_FW;
  timestamp = sl_si91x_host_get_timestamp();
  do {
    if (sl_si91x_host_elapsed_time(timestamp) > SL_WIFI_BOARD_READY_WAIT_TIME) {
      return SL_STATUS_TIMEOUT;
    }
    status = sli_verify_device_boot(&rom_version);
  } while (status == SL_STATUS_WAITING_FOR_BOARD_READY || status == SL_STATUS_SPI_BUSY);
  VERIFY_STATUS_AND_RETURN(status);

  // selecting bootloader options
  if (select_option == LOAD_NWP_FW) {
    status = sl_si91x_bus_set_interrupt_mask(RSI_ACTIVE_HIGH_INTR);
  } else if (select_option == LOAD_DEFAULT_NWP_FW_ACTIVE_LOW) {
    status = sl_si91x_bus_set_interrupt_mask(RSI_ACTIVE_LOW_INTR);
  }
  VERIFY_STATUS_AND_RETURN(status);

#if RSI_FAST_FW_UP
  status = rsi_set_fast_fw_up();
  VERIFY_STATUS_AND_RETURN(status);
#endif

  // Select boot option
  status = sli_wifi_select_option(select_option);
  VERIFY_STATUS_AND_RETURN(status);
  return status;
}

sl_status_t sl_si91x_host_power_cycle(void)
{
  sl_si91x_host_hold_in_reset();
  sl_si91x_host_delay_ms(100);

  sl_si91x_host_release_from_reset();
  sl_si91x_host_delay_ms(100);

  return SL_STATUS_OK;
}

sl_status_t si91x_req_wakeup(void)
{
  // Wake device, if needed
  sl_si91x_host_set_sleep_indicator();
  uint32_t timestamp = sl_si91x_host_get_timestamp();
  do {
    if (sl_si91x_host_get_wake_indicator()) {
      sl_si91x_ulp_wakeup_init();
      break;
    }
    if (sl_si91x_host_elapsed_time(timestamp) > 1000) {
      return SL_STATUS_TIMEOUT;
    }
  } while (1);
  return SL_STATUS_OK;
}

void sli_submit_rx_buffer(void)
{
}
