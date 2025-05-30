#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// =================== USER-CONFIGURABLE SETTINGS ===================
const uint8_t STRIPS_PER_REVOLUTION = 4;       // 4 black strips
const unsigned long DEBOUNCE_MICROS = 250000;  // 250ms debounce for metal
const float RPM_SMOOTHING = 0.92;              // Heavy smoothing for stability
const unsigned long PULSE_TIMEOUT = 120000000; // 2 minutes (in µs)
const uint16_t DISPLAY_UPDATE_INTERVAL = 5000; // 5 seconds
// ===================================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);  // 16x2 LCD
#define IR_SENSOR_PIN 2              // Interrupt-capable pin

// ====================== GLOBAL STATE VARIABLES ==================
volatile uint32_t lastPulseMicros = 0;
volatile uint32_t pulseIntervals[4] = {0}; // Reduced buffer size for low RPM
volatile uint8_t pulseIndex = 0;
volatile uint32_t totalPulses = 0;
volatile bool newData = false;

float smoothedRPM = 0.0;
uint32_t lastDisplayUpdate = 0;
uint32_t totalRevs = 0; // Now volatile across resets

void pulseISR() {
  static uint32_t lastValidMicros = 0;
  uint32_t currentMicros = micros();
  
  if (currentMicros - lastValidMicros > DEBOUNCE_MICROS) {
    uint32_t interval = currentMicros - lastPulseMicros;
    
    pulseIntervals[pulseIndex] = interval;
    pulseIndex = (pulseIndex + 1) % 4;
    
    totalPulses++;
    lastPulseMicros = currentMicros;
    lastValidMicros = currentMicros;
    newData = true;
  }
}

float calculateRPM() {
  uint32_t sum = 0;
  uint8_t validCount = 0;
  
  for(uint8_t i=0; i<4; i++) {
    if(pulseIntervals[i] > 0 && pulseIntervals[i] < PULSE_TIMEOUT) {
      sum += pulseIntervals[i];
      validCount++;
    }
  }
  
  if(validCount == 0) return 0.0;
  
  float avgInterval = sum / (float)validCount;
  float instantRPM = 15000000.0 / avgInterval; // 60e6 / (4 * avgInterval)
  
  if(smoothedRPM == 0.0) {
    smoothedRPM = instantRPM;
  } else {
    smoothedRPM = (RPM_SMOOTHING * smoothedRPM) + 
                 ((1.0 - RPM_SMOOTHING) * instantRPM);
  }
  
  if(micros() - lastPulseMicros > PULSE_TIMEOUT) {
    smoothedRPM = 0.0;
    memset((void*)pulseIntervals, 0, sizeof(pulseIntervals));
  }
  
  return smoothedRPM;
}

void updateDisplay(float rpm, float rph) {
  lcd.clear();
  
  // RPM Line
  lcd.setCursor(0,0);
  lcd.print("RPM:");
  lcd.print(rpm, 3);
  
  // RPH Line
  lcd.setCursor(0,1);
  lcd.print("RPH:");
  lcd.print(rph, 1);
  
  // Revolutions Line
  lcd.setCursor(0,2);
  lcd.print("Total Revs:");
  lcd.print(totalRevs);
}

void setup() {
  // Initialize serial
  Serial.begin(115200);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Initializing...");
  
  // Configure IR sensor
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), pulseISR, FALLING);
  
  // Initial display
  lcd.clear();
  updateDisplay(0.0, 0.0);
}

void loop() {
  static float lastRPM = -1;
  
  // Calculate current values
  float currentRPM = calculateRPM();
  float currentRPH = currentRPM * 60.0;
  totalRevs = totalPulses / STRIPS_PER_REVOLUTION;
  
  // Update display periodically
  if(millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    updateDisplay(currentRPM, currentRPH);
    lastDisplayUpdate = millis();
  }
  
  // Serial output for debugging
  if(millis() - lastDisplayUpdate >= 1000) {
    Serial.print("RPM: ");
    Serial.print(currentRPM, 3);
    Serial.print(" | RPH: ");
    Serial.print(currentRPH, 1);
    Serial.print(" | Total Revs: ");
    Serial.println(totalRevs);
  }
}
