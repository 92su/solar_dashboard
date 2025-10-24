#include <ModbusMaster.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Define RS485 control pins (Mega)
#define MAX485_DE  6
#define MAX485_RE  7

// Define OneWire bus for DS18B20 sensors
#define ONE_WIRE_BUS 3 
#define DISCHARGE_POWER_H_REG 0x49      // Input Reg 73 for Battery Discharge Power High
#define DISCHARGE_POWER_L_REG 0x4A      // Input Reg 74 for Battery Discharge Power Low
#define BATTERY_POWER_H_REG 0x4D      // Input Reg 77 for Battery Power High
#define BATTERY_POWER_L_REG 0x4E      // Input Reg 78 for Battery Power Low

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Modbus instance
//ModbusMaster kws_ac301l;
ModbusMaster growatt1;
ModbusMaster growatt2;
/*
// Variables for KWS-AC301L
float kws_voltage = -1.0;
float kws_current = -1.0;
int16_t kws_temperature = -1;
*/
// Variables for DS18B20 sensors
float temp_ds18b20_1 = -999.0;
float temp_ds18b20_2 = -999.0;
//Variables for Growatt Inverter 1
float g1_solarVoltage = -1.0, g1_solarPower = -1.0, g1_solarCurrent = -1.0;
float g1_gridVoltage = -1.0, g1_gridFrequency = -1.0, g1_ACWatt = -1.0, g1_ACCurrent = -1.0;
float g1_batVoltage = -1.0, g1_chargePower = -1.0, g1_chargeCurrent = -1.0, g1_batTempt = -1.0;
float g1_inverterTemp = -1.0;
int g1_loadPercent = -1;
float g1_outputVoltage = -1.0, g1_outputCurrent = -1.0, g1_outputFrequency = -1.0;
float g1_batterySoC = -1.0, g1_dischargePower = -1.0;
int g1_warningCode = -1, g1_faultCode = -1, g1_status = -1;

// Variables for Growatt Inverter 2
float g2_solarVoltage = -1.0, g2_solarPower = -1.0, g2_solarCurrent = -1.0;
float g2_gridVoltage = -1.0, g2_gridFrequency = -1.0, g2_ACWatt = -1.0, g2_ACCurrent = -1.0;
float g2_batVoltage = -1.0,  g2_chargePower = -1.0, g2_chargeCurrent = -1.0, g2_batTempt = -1.0;
float g2_inverterTemp = -1.0;
int g2_loadPercent = -1;
float g2_outputVoltage = -1.0, g2_outputCurrent = -1.0, g2_outputFrequency = -1.0;
float g2_batterySoC = -1.0, g2_dischargePower = -1.0;
int g2_warningCode = -1, g2_faultCode = -1, g2_status = -1;

// Timing variables and state machine for polling
unsigned long previousMillis = 0;
long interval = 10000; // Default interval of 20 seconds
int pollingState = 0;
bool esp32IsConnected = false;
// Function to handle RS485 pre-transmission
void preTransmission() {
  digitalWrite(MAX485_DE, HIGH);
  digitalWrite(MAX485_RE, HIGH);
}

// Function to handle RS485 post-transmission
void postTransmission() {
  digitalWrite(MAX485_DE, LOW);
  digitalWrite(MAX485_RE, LOW);
}
/*
// Function to read KWS-AC301L data
void readKWSAC301L() {
  uint8_t result;
  
  result = kws_ac301l.readHoldingRegisters(14, 1);
  if (result == kws_ac301l.ku8MBSuccess) {
    kws_voltage = (float)kws_ac301l.getResponseBuffer(0) * 0.1;
  } else {
    kws_voltage = -1.0;
    return;
  }

  result = kws_ac301l.readHoldingRegisters(15, 2);
  if (result == kws_ac301l.ku8MBSuccess) {
    uint32_t rawCurrent = ((uint32_t)kws_ac301l.getResponseBuffer(1) << 16) | kws_ac301l.getResponseBuffer(0);
    kws_current = (float)rawCurrent * 0.001;
  } else {
    kws_current = -1.0;
    return;
  }

  result = kws_ac301l.readHoldingRegisters(26, 1);
  if (result == kws_ac301l.ku8MBSuccess) {
    kws_temperature = (int16_t)kws_ac301l.getResponseBuffer(0);
  } else {
    kws_temperature = -1;
  }
}
*/
// Function to read DS18B20 sensor data
void readDS18B20() {
  sensors.requestTemperatures();
  temp_ds18b20_1 = sensors.getTempCByIndex(0);
  temp_ds18b20_2 = sensors.getTempCByIndex(1);
}

void readGrowatt(ModbusMaster& growatt, int inverterNum) {
  uint8_t result;
  uint16_t buffer[100];
  
  // Read first 50 registers
  result = growatt.readInputRegisters(0x00, 50);
  if (result == growatt.ku8MBSuccess) {
    for (int i = 0; i < 50; i++) {
      buffer[i] = growatt.getResponseBuffer(i);
    }
  } else {
    // Return early on failure
    return;
  }

  // Read next 50 registers
  result = growatt.readInputRegisters(0x32, 50);
  if (result == growatt.ku8MBSuccess) {
    for (int i = 0; i < 50; i++) {
      buffer[i + 50] = growatt.getResponseBuffer(i);
    }
  } else {
    // Return early on failure
    return;
  }
/*
  // Read Discharge Power
  float dischargePower = -1.0;
  result = growatt.readInputRegisters(0x49, 2); // Using 0x49 and 2 as an example
  if (result == growatt.ku8MBSuccess) {
      dischargePower = (float)(((uint32_t)growatt.getResponseBuffer(0) << 16) | growatt.getResponseBuffer(1)) * 0.1;
  }*/
  // Read Charge Power
float chargePower = -1.0;
result = growatt.readInputRegisters(0x4D, 2);
if (result == growatt.ku8MBSuccess) {
    int32_t rawPower = ((int32_t)growatt.getResponseBuffer(0) << 16) | growatt.getResponseBuffer(1);
    chargePower = (float)rawPower * -0.1;
    if (chargePower < 0) {
    }
}
  // Parse and store data if both reads were successful
  float solarVoltage = buffer[1] * 0.1;
  float solarPower = ((uint32_t)buffer[3] << 16 | buffer[4]) * 0.1;
  float solarCurrent = (solarVoltage != 0) ? (solarPower / solarVoltage) : 0.0;
  float gridVoltage = buffer[20] * 0.1;
  float gridFrequency = buffer[21] * 0.01;
  float ACWatt = ((uint32_t)buffer[36] << 16 | buffer[37]) * 0.1;
  float ACCurrent = (gridVoltage != 0) ? (ACWatt / gridVoltage) : 0.0;
  float batVoltage = buffer[17] * 0.01;
  //float batPower = ((uint32_t)buffer[77 - 64] << 16 | buffer[78 - 64]) * 0.1;
  //float batChargeCurrent = buffer[68 - 64] * 0.002 ;
  //float batDischargeCurrent = (batVoltage != 0) ? (dischargePower / batVoltage) : 0.0;
  //batDischargeCurrent = roundf(batDischargeCurrent * 100.0) / 100.0;
  float chargeCurrent = (batVoltage != 0) ? (chargePower/ batVoltage) : 0.0;
  chargeCurrent = roundf(chargeCurrent * 100.0) / 100.0;
  float batTempt = buffer[96 - 64] * 0.1;
  float inverterTemp = buffer[25] * 0.1;
  int loadPercent = buffer[27]* 0.1;
  float outputVoltage = buffer[22] * 0.1;
  float outputCurrent = buffer[34] * 0.1;
  float outputFrequency = buffer[23] * 0.01;
  float batterySoC = buffer[18];
  int warningCode = buffer[43];
  int faultCode = buffer[42];
  int status = buffer[0];

  if (inverterNum == 1) {
    g1_solarVoltage = solarVoltage; g1_solarPower = solarPower; g1_solarCurrent = solarCurrent;
    g1_gridVoltage = gridVoltage; g1_gridFrequency = gridFrequency; g1_ACWatt = ACWatt; g1_ACCurrent = ACCurrent;
    g1_batVoltage = batVoltage; //g1_batPower = batPower; g1_batChargeCurrent = batChargeCurrent; g1_batDischargeCurrent = batDischargeCurrent; 
    g1_chargeCurrent = chargeCurrent;g1_batTempt = batTempt;
    g1_inverterTemp = inverterTemp; g1_loadPercent = loadPercent;
    g1_outputVoltage = outputVoltage; g1_outputCurrent = outputCurrent; g1_outputFrequency = outputFrequency;
    g1_batterySoC = batterySoC; g1_warningCode = warningCode; g1_faultCode = faultCode; g1_status = status;
    g1_chargePower = chargePower;
  } else if (inverterNum == 2) {
    g2_solarVoltage = solarVoltage; g2_solarPower = solarPower; g2_solarCurrent = solarCurrent;
    g2_gridVoltage = gridVoltage; g2_gridFrequency = gridFrequency; g2_ACWatt = ACWatt; g2_ACCurrent = ACCurrent;
    g2_batVoltage = batVoltage; //g2_batPower = batPower; g2_batChargeCurrent = batChargeCurrent; g2_batDischargeCurrent = batDischargeCurrent; 
    g2_chargeCurrent = chargeCurrent; g2_batTempt = batTempt;
    g2_inverterTemp = inverterTemp; g2_loadPercent = loadPercent;
    g2_outputVoltage = outputVoltage; g2_outputCurrent = outputCurrent; g2_outputFrequency = outputFrequency;
    g2_batterySoC = batterySoC; g2_warningCode = warningCode; g2_faultCode = faultCode; g2_status = status;
    g2_chargePower = chargePower;
  }
}

void sendAllData() {
  StaticJsonDocument<1500> doc;
  /*
  // Create nested JSON objects
  JsonObject kws = doc.createNestedObject("kws");
  kws["voltage"] = kws_voltage;
  kws["current"] = kws_current;
  kws["temperature"] = kws_temperature;
*/
  // Add DS18B20 data
  JsonObject ds18b20 = doc.createNestedObject("ds18b20");
  ds18b20["temp1"] = temp_ds18b20_1;
  ds18b20["temp2"] = temp_ds18b20_2;
  
  JsonObject g1 = doc.createNestedObject("g1");
  g1["solarVoltage"] = g1_solarVoltage;
  g1["solarPower"] = g1_solarPower;
  g1["solarCurrent"] = g1_solarCurrent;
  g1["gridVoltage"] = g1_gridVoltage;
  g1["gridFrequency"] = g1_gridFrequency;
  g1["ACWatt"] = g1_ACWatt;
  g1["ACCurrent"] = g1_ACCurrent;
  g1["batVoltage"] = g1_batVoltage;
  //g1["batPower"] = g1_batPower;
  //g1["batChargeCurrent"] = g1_batChargeCurrent;
  //g1["dischargePower"] = g1_dischargePower;
  //g1["batDischargeCurrent"] = g1_batDischargeCurrent;
  g1["chargePower"] = g1_chargePower;
  g1["chargeCurrent"] = g1_chargeCurrent;
  g1["batTempt"] = g1_batTempt;
  g1["inverterTemp"] = g1_inverterTemp;
  g1["loadPercent"] = g1_loadPercent;
  g1["outputVoltage"] = g1_outputVoltage;
  g1["outputCurrent"] = g1_outputCurrent;
  g1["outputFrequency"] = g1_outputFrequency;
  g1["batterySoC"] = g1_batterySoC;
  g1["warningCode"] = g1_warningCode;
  g1["faultCode"] = g1_faultCode;
  g1["status"] = g1_status;

  JsonObject g2 = doc.createNestedObject("g2");
  g2["solarVoltage"] = g2_solarVoltage;
  g2["solarPower"] = g2_solarPower;
  g2["solarCurrent"] = g2_solarCurrent;
  g2["gridVoltage"] = g2_gridVoltage;
  g2["gridFrequency"] = g2_gridFrequency;
  g2["ACWatt"] = g2_ACWatt;
  g2["ACCurrent"] = g2_ACCurrent;
  g2["batVoltage"] = g2_batVoltage;
  //g2["batPower"] = g2_batPower;
  //g2["batChargeCurrent"] = g2_batChargeCurrent;
  //g2["dischargePower"] = g2_dischargePower;
  //g2["batDischargeCurrent"] = g2_batDischargeCurrent;
  g2["chargePower"] = g2_chargePower;
  g2["chargeCurrent"] = g2_chargeCurrent;
  g2["batTempt"] = g2_batTempt;
  g2["inverterTemp"] = g2_inverterTemp;
  g2["loadPercent"] = g2_loadPercent;
  g2["outputVoltage"] = g2_outputVoltage;
  g2["outputCurrent"] = g2_outputCurrent;
  g2["outputFrequency"] = g2_outputFrequency;
  g2["batterySoC"] = g2_batterySoC;
  g2["warningCode"] = g2_warningCode;
  g2["faultCode"] = g2_faultCode;
  g2["status"] = g2_status;
  
  // Serialize to buffer
  char jsonBuffer[1500];
  serializeJson(doc, jsonBuffer);

  // Send to Serial2
  Serial2.println(jsonBuffer);
  Serial2.flush(); // Ensure all data is sent before continuing

  // Optional small wait using millis() instead of delay
  unsigned long txWaitStart = millis();
  while (millis() - txWaitStart < 10) {
    // brief non-blocking wait for line settling (adjust if needed)
  }

  // Send to Serial3
  Serial3.println(jsonBuffer);
  Serial3.flush(); // Ensure full transmission

  // Debug output
  Serial.println(jsonBuffer);
}
void setup() {
  pinMode(MAX485_DE, OUTPUT);
  pinMode(MAX485_RE, OUTPUT);
  postTransmission();
  
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600);
 Serial3.begin(9600); // Initialize Serial3
  // Initialize OneWire sensors
  sensors.begin();
 /* 
  kws_ac301l.begin(2, Serial1);
  kws_ac301l.preTransmission(preTransmission);
  kws_ac301l.postTransmission(postTransmission);
*/
  growatt1.begin(3, Serial1);
  growatt1.preTransmission(preTransmission);
  growatt1.postTransmission(postTransmission);
  
  growatt2.begin(1, Serial1);
  growatt2.preTransmission(preTransmission);
  growatt2.postTransmission(postTransmission);

  Serial.println("System Started...");
}
void checkSerialForInterval() {
 if (Serial2.available()) {
    String receivedData = Serial2.readStringUntil('\n');
    Serial.println("Received interval command from ESP32: " + receivedData);

    StaticJsonDocument<50> doc;
    DeserializationError error = deserializeJson(doc, receivedData);


      if (!error) {
        if (doc.containsKey("interval")) {
          int newInterval = doc["interval"];
          if (newInterval >= 1 && newInterval <= 60) {
            interval = (long)newInterval * 1000;
            previousMillis = millis(); // Reset the timer
            Serial.print("Updated data sending interval to ");
            Serial.print(newInterval);
            Serial.println(" seconds.");
          } else {
            Serial.println("Received invalid interval value. Must be between 1 and 60.");
          }
        }
      } else {
        Serial.println("Failed to parse interval JSON.");
      }
    }

}

void loop() {
  unsigned long currentMillis = millis(); 

  // Check for new interval command or 'C' from ESP32
  checkSerialForInterval();
  
  // Send data based on the current interval AND if the ESP32 is connected
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Serial.println("Polling sensors...");
    //readKWSAC301L();
    readGrowatt(growatt1, 1);
    readGrowatt(growatt2, 2);
    readDS18B20(); // Read DS18B20 data
    sendAllData();
  }
}
