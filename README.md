# Stepper Sequence Runner — PIC16F877A

A **PIC16F877A** microcontroller-based stepper motor sequence controller with **4x4 keypad** input and **SSD1306 OLED** display. Supports building, saving, and running multi-step motor sequences with 4 step types.

---

## Hardware Used


| Component                | Purpose                               |
| ------------------------ | ------------------------------------- |
| PIC16F877A               | Main microcontroller (20 MHz crystal) |
| 28BYJ-48 Stepper Motor   | Load                                  |
| ULN2003A Driver Board    | Stepper motor driver                  |
| 4x4 Matrix Keypad        | Step programming and control          |
| SSD1306 OLED (0.96" I2C) | Real-time sequence display            |
| 20 MHz Crystal           | Clock source                          |


---

## Pin Configuration

```
RD0 – RD3   →  ULN2003A IN1–IN4 (motor coils)
RB0 – RB3   →  Keypad columns (inputs, pull-up ON)
RB4 – RB7   →  Keypad rows (outputs)
RC3         →  OLED SCL (I2C clock)
RC4         →  OLED SDA (I2C data)
```

---

## Step Types


| Key | Type         | Description                              |
| --- | ------------ | ---------------------------------------- |
| 1   | Angle        | Rotate by degrees (1–3600°)              |
| 2   | Rotations    | Rotate by full rotations (1–99)          |
| 3   | Intermittent | Spin/idle cycles with configurable count |
| 4   | Timed        | Run for a fixed duration in seconds      |


---

## Features

- Build sequences of up to 5 steps, each with independent direction and RPM
- 4 step types — Angle, Rotations, Intermittent, Timed
- Set repeat count before running — runs the full sequence N times
- Save and reload last sequence with key `6`
- Abort any running step with `*`
- Half-step sequence for smooth 28BYJ-48 operation
- Timer1 interrupt-driven motor control — non-blocking execution
- Custom I2C bit-bang driver for SSD1306 OLED
- Compact font with digits + uppercase letters rendered on OLED

---

## Keypad Controls


| Key | Action                                              |
| --- | --------------------------------------------------- |
| 1–4 | Add step (Angle / Rotations / Intermittent / Timed) |
| #   | Run sequence (prompts for repeat count)             |
| *   | Clear sequence / abort running step                 |
| 6   | Load last saved sequence                            |


---

## Build and Flash

1. Open **MPLAB X IDE**
2. Create new project, select **PIC16F877A**
3. Add `stepper_sequence_runner.c` to project
4. Set **XC8** compiler with optimization level 1
5. Build and flash via **PICkit 3/4** or **TinyMultiBootLoader+**

> **Note:** Firmware is size-optimized to fit within bootloader flash constraints (~7680 words).

---

## Author

**Harie Goutaym D A**  
B.Tech ELC (IoT) — Amrita Vishwa Vidyapeetham  
[GitHub](https://github.com/HarieGoutaym) | [LinkedIn](https://linkedin.com/in/harie-goutaym-d-a-67722a36a)