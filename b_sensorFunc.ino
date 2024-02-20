void sensorSetup(){ 
  pinMode(LIMIT_PIN, INPUT_PULLUP);
  pinMode(STEP_PIN,OUTPUT); 
  pinMode(DIR_PIN,OUTPUT);
  pinMode(MOT_DRIVER_PIN,OUTPUT);


  t0 = 298.15;  //Temperature   t0 from datasheet, conversion from Celsius to kelvin
  pinMode(HEATER_PIN, OUTPUT);

  //rescale  timer according to dt
  SetTime = SetTime * TicksPerMS;

  ////////////////////////////////////////////////  Motor Driver Control  /////////////////////////////////////////////////
    
  digitalWrite(MOT_DRIVER_PIN, HIGH);

  ////////////////////////////////////////////////  MUX  /////////////////////////////////////////////////
  
  pinMode(muxS0, OUTPUT); 
  pinMode(muxS1, OUTPUT); 
  pinMode(muxS2, OUTPUT); 
  pinMode(muxS3, OUTPUT); 
  pinMode(COM1,OUTPUT);
  pinMode(COM2,OUTPUT);

  digitalWrite(muxS0, LOW);
  digitalWrite(muxS1, LOW);
  digitalWrite(muxS2, LOW);
  digitalWrite(muxS3, LOW);
  digitalWrite(COM1, LOW);
  digitalWrite(COM2, LOW);
 

  //////////////////////////////////////////////////    PWM    //////////////////////////////////////////////////////////
  
  ledcSetup(pwmChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(HEATER_PIN, pwmChannel);
  
  // Initialize Wire instances
  I2Cone.begin(SDA_PHOTODIODE, SCL_PHOTODIODE, 100000);
  I2Ctwo.begin(SDA_TEMP, SCL_TEMP, 100000);

  // Initialize each ADS1115 on the main I2C bus


  if (!adsTemp.begin(0x48, &I2Ctwo)) {
  Serial.println("Failed to initialize adsTemp.");
  while (1);
  }
  
  if (!ads1.begin(0x49, &I2Cone)) {
    Serial.println("Failed to initialize ads1.");
    while (1);
  }

  if (!ads2.begin(0x4B, &I2Cone)) {
    Serial.println("Failed to initialize ads2.");
    while (1);
  }

  if (!ads3.begin(0x48, &I2Cone)) {
    Serial.println("Failed to initialize ads3.");
    while (1);
  }

  if (!ads4.begin(0x4A, &I2Cone)) {
    Serial.println("Failed to initialize ads4.");
    while (1);
  }

  delay(500);

  // Set gain for all ADS1115 devices
  ads1.setGain(GAIN_ONE);
  ads2.setGain(GAIN_ONE);
  ads3.setGain(GAIN_ONE);
  ads4.setGain(GAIN_ONE);
  adsTemp.setGain(GAIN_ONE);
  
  printwhite(); //print white values on serial

//  tempShift = EEPROM.read(168)/10;
//  rotationIntervalEeprom = EEPROM.read(166)*256 + EEPROM.read(167) ; 
  
  
  readFromEEPROM(userEmail, 169, 50);
  Serial.print("User Email: ");
  Serial.println(userEmail);
  
  readFromEEPROM(userPassword, 219, 50);
  Serial.print("User Password: ");
  Serial.println(userPassword);
  
  connectToWiFi();
  
 }

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  firebaseInitilize();

}


void controlTemp(){
  temperature = adsTemp.readADC_SingleEnded(0); //adc value
  temperature = static_cast<float>(Thermistor(static_cast<double>(temperature)));//adc to Celcius
  temperature = temperature;// - tempShift; //tempShift = 0 by default
  Control_PID(temperature);
  }
  
  
void rotate_()
{
      Serial.println("motor rotate");
      if (digitalRead(LIMIT_PIN) == HIGH) motorRotate(0,16*55);// If limitswitch is not pressed then go towards the limitswitch, else don't
      motorRotate(1,16*55);//away from limitswitch
      motorRotate(0,16*55);//towards the limitswitch
      digitalWrite(MOT_DRIVER_PIN, HIGH); //motor deactivated
}


void motorRotate(int dir, int stepcount)
{
  digitalWrite(DIR_PIN,dir);
  digitalWrite(MOT_DRIVER_PIN, LOW); //motor activated
  // Makes 200 pulses for making one full cycle rotations
  for(int x = 0; x < stepcount; x++) {

        if (digitalRead(LIMIT_PIN) == LOW && dir == 0) //LOW when pressed
        { 
          digitalWrite(MOT_DRIVER_PIN, HIGH);//motor deactivated
          return; //false when unpressed i.e. keep continuing
        }
   
      digitalWrite(STEP_PIN,HIGH); 
      delay(3);    // more delay, slower speed and higher torque
      digitalWrite(STEP_PIN,LOW); 
      delay(3);    
  }
  delay(1000); // One second delay
}

int sensorRead(int ledID) { 
  ledID = channelarrangement[ledID]; //remapping the Led
  
  int channel = ledID / 2;
  int adsID = channel / 4 + 1; 
  int adsChannel = channel % 4;
  int sensorValue;
  
  Serial.print("ledID" + String(ledID) + " ");
  Serial.print("channel" + String(channel) + " ");
  Serial.print(" adschannel" + String(adsChannel) + " ");
  Serial.print(" adsID" + String(adsID) + ":  ");
  
  ledPinSelect(ledID, HIGH);

  switch (adsID) {
    case 1: sensorValue = ads1.readADC_SingleEnded(adsChannel); break;
    case 2: sensorValue = ads2.readADC_SingleEnded(adsChannel); break;
    case 3: sensorValue = ads3.readADC_SingleEnded(adsChannel); break;
    case 4: sensorValue = ads4.readADC_SingleEnded(adsChannel); break;
    default: sensorValue = 0; break;  // Default value in case of unexpected adsID
  }

  delay(200);
  Serial.println(sensorValue); 

  ledPinSelect(ledID, LOW);

  return sensorValue;
}



void ledPinSelect(int channel, bool state) {
  
  int controlPin[] = {muxS0, muxS1, muxS2, muxS3};
  int comPin;

  if (channel < numChannels) comPin = COM1;
  else comPin = COM2;

  channel = channel % numChannels;
  
  int muxChannel[16][4] = {
      {1, 0, 0, 0}, // channel 0 R1
      {0, 0, 0, 0}, // channel 1 IR1
      {1, 1, 0, 0}, // channel 2 R2
      {0, 1, 0, 0}, // channel 3 IR2
  
      {1, 0, 1, 0}, // channel 4 R3
      {0, 0, 1, 0}, // channel 5 IR3
      {1, 1, 1, 0}, // channel 6 R4
      {0, 1, 1, 0}, // channel 7 IR4
  
      {1, 0, 0, 1}, // channel 8 R5
      {0, 0, 0, 1}, // channel 9 IR5
      {1, 1, 0, 1}, // channel 10 R6
      {0, 1, 0, 1}, // channel 11 IR6
  
      {1, 0, 1, 1}, // channel 12 R7
      {0, 0, 1, 1}, // channel 13 IR7
      {1, 1, 1, 1}, // channel 14 R8
      {0, 1, 1, 1}  // channel 15 IR8
  };


  //loop through the 4 sig
  for(int i = 0; i < 4; i++){
    digitalWrite(controlPin[i], muxChannel[channel][i]);
  }

  if (!testingPhotodiode) digitalWrite(comPin,state);
  else digitalWrite(comPin,0);
  delay(1);
}

void rawdata()
{
  Serial.print(",");
  
  for (int i=0; i < numChannels*2 ; i++)
  {
    Serial.print(sensorRead(i)); 
    Serial.println(",");
  }
  
  Serial.println();
}



void takeWhite(int chID)
{
  Serial.println("White started for channel " + String(chID + 1) );
  for (int i = 0; i<2 ; i++)
  {
      whiteData[chID*2 + i] = sensorRead(chID*2 + i);
      Serial.println(String(whiteData[chID*2 + i]) + ",");
      
      byte quotient = whiteData[chID*2 + i] / 255;    // Calculate quotient (0-254)
      byte remainder = whiteData[chID*2 + i] % 255;   // Calculate remainder (0-254) 
      EEPROM.write(chID*4 + i*2, quotient);    // Store quotient
      EEPROM.write(chID*4 + i*2 + 1, remainder);   // Store remainder      
  }
  Serial.println();
  EEPROM.commit(); 
}

//Thermistor code
double Thermistor(double VRT) {
  double Temp;
  VRT   = (VRT / ADCmax) * VCC;      //Conversion to voltage 26255
  VR = VCC - VRT;
  RT = VRT / (VR / R);               //Resistance of RT
  ln = log(RT / Rt0);
  Temp = (1 / ((ln / B) + (1 / t0))); //Temperature from thermistor
  Temp = Temp - 273.15;
  
  return Temp;
}

//PID controller code
void Control_PID(float iTemp){
        
    //Overheat protection
        if(iTemp>MaxTemp){
          ledcWrite(pwmChannel, 0);
          //Serial.println("Error:overheat. Heater turned off");
          return;
        }
      
    //In range? If in range, maybe turn on LED?
        if((iTemp) >= SetTemp){

          if(bInRange==0){
            //digitalWrite(ledPin1, HIGH); 

            bInRange=1;
          }
        }else{
          if(bInRange==1){

            //digitalWrite(ledPin1, LOW); 
            bInRange=0;
          }

        }
        
      
      //PID subroutine
        float err = SetTemp - iTemp;
         //Serial.println(err);
        s_integral += err*dt;
        //Serial.println(s_integral);
        float s_derivative = (err - previous_error)/dt;
        //Serial.println(s_derivative);
        int U_in_ctrl = (K_P_ctrl*err + K_I_ctrl*s_integral + K_D_ctrl*s_derivative)/(MaxTemp-MinTemp)*1023;
        //if (U_in_ctrl > 700) U_in_ctrl = 700;
        
        previous_error = err;
               
        
      // put voltage to output and write value to serial monitor
        //  Serial.print("Output PWM frequency: ");
          
          if (U_in_ctrl<=1023){
             if (U_in_ctrl > 0){
                ledcWrite(pwmChannel, U_in_ctrl);
           //     Serial.println(U_in_ctrl);
              
             }           
             else
             {

                ledcWrite(pwmChannel, 1);
            //    Serial.println("1 / 0 V"); 
             }
          }
          else{
             ledcWrite(pwmChannel,1023);//ledcWrite(pwmChannel,700);

         //   Serial.println("1023 / 3.3 V");             
          }
                    
}


void printwhite() {
  Serial.print("White values: ");
  for (int i = 0; i < numChannels*2; i++) 
  {
    byte quotient = EEPROM.read(2*i);            // Read quotient
    byte remainder = EEPROM.read(2*i+1);         // Read remainder
 
    whiteData[i] = quotient * 255 + remainder;   // Combine quotient and remainder
    Serial.print(whiteData[i]); Serial.print(" ,");
  }
  
  Serial.println();
}

void setRotationInterval() {
  Serial.println("Enter the new rotation interval in minutes:");

  while (!Serial.available()) {
    delay(100); // Wait for input
  }

  // Read the input value from the Serial Monitor
  rotationIntervalEeprom = Serial.parseInt();

  // Validate the input
  if (Serial.read() == '\n') {
    // Input is valid, proceed to save to EEPROM

    // Extract quotient and remainder
    byte quotient = rotationIntervalEeprom / 255;
    byte remainder = rotationIntervalEeprom % 255;

    // Save quotient and remainder to EEPROM
    EEPROM.write(166, quotient);
    EEPROM.write(167, remainder);
    EEPROM.commit();

    Serial.println("Rotation interval updated and saved to EEPROM.");
  } else {
    // Input is not valid
    Serial.println("Invalid input. Please enter a valid rotation interval.");
  }
}

//void settempShift() {
//  Serial.println("Enter the new temperature shift value multiplied by 10:");
//
//  while (!Serial.available()) {
//    delay(100); // Wait for input
//  }
//
//  // Read the input value from the Serial Monitor
//  tempShift = Serial.parseInt();
//
//  // Validate the input
//  if (Serial.read() == '\n') {
//    // Input is valid, proceed to save to EEPROM
//
//    // Save quotient and remainder to EEPROM
//    EEPROM.write(168, tempShift);
//    EEPROM.commit();
//
//    Serial.println("Temperature shift value updated and saved to EEPROM.");
//  } else {
//    // Input is not valid
//    Serial.println("Invalid input. Please enter a valid Temperature shift value multiplied by 10:");
//  }
//}


void setSerial() {
  Serial.println("Enter the new serial value:");

  while (!Serial.available()) {
    delay(100); // Wait for input
  }

  // Read the input value from the Serial Monitor
  serialNum = Serial.parseInt();

  // Print the entered serial value for debugging
  Serial.print("Entered serial value: ");
  Serial.println(serialNum);

  // Extract quotient and remainder
  byte quotient = serialNum / 255;
  byte remainder = serialNum % 255;

  // Save quotient and remainder to EEPROM
  EEPROM.write(64, quotient);
  EEPROM.write(65, remainder);
  EEPROM.commit();
  
  Serial.println("Serial value updated and saved to EEPROM.");

  deviceID = "MBS" + String(serialNum + 100000).substring(1);
  Serial.println(deviceID);   
}

void setSSID() {
  Serial.println("Enter new SSID:");
  while (!Serial.available()) {
    delay(100);
  }
  String newSSID = Serial.readStringUntil('\n');
  Serial.println("New SSID: " + newSSID);

  // Write SSID to EEPROM
  saveToEEPROM(newSSID.c_str(), 66, 50);

  Serial.println("SSID updated and saved to EEPROM.");
}

void setPassword() {
  Serial.println("Enter new Password:");
  while (!Serial.available()) {
    delay(100);
  }
  String newPass = Serial.readStringUntil('\n');
  Serial.println("New Pass: " + newPass);

  // Write SSID to EEPROM
  saveToEEPROM(newPass.c_str(), 116, 50);

  Serial.println("Password updated and saved to EEPROM.");
}

void setUser() {
  Serial.println("Enter new User ID:");
  while (!Serial.available()) {
    delay(100);
  }
  String newUser = Serial.readStringUntil('\n');
  Serial.println("New User ID: " + newUser);

  // Write SSID to EEPROM
  saveToEEPROM(newUser.c_str(), 169, 50);

  Serial.println("User ID updated and saved to EEPROM.");
}

void setUserPass() {
  Serial.println("Enter new User Password:");
  while (!Serial.available()) {
    delay(100);
  }
  String newPass = Serial.readStringUntil('\n');
  Serial.println("New User Password:: " + newPass);

  // Write SSID to EEPROM
  saveToEEPROM(newPass.c_str(), 219, 50);

  Serial.println("User Password updated and saved to EEPROM.");
}

void processSerialCommand() {
  if (Serial.available() > 0) {
    state = Serial.readStringUntil('\n'); // Read until a newline character is received

    if (state == "rotate") {
      rotate_();
    }
    
    else if (state.startsWith("start#")) {
      int channelID = extractChannelID(state);
      startChannel(channelID, "Sample");
    } 
    
    else if (state.startsWith("stop#")) {
      int channelID = extractChannelID(state);
      stopChannel(channelID);
    } 
    
    else if (state.startsWith("white#")) {
      int channelID = extractChannelID(state);
      if (channelID >= 1 && channelID <= numChannels) {
        takeWhite(channelID - 1);
      } else if (channelID == 100) {
        for (int i = 0; i < numChannels; i++) {
          takeWhite(i);
        }
      }
      else {
        Serial.println("Invalid channel ID. Please use a number between 1 and " + String(numChannels) + " or 100.");
      }
    }
    
    else if (state == "temp") {
      Serial.print("Temperature: ");
      Serial.println(temperature,1); //1 decimal
    }

     else if (state == "setserial") {
      setSerial();
    }

     else if (state == "setssid") {
      setSSID();
      connectToWiFi();
    }

     else if (state == "setpassword") {
      setPassword();
      connectToWiFi();
    }

      else if (state == "setuser") {
      setUser();
    }

     else if (state == "setuserpassword") {
      setUserPass();
      }
    
     else if (state == "setrotation") {
      setRotationInterval();
    }


//    else if  (state == "settemp"){
//      settempShift();
//    }
    
    else if (state == "all") {
      rawReadingFlag = false;
      Serial.print(temperature);
      Serial.print(",");
      //allReading();
    }
    
    else if (state == "rawdata") {
      Serial.println("rawdata");
      rawdata();
    } 
    
    else if (state == "printwhite") {
      printwhite();
    } 

    
     else if (state == "complete") {
      completedChannel(6);
    } 
    
    else if (state == "serial") {
      Serial.println(serialNum);
    } 
    
    else if (state == "first") {
      for (int i = 0; i < numChannels; i++) {
        Serial.print(String(firstReading[i]) + ",");
      }
      Serial.println();
    } 
    
    else {
      Serial.println("Invalid command: " + state);
    }

    state = ""; // Reset state after processing
  }
}

int extractChannelID(String command) {
  // Extract the channel ID from the "start#X", "stop#X", or "white#X" command
  if (command.startsWith("start#") || command.startsWith("stop#") || command.startsWith("white#")) {
    return command.substring(6).toInt();
  }
  return -1; // Return -1 if the command doesn't contain a valid channel ID
}


void readChannels() {
    unsigned long currentMillis = millis();
    static unsigned long lastDisplayTime = 0;

    // Check if 30 seconds have passed since the last display
    if (currentMillis - lastDisplayTime >= datainterval * 60000) {
        lastDisplayTime = currentMillis;

        for (int i = 0; i < numChannels; i++) {
            if (isRunning[i]) {
                unsigned long durationSeconds = (currentMillis - channelStartTimes[i]) / 1000; // Convert to seconds
                float durationMinutes = durationSeconds / 60.0; // Convert to minutes with one decimal place
                Serial.print("Channel " + String(i + 1) + ": ");
                Serial.print(durationMinutes, 0); // Display with zero decimal place
                Serial.print(",");
                
                long absVal = round(100000*( log10((float)whiteData[2*i]/sensorRead(2*i)) - log10((float)whiteData[2*i+1]/sensorRead(2*i+1)) ));
                progressValues[i] = round(100*( (float)(firstReading[i] - absVal) / firstReading[i] ) );
                Serial.println(progressValues[i]);
                if (progressValues[i] > 99) progressValues[i] = 99;
                if (progressValues[i] < 0) progressValues[i] = 0;
                if (progressValues[i] >= progressThreshold) completedChannel(i+1);
                updateProgress(i+1);
                updateAllElapsedTime(); //priority 2
          
            }
        }
        
    }
}

void takeFirstReading(int chID) {
   // Serial.println("Channel " + String(chID+1) + " started");
    rawReadingFlag = false;
    
    int i = chID * 2;
    firstReading[chID] = round(100000 * (log10((float)whiteData[i] / sensorRead(i)) - log10((float)whiteData[i + 1] / sensorRead(i + 1))));
    Serial.print("First Reading = ");
    Serial.println(firstReading[chID]);
    progressValues[chID] = 0;
    autoReadFlag = true;

    //send firstreading to firebase
    //send white1 and white2
    String uidStr = String(auth.token.uid.c_str());
    sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(chID+1) + "/FirstReading" , String(firstReading[chID]));
    sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(chID+1) + "/White1" , String(whiteData[i]));
    sendDataToFirebase("/sampleDetails/"  + uidStr + " channel "   + String(chID+1) + "/White2" , String(whiteData[i+1]));

    
}
