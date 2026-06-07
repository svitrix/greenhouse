# ESP32-C6 — esp-dev-kits Documentation

> **Source:** `esp-dev-kits-en-master-esp32c6-2.pdf` (Espressif Systems, Release `master`, May 27, 2026) — verbatim transcription of the official user guide for ESP32-C6 development boards.
>
> This document covers two boards:
> - **ESP32-C6-DevKitC-1** (Chapter 1) — based on ESP32-C6-WROOM-1(U) module, 8 MB SPI flash.
> - **ESP32-C6-DevKitM-1** (Chapter 2) — based on ESP32-C6-MINI-1(U) module, 4 MB SPI flash. **This is the board used in `firmware/coordinator`.**
>
> Note: For the full list of Espressif development boards, please go to [ESP DevKits](https://www.espressif.com/en/products/devkits).

---

## Table of Contents

- [Chapter 1 — ESP32-C6-DevKitC-1](#chapter-1--esp32-c6-devkitc-1)
  - [1.1 ESP32-C6-DevKitC-1 v1.2](#11-esp32-c6-devkitc-1-v12)
    - [1.1.1 Getting Started](#111-getting-started)
    - [1.1.2 Hardware Reference](#112-hardware-reference)
    - [1.1.3 Hardware Revision Details](#113-hardware-revision-details)
    - [1.1.4 Related Documents](#114-related-documents)
- [Chapter 2 — ESP32-C6-DevKitM-1](#chapter-2--esp32-c6-devkitm-1)
  - [2.1 ESP32-C6-DevKitM-1](#21-esp32-c6-devkitm-1)
    - [2.1.1 Getting Started](#211-getting-started)
    - [2.1.2 Hardware Reference](#212-hardware-reference)
    - [2.1.3 Hardware Revision Details](#213-hardware-revision-details)
    - [2.1.4 Related Documents](#214-related-documents)
- [Chapter 3 — Related Documentation and Resources](#chapter-3--related-documentation-and-resources)
  - [3.1 Related Documentation](#31-related-documentation)
  - [3.2 Developer Zone](#32-developer-zone)
  - [3.3 Products](#33-products)
  - [3.4 Contact Us](#34-contact-us)
- [Chapter 4 — Disclaimer and Copyright Notice](#chapter-4--disclaimer-and-copyright-notice)

---

# Chapter 1 — ESP32-C6-DevKitC-1

ESP32-C6-DevKitC-1 is an entry-level development board based on ESP32-C6-WROOM-1(U), a general-purpose module with an 8 MB SPI flash. This board integrates complete Wi-Fi, Bluetooth LE, Zigbee, and Thread functions.

## 1.1 ESP32-C6-DevKitC-1 v1.2

The older version: *ESP32-C6-DevKitC-1 v1.1*

This user guide will help you get started with ESP32-C6-DevKitC-1 and will also provide more in-depth information.

ESP32-C6-DevKitC-1 is an entry-level development board based on [ESP32-C6-WROOM-1(U)](https://www.espressif.com/en/products/modules), a general-purpose module with a 8 MB SPI flash. This board integrates complete Wi-Fi, Bluetooth LE, Zigbee, and Thread functions.

Most of the I/O pins are broken out to the pin headers on both sides for easy interfacing. Developers can either connect peripherals with jumper wires or mount ESP32-C6-DevKitC-1 on a breadboard.

The document consists of the following major sections:

- *Getting Started*: Overview of ESP32-C6-DevKitC-1 and hardware/software setup instructions to get started.
- *Hardware Reference*: More detailed information about the ESP32-C6-DevKitC-1's hardware.
- *Hardware Revision Details*: Revision history, known issues, and links to user guides for previous versions (if any) of ESP32-C6-DevKitC-1.
- *Related Documents*: Links to related documentation.

### 1.1.1 Getting Started

This section provides a brief introduction of ESP32-C6-DevKitC-1, instructions on how to do the initial hardware setup and how to flash firmware onto it.

#### Description of Components

The key components of the board are described in a clockwise direction.

**Fig. 2: ESP32-C6-DevKitC-1 — front (labelled components on the front side of the board)**

- ESP32-C6-WROOM-1
- Pin Header (left)
- 5 V to 3.3 V LDO
- 3.3 V Power On LED
- USB-to-UART Bridge
- ESP32-C6 USB Type-C Port
- Boot Button
- Reset Button
- USB Type-C to UART Port
- RGB LED
- J5
- Pin Header (right)

| Key Component | Description |
|---|---|
| **ESP32-C6-WROOM-1 or ESP32-C6-WROOM-1U** | ESP32-C6-WROOM-1 and ESP32-C6-WROOM-1U are general-purpose modules supporting Wi-Fi 6 in 2.4 GHz band, Bluetooth 5, and IEEE 802.15.4 (Zigbee 3.0 and Thread 1.3). They are built around the ESP32-C6 chip, and come with a 8 MB SPI flash. ESP32-C6-WROOM-1 uses on-board PCB antenna, whereas ESP32-C6-WROOM-1U uses external antenna connector. For more information, see *ESP32-C6-WROOM-1 Datasheet*. |
| **Pin Header** | All available GPIO pins (except for the SPI bus for flash) are broken out to the pin headers on the board. |
| **5 V to 3.3 V LDO** | Power regulator that converts a 5 V supply into a 3.3 V output. |
| **3.3 V Power On LED** | Turns on when the USB power is connected to the board. |
| **USB-to-UART Bridge** | Single USB-to-UART bridge chip provides transfer rates up to 3 Mbps. |
| **ESP32-C6 USB Type-C Port** | The USB Type-C port on the ESP32-C6 chip compliant with USB 2.0 full speed. It is capable of up to 12 Mbps transfer speed (Note that this port does not support the faster 480 Mbps high-speed transfer mode). This port is used for power supply to the board, for flashing applications to the chip, for communication with the chip using USB protocols, as well as for JTAG debugging. |
| **Boot Button** | Download button. Holding down **Boot** and then pressing **Reset** initiates Firmware Download mode for downloading firmware through the serial port. |
| **Reset Button** | Press this button to restart the system. |
| **USB Type-C to UART Port** | Used for power supply to the board, for flashing applications to the chip, as well as the communication with the ESP32-C6 chip via the on-board USB-to-UART bridge. |
| **RGB LED** | Addressable RGB LED, driven by GPIO8. |
| **J5** | Used for current measurement. See details in Section *Current Measurement*. |

#### Start Application Development

Before powering up your ESP32-C6-DevKitC-1, please make sure that it is in good condition with no obvious signs of damage.

**Required Hardware**

- ESP32-C6-DevKitC-1
- USB-A to USB-C cable
- Computer running Windows, Linux, or macOS

> **Note:** Be sure to use a good quality USB cable. Some cables are for charging only and do not provide the needed data lines nor work for programming the boards.

**Software Setup** — Please proceed to [ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/get-started/index.html), which will quickly help you set up the development environment then flash an application example onto your board.

#### Contents and Packaging

**Retail orders** — If you order a few samples, each ESP32-C6-DevKitC-1 comes in an individual package in either antistatic bag or any packaging depending on your retailer.

For retail orders, please go to <https://www.espressif.com/en/company/contact/buy-a-sample>.

**Wholesale Orders** — If you order in bulk, the boards come in large cardboard boxes.

For wholesale orders, please check [Espressif Product Ordering Information](https://www.espressif.com/sites/default/files/documentation/espressif_products_ordering_information_en.pdf) (PDF).

### 1.1.2 Hardware Reference

#### Block Diagram

The block diagram below shows the components of ESP32-C6-DevKitC-1 and their interconnections.

**Fig. 3: ESP32-C6-DevKitC-1 (block diagram)**

```
                Power Supply/
                 Programming
                       │
 USB Type-C        D+/D-           TX/RX                ┌──────────┐
 to UART Port ───────────► USB-UART ─────────►          │ RGB LED  │
                            Bridge                      └────▲─────┘
                              │                              │ GPIO8
                              ▼                              │
                            ┌─────┐  3.3V  ┌──────────────────────┐
                            │ LDO │ ─────► │ ESP32-C6-WROOM-1     │
                            └─────┘        │   or                 │
                              ▲            │ ESP32-C6-WROOM-1U    │ ───► Header Block ×2
                              │            └──────────────────────┘
 ESP32-C6                     │                  ▲       ▲
 USB Type-C ──────────────────┘                  │       │
   Port                                        [Boot]  [RST]
```

#### Power Supply Options

There are three mutually exclusive ways to provide power to the board:

- USB Type-C to UART Port and ESP32-C6 USB Type-C Port (either one or both), default power supply (recommended)
- 5V and GND pin headers
- 3V3 and GND pin headers

#### Current Measurement

The J5 headers on ESP32-C6-DevKitC-1 (see J5 in Figure *ESP32-C6-DevKitC-1 — front*) can be used for measuring the current drawn by the ESP32-C6-WROOM-1(U) module:

- **Remove the jumper:** Power supply between the module and peripherals on the board is cut off. To measure the module's current, connect the board with an ammeter via J5 headers.
- **Apply the jumper (factory default):** Restore the board's normal functionality.

> **Note:** When using 3V3 and GND pin headers to power the board, please remove the J5 jumper, and connect an ammeter in series to the external circuit to measure the module's current.

#### Header Block

The two tables below provide the **Name** and **Function** of the pin headers on both sides of the board (J1 and J3). The pin header names are shown in Figure *ESP32-C6-DevKitC-1 — front*. The numbering is the same as in the ESP32-C6-DevKitC-1 Schematic v1.2 (PDF).

**J1**

| No. | Name | Type¹ | Function |
|---:|---|---|---|
| 1  | 3V3 | P     | 3.3 V power supply |
| 2  | RST | I     | High: enables the chip; Low: disables the chip. |
| 3  | 4   | I/O/T | MTMS³, GPIO4, LP_GPIO4, LP_UART_RXD, ADC1_CH4, FSPIHD |
| 4  | 5   | I/O/T | MTDI³, GPIO5, LP_GPIO5, LP_UART_TXD, ADC1_CH5, FSPIWP |
| 5  | 6   | I/O/T | MTCK, GPIO6, LP_GPIO6, LP_I2C_SDA, ADC1_CH6, FSPICLK |
| 6  | 7   | I/O/T | MTDO, GPIO7, LP_GPIO7, LP_I2C_SCL, FSPID |
| 7  | 0   | I/O/T | GPIO0, XTAL_32K_P, LP_GPIO0, LP_UART_DTRN, ADC1_CH0 |
| 8  | 1   | I/O/T | GPIO1, XTAL_32K_N, LP_GPIO1, LP_UART_DSRN, ADC1_CH1 |
| 9  | 8   | I/O/T | GPIO8²,³ |
| 10 | 10  | I/O/T | GPIO10 |
| 11 | 11  | I/O/T | GPIO11 |
| 12 | 2   | I/O/T | GPIO2, LP_GPIO2, LP_UART_RTSN, ADC1_CH2, FSPIQ |
| 13 | 3   | I/O/T | GPIO3, LP_GPIO3, LP_UART_CTSN, ADC1_CH3 |
| 14 | 5V  | P     | 5 V power supply |
| 15 | G   | G     | Ground |
| 16 | NC  | –     | No connection |

**J3**

| No. | Name | Type | Function |
|---:|---|---|---|
| 1  | G   | G     | Ground |
| 2  | TX  | I/O/T | U0TXD, GPIO16, FSPICS0 |
| 3  | RX  | I/O/T | U0RXD, GPIO17, FSPICS1 |
| 4  | 15  | I/O/T | GPIO15³ |
| 5  | 23  | I/O/T | GPIO23, SDIO_DATA3 |
| 6  | 22  | I/O/T | GPIO22, SDIO_DATA2 |
| 7  | 21  | I/O/T | GPIO21, SDIO_DATA1, FSPICS5 |
| 8  | 20  | I/O/T | GPIO20, SDIO_DATA0, FSPICS4 |
| 9  | 19  | I/O/T | GPIO19, SDIO_CLK, FSPICS3 |
| 10 | 18  | I/O/T | GPIO18, SDIO_CMD, FSPICS2 |
| 11 | 9   | I/O/T | GPIO9³ |
| 12 | G   | G     | Ground |
| 13 | 13  | I/O/T | GPIO13, USB_D+ |
| 14 | 12  | I/O/T | GPIO12, USB_D− |
| 15 | G   | G     | Ground |
| 16 | NC  | –     | No connection |

**Notes:**
- ¹ P: Power supply; I: Input; O: Output; T: High impedance.
- ² Used to drive the RGB LED.
- ³ MTMS, MTDI, GPIO8, GPIO9, and GPIO15 are strapping pins of the ESP32-C6 chip. These pins are used to control several chip functions depending on binary voltage values applied to the pins during chip power-up or system reset. For description and application of the strapping pins, please refer to *ESP32-C6 Datasheet > Section Strapping Pins*.

#### Pin Layout

**Fig. 4: ESP32-C6-DevKitC-1 Pin Layout**

```
ESP32-C6 Specs (from the pin-layout diagram)
- 32-bit RISC-V single-core @160 MHz
- Wi-Fi IEEE 802.11 ax 2.4 GHz + Bluetooth LE 5 + IEEE 802.15.4 (Zigbee and Thread)
- 512 KB SRAM (21 KB for cache)
- 320 KB ROM
- 30 or 22 GPIOs, 3× SPI, 2× UART, 1× I2C, RMT
- LED PWM 6ch, 1× 12-bit ADC with 7ch, TWAI®
- USB Serial/JTAG, ETM, MCPWM, SDIO Slave
```

Pin-layout legend (color-coded zones on the diagram):
- PWM Capable Pin
- Fast SPI Functions
- General Purpose Input / Output
- Low-Power UART Functions
- Low-Power I2C Functions
- Other Related Functions
- Strapping Pin Functions
- JTAG for Debugging and/or USB
- Analog-to-Digital Converter
- Serial for Debug/Programming
- Ground Plane
- Power Rails (3V3 and 5V)
- Low-Power GPIO Functions

### 1.1.3 Hardware Revision Details

**ESP32-C6-DevKitC-1 v1.2**

- For boards with the PW number of and after **PW-2023-02-0139** (on and after February 2023), J5 is changed from straight headers to curved headers.
- For boards with the PW number of and after **PW-2023-07-XXXX** (on and after July 2023), multi-point calibration is performed on ADC instead of two-point calibration, and the measurement range and accuracy are illustrated in *ESP32-C6 Datasheet > Section ADC Characteristics*. For boards with earlier PW number, please ask our sales team to provide the actual range and accuracy according to batch.
- For boards with the PW number of and after **PW-2023-07-0440** (on and after July 2023), to optimize the WS2812 driving circuit, the resistance of **R29 is updated from 4.7 kΩ to 10 kΩ**, and the resistance of **R6 is updated from 10 kΩ to 3.3 kΩ**. For details, see *ESP32-C6-DevKitC-1 Schematic v1.3*.
- For boards with the PW number of and after **PW-2024-03-0595** and **PW-2024-03-0921** (on and after March 2024), to optimize the circuit, the resistance of **R7 on UART_RXD is updated from 0 Ω to 470 Ω**. For details, see *ESP32-C6-DevKitC-1 Schematic v1.4*.

> **Note:** The PW number can be found in the product label on the large cardboard boxes for wholesale orders.

**ESP32-C6-DevKitC-1 v1.1** — *Initial release*

### 1.1.4 Related Documents

Please download the following documents from the HTML version of esp-dev-kits Documentation:

- ESP32-C6 Datasheet (PDF)
- ESP32-C6-WROOM-1 Datasheet (PDF)
- ESP32-C6-DevKitC-1 Schematic v1.4 (PDF) — Applies to boards of and after PW-2024-03-0595 and PW-2024-03-0921
- ESP32-C6-DevKitC-1 Schematic v1.3 (PDF) — Applies to boards of and after PW-2023-07-0440
- ESP32-C6-DevKitC-1 Schematic v1.2 (PDF) — Applies to boards before PW-2023-07-0440
- ESP32-C6-DevKitC-1 PCB Layout (PDF)
- ESP32-C6-DevKitC-1 Dimensions (PDF)
- ESP32-C6-DevKitC-1 Dimensions source file (DXF) — You can view it with Autodesk Viewer online

For further design documentation for the board, please contact us at <sales@espressif.com>.

#### ESP32-C6-DevKitC-1 v1.1 (older version)

> New version available: *ESP32-C6-DevKitC-1 v1.2*

This user guide will help you get started with ESP32-C6-DevKitC-1 and will also provide more in-depth information.

ESP32-C6-DevKitC-1 is an entry-level development board based on [ESP32-C6-WROOM-1](https://www.espressif.com/en/products/modules), a general-purpose module with a 8 MB SPI flash. This board integrates complete Wi-Fi, Bluetooth LE, Zigbee, and Thread functions.

Most of the I/O pins are broken out to the pin headers on both sides for easy interfacing. Developers can either connect peripherals with jumper wires or mount ESP32-C6-DevKitC-1 on a breadboard.

The document consists of the following major sections:

- *Getting Started*: Overview of ESP32-C6-DevKitC-1 and hardware/software setup instructions to get started.
- *Hardware Reference*: More detailed information about the ESP32-C6-DevKitC-1's hardware.
- *Hardware Revision Details*: Revision history, known issues, and links to user guides for previous versions (if any) of ESP32-C6-DevKitC-1.
- *Related Documents*: Links to related documentation.

**Getting Started (v1.1)** — same content as v1.2 above. Components shown on front-view photo: ESP32-C6-WROOM-1, Pin Header, 5 V to 3.3 V LDO, 3.3 V Power On LED, USB-to-UART Bridge, ESP32-C6 USB Type-C Port, Boot Button, Reset Button, USB Type-C to UART Port, RGB LED, J5.

The Key Component table, *Start Application Development*, *Required Hardware*, *Software Setup*, and *Contents and Packaging* sections for v1.1 are identical to those of v1.2 above.

**Hardware Reference (v1.1)** — Block diagram identical to v1.2 above. *Power Supply Options*, *Current Measurement*, and *Header Block* sections are identical to v1.2 (same J1 / J3 pin assignments, same notes).

**Pin Layout (v1.1)** — Identical specs to v1.2 Fig. 4.

**Hardware Revision Details (v1.1)** — No previous versions available.

**Related Documents (v1.1)**

- ESP32-C6 Datasheet (PDF)
- ESP32-C6-WROOM-1 Datasheet (PDF)
- ESP32-C6-DevKitC-1 Schematic (PDF)
- ESP32-C6-DevKitC-1 PCB Layout (PDF)
- ESP32-C6-DevKitC-1 Dimensions (PDF)
- ESP32-C6-DevKitC-1 Dimensions source file (DXF)

For further design documentation for the board, please contact us at <sales@espressif.com>.

---

# Chapter 2 — ESP32-C6-DevKitM-1

> **★ This is the board used in `firmware/coordinator`.**

ESP32-C6-DevKitM-1 is an entry-level development board based on **ESP32-C6-MINI-1(U)**, a general-purpose module with a **4 MB SPI flash**. This board integrates complete Wi-Fi, Bluetooth LE, Zigbee, and Thread functions.

## 2.1 ESP32-C6-DevKitM-1

This user guide will help you get started with ESP32-C6-DevKitM-1 and will also provide more in-depth information.

ESP32-C6-DevKitM-1 is an entry-level development board based on [ESP32-C6-MINI-1(U)](https://www.espressif.com/en/products/modules), a general-purpose module with a 4 MB SPI flash in the chip's package. This board integrates complete Wi-Fi, Bluetooth LE, Zigbee, and Thread functions.

Most of the I/O pins are broken out to the pin headers on both sides for easy interfacing. Developers can either connect peripherals with jumper wires or mount ESP32-C6-DevKitM-1 on a breadboard.

The document consists of the following major sections:

- *Getting Started*: Overview of ESP32-C6-DevKitM-1 and hardware/software setup instructions to get started.
- *Hardware Reference*: More detailed information about the ESP32-C6-DevKitM-1's hardware.
- *Hardware Revision Details*: Revision history, known issues, and links to user guides for previous versions (if any) of ESP32-C6-DevKitM-1.
- *Related Documents*: Links to related documentation.

### 2.1.1 Getting Started

This section provides a brief introduction of ESP32-C6-DevKitM-1, instructions on how to do the initial hardware setup and how to flash firmware onto it.

#### Description of Components

The key components of the board are described in a clockwise direction.

**Fig. 2: ESP32-C6-DevKitM-1 — front (labelled components on the front side of the board)**

- ESP32-C6-MINI-1
- Pin Header (left)
- 5 V to 3.3 V LDO
- 3.3 V Power On LED
- USB-to-UART Bridge
- ESP32-C6 USB Type-C Port
- Boot Button
- Reset Button
- USB Type-C to UART Port
- RGB LED
- J5
- Pin Header (right)

| Key Component | Description |
|---|---|
| **ESP32-C6-MINI-1 or ESP32-C6-MINI-1U** | ESP32-C6-MINI-1 and ESP32-C6-MINI-1U are general-purpose modules supporting Wi-Fi 6 in 2.4 GHz band, Bluetooth 5, and IEEE 802.15.4 (Zigbee 3.0 and Thread 1.3). ESP32-C6-MINI-1 comes with an on-board PCB antenna, whereas ESP32-C6-MINI-1U comes with an external antenna connector. The module is built around the **ESP32-C6FH4 chip**, which has a **4 MB flash in the chip's package**. For more information, see *ESP32-C6-MINI-1 Datasheet*. |
| **Pin Header** | All available GPIO pins (except for the SPI bus for flash) are broken out to the pin headers on the board. |
| **5 V to 3.3 V LDO** | Power regulator that converts a 5 V supply into a 3.3 V output. |
| **3.3 V Power On LED** | Turns on when the USB power is connected to the board. |
| **USB-to-UART Bridge** | Single USB-to-UART bridge chip provides transfer rates up to 3 Mbps. |
| **ESP32-C6 USB Type-C Port** | The USB Type-C port on the ESP32-C6 chip compliant with USB 2.0 full speed. It is capable of up to 12 Mbps transfer speed (Note that this port does not support the faster 480 Mbps high-speed transfer mode). This port is used for power supply to the board, for flashing applications to the chip, for communication with the chip using USB protocols, as well as for JTAG debugging. |
| **Boot Button** | Download button. Holding down **Boot** and then pressing **Reset** initiates Firmware Download mode for downloading firmware through the serial port. |
| **Reset Button** | Press this button to restart the system. |
| **USB Type-C to UART Port** | Used for power supply to the board, for flashing applications to the chip, as well as the communication with the ESP32-C6 chip via the on-board USB-to-UART bridge. |
| **RGB LED** | Addressable RGB LED, driven by GPIO8. |
| **J5** | Used for current measurement. See details in Section *Current Measurement*. |

#### Start Application Development

Before powering up your ESP32-C6-DevKitM-1, please make sure that it is in good condition with no obvious signs of damage.

**Required Hardware**

- ESP32-C6-DevKitM-1
- USB-A to USB-C cable
- Computer running Windows, Linux, or macOS

> **Note:** Be sure to use a good quality USB cable. Some cables are for charging only and do not provide the needed data lines nor work for programming the boards.

**Software Setup** — Please proceed to [ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/get-started/index.html), which will quickly help you set up the development environment then flash an application example onto your board.

#### Contents and Packaging

**Retail orders** — If you order a few samples, each ESP32-C6-DevKitM-1 comes in an individual package in either antistatic bag or any packaging depending on your retailer.

For retail orders, please go to <https://www.espressif.com/en/company/contact/buy-a-sample>.

**Wholesale Orders** — If you order in bulk, the boards come in large cardboard boxes.

For wholesale orders, please check [Espressif Product Ordering Information](https://www.espressif.com/sites/default/files/documentation/espressif_products_ordering_information_en.pdf) (PDF).

### 2.1.2 Hardware Reference

#### Block Diagram

The block diagram below shows the components of ESP32-C6-DevKitM-1 and their interconnections.

**Fig. 3: ESP32-C6-DevKitM-1 (block diagram)**

```
                Power Supply/
                 Programming
                       │
 USB Type-C        D+/D-           TX/RX                ┌──────────┐
 to UART Port ───────────► USB-UART ─────────►          │ RGB LED  │
                            Bridge                      └────▲─────┘
                              │                              │ GPIO8
                              ▼                              │
                            ┌─────┐  3.3V  ┌──────────────────────┐
                            │ LDO │ ─────► │ ESP32-C6-MINI-1(U)   │ ───► Header Block ×2
                            └─────┘        └──────────────────────┘
                              ▲                  ▲       ▲
 ESP32-C6                     │                  │       │
 USB Type-C ──────────────────┘                [Boot]  [RST]
   Port
```

#### Power Supply Options

There are three mutually exclusive ways to provide power to the board:

- USB Type-C to UART Port and ESP32-C6 USB Type-C Port (either one or both), default power supply (recommended)
- 5V and GND pin headers
- 3V3 and GND pin headers

#### Current Measurement

The J5 headers on ESP32-C6-DevKitM-1 (see J5 in Figure *ESP32-C6-DevKitM-1 — front*) can be used for measuring the current drawn by the ESP32-C6-MINI-1(U) module:

- **Remove the jumper:** Power supply between the module and peripherals on the board is cut off. To measure the module's current, connect the board with an ammeter via J5 headers.
- **Apply the jumper (factory default):** Restore the board's normal functionality.

> **Note:** When using 3V3 and GND pin headers to power the board, please remove the J5 jumper, and connect an ammeter in series to the external circuit to measure the module's current.

#### Header Block

The two tables below provide the **Name** and **Function** of the pin headers on both sides of the board (J1 and J3). The pin header names are shown in Figure *ESP32-C6-DevKitM-1 — front*. The numbering is the same as in the ESP32-C6-DevKitM-1 Schematic (PDF).

**J1**

| No. | Name | Type¹ | Function |
|---:|---|---|---|
| 1  | 3V3  | P     | 3.3 V power supply |
| 2  | RST  | I     | High: Power up; Low: Power down. |
| 3  | 2    | I/O/T | GPIO2, LP_GPIO2, LP_UART_RTSN, ADC1_CH2, FSPIQ |
| 4  | 3    | I/O/T | GPIO3, LP_GPIO3, LP_UART_CTSN, ADC1_CH3 |
| 5  | 4    | I/O/T | MTMS³, GPIO4, LP_GPIO4, LP_UART_RXD, ADC1_CH4, FSPIHD |
| 6  | 5    | I/O/T | MTDI³, GPIO5, LP_GPIO5, LP_UART_TXD, ADC1_CH5, FSPIWP |
| 7  | 0/N  | I/O/T | GPIO0, XTAL_32K_P, LP_GPIO0, LP_UART_DTRN, ADC1_CH0 |
| 8  | 1/N  | I/O/T | GPIO1, XTAL_32K_N, LP_GPIO1, LP_UART_DSRN, ADC1_CH1 |
| 9  | 8    | I/O/T | GPIO8²,³ |
| 10 | 6    | I/O/T | MTCK, GPIO6, LP_GPIO6, LP_I2C_SDA, ADC1_CH6, FSPICLK |
| 11 | 7    | I/O/T | MTDO, GPIO7, LP_GPIO7, LP_I2C_SCL, FSPID |
| 12 | 14   | I/O/T | GPIO14 |
| 13 | G    | G     | Ground |
| 14 | 5V   | P     | 5 V power supply |
| 15 | G    | G     | Ground |

**J3**

| No. | Name | Type | Function |
|---:|---|---|---|
| 1  | G   | G     | Ground |
| 2  | TX  | I/O/T | U0TXD, GPIO16, FSPICS0 |
| 3  | RX  | I/O/T | U0RXD, GPIO17, FSPICS1 |
| 4  | 23  | I/O/T | GPIO23, SDIO_DATA3 |
| 5  | 22  | I/O/T | GPIO22, SDIO_DATA2 |
| 6  | 21  | I/O/T | GPIO21, SDIO_DATA1, FSPICS5 |
| 7  | 20  | I/O/T | GPIO20, SDIO_DATA0, FSPICS4 |
| 8  | 19  | I/O/T | GPIO19, SDIO_CLK, FSPICS3 |
| 9  | 18  | I/O/T | GPIO18, SDIO_CMD, FSPICS2 |
| 10 | 15  | I/O/T | GPIO15³ |
| 11 | 9   | I/O/T | GPIO9³ |
| 12 | G   | G     | Ground |
| 13 | 13  | I/O/T | GPIO13, USB_D+ |
| 14 | 12  | I/O/T | GPIO12, USB_D− |
| 15 | G   | G     | Ground |

**Notes:**
- ¹ P: Power supply; I: Input; O: Output; T: High impedance.
- ² Used to drive the RGB LED.
- ³ MTMS, MTDI, GPIO8, GPIO9, and GPIO15 are strapping pins of the ESP32-C6 chip. These pins are used to control several chip functions depending on binary voltage values applied to the pins during chip power-up or system reset. For description and application of the strapping pins, please refer to *ESP32-C6 Datasheet > Section Strapping Pins*.

#### Pin Layout

**Fig. 4: ESP32-C6-DevKitM-1 Pin Layout**

```
ESP32-C6 Specs (from the pin-layout diagram)
- 32-bit RISC-V single-core @160 MHz
- Wi-Fi IEEE 802.11 ax 2.4 GHz + Bluetooth LE 5 + IEEE 802.15.4 (Zigbee and Thread)
- 512 KB SRAM (21 KB for cache)
- 320 KB ROM
- 30 or 22 GPIOs, 3× SPI, 2× UART, 1× I2C, RMT
- LED PWM 6ch, 1× 12-bit ADC with 7ch, TWAI®
- USB Serial/JTAG, ETM, MCPWM, SDIO Slave
```

Pin-layout legend (color-coded zones on the diagram):
- PWM Capable Pin
- Fast SPI Functions
- General Purpose Input / Output
- Low-Power UART Functions
- Low-Power I2C Functions
- SDIO Functions
- Other Related Functions
- Strapping Pin Functions
- JTAG for Debugging and/or USB
- Analog-to-Digital Converter
- Serial for Debug/Programming
- Ground Plane
- Power Rails (3V3 and 5V)
- Low-Power GPIO Functions

### 2.1.3 Hardware Revision Details

- For boards with the PW number of and after **PW-2023-06-XXXX** (on and after June 2023), multi-point calibration is performed on ADC instead of two-point calibration, and the measurement range and accuracy are illustrated in *ESP32-C6 Datasheet > Section ADC Characteristics*. For boards with earlier PW number, please ask our sales team to provide the actual range and accuracy according to batch.

> **Note:** The PW number can be found in the product label on the large cardboard boxes for wholesale orders.

### 2.1.4 Related Documents

Please download the following documents from the HTML version of esp-dev-kits Documentation:

- ESP32-C6 Datasheet (PDF)
- ESP32-C6-MINI-1 Datasheet (PDF)
- ESP32-C6-DevKitM-1 Schematic (PDF)
- ESP32-C6-DevKitM-1 PCB Layout (PDF)
- ESP32-C6-DevKitM-1 Dimensions (PDF)
- ESP32-C6-DevKitM-1 Dimensions source file (DXF) — You can view it with Autodesk Viewer online

For further design documentation for the board, please contact us at <sales@espressif.com>.

---

# Chapter 3 — Related Documentation and Resources

## 3.1 Related Documentation

- **ESP32-C6 Datasheet** — Specifications of the ESP32-C6 hardware.
- **ESP32-C6 Technical Reference Manual** — Detailed information on how to use the ESP32-C6 memory and peripherals.
- **ESP32-C6 Hardware Design Guidelines** — Guidelines on how to integrate the ESP32-C6 into your hardware product.
- **ESP32-C6 Product/Process Change Notifications (PCN)** — <https://espressif.com/en/support/documents/pcns?keys=ESP32-C6>
- **ESP32-C6 Advisories** — Information on security, bugs, compatibility, component reliability. <https://espressif.com/en/support/documents/advisories?keys=ESP32-C6>
- **Certificates** — <https://espressif.com/en/support/documents/certificates>
- **Documentation Updates and Update Notification Subscription** — <https://espressif.com/en/support/download/documents>

## 3.2 Developer Zone

- **ESP-IDF Programming Guide for ESP32-C6** — Extensive documentation for the ESP-IDF development framework.
- **ESP-IoT-Solution Programming Guide** — Extensive documentation for the ESP-IoT-Solution development framework.
- **ESP-FAQ** — A summary document of frequently asked questions released by Espressif.
- **ESP-IDF and other development frameworks on GitHub** — <https://github.com/espressif>
- **ESP32 BBS Forum** — Engineer-to-Engineer (E2E) Community for Espressif products where you can post questions, share knowledge, explore ideas, and help solve problems with fellow engineers. <https://esp32.com/>
- **The ESP Journal** — Best Practices, Articles, and Notes from Espressif folks. <https://blog.espressif.com/>
- See the tabs **SDKs and Demos**, **Apps**, **Tools**, **AT Firmware** — <https://espressif.com/en/support/download/sdks-demos>

## 3.3 Products

- **ESP32-C6 Series SoCs** — Browse through all ESP32-C6 SoCs. <https://espressif.com/en/products/socs?id=ESP32-C6>
- **ESP32-C6 Series Modules** — Browse through all ESP32-C6-based modules. <https://espressif.com/en/products/modules?id=ESP32-C6>
- **ESP32-C6 Series DevKits** — Browse through all ESP32-C6-based devkits. <https://espressif.com/en/products/devkits?id=ESP32-C6>
- **ESP Product Selector** — Find an Espressif hardware product suitable for your needs by comparing or applying filters. <https://products.espressif.com/#/product-selector>

## 3.4 Contact Us

- See the tabs **Sales Questions**, **Technical Enquiries**, **Circuit Schematic & PCB Design Review**, **Get Samples (Online stores)**, **Become Our Supplier**, **Comments & Suggestions**. <https://espressif.com/en/contact-us/sales-questions>

---

# Chapter 4 — Disclaimer and Copyright Notice

Information in this document, including URL references, is subject to change without notice.

All third party's information in this document is provided as is with no warranties to its authenticity and accuracy.

No warranty is provided to this document for its merchantability, non-infringement, fitness for any particular purpose, nor does any warranty otherwise arising out of any proposal, specification or sample.

All liability, including liability for infringement of any proprietary rights, relating to use of information in this document is disclaimed. No licenses express or implied, by estoppel or otherwise, to any intellectual property rights are granted herein.

The Wi-Fi Alliance Member logo is a trademark of the Wi-Fi Alliance. The Bluetooth logo is a registered trademark of Bluetooth SIG.

All trade names, trademarks and registered trademarks mentioned in this document are property of their respective owners, and are hereby acknowledged.

---

> **Document feedback** — See "Submit Document Feedback" link on each page of the original PDF.
> **Espressif Systems · Release master · May 27, 2026.**
