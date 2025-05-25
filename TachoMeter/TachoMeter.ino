#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// -------------------- CONFIGURABLE CONSTANTS --------------------
#define PULSES_PER_REV 5             // <--- Change this to match your wheel's marker count!
#define CALIBRATION_REVS 3           // <--- Change this to set how many revolutions to use for calibration
#define SENSOR_PIN 2                 // TCRT5000 OUT to D2 (INT0)
#define LCD_ADDR 0x27                // I2C address for 16x2 LCD (change if needed)
#define EEPROM_ADDR 0                // EEPROM address for total revolutions
#define INACTIVITY_TIMEOUT 20000     // 20 seconds in ms
#define EEPROM_SAVE_INTERVAL 30000   // 30 seconds in ms
#define MAX_PULSE_INTERVAL 10000000  // 10 seconds in us (sanity check)
#define MOVING_AVG_SIZE 12           // Size of moving average buffer

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// -------------------- State Machine --------------------
enum State {
  WAITING_FOR_PULSES,
  CALIBRATING,
  RUNNING,
  INACTIVE
};
State state = WAITING_FOR_PULSES;

// -------------------- Variables --------------------
// Pulse timing
volatile unsigned long lastPulseTime = 0;
volatile unsigned long pulseInterval = 0;
volatile bool pulseDetected = false;

// Calibration
unsigned int calibPulseCount = 0;
unsigned long calibPulseTimes[CALIBRATION_REVS * PULSES_PER_REV];
unsigned long calibStartTime = 0;

// Moving average for RPM
unsigned long pulseIntervals[MOVING_AVG_SIZE];
unsigned int intervalIndex = 0;
unsigned int intervalsCollected = 0;
bool avgNeedsUpdate = false;

// Measurement
unsigned long totalPulses = 0;
unsigned long totalRevs = 0;
unsigned long lastSavedRevs = 0;
int rpm = 0;
int rph = 0;
int lastRpm = 0;
int lastRph = 0;

// Timing
unsigned long lastActivityTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastEEPROMSave = 0;

// Serial
String serialInput = "";

// -------------------- Function Prototypes --------------------
void fullSystemReset(bool showInit = true);
void startCalibration();
void processCalibrationPulse();
void finishCalibration();
void processRunningPulse();
void handleInactivity();
void updateDisplay();
void saveTotalRevsToEEPROM();
void loadTotalRevsFromEEPROM();
void clearEEPROM();
void showInitScreen();
void showPulseDetectedScreen();
void showCalibratedScreen();
void showNoRotationScreen();
void showReadingPulsesScreen();
void showCalibratingScreen();
void showMeasurementScreen();
void updateAveragedRPM();
void processSerialInput();
void pulseISR();
void printDebugState();

// -------------------- Setup --------------------
void setup() {
  lcd.init();
  lcd.backlight();

  Serial.begin(9600);
  Serial.println(F("===== Arduino Tachometer Boot ====="));
  Serial.print(F("Hardware: Arduino Nano + TCRT5000 + 16x2 I2C LCD | PULSES_PER_REV = "));
  Serial.print(PULSES_PER_REV);
  Serial.print(F(" | CALIBRATION_REVS = "));
  Serial.println(CALIBRATION_REVS);
  Serial.println(F("Ready for initialization..."));

  pinMode(SENSOR_PIN, INPUT_PULLUP);

  showInitScreen();

  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), pulseISR, FALLING);

  loadTotalRevsFromEEPROM();

  fullSystemReset(false);
}

// -------------------- Main Loop --------------------
void loop() {
  unsigned long now = millis();

  processSerialInput();

  if (state == RUNNING && (now - lastActivityTime > INACTIVITY_TIMEOUT)) {
    handleInactivity();
    return;
  }

  if (state == RUNNING && (now - lastEEPROMSave > EEPROM_SAVE_INTERVAL)) {
    saveTotalRevsToEEPROM();
    lastEEPROMSave = now;
    Serial.print(F("[EEPROM] Saved totalRevs: "));
    Serial.println(totalRevs);
  }

  if (pulseDetected) {
    pulseDetected = false;

    if (state == INACTIVE) {
      Serial.println(F("[EVENT] Pulse detected after inactivity. Restarting..."));
      showPulseDetectedScreen();
      delay(1000);
      fullSystemReset(false);
      return;
    }

    if (state == WAITING_FOR_PULSES) {
      Serial.println(F("[STATE] First pulse received. Entering calibration."));
      startCalibration();
      processCalibrationPulse();
      return;
    }

    if (state == CALIBRATING) {
      processCalibrationPulse();
      return;
    }

    if (state == RUNNING) {
      processRunningPulse();
      return;
    }
  }

  if (now - lastDisplayUpdate > 200) {
    updateDisplay();
    lastDisplayUpdate = now;
  }
}

// -------------------- ISR: Pulse Detection --------------------
void pulseISR() {
  unsigned long now = micros();
  if (lastPulseTime != 0) {
    unsigned long interval = now - lastPulseTime;
    if (interval > 1000 && interval < MAX_PULSE_INTERVAL) {
      pulseInterval = interval;
      pulseDetected = true;
    } else if (interval >= MAX_PULSE_INTERVAL) {
      Serial.print(F("[WARN] Outlier pulse interval ignored: "));
      Serial.println(interval);
    }
  } else {
    pulseDetected = true;
  }
  lastPulseTime = now;
}

// -------------------- System Reset --------------------
void fullSystemReset(bool showInit) {
  detachInterrupt(digitalPinToInterrupt(SENSOR_PIN));

  lastPulseTime = 0;
  pulseInterval = 0;
  calibPulseCount = 0;
  intervalsCollected = 0;
  intervalIndex = 0;
  totalPulses = 0;
  totalRevs = 0;
  rpm = 0;
  rph = 0;
  lastRpm = 0;
  lastRph = 0;
  avgNeedsUpdate = false;
  serialInput = "";
  for (unsigned int i = 0; i < MOVING_AVG_SIZE; i++) pulseIntervals[i] = 0;
  for (unsigned int i = 0; i < CALIBRATION_REVS * PULSES_PER_REV; i++) calibPulseTimes[i] = 0;

  clearEEPROM();
  loadTotalRevsFromEEPROM();

  state = WAITING_FOR_PULSES;
  lastActivityTime = millis();
  lastDisplayUpdate = 0;
  lastEEPROMSave = millis();

  if (showInit) showInitScreen();

  Serial.println(F("[RESET] System and EEPROM cleared. Waiting for pulses..."));

  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), pulseISR, FALLING);
}

// -------------------- Calibration --------------------
void startCalibration() {
  state = CALIBRATING;
  calibPulseCount = 0;
  calibStartTime = millis();
  for (unsigned int i = 0; i < CALIBRATION_REVS * PULSES_PER_REV; i++) calibPulseTimes[i] = 0;
  showReadingPulsesScreen();
  delay(500);
  showCalibratingScreen();
  Serial.print(F("[CALIBRATION] Started. Waiting for "));
  Serial.print(CALIBRATION_REVS);
  Serial.print(F(" full revolutions ("));
  Serial.print(CALIBRATION_REVS * PULSES_PER_REV);
  Serial.println(F(" pulses)..."));
}

void processCalibrationPulse() {
  calibPulseTimes[calibPulseCount] = micros();
  calibPulseCount++;
  lastActivityTime = millis();

  Serial.print(F("[CALIBRATION] Pulse "));
  Serial.print(calibPulseCount);
  Serial.print(F(" at "));
  Serial.print(calibPulseTimes[calibPulseCount - 1]);
  Serial.println(F(" us"));

  if (calibPulseCount >= CALIBRATION_REVS * PULSES_PER_REV) {
    finishCalibration();
  }
}

void finishCalibration() {
  state = RUNNING;
  showCalibratedScreen();
  delay(1000);
  lastActivityTime = millis();
  lastDisplayUpdate = 0;
  intervalsCollected = 0;
  intervalIndex = 0;
  for (unsigned int i = 0; i < MOVING_AVG_SIZE; i++) pulseIntervals[i] = 0;
  rpm = 0;
  rph = 0;
  lastRpm = 0;
  lastRph = 0;
  avgNeedsUpdate = false;
  // Add calibration revolutions to totalRevs
  totalRevs += CALIBRATION_REVS;
  saveTotalRevsToEEPROM();
  Serial.print(F("[CALIBRATION] Added "));
  Serial.print(CALIBRATION_REVS);
  Serial.println(F(" calibration revolutions to totalRevs."));
  Serial.println(F("[CALIBRATION] Complete. Entering RUNNING state."));
  printDebugState();
}

// -------------------- Running Measurement --------------------
void processRunningPulse() {
  lastActivityTime = millis();

  pulseIntervals[intervalIndex] = pulseInterval;
  intervalIndex = (intervalIndex + 1) % MOVING_AVG_SIZE;
  if (intervalsCollected < MOVING_AVG_SIZE) intervalsCollected++;

  totalPulses++;
  if (PULSES_PER_REV > 0 && (totalPulses % PULSES_PER_REV == 0)) {
    totalRevs++;
    avgNeedsUpdate = true;
    Serial.print(F("[MEASURE] Revolution completed. TotalRevs: "));
    Serial.println(totalRevs);
  }

  if (avgNeedsUpdate && (totalRevs % 2 == 0)) {
    updateAveragedRPM();
    avgNeedsUpdate = false;
  }
}

// -------------------- Inactivity Handling --------------------
void handleInactivity() {
  state = INACTIVE;
  rpm = 0;
  rph = 0;
  lastRpm = 0;
  lastRph = 0;
  intervalsCollected = 0;
  intervalIndex = 0;
  for (unsigned int i = 0; i < MOVING_AVG_SIZE; i++) pulseIntervals[i] = 0;
  showNoRotationScreen();
  Serial.println(F("[INACTIVE] No pulses for 20s. System inactive. Waiting for new pulses..."));
}

// -------------------- RPM/RPH Averaging --------------------
void updateAveragedRPM() {
  if (intervalsCollected == 0 || PULSES_PER_REV == 0) return;
  unsigned long sum = 0;
  for (unsigned int i = 0; i < intervalsCollected; i++) {
    sum += pulseIntervals[i];
  }
  unsigned long avgInterval = sum / intervalsCollected;
  if (avgInterval == 0) return;
  int newRpm = (int)(60000000UL / (avgInterval * PULSES_PER_REV));
  int newRph = newRpm * 60;

  if (newRpm != lastRpm || newRph != lastRph) {
    rpm = newRpm;
    rph = newRph;
    lastRpm = newRpm;
    lastRph = newRph;
    Serial.print(F("[MEASURE] RPM updated: "));
    Serial.print(rpm);
    Serial.print(F(" | RPH: "));
    Serial.println(rph);
  }
}

// -------------------- Display Functions --------------------
void updateDisplay() {
  switch (state) {
    case WAITING_FOR_PULSES:
      showInitScreen();
      break;
    case CALIBRATING:
      showCalibratingScreen();
      break;
    case RUNNING:
      showMeasurementScreen();
      break;
    case INACTIVE:
      showNoRotationScreen();
      break;
  }
}

void showInitScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tachometer");
  lcd.setCursor(0, 1);
  lcd.print("initialization...");
}

void showPulseDetectedScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pulse Detected");
  lcd.setCursor(0, 1);
  lcd.print("Restarting...");
}

void showReadingPulsesScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reading pulses...");
  lcd.setCursor(0, 1);
  lcd.print("calibrating...");
}

void showCalibratingScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reading pulses...");
  lcd.setCursor(0, 1);
  lcd.print("calibrating...");
}

void showCalibratedScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrated.");
  lcd.setCursor(0, 1);
  lcd.print(" ");
}

void showMeasurementScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RPM:");
  lcd.setCursor(4, 0);
  lcd.print(rpm);
  lcd.setCursor(8, 0);
  lcd.print("RPH:");
  lcd.setCursor(12, 0);
  lcd.print(rph);
  lcd.setCursor(0, 1);
  lcd.print("Total Revs:");
  lcd.setCursor(11, 1);
  lcd.print(totalRevs);
}

void showNoRotationScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("No Rotation.");
  lcd.setCursor(0, 1);
  lcd.print("Total Revs:");
  lcd.setCursor(11, 1);
  lcd.print(totalRevs);
}

// -------------------- EEPROM Functions --------------------
void saveTotalRevsToEEPROM() {
  for (int i = 0; i < 4; i++)
    EEPROM.write(EEPROM_ADDR + i, (totalRevs >> (8 * (3 - i))) & 0xFF);
}

void loadTotalRevsFromEEPROM() {
  unsigned long val = 0;
  for (int i = 0; i < 4; i++)
    val = (val << 8) | EEPROM.read(EEPROM_ADDR + i);
  totalRevs = val;
  Serial.print(F("[EEPROM] Loaded totalRevs: "));
  Serial.println(totalRevs);
}

void clearEEPROM() {
  for (int i = 0; i < 4; i++) EEPROM.write(EEPROM_ADDR + i, 0);
  Serial.println(F("[EEPROM] EEPROM cleared."));
}

// -------------------- Serial Input Handling --------------------
void processSerialInput() {
  while (Serial.available()) {
    char ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      serialInput.trim();
      if (serialInput.equalsIgnoreCase("reset") || serialInput.equalsIgnoreCase("r")) {
        Serial.println(F("[COMMAND] Reset command received via Serial."));
        fullSystemReset(true);
        delay(500);
      }
      serialInput = "";
    } else {
      serialInput += ch;
    }
  }
}

// -------------------- Debug Helper --------------------
void printDebugState() {
  Serial.print(F("[STATE] PULSES_PER_REV: "));
  Serial.println(PULSES_PER_REV);
  Serial.print(F("[STATE] CALIBRATION_REVS: "));
  Serial.println(CALIBRATION_REVS);
  Serial.print(F("[STATE] totalRevs: "));
  Serial.println(totalRevs);
  Serial.print(F("[STATE] RPM: "));
  Serial.print(rpm);
  Serial.print(F(" | RPH: "));
  Serial.println(rph);
}
