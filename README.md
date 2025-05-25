# Arduino Nano Advanced Tachometer

## Overview

This project is a robust, configurable digital tachometer using an **Arduino Nano**, a **TCRT5000 IR reflective sensor**, and a **16x2 I2C LCD**. It measures the RPM (revolutions per minute), RPH (revolutions per hour), and total revolutions of a rotating wheel with black strips/markers. The system features auto-calibration, inactivity reset, EEPROM storage, and is designed for industrial or long-term use.

---

## Features

- **Configurable Pulses per Revolution:** Set the number of markers/strips on your wheel for accurate measurement.
- **Configurable Calibration Revolutions:** Set how many revolutions are used for auto-calibration.
- **Auto-Calibration:** Detects pulses for a set number of revolutions at startup or reset, and includes these revolutions in the total count.
- **Moving Average Filtering:** For stable, noise-immune RPM readings.
- **Integer RPM & RPH:** Only integer values are displayed.
- **Total Revolution Tracking:** Includes calibration revolutions in the count.
- **EEPROM Storage:** Saves total revolutions every 30 seconds for non-volatile storage.
- **Inactivity Reset:** If no pulses for 20 seconds, enters inactive state and waits for new pulses.
- **User & Hardware Reset:** Type `reset` or `r` in Serial Monitor or press Nano reset button to fully reset and recalibrate.
- **Clear LCD Status:** Displays calibration, measurement, inactivity, and reset messages.
- **Comprehensive Serial Debugging:** All state changes, calibration, resets, and measurement updates are logged to Serial Monitor.

---

## Hardware Requirements

- **Arduino Nano**
- **TCRT5000 IR reflective sensor** (digital output)
- **16x2 I2C LCD** (commonly 0x27 or 0x3F)
- **12V DC power supply**
- **LM7805 voltage regulator** (for stable 5V output)
- **Perfboard, header pins, wires, capacitors**
- **Black reflective tape or paint** (for wheel markers)
- **Optional:** Enclosure for protection

### Wiring

| Component      | Arduino Nano Pin | Notes                    |
| -------------- | ---------------- | ------------------------ |
| TCRT5000 OUT   | D2 (INT0)        | Digital output, interrupt|
| TCRT5000 VCC   | 5V               |                          |
| TCRT5000 GND   | GND              |                          |
| LCD SDA        | A4               | I2C data                 |
| LCD SCL        | A5               | I2C clock                |
| LCD VCC        | 5V               |                          |
| LCD GND        | GND              |                          |

Power the Nano’s VIN pin with regulated 5V from the LM7805, which is fed by your 12V DC supply.

---

## Software Configuration

At the top of the code, set these constants as needed:

define PULSES_PER_REV 4 // Number of black strips/markers on your wheel
define CALIBRATION_REVS 3 // Number of revolutions for calibration

**Example:**  
If your wheel has 6 strips and you want to calibrate for 2 revolutions:
define PULSES_PER_REV 6
define CALIBRATION_REVS 2

---

## How It Works

1. **Startup:**
    - LCD: `Tachometer` / `initialization...`
    - Waits for first pulse.

2. **Calibration:**
    - LCD: `Reading pulses...` / `calibrating...`
    - Collects pulses for `CALIBRATION_REVS` revolutions.
    - After calibration, adds these revolutions to total count.

3. **Measurement:**
    - LCD: `RPM: <value>  RPH: <value>` / `Total Revs: <value>`
    - RPM and RPH are updated only after every 2 revolutions if speed changes.
    - Total revolutions are incremented every full revolution (including calibration revolutions).

4. **Inactivity:**
    - If no pulses for 20 seconds:
        - LCD: `No Rotation.` / `Total Revs: <value>`
        - System waits for new pulses.
    - On new pulse:
        - LCD: `Pulse Detected` / `Restarting...`
        - System restarts calibration.

5. **Reset:**
    - Type `reset` or `r` in the Serial Monitor, or press the Nano’s reset button.
    - All values and EEPROM are cleared, and calibration restarts.

---

## Serial Debugging

- All state changes, calibration steps, RPM/RPH updates, EEPROM actions, and errors are logged to the Serial Monitor.
- Use 9600 baud for Serial Monitor.
- Type `reset` or `r` (and press Enter) to reset the system at any time.

---

## Code Structure

- **State Machine:** WAITING_FOR_PULSES, CALIBRATING, RUNNING, INACTIVE
- **Modular Functions:** For calibration, reset, inactivity, measurement, display, and EEPROM handling
- **Configurable Constants:** For easy adaptation to different wheels and calibration strategies
- **Well-Commented:** For maintainability and future upgrades

---

## Customization

- **Change `PULSES_PER_REV`** to match your wheel’s number of black strips or markers.
- **Change `CALIBRATION_REVS`** to set how many revolutions are used for calibration.
- **Adjust timeouts or moving average buffer size** as needed for your application.

---

## Example LCD Screens

Tachometer
initialization...

Reading pulses...
calibrating...

Calibrated.

RPM: 1234 RPH: 74040
Total Revs: 7

No Rotation.
Total Revs: 42

Pulse Detected
Restarting...
---

## Troubleshooting & Tips

- **No display on LCD:** Double-check I2C address and wiring.
- **Inaccurate RPM:** Confirm `PULSES_PER_REV` matches your actual marker count.
- **No revolution count:** Ensure the sensor detects every marker; adjust marker contrast or sensor distance if needed.
- **EEPROM not saving:** Check Nano wiring and power stability.
- **Serial Monitor not working:** Set baud rate to 9600 in the Arduino IDE.

---

## License

This project is released under the MIT License.  
Feel free to use and modify for your own industrial or hobby applications!

---

## Support

For questions, improvements, or troubleshooting, use the Serial Monitor output for debugging or open an issue in your project repository.

---

## PDF Export

- Use any Markdown-to-PDF tool (e.g., [Dillinger](https://dillinger.io/), VSCode Markdown PDF extension, or print from GitHub’s rendered Markdown view as PDF).

---