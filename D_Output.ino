#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <Adafruit_SH110X.h>
#include <splash.h>
#include <LoRa.h>
#include <Wire.h>

// ค่าคงที่สำหรับ BLE
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BUTTON_PIN 17

// ค่าคงที่สำหรับ LoRa
#define LORA_SS 5
#define LORA_RST 14
#define LORA_MOSI 23

// ค่าคงที่สำหรับเซ็นเซอร์และจอ OLED
#define SOIL_SENSOR_PIN 4
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

// ตัวแปรสำหรับ BLE
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool bleEnabled = false;
bool lastButtonState = HIGH;
String deviceId = "UNKNOWN";
String deviceName; 
Preferences preferences;

// ตัวแปรสำหรับเซ็นเซอร์
int soil_value, percent;

// ตัวแปรสำหรับจอ OLED
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long packetCounter = 0; // เพิ่มบรรทัดนี้


void displayStatus(String message) {
    display.clearDisplay(); 
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);

    // แสดงสถานะ BLE
    display.println("Device_ID: " + deviceId);
    display.println(bleEnabled ? "BLE: ON" : "BLE: OFF"); 

    // แสดงสถานะการเชื่อมต่อ BLE
    display.println(deviceConnected ? "BLE: Connected" : "BLE: Disconnected"); 

    // แสดงค่าจากเซ็นเซอร์
    display.println("Soil: " + String(percent) + "%"); 

    // แสดงข้อมูลแพ็กเกจที่ส่งไปยัง Gateway
    display.println("LoRa Packet:");
    display.print("PackageID: ");
    display.println(packetCounter - 1); // แสดง PackageID ล่าสุด
    display.print("Device_ID: ");
    display.println(deviceId);
    display.print("Soil: ");
    display.println(percent);

    // แสดงข้อความเพิ่มเติม (ถ้ามี)
    if (message != "") {
      display.println(); // เว้นบรรทัด
      display.println(message); 
    }

    display.display();
}
// Callbacks สำหรับ BLE
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("อุปกรณ์เชื่อมต่อแล้ว");
        displayStatus("BLE Connected");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("อุปกรณ์ยกเลิกการเชื่อมต่อแล้ว");
        displayStatus("BLE Disconnected");
        // เพิ่มการรีสตาร์ทโปรแกรมเมื่ออุปกรณ์ยกเลิกการเชื่อมต่อ
        ESP.restart(); 
    }
};

class MyCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        uint8_t* data = pCharacteristic->getData();
        size_t len = pCharacteristic->getValue().length();
        
        if (len > 0) {
            String command = "";
            for (int i = 0; i < len; i++) {
                command += (char)data[i];
            }
            
            Serial.println("ได้รับคำสั่ง: " + command);
            
            if (command.startsWith("SET_ID:")) {
                String newId = command.substring(7);
                newId.trim();
                
                preferences.begin("device", false);
                preferences.putString("id", newId);
                preferences.end();
                
                deviceId = newId;
                Serial.println("อัพเดท Device ID เป็น: " + deviceId);
                
                String response = "ID_SET:" + deviceId;
                pCharacteristic->setValue((uint8_t*)response.c_str(), response.length());
                pCharacteristic->notify();
            } else if (command == "RESTART") { // เพิ่มคำสั่ง RESTART
                Serial.println("กำลังรีสตาร์ท ESP32...");
                pCharacteristic->setValue("RESTARTING");
                pCharacteristic->notify();
                delay(1000); 
                ESP.restart();
            }
        }
    }
};

// ฟังก์ชันเริ่มต้น BLE
void startBLE() {
    deviceConnected = false;
    
    if (BLEDevice::getInitialized()) {
        BLEDevice::deinit(true);
        delay(500);
    }
    
    BLEDevice::init(deviceName.c_str());
    
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    pCharacteristic = pService->createCharacteristic(
                                                    CHARACTERISTIC_UUID,
                                                    BLECharacteristic::PROPERTY_READ |
                                                    BLECharacteristic::PROPERTY_WRITE |
                                                    BLECharacteristic::PROPERTY_NOTIFY
                                                    );
    
    pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());
    
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    BLEDevice::startAdvertising();
    
    Serial.println("เริ่มต้น Advertising แล้ว!");
    displayStatus("BLE Started\nID: " + deviceId);
    bleEnabled = true;
}

// ฟังก์ชันหยุด BLE
void stopBLE() {
    if (bleEnabled) {
        deviceConnected = false;
        bleEnabled = false;
        BLEDevice::deinit(true);
        Serial.println("ปิด BLE แล้ว");
        displayStatus("BLE Stopped");
        delay(500);
    }
}

// ฟังก์ชันอ่านค่าเซ็นเซอร์
// ฟังก์ชันอ่านค่าเซ็นเซอร์
void readSensor() {
    soil_value = analogRead(SOIL_SENSOR_PIN);
    percent = map(soil_value, 0, 4095, 100, 0);
    
    // แสดงค่าใน Serial Monitor
    Serial.print("Soil Value: ");
    Serial.print(soil_value);
    Serial.print("\tPercent: ");
    Serial.print(percent);
    Serial.println("%");

    // แสดงค่าในจอ OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);
    display.println("Soil Sensor:");
    display.print("Value: ");
    display.println(soil_value);
    display.print("Percent: ");
    display.println(percent);
    display.display();
}

// ฟังก์ชันส่งข้อมูลผ่าน LoRa
void sendLoRaData() {
    if (LoRa.beginPacket()) {
        LoRa.print("PackageID=");
        LoRa.print(packetCounter);
        LoRa.print("Device_ID = ");  // เพิ่ม Device ID
        LoRa.print(deviceId);
        LoRa.print("Soil = ");
        LoRa.println(percent);
        packetCounter++; // เพิ่มค่า PackageID ทุกครั้งที่ส่งข้อมูล
        LoRa.endPacket();
        Serial.println("ส่งข้อมูล LoRa สำเร็จ");
        Serial.print("PackageID: ");
        Serial.println(packetCounter - 1); // แสดง PackageID ที่ส่งไป
        Serial.print("Device_ID: ");
        Serial.println(deviceId);
        Serial.print("Soil: ");
        Serial.println(percent);
        displayStatus("LoRa Sent:\nDevice_ID: " + deviceId + "\nSoil: " + String(percent) + "%");
    } else {
        Serial.println("การส่งข้อมูล LoRa ล้มเหลว");
        displayStatus("LoRa Send Failed");
    }
}

void setup() {
    Serial.begin(9600);

// อ่านค่า Device ID
    preferences.begin("device", true);
    deviceId = preferences.getString("id", "UNKNOWN");
    preferences.end();

    // กำหนดค่า deviceName
    deviceName = "ESP32-" + deviceId; 

    
    // ตั้งค่าพิน - เพิ่ม debug message
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Serial.println("Button PIN initialized");
    
    // เริ่มต้นจอ OLED
    if (!display.begin(OLED_ADDRESS, true)) {
        Serial.println(F("SH1106 initialization failed"));
        for(;;);
    }
    displayStatus("Initializing...");
    
    // เริ่มต้น LoRa
    LoRa.setPins(LORA_SS, LORA_RST, LORA_MOSI);
    while (!LoRa.begin(915E6)) {
        Serial.println("wait lora");
        delay(500);
    }
    LoRa.setSyncWord(0xF3);
    Serial.println("LoRa เริ่มต้นสำเร็จ");
    displayStatus("LoRa Ready");

    // ตั้งค่ากำลังส่งของ LoRa                                                          
    LoRa.setTxPower(14); // กำลังส่ง 17 dBm
    Serial.print("LoRa Tx Power Set To: ");
    Serial.println(14); // แสดงค่าที่ตั้งไว้ใน Serial Monitor

    Serial.println("LoRa เริ่มต้นสำเร็จ");
    displayStatus("LoRa Ready");
    
    delay(2000);
    Serial.println("กดปุ่มเพื่อเปิด/ปิด BLE");
    displayStatus("Press Button\nfor BLE");
}

void loop() {
    bool currentButtonState = digitalRead(BUTTON_PIN);

    // Debug button state
    Serial.print("Button State: ");
    Serial.println(currentButtonState);

    // ตรวจจับการกดปุ่ม - เปลี่ยนเงื่อนไขการตรวจจับ
    if (currentButtonState == LOW && lastButtonState == HIGH) {  // เมื่อปุ่มถูกกด
        delay(50); // debounce
        if (digitalRead(BUTTON_PIN) == LOW) { // ตรวจสอบอีกครั้งว่ายังกดอยู่
            Serial.println("Button Pressed!");
            if (!bleEnabled) {
                Serial.println("Starting BLE...");
                startBLE();
            } else {
                Serial.println("Stopping BLE...");
                stopBLE();
            }
        }
    }
    lastButtonState = currentButtonState;

    // อ่านค่าเซ็นเซอร์
    readSensor();

    displayStatus(""); // แสดงข้อมูลทั้งหมดบนจอ OLED


    // ส่งข้อมูลผ่าน BLE ถ้าเชื่อมต่ออยู่
    if (bleEnabled && deviceConnected) {
        String message = "ID:" + deviceId + ",Soil:" + String(percent);
        pCharacteristic->setValue((uint8_t*)message.c_str(), message.length());
        pCharacteristic->notify();
    }

    // ส่งข้อมูลผ่าน LoRa
    sendLoRaData();

    delay(5000); // ลดเวลารอลงเพื่อให้ปุ่มตอบสนองได้ดีขึ้น
}