# MFRC522 Driver & RFIDTag Library for STM32

## 📖 Table of Contents

1. [Introduction](#introduction)  
2. [What is the MFRC522?](#what-is-the-mfrc522)  
3. [Hardware Setup / Pinout](#hardware-setup--pinout)  
4. [How ISO14443A Tag Exchange Works](#how-iso14443a-tag-exchange-works)  
5. [Frame & Register Structure](#frame--register-structure)  
6. [Key Features](#key-features)  
7. [Card Type Detection](#card-type-detection)  
8. [Integration](#integration)  
9. [Getting Started](#getting-started)  

---

## Introduction
This repository provides a **high-performance**, **production-ready** C driver for the **NXP MFRC522** RFID reader, plus a higher-level **RFIDTag** abstraction layer. It enables STM32-based embedded systems to discover, anticollide, select, authenticate, read from and write to ISO14443A / MIFARE Classic and Ultralight tags.

Whether you’re building access control, asset tracking, or IoT sensor networks, this driver gives you a clean, portable, and fully-featured foundation.

---

## What is the MFRC522?
The **MFRC522** is an NXP-made, low-cost, highly integrated contactless 13.56 MHz RFID reader/writer IC. It communicates with microcontrollers over **SPI**, **I²C**, or **UART**, and fully implements the ISO14443A protocol for card discovery, anticollision, selection, CRC, and data exchange.

Key hardware highlights:  
- Operating voltage 2.5…3.3 V  
- SPI clock up to 10 MHz  
- On-chip antenna driver  
- Integrated CRC coprocessor  
- Support for MIFARE Classic, Ultralight, NTAG, DESFire, Type 4 tags

---

## Hardware Setup / Pinout

A small table showing exactly how to wire your MFRC522 to an **STM32G431XX** (using SPI2):

| MFRC522 Pin   | STM32 Pin | Notes                  |
| ------------- | --------- | ---------------------- |
| VCC           | 3.3 V      |                        |
| GND           | GND       |                        |
| SDA (CS)      | PA4       | `MFRC522_CS_PIN`       |
| SCK           | PA5       | `SPI1_SCK`             |
| MOSI          | PA7       | `SPI1_MOSI`            |
| MISO          | PA6       | `SPI1_MISO`            |
| RST           | PB0       | `MFRC522_RST_PIN`      |
| IRQ (optional)| PB1       | External interrupt     |

> **Note:** This example wiring uses the STM32G431XX series and the SPI2 peripheral. Ensure your CubeMX or HAL setup matches these pin assignments.


---

## How ISO14443A Tag Exchange Works
1. **REQA / WUPA**  
   - Reader broadcasts a 7-bit “Request” (0x26 or 0x52).  
   - Tag(s) reply with **Answer-To-Request (ATQA)** (2 bytes) indicating basic capabilities.

2. **Anticollision**  
   - If multiple tags respond, the reader enters anticollision loops (`PICC_ANTICOLL`) to retrieve one 4-byte UID (plus BCC).

3. **Select**  
   - Reader sends the full UID in a **SELECT** frame.  
   - Tag returns a one-byte **SAK** (Select Acknowledge), confirming cascade levels and ISO-4 support.

4. **Authenticate / Transceive**  
   - For MIFARE Classic, reader authenticates a block using Key A/B.  
   - Reader then reads or writes 16-byte data blocks via **Transceive** commands.

5. **Halt**  
   - Once operations are complete, the reader issues **HALT** to put the tag to sleep.

---

## Frame & Register Structure

| Phase          | Command                  | Sent Bytes                                     | Response Length            |
|:--------------:|:-------------------------|:-----------------------------------------------|:---------------------------|
| Request        | `PICC_REQIDL` (0x26)     | 1 byte                                         | 16 bits (ATQA)             |
| Anticollision  | `PICC_ANTICOLL` (0x93)   | 2 bytes (`0x93`, NVB=0x20)                     | 32 bits (UID + BCC)        |
| Select         | `PICC_SELECTTAG` (0x93)  | 7 bytes UID + 2 bytes CRC                      | 24 bits (SAK)              |
| Authenticate   | `PCD_AUTHENT` (0x0E)     | 12 bytes (authMode, blockAddr, key, UID) + CRC | 4 bits ACK                 |
| Read / Write   | `PICC_READ` / `PICC_WRITE` | 4 or 18 bytes (with CRC)                       | 0x90 bits / 4 bits ACK     |
| Halt           | `PICC_HALT` (0x50)       | 4 bytes (with CRC)                             | None                       |

All commands and responses go through the MFRC522’s FIFO register, with interrupts indicating completion. CRC is offloaded to the on-chip coprocessor.

---

## Key Features

- **Low-level MFRC522 driver** (`src/MFRC522.c`)  
  - SPI read/write registers  
  - Bit-mask setting and clearing  
  - CRC calculation via hardware  
  - Command engine (`ToCard`) with timeout and error handling  

- **High-level RFIDTag API** (`src/RfidTag.c`)  
  - Simplified `ReadBlock()`, `WriteBlock()`, `AuthenticateBlock()`, `ScanTag()`  
  - Event callbacks for **tag detected**, **tag lost**, **read success**, **auth fail**  

- **Tag type detection**  
  - Parses **ATQA** and **SAK** to identify Ultralight, Classic, DESFire, Type 4, etc.

- **Configurable**  
  - `include/MFRC522.h` and `include/RfidTag.h` expose all registers, commands, and APIs.  
  - Define your SPI handle, GPIO pins, and timeouts via `main.h` or project options.

- **Example projects**  
  - `examples/polling/` – A simple polling-based demo for STM32G431XX showing how to use the RFIDTag abstraction to detect a tag, authenticate, write a block, read it back, and print tag info.

- **Portable & Minimal**  
  - ANSI-C99, no RTOS dependency (works with FreeRTOS, CMSIS-RTOS, or bare-metal).  
  - Can be compiled as a static library or integrated directly.

---

## Card Type Detection

Use ATQA and SAK values to identify common cards:

| Card Type              | ATQA   | SAK  |
|------------------------|--------|------|
| MIFARE Ultralight      | 0x0044 | 0x00 |
| MIFARE Classic 1K      | 0x0004 | 0x08 |
| MIFARE Classic 4K      | 0x0002 | 0x18 |
| MIFARE DESFire         | 0x0344 | 0x20 |

---

## Integration

1. Copy `MFRC522.c` and `MFRC522.h` into your STM32 project.  
2. Define your SPI handle (e.g., `hspi2`) and CS/RST GPIOs in `main.h`.  
3. Call `MFRC522_Init()` in `main()` and start polling for tags.  

**Notes:**  
- The RST pin should be configured as a GPIO output and driven **High** on startup.  
- A `printf()` log feature is available via **SWV (Serial Wire Viewer)** for easy debugging.  

---

## Getting Started

**Clone the repository or download it manually**  
```bash
git clone https://github.com/Regulus98/MFRC522/examples/MFRC522_Driver.zip
cd MFRC522_Driver.zip
