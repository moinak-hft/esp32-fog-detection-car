/*
   ESP32 WiFi Fog Detection Car - FINAL VERSION
   
   Fixes Included:
   1. SILENT SHUTDOWN: Increased LDR_OFF_THRESHOLD to 200.
      - Prevents buzzer from ringing when you switch off the laser/system.
   2. FILTERED SENSORS: Removes ultrasonic flickering (-1).
   3. SAFE PINS: Laser moved to Pin 27.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h> 
#include <Wire.h>

// --- WIFI CONFIGURATION ---
const char* ssid = "ESP32_FOG_CAR";
const char* password = "password123";

// --- PIN DEFINITIONS ---
const int IN1 = 26; // Right Motor
const int IN2 = 25;
const int IN3 = 33; // Left Motor
const int IN4 = 32;

// I2C LCD (SDA=21, SCL=22)
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Sensors
const int TRIG_PIN = 5;
const int ECHO_PIN = 18;
const int LASER_PIN = 27; 
const int BUZZER_PIN = 19;
const int LDR_PIN = 34; 

// --- LOGIC CONSTANTS (CALIBRATION) ---
const int LDR_CLEAR_THRESHOLD = 2000; // > 2000 = Clear Path
const int LDR_DENSE_FOG_THRESHOLD = 800; // < 800 = Foggy
const int LDR_OFF_THRESHOLD = 200; // < 200 = System/Laser OFF -> Silence

const int SPEED_MAX = 255;
const int SPEED_SAFE = 140; 
const int SPEED_STOP = 0;

// --- GLOBAL VARIABLES ---
WebServer server(80);
int currentMaxSpeed = SPEED_MAX;
String driveState = "S"; 

long lastLCDUpdate = 0;
long lastSensorRead = 0;

float distance = 0;      
float lastValidDist = 0; 
int ldrValue = 0;
bool fogDetected = false;

// PWM Channels
const int freq = 30000;
const int resolution = 8;
const int ch1=0, ch2=1, ch3=2, ch4=3;

// --- HELPER DECLARATIONS ---
void handleRoot();
void applyMotorLogic();
void stopMotors();
float getFilteredDistance(); 
int getAverageLDR();
void updateDisplay();

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  delay(500);

  // Motor Setup
  ledcSetup(ch1, freq, resolution); ledcAttachPin(IN1, ch1);
  ledcSetup(ch2, freq, resolution); ledcAttachPin(IN2, ch2);
  ledcSetup(ch3, freq, resolution); ledcAttachPin(IN3, ch3);
  ledcSetup(ch4, freq, resolution); ledcAttachPin(IN4, ch4);
  
  // Sensor Setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LASER_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); 
  pinMode(LDR_PIN, INPUT);

  // Turn on Laser Logic (if connected to Pin 27)
  digitalWrite(LASER_PIN, HIGH); 
  digitalWrite(TRIG_PIN, LOW);   
  delay(100);

  // LCD Setup
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("System Starting");
  delay(1000);
  lcd.clear();

  // WiFi Setup
  WiFi.softAP(ssid, password);
  delay(500);
  
  Serial.println("System Ready.");
  lcd.setCursor(0, 0);
  lcd.print("IP:");
  lcd.print(WiFi.softAPIP());

  // Web Server Routes
  server.on("/", handleRoot);
  server.on("/F", []() { driveState = "F"; server.send(200, "text/plain", "F"); });
  server.on("/B", []() { driveState = "B"; server.send(200, "text/plain", "B"); });
  server.on("/L", []() { driveState = "L"; server.send(200, "text/plain", "L"); });
  server.on("/R", []() { driveState = "R"; server.send(200, "text/plain", "R"); });
  server.on("/S", []() { driveState = "S"; server.send(200, "text/plain", "S"); });
  
  server.begin();
}

// --- MAIN LOOP ---
void loop() {
  server.handleClient(); 

  // Read sensors every 60ms
  if (millis() - lastSensorRead > 60) {
    
    distance = getFilteredDistance();
    ldrValue = getAverageLDR();
    
    // --- FOG & SHUTDOWN LOGIC ---
    if (ldrValue >= LDR_CLEAR_THRESHOLD) {
      // CASE 1: CLEAR
      currentMaxSpeed = SPEED_MAX;
      digitalWrite(BUZZER_PIN, LOW);
      fogDetected = false;
    } 
    else if (ldrValue < LDR_CLEAR_THRESHOLD && ldrValue > LDR_DENSE_FOG_THRESHOLD) {
      // CASE 2: LIGHT FOG
      currentMaxSpeed = SPEED_SAFE;
      digitalWrite(BUZZER_PIN, HIGH); 
      fogDetected = true;
    } 
    else if (ldrValue <= LDR_DENSE_FOG_THRESHOLD && ldrValue > LDR_OFF_THRESHOLD) {
      // CASE 3: HEAVY FOG (Real)
      currentMaxSpeed = SPEED_STOP;
      digitalWrite(BUZZER_PIN, HIGH); 
      fogDetected = true;
    }
    else {
      // CASE 4: SYSTEM OFF / LASER OFF
      // Light is < 200. Silence the buzzer.
      currentMaxSpeed = SPEED_STOP;
      digitalWrite(BUZZER_PIN, LOW); // <--- SILENCE
      fogDetected = true; 
    }

    lastSensorRead = millis();
  }

  applyMotorLogic();

  // Update LCD & Serial Monitor every 500ms
  if (millis() - lastLCDUpdate > 500) {
    updateDisplay();
    
    // SERIAL DEBUGGING
    Serial.print("LDR: "); Serial.print(ldrValue);
    Serial.print(" | Dist: "); Serial.print((int)distance); 
    Serial.print("cm | Speed: "); Serial.print(currentMaxSpeed);
    Serial.print(" | Cmd: "); Serial.println(driveState);
    
    lastLCDUpdate = millis();
  }

  delay(5); 
}

// --- HELPER FUNCTIONS ---

float getFilteredDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) return lastValidDist; 
  float cm = (duration * 0.0343) / 2;
  if (cm > 400 || cm < 2) return lastValidDist; 

  lastValidDist = cm; 
  return cm;
}

int getAverageLDR() {
  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(LDR_PIN);
    delay(1);
  }
  return (int)(sum / 20);
}

void applyMotorLogic() {
  int spd = currentMaxSpeed;
  if (driveState == "S") stopMotors();
  else if (driveState == "F") { ledcWrite(ch1, 0); ledcWrite(ch2, spd); ledcWrite(ch3, 0); ledcWrite(ch4, spd); } 
  else if (driveState == "B") { ledcWrite(ch1, spd); ledcWrite(ch2, 0); ledcWrite(ch3, spd); ledcWrite(ch4, 0); } 
  else if (driveState == "L") { ledcWrite(ch1, 0); ledcWrite(ch2, spd); ledcWrite(ch3, spd); ledcWrite(ch4, 0); } 
  else if (driveState == "R") { ledcWrite(ch1, spd); ledcWrite(ch2, 0); ledcWrite(ch3, 0); ledcWrite(ch4, spd); }
}

void stopMotors() {
  ledcWrite(ch1, 0); ledcWrite(ch2, 0); ledcWrite(ch3, 0); ledcWrite(ch4, 0);
}

void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  
  if (ldrValue <= LDR_OFF_THRESHOLD) {
    lcd.print("SYSTEM OFF/LOW"); 
  }
  else if (fogDetected) {
    lcd.print(currentMaxSpeed == 0 ? "DANGER: FOG!" : "CAUTION: FOG");
  } else {
    lcd.print("PATH CLEAR");
  }

  lcd.setCursor(0, 1);
  lcd.print("D:");
  lcd.print((int)distance); 
  lcd.print(" L:"); 
  lcd.print(ldrValue);
}

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <title>FogBot Control</title>
      <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
      <style>
        /* Minimalist Dark Theme */
        body { 
          background-color: #121212; 
          color: #ffffff; 
          font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
          display: flex;
          flex-direction: column;
          align-items: center;
          justify-content: center;
          height: 100vh;
          margin: 0;
          touch-action: none; /* Prevents scroll/zoom on mobile */
        }

        h2 { 
          font-weight: 300; 
          letter-spacing: 2px; 
          margin-bottom: 40px; 
          color: #a0a0a0;
        }

        /* D-Pad Grid Layout */
        .controls {
          display: grid;
          grid-template-columns: 80px 80px 80px;
          grid-template-rows: 80px 80px 80px;
          gap: 15px;
        }

        /* Button Styling */
        .btn {
          width: 80px;
          height: 80px;
          background-color: #1e1e1e;
          border: 1px solid #333;
          border-radius: 50%;
          color: #e0e0e0;
          font-size: 28px;
          cursor: pointer;
          display: flex;
          align-items: center;
          justify-content: center;
          transition: all 0.1s ease;
          box-shadow: 0 4px 10px rgba(0,0,0,0.3);
          -webkit-tap-highlight-color: transparent;
        }

        /* Active State (Pressed) */
        .btn:active {
          background-color: #00d2ff; /* Neon Blue Accent */
          color: #000;
          transform: scale(0.95);
          box-shadow: 0 2px 5px rgba(0,0,0,0.2);
          border-color: #00d2ff;
        }

        /* Grid Positioning */
        #F { grid-column: 2; grid-row: 1; }
        #L { grid-column: 1; grid-row: 2; }
        #R { grid-column: 3; grid-row: 2; }
        #B { grid-column: 2; grid-row: 3; }
      </style>

      <script>
        function mv(c) { fetch("/" + c); }
        function su() { fetch("/S"); }
        
        window.onload = () => {
          ['F','B','L','R'].forEach(d => {
             let b = document.getElementById(d);
             // Touch Events (Mobile)
             b.addEventListener('touchstart', (e) => { e.preventDefault(); mv(d); });
             b.addEventListener('touchend', (e) => { e.preventDefault(); su(); });
             // Mouse Events (Desktop)
             b.addEventListener('mousedown', () => { mv(d); });
             b.addEventListener('mouseup', () => { su(); });
          });
        };
      </script>
    </head>
    <body>
      <h2>FOGBOT</h2>
      
      <div class="controls">
        <div id="F" class="btn">&#8593;</div>
        <div id="L" class="btn">&#8592;</div>
        <div id="R" class="btn">&#8594;</div>
        <div id="B" class="btn">&#8595;</div>
      </div>
      
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}