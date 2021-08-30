#include <Arduino.h>
#include <TFT_eSPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

/* BLE Settings */
#define BLE_SCAN_TIME  30
#define BLE_INTERVAL   500
#define BLE_WINDOW     499

BLEScan* pBLEScan;

// Callback for aadvertised devices
class PetPlantCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String data = String(advertisedDevice.toString().c_str());
        Serial.println(data);
    }
};

void scanForDevices() {
    while(true) {
        Serial.println("Scanning...");
        pBLEScan->start(BLE_SCAN_TIME, false);
        pBLEScan->clearResults();
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n\n\n***********************\nPETPLANT DEVICE STARTED\n***********************\nInitializing...");

    Serial.println("Staring BLE Scanning...");
    // Initialize BLE device
    BLEDevice::init("Pet Plant");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new PetPlantCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(500);
    pBLEScan->setWindow(499);
}

void loop() {
    scanForDevices();
}
