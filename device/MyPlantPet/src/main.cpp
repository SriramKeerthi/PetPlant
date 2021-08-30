#include <Arduino.h>
#include <TFT_eSPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Preferences.h>

/* BLE Settings */
#define BLE_SCAN_TIME  30
#define BLE_INTERVAL   500
#define BLE_WINDOW     499

/* Config Strings */
#define CFG_OWNER_UUID "OWNER_UUID"

Preferences pref;

BLEScan* pBLEScan;

// Callback for aadvertised devices
class PetPlantCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        String data = String(advertisedDevice.toString().c_str());
        Serial.println(data);
    }
};

void scanForDevices( void * pvParameters ) {
    while(true) {
        Serial.println("Scanning...");
        pBLEScan->start(BLE_SCAN_TIME, false);
        pBLEScan->clearResults();
    }
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n\n\n*************************\nMYPLANTPET DEVICE STARTED\n*************************\nInitializing...");

    Serial.println("Checking for owner...");
    pref.begin("myplantpet");
    if (pref.isKey(CFG_OWNER_UUID)) {
        Serial.printf("Owner exists: %s\n", pref.getString(CFG_OWNER_UUID).c_str());
    } else {
        Serial.println("No owner found");
    }
    
    Serial.println("Staring BLE Scanning...");
    // Initialize BLE device
    BLEDevice::init("MyPlantPet");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new PetPlantCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(BLE_INTERVAL);
    pBLEScan->setWindow(BLE_WINDOW);

    // Scan for devices in a different core
    xTaskCreatePinnedToCore(
        scanForDevices,
        "scanForDevices",
        10000,
        NULL,
        0,
        NULL,
        1);
}

void loop() {
    delay(100);
}
