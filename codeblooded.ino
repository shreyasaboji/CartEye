#include <WiFi.h>
#include <HTTPClient.h>
#include "HX711.h"

// ===== CONFIG =====
const char* ssid     = "Lecture Hall 2";
const char* password = "SOE-LH@2000";
const char* FIREBASE_URL = "https://carteyeweb-default-rtdb.firebaseio.com/arduino/current_item.json";

// ===== HX711 CONFIG =====
HX711 scale;
const int LOADCELL_DOUT_PIN = 19;
const int LOADCELL_SCK_PIN  = 18;
float calibration_factor = -7050.0;

// Track last sent values
float lastSentWeight = 0;
unsigned long lastSendMillis = 0;
const unsigned long SEND_INTERVAL = 1200; // ms

// 🔍 Simple lookup for item recognition
String identifyItem(float weight) {
  if (weight < 10) return "Empty";

  if (weight > 10 && weight < 50)
    return "Coke";
  else if (weight > 450 && weight < 600)
    return "Water Bottle";
  else if (weight > 200 && weight < 300)
    return "detergent";
  else if (weight > 10 && weight < 50)
    return "sprite";
  else
    return "Unknown Item";
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");

  // Setup HX711
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  scale.tare();

  Serial.println("🔧 Setup done.");
}

// Read weight from HX711
float readWeightGrams() {
  float val = scale.get_units(10);
  if (val < 0.0) val = 0.0;
  Serial.printf("⚖️  Weight: %.2f g\n", val);
  return val;
}

// Send data to Firebase
void sendToFirebase(float weight, String itemName) {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(FIREBASE_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"weight\":" + String(weight, 2) + ",";
  payload += "\"item\":\"" + itemName + "\",";
  payload += "\"timestamp\":" + String(millis());
  payload += "}";

  int httpCode = http.PUT(payload);
  if (httpCode > 0) {
    Serial.printf("✅ Sent: %s | resp=%d\n", payload.c_str(), httpCode);
  } else {
    Serial.printf("❌ HTTP error: %d\n", httpCode);
  }
  http.end();
}

void loop() {
  float w = readWeightGrams();
  if (w < 5.0) { w = 0; }

  String item = identifyItem(w);
  Serial.println("🛒 Detected Item: " + item);

  if ((fabs(w - lastSentWeight) > 0.5) || (millis() - lastSendMillis > SEND_INTERVAL)) {
    lastSentWeight = w;
    lastSendMillis = millis();
    sendToFirebase(w, item);
  }

  delay(400);
}


