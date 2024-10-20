#include <BLEDevice.h>


const int ignition = 12; // For command 4
const int start_measure = 14; // For command 5
const int restart_measure = 27; // For command 6
const int SD_save = 26; // For command 7
const int sent_message_indicator = 15; //sent message indicator

int command=0; //command

// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");
static BLEUUID alarmCharUUID("8e93f12a-1bf5-4a5e-9e03-735c6d1e7a82");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = true; // Initially set to true to start scanning
static BLERemoteCharacteristic *pRemoteCharacteristic = nullptr;
static BLERemoteCharacteristic *pRemoteAlarmCharacteristic = nullptr;
static BLEAdvertisedDevice *myDevice = nullptr;

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  // Convert the received data to a string and then to a float
  String value = std::string((char*)pData, length).c_str();
  float numericValue = value.toFloat();
  Serial.println(numericValue);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {
    connected = true;
    Serial.println("Connected to server");
  }

  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("Disconnected from server");
    doScan = true; // Set doScan to true to restart scanning after disconnect
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remote BLE Server.
  if (!pClient->connect(myDevice)) {
    Serial.println("Failed to connect");
    return false;
  }
  Serial.println(" - Connected to server");

  pClient->setMTU(517);  // set client to request maximum MTU from server (default is 23 otherwise)

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  pRemoteAlarmCharacteristic = pRemoteService->getCharacteristic(alarmCharUUID);
  if (pRemoteCharacteristic == nullptr || pRemoteAlarmCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    String value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }

  connected = true;
  return true;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      Serial.print("Found device with matching UUID: ");
      Serial.println(advertisedDevice.toString().c_str());
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  BLEDevice::init("");
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);

  Serial.println("Starting BLE scan...");
  pBLEScan->start(30, false); // Start scanning for 30 seconds
  Serial.println("BLE scan started for 30 seconds.");

  pinMode(ignition, INPUT_PULLUP);
  pinMode(start_measure, INPUT_PULLUP);
  pinMode(restart_measure, INPUT_PULLUP);
  pinMode(SD_save, INPUT_PULLUP);

  
}

void loop() {

  if(digitalRead(ignition) == LOW){
    command = 4;
  } else if(digitalRead(start_measure) == LOW){
    command = 5;
  } else if(digitalRead(restart_measure) == LOW){
    command = 6;
  } else if(digitalRead(SD_save) == LOW){
    command = 7;
  } else command = 0;







  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect) {
    Serial.println("Attempting to connect to the server...");
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected && pRemoteCharacteristic != nullptr) { // Ensure the characteristic is valid
    // The notify callback will handle printing the received values
  } else if (doScan) {
    Serial.println("Restarting BLE scan...");
    BLEDevice::getScan()->start(30);  // Restart scan for 30 seconds
    doScan = false; // Reset doScan to avoid continuous restarts
  }

  
  
    
    if (command == 5) {
      // Send start measurement trigger
      if (pRemoteAlarmCharacteristic != nullptr) {
        pRemoteAlarmCharacteristic->writeValue("5");
        Serial.println("Sent start measurement trigger");
        digitalWrite(sent_message_indicator, HIGH);
        delay(500);
        digitalWrite(sent_message_indicator, LOW);
        command = 0;
      }
    } else if (command == 4) {
      // Send digital write trigger
      if (pRemoteAlarmCharacteristic != nullptr) {
        pRemoteAlarmCharacteristic->writeValue("4");
        Serial.println("Sent digital write trigger");
        digitalWrite(sent_message_indicator, HIGH);
        delay(500);
        digitalWrite(sent_message_indicator, LOW);
        command = 0;
      }
    }
    else if (command == 6) {
      // Send digital write trigger
      if (pRemoteAlarmCharacteristic != nullptr) {
        pRemoteAlarmCharacteristic->writeValue("6");
        Serial.println("Sent restart measurement trigger");
        digitalWrite(sent_message_indicator, HIGH);
        delay(500);
        digitalWrite(sent_message_indicator, LOW);
        command = 0;
      }
    } 
    else if (command == 7) {
      // Send digital write trigger
      if (pRemoteAlarmCharacteristic != nullptr) {
        pRemoteAlarmCharacteristic->writeValue("7");
        Serial.println("Sent SD stop trigger");
        digitalWrite(sent_message_indicator, HIGH);
        delay(500);
        digitalWrite(sent_message_indicator, LOW);
        command = 0;
      }
    }
  

  delay(1000);  // Delay a second between loops.
}
