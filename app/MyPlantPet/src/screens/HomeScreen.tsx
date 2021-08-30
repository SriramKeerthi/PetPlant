import BeaconBroadcast from '@jaidis/react-native-ibeacon-simulator';
import { Button, Input, Text } from '@ui-kitten/components';
import React, { useEffect, useState } from 'react';
import { Alert, Platform, View } from 'react-native';
import * as AuthSession from 'expo-auth-session';
import jwtDecode from 'jwt-decode';
import credentials from '../config/auth0-configuration';
import getUuid from 'uuid-by-string';

const authorizationEndpoint = "https://myplantpet.eu.auth0.com/authorize";

const useProxy = Platform.select({ web: false, default: true });
const redirectUri = AuthSession.makeRedirectUri({ useProxy });

type JWTToken = {
    aud: string,
    exp: number,
    family_name: string,
    given_name: string,
    iat: number,
    iss: string,
    locale: string,
    name: string,
    nickname: string,
    nonce: string,
    picture: string,
    sub: string,
    updated_at: string
}

const HomeScreen = () => {
    const [uuid, setUuid] = useState('');
    const [request, result, promptAsync] = AuthSession.useAuthRequest(
        {
            redirectUri,
            clientId: credentials.clientId,
            // id_token will return a JWT token
            responseType: 'id_token',
            // retrieve the user's profile
            scopes: ['openid', 'profile'],
            extraParams: {
                // ideally, this will be a random value
                nonce: 'nonce',
            },
        },
        { authorizationEndpoint }
    );

    useEffect(() => {
        if (result) {
            if (result.type === 'error') {
                Alert.alert(
                    'Authentication error',
                    result.params.error_description || 'something went wrong'
                );
                return;
            }
            if (result.type === 'success') {
                // Retrieve the JWT token and decode it
                const jwtToken = result.params.id_token;
                const sub = jwtDecode<JWTToken>(jwtToken).sub;
                setUuid(getUuid(sub));
            }
        }
    }, [result]);


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

    const logout = () => {
        setUuid('');
        reset();
    }

    return (
        <View style={{ padding: 20 }}>
            {!!uuid && <>{beaconState == 0 &&
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
                <Button style={{ marginTop: 20 }} onPress={logout}>Logout</Button></>}
            {!uuid &&
                <>
                    <Text>You're not logged in</Text>
                    <Button onPress={() => promptAsync({ useProxy })}>Login</Button>
                </>}
        </View>
    )
}

export default HomeScreen
