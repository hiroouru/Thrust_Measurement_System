#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <HX711_ADC.h>
#include <SimpleKalmanFilter.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Define pins for HX711 and SD card
const int HX711_dout = 32;
const int HX711_sck = 33;
const int chipSelect = 5;
const int SD_stop = 34;

// Define pins for LEDs and stop trigger
const int LED1 = 14;
const int LED2 = 27;
const int LED3 = 26;
const int LED4 = 25;
const int LED_DATA_AVAILABLE = 13;
const int IGNITION_PIN = 12; // Pin to be set high on command

float LoadCellAverageSum = 0;
float Average;
int checkingtimes = 0;

HX711_ADC LoadCell(HX711_dout, HX711_sck);

boolean decreasing = false;
float maxthrust = 0;
float thtime = 0, tottime = 0;
boolean increasing = false;
boolean AverageCheck = false;
File dataFile;

float loadCellData = 0;
float timeCount = 0;

boolean sdBeginSuccess = true;
boolean deviceConnected = false;
boolean startMeasurement = false;
boolean restartmeasurement = false;
boolean SD_save = false;

// Initialize the Kalman filter
SimpleKalmanFilter kalmanFilter(1, 1, 0.01);

// Define BLE characteristics
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLECharacteristic *pAlarmCharacteristic = NULL;
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define ALARM_CHARACTERISTIC_UUID "8e93f12a-1bf5-4a5e-9e03-735c6d1e7a82"


void startMeasurementProcess() {
  thtime = 0;
  tottime = 0;
  timeCount = 0;
  AverageCheck = false; // Reset for new measurement
  LoadCellAverageSum = 0;
  checkingtimes = 0;
  restartmeasurement = false;
  SD_save = false; 
  digitalWrite(SD_stop, LOW);

  if (sdBeginSuccess) {
    dataFile = SD.open("/Data.txt", FILE_WRITE);
    if (!dataFile) {
      Serial.println("ERROR: could not open Data.txt for writing.");
      sdBeginSuccess = false;
      digitalWrite(LED1, HIGH);
      return;
    } else {
      Serial.println("Data.txt opened successfully for writing.");
    }

    Serial.println("Starting data collection...");

    delay(1000);
    digitalWrite(LED4, LOW);
    digitalWrite(LED1, HIGH);
    delay(2000);

    while (deviceConnected) {

      if(SD_save){
        sdBeginSuccess = false;
        dataFile.close();
        digitalWrite(SD_stop, HIGH);
        break;
      }

      if(restartmeasurement){
        break;
      }
      LoadCell.update();

      if (!AverageCheck) {
        for (int i = 0; i < 30000; i++) {
          float rawData = LoadCell.getData();
          if (rawData > 0) {
            LoadCellAverageSum += rawData;
            checkingtimes += 1;
          }
        }
        Average = LoadCellAverageSum / checkingtimes;
        digitalWrite(LED1, LOW);
        AverageCheck = true;
      }

      float rawLoadCellData = LoadCell.getData();
      loadCellData = kalmanFilter.updateEstimate(rawLoadCellData - Average - 15);

      if (loadCellData < 0) {
        loadCellData = 0;
      }

      timeCount += 12.5;
      delay(11);
      dataFile.println(loadCellData);
      Serial.println(loadCellData);

      digitalWrite(LED_DATA_AVAILABLE, HIGH);
      delay(50);
      digitalWrite(LED_DATA_AVAILABLE, LOW);

      pCharacteristic->setValue(String(loadCellData).c_str());
      pCharacteristic->notify();

      if (loadCellData >= maxthrust) {
        maxthrust = loadCellData;
      }

      if (!increasing) {
        thtime = timeCount;
        if (loadCellData >= 50) {
          increasing = true;
          thtime = timeCount;
        }
      } else if (increasing) {
        if (loadCellData >= 100) {
          decreasing = true;
        }
        if (decreasing) {
          if (loadCellData < 5) {
            dataFile.close();
            tottime = timeCount - thtime;
            Serial.print("Total time: ");
            Serial.println(tottime);
            sdBeginSuccess = false;
            Serial.println("SD stopped.");
            digitalWrite(LED2, LOW);
            digitalWrite(LED3, HIGH);
            break;
          }
        }
      }
    }

    dataFile = SD.open("/Data.txt");
    if (dataFile) {
      Serial.println("Reading from Data.txt:");
      while (dataFile.available()) {
        Serial.write(dataFile.read());
      }
      dataFile.close();
      Serial.println("Done reading from Data.txt.");
    } else {
      Serial.println("Error opening Data.txt for reading");
    }
  } else {
    Serial.println("SD initialization failed. Cannot start measurement.");
  }
}

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    digitalWrite(LED2, HIGH);
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    digitalWrite(LED2, LOW);
    Serial.println("Client disconnected");
  }
};

class AlarmCallbackHandler : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue().c_str();
    Serial.print("Received command from relay: ");
    Serial.println(value);
    
    if (value == "6") {
      restartmeasurement = true;
      startMeasurement = true;
      
      digitalWrite(LED3, HIGH);
      Serial.println("Starting measurements...");
    } else if (value == "5") {
      
      startMeasurement = true;
      Serial.println("Received command to start measurement at first.");
    } else if (value == "4") {
      
      digitalWrite(IGNITION_PIN, HIGH);
      delay(1000); // Delay for 100 milliseconds
      digitalWrite(IGNITION_PIN, LOW);
      Serial.println("Trigger alarm executed. Pin set high then low.");
    } else if (value == "7") {
      
      SD_save = true;
      Serial.println("Received command to stop measurement and save SD.");
    } else {
      Serial.println("Unknown command received");
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(10);
  LoadCell.begin();

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(LED_DATA_AVAILABLE, OUTPUT);
  pinMode(IGNITION_PIN, OUTPUT);
  digitalWrite(IGNITION_PIN, LOW); // Ensure pin is low at startup
  pinMode(chipSelect, OUTPUT);
  pinMode(SD_stop, OUTPUT);
  digitalWrite(SD_stop, LOW);

  // LoadCell start and tare
  boolean _tare = true;
  unsigned long stabilizingtime = 10000;
  LoadCell.start(stabilizingtime, _tare);
  LoadCell.setCalFactor(15.67);

  // Initialize SD card
  Serial.println("Initializing SD card...");
  if (SD.begin(chipSelect)) {
    Serial.println("SD card initialized successfully.");
    sdBeginSuccess = true;
    digitalWrite(LED1, LOW);
    digitalWrite(LED4, HIGH);
    if (SD.exists("/Data.txt")) {
      if (SD.remove("/Data.txt")) {
        Serial.println("Old Data.txt removed successfully.");
      } else {
        Serial.println("ERROR: Failed to remove old Data.txt.");
      }
    }
  } else {
    sdBeginSuccess = false;
    Serial.println("ERROR: SD card failed to initialize.");
    digitalWrite(LED1, HIGH);
  }

  // Initialize BLE
  BLEDevice::init("ESP32_Thrust_Server");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->setValue("0");
  pAlarmCharacteristic = pService->createCharacteristic(
                          ALARM_CHARACTERISTIC_UUID,
                          BLECharacteristic::PROPERTY_WRITE
                        );
  pAlarmCharacteristic->setCallbacks(new AlarmCallbackHandler());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
  Serial.println("BLE server is now advertising.");
}


void loop() {
  if (deviceConnected) {
    digitalWrite(LED2, HIGH);
  } else {
    digitalWrite(LED2, LOW);
  }

  if (startMeasurement && deviceConnected) {
    digitalWrite(LED3, HIGH);
    startMeasurement = false; // Reset the flag to allow remeasurement
    startMeasurementProcess();
  } else {
    digitalWrite(LED3, LOW);
    delay(1000);
  }
}
