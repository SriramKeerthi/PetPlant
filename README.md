# MyPlantPet
This repository provides the code to build and run your own MyPlantPet.

## Requirements

### Software

For App:

- Expo version 42+ (Requires ExpoKit with `run:android`)
- Android SDK
- ADB

For Device:

- PlatformIO with support for ESP32
- OpenScad for generating STL for planter *

<html><i>* - coming soon</i></html>

### Hardware

For App:

- Android device with BLE

For Device:

- TTGO T-Display (ESP32 device with color screen and free touch pins)
- 3D printer to bring STLs to life
- Soldering iron
- Wires
- Hot glue gun
- Female USB-C port (optional)

## How to use

### App

Configure your Android SDK directory, either as an environment variable, or put the following in `/app/MyPlantPet/android/local.properties`:

```
sdk.dir=/home/path/to/Android/Sdk
```

After that connect a physical Android developer device to your machine and run:

```
adb devices
```

This should start the `adb` daemon, and list your device. If it doesn't, then please fix that before proceeding. Once that works, you can just navigate to the app directory and start the app as follows:

```shell
cd app/MyPlantPet
expo run:android
```

This makes Expo build the APK for your project and install it on your Android device. During this compilation, there might be a failure in `node_modules/@jaidis/react-native-ibeacon-siimulator/android/src/main/java/com/ibeacon/simulator/BeaconBroadcast.java` about `import android.support.annotation.Nullable`. Modify the file as follows:

```java
import androidx.annotation.NonNull;
// import android.support.annotation.Nullable;
```

This should fix it and you should be able to run `expo run:android` again.

### Device

Load up the `device/MyPlantPet` directory in your PlatformIO IDE of choice (I use VSCode). Once PlatformIO loads up and makes dependencies available, you can upload the project to the `TTGO T-Display` device and it should just work.

### Planter

Print the STL (not published yet, in the process of building a custom device design), solder wires to pin 12, 13, 5V, GND as follows:

| ESP32 | Planter   |
| ----- | --------- |
| 12    | Plant     |
| 13    | Plant     |
| 5V    | USB-C 5V  |
| GND   | USB-C GND |

Make sure that the wires for the connections have been routed correctly through the planter and the USB-C plug is placed in the correct position. Once this is done, hotglue all the holes where water and soil might interact with the electronics to keep things safe. Wires are OK since they have plastic insulation, everything else would need to be coated.

That's it, now when the Planter is powered by the USB-C, it should turn on and display a code for you to use with your app.

## Coming soon

- myplant.pet site
- Play Store link
- Apple Store link
- STLs and OpenScad designs for planter

