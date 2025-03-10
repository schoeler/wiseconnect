# Calibration

## 1. Purpose / Scope

This application demonstrates the procedure to calibrate the carrier frequency offset, Tx gain offset and few more parameters and update them to the Flash/Efuse. Customers need to calibrate the aforementioned parameters on their platforms after chip integration.

## 2. Prerequisites / Setup Requirements

### 2.1 Hardware Requirements

- A Windows PC.
- SiWx91x Wi-Fi Evaluation Kit. The SiWx91x supports multiple operating modes. See [Operating Modes]() for details.
- **SoC Mode**:
  - Silicon Labs [BRD4325A, BRD4325B, BRD4325C, BRD4325G, BRD4388A](https://www.silabs.com/)
- **NCP Mode**:
  - Silicon Labs [BRD4180B](https://www.silabs.com/)
  - Host MCU Eval Kit. This example has been tested with:
    - Silicon Labs [WSTK + EFR32MG21](https://www.silabs.com/development-tools/wireless/efr32xg21-bluetooth-starter-kit)
- Spectrum Analyzer
- RF Cable connects between EVK and Spectrum Analyzer.

### 2.2 Software Requirements

- Simplicity Studio IDE
  - Download the [Simplicity Studio IDE](https://www.silabs.com/developers/simplicity-studio).
  - Follow the [Simplicity Studio user guide](https://docs.silabs.com/simplicity-studio-5-users-guide/1.1.0/ss-5-users-guide-getting-started/install-ss-5-and-software#install-ssv5) to install Simplicity Studio IDE.
  
### 2.3 Set up Diagram

#### SoC Mode

![Figure: Setup Diagram SoC Mode for Calibration Example](resources/readme/calibrationsoc.png)

#### NCP Mode  

![Figure: Setup Diagram NCP Mode for Calibration Example](resources/readme/calibrationncp.png)

Follow the [Getting Started with Wiseconnect3 SDK](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/) guide to set up the hardware connections and Simplicity Studio IDE.

## 3. Project Environment

- Ensure the SiWx91x loaded with the latest firmware following the [Upgrade Si91x firmware](https://docs.silabs.com/wiseconnect/latest/wiseconnect-getting-started/getting-started-with-soc-mode#upgrade-si-wx91x-connectivity-firmware)

### 3.1 Creating the project

#### 3.1.1 SoC mode

- In the Simplicity Studio IDE, the SiWx91x SoC board will be detected under **Debug Adapters** pane as shown below.

   **![Soc Board detection](resources/readme/soc_board_detection.png)**

- Ensure the latest Gecko SDK along with the WiSeConnect3 extension is added to Simplicity Studio.

- Studio should detect your board. Your board will be shown here. Click on the board detected and go to **EXAMPLE PROJECTS & DEMOS** section.

- Filter for Wi-Fi examples from the Gecko SDK added. For this, check the *Wi-Fi* checkbox under **Wireless Technology**

  ![project_selection](resources/readme/projct_selection_soc.png)

- Click 'Create'. The "New Project Wizard" window appears. Click 'Finish'

  ![creation_final](resources/readme/creation_final_soc.png)

#### 3.1.2 NCP mode

- In the Simplicity Studio IDE, the EFR32 board will be detected under **Debug Adapters** pane as shown below.

   **![EFR32 Board detection](resources/readme/ncp_board_detection.png)**

- Ensure the latest Gecko SDK along with the WiSeConnect3 extension is added to Simplicity Studio.

- Go to the 'EXAMPLE PROJECT & DEMOS' tab and select Wi-Fi - Calibration App (NCP)

  ![project_selection](resources/readme/projct_selection_ncp.png)

- Click 'Create'. The "New Project Wizard" window appears. Click 'Finish'

  ![creation_final](resources/readme/creation_final_ncp.png)

### 3.3 Set up for application prints

You can use either of the below USB to UART converters for application prints for BRD4325A/BRD4325B boards.

1. Set up using USB to UART converter board.

   - Connect Tx (Pin-6) to P27 on WSTK
   - Connect GND (Pin 8 or 10) to GND on WSTK

      ![FTDI_prints](resources/readme/usb_to_uart_1.png)

2. Set up using USB to UART converter cable.

   - Connect RX (Pin 5) of TTL convertor to P27 on WSTK
   - Connect GND (Pin1) of TTL convertor to GND on WSTK

      ![FTDI_prints](resources/readme/usb_to_uart_2.png)

**Tera term set up - for NCP and SoC modes**

1. Open the Tera Term tool.
   - For SoC mode, choose the serial port to which USB to UART converter is connected and click on **OK**.

     **![](resources/readme/port_selection_soc.png)**

   - For NCP mode, choose the J-Link port and click on **OK**.

     **![](resources/readme/port_selection.png)**

2. Navigate to the Setup → Serial port and update the baud rate to **115200** and click on **OK**.

    **![](resources/readme/serial_port_setup.png)**

    **![](resources/readme/serial_port.png)**


For BRD4325C, BRD4325G, BRD4388A boards, the prints can be observed over the JLink port. The serial terminal should be connected to the JLink port with baud rate set to 115200.

1. Open the Tera Term tool.
choose the J-Link port and click on **OK**.

    **![](resources/readme/port_selection.png)**

2. Navigate to the Setup → Serial port and update the baud rate to **115200** and click on **OK**.

    **![](resources/readme/serial_port_setup.png)**

    **![](resources/readme/serial_port.png)**

## 4. Application Configuration Parameters

### 4.1 Configure the Application

The application can be configured to suit user requirements and development environment. Read through the following sections and make any changes needed.

Configure the following parameters in **app.c** to test throughput app as per requirements

To set TX power in dBm. The valid values are from (2 to 18)dbm and 127.

```c
#define TX_TEST_POWER             18
```
  
To set transmit data rate.

```c
#define TX_TEST_RATE              0
```

To configure length of the TX packet. Valid values are in the range of 24 to 1500 bytes in the burst mode and range of 24 to 260 bytes in the continuous mode.

```c
#define TX_TEST_LENGTH            30
```

To configure Burst mode or Continuous mode

```c
#define TX_TEST_MODE              BURST_MODE
```

To configure the channel number in 2.4 GHz or 5GHz. Here mention the channel number.

```c
#define TEST_CHANNEL              1
```

### 4.2 Build and run the application

- Right click on the project name and click on the Build Project option

  **![Build as](resources/readme/calibrationappbuild.png)**

- When the build process completes, the console output would be as follows:

  **![Build as](resources/readme/calibrationappbuildoutput.png)**

- Right click on the project name and click on the Run As > Silicon Labs ARM Program option
  **![Build as](resources/readme/calibrationapprun.png)**

### 4.3 Application execution flow

1. Make connections as per setup diagram (above) and set the appropriate settings on the Spectrum Analyzer by using the steps mentioned in below **Spectrum Analyzer Settings** section.

2. After the program gets executed, the SiWx91x device will start the transmit test with the given configuration. Application prints (over JLink port) would be as follows:

   ![Application prints](resources/readme/calibrationappprints.png)

3. The calibration commands (discussed in the sections [Frequency offset correction](#frequency-offset-correction), [Gain offset correction](#gain-offset-correction)) can be entered in the serial terminal as input.

   ![Calibration commands input](resources/readme/commandexamples.png)

4. Refer the below image which shows when SiWx91x device transmits packets in Burst mode with different Tx power and different transmission rates in channel 1 with length 1000 bytes.

   ```sh
   #define TX_TEST_POWER   18
   #define TX_TEST_RATE    0
   #define TX_TEST_LENGTH  30
   #define TX_TEST_MODE    BURST_MODE
   #define TEST_CHANNEL    1
   ```

5. Observe the Avg Freq Error (highlighted) on the screen and now try to adjust the frequency offset by using CLI commands with serial terminal (Docklight or Teraterm)

![Avg Freq Error (highlighted) on the screen](resources/readme/image_c7.png)

### Frequency Offset Correction

Frequency offset correction will be done by using the sl_freq_offset command. This command is used during the RF calibration process and requires PER mode transmissions to be initiated prior. This command sends freq_offset (deviation) as observed on the signal analyzer against the expected channel frequency.

Frequency offset correction command syntax :  
> `sl_freq_offset = freq_offset_in_khz <CR><LF>`  
>
> Here freq_offset_in_khz means Frequency deviation in KHz

Examples:

If an error of 10KHz is observed on the instrument the command should be used as follows:
> `sl_freq_offset=10<CR><LF>`

If an error of -10KHz is observed on the instrument the command should be used as follows:
> `sl_freq_offset=-10<CR><LF>`

> **Note:** User can use the above command for any number of times till it gets tuned to desired frequency offset.

User can provide input to correct frequency offset by sending the commands on console. This should lead towards a correction in the frequency offset as observed earlier and repeat till the error is within the tolerance limits (+/- 2 KHz tolerance limits).

See the below picture after frequency offset correction.

![After frequency offset correction](resources/readme/image_c9.png)

> **Note:** freq_offset_in_khz can be either +ve or -ve. When user enters the freq offset as observed on signal analyzer (+ve/-ve), a small freq offset correction is done. User needs to iterate this till the freq offset is within tolerance limits.

The corrected value of frequency offset is stored within a hardware register the sl_freq_offset command is given, for the correction to reflect upon reset, it is required that we copy the corrected value to flash. The value can be written to flash using the sl_calib_write command. The command should be used as follows: `sl_calib_write = 1,2,0,0,0`. Further details regarding the syntax of the sl_calib_write command are provided in the subsequent sections.

### Gain Offset Correction

#### Update XO Ctune and Gain Offset

Using sl_calib_write command the calibrated XO Ctune and calculated gain offset can be updated to target memory (Flash/Efuse).  Adjusting the gain offsets increases/reduces the average Tx power channel power in a particular channel.

   ```
   sl_calib_write command syntax :
   sl_calib_write=<target>,<flags>,<gain_offset_low>,<gain_offset_mid>,<gain_offset_high>
   OR
   sl_calib_write=<target>,<flags>,<gain_offset_low>,<gain_offset_mid>,<gain_offset_high>,<xo_ctune>
   ```

**Structure Members**

   | Parameter  | Description |                 |          |
   | :---       | :---        | :---            | :---     |
   | target     | Value       | Macro         | Description |
   |            |   0         | BURN_INTO_EFUSE | Burns calibration data to EFuse |
   |            |   1         | BURN_INTO_FLASH | Burns calibration data to Flash |
   | flags      |  BIT        | Macro                  | Description |
   |            |   0         |                       |  Reserved for future use
   |            |   1         | BURN_FREQ_OFFSET      | 1 - Update XO Ctune to calibration data
   |||| 0 - Skip XO Ctune update
   |            |   2         | SW_XO_CTUNE_VALID     | 1 - Use XO Ctune provided as argument to update calibration data  
   |||| 0 - Use XO Ctune value as read from hardware register
   |            |   3         | NA  |     NA
   |            |   4         |  BURN_GAIN_OFFSET_LOW  | 1 - Update gain offset for low sub-band (2 GHz)  
   |||| 0 - Skip low sub-band gain-offset update
   |            |   5         |  BURN_GAIN_OFFSET_MID  | 1 - Update gain offset for mid sub-band (2 GHz)  
   |||| 0 - Skip mid sub-band gain-offset update
   |            |   6         |  BURN_GAIN_OFFSET_HIGH | 1 - Update gain offset for high sub-band (2 GHz)  
   |            |   31-7      |                        | Reserved for future use
   |gain_offset_low | gain_offset as observed in dBm in channel-1 | |
   |gain_offset_mid | gain_offset as observed in dBm in channel-6 | |
   |gain_offset_high| gain_offset as observed in dBm in channel-11 | |
   | xo_ctune   | This field allows user to directly update xo_ctune value to calibration data bypassing the freq offset loop, valid only when BURN_FREQ_OFFSET & SW_XO_CTUNE_VALID of flags is set. |   | The range of xo_ctune is [0, 255], and the typical value is 80 |
   

> **Note:** For SiWx917, the user needs to calibrate gain-offset for low sub-band (channel-1), mid sub-band (channel-6), and high sub-band (channel-11) and input the three gain-offsets to this API and set the corresponding flags to validate it.

> **Precondition:**
sl_freq_offset command needs to be called before this command when xo ctune value from hardware register is to be used.

Gain offset can be calculated using the following equation :

   gain_offset = observed_power_level + cable_loss - required_power_level

Example :

   gain_offset = 14.3 + 1.7 (assuming) - 18 = -2 dBm

The corresponding value to be passed via the command would be -4 (2*calculated value).

Example usage:

`sl_calib_write=1,16,-4,0,0`
```
Target = 1 (data to be written to flash)
Flags  = BIT(4) => 2^4 = 16 (BURN_GAIN_OFFSET_LOW => Gain offset in channel 1 is to be updated in the flash)
gain_offset_low  = -4 (Updates the channel 1 gain offset such that the average tx power in channel 1 is incremented by 2dbm)
gain_offset_mid  = 0 (Not updating the gain offsets of channel 6, the corresponding bit is disabled in the flags)
gain_offset_high = 0 (Not updating the gain offsets of channel 11, the corresponding bit is disabled in the flags)
```
For 2 GHz band, the gain offset has to calibrated for three channels, viz. channel-1, channel-6, channel-11. After the three gain offsets are calculated these can be written to Flash using the `sl_calib_write` command.

> NOTE :
> The gain_offset can be negative but not a floating value.
> Once the frequency offset is corrected after multiple tries, sl_calib_write command has to be given once to write the values to flash.

### Spectrum Analyzer Settings

**Below are the necessary settings to see Polar Graph Spectrum Analyzer settings**

1. Frequency channel → center frequency→ 2412MHz for channel1.

2. SpanX scale → span→ 50MHz

3. Mode → WLAN → Mode setup → Radio Std → 802.11a/b/g → 802.11 b/g

4. Trigger → RF Burst

The frequency error section shows the error that needs to be adjusted. Using `sl_freq_offset` and `sl_calib_write` command user should be able to adjust the frequency error.

### Acronyms and Abbreviations

| Acronym   | Description |
| :----  | :----  |
| TX      | Transmit       |
|RX | Receive |
|RF | Radio Frequency |
|WLAN | Wireless Local Area Network |
|XO | Crystal Oscillator |
|Ctune| Captune |