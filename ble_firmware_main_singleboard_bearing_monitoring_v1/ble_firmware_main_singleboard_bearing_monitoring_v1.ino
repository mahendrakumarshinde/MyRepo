/*
  Infinite Uptime BLE Module Firmware
*/

#include <i2c_t3.h>
#include <SPI.h>
#include <i2s.h> /* I2S digital audio */

#define ARM_MATH_CM4
#include <arm_math.h> /* CMSIS-DSP library for RFFT */
#include "constants.h" /* Register addresses and hamming window presets  */

//====================== Module Configuration Variables ========================

#define CLOCK_TYPE         (I2S_CLOCK_48K_INTERNAL)     // I2S clock
bool statusLED = true;                                  // Status LED ON/OFF
String MAC_ADDRESS = "88:4A:EA:69:DE:A0";
const uint16_t AUDIO_FREQ = 8000; // Reduce RUN frequency if needed.
uint16_t TARGET_AUDIO_SAMPLE = AUDIO_FREQ;          // Audio frequency
uint16_t TARGET_ACCEL_SAMPLE = 1000;              // Accel frequency
const uint32_t RESTING_INTERVAL = 0;                    // Inter-batch gap

// NOTE: Do NOT change the following variables unless you know what you're doing
const uint16_t RUN_ACCEL_AUDIO_RATIO = AUDIO_FREQ / TARGET_ACCEL_SAMPLE;
uint16_t ACCEL_COUNTER_TARGET = TARGET_AUDIO_SAMPLE / TARGET_ACCEL_SAMPLE;
uint16_t DOWNSAMPLE_MAX = 48000 / TARGET_AUDIO_SAMPLE;
uint16_t subsample_counter = 0;
uint16_t accel_counter = 0;

//Temperature variables
float tempSensitivity = 333.87;
int16_t tempCount = 0;
float temperature = 0.0;
bool temperature_ticks = true; // Counter for temperature reading update
//==============================================================================
//====================== Feature Computation Variables =========================
//==============================================================================

// TIME DOMAIN FEATURES
// NOTE: Intervals are checked based on AUDIO, not ACCEL. Read vibration energy
//       code to understand the sampling mechanism.

char rec_axis = 'X';  // To send data for record function
const uint8_t NUM_FEATURES = 6;    // Total number of features
const uint8_t NUM_TD_FEATURES = 2; // Total number of frequency domain features
int chosen_features = 0;// ZAB: To send data over BLE

//ZAB: Function Declaration for features
float feature_energy(); // Index 0
float velocityX();      // Index 1
float velocityY();      // Index 2
float velocityZ();      // Index 3
float currentTemparature(); // Index 4
float audioDB();        // Index 5

typedef float (* FeatureFuncPtr) (); // this is a typedef to feature functions
FeatureFuncPtr features[NUM_FEATURES] = {feature_energy,
                                         velocityX,
                                         velocityY,
                                         velocityZ,
                                         currentTemparature,
                                         audioDB
                                        };

// NOTE: DO NOT CHANGE THIS UNLESS YOU KNOW WHAT YOU'RE DOING
const uint16_t MAX_INTERVAL_ACCEL = 512;
const uint16_t MAX_INTERVAL_AUDIO = 4096;

// Byte representing which features are enabled. Feature index 0 is enabled by default.
byte enabledFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B11111100};
int FeatureID[NUM_FEATURES];

// These should only change when list of features are changed.
const byte audioFDFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B00000100};
const byte accelFDFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B00111000};

// NOTE: By default, FD features are turned off. Turn these on if you want to
//       force FD computation for debugging.
bool audioFDCompute = true;
bool accelFDCompute = true;

// Array of thresholds; add new array and change code accordingly if you want
// more states.
float featureNormalThreshold[NUM_FEATURES] = {30,
                                              100,
                                              2000,
                                              2000,
                                              200,
                                              500
                                             };
float featureWarningThreshold[NUM_FEATURES] = {600,
                                               150,
                                               3000,
                                               3000,
                                               205,
                                               1000
                                              };
float featureDangerThreshold[NUM_FEATURES] = {1200,
                                              200,
                                              4000,
                                              4000,
                                              210,
                                              1500
                                             };

// NOTE: These variables are responsible for "buffer-switching" mechanism.
//       Change these only if you know what you're doing.
bool compute_feature_now[2] = {false, false};
bool record_feature_now[2] = {true, true};
uint8_t buffer_compute_index = 0;
uint8_t buffer_record_index = 0;

// TODO: Fix the default value to ENERGY_INTERVAL_AUDIO after computation results
//       are verified. This variable will determine the sampling batch size
//       and should change according to which features are enabled currently.
uint32_t featureBatchSize = MAX_INTERVAL_AUDIO;

uint32_t audioSamplingCount = 0;
uint32_t accelSamplingCount = 0;
uint32_t restCount = 0;
uint8_t highest_danger_level = 0;
uint8_t current_danger_level = 0;
float feature_value[NUM_FEATURES];

// Acceleration Buffers
float accel_x_batch[2][MAX_INTERVAL_ACCEL];
float accel_y_batch[2][MAX_INTERVAL_ACCEL];
float accel_z_batch[2][MAX_INTERVAL_ACCEL];

// Velocity variables
float u_x = 0, u_y = 0, u_z = 0;

// Audio Buffers
float audio_batch[2][MAX_INTERVAL_AUDIO];

// RFFT Variables
const uint16_t ACCEL_NFFT = 512;
const uint16_t AUDIO_NFFT = 2048;

const uint8_t ACCEL_RESCALE = 8;
const uint8_t AUDIO_RESCALE = 10;

//==============================================================================
//============================ Internal Variables ==============================
//==============================================================================


// Specify sensor full scale
uint8_t Ascale = AFS_2G;           // set accel full scale
uint8_t ACCBW  = 0x08 & ABW_1000Hz;  // Choose bandwidth for accelerometer

// Pin definitions
int myLed       = 13;  // LED on the Teensy 3.1/3.2
int chargerCHG  = 2;   // CHG pin to detect charging status

// MPU9250 variables
int16_t accelCount[3];  // Stores the 16-bit signed accelerometer sensor output
float accelBias[3] = {0, 0, 0}; // Bias corrections for gyro, accelerometer, mag

float ax, ay, az; // variables to hold latest sensor data values
float aRes; //resolution of accelerometer

// Keep this as passThru to minimize latency in between accelerometer samples
bool passThru = true;

// Operation mode enum
enum OpMode {
  RUN              = 0,
  CHARGING         = 1,
  DATA_COLLECTION  = 2
};

OpMode currMode = RUN;

// Modes of Operation String Constants
const String START_COLLECTION = "IUCMD_START";
const String START_CONFIRM = "IUOK_START";
const String END_COLLECTION = "IUCMD_END";
const String END_CONFIRM = "IUOK_END";

// Operation state enum
enum OpState {
  NOT_CUTTING     = 0,
  NORMAL_CUTTING  = 1,
  WARNING_CUTTING = 2,
  BAD_CUTTING     = 3
};

OpState currState = NOT_CUTTING;

// LED color code enum
enum LEDColors {
  RED_BAD         , // L H H
  BLUE_NOOP       , // H H L
  GREEN_OK        , // H L H
  ORANGE_WARNING  , // L L H
  PURPLE_CHARGE   , // L H L
  CYAN_DATA       , // H L L
  WHITE_NONE      , // L L L
  SLEEP_MODE        // H H H
};

// Explicit function declaration for enum argument
void changeStatusLED(LEDColors color);

boolean silent = false;

// Hard-wired communication buffers
String wireBuffer = ""; // Serial only expects string literals
char characterRead; // Holder variable for newline check

// Two-way Communication Variables
char bleBuffer[19] = "abcdefghijklmnopqr";
uint8_t bleBufferIndex = 0;
int bleFeatureIndex = 0;
boolean didThresChange = false;
int args_assigned = 0;
int args_assigned2 = 0;
boolean rubbish = false;
int date = 0;
int dateset = 0;
double dateyear = 0;
int dateyear1 = 0;
int parametertag = 0;

//Battery state
int bat = 100;

//Regular data transmit variables
boolean datasendtime = false;
int datasendlimit = 500;
int currentmillis = 0;
int prevmillis = 0;
int currenttime = 0;
int prevtime = 0;

//Data reception robustness variables
int buffnow = 0;
int buffprev = 0;
int datareceptiontimeout = 2000;

//Sleep mode variables
int blue = 0;
boolean sleepmode = false;
int bluemillisnow = 0;
int bluesleeplimit = 60000;
int bluetimerstart = 0;

/*==============================================================================
  ====================== Feature Computation Caller ============================
  ==============================================================================*/
// Thresholding helper function to reduce code redundancy
uint8_t threshold_feature(int index, float featureVal) {
  if (featureVal < featureNormalThreshold[index]) {
    return 0; // level 0: not cutting
  }
  else if (featureVal < featureWarningThreshold[index]) {
    return 1; // level 1: normal cutting
  }
  else if (featureVal < featureDangerThreshold[index]) {
    return 2; // level 2: warning cutting
  }
  else {
    return 3; // level 3: bad cutting
  }
}

//ZAB: Function to send accel raw data which will be plotted after fft on app
void show_record_fft(char rec_axis)
{
  BLEport.print("REC,");
  BLEport.print(MAC_ADDRESS);
  if(rec_axis == 'X')
  {
    BLEport.print(",X");
    for (int i = 0; i < 512; i++) {
      BLEport.print(",");
      BLEport.print(accel_x_batch[buffer_compute_index][i]);
    }
  }        
  else if(rec_axis == 'Y')
  {
    BLEport.print(",Y");
    for (int i = 0; i < 512; i++) {
      BLEport.print(",");
      BLEport.print(accel_y_batch[buffer_compute_index][i]);
    }
  }
  else if(rec_axis == 'Z')
  {
    BLEport.print(",Z");
    for (int i = 0; i < 512; i++) {
      BLEport.print(",");
      BLEport.print(accel_z_batch[buffer_compute_index][i]);
    }
  }
  BLEport.print(";");
  BLEport.flush();
}

//ZAB: function to send feature data
void send_features_to_BLE(uint8_t level)
{
  BLEport.print(MAC_ADDRESS);
  if(level == 0)
  { BLEport.print(",00,"); }
  else if(level == 1)
  { BLEport.print(",01,"); }
  else if(level == 2)
  { BLEport.print(",02,"); }
  else if(level == 3)
  { BLEport.print(",03,"); }
  
  BLEport.print(bat);
  for (int i = 0; i < chosen_features; i++) {
    BLEport.print(",");
    BLEport.print("000");
    BLEport.print(FeatureID[i] + 1);
    BLEport.print(",");
    BLEport.print(feature_value[i]);
  }
  BLEport.print(",");
  BLEport.print(gettimestamp());
  BLEport.print(";");
  BLEport.flush();
}

void compute_features() {
  // Turn the boolean off immediately.
  compute_feature_now[buffer_compute_index] = false;
  record_feature_now[buffer_compute_index] = false;
  highest_danger_level = 0;

//  for (int i = 0; i < NUM_TD_FEATURES; i++) 
//  {
//    // Compute time domain feature only when corresponding bit is turned on.
//    feature_value[i] = features[i]();
//    if(i == 0)
//    {
//    current_danger_level = threshold_feature(i, feature_value[i]);
//    
//    // TODO: Variable is separated for possible feature value output.
//    //       AKA, when danger level is 2, output feature index, etc.
//    highest_danger_level = max(highest_danger_level, current_danger_level);
//    }
//  }
  //ZAB: Temporary
  feature_value[0] = feature_energy();
  current_danger_level = threshold_feature(0, feature_value[0]);
  highest_danger_level = max(highest_danger_level, current_danger_level);
    
  feature_value[1] = velocityX();
  feature_value[2] = velocityY();
  feature_value[3] = velocityZ();
  feature_value[4] = currentTemparature();
  feature_value[5] = audioDB();

  // Reflect highest danger level if differs from previous state.
  LEDColors c;
  switch (highest_danger_level) 
  {
    case 0: //IDLE BLUE
      if (sleepmode) {
        // Do nothing
      }
      else {
        if (currState != NOT_CUTTING || datasendtime) {
          currState = NOT_CUTTING;
          send_features_to_BLE(0);
        }
      }
      //Sleep mode check start!
      if (blue == 0 && ~sleepmode) {
        bluetimerstart = millis();
        blue = 1;
      }
      if (blue == 1) {
        bluemillisnow = millis();
        if (bluemillisnow - bluetimerstart >= bluesleeplimit) { //sleep mode
          sleepmode = true;
          blue = 0;
        }
      }
      //Sleep mode check end
      if (sleepmode) {
        c = SLEEP_MODE;
      }
      else {
        c = BLUE_NOOP;
      }
      changeStatusLED(c);
      datasendtime = false;
      break;
      
    case 1: //GREEN 
      if (currState != NORMAL_CUTTING || datasendtime) {
        currState = NORMAL_CUTTING;
        send_features_to_BLE(1);
      }
      c = GREEN_OK;
      changeStatusLED(c);
      datasendtime = false;
      blue = 0;
      sleepmode = false;
      break;
      
    case 2:
      if (currState != WARNING_CUTTING || datasendtime) {
        currState = WARNING_CUTTING;
        send_features_to_BLE(2);
      }
      c = ORANGE_WARNING;
      changeStatusLED(c);
      datasendtime = false;
      blue = 0;
      sleepmode = false;
      break;
      
    case 3:
      if (currState != BAD_CUTTING || datasendtime) {
        currState = BAD_CUTTING;
        send_features_to_BLE(3);
      }
      c = RED_BAD;
      changeStatusLED(c);
      datasendtime = false;
      blue = 0;
      sleepmode = false;
      break;
  }

  // Allow recording for old index buffer.
  record_feature_now[buffer_compute_index] = true;
  // Flip computation index AFTER all computations are done.
  buffer_compute_index = buffer_compute_index == 0 ? 1 : 0;
}


//Temparature calculation
int16_t readTempData()
{
  //Serial.println("START TEMPERATURE READING");
  uint8_t rawData[2];  // x/y/z gyro register data stored here
  readBytes(MPU9250_ADDRESS, 0x41, 2, &rawData[0]);  // Read the two raw data registers sequentially into data array
  return ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a 16-bit value
}

// Offset calculation
void calc_accelBias()
{
  int16_t i = 0;
  int16_t sample_lim = 500; // Offset for first500 samples
  
  for(i = 0; i < sample_lim; i++)  
  {
    readAccelData(accelCount);
    accelBias[0] += (float) accelCount[0];
    accelBias[1] += (float) accelCount[1];
    accelBias[2] += (float) accelCount[2];
  }

  accelBias[0] /= sample_lim;
  accelBias[1] /= sample_lim;
  accelBias[2] /= sample_lim;
}

/*==============================================================================
  ====================== Feature Computation Functions =========================

  Every function should include:
  (1) Feature calculation
  (2) Feature value thresholding
  (3) Return danger level according to the thresholds

  You can assume that buffers are correctly populated as feature requires.
  ==============================================================================*/

// TIME DOMAIN FEATURES

// Signal Energy ACCEL, index 0
// Simple loops  : 4200 microseconds
// All CMSIS-DSP : 2840 microseconds
// Partial       : 2730 microseconds (current)
//ZAB: Change for 512 samples
float feature_energy() {
  float ave_x = 0, ave_y = 0, ave_z = 0, ene = 0;

  // Calculate average
  arm_mean_f32(&accel_x_batch[buffer_compute_index][0], 512, &ave_x);
  arm_mean_f32(&accel_y_batch[buffer_compute_index][0], 512, &ave_y);
  arm_mean_f32(&accel_z_batch[buffer_compute_index][0], 512, &ave_z);

  // Calculate energy.
  for (int i = 0; i < 512; i++) {
    ene += sq(accel_x_batch[buffer_compute_index][i] - ave_x)
           +  sq(accel_y_batch[buffer_compute_index][i] - ave_y)
           +  sq(accel_z_batch[buffer_compute_index][i] - ave_z);
  }
  //ZAB: Serial.printf("Energy = %f", ene);
  return ene;
}

float velocityX()      // Index 1
{
  float v_x = 0.0;
  int16_t j = 0;
  for(j = 0; j < MAX_INTERVAL_ACCEL; j++)
  {
    v_x += sq(accel_x_batch[buffer_compute_index][j] * 0.001);
  }
  v_x /= MAX_INTERVAL_ACCEL;

  v_x = sqrt(v_x);  //RMS Value
  v_x *= 1000000; // Upscaling the velocity
  
  return v_x;
}
float velocityY()      // Index 2
{
  float v_y = 0.0;
  int16_t j = 0;
  for(j = 0; j < MAX_INTERVAL_ACCEL; j++)
  {
    v_y += sq(accel_y_batch[buffer_compute_index][j] * 0.001);
  }
  v_y /= MAX_INTERVAL_ACCEL;

  v_y = sqrt(v_y);  //RMS Value
  v_y *= 1000000; // Upscaling the velocity
  
  return v_y;
}
float velocityZ()      // Index 3
{
  float v_z = 0.0;
  int16_t j = 0;
  for(j = 0; j < MAX_INTERVAL_ACCEL; j++)
  {
    v_z += sq(accel_z_batch[buffer_compute_index][j] * 0.001);
  }
  v_z /= MAX_INTERVAL_ACCEL;

  v_z = sqrt(v_z);  //RMS Value
  v_z *= 1000000; // Upscaling the velocity
  
  return v_z;
}

float currentTemparature() // Index 4
{
  if (temperature_ticks) //Update temperature every (30 x 0.5 secs)
  {
    //Serial.println(accel_x_batch[0][0]);
    tempCount = readTempData();  // Read the adc values
    temperature = ((float) tempCount) / tempSensitivity + 21.0; // Temperature in degrees Centigrade
    temperature_ticks = false;
  }
  return temperature;
}  
float audioDB()        // Index 5
{
  float aud_db = 0.0;
  // Code here
  return aud_db;
}
//==============================================================================
//================================= Main Code ==================================
//==============================================================================

// extract the 24bit INMP441 audio data from 32bit sample
void extractdata_inplace(int32_t  *pBuf)
{
  // set highest bit to zero, then bitshift right 7 times
  // do not omit the first part (!)
  pBuf[0] = (pBuf[0] & 0x7fffffff) >> 7;
}

#define bufferSize 8000*3
const uint16_t target_sample = 8000;
const uint16_t inter = 48000 / target_sample;
unsigned char audio_buffer[bufferSize];

/* --- Direct I2S Receive, we get callback to read 2 words from the FIFO --- */

void i2s_rx_callback( int32_t *pBuf )
{
  //Serial.println("Audio Interrupt");
  
  if (currMode == RUN)
  {
    // If feature computation is lagging behind, reset all sampling routine and
    // Don't run any data collection.
    // TODO: Whenever this happens and STFT features are turned on, reset STFT
    // TODO: May want to do timing checks to save STFT resets
    if (audioSamplingCount == 0 && !record_feature_now[buffer_record_index])
    {
      //Serial.print("Returning on other\n");
      return;
    }
     // Filled out the current buffer; switch buffer index
    if (audioSamplingCount == featureBatchSize)
    {
      subsample_counter = 0;
      accel_counter = 0;
      audioSamplingCount = 0;
      accelSamplingCount = 0;
      restCount = 0;
   
      record_feature_now[buffer_record_index] = false;
      compute_feature_now[buffer_record_index] = true;
      buffer_record_index = buffer_record_index == 0 ? 1 : 0;

      // Stop collection if next buffer is not yet ready for collection
      if (!record_feature_now[buffer_record_index])
        return;
    }
  }

  // Downsampling routine
  if (subsample_counter != DOWNSAMPLE_MAX - 2)
  {
    subsample_counter ++;
    return;
  }
  else
  {
    subsample_counter = 0;
    accel_counter++;
  }

  if (currMode == RUN)
  {
    //Serial.print("currMode=RUN\n");
    if (restCount < RESTING_INTERVAL) {
      restCount ++;
    } else {
      // perform the data extraction for single channel mic
      extractdata_inplace(&pBuf[0]);

      byte* ptr = (byte*)&audio_batch[buffer_record_index][audioSamplingCount];
      ptr[0] = (pBuf[0] >> 8) & 0xFF;
      ptr[1] = (pBuf[0] >> 16) & 0xFF;

      //audio_batch[buffer_record_index][audioSamplingCount] =
      //                (((pBuf[0] >> 8) & 0xFF) << 8) | ((pBuf[0] >> 16) & 0xFF);
      audioSamplingCount ++;

      if (accel_counter == ACCEL_COUNTER_TARGET) {
        readAccelData(accelCount);
        //ZAB: Storing the Accel data in terms of g (-2g to +2g)
        accel_x_batch[buffer_record_index][accelSamplingCount] = ((float)accelCount[0] - accelBias[0]) / 16384.0 * 9.8;
        accel_y_batch[buffer_record_index][accelSamplingCount] = ((float)accelCount[1] - accelBias[1]) / 16384.0 * 9.8;
        accel_z_batch[buffer_record_index][accelSamplingCount] = ((float)accelCount[2] - accelBias[2]) / 16384.0 * 9.8;

        //Serial.println(accel_x_batch[buffer_record_index][accelSamplingCount]);
        
        accelSamplingCount ++;

        accel_counter = 0;
      }
    }
  }
  else if (currMode == DATA_COLLECTION)
  {
    // perform the data extraction for single channel mic
    extractdata_inplace(&pBuf[0]);

    // Dump sound data first
    Serial.write((pBuf[0] >> 16) & 0xFF);
    Serial.write((pBuf[0] >> 8) & 0xFF);
    Serial.write(pBuf[0] & 0xFF);

    if (accel_counter == ACCEL_COUNTER_TARGET)
    {
      readAccelData(accelCount);

      ax = (float)accelCount[0] * aRes + accelBias[0];
      ay = (float)accelCount[1] * aRes + accelBias[1];
      az = (float)accelCount[2] * aRes + accelBias[2];

      byte* axb = (byte *) &ax;
      byte* ayb = (byte *) &ay;
      byte* azb = (byte *) &az;

      Serial.write(axb, 4);
      Serial.write(ayb, 4);
      Serial.write(azb, 4);
      Serial.flush();

      accel_counter = 0;
    }
  }
}

/* --- Time functions --- */
double gettimestamp()
{
  return dateyear;
}

/* ----------------------- begin -------------------- */
void setup()
{
  // Setup for Master mode, pins 18/19, external pullups, 400kHz for Teensy 3.1
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  delay(5);//ZAB: delay(1000);
  
  // Serial Initialization for Data Collection mode
  Serial.begin(115200);
  delay(5);
  
  //Acoustic sensor initialization
  I2Cscan(); // should detect SENtral at 0x28
  // Set up the SENtral as sensor bus in normal operating mode
  // ld pass thru
  // Put EM7180 SENtral into pass-through mode
  SENtralPassThroughMode();
  // should see all the devices on the I2C bus including two from the EEPROM (ID page and data pages)
  I2Cscan();
  //LED pins initialization
  if (statusLED) {
    pinMode(27, OUTPUT);
    pinMode(28, OUTPUT);
    pinMode(29, OUTPUT);
  }
  //Serial.println("Initialize Accel");
  byte c = readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250); // Read WHO_AM_I register on MPU
  if (c == 0x71) { //WHO_AM_I should always be 0x71 for the entire chip
    initMPU9250();
    getAres();  //Ascale = AFS_2G
    delay(50);
    calc_accelBias();
   // Serial.printf("ax: %f, ay: %f, az: %f\n", accelBias[0], accelBias[1], accelBias[2]);
  }
  else
  {
    Serial.print("Could not connect to MPU9250: 0x");
    Serial.println(c, HEX);
    while (1) ; // Loop forever if communication doesn't happen
  }
  // << nothing before the first delay will be printed to the serial
  delay(5);//ZAB: delay(1500);

  if (!silent) {
    Serial.print("Pin configuration setting: ");
    Serial.println( I2S_PIN_PATTERN , HEX );
    Serial.println( "Initializing." );
    Serial.println( "Initialized I2C Codec" );
  }
  // Acoustic sensor interrupt settings
  I2SRx0.begin( CLOCK_TYPE, i2s_rx_callback );
  if (!silent) Serial.println( "Initialized I2S RX without DMA" );
  delay(5);//ZAB: delay(5000);
  // start the I2S RX
  I2SRx0.start();
  if (!silent) Serial.println( "Started I2S RX" );
  
  //For Battery Monitoring//
  analogReference(EXTERNAL);
  analogReadResolution(12);
  analogReadAveraging(32);
  bat = getInputVoltage();

  //ZAB: No need of this
  
  // Calculate feature size and which ones are selected
  for (int i = 0; i < NUM_FEATURES; i++) {
    // Consider only when corresponding bit is turned on.
    if ((enabledFeatures[i / 8] << (i % 8)) & B10000000)
    {
      FeatureID[chosen_features] = i;
      chosen_features++;
    }
  }

  // Enable LiPo charger's CHG read.
  pinMode(chargerCHG, INPUT_PULLUP);

  // Make sure to initialize BLEport at the end of setup
  BLEport.begin(9600);
  Serial.println( "Start the port. . ." );
}

void loop()
{
  //Serial.print("Entering loop..");
  bat = getInputVoltage();

  //Timer to send data regularly//
  currentmillis = millis();
  if (currentmillis - prevmillis >= datasendlimit) {
    prevmillis = currentmillis;
    datasendtime = true;
  }

  currenttime = millis();
  if (currenttime - prevtime >= 1) {
    prevtime = currenttime;
    dateyear = dateyear + 0.001;
  }

  //Robustness for data reception//
  buffnow = millis();
  if (bleBufferIndex > 0) {
    if (buffnow -  buffprev > datareceptiontimeout) {
      bleBufferIndex = 0;
    }
  } else {
    buffprev = buffnow;
  }

  /* -------------------------- USB Connection Check ----------------------- */
  if (bitRead(USB0_OTGSTAT, 5))
  {
    //      // Disconnected; this flag will briefly come up when USB gets disconnected.
    //      // Immediately put into RUN mode.
    if (currMode != RUN) { //If disconnected and NOT in RUN mode, set to run mode with idle state initialized
      BLEport.begin(9600);
      BLEport.flush();
      resetSampling(true);
      currMode = RUN;
      LEDColors c = BLUE_NOOP;
      changeStatusLED(c);
      currState = NOT_CUTTING;
    }
  }

  /*Serial.print("USB connection check complete. Starting BLE\n");*/

  /* ----------- Two-way Communication via BLE, only on RUN mode. ----------- */

  if (currMode == RUN)
  {
    while (BLEport.available() > 0)
    {
      bleBuffer[bleBufferIndex] = BLEport.read();
      bleBufferIndex++;
    }
    if (bleBufferIndex > 18)
    {
      Serial.println(bleBuffer);
      if (bleBuffer[0] == '0')
      {
        int newThres = 0, newThres2 = 0, newThres3 = 0;
        args_assigned = sscanf(bleBuffer, "%d-%d-%d-%d", &bleFeatureIndex, &newThres, &newThres2, &newThres3);
        didThresChange = true;
        bleBufferIndex = 0;
        featureNormalThreshold[bleFeatureIndex] = newThres;
        featureWarningThreshold[bleFeatureIndex] = newThres2;
        featureDangerThreshold[bleFeatureIndex] = newThres3;
      }
      else if (bleBuffer[0] == '1')      // receive the timestamp data from the hub
      {
        args_assigned2 = sscanf(bleBuffer, "%d:%d.%d", &date, &dateset, &dateyear1);
        dateyear = double(dateset) + double(dateyear1) / double(1000000);
        Serial.println(dateyear);
      }
      else if (bleBuffer[0] == '2')      // Wireless parameter setting
      {
        args_assigned2 = sscanf(bleBuffer, "%d:%d-%d-%d", &parametertag, &datasendlimit, &bluesleeplimit, &datareceptiontimeout);
        Serial.print("Data send limit is : ");
        Serial.println(datasendlimit);
      }
      else if (bleBuffer[0] == '3')      // Record button pressed - go into record mode to record FFTs
      {
        Serial.println("Record button Pressed!");
        show_record_fft('X');
        show_record_fft('Y');
        show_record_fft('Z');
      }
      else if (bleBuffer[0] == '4')      // Stop button pressed - go out of record mode back into RUN mode
      {
        Serial.print("Stop recording and sending FFTs");
      }

      bleBufferIndex = 0;
    }

  }//--- if (currMode == RUN)

  /* ------------- Hardwire Serial for DATA_COLLECTION commands ------------- */
  while (Serial.available() > 0)
  {
    characterRead = Serial.read();
    if (characterRead != '\n')
      wireBuffer.concat(characterRead);
  }


  if (wireBuffer.length() > 0)
  {
    if (currMode == RUN)
    {
      // Check the received info; iff data collection request, change the mode
      if (wireBuffer.indexOf(START_COLLECTION) > -1)
      {
        LEDColors c = CYAN_DATA;
        changeStatusLED(c);
        Serial.print(START_CONFIRM);
        resetSampling(false);
        currMode = DATA_COLLECTION;
      }
    } 
    
    if (currMode == DATA_COLLECTION) {
      Serial.println(wireBuffer);
      // Arange changed
      if (wireBuffer.indexOf("Arange") > -1)
      {
        int Arangelocation = wireBuffer.indexOf("Arange");
        String Arange2 = wireBuffer.charAt(Arangelocation + 7);
        int Arange = Arange2.toInt();
        switch (Arange) {
          case 0:
            Ascale = AFS_2G;
            break;
          case 1:
            Ascale = AFS_4G;
            break;
          case 2:
            Ascale = AFS_8G;
            break;
          case 3:
            Ascale = AFS_16G;
            break;
        }
        getAres();
      }

      // LED run color changed
      if (wireBuffer.indexOf("rgb") > -1)
      {
        int rbglocation = wireBuffer.indexOf("rgb");
        int Red = wireBuffer.charAt(rbglocation + 7) - 48;
        int Blue = wireBuffer.charAt(rbglocation + 8) - 48;
        int Green = wireBuffer.charAt(rbglocation + 9) - 48;
        if (Blue == 1) {
          digitalWrite(27, LOW);
        }
        else {
          digitalWrite(27, HIGH);
        }
        if (Red == 1) {
          digitalWrite(29, LOW);
        }
        else {
          digitalWrite(29, HIGH);
        }
        if (Green == 1) {
          digitalWrite(28, LOW);
        }
        else {
          digitalWrite(28, HIGH);
        }
      }


      //Acoustic sampling rate changed
      if (wireBuffer.indexOf("acosr") > -1)
      {
        int acosrLocation = wireBuffer.indexOf("acosr");
        int target_sample_A = wireBuffer.charAt(acosrLocation + 6) - 48;
        int target_sample_B = wireBuffer.charAt(acosrLocation + 7) - 48;
        TARGET_AUDIO_SAMPLE = (target_sample_A * 10 + target_sample_B) * 1000;
        DOWNSAMPLE_MAX = 48000 / TARGET_AUDIO_SAMPLE;

        //Re-initialize sampling counters
        accel_counter = 0;
        subsample_counter = 0;
      }


      //Acceleromter sampling rate changed
      if (wireBuffer.indexOf("accsr") > -1)
      {
        int accsrLocation = wireBuffer.indexOf("accsr");
        int target_sample_C = wireBuffer.charAt(accsrLocation + 6) - 48;
        int target_sample_D = wireBuffer.charAt(accsrLocation + 7) - 48;
        int target_sample_E = wireBuffer.charAt(accsrLocation + 8) - 48;
        int target_sample_F = wireBuffer.charAt(accsrLocation + 9) - 48;

        int target_sample_vib1 = (target_sample_C * 1000 + target_sample_D * 100 + target_sample_E * 10 + target_sample_F);
        switch (target_sample_vib1) {
          case 8:
            TARGET_ACCEL_SAMPLE = 7.8125;
            break;
          case 16:
            TARGET_ACCEL_SAMPLE = 15.625;
            break;
          case 31:
            TARGET_ACCEL_SAMPLE = 31.25;
            break;
          case 63:
            TARGET_ACCEL_SAMPLE = 62.5;
            break;
          case 125:
            TARGET_ACCEL_SAMPLE = 125;
            break;
          case 250:
            TARGET_ACCEL_SAMPLE = 250;
            break;
          case 500:
            TARGET_ACCEL_SAMPLE = 500;
            break;
          case 1000:
            TARGET_ACCEL_SAMPLE = 1000;
            break;
        }
        ACCEL_COUNTER_TARGET = TARGET_AUDIO_SAMPLE / TARGET_ACCEL_SAMPLE;
        accel_counter = 0;
        subsample_counter = 0;
      }
      // Check the received info; iff data collection finished, change the mode
      if (wireBuffer == END_COLLECTION) {
        LEDColors c = BLUE_NOOP;
        changeStatusLED(c);

        Serial.print(END_CONFIRM);
        currMode = RUN;

        Ascale = AFS_2G;
        getAres();

        TARGET_AUDIO_SAMPLE = 8000;
        DOWNSAMPLE_MAX = 48000 / TARGET_AUDIO_SAMPLE;

        TARGET_ACCEL_SAMPLE = 1000;
        ACCEL_COUNTER_TARGET = TARGET_AUDIO_SAMPLE / TARGET_ACCEL_SAMPLE;

        accel_counter = 0;
        subsample_counter = 0;
      }
    } // end of currMode == DATA_COLLECTION

    wireBuffer = "";  // Clear wire buffer
  }

  // After handling all state / mode changes, check if features need to be computed.
  if (currMode == RUN && compute_feature_now[buffer_compute_index])
  {
    //ZAB: Serial.println("Computing Buffers");
    compute_features();
  }
}

//Battery Status calculation function
int getInputVoltage() {
  float x = float(analogRead(39)) / float(1000);
  return int(-261.7 * x  + 484.06);
}

// Function for resetting the data collection routine.
// Need to call this before entering RUN and DATA_COLLECTION modes
void resetSampling(bool run_mode) {
  // Reset downsampling variables
  subsample_counter = 0;
  accel_counter = 0;

  // Reset feature variables
  audioSamplingCount = 0;
  accelSamplingCount = 0;
  restCount = 0;

  // Reset buffer management variables
  buffer_record_index = 0;
  buffer_compute_index = 0;
  compute_feature_now[0] = false;
  compute_feature_now[1] = false;
  record_feature_now[0] = true;
  record_feature_now[1] = true;

  if (run_mode) {
    TARGET_AUDIO_SAMPLE = AUDIO_FREQ;
  } else {
    TARGET_AUDIO_SAMPLE = AUDIO_FREQ;
  }

  ACCEL_COUNTER_TARGET = TARGET_AUDIO_SAMPLE / TARGET_ACCEL_SAMPLE;
  DOWNSAMPLE_MAX = 48000 / TARGET_AUDIO_SAMPLE;
}

// Re-format 24 bit signed integer to float for CFFT -- DEPRECATED.
// Expects input to be in 24 bit signed integer in Big Endian, as microphone does.
float signed24ToFloat(byte* input_ptr) {
  byte ptr[4] = {0x00, 0x00, 0x00, 0x00};

  // Little-endian order; ARM uses little-endian
  ptr[0] = input_ptr[2];
  ptr[1] = input_ptr[1];
  ptr[2] = input_ptr[0] & B01111111;
  ptr[3] = input_ptr[0] & B10000000;

  // Read the raw bytes as signed 32 bit integer then cast to float
  int32_t* int_ptr = (int32_t *)&ptr[0];
  return (float)(*int_ptr);
}

// Function to estimate memory availability programmatically.
  uint32_t FreeRam() {
  uint32_t stackTop;
  uint32_t heapTop;

  // current position of the stack.
  stackTop = (uint32_t) &stackTop;

  // current position of heap.
  void* hTop = malloc(1);
  heapTop = (uint32_t) hTop;
  free(hTop);

  // The difference is the free, available ram.
  return stackTop - heapTop;
}

int greenPin = 27;
int redPin = 29;
int bluePin = 28;

// Function that reflects LEDColors into statusLED.
// Will not have effect if statusLED is turned off.
void changeStatusLED(LEDColors color) {
  if (!statusLED) return;
  switch (color) {
    case RED_BAD:
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, HIGH);
      digitalWrite(bluePin, HIGH);
      break;
    case BLUE_NOOP:
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, HIGH);
      digitalWrite(bluePin, LOW);
      break;
    case GREEN_OK:
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, HIGH);
      break;
    case ORANGE_WARNING:
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, HIGH);
      break;
    case PURPLE_CHARGE:
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, HIGH);
      digitalWrite(bluePin, LOW);
      break;
    case CYAN_DATA:
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, LOW);
      break;
    case WHITE_NONE:
      digitalWrite(redPin, LOW);
      digitalWrite(greenPin, LOW);
      digitalWrite(bluePin, LOW);
      break;
    case SLEEP_MODE:
      digitalWrite(redPin, HIGH);
      digitalWrite(greenPin, HIGH);
      digitalWrite(bluePin, HIGH);
      break;
  }
}


//===================================================================================================================
//====== Set of useful function to access acceleration. gyroscope, magnetometer, and temperature data
//===================================================================================================================

void getAres() {
  switch (Ascale)
  {
    // Possible accelerometer scales (and their register bit settings) are:
    // 2 Gs (0011), 4 Gs (0101), 8 Gs (1000), and 16 Gs  (1100).
    // BMX055 ACC data is signed 12 bit
    // CHANGE: MPU data is signed 16 bit??
    case AFS_2G:
      aRes = 2.0 / 32768.0; // change from 2^11 to 2^15
      break;
    case AFS_4G:
      aRes = 4.0 / 32768.0; // is this even right? should it be 2^16?
      break;
    case AFS_8G:
      aRes = 8.0 / 32768.0;
      break;
    case AFS_16G:
      aRes = 16.0 / 32768.0;
      break;
  }
}

void readAccelData(int16_t * destination)
{
  uint8_t rawData[6];  // x/y/z accel register data stored here
  readBytes(MPU9250_ADDRESS, ACCEL_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers into data array
  destination[0] = ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a signed 16-bit value
  destination[1] = ((int16_t)rawData[2] << 8) | rawData[3] ;
  destination[2] = ((int16_t)rawData[4] << 8) | rawData[5] ;
  //Serial.printf("Accel data read: %d, %d, %d", destination[0], destination[1], destination[2]);
}

void SENtralPassThroughMode()
{
  // First put SENtral in standby mode
  uint8_t c = readByte(EM7180_ADDRESS, EM7180_AlgorithmControl);
  writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, c | 0x01);
  //  c = readByte(EM7180_ADDRESS, EM7180_AlgorithmStatus);
  //  Serial.print("c = "); Serial.println(c);
  // Verify standby status
  // if(readByte(EM7180_ADDRESS, EM7180_AlgorithmStatus) & 0x01) {
  if (!silent) Serial.println("SENtral in standby mode");
  // Place SENtral in pass-through mode
  writeByte(EM7180_ADDRESS, EM7180_PassThruControl, 0x01);
  if (readByte(EM7180_ADDRESS, EM7180_PassThruStatus) & 0x01) {
    if (!silent) Serial.println("SENtral in pass-through mode");
  }
  else {
    Serial.println("ERROR! SENtral not in pass-through mode!");
  }

}

void initMPU9250()
{
  // wake up device
  writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x00); // Clear sleep mode bit (6), enable all sensors
  delay(100); // Wait for all registers to reset

  // get stable time source
  writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x01);  // Auto select clock source to be PLL gyroscope reference if ready else
  delay(200);

  // disable gyroscope
  writeByte(MPU9250_ADDRESS, PWR_MGMT_2, 0x07);
  
  // Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
  writeByte(MPU9250_ADDRESS, SMPLRT_DIV, 0x00);  // Use a 200 Hz rate; a rate consistent with the filter update rate
  // determined inset in CONFIG above

  // Set accelerometer full-scale range configuration
  byte c = readByte(MPU9250_ADDRESS, ACCEL_CONFIG); // get current ACCEL_CONFIG register value
  // c = c & ~0xE0; // Clear self-test bits [7:5]
  c = c & ~0x18;  // Clear accelerometer full scale bits [4:3]
  c = c | Ascale << 3; // Set full scale range for the accelerometer
  writeByte(MPU9250_ADDRESS, ACCEL_CONFIG, c); // Write new ACCEL_CONFIG register value

  // Set accelerometer sample rate configuration
  // It is possible to get a 4 kHz sample rate from the accelerometer by choosing 1 for
  // accel_fchoice_b bit [3]; in this case the bandwidth is 1.13 kHz

  //MODIFIED SAMPLE RATE
  c = readByte(MPU9250_ADDRESS, ACCEL_CONFIG2); // get current ACCEL_CONFIG2 register value
  c = c & ~0x0F; // Clear accel_fchoice_b (bit 3) and A_DLPFG (bits [2:0])
  c = c | 0x03;  // MODIFIED: Instead of 1k rate, 41hz bandwidth: we are doing 4k rate, 1.13K bandwidth
  writeByte(MPU9250_ADDRESS, ACCEL_CONFIG2, c); // Write new ACCEL_CONFIG2 register value
  // The accelerometer, gyro, and thermometer are set to 1 kHz sample rates,
  // but all these rates are further reduced by a factor of 5 to 200 Hz because of the SMPLRT_DIV setting

  // Configure Interrupts and Bypass Enable
  // Set interrupt pin active high, push-pull, hold interrupt pin level HIGH until interrupt cleared,
  // clear on read of INT_STATUS, and enable I2C_BYPASS_EN so additional chips
  // can join the I2C bus and all can be controlled by the Arduino as master
  writeByte(MPU9250_ADDRESS, INT_PIN_CFG, 0x22);
  writeByte(MPU9250_ADDRESS, INT_ENABLE, 0x01);  // Enable data ready (bit 0) interrupt
  delay(100);
  Serial.print("MPU-9250 initialized successfully.\n\n\n");
}

// simple function to scan for I2C devices on the bus
void I2Cscan()
{
  // scan for i2c devices
  byte error, address;
  int nDevices;

  if (!silent) Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 129; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      if (!silent) {
        Serial.print("I2C device found at address 0x");
        if (address < 16)
          Serial.print("0");
        Serial.print(address, HEX);
        Serial.println("  !");
      }

      nDevices++;
    }
    else if (error == 4)
    {
      if (!silent) {
        Serial.print("Unknow error at address 0x");
        if (address < 16)
          Serial.print("0");
        Serial.println(address, HEX);
      }
    }
  }
  if (nDevices == 0) {
    LEDColors c = WHITE_NONE;
    changeStatusLED(c);
    if (!silent) Serial.println("No I2C devices found\n");
  }
  else {
    if (!silent) Serial.println("done\n");
  }
}


// I2C read/write functions for the MPU9250 and AK8963 sensors

void writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
{
  Wire.beginTransmission(address);  // Initialize the Tx buffer
  Wire.write(subAddress);           // Put slave register address in Tx buffer
  Wire.write(data);                 // Put data in Tx buffer
  Wire.endTransmission();           // Send the Tx buffer
}

uint8_t readByte(uint8_t address, uint8_t subAddress)
{
  uint8_t data; // `data` will store the register data
  Wire.beginTransmission(address);         // Initialize the Tx buffer
  Wire.write(subAddress);                   // Put slave register address in Tx buffer
  Wire.endTransmission(I2C_NOSTOP);        // Send the Tx buffer, but send a restart to keep connection alive
  //  Wire.endTransmission(false);             // Send the Tx buffer, but send a restart to keep connection alive
  //  Wire.requestFrom(address, 1);  // Read one byte from slave register address
  Wire.requestFrom(address, (size_t) 1);   // Read one byte from slave register address
  data = Wire.read();                      // Fill Rx buffer with result
  return data;                             // Return data read from slave register
}

void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest)
{
  Wire.beginTransmission(address);   // Initialize the Tx buffer
  Wire.write(subAddress);            // Put slave register address in Tx buffer
  Wire.endTransmission(I2C_NOSTOP);  // Send the Tx buffer, but send a restart to keep connection alive
  //  Wire.endTransmission(false);       // Send the Tx buffer, but send a restart to keep connection alive
  uint8_t i = 0;
  //        Wire.requestFrom(address, count);  // Read bytes from slave register address
  Wire.requestFrom(address, (size_t) count);  // Read bytes from slave register address
  while (Wire.available()) {
    dest[i++] = Wire.read();
  }         // Put read results in the Rx buffer
}
