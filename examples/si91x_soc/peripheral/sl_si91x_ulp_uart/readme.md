# ULP UART

## Introduction

- This application demonstrates how to configure ULP_UART In asyncronous mode, it will send and receive data in loopback mode

## Overview

- ULP_UART is used in communication through wired medium in Asynchronous fashion. It enables the device to
  communicate using serail protocols
- This application is configured with following configs
  - Tx and Rx enabled
  - Asynchronous mode
  - 8 Bit data transfer
  - Stop bits 1
  - No Parity
  - No Auto Flow control
  - Baud Rates - 115200

## About Example Code

- \ref ulp_uart_example.c this example code demonstates how to configure the uart to send and receive data in loopback mode
- In this example, first uart get initialized if it's not initialized already with clock and dma configurations if dma is
  enalbed using \ref sl_si91x_uart_init
- After uart initialization ,sets the uart power mode using \ref sl_si91x_uart_set_power_mode() and configures the UART
  with default configurations from UC and uart transmitt and receive lines using \ref sl_si91x_uart_set_configuration()
- Then register's user event callback for send ,recevie and transfer complete notification using
  \ref sl_si91x_uart_register_event_callback()
- After user event callback registered data send and receive can happen through \ref sl_si91x_uart_send_data() and
  \ref sl_si91x_uart_receive_data() respectively
- Once the receive data event triggered ,compares both transmitt and receive buffers to confirm the received data if data is
  same then it prints the Data comparison successful, Loop Back Test Passed on the uart console.

## Running Example code

- To use this application following Hardware, Software and the Project Setup is required.

### Hardware Setup

- Windows PC
- Silicon Labs [WSTK + BRD4338A]

![Figure: Introduction](resources/readme/image513a.png)

### Software Setup

- Si91x SDK
- Embedded Development Environment
  - For Silicon Labs Si91x, use the latest version of Simplicity Studio (refer **"Download and Install Simplicity Studio"** section in **getting-started-with-siwx917-soc** guide at **release_package/docs/index.html**)

## Project Setup

- **Silicon Labs Si91x** refer **"Download SDK"** section in **getting-started-with-siwx917-soc** guide at **release_package/docs/index.html** to work with Si91x and Simplicity Studio

## Loading Application on Simplicity Studio

1. With the product Si917 selected, navigate to the example projects by clicking on Example Projects & Demos
   in simplicity studio and click on to SI91x - SoC UART Example application as shown below.

![Figure: Selecting Example project](resources/readme/image513b.png)

## Build

1. Compile the application in Simplicity Studio using build icon
   ![Figure: Build run and Debug](resources/readme/image513c.png)


## Device Programming

- To program the device ,refer **"Burn M4 Binary"** section in **getting-started-with-siwx917-soc** guide at **release_package/docs/index.html** to work with Si91x and Simplicity Studio

## Pin Configuration 

|         GPIO pin    |Description|
|   --------------    | --------- |
|ULP_GPIO_11  [F6]    |    TX     |
|ULP_GPIO_9   [F7]    |    RX     |
|ULP_GPIO_10  [P17]   |GPIO_Toggle|

## Note
 - Enable the ULP_UART mode in UC before running/flashing the code.
  ![Figure: peripheral configuration](resources/uc_screen/image513d.png) 
 
## Executing the Application
1. When the application runs,ULP_UART sends and receives data in full duplex mode

## Expected Results 
 - When tx and rx data both are matching ULP_GPIO_10 should be toggled ,connect logic analyser to observe the toggle state. 
 - Here same PINS which are used to send and recive the data where used for data transfer so we cannot able to see prins 
 we can use GPIO toggling instead.
## Note
Note
- This applicatin is executed from RAM.
- In this application while changing the MCU mode from PS4 to PS2, M4 flash will be turned off.
- The debug feature of Simplicity Studio will not work after M4 flash is turned off.
- After Flashing ULP examples as M4 flash will be turned off,flash erase does not work.
- To Erase the chip follow the below procedure
- Turn ON ISP switch and press the reset button → Turn OFF ISP Switch → Now perform Chip erase
through commander.
