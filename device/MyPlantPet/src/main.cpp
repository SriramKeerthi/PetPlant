#include <Arduino.h>
#include <TFT_eSPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Preferences.h>
#include <Button2.h>

/* Screen Settings */
#define LED_CHANNEL    1
#define LED_PIN        4

/* Input Settings */
#define TOUCH_1        12
#define TOUCH_2        13
#define BUTTON_1       0
#define BUTTON_2       35

/* BLE Settings */
#define BLE_SCAN_TIME  30
#define BLE_INTERVAL   100
#define BLE_WINDOW     99

/* Config Strings */
#define CFG_OWNER_UUID "OWNER_UUID"
#define CFG_CODE       "CODE"

#define _RGB(r, g, b) ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

TFT_eSPI tft = TFT_eSPI(135, 240);
Preferences pref;
BLEScan* pBLEScan;
Button2 button1 = Button2(BUTTON_1);
Button2 button2 = Button2(BUTTON_2);

void clearScreen(int red = 0x4B, int green = 0xAB, int blue = 0x5C) {
    tft.fillScreen(_RGB(red, green, blue));
    tft.setTextColor(TFT_WHITE, _RGB(red, green, blue));
    tft.setCursor(0,0);
}

// Calculates distance from the beacon
double calculateDistance(int txPower, double rssi) {
    if (rssi == 0) {
        return -1.0; // if we cannot determine accuracy, return -1.
    }

    double ratio = rssi*1.0/txPower;
    if (ratio < 1.0) {
        return pow(ratio,10);
    }
    else {
        double accuracy =  (0.89976)*pow(ratio,7.7095) + 0.111;
        return accuracy;
    }
    // return sqrt(pow(10, (txPower - rssi)/10.0));
}

long lastDeviceNear = millis();

// Callback for aadvertised devices
class PetPlantCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        uint8_t *data = (uint8_t*)advertisedDevice.getManufacturerData().data();
        int len = advertisedDevice.getManufacturerData().length();
        String manufacturerData = String(BLEUtils::buildHexData(nullptr, data, len));
        int major = (data[len-5]<<8 | data[len-4]);
        int minor = (data[len-3]<<8 | data[len-2]);
        int txPower = (int8_t)data[len-1];
        if (pref.isKey(CFG_OWNER_UUID)) { // There is an owner already
            if (pref.getString(CFG_OWNER_UUID).equals(manufacturerData.substring(8, 40))) { // And it's communicating
                if (major == pref.getInt(CFG_CODE)) {
                    int rxPower = advertisedDevice.getRSSI();
                    double distance = calculateDistance(txPower, rxPower);
                    Serial.printf("TX: %d RX: %d Distance: %lfm\n", txPower, rxPower, distance);
                    if (distance < 2.0) {
                        lastDeviceNear = millis();
                        if (distance < 0.5) {
                            ledcWrite(LED_CHANNEL, 255);
                        }
                    }
                    if (minor == 0) {
                        clearScreen();
                        tft.setTextSize(4);
                        tft.setCursor(0,42);
                        tft.println("Goodbye :(");
                        ledcWrite(LED_CHANNEL, 255);
                        Serial.printf("Removed owner, restarting...");
                        delay(1000);
                        pref.remove(CFG_OWNER_UUID);
                        ESP.restart();
                    }
                } else {
                    // Code seems wrong, ignore
                }
            }
        } else {
            // Check if advertiser is trying to connect
            if (pref.getInt(CFG_CODE) == major && minor > 0) {
                pref.putString(CFG_OWNER_UUID, manufacturerData.substring(8, 40));
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

int t1Base = 10;
int t2Base = 10;
void buttonClickHandler(Button2& btn) {
    t1Base = touchRead(TOUCH_1);
    t2Base = touchRead(TOUCH_2);
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

    Serial.println("Setting up buttons...");
    button1.setClickHandler(buttonClickHandler);
    button1.setLongClickHandler(buttonClickHandler);
    button1.setDoubleClickHandler(buttonClickHandler);
    button1.setTripleClickHandler(buttonClickHandler);
    button2.setClickHandler(buttonClickHandler);
    button2.setLongClickHandler(buttonClickHandler);
    button2.setDoubleClickHandler(buttonClickHandler);
    button2.setTripleClickHandler(buttonClickHandler);

    Serial.println("Setting up screen...");
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

char touchVal(int base, int val) {
    if (val < base - 5) {
        return 2;
    }
    if (val < base - 2) {
        return 1;
    }
    return ' ' - 175;
}

char isTouched(int base, int val) {
    return val < base - 5;
}

int r = 0x4B;
int g = 0xAB;
int b = 0x5C;
long refreshRate = 100;
long printRate = 500;
long logRate = 200;
char* msg = new char[100];
char hl[] = {253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 0};
long lastPrint = millis();
long lastLog = millis();
void loop() {
    long startTime = millis();
    button1.loop();
    button2.loop();

    int t1 = touchRead(TOUCH_1);
    if (t1 > t1Base)
        t1Base = t1;

    int t2 = touchRead(TOUCH_2);
    if (t2 > t2Base)
        t2Base = t2;

    // Something went wrong with the read, retry method
    if (t1 == 0 || t2 == 0)
        return;

    if (!pref.isKey(CFG_OWNER_UUID)) {
        ledcWrite(LED_CHANNEL, 255);
        tft.setTextSize(7);
        tft.setCursor(42,42);
        tft.printf("%04d", pref.getInt(CFG_CODE));
    } else {
        if (millis() - lastDeviceNear > 5000) {
            ledcWrite(LED_CHANNEL, 0);
        } else {
            sprintf(msg, " MyPlantPet\n%s\n\n T1: %02d/%02d %c\n T2: %02d/%02d %c\n", hl, t1, t1Base, 175 + touchVal(t1Base, t1), t2, t2Base, 175 + touchVal(t2Base, t2));
            if (isTouched(t1Base, t1) && isTouched(t2Base, t2)) {
                r = random(0,255);
                g = random(0,255);
                b = random(0,255);
                clearScreen(r,g,b);
                lastPrint = 0;
            }
            if (millis() - lastLog > logRate) {
                Serial.println(msg);
                lastLog = millis();
            }
            if (millis() - lastPrint > printRate) {
                lastPrint = millis();
                tft.setTextSize(3);
                tft.setCursor(0,0);
                tft.printf(msg);
            }
        }
    }
    delay(max(0ul, refreshRate - (millis() - startTime)));
}
