/*******************************************************************************
* @file  rsi_hal_mcu_m4.c
* @brief 
*******************************************************************************
* # License
* <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#include "rsi_driver.h"
#include "rsi_m4.h"
#include "rsi_board.h"
#ifdef COMMON_FLASH_EN
#include "rsi_power_save.h"
#endif

rsi_m4ta_desc_t tx_desc[2];
rsi_m4ta_desc_t rx_desc[2];

/** @addtogroup SOC4
* @{
*/
uint32_t NVIC_GetIRQEnable(IRQn_Type IRQn)
{
  return ((NVIC->ICER[((uint32_t)(IRQn) >> 5)] & (1 << ((uint32_t)(IRQn)&0x1F))) ? 1 : 0);
}

void rsi_assertion(uint16_t assertion_val, const char *string)
{
  uint16_t i;
  if (assertion_val == 0) {
    for (i = 0; i < strlen(string); i++) {
#ifdef DEBUG_UART
#ifdef DEBUG_ASSERTION
      Board_UARTPutChar(string[i]);
#endif
#endif
    }

    return;
  } else {
    for (i = 0; i < strlen(string); i++) {
#ifdef DEBUG_UART
#ifdef DEBUG_ASSERTION
      Board_UARTPutChar(string[i]);
#endif
#endif
    }

    while (1)
      ;
  }
}

void IRQ074_Handler(void)
{
  ROM_WL_rsi_m4_interrupt_isr(global_cb_p);
}

/*==============================================*/
/**
 * @fn           void rsi_raise_pkt_pending_interrupt_to_ta()
 * @brief        This function raises the packet pending interrupt to TA
 * @param[in]    none 
 * @param[out]   none
 * @return       none
 * @section description
 * This function raises the packet pending interrupt to TA
 *
 *
 */
void rsi_m4_ta_interrupt_init(void)
{
#ifdef CHIP_917
  //! Unmask the interrupt
  unmask_ta_interrupt(TX_PKT_TRANSFER_DONE_INTERRUPT | RX_PKT_TRANSFER_DONE_INTERRUPT | TA_WRITING_ON_COMM_FLASH
#ifdef M4_OTAF_DF
                      | M4_IMAGE_UPGRADATION_PENDING_INTERRUPT
#endif
  );
#else
  //! Unmask the interrupt
  unmask_ta_interrupt(TX_PKT_TRANSFER_DONE_INTERRUPT | RX_PKT_TRANSFER_DONE_INTERRUPT);
#endif
  P2P_STATUS_REG |= M4_is_active;

  *(volatile uint32_t *)0xE000E108 = 0x00000400;

#ifdef RSI_WITH_OS
  //! Set P2P Intr priority
  NVIC_SetPriority(TASS_P2P_IRQn, TASS_P2P_INTR_PRI);
#endif

  return;
}

void mask_ta_interrupt(uint32_t interrupt_no)
{
#ifdef ROM_WIRELESS
  ROMAPI_WL->mask_ta_interrupt(interrupt_no);
#else
  api_wl->mask_ta_interrupt(interrupt_no);
#endif
}

void unmask_ta_interrupt(uint32_t interrupt_no)
{
#ifdef ROM_WIRELESS
  ROMAPI_WL->unmask_ta_interrupt(interrupt_no);
#else
  api_wl->unmask_ta_interrupt(interrupt_no);
#endif
}

int rsi_submit_rx_pkt(void)
{
#ifdef ROM_WIRELESS
  return ROMAPI_WL->rsi_submit_rx_pkt(global_cb_p);
#else
  return api_wl->rsi_submit_rx_pkt(global_cb_p);
#endif
}

/*====================================================*/
/**
 * @fn          int16_t rsi_frame_read(uint8_t *pkt_buffer)
 * @brief This  function is used to read the response from module.
 * @param[in]   uint8_t *pkt_buffer, pointer to the buffer to which packet has to read
 *              which is used to store the response from the module
 * @param[out]  none
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description 
 * This is a common function to read response for all the command and data from Wi-Fi module.
 */

rsi_pkt_t *rsi_frame_read(void)
{
#ifdef ROM_WIRELESS
  return ROMAPI_WL->rsi_frame_read(global_cb_p);
#else
  return api_wl->rsi_frame_read(global_cb_p);
#endif
}

/*====================================================*/
/**
 * @fn          int16_t rsi_frame_write(rsi_frame_desc_t *uFrameDscFrame,uint8_t *payloadparam,uint16_t size_param)
 * @brief       Common function for all the commands.
 * @param[in]   uFrameDsc uFrameDscFrame, frame descriptor
 * @param[in]   uint8_t *payloadparam, pointer to the command payload parameter structure
 * @param[in]   uint16_t size_param, size of the payload for the command
 * @return      errCode
 *              -1 = SPI busy / Timeout
 *              -2 = SPI Failure
 *              0  = SUCCESS
 * @section description 
 * This is a common function used to process a command to the Wi-Fi module.
 */

int16_t rsi_frame_write(rsi_frame_desc_t *uFrameDscFrame, uint8_t *payloadparam, uint16_t size_param)
{
#ifdef ROM_WIRELESS
  return ROMAPI_WL->rsi_frame_write(global_cb_p, uFrameDscFrame, payloadparam, size_param);
#else
  return api_wl->rsi_frame_write(global_cb_p, uFrameDscFrame, payloadparam, size_param);
#endif
}
/*==============================================*/
/**
 * @fn           void rsi_update_tx_dma_desc(uint8 skip_dma_valid)
 * @brief        This function updates the TX DMA descriptor address
 * @param[in]    skip_dma_valid 
 * @param[out]   none
 * @return       none
 * @section description
 * This function updates the TX DMA descriptor address
 *
 *
 */

void rsi_update_tx_dma_desc(uint8_t skip_dma_valid)
{
  if (!skip_dma_valid) {
#ifdef COMMON_FLASH_EN
    if (!(M4_ULP_SLP_STATUS_REG & MCU_ULP_WAKEUP))
#endif
    {
      while (M4_TX_DMA_DESC_REG & DMA_DESC_REG_VALID)
        ;
    }
  }
  M4_TX_DMA_DESC_REG = (uint32_t)&tx_desc;
}

/*==============================================*/
/**
 * @fn           void rsi_update_rx_dma_desc()
 * @brief        This function updates the RX DMA descriptor address
 * @param[in]    none 
 * @param[out]   none
 * @return       none
 * @section description
 * This function updates the RX DMA descriptor address
 *
 *
 */
void rsi_update_rx_dma_desc(void)
{
  M4_RX_DMA_DESC_REG = (uint32_t)&rx_desc;
}

/*==============================================*/
/**
 * @fn           void rsi_config_m4_dma_desc_on_reset()
 * @brief        This function updates the RX DMA and TX DMA descriptor address after reset
 * @param[in]    none 
 * @param[out]   none
 * @return       none
 * @section description
 * This function updates the RX DMA  and TX DMA descriptor address
 *
 *
 */
void rsi_config_m4_dma_desc_on_reset(void)
{
  //! Wait for TA to go to sleep
  while (P2P_STATUS_REG & TA_is_active)
    ;
  //! Wait for TA to wakeup and should be in bootloader
  while (!(P2P_STATUS_REG & TA_is_active))
    ;
  //! UPdate M4 TX and RX dma descriptors
  M4_TX_DMA_DESC_REG = (uint32_t)&tx_desc;
  M4_RX_DMA_DESC_REG = (uint32_t)&rx_desc;
}

/*==================================================*/
/**
 * @fn          int16_t rsi_device_interrupt_status(uint8_t *int_status)
 * @brief       Returns the value of the Interrupt register
 * @param[in]   status
 * @param[out]  buffer full status reg value 
 * @return      errorcode
 *               0 = Success
 *              -2 = Reg read failure  
 */
int16_t rsi_device_interrupt_status(uint8_t *int_status)
{

  //! Check for TA active .If it is not active Buffer full status is not valid,
  //! SO return fail from here
  if (!(P2P_STATUS_REG & TA_is_active)) {
    return RSI_FAIL;
  }
  //! copy buffer full status reg value
  *int_status = (uint8_t)HOST_INTR_STATUS_REG;

  return RSI_SUCCESS;
}
/** @} */
