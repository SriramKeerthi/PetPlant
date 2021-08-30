import BeaconBroadcast from '@jaidis/react-native-ibeacon-simulator';
import { Button, Input, Text } from '@ui-kitten/components';
import React, { useEffect, useState } from 'react';
import { View } from 'react-native';

const HomeScreen = () => {
    const uuid = 'e59a019e-d09a-41e2-9719-2a8f11983467';
    const [code, setCode] = useState<string>('');
    const [major, setMajor] = useState<Number | undefined>(undefined);
    const [minor, setMinor] = useState<Number | undefined>(undefined);
    const [beaconState, setBeaconState] = useState(0);

    useEffect(() => {
        BeaconBroadcast.checkTransmissionSupported()
            .then(() => {
                BeaconBroadcast.stopAdvertisingBeacon()
                if (major != undefined && minor != undefined) {
                    console.log("Starting beacon!", uuid, major, minor);
                    BeaconBroadcast.startAdvertisingBeaconWithString(uuid, 'Hello', major, minor)
                }
            })
            .catch((e: any) => {
                /* handle return errors */
                // - NOT_SUPPORTED_MIN_SDK
                // - NOT_SUPPORTED_BLE
                // - DEPRECATED_NOT_SUPPORTED_MULTIPLE_ADVERTISEMENTS
                // - NOT_SUPPORTED_CANNOT_GET_ADVERTISER
                // - NOT_SUPPORTED_CANNOT_GET_ADVERTISER_MULTIPLE_ADVERTISEMENTS
                console.log(e);
            })
        return () => BeaconBroadcast.stopAdvertisingBeacon();
    }, [beaconState]);

    const connect = () => {
        if (code) {
            setBeaconState(1);
            setMajor(parseInt(code));
            setMinor(1);
        }
    }

    const disconnect = () => {
        setBeaconState(2);
        setMinor(0);
    }

    const reset = () => {
        setBeaconState(0);
        setMajor(undefined);
        setMinor(undefined);
        setCode('');
    }

    return (
        <View style={{ padding: 20 }}>
            {beaconState == 0 &&
                <>
                    <Input size='large' placeholder="Enter your plant's code" onSubmitEditing={connect} onChangeText={setCode} value={code} />
                    <Button disabled={code.length != 4} onPress={connect}>Start Petting!</Button>
                </>
            }
            {beaconState == 1 &&
                <>
                    <Text style={{ alignSelf: "center" }}>Ready to pet your plant!</Text>
                    <Button onPress={disconnect}>Disconnect</Button>
                </>
            }
            {
                beaconState == 2 &&
                <>
                    <Text style={{ alignSelf: "center" }}>Bring your phone near the plant and click done when the plant acknowledges!</Text>
                    <Button onPress={reset}>Done!</Button>
                </>
            }
        </View>
    )
}

export default HomeScreen
