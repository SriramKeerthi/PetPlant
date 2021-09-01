import { createNativeStackNavigator } from '@react-navigation/native-stack';

import React from 'react';
import HomeScreen from './src/screens/HomeScreen';
import { DefaultTheme, Provider as PaperProvider } from 'react-native-paper';


const theme = {
  ...DefaultTheme,
  roundness: 2,
  colors: {
    ...DefaultTheme.colors,
    primary: '#219446aa',
    accent: '#7ac29766',
    text: "#fff"
  },
};
export default function App() {
  return <PaperProvider theme={theme}>
    < HomeScreen />
  </PaperProvider>


}
