#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <EEPROM.h>
#include <math.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> 
#include <NTPClient.h>
#include <stdlib.h>


const byte rxPin = 16; // rx2
const byte txPin = 17; // tx2
HardwareSerial dwin(1);

#define numChannels 16


//for firebase
#define MAX_CHANNELS 16
String channelUIDs[MAX_CHANNELS];


const float datainterval = 0.5; //in min
//const float set_temp = 37.6; //deg C
float temperature;
float tempShift = 37.6; // temperature shift value
float set_temp = 37.6 - (tempShift-37.6); //deg C

bool rotateFlag = false;
bool rotationEnableFlag;//flag for rotation 


int rotationIntervalMinutes;
int rotationIntervalEeprom = 1 ; //rotation interval in minutes set  from firebsae/dashboard

bool wifiConnected = false;


//firebase
//struct SensorData {
//  String sampleName;
//  int channelId;
//  int firstReading;
//  int white1;
//  int white2;
//  long startTime;
//  long endTime;
////  String date;
//  int mbrt;
//  int endProgress;
////  int progress;
//};

//ntp for date and time
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

WiFiUDP udpClient;
NTPClient timeClient(udpClient, ntpServer, gmtOffset_sec, daylightOffset_sec);

int currentYearUTC, currentMonthUTC, currentDayUTC;
int currentHourUTC, currentMinuteUTC;

int currentHourIST, currentMinuteIST;

String currentDate, currentTime;

void updateTime() {
    // Fetch the current time using NTP
    timeClient.update();

    // Get the current time components in UTC
    currentHourUTC = timeClient.getHours();
    currentMinuteUTC = timeClient.getMinutes();

    // Calculate the IST time by adding the offset
    currentHourIST = (currentHourUTC + 5) % 24;  // Add 5 hours for IST
    currentMinuteIST = (currentMinuteUTC + 30) % 60;  // Add 30 minutes for IST

    // Format the time as "HH:mm"
    currentTime = String(currentHourIST) + ":" + (currentMinuteIST < 10 ? "0" : "") + String(currentMinuteIST);
}


void updateDate() {
    // Fetch the current time using NTP
    timeClient.update();

    // Get the current epoch time
    unsigned long epochTime = timeClient.getEpochTime();

    // Convert epoch time to a time_t variable
    time_t timeNow = static_cast<time_t>(epochTime);

    // Convert time_t to a human-readable format
    struct tm timeinfo;
    gmtime_r(&timeNow, &timeinfo);

    // Extract year, month, and day components
    currentYearUTC = timeinfo.tm_year + 1900;
    currentMonthUTC = timeinfo.tm_mon + 1;
    currentDayUTC = timeinfo.tm_mday;

    // Format the date as "DD-MM-YYYY"
    currentDate = (currentDayUTC < 10 ? "0" : "") + String(currentDayUTC) + "-" +
                  (currentMonthUTC < 10 ? "0" : "") + String(currentMonthUTC) + "-" +
                  String(currentYearUTC);
}


//int rotationIntervalMinutes; // Rotation interval in minutes initially 0
int progressThreshold; // progress threshold initially set to zero

//user functions
char userEmail[50];
char userPassword[50];

#define API_KEY "AIzaSyCeNaLokSCzF9yp-IAN27mdh6bVCJrMCho"
#define DATABASE_URL "https://mbscan-dashboard-default-rtdb.firebaseio.com"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
HTTPClient http;

bool initialSet = false; // to check if there  is any updated value of rotation interval 

void firebaseInitilize() {
  // Check WiFi status
  if (WiFi.status() != WL_CONNECTED) {
//    Serial.println("WiFi not connected. Firebase initialization skipped.");
    wifiConnected = false;
    return; // Exit the function if WiFi is not connected
  }

  // Continue with Firebase initialization if WiFi is connected
  config.api_key = API_KEY;
  auth.user.email = userEmail;
  auth.user.password = userPassword;
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  Firebase.begin(&config, &auth);
  Serial.println("Connected to Firebase!");

  rotationIntervalMinutes = readDataFromFirebase("/deviceDetails/rotationTime");
  tempShift = readDataFromFirebase("/deviceDetails/tempShift");
  progressThreshold = readDataFromFirebase("/deviceDetails/endPointPercentage");
  //  rotateFlag = readDataFromFirebase("/deviceDetails/rotateFlag");
  rotationEnableFlag = readDataFromFirebase("/deviceDetails/rotationEnabled");

  if (!initialSet) {
    sendRotationTimeToFirebase();
    sendtempShiftToFirebase();
    sendEndPointPercentageToFirebase();
    sendRotateFlagToFirebase();
  }
}

void updateConnectionStatus() {
  String uidStr = String(auth.token.uid.c_str());
  String fullPath = "/users/" + uidStr + "/connectionStatus";

  if (Firebase.RTDB.setString(&fbdo, fullPath.c_str(), "Connected")) {
    Serial.println("Firebase Connected");
  } else {
    Serial.print("Failed to update connection status to Firebase. ");
    Serial.println(fbdo.errorReason());
    firebaseInitilize();
  }
}


bool readBoolDataFromFirebase(const String& path) {
  String uidStr = String(auth.token.uid.c_str());
  String fullPath = "/users/" + uidStr + path;

  if (Firebase.RTDB.getBool(&fbdo, fullPath.c_str())) {
    return fbdo.boolData();
  } else {
    Serial.println("Failed to read data from Firebase (" + path + ").");
    return false; // Return a default value in case of failure
  }
}


float readDataFromFirebase(const String& path) {
  String uidStr = String(auth.token.uid.c_str());
  String fullPath = "/users/" + uidStr + path;

  if (Firebase.RTDB.getFloat(&fbdo, fullPath.c_str())) {
    return fbdo.floatData();
  } else {
    Serial.println("Failed to read data from Firebase (" + path + ").");
    return 0.0; // Return a default value in case of failure
  }
}

// send data to firebase
template<typename T>
void sendDataToFirebase(const String& path, T data) {
  String uidStr = String(auth.token.uid.c_str());
  String fullPath = "/users/" + uidStr + path;

  if (Firebase.RTDB.set(&fbdo, fullPath.c_str(), data)) {
//    Serial.println(path + " sent to Firebase successfully!");
  } else {
//    Serial.println("Failed to send " + path + " to Firebase.");
  }
}


String generateChannelUID(int channelNumber) {
  // Get current user UID (assuming it's available in auth.token.uid.c_str())
  String currentUserUID = String(auth.token.uid.c_str());

  // Generate a random number
  int randomValue = random(1000); // Adjust the range based on your needs

  // Get current timestamp
  unsigned long currentTime = millis();

  // Combine the components to create the UID
  String uidStr = currentUserUID + "_" + String(randomValue) + "_" + String(currentTime);
    // Store the generated UID in the channelUIDs array
  channelUIDs[channelNumber - 1] = uidStr;
  return uidStr;
}


// set or update rotation time
void sendRotationTimeToFirebase() {
  float existingValue = readDataFromFirebase("/deviceDetails/rotationTime");
  
  if (existingValue == 0) {
    rotationIntervalEeprom = 30; //if rotation minutes is 0 then it updates it to 30 by default
    sendDataToFirebase("/deviceDetails/rotationTime", int(rotationIntervalEeprom));
  }
  else {
    rotationIntervalEeprom = existingValue; // Update the rotationIntervalEeprom with the value from Firebase
    Serial.print("Current Rotation Interval: ");
    Serial.println(rotationIntervalEeprom);
  }
}


// set or update endpoint percentage
void sendEndPointPercentageToFirebase() {
  float existingValue = readDataFromFirebase("/deviceDetails/endPointPercentage");

  if (existingValue == 0) {
    progressThreshold = 95;
    sendDataToFirebase("/deviceDetails/endPointPercentage", int(progressThreshold));
  } else {
    progressThreshold = existingValue;
    Serial.print("Current progress Threshold: ");
    Serial.println(progressThreshold);
  }
}

// set or update tempShift  //ask sir about it
void sendtempShiftToFirebase() {
  float existingValue = readDataFromFirebase("/deviceDetails/tempShift");

  if (existingValue == 0.0) {
    tempShift = 37.6;
    sendDataToFirebase("/deviceDetails/tempShift", float(tempShift));
  } else {
    tempShift = existingValue;
    Serial.print("Current tempShift: ");
    Serial.println(tempShift);
    Serial.print("eeprom tempShift: ");
    Serial.println(tempShift);
  }
}


//// set or update rotate flag 
void sendRotateFlagToFirebase() {
  bool existingValue = readDataFromFirebase("/deviceDetails/rotationEnabled");
  if (existingValue != true && existingValue != false) {

    rotationEnableFlag = true;
    sendDataToFirebase("/deviceDetails/rotationEnabled", bool(rotationEnableFlag));
  } else {
    rotationEnableFlag = existingValue;
    Serial.print("Current rotation flag: ");
    Serial.println(rotationEnableFlag);
  }
}



//const int BUFFER_SIZE = 10;
//String dataBuffer[16][BUFFER_SIZE];
//int bufferIndex[16] = {0}; // One buffer index for each channel
//
//void addToBuffer(int channel, const String& data) {
//  if (bufferIndex[channel] < BUFFER_SIZE) {
//    dataBuffer[channel][bufferIndex[channel]] = data;
//    bufferIndex[channel]++;
//  } else {
//    // Buffer is full, handle accordingly (e.g., overwrite old data, discard, etc.)
//  }
//}
//
//void processBuffer(int channel) {
//  for (int i = 0; i < bufferIndex[channel]; i++) {
//    sendDataToFirebase(dataBuffer[channel][i]);
//  }
//  // Reset buffer after processing
//  bufferIndex[channel] = 0;
//}


void setup() {
    
  Serial.begin(115200);
  dwin.begin(115200, SERIAL_8N1, rxPin, txPin);
  EEPROM.begin(512);
  
  displayReset();
 

  delay(750); // this delay ensures the ESP32 writes values to the display only after the display has fully booted up.
  initializeDisplay();
  
  sensorSetup();
  Serial.println("Device is on");
  timeClient.begin();


}

void loop()
{  
  controlTemp(); //priority 3
  processSerialCommand(); //priority 5
  readChannels();//check 30 seconds elapsed and take readings. priority 4

  //Display functions  
  sendTemp(); // low priority
  fetchText(); // priority 1
//  updateAllElapsedTime(); //priority 2
  if (rotateFlag) displayRotationScreen();
  if (!wifiConnected) checkWiFiConnection();  // low priority
}
