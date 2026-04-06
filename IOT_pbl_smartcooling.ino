/*
 * IoT Thermal Monitoring System - V4 (ULTIMATE FIX)
 * FIXED: L298N PWM Frequency changed to 1000Hz for HW-095 compatibility.
 * ADDED: Diagnostic Boot Beep to prove ESP32 is alive.
 * LOWERED: Overheat threshold to 32C for easy testing.
 */

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Wi-Fi Credentials
#define WIFI_SSID "SW"
#define WIFI_PASSWORD "12345678"

// Firebase Configuration
#define FIREBASE_HOST "temperaturectrl-default-rtdb.asia-southeast1.firebasedatabase.app"
#define API_KEY "AIzaSyBv6RvYUwO2r5KMwarh_kSRmqK-f59XNtE" 

// ESP32 Pin Definitions
#define DHTPIN 4
#define DHTTYPE DHT11    // Blue Sensor
#define MOTOR_EN_PIN 16  // HW-095 ENA (PWM Speed)
#define MOTOR_IN1_PIN 17 // HW-095 IN1 (Forward)
#define MOTOR_IN2_PIN 5  // HW-095 IN2 (Reverse/Ground)
#define LED_GREEN 18
#define LED_RED 19
#define BUZZER_PIN 23

// L298N Hardware PWM Properties (MUST BE SLOW ~1000Hz for L298N to work)
const int pwmFreq = 1000; 
const int pwmChannel = 0;
const int pwmResolution = 8; // 0-255

// Objects
DHT dht(DHTPIN, DHTTYPE);
// REVERTED: 0x3F caused a system hang. Your HW-61 uses the standard 0x27!
LiquidCrystal_I2C lcd(0x27, 16, 2); 
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long previousMillis = 0;
unsigned long previousWiFiMillis = 0;
const long interval = 2000; 
String currentMode = "off"; 
int currentFanSpeed = 0; 
bool sensorError = false; 

void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== SYSTEM BOOTING ===");

  // --- NEW: I2C Scanner for HW-61 Debugging ---
  Wire.begin(21, 22); 
  Serial.println("Scanning I2C bus for HW-61 LCD...");
  int nDevices = 0;
  for(byte address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("SUCCESS: I2C device (HW-61) found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("ERROR: No I2C devices found! Check LCD SDA(21)/SCL(22) wiring.");

  // Initialize Output Pins
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // 1. DIAGNOSTIC BEEP (If you don't hear this, board is not running!)
  digitalWrite(BUZZER_PIN, HIGH);
  delay(500);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("Diagnostic Beep completed.");

  // 2. Initialize Motor to OFF securely
  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW); // IN2 must be LOW for forward spin
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(MOTOR_EN_PIN, pwmChannel);
  ledcWrite(pwmChannel, 0);

  // 3. Initialize LCD
  Serial.println("Initializing LCD...");
  // Wire.begin(21, 22);  <- Moved to the top for the scanner
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Booting..");
  Serial.println("LCD Initialized.");

  // 4. WiFi Connection
  Serial.print("Connecting to WiFi ");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
    lcd.clear();
    lcd.print("WiFi Connected!");
  } else {
    Serial.println("\nWiFi Failed! Running offline mode.");
    lcd.clear();
    lcd.print("WiFi Failed!");
  }

  // 5. Firebase Setup
  Serial.println("Initializing Firebase...");
  config.api_key = API_KEY;
  config.database_url = FIREBASE_HOST;
  Firebase.signUp(&config, &auth, "", ""); 
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase Ready.");

  // 6. Sensor Init
  Serial.println("Warming up DHT11...");
  dht.begin();
  delay(2000); 
  lcd.clear();
  
  Serial.println("=== SYSTEM RUNNING ===");
}

void loop() {
  unsigned long currentMillis = millis();

  // Reconnect WiFi if lost
  if (WiFi.status() != WL_CONNECTED) {
    if (currentMillis - previousWiFiMillis >= 10000) {
      WiFi.disconnect();
      WiFi.reconnect();
      previousWiFiMillis = currentMillis;
    }
  }

  // Main Logic Loop (Runs every 2 seconds)
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    // Verify Sensor
    if (isnan(humidity) || isnan(temperature)) {
      sensorError = true;
      temperature = 0.0;
      humidity = 0.0;
    } else {
      sensorError = false;
    }

    // Firebase Sync
    if (Firebase.ready()) {
      Firebase.RTDB.setBool(&fbdo, "/sensor/error", sensorError);
      if (!sensorError) {
        Firebase.RTDB.setFloat(&fbdo, "/sensor/temperature", temperature);
        Firebase.RTDB.setFloat(&fbdo, "/sensor/humidity", humidity);
      }
      if (Firebase.RTDB.getString(&fbdo, "/control/mode")) {
        currentMode = fbdo.stringData();
      }
    }
    
    // Execute Hardware
    executeCoolingLogic(temperature, currentMode, sensorError);
    executeSafetyAlerts(temperature, sensorError);
    updateLCD(temperature, humidity, sensorError);
  }
}

void executeCoolingLogic(float temp, String mode, bool hasError) {
  int targetPwm = 0;

  // Manual Controls (Works even if sensor is broken)
  if (mode == "off") targetPwm = 0;
  else if (mode == "low") targetPwm = 100;
  else if (mode == "mid") targetPwm = 175;
  else if (mode == "high") targetPwm = 255;
  
  // Auto Controls
  else if (mode == "auto") {
    if (hasError) targetPwm = 0; 
    else if (temp < 25.0) targetPwm = 0;
    else if (temp >= 25.0 && temp < 28.0) targetPwm = 120;
    else if (temp >= 28.0 && temp < 31.0) targetPwm = 180;
    else targetPwm = 255;
  }
  else if (mode == "ai") {
    if (hasError) targetPwm = 0; 
    else if (Firebase.RTDB.getInt(&fbdo, "/control/fanSpeed")) {
      targetPwm = fbdo.intData();
    }
  }

  targetPwm = constrain(targetPwm, 0, 255);
  currentFanSpeed = targetPwm;

  // HW-095 Motor Execution
  digitalWrite(MOTOR_IN2_PIN, LOW); // Always LOW for forward motion
  
  if (targetPwm > 0) {
    digitalWrite(MOTOR_IN1_PIN, HIGH); 
    ledcWrite(pwmChannel, targetPwm);  
  } else {
    digitalWrite(MOTOR_IN1_PIN, LOW);  
    ledcWrite(pwmChannel, 0);          
  }
}

void executeSafetyAlerts(float temp, bool hasError) {
  if (hasError) {
    digitalWrite(LED_RED, (millis() / 500) % 2); // Fast Blink
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    return; 
  }

  // LOWERED THRESHOLDS FOR EASIER TESTING
  if (temp >= 32.0) { // OVERHEAT (Was 35)
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER_PIN, HIGH); // Continuous Beep!
  } else if (temp >= 30.0) { // WARNING
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  } else { // NORMAL
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void updateLCD(float temp, float hum, bool hasError) {
  lcd.setCursor(0, 0);
  
  if (hasError) {
    lcd.print("T:ERR  H:ERR    "); 
    Serial.println("\n[ERROR] DHT11 SENSOR NOT DETECTED!");
  } else {
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print("C H:");
    lcd.print(hum, 0);
    lcd.print("%  ");
  }

  lcd.setCursor(0, 1);
  String modeStr = currentMode;
  modeStr.toUpperCase();
  lcd.print("M:");
  lcd.print(modeStr.substring(0,4)); 
  lcd.print(" P:");
  lcd.print(currentFanSpeed);
  lcd.print("   "); 

  // Live Serial Output
  Serial.println("\n--- LIVE DATA ---");
  Serial.print("Temp: "); Serial.print(temp, 1); Serial.println("C");
  Serial.print("Hum : "); Serial.print(hum, 0); Serial.println("%");
  Serial.print("Mode: "); Serial.println(modeStr);
  Serial.print("PWM : "); Serial.println(currentFanSpeed);
}