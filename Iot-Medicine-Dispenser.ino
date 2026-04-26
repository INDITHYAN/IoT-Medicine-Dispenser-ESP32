#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ==================== CONFIG ====================
const char* ssid = "your wifi";
const char* password = "your ssid";

// Twilio
String ACCOUNT_SID   = "SID";
String AUTH_TOKEN    = "AUth TOken";
String TWILIO_NUMBER = "Number";
String CALL_TO       = "Recevier Number";

// Telegram
const String BOT_TOKEN = "Token";
const String CHAT_ID   = "Chat Id";

// =====================================================

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo dispenserServo;

const int SERVO_PIN  = 13;
const int BUZZER_PIN = 12;
const int TRIG_PIN   = 5;
const int ECHO_PIN   = 18;

const int COUNTDOWN_SECONDS    = 30;
const int TAKE_TIMEOUT_SECONDS = 30;
const float HAND_THRESHOLD_CM  = 5.0;

int secondsLeft = COUNTDOWN_SECONDS;
unsigned long lastTick = 0;
bool wifiConnected = false;

// ------------------------------------------------------------------
// Telegram
void sendTelegram(String message) {
  if (!wifiConnected) return;
  message.replace(" ", "%20");
  HTTPClient http;
  String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + message;
  http.begin(url);
  http.GET();
  http.end();
  Serial.println("Telegram Sent: " + message);
}

// ------------------------------------------------------------------
// Twilio Call
void makeTwilioCall() {
  if (!wifiConnected) {
    Serial.println("No WiFi - Cannot make call");
    return;
  }
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  String url = "https://api.twilio.com/2010-04-01/Accounts/" + ACCOUNT_SID + "/Calls.json";
  String twiml = "<Response><Say voice='Polly.Joanna'>Please take your medicine immediately.</Say><Pause length='1'/><Say voice='Polly.Joanna'>Marundai sapudunga.</Say></Response>";

  String encoded = twiml;
  encoded.replace(" ", "%20"); encoded.replace("<", "%3C"); encoded.replace(">", "%3E");
  encoded.replace("'", "%27"); encoded.replace(",", "%2C"); encoded.replace(".", "%2E");

  String postData = "To=" + CALL_TO + "&From=" + TWILIO_NUMBER + "&Twiml=" + encoded;

  https.begin(client, url);
  https.setAuthorization(ACCOUNT_SID.c_str(), AUTH_TOKEN.c_str());
  https.addHeader("Content-Type", "application/x-www-form-urlencoded");
  
  int code = https.POST(postData);
  Serial.println((code == 201) ? "✅ Twilio Call Sent Successfully!" : "❌ Twilio Call Failed");
  https.end();
}

// ------------------------------------------------------------------
// Ultrasonic
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  float distance = (duration * 0.0343) / 2.0;
  return (distance < 0 || distance > 400) ? 999 : distance;
}

// ------------------------------------------------------------------
// Buzzer & LCD
void buzzBeep(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH); delay(onMs);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < times-1) delay(offMs);
  }
}

void showCountdown(int secs) {
  lcd.setCursor(0, 0); lcd.print(" Medicine Box ");
  lcd.setCursor(0, 1); lcd.print("Next dose: ");
  if (secs < 10) lcd.print("0");
  lcd.print(secs); lcd.print("s ");
}

// ------------------------------------------------------------------
// Dispense Function
void dispenseMedicine() {
  Serial.println("\n=== DISPENSING MEDICINE ===");
  
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(" DISPENSING! ");
  lcd.setCursor(0, 1); lcd.print(" Take medicine ");
  
  buzzBeep(3, 200, 150);
  dispenserServo.write(180);
  delay(2000);
  dispenserServo.write(0);
  delay(1000);
  buzzBeep(1, 500, 0);

  sendTelegram("🔔 Medicine dispensed! Please take now.");

  // Hand Detection Phase
  Serial.println("Starting 30s hand detection...");
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(" Take Medicine ");
  lcd.setCursor(0, 1); lcd.print("Detecting.. ");

  unsigned long start = millis();
  bool taken = false;

  while (millis() - start < TAKE_TIMEOUT_SECONDS * 1000UL) {
    float dist = getDistance();
    Serial.printf("Distance: %.1f cm\n", dist);

    if (dist < HAND_THRESHOLD_CM) {
      taken = true;
      Serial.println("✅ Hand Detected!");
      break;
    }

    int left = TAKE_TIMEOUT_SECONDS - ((millis()-start)/1000);
    lcd.setCursor(13,1); 
    lcd.print(left < 10 ? "0" : ""); lcd.print(left); lcd.print("s");
    delay(700);
  }

  if (taken) {
    Serial.println("Medicine Taken Successfully");
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(" Medicine Taken ");
    lcd.setCursor(0,1); lcd.print(" Thank You! 👍 ");
    buzzBeep(3,150,100);
    sendTelegram("✅ Medicine taken successfully!");
  } else {
    Serial.println("❌ No Response - Calling Caretaker");
    lcd.clear();
    lcd.setCursor(0,0); lcd.print(" NO RESPONSE! ");
    lcd.setCursor(0,1); lcd.print(" Calling Carer ");
    buzzBeep(8,120,80);
    makeTwilioCall();
    sendTelegram("🚨 URGENT: Medicine NOT taken! Called caretaker.");
  }

  delay(2500);
  secondsLeft = COUNTDOWN_SECONDS;
  lastTick = millis();
  showCountdown(secondsLeft);
}

// ------------------------------------------------------------------
// SETUP
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== IoT Medicine Dispenser Starting ===\n");

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dispenserServo.attach(SERVO_PIN, 500, 2400);
  dispenserServo.write(0);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(" IoT Med Box ");
  lcd.setCursor(0, 1); lcd.print(" Booting... ");
  delay(800);

  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart < 12000)) {
    delay(300);
    Serial.print(".");
  }

  wifiConnected = (WiFi.status() == WL_CONNECTED);

  if (wifiConnected) {
    Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("WiFi Connected");
    lcd.setCursor(0, 1); lcd.print(WiFi.localIP().toString());
    sendTelegram("🟢 Medicine Dispenser Started Successfully!");
  } else {
    Serial.println("\nWiFi Failed - Running Offline");
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("WiFi Failed!");
    lcd.setCursor(0, 1); lcd.print("Offline Mode");
  }

  delay(1200);
  lcd.clear();
  lastTick = millis();
  showCountdown(secondsLeft);

  Serial.println("✅ System Ready - Fast Boot Mode\n");
}

// ------------------------------------------------------------------
// LOOP
void loop() {
  unsigned long now = millis();
  if (now - lastTick >= 1000) {
    lastTick += 1000;
    secondsLeft--;

    if (secondsLeft <= 0) {
      dispenseMedicine();
    } else {
      showCountdown(secondsLeft);
      if (secondsLeft == 10) buzzBeep(2, 100, 100);
      if (secondsLeft == 5)  buzzBeep(3, 80, 80);
    }
  }
}