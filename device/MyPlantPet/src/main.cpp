#include <Arduino.h>
#include <TFT_eSPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Preferences.h>
#include <Button2.h>


/* Screen Settings */
#define LED_CHANNEL        1
#define LED_PIN            4
#define PIXEL_SIZE         15
#define PIXELS_X           TFT_HEIGHT/PIXEL_SIZE
#define PIXELS_Y           TFT_WIDTH/PIXEL_SIZE
#define INACTIVITY_TIMEOUT 2000
#define MAX_BRIGHT         64
#define _RGB(r, g, b)      ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


/* Input Settings */
#define TOUCH_1            12
#define TOUCH_2            13
#define BUTTON_1           0
#define BUTTON_2           35

/* BLE Settings */
#define BLE_SCAN_TIME      30
#define BLE_INTERVAL       100
#define BLE_WINDOW         99
#define CLOSE_DISTANCE     0.8
#define NEARBY_DISTANCE    1.4

/* Config Strings */
#define CFG_OWNER_UUID     "OWNER_UUID"
#define CFG_CODE           "CODE"

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
class MyPlantPetCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        uint8_t *data = (uint8_t*)advertisedDevice.getManufacturerData().data();
        int len = advertisedDevice.getManufacturerData().length();
        int major = (data[len-5]<<8 | data[len-4]);
        int minor = (data[len-3]<<8 | data[len-2]);
        String uuid = String(advertisedDevice.toString().c_str());
        int subIndex = uuid.indexOf("manufacturer data: ") + 19;
        uuid = uuid.substring(subIndex, subIndex + 32);

        if (pref.isKey(CFG_OWNER_UUID)) { // There is an owner already
            if (pref.getString(CFG_OWNER_UUID).equals(uuid)) { // And it's communicating
                if (major == pref.getInt(CFG_CODE)) {
                    int txPower = (int8_t)advertisedDevice.getManufacturerData().data()[len-1];
                    int rxPower = advertisedDevice.getRSSI();
                    double distance = calculateDistance(txPower, rxPower);
                    Serial.printf("TX: %d RX: %d Distance: %lfm\n", txPower, rxPower, distance);
                    if (distance < NEARBY_DISTANCE) {
                        lastDeviceNear = millis();
                        if (distance < CLOSE_DISTANCE) {
                            ledcWrite(LED_CHANNEL, MAX_BRIGHT);
                        }
                    }
                    if (minor == 0) {
                        clearScreen();
                        tft.setTextSize(4);
                        tft.setCursor(0,42);
                        tft.println("Goodbye :(");
                        ledcWrite(LED_CHANNEL, MAX_BRIGHT);
                        Serial.printf("Removed owner, restarting...");
                        delay(1000);
                        pref.remove(CFG_OWNER_UUID);
                        ESP.restart();
                    }
                } else {
                    // Owner Code seems wrong, ignore
                }
            }
        } else {
            // Check if advertiser is trying to connect
            if (pref.getInt(CFG_CODE) == major && minor > 0) {
                pref.putString(CFG_OWNER_UUID, uuid);
                Serial.printf("Registered new owner: %s!\n", pref.getString(CFG_OWNER_UUID).c_str());
                ledcWrite(LED_CHANNEL, 0);
                clearScreen();
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
bool faceMode = true;
int r = 0x4B;
int g = 0xAB;
int b = 0x5C;
int reset = 0;
void buttonClickHandler(Button2& btn) {
    t1Base = touchRead(TOUCH_1);
    t2Base = touchRead(TOUCH_2);
    reset = 0;
}

void tripleButtonClickHandler(Button2& btn) {
    faceMode = !faceMode;
    reset = reset + 1;
    if (reset == 3) {
        pref.remove(CFG_OWNER_UUID);
        ESP.restart();
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
        Serial.println("No owner found, adding a code");
        pref.putInt(CFG_CODE, random(10000)); // Generate a random number between 0 - 10000
    }

    Serial.println("Setting up buttons...");
    button1.setClickHandler(buttonClickHandler);
    button1.setLongClickHandler(buttonClickHandler);
    button1.setDoubleClickHandler(buttonClickHandler);
    button1.setTripleClickHandler(tripleButtonClickHandler);
    button2.setClickHandler(buttonClickHandler);
    button2.setLongClickHandler(buttonClickHandler);
    button2.setDoubleClickHandler(buttonClickHandler);
    button2.setTripleClickHandler(tripleButtonClickHandler);

    Serial.println("Setting up screen...");
    ledcSetup(LED_CHANNEL, 5000, 8);
    ledcAttachPin(LED_PIN, LED_CHANNEL);
    tft.init();
    tft.setRotation(3);
    tft.setTextSize(7);
    clearScreen();

    Serial.println("Staring BLE Scanning...");
    // Initialize BLE device
    BLEDevice::init("MyPlantPet");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyPlantPetCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(BLE_INTERVAL);
    pBLEScan->setWindow(BLE_WINDOW);

    // Scan for devices in a different core
    xTaskCreatePinnedToCore(
        scanForDevices,
        "scanForDevices",
        64000,
        NULL,
        1,
        NULL,
        1);
}


int lastFace = 0;
int lastFrame = 0;
String faces[4][2] = {
    {
        "...................................................................X..X..X..X.......XX....XX....................................................",
        "...................................................................................X..X..X..X.......XX....XX...................................."
    },
    {
        ".........................................................................XXXX......X..X..X..X.......XX....XX....................................",
        "...................................................................XXXX............X..X..X..X.......XX....XX...................................."
    },
    {
        "....................................................................XX....XX.......X..X..X..X...................................................",
        "....................................................XX....XX.......X..X..X..X..................................................................."
    },
    {
        "....................................XX....XX.......X..X..X..X......X..X..X..X......X..X..X..X.......XX....XX....................................",
        "....................................XX....XX.......X..X..X..X......X..X..X..X......X..X..X..X.......XX....XX...................................."
    }
};

void drawBitmap(String bitmap) {
    for (int x = 0; x < PIXELS_Y; x++) {
        for(int y = 0; y < PIXELS_X; y++) {
            uint32_t col = bitmap.charAt(x*PIXELS_X+y) == '.' ? _RGB(r,g,b) : TFT_WHITE;
            if (tft.readPixel(y*PIXEL_SIZE, x*PIXEL_SIZE) != col) {
                tft.fillRect(y*PIXEL_SIZE, x*PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE, col);
            }
        }
    }
}

void drawFace(int t1, int t2) {
    int newFace = 0;
    int newFrame = (lastFrame + 1) % 2;
    if (t1 == 2 && t2 == 2) {
        newFace = 3;
    }
    else if (t1 == 2 || t2 == 2 || (t1 == 1 && t2 == 1)) {
        newFace = 2;
    }
    else if (t1 == 1 || t2 == 1) {
        newFace = 1;
    }
    else {
        newFace = 0;
    }
    if (newFace != lastFace) {
        newFrame = 0;
    }
    drawBitmap(faces[newFace][newFrame]);
    lastFace = newFace;
    lastFrame = newFrame;
}

int touchVal(int base, int val) {
    if (val < base - 5) {
        return 2;
    }
    if (val < base - 2) {
        return 1;
    }
    return 0;
}

char levelToChar(int level) {
    return level == 0 ? ' ' : (level == 1 ? 176 : 177);
}

long refreshRate = 50;
long printRate = 500;
long logRate = 1000;
char* msg = new char[100];
char hl[] = {253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 253, 0};
long lastChange = millis();
long lastPrint = millis();
long lastLog = millis();
void loop() {
    button1.loop();
    button2.loop();

    // Calculate touch values
    int t1 = touchRead(TOUCH_1);
    if (t1 > t1Base)
        t1Base = t1;

    int t2 = touchRead(TOUCH_2);
    if (t2 > t2Base)
        t2Base = t2;

    // Something went wrong with the read, retry method
    if (t1 == 0 || t2 == 0)
        return;

    // Ease the difference in baseline to measured value if there's a large but fixed change
    if(millis()-lastChange > 5000) {
        lastChange = millis();
        t1Base = max(t1Base - 1, t1);
        t2Base = max(t2Base - 1, t2);
    }

    if (!pref.isKey(CFG_OWNER_UUID)) { // Device not bonded yet
        ledcWrite(LED_CHANNEL, MAX_BRIGHT);
        tft.setTextSize(7);
        tft.setCursor(42,42);
        tft.printf("%04d", pref.getInt(CFG_CODE));
    } else {
        if (millis() - lastDeviceNear > INACTIVITY_TIMEOUT) { // Device inactive, turn off
            ledcWrite(LED_CHANNEL, 0);
        } else {
            int t1Level = touchVal(t1Base, t1);
            int t2Level = touchVal(t2Base, t2);
            if (t1Level > 0 || t2Level > 0) {
                lastDeviceNear = millis(); // If it's touched, keep screen alive
            }

            sprintf(msg, " MyPlantPet\n%s\n\n T1: %02d/%02d %c\n T2: %02d/%02d %c\n", hl, t1, t1Base, levelToChar(t1Level), t2, t2Base, levelToChar(t2Level));
            if (t1Level == 2 && t2Level == 2) {
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
                if (faceMode) {
                    drawFace(t1Level, t2Level);
                }
                else {
                    tft.setTextSize(3);
                    tft.setCursor(0,0);
                    tft.printf(msg);
                }
            }
        }
    }
    delay(refreshRate);
}
