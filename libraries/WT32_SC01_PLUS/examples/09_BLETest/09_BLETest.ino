#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>

static constexpr uint32_t BLE_SCAN_SECONDS = 6;
static constexpr const char *BLE_DEVICE_NAME = "WT32-SC01-PLUS-BLE";
static constexpr const char *SERVICE_UUID = "7b9d0001-5d9f-4c8a-a6e8-4f7f6b320001";
static constexpr const char *CHAR_UUID = "7b9d0002-5d9f-4c8a-a6e8-4f7f6b320001";
static constexpr const char *PING_TEXT = "WT32-BLE-PING";
static constexpr const char *PONG_TEXT = "WT32-BLE-PONG";

namespace {

BLEServer *gServer = nullptr;
BLECharacteristic *gCharacteristic = nullptr;
volatile bool gConnected = false;
volatile bool gWritePass = false;

class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
 public:
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    Serial.printf("[ADV] RSSI=%4d dBm  ADDR=%s  NAME=%s\n",
                  advertisedDevice.getRSSI(),
                  advertisedDevice.getAddress().toString().c_str(),
                  advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "[unnamed]");
  }
};

class ServerCallbacks : public BLEServerCallbacks {
 public:
  void onConnect(BLEServer *server) override {
    (void)server;
    gConnected = true;
    Serial.println("[PASS] BLE client connected");
  }

  void onDisconnect(BLEServer *server) override {
    gConnected = false;
    Serial.println("[INFO] BLE client disconnected; advertising restarted");
    server->startAdvertising();
  }
};

class CharacteristicCallbacks : public BLECharacteristicCallbacks {
 public:
  void onWrite(BLECharacteristic *characteristic) override {
    String value = characteristic->getValue();
    Serial.printf("[GATT] RX: %s\n", value.c_str());

    if (value == PING_TEXT) {
      characteristic->setValue(PONG_TEXT);
      characteristic->notify();
      gWritePass = true;
      Serial.println("[PASS] GATT write matched WT32-BLE-PING");
      Serial.println("[PASS] Characteristic updated/notified with WT32-BLE-PONG");
    } else {
      Serial.println("[INFO] Write received, but expected WT32-BLE-PING");
    }
  }
};

bool runScanStage() {
  Serial.printf("[SCAN] Active BLE scan for %lu second(s)...\n", BLE_SCAN_SECONDS);

  BLEScan *scan = BLEDevice::getScan();
  scan->setAdvertisedDeviceCallbacks(new ScanCallbacks());
  scan->setActiveScan(true);
  scan->setInterval(100);
  scan->setWindow(99);

  BLEScanResults *results = scan->start(BLE_SCAN_SECONDS, false);
  if (results == nullptr) {
    Serial.println("[FAIL] BLE scan returned null results");
    return false;
  }

  const int count = results->getCount();
  Serial.printf("[PASS] BLE scan completed: %d device(s) found\n", count);
  scan->clearResults();
  return count > 0;
}

void startGattStage() {
  Serial.println("------------------------------------------------------------");
  Serial.println("[GATT] Starting BLE peripheral / GATT validation stage");

  gServer = BLEDevice::createServer();
  gServer->setCallbacks(new ServerCallbacks());
  gServer->advertiseOnDisconnect(true);

  BLEService *service = gServer->createService(SERVICE_UUID);
  gCharacteristic = service->createCharacteristic(
      CHAR_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);

  gCharacteristic->setCallbacks(new CharacteristicCallbacks());
  gCharacteristic->setValue("WT32-BLE-READY");
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.printf("[PASS] Advertising started as: %s\n", BLE_DEVICE_NAME);
  Serial.printf("[INFO] Service UUID       : %s\n", SERVICE_UUID);
  Serial.printf("[INFO] Characteristic UUID: %s\n", CHAR_UUID);
  Serial.println("[ACTION] On a phone BLE scanner, connect to WT32-SC01-PLUS-BLE.");
  Serial.println("[ACTION] Read characteristic: expect WT32-BLE-READY.");
  Serial.println("[ACTION] Write ASCII: WT32-BLE-PING.");
  Serial.println("[ACTION] Read/notification should return WT32-BLE-PONG.");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WT32-SC01-PLUS Arduino BSP / 09_BLETest");
  Serial.println(" BLE scan + advertising + GATT read/write/notify validation");
  Serial.println("============================================================");

  if (!BLEDevice::init(BLE_DEVICE_NAME)) {
    Serial.println("[FAIL] BLEDevice::init failed");
    return;
  }

  if (!runScanStage()) {
    Serial.println("RESULT = FAIL (BLE scan)");
    return;
  }

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" BLE RADIO / SCAN PHYSICAL PASS CANDIDATE");
  Serial.println(" Advertising/GATT validation: PENDING USER ACTION");
  Serial.println("============================================================");

  startGattStage();
}

void loop() {
  static bool finalPrinted = false;

  if (gConnected && gWritePass && !finalPrinted) {
    Serial.println();
    Serial.println("============================================================");
    Serial.println(" BLE TEST PHYSICAL PASS CANDIDATE");
    Serial.println(" Scan + advertise + connect + GATT read/write/notify passed.");
    Serial.println("============================================================");
    finalPrinted = true;
  }

  delay(200);
}
