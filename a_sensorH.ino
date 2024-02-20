//------------------------- pin declaration start--------------------------//
// Photodiode Pins
#define SDA_PHOTODIODE 21
#define SCL_PHOTODIODE 22

// Heater & Temperature Sensor Pins
#define HEATER_PIN 2
#define SDA_TEMP 33
#define SCL_TEMP 32

// Stepper Motor Pins
#define STEP_PIN 18
#define DIR_PIN 19
#define MOT_DRIVER_PIN 25
#define LIMIT_PIN 15

// Mux Control Pins
int muxS0 = 4;
int muxS1 = 12;
int muxS2 = 13;
int muxS3 = 14;

// Mux "SIG" Pins
int COM1 = 27;
int COM2 = 26;

//------------------------- pin declaration ends --------------------------//

// Create instances of the Adafruit ADS1115 class for each device
Adafruit_ADS1115 ads1; //0X49
Adafruit_ADS1115 ads2; //0X49
Adafruit_ADS1115 ads3; //0X48
Adafruit_ADS1115 ads4; //0X49
Adafruit_ADS1115 adsTemp; //0X48

TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

// ADS1115 declaration ends

bool autoReadFlag, whiteAllCommandSent, rawReadingFlag;

bool testingPhotodiode = false; //true when testing photodiodes. true will turn off the LEDs.

String state;

int firstReading[numChannels*2];
int progressValues[numChannels];
//int whiteData[numChannels*2];
int whiteData[numChannels*2]={23154,11073,25107,13987,23393,11961,24426,11920,9913,9914,20386,9660,20046,9768,19143,9810};

int channelarrangement[numChannels*2] = {6,7,4,5,2,3,0,1,14,15,12,13,10,11,8,9,24,25,26,27,28,29,30,31,16,17,18,19,20,21,22,23};


// setting PWM properties
const int freq = 5000;
const int pwmChannel = 1;
const int ledpwmChannel = 2;
const int resolution = 10;

//double tempShift;

unsigned long lastMillis = 0, currentMillis = 0, previousMillisWifi = 0;


float minute = 0;
int current_limitpin, prev_limitpin;


#define VCC 3.29    //Supply   voltage
#define ADCmax 26287 //TempADC maxvalue at VCC input

const int dt = 500; // [ms] time constant in milliseconds(controller clock rate = 1/(dt/1000) [Hz])
#define SetTemp set_temp //[degC] set temperature in DegC
float MinTemp = set_temp - 5; // [degC] minimum expected temperature (needed for rescaling inputs)
float MaxTemp = set_temp; // [degC] maximum allowed temperature, over which heater is turned off (needed for rescaling inputs)
int SetTime = 1800; // [s] timer in seconds, if reached, running stops [Default: 1800]  
double K_P_ctrl = 10; //proportional gain
double K_I_ctrl = 0; //integral gain (set to lower values i.e. 10^-3)
double K_D_ctrl = 0; //derivative gain
///////////////////////////////////
#define Rt0 100000   // O
#define R 100000  //R=100KO
#define B 3950      //   K
float RT, VR, ln, tx, t0, VRT, previous_error, s_integral;
bool bInRange = 0;
int TicksPerMS = floor(1000/dt);

//char userEmail[50];
//char userPassword[50];
