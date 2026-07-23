
//  Solar Tracker + DHT11 Temperature & Humidity Display
//  Hardware: Arduino Uno/Nano R3
//  LCD: 16x2 with I2C backpack (PCF8574) — SDA=A4, SCL=A5
//  Servo: D9
//  Sensor: DHT11 on A2
//  LDR Left: A1  |  LDR Right: A0

// ---------- Libraries ----------
#include <Wire.h>               // I2C (built-in)
#include <LiquidCrystal_I2C.h> // LCD I2C
#include <Servo.h>              // Servo
#include <DHT.h>                // DHT11/DHT22 driver

// ============================================================
//  PIN DEFINITIONS
// ============================================================
#define PIN_SERVO        9      // Servo PWM signal
#define PIN_DHT          A2     // DHT11 data line
#define PIN_LDR_LEFT     A1     // Left  LDR
#define PIN_LDR_RIGHT    A0     // Right LDR

// DHT sensor type — change to DHT22 if you upgrade later
#define DHT_TYPE         DHT11

// ============================================================
//  TUNABLE PARAMETERS
// ============================================================
#define SERVO_MIN_ANGLE    0    // Servo left  limit (°)
#define SERVO_MAX_ANGLE    180  // Servo right limit (°)
#define SERVO_START_ANGLE  90  // Boot position (centre)

#define LDR_THRESHOLD      15  // Dead-band to prevent jitter
#define SERVO_STEP         2   // Degrees per move
#define SERVO_SPEED_MS     20  // ms between servo steps

// DHT11 needs at least 2000ms between reads
#define DHT_READ_INTERVAL  2000
#define LCD_UPDATE_MS      600

// ============================================================
//  OBJECT INSTANCES
// ============================================================

// Try 0x27 first; if LCD blank, change to 0x3F
LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo trackServo;

DHT dht(PIN_DHT, DHT_TYPE);

// ============================================================
//  GLOBAL STATE
// ============================================================
int   servoAngle  = SERVO_START_ANGLE;
float tempC       = 0.0;
float humidity    = 0.0;
bool  sensorOK    = false;

unsigned long lastDHTTime   = 0;
unsigned long lastLcdTime   = 0;
unsigned long lastServoTime = 0;

// Custom LCD characters
byte degreeChar[8] = {      // ° symbol
  0b00110, 0b01001, 0b01001,
  0b00110, 0b00000, 0b00000,
  0b00000, 0b00000
};

byte tempIcon[8] = {        // small thermometer icon
  0b00100, 0b01010, 0b01010,
  0b01110, 0b01110, 0b11111,
  0b11111, 0b01110
};

byte dropIcon[8] = {        // water drop icon for humidity
  0b00100, 0b00100, 0b01110,
  0b01110, 0b11111, 0b11111,
  0b11111, 0b01110
};

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(9600);
  Serial.println(F("=== Solar Tracker (DHT11) Boot ==="));

  // --- LCD init ---
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, degreeChar);  // slot 0 = °
  lcd.createChar(1, tempIcon);    // slot 1 = thermometer
  lcd.createChar(2, dropIcon);    // slot 2 = drop

  // Boot splash
  lcd.setCursor(0, 0);
  lcd.print(F(" Solar Tracker  "));
  lcd.setCursor(0, 1);
  lcd.print(F("  with DHT11    "));
  delay(1500);
  lcd.clear();

  // --- Servo init ---
  trackServo.attach(PIN_SERVO);
  trackServo.write(SERVO_START_ANGLE);
  Serial.print(F("Servo centred at "));
  Serial.print(SERVO_START_ANGLE);
  Serial.println(F("°"));

  // --- DHT11 init ---
  dht.begin();
  delay(2000);  // DHT11 needs 2s after power-on before first read

  // First sensor read
  readDHT();
  lastDHTTime = millis();

  Serial.println(F("Setup complete."));
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  unsigned long now = millis();

  // ── 1. Read LDRs ─────────────────────────────────────────
  int ldrLeft  = analogRead(PIN_LDR_LEFT);
  int ldrRight = analogRead(PIN_LDR_RIGHT);
  int ldrDiff  = ldrLeft - ldrRight;   // +ve → left brighter

  // ── 2. Move servo based on light imbalance ────────────────
  if (now - lastServoTime >= SERVO_SPEED_MS) {
    lastServoTime = now;

    if (ldrDiff > LDR_THRESHOLD) {
      servoAngle -= SERVO_STEP;          // track left
    } else if (ldrDiff < -LDR_THRESHOLD) {
      servoAngle += SERVO_STEP;          // track right
    }

    servoAngle = constrain(servoAngle, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
    trackServo.write(servoAngle);
  }

  // ── 3. Read DHT11 (max once per 2 s) ─────────────────────
  if (now - lastDHTTime >= DHT_READ_INTERVAL) {
    lastDHTTime = now;
    readDHT();
  }

  // ── 4. Update LCD (every 600 ms) ─────────────────────────
  if (now - lastLcdTime >= LCD_UPDATE_MS) {
    lastLcdTime = now;
    updateLCD(ldrLeft, ldrRight, ldrDiff);
  }
}

// ============================================================
//  READ DHT11 SENSOR
// ============================================================
void readDHT() {
  float newTemp = dht.readTemperature();   // Celsius
  float newHum  = dht.readHumidity();

  // isnan() catches failed reads
  if (isnan(newTemp) || isnan(newHum)) {
    sensorOK = false;
    Serial.println(F("DHT11 read FAILED — check wiring & pull-up!"));
  } else {
    tempC    = newTemp;
    humidity = newHum;
    sensorOK = true;
    Serial.print(F("Temp: ")); Serial.print(tempC, 1);
    Serial.print(F("C  Humidity: ")); Serial.print(humidity, 1);
    Serial.println(F("%"));
  }
}

// ============================================================
//  LCD UPDATE
// ============================================================
//
//  Layout when sensor OK:
//  ┌────────────────┐
//  │⊞ 28.5°C  H:65%│   row 0: temp + humidity
//  │Ang:090  =OK=   │   row 1: servo angle + direction
//  └────────────────┘
//
//  Layout on sensor error:
//  ┌────────────────┐
//  │ DHT11 ERROR!   │
//  │Check wiring    │
//  └────────────────┘
// ============================================================
void updateLCD(int ldrL, int ldrR, int diff) {

  // ── Row 0 ─────────────────────────────────────────────────
  lcd.setCursor(0, 0);

  if (!sensorOK) {
    lcd.print(F(" DHT11 ERROR!   "));
    lcd.setCursor(0, 1);
    lcd.print(F(" Check wiring   "));
    return;  // skip row 1 drawing below
  }

  // Thermometer icon
  lcd.write(byte(1));
  lcd.print(F(" "));

  // Temperature: e.g. "28.5" (right-pad to fixed width)
  if (tempC < 10.0) lcd.print(F(" "));   // leading space for single digit
  lcd.print(tempC, 1);
  lcd.write(byte(0));     // ° symbol
  lcd.print(F("C"));

  lcd.print(F("  "));

  // Drop icon + humidity
  lcd.write(byte(2));
  lcd.print(F(":"));
  if (humidity < 10.0) lcd.print(F(" "));
  lcd.print((int)humidity);   // DHT11 gives integer humidity
  lcd.print(F("%  "));        // trailing spaces clear old chars

  // ── Row 1 ─────────────────────────────────────────────────
  lcd.setCursor(0, 1);
  lcd.print(F("Ang:"));

  // 3-digit angle with leading zeros
  if (servoAngle < 100) lcd.print(F("0"));
  if (servoAngle < 10)  lcd.print(F("0"));
  lcd.print(servoAngle);

  lcd.print(F("  "));

  // Direction label
  if (diff > LDR_THRESHOLD) {
    lcd.print(F("<< L"));    // tracking left
  } else if (diff < -LDR_THRESHOLD) {
    lcd.print(F("R >>"));    // tracking right
  } else {
    lcd.print(F("=OK="));    // balanced / locked on sun
  }

  lcd.print(F("   "));       // clear any leftover chars

  // ── Serial mirror ─────────────────────────────────────────
  Serial.print(F("L:")); Serial.print(ldrL);
  Serial.print(F(" R:")); Serial.print(ldrR);
  Serial.print(F(" Δ:")); Serial.print(diff);
  Serial.print(F(" Servo:")); Serial.print(servoAngle);
  Serial.print(F("° T:")); Serial.print(tempC, 1);
  Serial.print(F("C H:")); Serial.print(humidity, 1);
  Serial.println(F("%"));
}