# Arduino Nano Advanced Tachometer V2

## Overview

This project is a streamlined, high-precision digital tachometer using an Arduino Nano, a TCRT5000 IR reflective sensor, and a 16x2 I2C LCD. It measures RPM (revolutions per minute), RPH (revolutions per hour), and total revolutions of a rotating object with black strips/markers. The system features smooth filtering, customizable debounce, and is designed for industrial or long-term monitoring applications.

## Features

- **User-Configurable Settings**: Easy-to-adjust constants at the top of the code for customization
- **Configurable Strips Per Revolution**: Set the number of markers/strips on your wheel for accurate measurement
- **Real-Time Smoothed Readings**: Adjustable smoothing factor for stable, noise-immune RPM readings
- **Floating-Point Precision**: High-precision decimal RPM & RPH values instead of integers
- **Debounce Protection**: Configurable debounce time to prevent false readings from metal vibration or reflection
- **Pulse Timeout**: Automatically zeros the display when rotation stops
- **Total Revolution Tracking**: Keeps count of total revolutions
- **Interrupt-Driven**: Uses hardware interrupts for precise timing and detection
- **Moving Average Filtering**: For stable, noise-immune RPM readings
- **Periodic Display Updates**: Configurable refresh rate for the LCD display
- **Comprehensive Serial Debugging**: All measurement updates are logged to Serial Monitor

## Library Requirements

This project uses the following Arduino libraries:

- Wire (built-in)
- LiquidCrystal_I2C (external)

## How to Install Libraries

1. **Wire**
   This library is included by default with the Arduino IDE. No installation is required.

2. **LiquidCrystal_I2C**
   **Option A: Using Arduino Library Manager**
   - Open the Arduino IDE
   - Go to Sketch → Include Library → Manage Libraries...
   - In the Library Manager, search for "LiquidCrystal I2C"
   - Find "LiquidCrystal I2C by Frank de Brabander" or "LiquidCrystal I2C by John Rickman" and click Install

   **Option B: Manual Installation**
   - Download the library ZIP from this [GitHub link](https://github.com/johnrickman/LiquidCrystal_I2C)
   - In the Arduino IDE, go to Sketch → Include Library → Add .ZIP Library...
   - Select the downloaded ZIP file and click Open

## Hardware Requirements

- Arduino Nano
- TCRT5000 IR reflective sensor (digital output)
- 16x2 I2C LCD (commonly 0x27 or 0x3F address)
- 5V DC power supply (stable)
- 22µF 50V electrolytic capacitor
- DC Jack input (for DC power supply to Arduino Nano)
- Perfboard, header pins, wires/jumpers
- Black reflective tape or paint (for wheel markers)
- Optional: Enclosure for protection

## Wiring

| Component        | Arduino Nano Pin | Notes               |
|------------------|------------------|---------------------|
| TCRT5000 OUT     | D2 (INT0)        | Digital output, interrupt |
| TCRT5000 VCC     | 5V               |                     |
| TCRT5000 GND     | GND              |                     |
| LCD SDA          | A4               | I2C data            |
| LCD SCL          | A5               | I2C clock           |
| LCD VCC          | 5V               |                     |
| LCD GND          | GND              |                     |
| Power Supply +5V | 5V pin           | Direct connection   |
| Power Supply GND | GND pin          | Direct connection   |
| Capacitor +      | 5V pin           | Parallel with power |
| Capacitor -      | GND pin          | Parallel with power |

**Note**: In this version, the 5V regulator has been removed. The DC power supply (5V) connects directly to the 5V and GND pins of the Arduino Nano. A 22µF 50V electrolytic capacitor is connected in parallel between 5V and GND for power filtering and stability.

## Power Supply Considerations

- This project requires a stable 5V DC power supply
- Connect the power supply directly to the 5V and GND pins of the Arduino Nano
- The 22µF electrolytic capacitor helps filter noise and stabilize the power supply
- **IMPORTANT**: Make sure your power supply outputs a clean 5V (±0.2V) to prevent damage to the Arduino
- **CAUTION**: When connecting an electrolytic capacitor, observe correct polarity (+ to 5V, - to GND)
- **WARNING**: Never connect USB power while also powering through the 5V pin to avoid damaging the Arduino

## Software Configuration

At the top of the code, you can adjust these constants as needed:

```cpp
// =================== USER-CONFIGURABLE SETTINGS ===================
const uint8_t STRIPS_PER_REVOLUTION = 4;       // 4 black strips
const unsigned long DEBOUNCE_MICROS = 250000;  // 250ms debounce for metal
const float RPM_SMOOTHING = 0.92;              // Heavy smoothing for stability
const unsigned long PULSE_TIMEOUT = 120000000; // 2 minutes (in µs)
const uint16_t DISPLAY_UPDATE_INTERVAL = 5000; // 5 seconds
// ===================================================================
```

### Configuration Parameters

- **STRIPS_PER_REVOLUTION**: Number of black strips/markers on your wheel
- **DEBOUNCE_MICROS**: Minimum time (in microseconds) between valid pulses to prevent bouncing
- **RPM_SMOOTHING**: Value between 0-1 determining how much smoothing to apply (higher = smoother)
- **PULSE_TIMEOUT**: Time (in microseconds) after which RPM is set to zero if no pulses are detected
- **DISPLAY_UPDATE_INTERVAL**: Time (in milliseconds) between LCD display updates

## How It Works

### Initialization

1. LCD initializes and displays "Initializing..."
2. IR sensor pin is configured with a pull-up resistor
3. Interrupt is attached to the falling edge of the IR sensor signal
4. LCD is cleared and initial display is updated with zeros

### Measurement

1. The IR sensor detects black strips/markers as they pass by
2. Each detection triggers an interrupt that calculates pulse intervals
3. The main loop calculates current RPM using a moving average of pulse intervals
4. RPH (revolutions per hour) is calculated by multiplying RPM by 60
5. Total revolutions are tracked by dividing total pulses by strips per revolution
6. The LCD display is updated periodically with the current values
7. Serial output provides debugging information

### Display

- **RPM**: Current revolutions per minute (with 3 decimal places)
- **RPH**: Current revolutions per hour (with 1 decimal place)
- **Total Revs**: Cumulative number of complete revolutions

## Serial Debugging

- All RPM/RPH updates and total revolution counts are logged to the Serial Monitor
- Use 115200 baud rate for Serial Monitor

## Customization

- Change `STRIPS_PER_REVOLUTION` to match your wheel's number of black strips or markers
- Adjust `RPM_SMOOTHING` to balance between responsive readings and stability
- Increase `DEBOUNCE_MICROS` if you get erratic readings due to vibrations or reflections
- Modify `PULSE_TIMEOUT` to change how quickly the display zeros when rotation stops
- Change `DISPLAY_UPDATE_INTERVAL` to update the LCD more or less frequently

## Example LCD Screens

```
Initializing...
```

```
RPM:0.000
RPH:0.0
```

```
RPM:60.125
RPH:3607.5
```

## Troubleshooting & Tips

- **No display on LCD**: Double-check I2C address (default is 0x27, but might be 0x3F)
- **Erratic RPM**: Increase `DEBOUNCE_MICROS` or improve the contrast of your markers
- **Always reading zero**: Check sensor alignment and ensure it's detecting the markers
- **Inaccurate readings**: Verify `STRIPS_PER_REVOLUTION` matches your actual marker count
- **Power issues**: Ensure your 5V supply is stable and the capacitor is correctly connected
- **Unstable readings**: Increase `RPM_SMOOTHING` value closer to 1.0 for more stability

## Differences from Previous Version

- Simplified code with no state machine or auto-calibration
- Removed EEPROM storage of total revolutions
- Direct 5V power connection instead of using voltage regulator
- Added power filtering capacitor for stability
- Floating-point precision for RPM and RPH values
- Configurable smoothing factor for stable readings
- Interrupt-driven pulse detection for higher accuracy
- Customizable display update interval

## License

This project is released under the MIT License.
Feel free to use and modify for your own industrial or hobby applications!

## Support

For questions, improvements, or troubleshooting, use the Serial Monitor output for debugging or open an issue in your project repository.
