#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ==== WiFi Config ====
const char* ssid = "Control Engineering";
const char* password = "staff@CE2025";

// ==== Flask Server ====
String serverUrl = "http://192.168.100.99:5000/data";

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);  // RX2 = GPIO16, TX2 = GPIO17

  Serial.println("ESP32 JSON Parser Started...");

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.println(WiFi.localIP());
}

void loop() {
  static String incomingData = "";

  while (Serial2.available()) {
    char c = Serial2.read();
    if (c == '\n') {
      Serial.println("=== Received JSON ===");
      Serial.println(incomingData);

      StaticJsonDocument<4000> doc; // Increase size for g1 & g2
      DeserializationError error = deserializeJson(doc, incomingData);

      if (error) {
        Serial.print("❌ JSON Parse failed: ");
        Serial.println(error.f_str());
      } else {
        // --- DS18B20 ---
        float temp1 = doc["ds18b20"]["temp1"];
        float temp2 = doc["ds18b20"]["temp2"];
        Serial.printf("DS18B20 Temp1: %.2f °C\n", temp1);
        Serial.printf("DS18B20 Temp2: %.2f °C\n", temp2);

        // --- g1 ---
        Serial.println("--- g1 ---");
        Serial.printf("Solar Voltage: %.2f V\n", doc["g1"]["solarVoltage"].as<float>());
        Serial.printf("Solar Power: %.2f W\n", doc["g1"]["solarPower"].as<float>());
        Serial.printf("Solar Current: %.2f A\n", doc["g1"]["solarCurrent"].as<float>());
        Serial.printf("Grid Voltage: %.2f V\n", doc["g1"]["gridVoltage"].as<float>());
        Serial.printf("Grid Frequency: %.2f Hz\n", doc["g1"]["gridFrequency"].as<float>());
        Serial.printf("AC Watt: %.2f W\n", doc["g1"]["ACWatt"].as<float>());
        Serial.printf("AC Current: %.2f A\n", doc["g1"]["ACCurrent"].as<float>());
        Serial.printf("Battery Voltage: %.2f V\n", doc["g1"]["batVoltage"].as<float>());
        Serial.printf("Charge Power: %.2f W\n", doc["g1"]["chargePower"].as<float>());
        Serial.printf("Charge Current: %.2f A\n", doc["g1"]["chargeCurrent"].as<float>());
        Serial.printf("Battery Temp: %.2f °C\n", doc["g1"]["batTempt"].as<float>());
        Serial.printf("Inverter Temp: %.2f °C\n", doc["g1"]["inverterTemp"].as<float>());
        Serial.printf("Load Percent: %.2f %%\n", doc["g1"]["loadPercent"].as<float>());
        Serial.printf("Output Voltage: %.2f V\n", doc["g1"]["outputVoltage"].as<float>());
        Serial.printf("Output Current: %.2f A\n", doc["g1"]["outputCurrent"].as<float>());
        Serial.printf("Output Frequency: %.2f Hz\n", doc["g1"]["outputFrequency"].as<float>());
        Serial.printf("Battery SoC: %.2f %%\n", doc["g1"]["batterySoC"].as<float>());
        Serial.printf("Warning Code: %d\n", doc["g1"]["warningCode"].as<int>());
        Serial.printf("Fault Code: %d\n", doc["g1"]["faultCode"].as<int>());
        Serial.printf("Status: %d\n", doc["g1"]["status"].as<int>());

        // --- g2 ---
        Serial.println("--- g2 ---");
        Serial.printf("Solar Voltage: %.2f V\n", doc["g2"]["solarVoltage"].as<float>());
        Serial.printf("Solar Power: %.2f W\n", doc["g2"]["solarPower"].as<float>());
        Serial.printf("Solar Current: %.2f A\n", doc["g2"]["solarCurrent"].as<float>());
        Serial.printf("Grid Voltage: %.2f V\n", doc["g2"]["gridVoltage"].as<float>());
        Serial.printf("Grid Frequency: %.2f Hz\n", doc["g2"]["gridFrequency"].as<float>());
        Serial.printf("AC Watt: %.2f W\n", doc["g2"]["ACWatt"].as<float>());
        Serial.printf("AC Current: %.2f A\n", doc["g2"]["ACCurrent"].as<float>());
        Serial.printf("Battery Voltage: %.2f V\n", doc["g2"]["batVoltage"].as<float>());
        Serial.printf("Charge Power: %.2f W\n", doc["g2"]["chargePower"].as<float>());
        Serial.printf("Charge Current: %.2f A\n", doc["g2"]["chargeCurrent"].as<float>());
        Serial.printf("Battery Temp: %.2f °C\n", doc["g2"]["batTempt"].as<float>());
        Serial.printf("Inverter Temp: %.2f °C\n", doc["g2"]["inverterTemp"].as<float>());
        Serial.printf("Load Percent: %.2f %%\n", doc["g2"]["loadPercent"].as<float>());
        Serial.printf("Output Voltage: %.2f V\n", doc["g2"]["outputVoltage"].as<float>());
        Serial.printf("Output Current: %.2f A\n", doc["g2"]["outputCurrent"].as<float>());
        Serial.printf("Output Frequency: %.2f Hz\n", doc["g2"]["outputFrequency"].as<float>());
        Serial.printf("Battery SoC: %.2f %%\n", doc["g2"]["batterySoC"].as<float>());
        Serial.printf("Warning Code: %d\n", doc["g2"]["warningCode"].as<int>());
        Serial.printf("Fault Code: %d\n", doc["g2"]["faultCode"].as<int>());
        Serial.printf("Status: %d\n", doc["g2"]["status"].as<int>());

        // --- Send full JSON to Flask ---
        if (WiFi.status() == WL_CONNECTED) {
          HTTPClient http;
          http.begin(serverUrl);
          http.addHeader("Content-Type", "application/json");

          int httpResponseCode = http.POST(incomingData);

          if (httpResponseCode > 0) {
            Serial.printf("✅ Data sent! Response code: %d\n", httpResponseCode);
          } else {
            Serial.printf("❌ Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
          }
          http.end();
        } else {
          Serial.println("⚠️ WiFi not connected, cannot send data.");
        }
      }

      Serial.println("======================");
      incomingData = ""; // Reset buffer
    } else {
      incomingData += c;
    }
  }
}
