import BeaconBroadcast from '@jaidis/react-native-ibeacon-simulator';
import React, { useEffect, useState } from 'react';
import { Alert, Platform, View, ImageBackground, StyleSheet, } from 'react-native';
import * as AuthSession from 'expo-auth-session';
import jwtDecode from 'jwt-decode';
import credentials from '../config/auth0-configuration';
import getUuid from 'uuid-by-string';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { Asset } from "expo-asset";
import { Button, Text, } from 'react-native-paper';
const authorizationEndpoint = "https://myplantpet.eu.auth0.com/authorize";
import { StatusBar } from 'expo-status-bar';
import SmoothPinCodeInput from "react-native-smooth-pincode-input";
const useProxy = Platform.select({ web: false, default: true });
const redirectUri = AuthSession.makeRedirectUri({ useProxy });
import { BlurView } from 'expo-blur';

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
    const [code, setCode] = useState<string>('');
    const [major, setMajor] = useState<Number | undefined>(undefined);
    const [minor, setMinor] = useState<Number | undefined>(undefined);
    const [beaconState, setBeaconState] = useState(0);

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
                AsyncStorage.getItem('code', (error, result) => {
                    console.log('Got code', result);
                    if (result) {
                        setCode(result);
                        connect();
                    }
                });
            }
        }
    }, [result]);


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
            AsyncStorage.setItem('code', code);
            setMajor(parseInt(code));
            setMinor(1);
            setBeaconState(1);
            console.log('Connecting');
        }
    }

    const disconnect = () => {
        setBeaconState(2);
        setMinor(0);
    }

    const reset = () => {
        setMajor(undefined);
        setMinor(undefined);
        setBeaconState(0);
        setCode('');
        AsyncStorage.setItem('code', '');
    }

    const logout = () => {
        setUuid('');
        setBeaconState(0);
        setMajor(undefined);
        setMinor(undefined);
    }

    console.log({ code })
    return (

        <View style={styles.container}>

            <ImageBackground source={{
                uri: Asset.fromModule(require("../../assets/1.jpg")).uri,

            }}
                blurRadius={1.5}

                style={styles.image}>
                <View style={styles.header}><Text style={styles.headerText}>My Plant Pet</Text></View>

                <View style={styles.content}>{!!uuid && <>{
                    beaconState == 0 &&
                <>
                        <SmoothPinCodeInput
                            value={code}
                            onTextChange={setCode}
                            codeLength={4}

                            restrictToNumbers
                            containerStyle={styles.pinInput}
                            cellStyle={styles.cell}
                            textStyle={styles.cellText}
                            cellStyleFocused={styles.cellFocused}
                            textStyleFocused={styles.cellFocusedText}
                        // onBackspace={this._focusePrevInput}
                        />


                        {code.length === 4 && <Button onPress={connect} mode="contained">Start Petting!</Button>}
                        <Button onPress={logout}>Logout</Button>
                </>
            }
                {beaconState == 1 &&
                    <>
                        <Text style={styles.contentText}>You are good to go! Pet away!</Text>
                        <View style={styles.horizontal}><Button onPress={disconnect}>Disconnect</Button><Button onPress={logout}>Logout</Button></View>
                    </>
                }
                {
                    beaconState == 2 &&
                    <>
                            <Text style={styles.contentText}>Bring your phone near the plant and click done when the plant acknowledges!</Text>
                            <View style={styles.horizontal}><Button onPress={reset}>Done!</Button><Button onPress={logout}>Logout</Button></View>
                    </>
                    }</>}
                    {!uuid &&
                        <>
                            <Text style={styles.contentText}>Login to interact with your plant!</Text>
                            <Button mode='contained' onPress={() => promptAsync({ useProxy })} style={styles.mainButton} labelStyle={styles.mainButtonContent}>Login</Button>
                        </>}
                </View>
            </ImageBackground>
            <StatusBar style="light" />
        </View>

    )
}

const styles = StyleSheet.create({
    container: { flex: 1, justifyContent: "space-between", },
    header: { alignContent: "center", alignSelf: "center", marginBottom: "auto", marginTop: 100, },
    headerText: { fontSize: 36 },
    image: {
        flex: 1,
        resizeMode: 'cover',
        justifyContent: 'center',

    },
    content: {
        alignContent: "center",
        alignItems: "center",
        justifyContent: "center",
        backgroundColor: "#000000cc",
        padding: 70,
    },
    contentText: { fontSize: 24, marginTop: 40, marginBottom: 40, textAlign: "center", color: "white" },
    pinInput: {
        alignSelf: "center",
        marginBottom: 100,
    },
    cell: {
        height: 100,
        borderColor: "white",
        borderBottomWidth: 2,
        color: "white"
    },
    cellText: {
        color: "white"
    },

    cellFocused: {
        borderBottomWidth: 1,
        height: 100,
        borderColor: "green"
    },
    cellFocusedText: { color: "red" },

    mainButton: {
        marginTop: 10,
        marginBottom: 10,
        width: "100%"
    },

    mainButtonContent: {
        color: 'white',
        fontSize: 18
    },
    horizontal: {
        flexDirection: 'row'
        ,
        alignItems: "center"
    }


});

export default HomeScreen
