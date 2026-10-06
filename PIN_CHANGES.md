# Pin changes (PCB rework)

What changed between the V1 and V2 boards. The pin moves were made in the PCB
editor and have been applied to `include/pod_config.hpp` and the copy in
`tools/tof_test/src/main.cpp`. Not yet tested on a built V2 board.

V1 firmware is commit `f138950` (`git checkout f138950`).

## All changes at a glance

Eight pins moved. Everything else is the same.

| Signal | V1 GPIO | V2 GPIO |
|---|---|---|
| Vertical encoder A | 1 | **40** |
| Vertical encoder B | 2 | **42** |
| Horizontal encoder A | 3 | **39** |
| Horizontal encoder B | 4 | **41** |
| RS-485 DE/RE | 21 | **7** |
| ToF EN, FRONT | 11 | **4** |
| ToF EN, REAR | 14 | **1** |
| ToF EN, LEFT | 13 | **2** |

Unchanged: IMU RX 8, RS-485 TX/RX 17/18, I2C SDA/SCL 16/15, ToF EN RIGHT 12.

### V1 and V2 firmware are NOT interchangeable

Some GPIO numbers changed job, so flashing the wrong firmware drives the wrong
wire:

| GPIO | On V1 | On V2 |
|---|---|---|
| 1 | vertical encoder A (input) | ToF EN, REAR (output) |
| 2 | vertical encoder B (input) | ToF EN, LEFT (output) |
| 4 | horizontal encoder B (input) | ToF EN, FRONT (output) |
| 3 | horizontal encoder A | unused |
| 21 | RS-485 DE | unused |
| 11, 13, 14 | ToF EN | unused |

Always flash V2 firmware on the V2 board and V1 firmware on the V1 board.

Board: ESP32-S3-DevKitC-1, **N8R8** module. GPIO33-37 are wired to the octal
PSRAM inside the module and must not be used.

## Encoders

Moved from GPIO1-4 to GPIO39-42, four adjacent pins on header J3 (pins 6-9).

The converter (SN74LVC245A) is wired so B1/B2 carry the encoders' **B**
channels and B3/B4 carry their **A** channels, to make the wiring easier.
Each converter pin passes straight across to its partner (A1 <-> B1, etc.).

| Converter pin | Encoder wire     | ESP32 GPIO | Header pin | Old GPIO |
|---------------|------------------|------------|------------|----------|
| B1            | Vertical **B**   | 42         | J3_6       | 2        |
| B2            | Horizontal **B** | 41         | J3_7       | 4        |
| B3            | Vertical **A**   | 40         | J3_8       | 1        |
| B4            | Horizontal **A** | 39         | J3_9       | 3        |

Assumes DIR is tied high (A side -> B side): encoders on A1-A4, ESP32 on B1-B4.
If DIR is tied low, swap the sides.

### Code change (applied)

```cpp
constexpr int kVertEncAPin  = 40;
constexpr int kVertEncBPin  = 42;
constexpr int kHorizEncAPin = 39;
constexpr int kHorizEncBPin = 41;
```

If an encoder counts backwards after this, its A and B are swapped: flip
`kVertReversed` / `kHorizReversed` rather than rewiring.

## ToF sensors

FRONT, REAR and LEFT sensor EN moved to GPIO4, GPIO1 and GPIO2, which the
encoders freed up. GPIO11, GPIO13 and GPIO14 are now unused. (GPIO21 was
considered for LEFT but was RS-485 DE at the time; GPIO19 is native USB D-.)

| Signal            | Old GPIO | New GPIO |
|-------------------|----------|----------|
| EN, FRONT (tof0)  | 11       | 4        |
| EN, REAR (tof2)   | 14       | 1        |
| EN, LEFT (tof3)   | 13       | 2        |

### Code change (applied)

`include/pod_config.hpp` and `tools/tof_test/src/main.cpp`:

```cpp
constexpr int kTofEnPins[4] = {4, 12, 1, 2};
```

## RS-485 link

DE (tied to RE on the SP3485) moved to GPIO7 on the left header (J1), with
TX/RX on 17/18. It is the nearest free non-strapping pin: 15/16 between them
are I2C. GPIO21 is now unused. TX and RX are unchanged.

| Signal                | Old GPIO | New GPIO |
|-----------------------|----------|----------|
| RS-485 DE/RE (SP3485) | 21       | 7        |

### Code change (applied)

`include/pod_config.hpp`:

```cpp
constexpr int kRs485DePin = 7;
```

## Unchanged

| Signal              | GPIO               |
|---------------------|--------------------|
| IMU RX (BNO085)     | 8                  |
| RS-485 TX / RX      | 17 / 18            |
| I2C SDA / SCL       | 16 / 15            |
| ToF EN RIGHT        | 12                 |
