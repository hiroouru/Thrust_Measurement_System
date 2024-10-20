#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEClient.h>

// Define UUIDs
#define SERVICE_UUID                "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID         "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define ALARM_CHARACTERISTIC_UUID   "8e93f12a-1bf5-4a5e-9e03-735c6d1e7a82"

// Define pins for digitalWrite operations
const int RELAY_PIN1 = 12; // For command 4
const int RELAY_PIN2 = 13; // For command 5
int command = 0;

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
BLECharacteristic *pAlarmCharacteristic = NULL;
BLEClient *pClient;
BLERemoteCharacteristic *pRemoteCharacteristic;
BLERemoteCharacteristic *pRemoteAlarmCharacteristic;
bool deviceConnected = false;
bool doConnect = false;
BLEAdvertisedDevice *myDevice = nullptr;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Client disconnected");
  }
};

class MyClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("Connected to server");
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("Disconnected from server");
    doConnect = true; // Set doConnect to true to reconnect after disconnect
  }
};

void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  String value = std::string((char*)pData, length).c_str();
  Serial.println("Received data: " + value);
  if (deviceConnected) {
    pCharacteristic->setValue(value.c_str());
    pCharacteristic->notify();
  }
}

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallbacks());

  if (!pClient->connect(myDevice)) {
    Serial.println("Failed to connect");
    return false;
  }

  Serial.println(" - Connected to server");

  BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find our service UUID");
    pClient->disconnect();
    return false;
  }

  Serial.println(" - Found our service");

  pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_UUID));
  pRemoteAlarmCharacteristic = pRemoteService->getCharacteristic(BLEUUID(ALARM_CHARACTERISTIC_UUID));
  if (pRemoteCharacteristic == nullptr || pRemoteAlarmCharacteristic == nullptr) {
    Serial.println("Failed to find our characteristic UUIDs");
    pClient->disconnect();
    return false;
  }

  Serial.println(" - Found our characteristics");

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println(" - Registered for notifications");
  }

  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      Serial.print("Found device with matching UUID: ");
      Serial.println(advertisedDevice.toString().c_str());
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

class AlarmCallbackHandler : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue().c_str();
    Serial.print("Received command: ");
    Serial.println(value);
    if (value == "5") {
      digitalWrite(RELAY_PIN1, HIGH);
      delay(100); // Optional delay to ensure the relay is activated
      digitalWrite(RELAY_PIN1, LOW);
      command = 5;
      Serial.println("Triggered RELAY_PIN1 for command 5");
    } else if (value == "4") {
      digitalWrite(RELAY_PIN2, HIGH);
      delay(100); // Optional delay to ensure the relay is activated
      digitalWrite(RELAY_PIN2, LOW);
      command = 4;
      Serial.println("Triggered RELAY_PIN2 for command 4");
    } else if (value == "6") {
      command = 6;
      Serial.println("Received command 6");
    } else if (value == "7") {
      command = 7;
      Serial.println("Received command 7");
    }
  }
};

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PIN1, OUTPUT);
  pinMode(RELAY_PIN2, OUTPUT);
  digitalWrite(RELAY_PIN1, LOW);
  digitalWrite(RELAY_PIN2, LOW);

  BLEDevice::init("ESP32_Relay_Node_1");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->setValue("0");
  pAlarmCharacteristic = pService->createCharacteristic(
                          ALARM_CHARACTERISTIC_UUID,
                          BLECharacteristic::PROPERTY_WRITE |
                          BLECharacteristic::PROPERTY_NOTIFY
                        );
  pAlarmCharacteristic->setCallbacks(new AlarmCallbackHandler());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Connected to BLE Server.");
    } else {
      Serial.println("Failed to connect to BLE Server.");
    }
    doConnect = false;
  }

  // Check for incoming commands from the client and relay them to the server
  if (command == 5) {
    if (pRemoteAlarmCharacteristic != nullptr) {
      pRemoteAlarmCharacteristic->writeValue("5");
      Serial.println("Sent start measurement command to server");
    }
    command = 0;
  } else if (command == 4) {
    if (pRemoteAlarmCharacteristic != nullptr) {
      pRemoteAlarmCharacteristic->writeValue("4");
      Serial.println("Sent digital write command to server");
    }
    command = 0;
  } else if (command == 6) {
    if (pRemoteAlarmCharacteristic != nullptr) {
      pRemoteAlarmCharacteristic->writeValue("6");
      Serial.println("Sent restart measurement command to server");
    }
    command = 0;
  } else if (command == 7) {
    if (pRemoteAlarmCharacteristic != nullptr) {
      pRemoteAlarmCharacteristic->writeValue("7");
      Serial.println("Sent SD stop command to server");
    }
    command = 0;
  } 

  delay(1000);
}
