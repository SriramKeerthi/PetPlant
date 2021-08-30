#include <Arduino.h>
#include <TFT_eSPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Preferences.h>

/* Screen Settings */
#define LED_CHANNEL 1
#define LED_PIN     4

/* BLE Settings */
#define BLE_SCAN_TIME  30
#define BLE_INTERVAL   500
#define BLE_WINDOW     499

/* Config Strings */
#define CFG_OWNER_UUID "OWNER_UUID"
#define CFG_CODE       "CODE"

#define _RGB(r, g, b) ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

TFT_eSPI tft = TFT_eSPI(135, 240);
Preferences pref;
BLEScan* pBLEScan;

void clearScreen(int red = 0x4B, int green = 0xAB, int blue = 0x5C) {
    tft.fillScreen(_RGB(red, green, blue));
    tft.setTextColor(TFT_WHITE, _RGB(red, green, blue));
    tft.setCursor(0,0);
}

// Callback for aadvertised devices
class PetPlantCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        uint8_t *data = (uint8_t*)advertisedDevice.getManufacturerData().data();
        int len = advertisedDevice.getManufacturerData().length();
        String adv = String(BLEUtils::buildHexData(nullptr, data, len));
        int major = (data[len-5]<<8 | data[len-4]);
        int minor = (data[len-3]<<8 | data[len-2]);
        if (pref.isKey(CFG_OWNER_UUID)) { // There is an owner already
            if (pref.getString(CFG_OWNER_UUID).equals(adv.substring(8, 40))) { // And it's communicating
                if (major == pref.getInt(CFG_CODE)) {
                    if (minor == 0) {
                        clearScreen();
                        tft.setTextSize(5);
                        tft.setCursor(0,0);
                        tft.println("Goodbye :(");
                        ledcWrite(LED_CHANNEL, 255);
                        pref.remove(CFG_OWNER_UUID);
                        Serial.printf("Removed owner, restarting...");
                        delay(1000);
                        ESP.restart();
                    } else {
                        // Do something useful
                    }
                } else {
                    // Code seems wrong, ignore
                }
            }
        } else {
            // Check if advertiser is trying to connect
            if (pref.getInt(CFG_CODE) == major && minor > 0) {
                pref.putString(CFG_OWNER_UUID, adv.substring(8, 40));
                Serial.printf("Registered new owner: %s!\n", pref.getString(CFG_OWNER_UUID).c_str());
                ledcWrite(LED_CHANNEL, 0);
            }
        }
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
    pref.remove(CFG_OWNER_UUID);
    if (pref.isKey(CFG_OWNER_UUID)) {
        Serial.printf("Owner exists: %s\n", pref.getString(CFG_OWNER_UUID).c_str());
    } else {
        Serial.println("No owner found, adding a code");
        pref.putInt(CFG_CODE, random(10000)); // Generate a random number between 0 - 10000
    }

    // Set up screen
    ledcSetup(LED_CHANNEL, 5000, 8);
    ledcAttachPin(LED_PIN, LED_CHANNEL);
    tft.init();
    tft.setRotation(1);
    tft.setTextSize(7);
    clearScreen();

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
    if (!pref.isKey(CFG_OWNER_UUID)) {
        ledcWrite(LED_CHANNEL, 255);
        tft.setCursor(0,0);
        tft.printf("%04d", pref.getInt(CFG_CODE));
    } else {
        // Do something else
    }
    delay(100);
}
