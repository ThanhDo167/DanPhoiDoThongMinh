# Smart Dry Rack

An automated clothes drying rack control system based on the STM32F103C8T6 microcontroller. The system automatically extends or retracts the rack based on environmental conditions (rain, light, humidity), supports manual control via a 4×4 matrix keypad, displays real-time status on a 16×2 LCD, and provides remote monitoring through UART/PuTTY.

---

## Table of Contents

1. [Features](#1-features)
2. [Hardware and Pin Configuration](#2-hardware-and-pin-configuration)
3. [Software Architecture](#3-software-architecture)
4. [Module Descriptions](#4-module-descriptions)
5. [System Logic](#5-system-logic)
6. [Keypad Control](#6-keypad-control)
7. [UART Monitoring via PuTTY](#7-uart-monitoring-via-putty)
8. [Build and Flash](#8-build-and-flash)
9. [Limitations and Future Work](#9-limitations-and-future-work)

---

## 1. Features

**AUTO Mode**
- Automatically extends the rack when it is bright, dry, and humidity is below 80%.
- Automatically retracts the rack when rain is detected, light is insufficient, or humidity exceeds 80%.
- Rain is the absolute highest priority condition — the rack is always retracted regardless of the current mode.

**MANUAL Mode**
- Enter MANUAL mode by typing code `1#` on the keypad.
- Use keys `A / B / C / D` to retract, extend, turn the fan on, or turn the fan off.
- Entering MANUAL mode is blocked while it is raining; extending the rack is also blocked while raining even if already in MANUAL mode.
- Automatically returns to AUTO after 15 seconds of inactivity.
- Type `0#` to exit MANUAL mode and return to AUTO immediately.

**Rain Protection**
- The rack retracts immediately when rain is detected, regardless of the current mode.
- The fan turns on automatically once the rack has fully retracted, to circulate air and assist drying.
- The fan continues running for the entire duration of the rain.
- After rain stops, the fan runs for an additional 10 seconds before turning off.

**Rain Alarm**
- The buzzer sounds when rain is detected.
- Type `1234#` to silence the buzzer. This works independently of AUTO/MANUAL mode.
- The buzzer automatically resets for the next rain event.

**Security**
- The system locks completely after 5 consecutive incorrect code entries.

**Remote Monitoring**
- A read-only dashboard transmitted over UART updates every second in PuTTY, displaying all sensor readings and system states.

---

## 2. Hardware and Pin Configuration

### Components

| Component | Function |
|---|---|
| STM32F103C8T6 (Blue Pill) | Main microcontroller |
| 28BYJ-48 stepper motor + L298N driver | Extends and retracts the rack |
| Rain sensor module (digital output) | Detects rainfall |
| LDR (Light Dependent Resistor) | Detects ambient light level |
| DHT11 | Measures temperature and humidity |
| 16×2 LCD + I2C module (PCF8574) | Displays system status |
| 4×4 matrix keypad | Manual user input |
| 5V relay module | Controls the fan |
| Buzzer | Rain alarm |
| USB-TTL adapter (CP2102 / CH340) | UART connection to PC |

### STM32 Pin Mapping

| STM32 Pin | Function | Notes |
|---|---|---|
| PA0 | LDR — ADC1 Channel 0 | Reads light level via ADC |
| PA1 | Rain sensor digital output | INPUT PULLUP — LOW = raining |
| PA3 | Buzzer | OUTPUT — HIGH = buzzer on |
| PA4 | Fan relay | OUTPUT — HIGH = fan on |
| PA5 | Stepper IN1 | L298N input |
| PA6 | Stepper IN2 | L298N input |
| PA7 | Stepper IN3 | L298N input |
| PA9 | USART1 TX | Connect to RX of USB-TTL adapter |
| PA10 | USART1 RX | Unused — TX-only mode |
| PB8 | Stepper IN4 | L298N input |
| PB6 | I2C1 SCL | LCD via PCF8574 |
| PB7 | I2C1 SDA | LCD via PCF8574 |
| PB0 | Keypad ROW 1 | OUTPUT |
| PB1 | Keypad ROW 2 | OUTPUT |
| PB10 | Keypad ROW 3 | OUTPUT |
| PB11 | Keypad ROW 4 | OUTPUT |
| PB12 | Keypad COL 1 | INPUT PULLUP |
| PB13 | Keypad COL 2 | INPUT PULLUP |
| PB14 | Keypad COL 3 | INPUT PULLUP |
| PB15 | Keypad COL 4 | INPUT PULLUP |

### Clock Configuration

| Parameter | Value |
|---|---|
| Clock source | HSE (external crystal) |
| PLL multiplier | × 9 |
| SYSCLK | 72 MHz |
| APB1 | 36 MHz |
| APB2 | 72 MHz |
| ADC clock | APB2 / 6 = 12 MHz |

---

## 3. Software Architecture

### File Structure

```
Core/
├── Inc/
│   ├── system_state.h   — Shared global variables (extern declarations)
│   ├── sensor.h         — Rain sensor, LDR, DHT11
│   ├── stepper.h        — Non-blocking stepper motor driver
│   ├── actuator.h       — Fan (relay) and buzzer
│   ├── logic.h          — AUTO / MANUAL business logic
│   ├── keypad.h         — 4×4 matrix keypad scanner
│   ├── display.h        — 16×2 LCD over I2C
│   └── uart_cli.h       — Read-only UART dashboard
└── Src/
    ├── system_state.c   — Global variable definitions
    ├── sensor.c
    ├── stepper.c
    ├── actuator.c
    ├── logic.c
    ├── keypad.c
    ├── display.c
    ├── uart_cli.c
    └── main.c           — HAL initialization and main loop
```

### Layered Architecture

```
┌─────────────────────────────────────────┐
│              main.c                     │  Orchestration
├──────────┬──────────┬───────────────────┤
│ keypad.c │display.c │   uart_cli.c      │  Input / Output
├──────────┴──────────┴───────────────────┤
│              logic.c                    │  Business Logic
├─────────────────────────────────────────┤
│  sensor.c  │  stepper.c  │ actuator.c  │  Hardware Abstraction
├─────────────────────────────────────────┤
│              HAL / CubeMX               │  STM32 Drivers
└─────────────────────────────────────────┘
```

---

## 4. Module Descriptions

### system_state.h / system_state.c

The single source of truth for all global variables. Every other module includes `system_state.h` and accesses variables through `extern` declarations — no module defines its own copy of shared state.

Key state variables:

| Variable | Type | Description |
|---|---|---|
| `currentState` | SystemState | Actual physical position of the rack |
| `targetState` | SystemState | Target position the rack should move to |
| `manual_mode` | uint8_t | 0 = AUTO, 1 = MANUAL |
| `system_locked` | uint8_t | 1 = locked after 5 wrong code entries |
| `temp` / `humi` | float | Temperature (°C) and humidity (%) |
| `last_isRaining` | uint8_t | Filtered rain sensor result |
| `last_isBright` | uint8_t | 1 = sufficient light, 0 = dark |
| `fan_state` | uint8_t | Current fan state (ON / OFF) |
| `fan_manual` | uint8_t | 0 = fan controlled automatically, 1 = manual |

Control codes defined here as constants:

| Constant | Value | Purpose |
|---|---|---|
| `CORRECT_PASS` | `"1234"` | Code to silence the buzzer |
| `MANUAL_ENTER_CODE` | `"1"` | Code to enter MANUAL mode |
| `MANUAL_EXIT_CODE` | `"0"` | Code to return to AUTO mode |
| `MAX_WRONG_PASS` | `5` | Wrong entries before system lock |

### sensor.c

Reads raw data from hardware sensors. Contains no business logic.

- **`Sensor_RainDetect()`** — Majority-vote filter: reads the rain sensor 5 times at 5 ms intervals and returns 1 if at least 3 of 5 readings are LOW (rain detected). This eliminates false triggers from vibration or brief water splashes.

- **`Sensor_ReadLDR()`** — Starts an ADC polling conversion on channel 0 (PA0), reads the 12-bit result, then stops ADC. Values below 2000 are interpreted as "bright" by the caller in `main.c`.

- **`Sensor_ReadDHT11()`** — Wrapper around the DHT11 library. On read failure, writes 0.0 to both output pointers. `logic.c` uses a guard condition `humidity > 0.0f` before checking the 80% threshold, so a failed read does not incorrectly influence the rack decision.

### stepper.c

Controls the 28BYJ-48 stepper motor via the L298N driver using 8-step half-step sequencing. Fully non-blocking — no `HAL_Delay()` is used.

- **`Stepper_Start(steps, dir)`** — Stores the target step count and direction, then sets the `s_motor_running` flag. Does not move the motor by itself.

- **`Stepper_Run()`** — Called every iteration of `while(1)`. Uses `HAL_GetTick()` to advance exactly one step if at least `STEPPER_DELAY` (1 ms) has elapsed since the last step. When the step count reaches `STEPPER_STEPS` (4096), powers off the coils and clears the running flag.

- **`Stepper_IsBusy()`** — Returns `s_motor_running`. Used by `main.c` to detect the falling edge (1 → 0 transition) that signals the motor has just completed its movement.

- **`Stepper_PowerOff()`** — Writes LOW to all four coil pins (IN1–IN4) to prevent the motor from heating up while stationary.

### actuator.c

The only module permitted to write to the relay and buzzer GPIO pins.

- **`Actuator_SetFan(state)`** — Drives the relay pin and synchronises the `fan_state` global variable.

- **`Actuator_HandleBuzzer(isRaining)`** — Priority order: system locked → buzzer silenced by user (`buzzer_off`) → rain detected. Automatically clears `buzzer_off` when rain stops, so the buzzer will sound again on the next rain event.

### logic.c

The decision-making core. Never reads GPIO directly.

- **`Logic_UpdateRackState(isRaining, isBright, humidity)`** — Determines `targetState` with the following priority:
  1. **Rain** — always sets `STATE_THU` (retract), ignores `manual_mode`.
  2. **MANUAL mode, no rain** — returns without changing `targetState`, preserving the user's last command.
  3. **AUTO mode, no rain** — sets `STATE_PHOI` (extend) only if it is bright and humidity is valid and below 80%; otherwise sets `STATE_THU`.

- **`Logic_HandleFanAuto(isRaining)`** — Manages fan behaviour across rain events. Turns the fan on once `currentState == STATE_THU` while raining. Detects the falling edge of `isRaining` to start a 10-second post-rain cooldown timer, then turns the fan off. Also checks whether the 15-second MANUAL inactivity timeout has elapsed and resets `manual_mode` to 0 if so.

- **`Logic_SetManualStartTick(tick)`** — Stores the `HAL_GetTick()` value at the moment the user performs any action in MANUAL mode. Called from `keypad.c` on every `A / B / C / D` press and on `1#` entry. Resets the 15-second inactivity timer.

### keypad.c

Scans the 4×4 matrix using row-low / column-read technique with 20 ms debounce and waits for key release before returning.

`Keypad_Handle()` dispatches to the following actions:

| Key / Code | Condition | Action |
|---|---|---|
| `1#` | Not raining | Enter MANUAL mode |
| `1#` | Raining | Rejected — LCD shows "DANG MUA!" |
| `0#` | Any | Exit MANUAL, return to AUTO; recompute `targetState` immediately |
| `A` | In MANUAL | Retract rack |
| `B` | In MANUAL, not raining | Extend rack |
| `B` | In MANUAL, raining | Rejected — LCD shows "DANG MUA!" |
| `C` | In MANUAL | Turn fan on (manual) |
| `D` | In MANUAL | Turn fan off (manual) |
| `A / B / C / D` | Not in MANUAL | LCD prompts "NHAP 1# TRUOC" |
| `1234#` | Any | Silence buzzer; does not affect mode |
| Wrong code + `#` | — | Increment `wrong_count`; lock after 5 |
| `*` | — | Clear code input buffer |

Each LCD notification is held for 1500 ms (`LCD_FLASH_HOLD_MS`) before the display returns to normal, giving the user time to read it.

### display.c

Drives the 16×2 LCD over I2C using a two-line string cache (`old_line1`, `old_line2`). The display is only written when content actually changes, reducing unnecessary I2C traffic on the 100 kHz bus.

Line 1 format: `T:<°C> H:<%%> F:<fan> E:<wrong>`
Line 2 format (normal): `<Rain/Sun>|<Bright/Dark>|<In/Out>`
Line 2 format (during code input): `PASS:***`

### uart_cli.c

Transmit-only (TX) dashboard sent to PuTTY over USART1 at 115200 baud. Updates every second without causing screen flicker.

**Anti-flicker technique:** The static frame (borders, title, labels) is drawn once at startup using `ANSI_CLEAR`. Every second, only the value portion of each data line is overwritten using absolute cursor positioning (`\033[row;colH`) followed by `ANSI_CLREOL` to erase leftover characters. The full screen is never cleared after initialisation, so no blank flash occurs between updates.

### main.c

Initialises all HAL peripherals, then runs the `while(1)` loop in four sequential sections:

**Section 1 — Keypad** (every loop iteration)
```c
Keypad_Handle();
```

**Section 2 — Stepper motor control** (every loop iteration, non-blocking)
```c
uint8_t prev_busy = Stepper_IsBusy();
Stepper_Run();

// Sync position only on the exact cycle the motor stops (falling edge)
if (prev_busy && !Stepper_IsBusy())
    currentState = targetState;

// Start a new movement only when motor is idle and position is wrong
if (!Stepper_IsBusy() && currentState != targetState)
    Stepper_Start(STEPPER_STEPS, dir);
```

**Section 3 — 1-second update block**
```c
if (HAL_GetTick() - lastUpdate > 1000U) {
    Sensor_ReadDHT11(&temp, &humi);
    adcValue       = Sensor_ReadLDR();
    last_isBright  = (adcValue < LIGHT_THRESHOLD) ? 1 : 0;
    last_isRaining = Sensor_RainDetect();

    Logic_UpdateRackState(last_isRaining, last_isBright, humi);
    Logic_HandleFanAuto(last_isRaining);

    Actuator_HandleBuzzer(last_isRaining);
    Display_Update(last_isRaining, last_isBright);
    UART_CLI_Update();
}
```

---

## 5. System Logic

### Rack State Decision Flow

```
Read sensors every 1 second
            │
            ▼
      Is it raining?
      ┌─────YES──────────────────────┐
      │                              │
      ▼ NO                           ▼
  Is MANUAL mode active?       RETRACT rack
  ┌────YES────┐                (overrides mode)
  │           │
  ▼           ▼ NO (AUTO mode)
Keep       Is humidity > 80%
current    or is it dark?
state      ┌─────YES──────┐
           │              │
           ▼              ▼ NO
       RETRACT         EXTEND
```

### Fan Behaviour During Rain

```
Rain detected
      │
      ▼
Motor retracts rack
      │
      ▼
currentState == STATE_THU?
      │ YES
      ▼
Fan ON ──────────────────────────────────────────────┐
      │                                              │
      └──────────── Rain continues: fan stays ON ───┘
                                │
                    Rain stops (falling edge)
                                │
                                ▼
                        Start 10-second timer
                                │
                                ▼
                           Fan OFF
```

### Motor Edge-Detection Pattern

A naive implementation that writes `currentState = targetState` whenever `!IsBusy()` introduces a critical bug: when `targetState` changes while the motor is idle, `currentState` is immediately pulled to match it, making the start condition `currentState != targetState` permanently false — the motor never moves.

The fix is falling-edge detection on `IsBusy()`:

```c
uint8_t prev_busy = Stepper_IsBusy();
Stepper_Run();

// Update position only on the cycle the motor JUST stopped (1 → 0)
if (prev_busy && !Stepper_IsBusy())
    currentState = targetState;

// Start only when idle and not yet at target
if (!Stepper_IsBusy() && currentState != targetState) {
    int dir = (targetState == STATE_THU) ? 1 : -1;
    Stepper_Start(STEPPER_STEPS, dir);
}
```

This ensures `currentState` is updated exactly once — on the cycle the motor finishes — and not prematurely while the motor is waiting for a new command.

---

## 6. Keypad Control

### Keypad Layout

```
┌───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ A │  A = Retract rack    (MANUAL only)
├───┼───┼───┼───┤
│ 4 │ 5 │ 6 │ B │  B = Extend rack     (MANUAL only, no rain)
├───┼───┼───┼───┤
│ 7 │ 8 │ 9 │ C │  C = Fan ON          (MANUAL only)
├───┼───┼───┼───┤
│ * │ 0 │ # │ D │  D = Fan OFF         (MANUAL only)
└───┴───┴───┴───┘
  * = Clear current code input
  # = Confirm code
```

### Control Codes

| Code | Function |
|---|---|
| `1` + `#` | Enter MANUAL mode (blocked while raining) |
| `0` + `#` | Exit MANUAL, return to AUTO immediately |
| `1234` + `#` | Silence rain buzzer (works in any mode) |
| Any other code + `#` | Wrong code — increments error counter |

### Security

- After **5 incorrect code entries** (of any code), the system is fully locked (`system_locked = 1`).
- While locked, all keypad input is ignored, the buzzer is silenced, and the LCD displays "BI KHOA!".
- The only way to unlock is a hardware reset (RESET button on the board).

---

## 7. UART Monitoring via PuTTY

### PuTTY Configuration

| Setting | Value |
|---|---|
| Connection type | Serial |
| Serial line | COM* (check Windows Device Manager) |
| Speed (baud rate) | 115200 |
| Data bits | 8 |
| Stop bits | 1 |
| Parity | None |
| Flow control | None |

Under **Terminal**: enable `Implicit CR in every LF`.
Under **Window → Colours**: enable `Allow terminal to use xterm 256-colour mode`.

### Physical Connection

```
USB-TTL adapter RX pin  →  PA9  (STM32 USART1 TX)
USB-TTL adapter GND     →  GND
```

PA10 (USART1 RX) is not used because the dashboard is transmit-only.

### Dashboard Layout

```
+=============================================+
|      SMART DRY RACK - SYSTEM MONITOR       |
+---------------------------------------------+
|  [SENSORS]                                  |
| TEMPERATURE : 28.5 C                        |
| HUMIDITY    : 65.0 %                        |
| LIGHT       : BRIGHT  (ADC:   98)           |
| RAIN        : NO RAIN                       |
+---------------------------------------------+
|  [STATUS]                                   |
| MODE        : AUTO                          |
| RACK        : EXTENDED                      |
| FAN         : OFF   (Auto  )                |
| SYSTEM      : RUNNING                       |
| WRONG CODE  : 0 / 5                         |
+=============================================+
```

Color coding:
- **Green** — normal / good
- **Yellow** — warning / motor moving / MANUAL mode active
- **Red** — alert / raining / system locked

The dashboard refreshes every 1 second. Only the value fields are overwritten using ANSI cursor positioning (`\033[row;colH`) — the full screen is never cleared after the initial draw, so there is no visible flicker.

---

## 8. Build and Flash

### Requirements

- STM32CubeIDE 1.10 or later (includes STM32CubeMX)
- ST-Link V2 programmer (or the built-in ST-Link on a Nucleo board)
- Two additional library files placed in `Core/Src` and `Core/Inc`:
  - `dht11.c` / `dht11.h` — DHT11 one-wire driver
  - `lcd_i2c.c` / `lcd_i2c.h` — 16×2 LCD driver over PCF8574 I2C expander

### Steps

1. Extract or clone the project into a working directory.
2. Open STM32CubeIDE and select **File → Open Projects from File System**, pointing to the project folder.
3. Add `dht11.c`, `dht11.h`, `lcd_i2c.c`, `lcd_i2c.h` to `Core/Src` and `Core/Inc` respectively.
4. Verify that `Core/Inc` is listed under **Project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths**.
5. Press **Build** (`Ctrl+B`). Expected result: 0 errors, at most 2 warnings.
6. Flash the firmware via ST-Link using **Run → Debug**, or use STM32CubeProgrammer with the `.hex` file from the `Debug/` folder.

### Common Build Warning

```
warning: passing argument 1 of 'lcd_send_string' discards 'const' qualifier
```

**Cause:** The `lcd_send_string()` function in the third-party library declares its parameter as `char *str` instead of `const char *str`.

**Fix option 1** — Change the prototype in `lcd_i2c.h` to `void lcd_send_string(const char *str)` if the implementation does not modify the string (it typically does not).

**Fix option 2** — Add an explicit cast at each call site: `lcd_send_string((char *)my_string)`. This suppresses the warning but does not fix the underlying mismatch.

---

## 9. Limitations and Future Work

### Current Limitations

**Open-loop motor control** — The system counts the number of step commands issued (`s_step_count`) but has no feedback on actual shaft position. If the motor skips steps due to excessive load (wet, heavy clothes) or loses power mid-movement, `currentState` will still be updated as if the rack reached its target position, even if it did not. There is no way to detect or recover from this mismatch without a hardware position sensor.

**DHT11 reliability** — DHT11 has a relatively high read-failure rate, particularly when the main loop is blocked by `HAL_Delay()` calls in the keypad handler. On failure, both temperature and humidity are set to 0.0, and the humidity condition is excluded from the rack decision (guarded by `humidity > 0.0f`). The system continues to operate using rain and light data only.

**Keypad blocking delay** — Each LCD notification holds the display for 1500 ms using `HAL_Delay()`, which blocks the entire main loop including `Stepper_Run()`. If the motor is mid-movement when a key is pressed, it pauses for up to 1.5 seconds. This is an intentional trade-off since notifications are only triggered by deliberate user input, not automatically.

**No persistent state** — All runtime state is stored in SRAM and is lost on power loss. The system always boots with `currentState = STATE_THU` (retracted), regardless of where the rack was physically positioned before power was removed.

### Future Improvements

- Add **limit switches** at both ends of the rack travel to calibrate physical position on startup, converting the control loop from open-loop to closed-loop.
- Replace DHT11 with **DHT22** or **SHT31** for higher accuracy, faster response, and greater read reliability.
- Use **TIM interrupt** for the stepper motor instead of polling `HAL_GetTick()` inside the main loop, eliminating timing dependency on loop execution speed.
- Use **DMA** for UART transmission instead of blocking `HAL_UART_Transmit()` to free the CPU during data transfer.
- Add **Wi-Fi connectivity** (ESP8266 or ESP32 co-processor) to enable smartphone control and push notifications when rain is detected.
- Store critical state in **internal Flash or EEPROM** to survive power loss and resume in the correct position on reboot.
- Replace the 16×2 LCD with an **OLED display** for more information density and a cleaner visual interface.
