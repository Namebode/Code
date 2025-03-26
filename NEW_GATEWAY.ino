#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <queue>

#define ss 5
#define rst 14
#define mosi 23
#define TRIGGER_PIN 4  // Pin to trigger WiFi configuration portal

#define i2c_Address 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// MQTT Configuration
const char* mqttServer = "192.168.43.39";
const int mqttPort = 1883;
const char* mqttUser = "ServerMQTT";
const char* mqttPassword = "ohmza2134";
const char* mqttTopic = "gateway/GATEWAY_TEST";
std::queue<String> messageBuffer; // บัฟเฟอร์สำหรับเก็บข้อความ
bool skipMQTTConnection = false; // ข้ามการเชื่อมต่อ MQTT
int totalReceivedPackets = 0;  // ตัวแปรเก็บจำนวนแพ็กเก็ตที่ได้รับทั้งหมด


WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ตัวแปรสำหรับตรวจสอบ Loss Package
int lastReceivedPackageID = -1; // ค่าเริ่มต้นคือ -1 (ยังไม่ได้รับแพ็กเก็ต)
int receivedPackets = 0;
int lostPackets = 0;

// Function prototypes
void reconnect();
void sendMessageToMQTT(String message);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void startWiFiManager();
int extractPackageID(String message);
void checkLossPackage(int packageID);
float getLossRate();
String formatMessage(String message);

const char* getMqttTopic(String message) {
  static String topic;
  int idStart = message.indexOf("Device_ID =");
  int soilStart = message.indexOf("Soil =");

  if (idStart != -1 && soilStart != -1) {
    String deviceId = message.substring(idStart + String("Device_ID =").length(), soilStart);
    deviceId.trim();

    if (deviceId.length() > 0) {
      topic = "gateway/test_gateway_" + deviceId;
      return topic.c_str();
    }
  }

  topic = "gateway/test_gatewayfail";
  return topic.c_str();
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("Starting setup...");

  // Set up trigger pin for WiFi configuration
  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  // Initialize WiFiManager but don't connect yet
  WiFi.mode(WIFI_STA);

  // Initialize LoRa
  LoRa.setPins(ss, rst, mosi);
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");

  // Initialize OLED display
  Wire.begin();
  if (!display.begin(i2c_Address, true)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // Show WiFi connection prompt on OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Press button to");
  display.println("connect WiFi");
  display.display();

  Serial.println("Press button to connect WiFi");

  // Wait for button press before connecting WiFi
  while (digitalRead(TRIGGER_PIN) == HIGH) {
    delay(100);
  }

  // Start WiFi connection
  startWiFiManager();

  // Set up MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);
}

void startWiFiManager() {
  Serial.println("Starting WiFiManager...");
  WiFiManager wm;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Portal:");
  display.println("LoRaGatewayAP");
  display.println("192.168.4.1");
  display.display();

  if (!wm.autoConnect("LoRaGatewayAP")) {
    Serial.println("WiFiManager: Failed to connect.");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Failed");
    display.println("Restarting...");
    display.display();
    delay(3000);
    ESP.restart();
  }

  Serial.println("WiFiManager: Connected to WiFi.");
  Serial.print("WiFi SSID: ");
  Serial.println(WiFi.SSID());
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Connected!");
  display.println(WiFi.SSID());
  display.display();
  delay(2000);
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Waiting for button press to reconnect...");

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi Disconnected");
    display.println("Press button to");
    display.println("reconnect");
    display.display();

    while (digitalRead(TRIGGER_PIN) == HIGH) {
      delay(100);
    }
    
    startWiFiManager();
  }

  // Check MQTT connection
  if (!client.connected() && !skipMQTTConnection) {
    reconnect();
  }
  client.loop();

//   int packageSize = LoRa.parsePacket();
//   if (packageSize) {
//     totalReceivedPackets++;  // นับจำนวนแพ็กเก็ตที่ได้รับจริง
//     Serial.print("Total Received Packets: ");
//     Serial.println(totalReceivedPackets);
//     Serial.print("Received package: ");

//     String message = "";
//     while (LoRa.available()) {
//       char incomingChar = (char)LoRa.read();
//       Serial.print(incomingChar);
//       message += incomingChar;
//     }
//     Serial.println();

//     // ดึงค่า PackageID
//     int packageID = extractPackageID(message);
//     if (packageID >= 0) {
//       checkLossPackage(packageID);
//     } else {
//       Serial.println("Error: Invalid PackageID.");
//     }

//     // Format the message for display
//     String formattedMessage = formatMessage(message);

//     // Display LoRa message on OLED with PackageID
//     display.clearDisplay();
//     display.setCursor(0, 0);
//     display.println("LoRa Received:");
//     display.print("PackageID: ");
//     display.println(packageID);
//     display.print("Total Packets: ");
//     display.println(totalReceivedPackets); 
//     display.println("-------------------");
//     display.println("MQTT Message:");
//     display.println(formattedMessage); // แสดงข้อความที่ส่งไปยัง MQTT
//     display.display();

//     // Send the formatted message to MQTT
//     sendMessageToMQTT(formattedMessage);
//   }
// }


// หลังจากที่รับแพ็กเก็ตจาก LoRa
int packageSize = LoRa.parsePacket();
if (packageSize) {
    totalReceivedPackets++;  // นับจำนวนแพ็กเก็ตที่ได้รับจริง
    Serial.print("Total Received Packets: ");
    Serial.println(totalReceivedPackets);
    Serial.print("Received package: ");

    String message = "";
    while (LoRa.available()) {
        char incomingChar = (char)LoRa.read();
        Serial.print(incomingChar);
        message += incomingChar;
    }
    Serial.println();

    // ดึงค่า RSSI ของแพ็กเก็ตนี้
    int rssi = LoRa.packetRssi();
    int snr = LoRa.packetSnr();    // ค่าสัญญาณต่อสัญญาณรบกวน (SNR)

    Serial.print("RSSI: ");
    Serial.println(rssi);  // แสดงค่า RSSI ใน Serial Monitor เท่านั้น
    Serial.print("SNR: ");
    Serial.println(snr);   // แสดง SNR

    // ดึงค่า PackageID
    int packageID = extractPackageID(message);
    if (packageID >= 0) {
        checkLossPackage(packageID);
    } else {
        Serial.println("Error: Invalid PackageID.");
    }

    // Format the message for display
    String formattedMessage = formatMessage(message);

    // Display LoRa message on OLED with PackageID (ถ้าคุณยังต้องการแสดงข้อมูลอื่น ๆ บนจอ OLED)
    display.clearDisplay();
    display.setCursor(0, 0);
    // display.println("LoRa Received:");
    display.print("RSSI: ");
    display.println(rssi);
    display.print("SNR: ");
    display.println(snr);
    display.print("PackageID: ");
    display.println(packageID);
    display.print("Total Packets: ");
    display.println(totalReceivedPackets); 
    // display.println("-------------------");
    display.println("MQTT Message:");
    display.println(formattedMessage); // แสดงข้อความที่ส่งไปยัง MQTT
    display.display();

    // Send the formatted message to MQTT
    sendMessageToMQTT(formattedMessage);
  }
}




// ฟังก์ชันดึงค่า PackageID
int extractPackageID(String message) {
  int startIndex = message.indexOf("PackageID=");
  if (startIndex != -1) {
    startIndex += 10; // ข้าม "PackageID="
    int endIndex = message.indexOf("Device_ID =", startIndex);
    if (endIndex == -1) endIndex = message.length();
    String packageIDStr = message.substring(startIndex, endIndex);
    packageIDStr.trim();
    return packageIDStr.toInt();
  }
  return -1; // ไม่พบ PackageID
}

// ฟังก์ชันตรวจสอบ Loss Package
void checkLossPackage(int packageID) {
  if (lastReceivedPackageID != -1) {
    int missedPackets = packageID - (lastReceivedPackageID + 1);
    if (missedPackets > 0) {
      lostPackets += missedPackets;
      Serial.print("Lost Packets: ");
      Serial.println(missedPackets);
    }
  }

  // อัปเดตค่า Package ล่าสุด
  lastReceivedPackageID = packageID;
  receivedPackets++;
}

// ฟังก์ชันคำนวณเปอร์เซ็นต์ Loss Package
float getLossRate() {
  int totalPackets = receivedPackets + lostPackets;
  return (totalPackets > 0) ? (lostPackets * 100.0 / totalPackets) : 0.0;
}

String formatMessage(String message) {
  message.trim();
  int idIndex = message.indexOf("Device_ID =");
  int soilIndex = message.indexOf("Soil =");

  if (idIndex != -1 && soilIndex != -1) {
    String deviceId = message.substring(idIndex, soilIndex);
    String soil = message.substring(soilIndex);

    // เพิ่มค่า Loss Rate ลงในข้อความ
    String lossRate = "Loss Rate = " + String(getLossRate(), 2) + "%";
    return deviceId + "\n" + soil + "\n" + lossRate;
  }

  return message;
}

void reconnect() {
  int retryCount = 0;

  if (skipMQTTConnection) {
    Serial.println("Skipping MQTT connection attempt.");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("MQTT Disconnected");
    display.println("Retry after reboot");
    display.display();
    return;
  }

  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("LoRaReceiverClient", mqttUser, mqttPassword)) {
      Serial.println("connected");
      skipMQTTConnection = false;
      
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println("MQTT Connected");
      display.display();
      break;
    } else {
      retryCount++;
      Serial.print("failed, rc=");
      Serial.println(client.state());
      Serial.println("Retrying in 5 seconds...");
      delay(5000);

      if (retryCount >= 3) {
        Serial.println("MQTT connection failed 3 times. Skipping...");
        skipMQTTConnection = true;

        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("MQTT Failed");
        display.println("No Connection");
        display.println("Check network");
        display.display();
        break;
      }
    }
  }
}

void sendMessageToMQTT(String message) {
  if (!client.connected()) {
    Serial.println("MQTT not connected. Storing message in buffer.");
    messageBuffer.push(message);
    return;
  }

  const char* newMqttTopic = getMqttTopic(message);
  if (client.publish(newMqttTopic, message.c_str())) {
    Serial.println("Message sent successfully.");
  } else {
    Serial.println("Failed to send message. Storing in buffer.");
    messageBuffer.push(message);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received from topic: ");
  Serial.println(topic);
  Serial.print("Payload: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}