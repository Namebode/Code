#include <ModbusMaster.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <BH1750FVI.h>


#define RS485_RX 16
#define RS485_TX 17
#define RS485_DIR 2
#define Sensor 3  
ModbusMaster node;

// WiFi & Server Configuration
const char* ssid = "AnuchaWiFi";
const char* password = "Anucha6457";

// BH1750FVI Light Sensor
BH1750FVI lightMeter(BH1750FVI::k_DevModeContHighRes2);
uint16_t light_value = 0;

void preTransmission() {
  digitalWrite(RS485_DIR, 1);
}

void postTransmission() {
  digitalWrite(RS485_DIR, 0);
}

void startWiFiManager() {
  Serial.println("Starting WiFiManager...");
  WiFiManager wm;
  Serial.println("WiFiManager: Connected to WiFi.");
  Serial.print("WiFi SSID: ");
  Serial.println(WiFi.SSID());
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Starting setup...");
  WiFi.mode(WIFI_STA);

  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  startWiFiManager();

  pinMode(RS485_DIR, OUTPUT);
  Serial2.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  Serial.println("\n Starting"); 
  digitalWrite(RS485_DIR, 0);

  node.begin(Sensor, Serial2);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  Wire.begin(21, 22);  // กำหนดพิน SDA และ SCL
  lightMeter.begin();
  delay(500);
  light_value = lightMeter.GetLightIntensity();

  if (light_value == 0) {
    Serial.println("ไม่พบเซ็นเซอร์ BH1750! รีสตาร์ท...");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("BH1750FVI เริ่มต้นสำเร็จ!");
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    WiFi.begin(ssid, password);
    return;
  }

  Serial.println("Attempting to read Modbus register...");
  uint8_t result = node.readHoldingRegisters(0, 1);

  
  if (result == node.ku8MBSuccess) {
    float wind_speed = node.getResponseBuffer(0) / 10.0;
    Serial.printf("Wind Speed = : %.2f\n", wind_speed);
  } else {
    Serial.printf("Failed to read, error code: %02X\n", result);
    if (result == 0xE2) {
      Serial.println("Timeout error: Slave ไม่ตอบกลับ");
      // Debug: ตรวจสอบสถานะ Serial2
      Serial.println("Checking Serial2 connection...");
      Serial2.flush();
      delay(1000);
    }
  }

  light_value = lightMeter.GetLightIntensity();
  if (light_value == 0) {
    Serial.println("เกิดข้อผิดพลาดในการอ่านแสง! รีเซ็ตเซ็นเซอร์...");
    Wire.begin(21, 22);
    lightMeter.begin();
    delay(500);
  } else {
    Serial.printf("Light = %d lux\n", light_value);
  }
  delay(2000);
}