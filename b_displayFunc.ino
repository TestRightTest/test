void initializeDisplay() {
  
  gotoScreen(0);

  // Initialize all channels from 1 to 16
  for (int i = 1; i <= 16; i++) {
    stopChannel(i);
  }
  
  serialNum = EEPROM.read(64) * 255 + EEPROM.read(65);
  deviceID = "MBS" + String(serialNum + 100000).substring(1);

  readFromEEPROM(ssid, 66, 50);
  Serial.print("SSID: "); Serial.println(ssid);
  sendText(ssidAdd, ssid, 50);

  readFromEEPROM(password, 116, 50);
  Serial.print("Password: "); Serial.println(password);
  sendText(passAdd, password, 50);

  Serial.println(deviceID);
  sendText(deviceIDadd, deviceID.c_str(), 50);
  sendText(helplineadd, helpline.c_str(), 50);
  sendText(rotationMsg, "Delay Rotation(60sec)", 30);

  sendText(temperatureAdd, String(temperature, 1).c_str(), 4); //sendTemp

}

void displayReset() {
  byte resetCommand[] = {0x5A, 0xA5, 0x07, 0x82, 0x00, 0x04, 0x55, 0xAA, 0x5A, 0xA5};
  dwin.write(resetCommand, sizeof(resetCommand));
}



//buffer to store data when wifi is not connected
//const int numChannels = 16;
//
//struct SensorData {
//  String sampleName;
//  int channelId;
//  int firstReading;
//  int white1;
//  int white2;
//  long startTime;
//  long endTime;
//  String date;
//  int mbrt;
//  int endProgress;
//  int progress;
//};
//
//SensorData sensorDataArray[numChannels];
//
//void clearSensorData(int channelIndex) {
//  // Clear the stored sensor data for the specified channel
//  sensorDataArray[channelIndex] = SensorData();
//}




void checkWiFiConnection() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisWifi >= intervalWifi) {
    if (WiFi.status() == WL_CONNECTED) {
        hideData(SPwifiIcon); // Connected
        Serial.println("Wifi Connected!");
        updateConnectionStatus();
        wifiConnected = true;
    } else {
        showData(SPwifiIcon, wifiIcon); // Disconnected
        Serial.println("Wifi Not Connected!");
        connectToWiFi();
        firebaseInitilize();
    }
    previousMillisWifi = currentMillis;
  }
}


void touch(int x, int y) {
  // Convert X and Y values to hexadecimal
  byte x_high = (byte)(x >> 8); // High byte of X
  byte x_low = (byte)(x & 0xFF); // Low byte of X
  byte y_high = (byte)(y >> 8); // High byte of Y
  byte y_low = (byte)(y & 0xFF); // Low byte of Y

  // Define the data array
  byte data[] = {0x5A, 0xA5, 0x0B, 0x82, 0x00, 0xD4, 0x5A, 0xA5, 0x00, 0x04, x_high, x_low, y_high, y_low};
  // Calculate the length of the data array
  int dataLength = sizeof(data);

  // Send each byte of the data array
  for (int i = 0; i < dataLength; i++) {
    dwin.write(data[i]);
  }
}



void writeChannelNum(){
  sendText(0x2030, String(selectedChannel).c_str(), 2);
}


void  updateAllElapsedTime() {
  rotateFlag = false;  // Reset rotateFlag at the beginning

  for (int i = 0; i < 16; i++) {
    if (isRunning[i]) {
      updateElapsedTime(i + 1);
      rotateFlag = true;  // Set rotateFlag if any channel is running
    }
  }

  if (!rotateFlag) {
    // If rotateFlag is off, update previousRotationMillis
    previousRotationMillis = millis();
  }
}

void updateProgress(int channel) {
  
  sendNum(progressArray[channel-1], progressValues[channel-1]); //send only when  changed. dont send continuously!

    //send to firebase
  
    // progress value to Firebase
    String uidStr = String(auth.token.uid.c_str());
    String rootPath = "/users/" + uidStr + "/progress/" + uidStr + " channel " + String(channel) + "/";
    
    // Attempt to push the data
    if (Firebase.RTDB.push(&fbdo, rootPath.c_str(), String(progressValues[channel]))) {
        Serial.println("Progress value sent to Firebase successfully");
    } else {
        Serial.println("Failed to send progress value to Firebase");
    }
}


void updateElapsedTime(int channel) {
  if (channelStartTimes[channel - 1] != 0) {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - channelStartTimes[channel - 1]) / (1000 * 60); // Convert milliseconds to minutes
    mbrt[channel - 1] = elapsedTime;
      elapsedTime = mbrt[channel - 1];
    // Format elapsed time as a string (e.g., "02:04")
    String elapsedTimeStr = String(elapsedTime / 60, DEC);
    elapsedTimeStr += ":";
    elapsedTimeStr += String(elapsedTime % 60, DEC);

    // Pad with leading zeros if needed
    if (elapsedTime / 60 < 10) {
      elapsedTimeStr = "0" + elapsedTimeStr;
    }
    if (elapsedTime % 60 < 10) {
      elapsedTimeStr = elapsedTimeStr.substring(0, 3) + "0" + elapsedTimeStr.substring(3);
    }

    // Check if the elapsed time has changed
    if (elapsedTimeStr != lastElapsedTime[channel - 1]) {
      // Update the time display
      sendText(timeArray[channel - 1], elapsedTimeStr.c_str(), 5);
      lastElapsedTime[channel - 1] = elapsedTimeStr;
    }
  }
}


void sendText(uint16_t vpAddress, const char* text, int textLength)
{
  eraseText(vpAddress, textLength); // Erase display based on the specified memory size

  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(strlen(text) + 3); // Length: Write command(1), vp address (2);
  dwin.write(0x82); // Write command
  dwin.write(highByte(vpAddress)); // Write address
  dwin.write(lowByte(vpAddress)); // Write Address
  dwin.write(text, strlen(text));
}

void sendTemp() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillisTemp >= intervalTemp) {
    previousMillisTemp = currentMillis;
//    temperature = temperature + (tempShift-37.6); //deg C
    sendText(temperatureAdd, String(temperature, 1).c_str(), 4);
    Serial.print("Display Temp value");
    Serial.println(temperature);
    Serial.print("settemp value");
    Serial.println(tempShift);
    Serial.print("set_temp value");
    Serial.println(set_temp);
  }

}


void sendNum(uint16_t vpAddress, int value)
{
//  eraseText(vpAddress, textLength); // Erase display based on the specified memory size

  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x05); // Length: Write command(1), vp address (2), number(2);
  dwin.write(0x82); // Write command
  dwin.write(highByte(vpAddress)); // Write address
  dwin.write(lowByte(vpAddress)); // Write Address
  dwin.write(highByte(value)); // highbyte of number
  dwin.write(lowByte(value)); // lowbyte of number
}


void beep(uint16_t durationMs){

  durationMs = durationMs / 8;
  
  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x05); // Length: Write command(1), vp address (2), number(2);
  dwin.write(0x82); // Write command
  
  dwin.write(0x00);
  dwin.write(0xA0);
  dwin.write(highByte(durationMs)); // Length: Write command(1), vp address (2), number(2);
  dwin.write(lowByte(durationMs)); // Write command
}

void eraseText(int vpAddress, int textLength)
{
  //clear text field on display - it an absolutely ugly solution but we can 
  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(textLength + 3); //Lengh 3: Write,address low and high
  dwin.write(0x82); //write command
  dwin.write(highByte(vpAddress));//write address
  dwin.write(lowByte(vpAddress)); //Write Address
 
  for (int i = 0; i < textLength; i++)
  {
    dwin.write((byte)0x20); //write message (0x20 is code for space)  
  }
}

void hideData(uint16_t SPaddress) {
  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x05);
  dwin.write(0x82);
  dwin.write(highByte(SPaddress));
  dwin.write(lowByte(SPaddress));
  dwin.write(0xFF);
  dwin.write(0x00);
}

void showData(uint16_t SPaddress, uint16_t varaddress) {
  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x05);
  dwin.write(0x82);
  dwin.write(highByte(SPaddress));
  dwin.write(lowByte(SPaddress));
  dwin.write(highByte(varaddress));
  dwin.write(lowByte(varaddress));
}

void gotoScreen(uint16_t pageNumber) {
  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x07);
  dwin.write(0x82);
  dwin.write(0x00);
  dwin.write(0x84);
  dwin.write(0x5A);
  dwin.write(0x01);
  dwin.write(highByte(pageNumber));
  dwin.write(lowByte(pageNumber));

}

void stopChannel(int i) {

  Serial.print("Stopped channel ");Serial.println(i);

  if (isRunning[selectedChannel - 1] == true && isCompleted[i-1] == false) 
  {
    Serial.println("Forced Stop");
//    send to firebase
  updateTime();
  String uidStr = String(auth.token.uid.c_str());
  sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(i) + "/endProgress" , int(progressValues[i-1]));
  sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(i) + "/stopTime" , String(currentTime));

  }

  isCompleted[i-1] = false;
  isRunning [i-1] = false;
  
  // Assuming ChannelID is between 1 and 16
  if (i < 1 || i > 16) {
    // Handle invalid ChannelID as needed
    return;
  }


  // Reset elapsed time-related variables and arrays
  channelStartTimes[i - 1] = 0;
  lastElapsedTime[i - 1] = ""; // Assign an empty string

  // Set initial values
  char startText[12];
  sprintf(startText, "Start  %02d", i);
  sendText(startArray[i-1], startText, 9);

  sendText(channelIdArray[i-1], "", 2); // Empty string for ChannelID
  sendText(nameArray[i-1], "", 16); // Empty string for name
  sendText(timeArray[i-1], "", 5); // Empty string for time
  sendNum(progressArray[i-1], 0); // Initial progress value, you can adjust if needed
  hideData(SPprogressArray[i-1]);
  hideData(SPprogressbar[i-1]);

}


void completedChannel(int i){

  Serial.print("Completed Channel ");Serial.println(i);

  isRunning[i-1] = false;
  isCompleted[i-1] = true;
  
  changeColor(SPprogressbar[i-1], 6, 0x04e5); // Change foreground
  changeColor(SPprogressbar[i-1], 7, 0x04e5); // Change background 
  changeColor(SPprogressArray[i-1], 3, 0xffff); // Change Progress%
  changeColor(SPtimeArray[i-1], 3, 0xffff); // Change Time
  hideData(SPprogressArray[i-1]);
  updateTime();
  beep(500);
  String uidStr = String(auth.token.uid.c_str());
  sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(i) + "/mbrt" , String(mbrt[i-1]));
  sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(i) + "/endProgress" , int(progressValues[i-1]));
  Serial.print("end progress value:");
  Serial.print(progressValues[i-1]);
  sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(i) + "/stopTime" , String(currentTime));
  }

void startChannel(int i, const String& receivedMessage) {

  //send start time and sample name to firebase

  isRunning[selectedChannel - 1] = true;
  isCompleted[i-1] = false;

  Serial.print("Started Channel ");Serial.println(i);
  
  // Assuming ChannelID is between 1 and 16
  if (i < 1 || i > 16) {
    return;
  }

  // Set values for startchannel
  sendText(startArray[i-1], "", 9); // Empty string for startchannel
  
  char channelIDText[3];
  sprintf(channelIDText, "%02d", i);
  sendText(channelIdArray[i-1], channelIDText, 2); // Set channel ID based on the argument

  sendText(timeArray[i-1], "00:00", 5);

  // Check if the channel is already started
  if (channelStartTimes[i - 1] == 0) {
    // Update start time for the channel
    channelStartTimes[i - 1] = millis();
  }

  sendNum(progressArray[i-1], 0); // Fixed initial progress value, you can adjust if needed

  // Show data at progressAddress and hiddenAddress
  showData(SPprogressArray[i-1], progressArray[i-1]);

  showData(SPprogressbar[i-1], progressArray[i-1]);

//change start colors
  changeColor(SPprogressbar[i-1], 6, 0xE73C);  // Change foreground 
  changeColor(SPprogressbar[i-1], 7, 0xBEDF);  // Change background
  changeColor(SPprogressArray[i-1], 3, 0x20E4); // Change Progress%
  changeColor(SPtimeArray[i-1], 3, 0x20E4); // Change Time

  // Update nameAddress with the received message
  sendText(nameArray[i-1], receivedMessage.c_str(), 16);
  
  takeFirstReading(selectedChannel - 1);

  //send start channel command to firebase to get initial time
  updateTime();
  updateDate();
  String uidStr = String(auth.token.uid.c_str());
  Serial.println("Channel " + String(i) + " data sent");
  sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(i) + "/SampleName" , String(receivedMessage));
  sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(i) + "/channelId" , String(i));
  sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(i) + "/startTime" , String(currentTime ));
  sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(i) + "/date" , String(currentDate));
}

void changeColor(uint16_t mainAddress, int offset, uint16_t color) {
  uint16_t vpAddress = mainAddress + offset;

  dwin.write(0x5A);
  dwin.write(0xA5);
  dwin.write(0x05);
  dwin.write(0x82);
  dwin.write(highByte(vpAddress));
  dwin.write(lowByte(vpAddress));
  dwin.write(highByte(color));
  dwin.write(lowByte(color));
}

String stringConvertedTime(int minutes) {
  // Calculate hours and minutes
  int hours = minutes / 60;
  int remainingMinutes = minutes % 60;

  // Format as "HH:MM"
  char formattedTime[6];
  sprintf(formattedTime, "%02d:%02d", hours, remainingMinutes);

  return String(formattedTime);
}

// Declare a variable to track the current screen
int currentScreen = 0;

void displayRotationScreen() {

  if(rotationEnableFlag){
  
    unsigned long currentMillis = millis();
  
    if(timeExtension) rotationIntervalMinutes = 1;
    else rotationIntervalMinutes = rotationIntervalEeprom;
    
    if (currentMillis - previousRotationMillis >= rotationIntervalMinutes * 60 * 1000) {
      sendText(rotationMsg, "Delay Rotation(60sec)", 30);
      gotoScreen(0x0006); // Rotation screen
      sendNum(countdownAdd, timerRotation);
      currentScreen = 6; // Update the current screen
      previousRotationMillis = currentMillis;
    }
  
    // Execute the following part only when on the rotation screen (0x0006)
    if (currentScreen == 6) {
      // Countdown every 1 second
      if (currentMillis - previousSecondMillis >= 1000 && timerRotation > 0) {
        timerRotation--; // Decrement the countdown value every 1 second
        sendNum(countdownAdd, timerRotation); // Update the countdown value on the screen
        previousSecondMillis = currentMillis; // Update the previousSecondMillis here
      }
  
      // If the countdown reaches 0, execute rotate_ function
      if (timerRotation == 0) {
        sendText(rotationMsg, "Rotating Now..", 30);
        rotate_();
        gotoScreen(0x00); // Home screen
        rotationIntervalMinutes = rotationIntervalEeprom;
        timeExtension = false;
        timerRotation = 10; // Reset the countdown for the next interval
  //      previousSecondMillis = currentMillis; // Reset the previousSecondMillis for the next countdown
        currentScreen = 0; // Update the current screen to home screen
      }
    }
  }
  else{
    Serial.println("rotation is off");
    }
}

void saveToEEPROM(const char* data, int address, int maxLength) {
  for (int i = 0; i < maxLength; ++i) {
    char c = data[i];
    EEPROM.write(address + i, c);
    if (c == '\0') break; // Stop writing at null terminator
  }
  EEPROM.commit(); // Commit changes to EEPROM
}

void readFromEEPROM(char* buffer, int address, int maxLength) {
  for (int i = 0; i < maxLength; ++i) {
    char c = EEPROM.read(address + i);
    buffer[i] = c;
    if (c == '\0') break; // Stop reading at null terminator
  }
}

int readFrame() {
  // A frame must have atleast 3 bytes.
  if (dwin.available() < 3) {
      return 0;  
  }
  if (dwin.read() != 0x5A) {
     return 0;
  }
  if (dwin.read() != 0xA5) {
     return 0;
  }
  
  int bytesToRead = dwin.read();
  // Test buffer overflow
  if ((bytesToRead + 3) > sizeof(incomingData)) {
    return 0;
  }

  int retry = 0;
  while ((dwin.available() < bytesToRead) and (retry < 10)) {
    delay(2);
    retry++;
  }
  
  if (retry >= 10) {
    return 0;
  }
    
  memset(incomingData, 0, sizeof(incomingData));
  incomingData[0] = 0x5A;
  incomingData[1] = 0xA5;
  incomingData[2] = (char)bytesToRead;
  for (int i = 0; i < bytesToRead; i++) {
    incomingData[i + 3] = (char)dwin.read();
  }
  return bytesToRead + 3;
}

void printFrame(int bytesRead) {
  Serial.print("Received data: ");
  for (int j = 0; j < bytesRead; j++) {
    Serial.print(incomingData[j], HEX);
    Serial.print(" ");
  }
  Serial.println();
}


void fetchText() {
  int bytesRead = readFrame();
  if (bytesRead == 0) {
    return;
  }
    
  printFrame(bytesRead);

  // ProcessFrame now
  if (incomingData[3] == (byte)0x83) {
    Serial.println("0x83 Received ");
    uint16_t buttonAddress = (incomingData[4] << 8) | incomingData[5];
    int k = 7;
    String receivedMessage = "";

    while (incomingData[k] != 0xFF) {
      receivedMessage += incomingData[k];
      k++;
    }

    // Print the received message
    Serial.print("Received message: ");
    Serial.println(receivedMessage);

    // Check if the received address is from 0x5000 to 0x50F0
    if (buttonAddress >= 0x5000 && buttonAddress <= 0x50F0) {
      // Update selectedChannel with the channel number
      selectedChannel = (buttonAddress - 0x5000) / 0x10 + 1;
      
      //If a started channel is repressed, take it to Stop channel screen
      if (!isRunning[selectedChannel-1] && !isCompleted[selectedChannel-1]){gotoScreen(0x0001);touch(100,75);}
      else if (isRunning[selectedChannel-1] && !isCompleted[selectedChannel-1]) gotoScreen(0x0003); // 0003 is stop channel screen
      else if (isCompleted[selectedChannel-1]) stopChannel(selectedChannel);
      else{}; 
      
      Serial.print("Selected Channel ");
      Serial.println(selectedChannel);
     
      writeChannelNum();
    }

    if (buttonAddress == 0x6000) {       
      startChannel(selectedChannel, receivedMessage);
    }

   

    if (buttonAddress == ssidAdd) {
      receivedMessage.toCharArray(ssid, 50);
      Serial.print("Wifi SSID: ");
      Serial.println(ssid);
      saveToEEPROM(ssid, 66, 50); //65 is the memory start address, 50 is total length
    
      connectToWiFi();
    }
  
    if (buttonAddress == passAdd) {
      receivedMessage.toCharArray(password, 50);
      Serial.print("Wifi Password: ");
      Serial.println(password);
      saveToEEPROM(password, 116, 50); //116 is the memory start address, 50 is total length
      
      connectToWiFi();
    }

    if (buttonAddress == 0x2020) {
      stopChannel(selectedChannel);
    }

    if (buttonAddress == 0x1960) {
      Serial.print("White readings for channel: ");
      Serial.println(selectedChannel);
      takeWhite(selectedChannel - 1);
      sendText(0x1980, "White calibration complete!", 30 );
    }

    if (buttonAddress == 0x1970) {
      for (int i = 0; i < numChannels; i++) {
        takeWhite(i);
      }
      sendText(0x1980, "White All calibration complete!", 30 );
    }

    if (buttonAddress == 0x2310) {
       previousRotationMillis = millis();
       timeExtension = true;//make it 1
       timerRotation = 10;
       currentScreen = 0;
       Serial.println("extended rotation");
    }
    
    // Empty the arrays to avoid garbage
    memset(incomingData, 0, sizeof(incomingData));
  }
}
