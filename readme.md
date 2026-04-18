# EMG-Controlled Tank Robot
### Lab Guide — NJIT Department of Biomedical Engineering
**By Erick Nunez**
**Wireless Muscle-Controlled Vehicle with ESP32 & Biopac MP36**

---

## System Overview

This project uses two ESP32 microcontrollers communicating over **ESP-NOW** — a peer-to-peer WiFi protocol that requires no router and has very low latency. One board acts as the **Transmitter** and the other as the **Receiver**.

```
Biopac MP36  →  LM324 Signal Conditioning  →  Transmitter ESP32
                  (both channels, one chip)          |
                                               ESP-NOW (wireless)
                                                     |
                                              Receiver ESP32
                                                     |
                                           DRV8833 Motor Driver
                                                     |
                                          Left Motor   Right Motor
```

**Transmitter:** Reads two analog channels from the Biopac MP36 analog output. Both signals pass through an LM324 quad op-amp conditioning circuit before reaching the ESP32 ADC pins. Channel 1 (speed) comes from a surface EMG electrode. Channel 2 (steering) comes from a hand dynamometer.

**Receiver:** Decodes incoming data packets and drives two DC motors in tank-drive configuration. The speed channel controls how fast both motors spin; the steering channel adjusts the balance between left and right to turn the robot.

> **This is your starting point — not your ending point.** The code is structured with clearly marked student sections. Once the base system works, you are encouraged to swap sensors, add features, change the control logic, or redesign the robot entirely.

---

## Required Components

### Core Hardware

| Qty | Item |
|-----|------|
| 2 | Seeed Studio XIAO ESP32-S3 (or ESP32-C3 as backup — see Section 3) |
| 1 | Biopac MP36 with BSL PRO software |
| 1 | EMG electrode leads (SS2LB or similar) |
| 1 | Hand dynamometer or any other Biopac-compatible sensor |
| 1 | DRV8833 dual motor driver module |
| 2 | Brushed DC motors (mounted on tank chassis) |
| 1 | Full-size breadboard |
| — | Jumper wires, USB-C cables |

### Signal Conditioning Circuit (Transmitter Breadboard)

One **LM324 quad op-amp** handles both conditioning channels. Only two of the four internal op-amps are used — the other two are available for expansion.

| Qty | Item | Purpose |
|-----|------|---------|
| 1 | LM324 quad op-amp (DIP-14) | Amplifier for both channels |
| 1 | 1N4007 Rectifier diode | Half-wave rectifier, one per channel |
| 1 | 100 kΩ resistor | Bleeder / discharge path, one per channel |
| 1 | 100 kΩ resistor | RC filter|
| 1 | 100 nF capacitor | RC filter |
| 2 | Ra resistor | Op-amp feedback Ra, one per channel |
| 2 | Rf resistor | Op-amp feedback Rf, one per channel |

### Power (Receiver + Motors)

| Item | Powers |
|------|--------|
| 3.7 V LiPo (or 4× AA holder) | Receiver ESP32 via the 5V/BAT pin |
| 9 V battery (or 4× AA holder) | Motor VM rail on DRV8833 |

> **Using 4× AA holders:** Four AA cells in series give ~6 V, which is enough for the motors.

---

## Section 1 — Biopac MP36 Setup

### 1.1 Channels and Sensors

This lab uses **two Biopac Boxes:**

| Channel | Default Sensor | Controls |
|---------|---------------|---------|
| Box 1 Channel 1 | EMG electrodes (SS2LB) | **Speed** — muscle activation drives the robot forward |
| Box 2 Channel 1 | Hand dynamometer | **Steering** — squeeze force steers left or right |

You are free to substitute the second sensor with a different Biopac-compatible sensor. Any sensor that produces a varying voltage output through the MP36 can be used as a control input — the conditioning circuit and code only care about the voltage that arrives at the ADC pin.

### 1.2 Electrode Placement (EMG Channel)

1. Clean the skin over the target muscle with an alcohol wipe and let it dry.
2. Place two active electrodes along the muscle belly, parallel to the muscle fibers, about 2 cm apart.
3. Place the reference electrode over a bony landmark (wrist, elbow, or knee) away from the target muscle.
4. Connect the leads to **Channel 1** of the MP36.

### 1.3 BSL PRO Configuration

1. Open BSL PRO and create a new recording template.
2. Configure **Box 1, Channel 1** as EMG (5-1000 Hz).
3. Configure **Box 2, Channel 1** for your chosen sensor (e.g., Hand Dynamometer preset).
4. Enable the analog output: go to **MP36 Menu → Output Control**. Assign **Channel 1** to the analog output port and enable the output.
5. **The analog output is only active while BSL PRO is actively recording.** Always start a recording session before testing the robot.

### 1.4 Analog Output Connector (DB-9, Rear Panel)

| DB-9 Pin | Signal | Notes |
|----------|--------|-------|
| Pin 2 | Analog output | Raw signal |
| Pin 3 | Ground | Connect to breadboard GND |

> ⚠️ **The MP36 analog output can reach ±5 V. The ESP32 ADC maximum input is 3.3 V.** Never connect the Biopac output directly to an ESP32 GPIO pin without the conditioning circuit in Section 2. Exceeding 3.3 V will permanently damage the microcontroller.

---

## Section 2 — Transmitter Signal Conditioning Circuit

The breadboard circuit has three jobs:

1. **Rectify** a diode is used so the signal stays positive
2. **Low pass filter** RC low pass filter is used to create an envelope of the signal
3. **Scale** an op-amp so the signal can fill the full 0–3.3 V ADC range

### 2.1 The LM324 Quad Op-Amp

The LM324 contains **four independent op-amps in a single DIP-14 package.** This lab uses two of them — one per channel — so you only need one chip for the entire transmitter. Op amps are used in a non-inverting configuration.

**Key properties:**
- Single-supply operation from 3 V to 32 V
- Input common-mode range extends down to GND — works correctly with signals near 0 V
- Output is **not** rail-to-rail: on a 5 V supply it tops out around 3.5 V, which is within safe range for the ESP32 ADC

**Power the LM324 from the ESP32 5V (VBUS) pin, not the 3.3 V pin.** On 3.3 V the output ceiling drops to about 1.8 V, cutting the usable ADC range roughly in half.

**LM324 DIP-14 pin assignments used in this circuit:**

| Pin | Name | Connection |
|-----|------|------------|
| 1 | OUT1 | Ch. 1 output → ESP32 A0 |
| 2 | IN1− | Ch. 1 feedback network |
| 3 | IN1+ | Ch. 1 (Speed) signal input |
| 4 | V+ | ESP32 5V (VBUS) |
| 11 | GND | GND rail |
| 12 | IN4+ | Ch. 2 (Steering) signal input |
| 13 | IN4− | Ch. 2 feedback network |
| 14 | OUT4 | Ch. 2 output → ESP32 A1 |
| 5–10 (remaining) | OUT2/3, IN2/3 | Unused |

### 2.2 Per-Channel Circuit

Build this circuit twice using two op-amp sections of the same LM324.

```
Biopac DB-9 Pin 2 (EMG Channel)
        |
    [Diode]    <- stripe (cathode) faces toward op-amp
        |
    +---+-------------------+
 100 kΩ            [RC LowPass Resistor]
(to GND)                    |
                  [RC LowPass Capacitor]  <- to GND
                            |
                  LM324 IN+  (non-inverting)
                  LM324 IN-  -- Ra Resistor -- GND
                            +- Rf Resistor -- LM324 OUT
                                                  |
                                            ESP32 A0 or A1
```

**Why the 100 kΩ bleeder resistor matters:** Without it, the capacitor has no discharge path. The voltage pumps up during muscle activity and does not return to 0 V at rest, so the ADC reads a stuck high value even when the muscle is relaxed. The bleeder provides that discharge path, ensuring a clean 0 V baseline.

**Gain:** G = 1 + Rf / Ra 

At rest → op-amp output ≈ 0 V → ADC reads ~0.  
Strong contraction → op-amp output ≈ 3.0 V → ADC reads ~3700 (Max: 4095).

## Section 3 — Arduino IDE Setup

### 3.1 Install the ESP32 Board Package

1. Open Arduino IDE. Go to **File → Preferences**.
2. In the *Additional Boards Manager URLs* field, paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools → Board → Boards Manager**. Search for `esp32` and install the **Espressif Systems** package.
4. Connect your board via USB-C and select the correct COM port under **Tools → Port**.

### 3.2 Board Selection

| Board | Arduino IDE Selection | Notes |
|-------|-----------------------|-------|
| XIAO ESP32-S3 | `XIAO_ESP32S3` | Primary board for this lab |
| XIAO ESP32-C3 | `XIAO_ESP32C3` | Backup — same code, same pin names, no changes needed |

Both boards support ESP-NOW and all features used in this lab. If you receive an ESP32-C3, simply select `XIAO_ESP32C3` in the board menu. The pin names A0, A1, A2, A3 map correctly on both boards.

> **ESP32-C3 note:** On the C3, some ADC channels are disabled when WiFi is active. This does not affect this project — the Espressif board package routes A0 and A1 to ADC1 channels that remain available during ESP-NOW operation.

### 3.3 Reference Links

- XIAO ESP32-S3 Wiki: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
- DRV8833 Motor Driver Tutorial: https://lastminuteengineers.com/drv8833-arduino-tutorial/
- Biopac MP36 Manual: https://www.biopac.com/wp-content/uploads/MP36-MP45.pdf

---

## Section 4 — Receiver Wiring (DRV8833 Motor Driver)

The DRV8833 is a dual H-bridge motor driver. Each motor uses two control pins: one for speed (PWM) and one for direction. In the default code both motors always run forward.

### 4.1 ESP32 to DRV8833 Connections

| ESP32 Pin | DRV8833 Pin | Function | Code Variable |
|-----------|------------|----------|--------------|
| A0 | IN1 | Left motor speed (PWM) | `leftMotorPin1` |
| A1 | IN2 | Left motor direction | `leftMotorPin2` |
| A2 | IN3 | Right motor speed (PWM) | `rightMotorPin1` |
| A3 | IN4 | Right motor direction | `rightMotorPin2` |

### 4.2 Power Connections

| DRV8833 Pin | Connect to | Notes |
|-------------|------------|-------|
| VM | 9 V battery + (or 4× AA) | Motor power rail |
| GND | Battery − **and** ESP32 GND | All grounds must share a common rail |

> ⚠️ **All grounds must be tied together** — the LiPo −, the motor battery −, and the ESP32 GND pin must all connect to the same rail. Floating grounds cause unpredictable motor behavior.

### 4.3 Motor Connections

Connect the left DC motor to **OUT1** and **OUT2** on the DRV8833. Connect the right motor to **OUT3** and **OUT4**. If either motor spins in the wrong direction, swap its two wires at the output terminals or switch which Pin to write the PWM to.

---

## Section 5 — Code Walkthrough

The code is split into two files: **EMGCarTransmitter.ino** and **EMGCarReceiver.ino**. Both use a consistent labeling convention:

- `// %%%%%` sections — communication framework; do not modify unless you have a specific reason
- `// =====` sections — your space for variables, setup code, and control logic

### 5.1 Step 1: Flash the Receiver First

The transmitter needs the receiver's MAC address before it can send packets.

1. Upload **EMGCarReceiver.ino** to one ESP32.
2. Open **Serial Monitor** at **115200 baud** and press the reset button.
3. Copy the address printed on this line:
   ```
   >>> RECEIVER MAC ADDRESS (copy into transmitter code): AA:BB:CC:DD:EE:FF
   ```

### 5.2 Step 2: Enter the MAC Address in the Transmitter

Open **EMGCarTransmitter.ino** and replace the placeholder:

```cpp
uint8_t receiverMAC[] = {0x1C, 0xDB, 0xD4, 0x78, 0xA6, 0xB0}; // <- REPLACE THIS
```

Convert each hex pair from the Serial Monitor printout. For example, `AA:BB:CC:DD:EE:FF` becomes:

```cpp
uint8_t receiverMAC[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
```

Upload the updated transmitter sketch. Both boards are now paired.

### 5.3 How the Transmitter Works

The transmitter reads both ADC pins and sends a packet every 250 ms:

```cpp
void loop() {
  sensorVal1 = analogRead(biopac1Pin);  // Speed channel    -> A0
  sensorVal2 = analogRead(biopac2Pin);  // Steering channel -> A1

  packet.SensorVal1 = constrain(sensorVal1, 0, 4095);
  packet.SensorVal2 = constrain(sensorVal2, 0, 4095);

  esp_now_send(receiverMAC, (uint8_t *)&packet, sizeof(packet));
  delay(250);
}
```

Both values are 12-bit ADC readings: 0 = 0 V at the pin, 4095 = 3.3 V at the pin.

### 5.4 How the Receiver Works

The receiver decodes each packet and converts sensor values into left and right motor PWM outputs:

```cpp
// 1. Read the latest packet
sensorValue1 = latestPacket.SensorVal1;  // speed
sensorValue2 = latestPacket.SensorVal2;  // steering

// 2. Exponential smoothing filter
speedValue = round(sensorValue1 * alpha + lastSpeed * (1.0 - alpha));
lastSpeed = speedValue;

// 3. Map steering: center (2048) = straight, above = left, below = right
steering = map(sensorValue2, 0, 4095, -2047, 2047);

// 4. Tank drive mixing
leftMotorSpeed  = speedValue - steering - EMGcap;
rightMotorSpeed = speedValue + steering - EMGcap;

// 5. Scale to 0-255 PWM and clamp
leftMotorSpeed  = constrain(map(leftMotorSpeed,  2047-EMGcap, 6142-EMGcap, 0, 255), 0, 255);
rightMotorSpeed = constrain(map(rightMotorSpeed, 2047-EMGcap, 6142-EMGcap, 0, 255), 0, 255);

// 6. Write to motor driver
analogWrite(leftMotorPin1,  leftMotorSpeed);
digitalWrite(leftMotorPin2, LOW);
analogWrite(rightMotorPin1, rightMotorSpeed);
digitalWrite(rightMotorPin2, LOW);
```

### 5.5 Tuning Variables

| Variable | File | Default | What It Does |
|----------|------|---------|--------------|
| `EMGcap` | Receiver | `2000` | **Activation threshold.** Only ADC values above this drive the motors. Raise it if the robot creeps at rest; lower it if it never responds. |
| `alpha` | Receiver | `0.75` | **Smoothing factor** (0–1). Higher = faster, more jitter. Lower = smoother, more lag. |
| `delay(250)` | Transmitter | `250` ms | **Packet rate** (~4 Hz). Reduce to `50` ms for snappier response. |

---

## Section 6 — Testing and Debugging

### 6.1 Pre-Power Checklist

- [ ] All grounds tied together: LiPo −, motor battery −, ESP32 GND, LM324 pin 11, Biopac DB-9 Pin 3
- [ ] LM324 pin 4 (V+) connected to ESP32 **5V (VBUS)**, not 3.3V
- [ ] Diode oriented correctly (stripe = cathode, pointing toward op-amp)
- [ ] 100 kΩ bleeder resistors connected from post-diode node to GND
- [ ] BSL PRO is recording and Channel 1 is assigned to the analog output port
- [ ] MAC address in transmitter code matches the receiver's printed MAC

### 6.2 Serial Monitor Checks

Open Serial Monitor at **115200 baud** on each board:

**Transmitter:**
```
Sensor1: 245, Sensor2: 1850
    Sent
```
- Values near 0–400 at rest with muscles relaxed = good signal conditioning
- `Sent` = packet delivered successfully
- `FAILED` = MAC address mismatch or receiver not powered

**Receiver:**
```
Left: 0, Right: 0               <- resting, below EMGcap threshold (expected)
Left: 180, Right: 180           <- both motors driving forward
Left: 80, Right: 180            <- differential drive, turning right
Signal lost - motors stopped    <- no packet received for 5 seconds
```

### 6.3 Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| ADC always reads 0 | No signal from Biopac | Verify BSL PRO is recording; Analog output is assigned to Channel 1; Enable output |
| ADC stuck high at rest | Missing bleeder, or LM324 on wrong supply | Confirm 100 kΩ bleeder to GND on each channel; confirm LM324 V+ is on 5V (VBUS) |
| `FAILED` on transmitter | MAC address mismatch | Re-flash receiver, copy MAC, update and re-flash transmitter |
| Motors don't move | `EMGcap` too high | Lower `EMGcap`; confirm values are arriving on receiver Serial Monitor |
| Motors run at rest | `EMGcap` too low | Raise `EMGcap` until motors stop when muscles are relaxed |
| Steering has no effect | Ch. 2 not reaching A1, or dynamometer inactive | Check second channel breadboard wiring; Verify dynamometer is connected in BSL PRO; Verify output is enabled |
| LM324 output too low | LM324 powered from 3.3V | Move LM324 V+ to the ESP32 5V (VBUS) pin |

---

## Section 7 — Customization and Extensions

The base system is a working platform. Here are directions to take it further.

### Swap the Sensors

The conditioning circuit and code are sensor-agnostic — they process whatever voltage arrives at the ADC pin. You can use any MP36-compatible sensor for either channel:
- Replace the hand dynamometer with a **second EMG channel** for more nuanced steering control
- Use an oscilloscope to measure the signal and fine tune your circuit before pulling into ESP32
- The LM324 has **two unused op-amp sections** (pins 8–14) — use them to condition a third or fourth channel if you want more inputs

### Change the Control Logic

- **Add reverse:** When a threshold is crossed on a third signal, set `leftMotorPin2` HIGH and write 0 to `leftMotorPin1` to reverse both motors
- **Proportional steering sensitivity:** Adjust the `map()` range on the steering line to make turns sharper or more gradual
- **Tank drive:** Control each side independently, trying using two EMG sensors

### Modify the Electronics

- **Adjustable gain:** Replace Rf with a potentiometer to make the gain tunable from the breadboard
- **LED feedback:** Drive the onboard LED with `analogWrite` scaled to `speedValue` as a visual indicator of motor output

### Extend the Wireless System

- **Lower latency:** Reduce `delay(250)` in the transmitter to `delay(50)` for a much snappier response 
> ⚠️ **Caution** — As the delay gets shorter the temperture of the ESP32 will go higher and can be hot to the touch.

---

## Quick Reference

### Transmitter Pin Map

| ESP32 Pin | Connected To |
|-----------|-------------|
| A0 | LM324 OUT1 — Speed (Box 1, Ch. 1) |
| A1 | LM324 OUT2 — Steering (Box 2, Ch. 1) |
| GND | Biopac DB-9 Pin 3, LM324 pin 11, GND rail |
| 5V (VBUS) | LM324 pin 4 (V+) |

### Receiver Pin Map

| ESP32 Pin | Connected To |
|-----------|-------------|
| A0 | DRV8833 IN1 (left motor PWM) |
| A1 | DRV8833 IN2 (left motor direction) |
| A2 | DRV8833 IN3 (right motor PWM) |
| A3 | DRV8833 IN4 (right motor direction) |
| GND | DRV8833 GND, motor battery − |
| 5V / BAT | LiPo + or 4× AA + |

### LM324 Power and Decoupling

| LM324 Pin | Connect to | Why |
|-----------|-----------|-----|
| 4 (V+) | ESP32 5V (VBUS) | Gives output swing up to ~3.5 V; on 3.3 V the ceiling drops to ~1.8 V |
| 11 (GND) | GND rail | |

### Board Comparison

| | ESP32-S3 | ESP32-C3 (backup) |
|---|---|---|
| Board Manager selection | `XIAO_ESP32S3` | `XIAO_ESP32C3` |
| ESP-NOW support | Yes | Yes |
| Code changes required | None | None |
| A0 / A1 GPIO | GPIO1 / GPIO2 | GPIO2 / GPIO3 |

### Packet Structure (must match in both files)

```cpp
struct ControlPacket {
  uint16_t SensorVal1;   // Speed:    0 -> 4095
  uint16_t SensorVal2;   // Steering: 0 -> 4095
};
```

If you add sensor channels, add new fields to this struct in **both** `.ino` files and re-flash both boards before testing.

---