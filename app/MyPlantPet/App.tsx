import { light as lightTheme, mapping } from '@eva-design/eva';
import { NavigationContainer } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import { ApplicationProvider, IconRegistry } from '@ui-kitten/components';
import { EvaIconsPack } from '@ui-kitten/eva-icons';
import React from 'react';
import HomeScreen from './src/screens/HomeScreen';

const Stack = createNativeStackNavigator();

export default function App() {
  return <>
    <IconRegistry icons={EvaIconsPack} />
    <ApplicationProvider mapping={mapping}
      theme={lightTheme}>
      <NavigationContainer>
        <Stack.Navigator>
          <Stack.Screen name='My Plant Pet!' component={HomeScreen} />
        </Stack.Navigator>
      </NavigationContainer>
    </ApplicationProvider>
  </>;
}
