/*******************************************************************************
* @file  startup_si91x.c
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

/**
 ******************************************************************************
 * @file      startup_si91x.c
 * @author    Coocox
 * @version   V1.0
 * @date      03/16/2013
 * @brief     Cortex M4 Devices Startup code.
 *            This module performs:
 *                - Set the initial SP
 *                - Set the vector table entries with the exceptions ISR address
 *                - Initialize data and bss
 *                - Call the application's entry point.
 *            After Reset the Cortex-M4 processor is in Thread mode,
 *            priority is Privileged, and the Stack is set to Main.
 *******************************************************************************
 */

#include "system_si91x.h"
#include "rsi_ps_ram_func.h"
#include "si91x_device.h"
#include "core_cm4.h"
/*----------Stack Symbols-----------------------------------------------*/
extern uint32_t __StackTop;
extern uint32_t __co_stackTop;

/*----------Macro definition--------------------------------------------------*/
#define WEAK __attribute__((weak))

/*-------To ignore -Wpedantic warnings--------*/
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

/*----------Symbols defined in linker script----------------------------------*/
extern unsigned long _sidata; /*!< Start address for the initialization 
                                      values of the .data section.            */
extern unsigned long _sdata;  /*!< Start address for the .data section     */
extern unsigned long _edata;  /*!< End address for the .data section       */

/*----------Function prototypes-----------------------------------------------*/
extern int main(void);             /*!< The entry point for the application  */
void Default_Reset_Handler(void);  /*!< Default reset handler                */
static void Default_Handler(void); /*!< Default exception handler            */
/**
 *@brief The minimal vector table for a Cortex M4.  Note that the proper constructs
 *       must be placed on this to ensure that it ends up at physical address
 *       0x00000000.
 */
__attribute__((used, section(".isr_vector"))) void (*const g_pfnVectors[])(void) = {
  /*----------------------------------Core Exceptions---------------------------------- */

  (void *)&__StackTop, /*!< The initial stack  pointer (0x00)            */
  (void *)0x300001,    /*!< Reset Handler                               */
  NMI_Handler,         /*!< NMI Handler                                 */
  HardFault_Handler,   /*!< Hard Fault Handler                          */
  MemManage_Handler,   /*!< MPU Fault Handler                           */
  BusFault_Handler,    /*!< Bus Fault Handler                           */
  UsageFault_Handler,  /*!< Usage Fault Handler                         */
  0,                   /*!< Reserved                                    */
  0,                   /*!< Reserved                                    */
  0,                   /*!< Reserved                                    */
  0,                   /*!< Reserved                                    */
  SVC_Handler,         /*!< SVCall Handler                              */
  DebugMon_Handler,    /*!< Debug Monitor Handler                       */
  0,                   /*!< Reserved                                    */
  PendSV_Handler,      /*!< PendSV Handler                              */
  SysTick_Handler,     /*!< SysTick Handler                             */

  IRQ000_Handler,            //   0: VAD interrupt
  IRQ001_Handler,            //   1: ULP Processor Interrupt1
  IRQ002_Handler,            //   2: ULP Processor Interrupt2
  IRQ003_Handler,            //   3: ULP Processor Interrupt3
  IRQ004_Handler,            //   4: ULP Processor Interrupt4
  IRQ005_Handler,            //   5: ULP Processor Interrupt5
  IRQ006_Handler,            //   6: ULP Processor Interrupt6
  IRQ007_Handler,            //   7: ULP Processor Interrupt7
  IRQ008_Handler,            //   8: ULP Processor Interrupt8
  IRQ009_Handler,            //   9: ULP Processor Interrupt8
  IRQ010_Handler,            //   10: ULP Processor Interrupt8
  IRQ011_Handler,            //   11: ULP Processor Interrupt8
  IRQ012_Handler,            //   12: ULP Processor Interrupt8
  IRQ013_Handler,            //   13: ULP Processor Interrupt8
  IRQ014_Handler,            //   14: ULP Processor Interrupt8
  IRQ015_Handler,            //   15: ULP Processor Interrupt8
  IRQ016_Handler,            //   16: ULP Processor Interrupt8
  IRQ017_Handler,            //   17: ULP Processor Interrupt8
  IRQ018_Handler,            //   18: ULP Processor Interrupt8
  IRQ019_Handler,            //   19: ULP Processor Interrupt8
  IRQ020_Handler,            //   20:  Sleep Sensor Interrupts 0
  IRQ021_Handler,            //   21:  Sleep Sensor Interrupts 1
  IRQ022_Handler,            //   22:  Sleep Sensor Interrupts 2
  IRQ023_Handler,            //   23:  Sleep Sensor Interrupts 3
  IRQ024_Handler,            //   24:  Sleep Sensor Interrupts 4
  IRQ025_Handler,            //   25:  Sleep Sensor Interrupts 5
  IRQ026_Handler,            //   26:  Sleep Sensor Interrupts 6
  IRQ027_Handler,            //   27:  Sleep Sensor Interrupts 7
  IRQ028_Handler,            //   28:  Sleep Sensor Interrupts 8
  IRQ029_Handler,            //   29:  Sleep Sensor Interrupts 9
  (void *)&__co_stackTop,    //   30: Reserved
  IRQ031_Handler,            //   31: RPDMA interrupt
  RSI_Default_Reset_Handler, //   32:   Reserved
  IRQ033_Handler,            //   33: UDMA interrupt
  IRQ034_Handler,            //   34: SCT interrupt
  HIF1_IRQHandler,           //   35:   HIF Interrupt1
  HIF2_IRQHandler,           //   36:  HIF Interrupt2
  IRQ037_Handler,            //   37:  SIO Interrupt
  IRQ038_Handler,            //   38:   USART 1 Interrupt
  IRQ039_Handler,            //   39:  USART 2 Interrupt
  RSI_PS_RestoreCpuContext,  //   40:  USART 3 Interrupt
  IRQ041_Handler,            //   41: GPIO WAKEUP INTERRUPT
  IRQ042_Handler,            //   42: I2C Interrupt
  (void *)0x10AD10AD,        //   43: Reserved
  IRQ044_Handler,            //   44: SSI Slave Interrupt
  0,                         //   45: Reserved
  IRQ046_Handler,            //   46: GSPI Master 1 Interrupt
  IRQ047_Handler,            //   47: Reserved
  IRQ048_Handler,            //   48: MCPWM Interrupt
  IRQ049_Handler,            //   49: QEI Interrupt
  IRQ050_Handler,            //   50: GPIO Group Interrupt 0
  IRQ051_Handler,            //   51: GPIO Group Interrupt 1
  IRQ052_Handler,            //   52: GPIO Pin Interrupt   0
  IRQ053_Handler,            //   53: GPIO Pin Interrupt   1
  IRQ054_Handler,            //   54: GPIO Pin Interrupt   2
  IRQ055_Handler,            //   55: GPIO Pin Interrupt   3
  IRQ056_Handler,            //   56: GPIO Pin Interrupt   4
  IRQ057_Handler,            //   57: GPIO Pin Interrupt   5
  IRQ058_Handler,            //   58: GPIO Pin Interrupt   6
  IRQ059_Handler,            //   59: GPIO Pin Interrupt   7
  IRQ060_Handler,            //   60: QSPI Interrupt
  IRQ061_Handler,            //   61: I2C 2 Interrupt
  IRQ062_Handler,            //   62: Ethernet Interrupt
  IRQ063_Handler,            //   63: Reserved
  IRQ064_Handler,            //   64: I2S master Interrupt
  0,                         //   65: Reserved
  IRQ066_Handler,            //   66: Can 1 Interrupt
  0,                         //   67: Reserved
  IRQ068_Handler,            //   68: SDMEM Interrupt
  IRQ069_Handler,            //   69: PLL clock ind Interrupt
  0,                         //   70: Reserved
  IRQ071_Handler,            //   71: CCI system Interrupt Out
  IRQ072_Handler,            //   72: FPU exception
  IRQ073_Handler,            //   73: USB INTR
  IRQ074_Handler,            //   74: TASS_P2P_INTR
  IRQ075_Handler,            //   75: WLAN Band1 intr0(TA)
  IRQ076_Handler,            //   76: WLAN Band1 intr1(TA)
  0,                         //   77: Reserved(TA)
  0,                         //   78: Reserved(TA)
  IRQ079_Handler,            //   79: BT intr(TA)
  IRQ080_Handler,            //   80: ZB intr(TA)
  0,                         //   81: Reserved(TA)
  IRQ082_Handler,            //   82: Modem disabled mode trigger intr(TA)
  IRQ083_Handler,            //   83: gpio intr(TA)
  IRQ084_Handler,            //   84: uart intr(TA)
  IRQ085_Handler,            //   85: watch dog level intr(TA)
  IRQ086_Handler,            //   86: ULP Sleep sensor interrupt(TA)
  IRQ087_Handler,            //   87: ECDH intr(TA)
  IRQ088_Handler,            //   88: DH intr(TA)
  IRQ089_Handler,            //   89: QSPI intr(TA)
  IRQ090_Handler,            //   90: ULP processor interrupt TASS(TA)
  IRQ091_Handler,            //   91: Sys Tick Timer(TA)
  IRQ092_Handler,            //   92: Real Timer interrupt(TA)
  IRQ093_Handler,            //   93: PLL lock interrupt(TA)
  0,                         //   94: Reserved(TA)
  IRQ095_Handler,            //   95: UART2 Interrupt(TA)
  0,                         //   96: Reserved(TA)
  IRQ097_Handler,            //   97: I2C Interrupt(TA)
};

/**
 * @brief  This is the code that gets never called, Dummy handler
 * @param  None
 * @retval None
 */
void Default_Reset_Handler(void)
{
  /*Generic Default reset handler for CM4 */
  while (1)
    ;
}

/**
 * @brief  This is the code that gets called when the processor first
 *         starts execution following a reset event. Only the absolutely
 *         necessary set is performed, after which the application
 *         supplied main() routine is called.
 */

#ifdef M4_PS2_STATE
__attribute__((section(".reset_handler")))
#endif
#ifdef EXECUTION_FROM_RAM
__attribute__((section(".ramVector"))) char RAM_VECTOR[sizeof(g_pfnVectors)];
__attribute__ ((section(".reset_handler")))
#endif

void RSI_Default_Reset_Handler(void)
{
  /* Initialize data and bss */
  volatile unsigned long *pulSrc, *pulDest;
  /* Copy the data segment initializers from flash to SRAM */
  pulSrc = &_sidata;

  for (pulDest = &_sdata; pulDest < &_edata;) {
    *(pulDest++) = *(pulSrc++);
  }

  /* Zero fill the bss segment.  This is done with inline assembly since this
     will clear the value of pulDest if it is not kept in a register. */
  __asm("  ldr     r0, =_sbss\n"
        "  ldr     r1, =_ebss\n"
        "  mov     r2, #0\n"
        "  .thumb_func\n"
        "zero_loop:\n"
        "    cmp     r0, r1\n"
        "    it      lt\n"
        "    strlt   r2, [r0], #4\n"
        "    blt     zero_loop");

  /*if C++ compilation required */
#ifdef SUPPORT_CPLUSPLUS
  extern void __libc_init_array(void);
  __libc_init_array();
#endif
#ifdef EXECUTION_FROM_RAM
  //copying the vector table from flash to ram
  memcpy(RAM_VECTOR, (uint32_t *)SCB->VTOR, sizeof(g_pfnVectors));
  //assing the ram vector address to VTOR register
  SCB->VTOR = (uint32_t)RAM_VECTOR;
#endif
  /*Init system level initializations */
  SystemInit();
  /* Call the application's entry point.*/
  main();
}

/**
 *@brief Provide weak aliases for each Exception handler to the Default_Handler.
 *       As they are weak aliases, any function with the same name will override
 *       this definition.
 */
#pragma weak Reset_Handler      = Default_Reset_Handler
#pragma weak NMI_Handler        = Default_Handler
#pragma weak HardFault_Handler  = Default_Handler
#pragma weak MemManage_Handler  = Default_Handler
#pragma weak BusFault_Handler   = Default_Handler
#pragma weak UsageFault_Handler = Default_Handler
#pragma weak SVC_Handler        = Default_Handler
#pragma weak DebugMon_Handler   = Default_Handler
#pragma weak PendSV_Handler     = Default_Handler
#pragma weak SysTick_Handler    = Default_Handler
/*----------------------------------external interrupts------------------------------ */
#pragma weak IRQ000_Handler  = Default_Handler
#pragma weak IRQ001_Handler  = Default_Handler
#pragma weak IRQ002_Handler  = Default_Handler
#pragma weak IRQ003_Handler  = Default_Handler
#pragma weak IRQ004_Handler  = Default_Handler
#pragma weak IRQ005_Handler  = Default_Handler
#pragma weak IRQ006_Handler  = Default_Handler
#pragma weak IRQ007_Handler  = Default_Handler
#pragma weak IRQ008_Handler  = Default_Handler
#pragma weak IRQ009_Handler  = Default_Handler
#pragma weak IRQ010_Handler  = Default_Handler
#pragma weak IRQ011_Handler  = Default_Handler
#pragma weak IRQ012_Handler  = Default_Handler
#pragma weak IRQ013_Handler  = Default_Handler
#pragma weak IRQ014_Handler  = Default_Handler
#pragma weak IRQ015_Handler  = Default_Handler
#pragma weak IRQ016_Handler  = Default_Handler
#pragma weak IRQ017_Handler  = Default_Handler
#pragma weak IRQ018_Handler  = Default_Handler
#pragma weak IRQ019_Handler  = Default_Handler
#pragma weak IRQ020_Handler  = Default_Handler
#pragma weak IRQ021_Handler  = Default_Handler
#pragma weak IRQ022_Handler  = Default_Handler
#pragma weak IRQ023_Handler  = Default_Handler
#pragma weak IRQ024_Handler  = Default_Handler
#pragma weak IRQ025_Handler  = Default_Handler
#pragma weak IRQ026_Handler  = Default_Handler
#pragma weak IRQ027_Handler  = Default_Handler
#pragma weak IRQ028_Handler  = Default_Handler
#pragma weak IRQ029_Handler  = Default_Handler
#pragma weak IRQ030_Handler  = Default_Handler
#pragma weak IRQ031_Handler  = Default_Handler
#pragma weak IRQ032_Handler  = Default_Handler
#pragma weak IRQ033_Handler  = Default_Handler
#pragma weak IRQ034_Handler  = Default_Handler
#pragma weak HIF1_IRQHandler = Default_Handler
#pragma weak HIF2_IRQHandler = Default_Handler
#pragma weak IRQ037_Handler  = Default_Handler
#pragma weak IRQ038_Handler  = Default_Handler
#pragma weak IRQ039_Handler  = Default_Handler
#pragma weak IRQ040_Handler  = Default_Handler
#pragma weak IRQ041_Handler  = Default_Handler
#pragma weak IRQ042_Handler  = Default_Handler
#pragma weak IRQ043_Handler  = Default_Handler
#pragma weak IRQ044_Handler  = Default_Handler
#pragma weak IRQ045_Handler  = Default_Handler
#pragma weak IRQ046_Handler  = Default_Handler
#pragma weak IRQ047_Handler  = Default_Handler
#pragma weak IRQ048_Handler  = Default_Handler
#pragma weak IRQ049_Handler  = Default_Handler
#pragma weak IRQ050_Handler  = Default_Handler
#pragma weak IRQ051_Handler  = Default_Handler
#pragma weak IRQ052_Handler  = Default_Handler
#pragma weak IRQ053_Handler  = Default_Handler
#pragma weak IRQ054_Handler  = Default_Handler
#pragma weak IRQ055_Handler  = Default_Handler
#pragma weak IRQ056_Handler  = Default_Handler
#pragma weak IRQ057_Handler  = Default_Handler
#pragma weak IRQ058_Handler  = Default_Handler
#pragma weak IRQ059_Handler  = Default_Handler
#pragma weak IRQ060_Handler  = Default_Handler
#pragma weak IRQ061_Handler  = Default_Handler
#pragma weak IRQ062_Handler  = Default_Handler
#pragma weak IRQ063_Handler  = Default_Handler
#pragma weak IRQ064_Handler  = Default_Handler
#pragma weak IRQ065_Handler  = Default_Handler
#pragma weak IRQ066_Handler  = Default_Handler
#pragma weak IRQ067_Handler  = Default_Handler
#pragma weak IRQ068_Handler  = Default_Handler
#pragma weak IRQ069_Handler  = Default_Handler
#pragma weak IRQ070_Handler  = Default_Handler
#pragma weak IRQ071_Handler  = Default_Handler
#pragma weak IRQ072_Handler  = Default_Handler
#pragma weak IRQ073_Handler  = Default_Handler
#pragma weak IRQ074_Handler  = Default_Handler
#pragma weak IRQ075_Handler  = Default_Handler
#pragma weak IRQ076_Handler  = Default_Handler
#pragma weak IRQ077_Handler  = Default_Handler
#pragma weak IRQ078_Handler  = Default_Handler
#pragma weak IRQ079_Handler  = Default_Handler
#pragma weak IRQ080_Handler  = Default_Handler
#pragma weak IRQ081_Handler  = Default_Handler
#pragma weak IRQ082_Handler  = Default_Handler
#pragma weak IRQ083_Handler  = Default_Handler
#pragma weak IRQ084_Handler  = Default_Handler
#pragma weak IRQ085_Handler  = Default_Handler
#pragma weak IRQ086_Handler  = Default_Handler
#pragma weak IRQ087_Handler  = Default_Handler
#pragma weak IRQ088_Handler  = Default_Handler
#pragma weak IRQ089_Handler  = Default_Handler
#pragma weak IRQ090_Handler  = Default_Handler
#pragma weak IRQ091_Handler  = Default_Handler
#pragma weak IRQ092_Handler  = Default_Handler
#pragma weak IRQ093_Handler  = Default_Handler
#pragma weak IRQ094_Handler  = Default_Handler
#pragma weak IRQ095_Handler  = Default_Handler
#pragma weak IRQ096_Handler  = Default_Handler
#pragma weak IRQ097_Handler  = Default_Handler
#pragma weak IRQ098_Handler  = Default_Handler

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.  This simply enters an infinite loop,
 *         preserving the system state for examination by a debugger.
 * @param  None
 * @retval None
 */
__attribute__((used)) void Default_Handler(void)
{
  /* Go into an infinite loop. */
  while (true) {
  }
}

/*********************** (C) COPYRIGHT 2009 Coocox ************END OF FILE*****/
