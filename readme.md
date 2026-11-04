# EMG-Controlled Tank Robot
**NJIT Department of Biomedical Engineering — Erick Nunez**

A wireless, muscle-controlled tank robot. Surface EMG and a second physiological sensor, recorded by a Biopac MP36, are conditioned on a breadboard, sampled by an ESP32, and sent over ESP-NOW to a second ESP32 that drives two DC motors.

> **Students:** the full step-by-step lab guide (setup, wiring, testing, troubleshooting) is in the PDF. This README is a project overview.

---

## How It Works

```
Biopac MP36 ──► LM324 conditioning ──► Transmitter ESP32
 (2 channels)   (rectify, filter, scale)        │
                                           ESP-NOW (wireless)
                                                │
                              Receiver ESP32 ──► DRV8833 ──► Left / Right motors
```

| Signal | Default sensor | Controls |
|--------|----------------|----------|
| Channel 1 | Surface EMG (SS2LB) | Speed of both motors |
| Channel 2 | Hand dynamometer | Steering (left/right balance) |

Either sensor can be swapped for any MP36-compatible sensor; the circuit and code only care about the voltage at the ADC pin.

---

## Hardware

- 2× Seeed XIAO ESP32 (S3 primary; C3 or C6 also work)
- Biopac MP36 with BSL PRO, EMG leads, hand dynamometer (or other sensor)
- LM324 quad op-amp (two of four sections used)
- Per channel: 1N4007 diode, 100 kΩ bleeder, RC low-pass (100 kΩ + 100 nF), gain resistors Ra / Rf
- DRV8833 dual motor driver, 2 brushed DC motors on a tank chassis
- Power: 3.7 V LiPo (or 4× AA) for the receiver ESP32; 9 V (or 4× AA) for the motor rail

---

## Signal Conditioning

Each channel goes through the same chain before the ESP32 ADC:

1. **Half-wave rectifier** (diode) keeps the signal positive
2. **100 kΩ bleeder** gives the filter capacitor a discharge path so the baseline returns to 0 V
3. **RC low-pass** produces a signal envelope
4. **Non-inverting op-amp**, gain G = 1 + Rf/Ra, scales the signal to fill the 0–3.3 V ADC range

The LM324 is powered from the ESP32 **5 V (VBUS)** pin. Its output tops out around 3.5 V on 5 V, which keeps the ADC safe; on 3.3 V the ceiling drops to about 1.8 V.

> ⚠️ The MP36 analog output can swing ±5 V. Never connect it directly to an ESP32 pin.

---

## Pin Maps

**Transmitter**

| ESP32 | Connection |
|-------|-----------|
| A0 | LM324 OUT1 (pin 1) — speed |
| A1 | LM324 OUT4 (pin 14) — steering |
| 5V (VBUS) | LM324 V+ (pin 4) |
| GND | LM324 GND (pin 11), Biopac DB-9 pin 3 |

Biopac analog output: DB-9 pin 2 (signal), pin 3 (ground).

**Receiver**

| ESP32 | DRV8833 | Function |
|-------|---------|----------|
| A0 | IN1 | Left motor PWM |
| A1 | IN2 | Left motor direction |
| A2 | IN3 | Right motor PWM |
| A3 | IN4 | Right motor direction |

Motors connect to OUT1/OUT2 (left) and OUT3/OUT4 (right). All grounds (batteries, ESP32, DRV8833) share one rail.

---

## Firmware

Two Arduino sketches, built with the Espressif ESP32 board package:

- **`EMGCarTransmitter.ino`** reads both ADC channels, applies exponential smoothing to the EMG, and sends a packet every 250 ms.
- **`EMGCarReceiver.ino`** decodes packets, mixes speed and steering into left/right PWM (tank drive), and stops the motors if no packet arrives for 5 seconds.

Code is marked into two zones:
- `// %%%%%` — communication framework (leave as is)
- `// =====` — student space for variables and control logic

**Pairing:** flash the receiver first, copy the MAC address it prints at 115200 baud into `receiverMAC[]` in the transmitter, then flash the transmitter.

**Packet (must match in both files):**
```cpp
struct ControlPacket {
  uint16_t SensorVal1;   // Speed:    0–4095
  uint16_t SensorVal2;   // Steering: 0–4095
};
```

**Main tuning knobs**

| Variable | File | Default | Effect |
|----------|------|---------|--------|
| `alpha` | Transmitter | 0.5 | EMG smoothing (higher = faster, noisier) |
| `delay(250)` | Transmitter | 250 ms | Packet rate; lower is snappier but heats the ESP32 |
| `maxSteeringValue` | Receiver | 100 | Steering sensitivity |
| `EMGcap` | Receiver | — | Rest threshold below which motors stay off |

---

## Board Notes

| Board | Arduino selection | Notes |
|-------|-------------------|-------|
| XIAO ESP32-S3 | `XIAO_ESP32S3` | Primary board |
| XIAO ESP32-C3 | `XIAO_ESP32C3` | No built-in LED, no A3 |
| XIAO ESP32-C6 | `XIAO_ESP32C6` | No A3 |

No code changes are needed between boards for A0–A2. The receiver uses A3, so plan alternate pin assignments if it runs on a C3 or C6.

---

## Extension Ideas

- Replace the dynamometer with a second EMG channel for independent left/right control
- Add reverse using a third signal and the direction pins
- Use the two spare LM324 sections for extra channels
- Swap Rf for a potentiometer to make gain adjustable
- Drive the onboard LED from motor output as visual feedback

---

## References

- [XIAO ESP32-S3 Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [XIAO ESP32-C3 Wiki](https://wiki.seeedstudio.com/xiao_esp32c3_getting_started/)
- [XIAO ESP32-C6 Wiki](https://wiki.seeedstudio.com/xiao_esp32c6_getting_started/)
- [DRV8833 Tutorial](https://lastminuteengineers.com/drv8833-arduino-tutorial/)
- [Biopac MP36 Manual](https://www.biopac.com/wp-content/uploads/MP36-MP45.pdf)