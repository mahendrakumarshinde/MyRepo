/*  Infinite Uptime BLE Module Firmware  */
#include <i2c_t3.h>
#include <SPI.h>
#include <i2s.h> /* I2S digital audio */

#define ARM_MATH_CM4
#include <arm_math.h> /* CMSIS-DSP library for RFFT */
#include "constants.h" /* Register addresses and hamming window presets  */

//============================= Constant Variables =============================
const uint16_t AUDIO_FREQ = 8000; // Reduce RUN frequency if needed.
const uint32_t RESTING_INTERVAL = 0;                    // Inter-batch gap
const uint16_t RUN_ACCEL_AUDIO_RATIO = 8;  // AUDIO_FREQ / TARGET_ACCEL_SAMPLE;
const uint8_t NUM_FEATURES = 6;    // Total number of features
const uint8_t NUM_TD_FEATURES = 2; // Total number of frequency domain features
const uint16_t MAX_INTERVAL_ACCEL = 256; //512
const uint16_t MAX_INTERVAL_AUDIO = 2048; //4096
// These should only change when list of features are changed.
const byte audioFDFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B00000100};
const byte accelFDFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B00111000};
// RFFT Variables
const uint16_t ACCEL_NFFT = MAX_INTERVAL_ACCEL;
const uint16_t AUDIO_NFFT = MAX_INTERVAL_AUDIO;

// Modes of Operation String Constants
const String START_COLLECTION = "IUCMD_START";
const String START_CONFIRM = "IUOK_START";
const String END_COLLECTION = "IUCMD_END";
const String END_CONFIRM = "IUOK_END";

//====================== Module Configuration Variables ========================

#define CLOCK_TYPE         (I2S_CLOCK_48K_INTERNAL)     // I2S clock
bool statusLED = true;                                  // Status LED ON/OFF
String MAC_ADDRESS = "88:4A:EA:69:DE:A0";
uint16_t TARGET_AUDIO_SAMPLE = AUDIO_FREQ;          // Audio frequency
uint16_t TARGET_ACCEL_SAMPLE = 1000;              // Accel frequency

// NOTE: Do NOT change the following variables unless you know what you're doing
uint16_t ACCEL_COUNTER_TARGET = TARGET_AUDIO_SAMPLE / TARGET_ACCEL_SAMPLE;
uint16_t DOWNSAMPLE_MAX = 48000 / TARGET_AUDIO_SAMPLE;
uint16_t subsample_counter = 0;
uint16_t accel_counter = 0;

//Temperature variables
float tempSensitivity = 333.87;
float temperature = 0.0;
bool temperature_ticks = true; // Counter for temperature reading update
//==============================================================================
//====================== Feature Computation Variables =========================
//==============================================================================

// TIME DOMAIN FEATURES
// NOTE: Intervals are checked based on AUDIO, not ACCEL. Read vibration energy
//       code to understand the sampling mechanism.

char rec_axis = 'X';  // To send data for record function
int chosen_features = 0;// ZAB: To send data over BLE
bool recordmode = false;

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

// Byte representing which features are enabled. Feature index 0 is enabled by default.
byte enabledFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B11111100};
int FeatureID[NUM_FEATURES];

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

int greenPin = 27;
int redPin = 29;
int bluePin = 28;

bool orientationChange = false;

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
q15_t accel_x_batch[2][MAX_INTERVAL_ACCEL];
q15_t accel_y_batch[2][MAX_INTERVAL_ACCEL];
q15_t accel_z_batch[2][MAX_INTERVAL_ACCEL];

// Audio Buffers
q15_t audio_batch[2][MAX_INTERVAL_AUDIO];

// RFFT Output and Magnitude Buffers
q15_t rfft_audio_buffer [AUDIO_NFFT * 2];
q15_t rfft_accel_buffer [ACCEL_NFFT * 2];

//TODO: May be able to use just one instance. Check later.
arm_rfft_instance_q15 audio_rfft_instance;
arm_cfft_radix4_instance_q15 audio_cfft_instance;

arm_rfft_instance_q15 accel_rfft_instance;
arm_cfft_radix4_instance_q15 accel_cfft_instance;

// Velocity variables
float v_x = 0, v_y = 0, v_z = 0;
float u_x = 0, u_y = 0, u_z = 0;
float t0 = 0.001; // 1 millisec = 1 / 1000Hz

//==============================================================================
//============================ Internal Variables ==============================
//==============================================================================


// Specify sensor full scale
uint8_t Gscale = GFS_250DPS;
uint8_t Ascale = AFS_2G;           // set accel full scale
uint8_t ACCBW  = 0x08 & ABW_1000Hz;  // Choose bandwidth for accelerometer

// Pin definitions
int myLed       = 13;  // LED on the Teensy 3.1/3.2
int chargerCHG  = 2;   // CHG pin to detect charging status

// MPU9250 variables
int16_t accelCount[3];              // Stores the 16-bit signed accelerometer sensor output
float accelBias[3] = {0, 0, 0};  // Bias corrections for gyro, accelerometer, mag
int16_t gyroCount[3];               // Stores2 the 16-bit signed gyro sensor output
float gyroBias[3] = {0, 0, 0};      // Bias corrections for gyro

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
boolean rubbish = false;
int date = 0;
int dateset = 0;
double dateyear = 0;
double bleDateyear = 0;
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
int bluesleeplimit = 5000;
int bluetimerstart = 0;

//----------------------- Temperature calculation ------------------------
// Specify BMP280 configuration
uint8_t BMP280Posr = P_OSR_16, BMP280Tosr = T_OSR_02, BMP280Mode = normal, BMP280IIRFilter = BW0_042ODR, BMP280SBy = t_62_5ms;     // set pressure amd temperature output data rate
// t_fine carries fine temperature as global value for BMP280
int32_t t_fine;
// BMP280 compensation parameters
uint16_t dig_T1, dig_P1;
int16_t  dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
double BMP280Temperature, BMP280Pressure; // stores MS5637 pressures sensor pressure and temperature
//---------------------------Gyro Calculation------------------------------
int gX = 0, gY = 0, gZ = 0;
int p_avgGX = 0, p_avgGY = 0, p_avgGZ = 0;
int aX = 0, aY = 0, aZ = 0; //For average calculation

/*==============================================================================
  ====================== Feature Computation Caller ============================
  ==============================================================================*/

// Convert acceleration from LSB to m/s2
float LSB_to_ms2(int16_t accelLSB)
{
  return ((float)accelLSB / 16384.0 * 9.8);
}
  
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
  if (rec_axis == 'X')
  {
    BLEport.print(",X");
    for (int i = 0; i < MAX_INTERVAL_ACCEL; i++) {
      BLEport.print(",");
      BLEport.print(accel_x_batch[buffer_compute_index][i]);
      BLEport.flush();
    }
  }
  else if (rec_axis == 'Y')
  {
    BLEport.print(",Y");
    for (int i = 0; i < MAX_INTERVAL_ACCEL; i++) {
      BLEport.print(",");
      //BLEport.print(accel_y_batch[buffer_compute_index][i]);
      BLEport.print(accel_y_batch[buffer_compute_index][i]);
      BLEport.flush();
    }
  }
  else if (rec_axis == 'Z')
  {
    BLEport.print(",Z");
    for (int i = 0; i < MAX_INTERVAL_ACCEL; i++) {
      BLEport.print(",");
      BLEport.print(accel_z_batch[buffer_compute_index][i]);
      BLEport.flush();
    }
  }
  BLEport.print(";");
  BLEport.flush();
}

//ZAB: function to send feature data
void send_features_to_BLE(uint8_t level)
{
  BLEport.print(MAC_ADDRESS);
  if (level == 0)
  {
    BLEport.print(",00,");
  }
  else if (level == 1)
  {
    BLEport.print(",01,");
  }
  else if (level == 2)
  {
    BLEport.print(",02,");
  }
  else if (level == 3)
  {
    BLEport.print(",03,");
  }

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
  if (!recordmode) {
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
  int16_t sample_lim = 1000; // Offset for first500 samples

  for (i = 0; i < sample_lim; i++)
  {
    readAccelData(accelCount);
    accelBias[0] += accelCount[0];
    accelBias[1] += accelCount[1];
    accelBias[2] += accelCount[2];
  }
  accelBias[0] /= sample_lim;
  accelBias[1] /= sample_lim;
  accelBias[2] /= sample_lim;
}

void calc_velocityOffset()
{
  int16_t i = 0;
  int16_t sample_lim = 500; // Offset for first500 samples

  for (i = 0; i < sample_lim; i++)
  {
    readAccelData(accelCount);
    u_x += LSB_to_ms2(accelCount[0] - (int16_t)accelBias[0]) * t0;
    u_y += LSB_to_ms2(accelCount[1] - (int16_t)accelBias[1]) * t0;
    u_z += LSB_to_ms2(accelCount[2] - (int16_t)accelBias[2]) * t0;
  }
  u_x /= sample_lim;
  u_y /= sample_lim;
  u_z /= sample_lim;
}

//ZAB: Change for 512 samples
float feature_energy() {
  q15_t ave_x = 0, ave_y = 0, ave_z = 0, ene = 0;

  // Calculate average
  arm_mean_q15(&accel_x_batch[buffer_compute_index][0], MAX_INTERVAL_ACCEL, &ave_x);
  arm_mean_q15(&accel_y_batch[buffer_compute_index][0], MAX_INTERVAL_ACCEL, &ave_y);
  arm_mean_q15(&accel_z_batch[buffer_compute_index][0], MAX_INTERVAL_ACCEL, &ave_z);

  // Calculate energy.
  for (int i = 0; i < MAX_INTERVAL_ACCEL; i++) {
    ene += sq(LSB_to_ms2(accel_x_batch[buffer_compute_index][i] - ave_x))
           +  sq(LSB_to_ms2(accel_y_batch[buffer_compute_index][i] - ave_y))
           +  sq(LSB_to_ms2(accel_z_batch[buffer_compute_index][i] - ave_z));
  }
  //ZAB: Serial.printf("Energy = %f", ene);
  return ene;
}

float velocityX()      // Index 1
{
  float v_x2 = 0.0;
  int16_t j = 0;
  float u_x0 = 0.0;
  v_x = 0.0;
  for(j = 0; j < MAX_INTERVAL_ACCEL; j++)
  {
      v_x2 += sq(LSB_to_ms2(accel_x_batch[buffer_compute_index][j]));
      u_x0 += (float)accel_x_batch[buffer_compute_index][j];
  }
  // To optimize computation time
  v_x2 *= sq(t0);
  u_x0 = LSB_to_ms2(u_x0) * t0;
  
  v_x = (v_x2 /MAX_INTERVAL_ACCEL) - sq(u_x); //Normalizing the velocity
  u_x = u_x0 / MAX_INTERVAL_ACCEL;  //Normalizing factor for next iteration
  
  v_x = sqrt(v_x);  //RMS Value
  v_x *= 1000; // Upscaling the velocity
  Serial.print("V_X: ");
  Serial.print(v_x);
  return v_x;
}
float velocityY()      // Index 2
{
  float v_y2 = 0.0;
  int16_t j = 0;
  float u_y0 = 0.0;
  v_y = 0.0;
  for(j = 0; j < MAX_INTERVAL_ACCEL; j++)
  {
    v_y2 += sq(LSB_to_ms2(accel_y_batch[buffer_compute_index][j]));
    u_y0 += (float)accel_y_batch[buffer_compute_index][j];
  }
  // To optimize computation time
  v_y2 *= sq(t0);
  u_y0 = LSB_to_ms2(u_y0) * t0;
  
  v_y = (v_y2 /MAX_INTERVAL_ACCEL) - sq(u_y); //Normalizing the velocity
  u_y = u_y0 / MAX_INTERVAL_ACCEL;  //Normalizing factor for next iteration
    
  v_y = sqrt(v_y);  //RMS Value
  v_y *= 1000; // Upscaling the velocity
  Serial.print("V_Y: ");
  Serial.print(v_y);
  return v_y;
}
float velocityZ()      // Index 3
{
  float v_z2 = 0.0;
  int16_t j = 0;
  float u_z0 = 0.0;
  v_z = 0.0;
  for(j = 0; j < MAX_INTERVAL_ACCEL; j++)
  {
    v_z2 += sq(LSB_to_ms2(accel_z_batch[buffer_compute_index][j]));
    u_z0 += (float)accel_z_batch[buffer_compute_index][j];
  }
  // To optimize computation time
  v_z2 *= sq(t0);
  u_z0 = LSB_to_ms2(u_z0) * t0;
  
  v_z = (v_z2 /MAX_INTERVAL_ACCEL) - sq(u_z); //Normalizing the velocity
  u_z = u_z0 / MAX_INTERVAL_ACCEL;  //Normalizing factor for next iteration
    
  v_z = sqrt(v_z);  //RMS Value
  v_z *= 1000; // Upscaling the velocity
  Serial.print("V_Z: ");
  Serial.println(v_z);
  return v_z;
}

float currentTemparature() // Index 4
{
 // if (temperature_ticks) //Update temperature every (30 x 0.5 secs)
  {
//    int16_t tempCount = 0;
//    //Serial.println(accel_x_batch[0][0]);
//    tempCount = readTempData();  // Read the adc values
//    temperature = ((float) tempCount) / tempSensitivity + 21.0; // Temperature in degrees Centigrade
//    temperature_ticks = false;
  
//  int32_t rawPress;
//  rawPress =  readBMP280Pressure();
//  BMP280Pressure = (float) bmp280_compensate_P(rawPress)/25600.; // Prwssure in mbar
  
    int32_t rawTemp = readBMP280Temperature();
//    Serial.println(rawTemp);
    BMP280Temperature = (float) bmp280_compensate_T(rawTemp)/100.;
//    Serial.println(BMP280Temperature);
  }
  return BMP280Temperature;
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
      int avgGX = gX / MAX_INTERVAL_ACCEL; 
      int avgGY = gY / MAX_INTERVAL_ACCEL;
      int avgGZ = gZ / MAX_INTERVAL_ACCEL;

      int avgAX = aX / MAX_INTERVAL_ACCEL; 
      int avgAY = aY / MAX_INTERVAL_ACCEL;
      int avgAZ = aZ / MAX_INTERVAL_ACCEL;
      aX = 0, aY = 0, aZ = 0;
  //    Serial.printf("ax: %d, ay: %d, az: %d\n", avgAX, avgAY, avgAZ);
  //    Serial.printf("bx: %f, by: %f, bz: %f\n", accelBias[0], accelBias[1], accelBias[2]);
      gX = 0, gY = 0, gZ = 0;
  //    Serial.printf("x: %d, y: %d, z: %d\n", avgGX, avgGY, avgGZ);
      if((abs(p_avgGX - avgGX) > 200) || (abs(p_avgGY - avgGY) > 200) || (abs(p_avgGZ - avgGZ) > 200))
      {
        calc_accelBias();
        u_x = 0, u_y = 0, u_z = 0;
        calc_velocityOffset();
      }
      if((abs(p_avgGX - avgGX) < 3) && (abs(p_avgGY - avgGY) < 3) && (abs(p_avgGZ - avgGZ) < 3))
      {
        // For no orientation change and stable position
        if((v_x > 0.05) || (v_y > 0.05) || (v_z > 0.05)) 
        {
          if(((abs(avgAX - (int)accelBias[0]) > 500) && (abs(avgAY - (int)accelBias[1]) > 500) && (abs(avgAZ - (int)accelBias[2]) > 500)) ||
            ((abs(avgAX - (int)accelBias[0]) < 50) && (abs(avgAY - (int)accelBias[1]) < 50) && (abs(avgAZ - (int)accelBias[2]) < 50)))
          {
            calc_accelBias();
            u_x = 0, u_y = 0, u_z = 0;
            calc_velocityOffset();
     //       Serial.println("Bias calculation");
          }
        }
      }
      if((abs(p_avgGX - avgGX) < 1) && (abs(p_avgGY - avgGY) < 1) && (abs(p_avgGZ - avgGZ) < 1))
      {
        calc_accelBias();
        u_x = 0, u_y = 0, u_z = 0;
        calc_velocityOffset();
      }
      p_avgGX = avgGX;
      p_avgGY = avgGY;
      p_avgGZ = avgGZ;
//      Serial.printf("x: %d, y: %d, z: %d\n", gX, gY, gZ);
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
        accel_x_batch[buffer_record_index][accelSamplingCount] = accelCount[0] - (int16_t)accelBias[0];
        accel_y_batch[buffer_record_index][accelSamplingCount] = accelCount[1] - (int16_t)accelBias[1];
        accel_z_batch[buffer_record_index][accelSamplingCount] = accelCount[2] - (int16_t)accelBias[2];
        aX += accelCount[0];
        aY += accelCount[1];
        aZ += accelCount[2];
        //Serial.printf("x: %d, y: %d, z: %d\n", accelCount[0], accelCount[1], accelCount[2]);
        readGyroData(gyroCount);
        gX += gyroCount[0];
        gY += gyroCount[1];
        gZ += gyroCount[2];
       // Serial.printf("x: %d, y: %d, z: %d\n", gyroCount[0], gyroCount[1], gyroCount[2]);
//        Serial.printf("batch: %d, cnt: %d, bias: %d\n", accel_x_batch[buffer_record_index][accelSamplingCount], accelCount[0], accelBias[0]);

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
    //calibrateMPU9250(gyroBias, accelBias);
    calc_accelBias();
    calc_velocityOffset();
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

  // Pressure and Temperature sensor
  // Read the WHO_AM_I register of the BMP280 this is a good test of communication
  byte f = readByte(BMP280_ADDRESS, BMP280_ID);  // Read WHO_AM_I register for BMP280
//  Serial.print("BMP280 "); 
//  Serial.print("I AM "); 
//  Serial.print(f, HEX); 
//  Serial.print(" I should be "); 
//  Serial.println(0x58, HEX);
//  Serial.println(" ");

  delay(1000); 

  writeByte(BMP280_ADDRESS, BMP280_RESET, 0xB6); // reset BMP280 before initilization
  delay(100);

  BMP280Init(); // Initialize BMP280 altimeter
//  Serial.println("BMP280 Calibration coeficients:");
//  Serial.print("dig_T1 ="); 
//  Serial.println(dig_T1);
//  Serial.print("dig_T2 ="); 
//  Serial.println(dig_T2);
//  Serial.print("dig_T3 ="); 
//  Serial.println(dig_T3);
//  Serial.print("dig_P1 ="); 
//  Serial.println(dig_P1);
//  Serial.print("dig_P2 ="); 
//  Serial.println(dig_P2);
//  Serial.print("dig_P3 ="); 
//  Serial.println(dig_P3);
//  Serial.print("dig_P4 ="); 
//  Serial.println(dig_P4);
//  Serial.print("dig_P5 ="); 
//  Serial.println(dig_P5);
//  Serial.print("dig_P6 ="); 
//  Serial.println(dig_P6);
//  Serial.print("dig_P7 ="); 
//  Serial.println(dig_P7);
//  Serial.print("dig_P8 ="); 
//  Serial.println(dig_P8);
//  Serial.print("dig_P9 ="); 
//  Serial.println(dig_P9);
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
  dateyear = bleDateyear + (currenttime - prevtime)/1000.0;
  

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
    while (BLEport.available() > 0 && bleBufferIndex < 19)
    {
      bleBuffer[bleBufferIndex] = BLEport.read();
      bleBufferIndex++;
    }
    if (bleBufferIndex > 18)
    {
      //int args_assigned = 0;
      //Serial.println(bleBuffer);
      if (bleBuffer[0] == '0')
      {
        int newThres = 0, newThres2 = 0, newThres3 = 0;
        //args_assigned = 
        sscanf(bleBuffer, "%d-%d-%d-%d", &bleFeatureIndex, &newThres, &newThres2, &newThres3);
        didThresChange = true;
        bleBufferIndex = 0;
        featureNormalThreshold[bleFeatureIndex] = newThres;
        featureWarningThreshold[bleFeatureIndex] = newThres2;
        featureDangerThreshold[bleFeatureIndex] = newThres3;
      }
      else if (bleBuffer[0] == '1')      // receive the timestamp data from the hub
      {
        //args_assigned = 
        sscanf(bleBuffer, "%d:%d.%d", &date, &dateset, &dateyear1);
     //   Serial.println(dateyear);
        bleDateyear = double(dateset) + double(dateyear1) / double(1000000);
        prevtime = millis();
      // Serial.println(bleBuffer);
      }
      else if (bleBuffer[0] == '2')      // Wireless parameter setting
      {
        //args_assigned = 
        sscanf(bleBuffer, "%d:%d-%d-%d", &parametertag, &datasendlimit, &bluesleeplimit, &datareceptiontimeout);
        Serial.print("Data send limit is : ");
        Serial.println(datasendlimit);
      }
      else if (bleBuffer[0] == '3')      // Record button pressed - go into record mode to record FFTs
      {
       // accel_rfft();
        Serial.println("Record button Pressed!");
        recordmode = true;
        delay(1);
        show_record_fft('X');
        BLEport.flush();
        delay(1);
        show_record_fft('Y');
        BLEport.flush();
        delay(1);
        show_record_fft('Z');
        BLEport.flush();
        delay(1);
        recordmode = false;
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

void readGyroData(int16_t * destination)
{
  uint8_t rawData[6];  // x/y/z gyro register data stored here
  readBytes(MPU9250_ADDRESS, GYRO_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers sequentially into data array
  destination[0] = ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a signed 16-bit value
  destination[1] = ((int16_t)rawData[2] << 8) | rawData[3] ;  
  destination[2] = ((int16_t)rawData[4] << 8) | rawData[5] ; 
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
  
 // Configure Gyro and Thermometer
 // Disable FSYNC and set thermometer and gyro bandwidth to 41 and 42 Hz, respectively; 
 // minimum delay time for this setting is 5.9 ms, which means sensor fusion update rates cannot
 // be higher than 1 / 0.0059 = 170 Hz
 // DLPF_CFG = bits 2:0 = 011; this limits the sample rate to 1000 Hz for both
 // With the MPU9250, it is possible to get gyro sample rates of 32 kHz (!), 8 kHz, or 1 kHz
  writeByte(MPU9250_ADDRESS, CONFIG, 0x03);  

 // Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
  writeByte(MPU9250_ADDRESS, SMPLRT_DIV, 0x04);  // Use a 200 Hz rate; a rate consistent with the filter update rate 
                                    // determined inset in CONFIG above
 
 // Set gyroscope full scale range
 // Range selects FS_SEL and AFS_SEL are 0 - 3, so 2-bit values are left-shifted into positions 4:3
  uint8_t c = readByte(MPU9250_ADDRESS, GYRO_CONFIG); // get current GYRO_CONFIG register value
 // c = c & ~0xE0; // Clear self-test bits [7:5] 
  c = c & ~0x02; // Clear Fchoice bits [1:0] 
  c = c & ~0x18; // Clear AFS bits [4:3]
  c = c | Gscale << 3; // Set full scale range for the gyro
 // c =| 0x00; // Set Fchoice for the gyro to 11 by writing its inverse to bits 1:0 of GYRO_CONFIG
  writeByte(MPU9250_ADDRESS, GYRO_CONFIG, c ); // Write new GYRO_CONFIG value to register
  
 // Set accelerometer full-scale range configuration
  c = readByte(MPU9250_ADDRESS, ACCEL_CONFIG); // get current ACCEL_CONFIG register value
 // c = c & ~0xE0; // Clear self-test bits [7:5] 
  c = c & ~0x18;  // Clear AFS bits [4:3]
  c = c | Ascale << 3; // Set full scale range for the accelerometer 
  writeByte(MPU9250_ADDRESS, ACCEL_CONFIG, c); // Write new ACCEL_CONFIG register value

 // Set accelerometer sample rate configuration
 // It is possible to get a 4 kHz sample rate from the accelerometer by choosing 1 for
 // accel_fchoice_b bit [3]; in this case the bandwidth is 1.13 kHz
  c = readByte(MPU9250_ADDRESS, ACCEL_CONFIG2); // get current ACCEL_CONFIG2 register value
  c = c & ~0x0F; // Clear accel_fchoice_b (bit 3) and A_DLPFG (bits [2:0])  
  c = c | 0x03;  // Set accelerometer rate to 1 kHz and bandwidth to 41 Hz
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
}

// Function which accumulates gyro and accelerometer data after device initialization. It calculates the average
// of the at-rest readings and then loads the resulting offsets into accelerometer and gyro bias registers.
void calibrateMPU9250(float * dest1, float * dest2)
{  
  uint8_t data[12]; // data array to hold accelerometer and gyro x, y, z, data
  uint16_t ii, packet_count, fifo_count;
  int32_t gyro_bias[3]  = {0, 0, 0}, accel_bias[3] = {0, 0, 0};
  
 // reset device
  writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
  delay(100);
   
 // get stable time source; Auto select clock source to be PLL gyroscope reference if ready 
 // else use the internal oscillator, bits 2:0 = 001
  writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x01);  
  writeByte(MPU9250_ADDRESS, PWR_MGMT_2, 0x00);
  delay(200);                                    

// Configure device for bias calculation
  writeByte(MPU9250_ADDRESS, INT_ENABLE, 0x00);   // Disable all interrupts
  writeByte(MPU9250_ADDRESS, FIFO_EN, 0x00);      // Disable FIFO
  writeByte(MPU9250_ADDRESS, PWR_MGMT_1, 0x00);   // Turn on internal clock source
  writeByte(MPU9250_ADDRESS, I2C_MST_CTRL, 0x00); // Disable I2C master
  writeByte(MPU9250_ADDRESS, USER_CTRL, 0x00);    // Disable FIFO and I2C master modes
  writeByte(MPU9250_ADDRESS, USER_CTRL, 0x0C);    // Reset FIFO and DMP
  delay(15);
  
// Configure MPU6050 gyro and accelerometer for bias calculation
  writeByte(MPU9250_ADDRESS, CONFIG, 0x01);      // Set low-pass filter to 188 Hz
  writeByte(MPU9250_ADDRESS, SMPLRT_DIV, 0x00);  // Set sample rate to 1 kHz
  writeByte(MPU9250_ADDRESS, GYRO_CONFIG, 0x00);  // Set gyro full-scale to 250 degrees per second, maximum sensitivity
  writeByte(MPU9250_ADDRESS, ACCEL_CONFIG, 0x00); // Set accelerometer full-scale to 2 g, maximum sensitivity
 
  uint16_t  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
  uint16_t  accelsensitivity = 16384;  // = 16384 LSB/g

    // Configure FIFO to capture accelerometer and gyro data for bias calculation
  writeByte(MPU9250_ADDRESS, USER_CTRL, 0x40);   // Enable FIFO  
  writeByte(MPU9250_ADDRESS, FIFO_EN, 0x78);     // Enable gyro and accelerometer sensors for FIFO  (max size 512 bytes in MPU-9150)
  delay(40); // accumulate 40 samples in 40 milliseconds = 480 bytes

// At end of sample accumulation, turn off FIFO sensor read
  writeByte(MPU9250_ADDRESS, FIFO_EN, 0x00);        // Disable gyro and accelerometer sensors for FIFO
  readBytes(MPU9250_ADDRESS, FIFO_COUNTH, 2, &data[0]); // read FIFO sample count
  fifo_count = ((uint16_t)data[0] << 8) | data[1];
  packet_count = fifo_count/12;// How many sets of full gyro and accelerometer data for averaging
  
  for (ii = 0; ii < packet_count; ii++) {
    int16_t accel_temp[3] = {0, 0, 0}, gyro_temp[3] = {0, 0, 0};
    readBytes(MPU9250_ADDRESS, FIFO_R_W, 12, &data[0]); // read data for averaging
    accel_temp[0] = (int16_t) (((int16_t)data[0] << 8) | data[1]  ) ;  // Form signed 16-bit integer for each sample in FIFO
    accel_temp[1] = (int16_t) (((int16_t)data[2] << 8) | data[3]  ) ;
    accel_temp[2] = (int16_t) (((int16_t)data[4] << 8) | data[5]  ) ;    
    gyro_temp[0]  = (int16_t) (((int16_t)data[6] << 8) | data[7]  ) ;
    gyro_temp[1]  = (int16_t) (((int16_t)data[8] << 8) | data[9]  ) ;
    gyro_temp[2]  = (int16_t) (((int16_t)data[10] << 8) | data[11]) ;
    
    accel_bias[0] += (int32_t) accel_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
    accel_bias[1] += (int32_t) accel_temp[1];
    accel_bias[2] += (int32_t) accel_temp[2];
    gyro_bias[0]  += (int32_t) gyro_temp[0];
    gyro_bias[1]  += (int32_t) gyro_temp[1];
    gyro_bias[2]  += (int32_t) gyro_temp[2];
            
}
    accel_bias[0] /= (int32_t) packet_count; // Normalize sums to get average count biases
    accel_bias[1] /= (int32_t) packet_count;
    accel_bias[2] /= (int32_t) packet_count;
    gyro_bias[0]  /= (int32_t) packet_count;
    gyro_bias[1]  /= (int32_t) packet_count;
    gyro_bias[2]  /= (int32_t) packet_count;
    
  if(accel_bias[2] > 0L) {accel_bias[2] -= (int32_t) accelsensitivity;}  // Remove gravity from the z-axis accelerometer bias calculation
  else {accel_bias[2] += (int32_t) accelsensitivity;}
   
// Construct the gyro biases for push to the hardware gyro bias registers, which are reset to zero upon device startup
  data[0] = (-gyro_bias[0]/4  >> 8) & 0xFF; // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format
  data[1] = (-gyro_bias[0]/4)       & 0xFF; // Biases are additive, so change sign on calculated average gyro biases
  data[2] = (-gyro_bias[1]/4  >> 8) & 0xFF;
  data[3] = (-gyro_bias[1]/4)       & 0xFF;
  data[4] = (-gyro_bias[2]/4  >> 8) & 0xFF;
  data[5] = (-gyro_bias[2]/4)       & 0xFF;
  
// Push gyro biases to hardware registers
  writeByte(MPU9250_ADDRESS, XG_OFFSET_H, data[0]);
  writeByte(MPU9250_ADDRESS, XG_OFFSET_L, data[1]);
  writeByte(MPU9250_ADDRESS, YG_OFFSET_H, data[2]);
  writeByte(MPU9250_ADDRESS, YG_OFFSET_L, data[3]);
  writeByte(MPU9250_ADDRESS, ZG_OFFSET_H, data[4]);
  writeByte(MPU9250_ADDRESS, ZG_OFFSET_L, data[5]);
  
// Output scaled gyro biases for display in the main program
  dest1[0] = (float) gyro_bias[0]/(float) gyrosensitivity;  
  dest1[1] = (float) gyro_bias[1]/(float) gyrosensitivity;
  dest1[2] = (float) gyro_bias[2]/(float) gyrosensitivity;

// Construct the accelerometer biases for push to the hardware accelerometer bias registers. These registers contain
// factory trim values which must be added to the calculated accelerometer biases; on boot up these registers will hold
// non-zero values. In addition, bit 0 of the lower byte must be preserved since it is used for temperature
// compensation calculations. Accelerometer bias registers expect bias input as 2048 LSB per g, so that
// the accelerometer biases calculated above must be divided by 8.

  int32_t accel_bias_reg[3] = {0, 0, 0}; // A place to hold the factory accelerometer trim biases
  readBytes(MPU9250_ADDRESS, XA_OFFSET_H, 2, &data[0]); // Read factory accelerometer trim values
  accel_bias_reg[0] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
  readBytes(MPU9250_ADDRESS, YA_OFFSET_H, 2, &data[0]);
  accel_bias_reg[1] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
  readBytes(MPU9250_ADDRESS, ZA_OFFSET_H, 2, &data[0]);
  accel_bias_reg[2] = (int32_t) (((int16_t)data[0] << 8) | data[1]);
  
  uint32_t mask = 1uL; // Define mask for temperature compensation bit 0 of lower byte of accelerometer bias registers
  uint8_t mask_bit[3] = {0, 0, 0}; // Define array to hold mask bit for each accelerometer bias axis
  
  for(ii = 0; ii < 3; ii++) {
    if((accel_bias_reg[ii] & mask)) mask_bit[ii] = 0x01; // If temperature compensation bit is set, record that fact in mask_bit
  }
  
  // Construct total accelerometer bias, including calculated average accelerometer bias from above
  accel_bias_reg[0] -= (accel_bias[0]/8); // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g (16 g full scale)
  accel_bias_reg[1] -= (accel_bias[1]/8);
  accel_bias_reg[2] -= (accel_bias[2]/8);
  
  data[0] = (accel_bias_reg[0] >> 8) & 0xFF;
  data[1] = (accel_bias_reg[0])      & 0xFF;
  data[1] = data[1] | mask_bit[0]; // preserve temperature compensation bit when writing back to accelerometer bias registers
  data[2] = (accel_bias_reg[1] >> 8) & 0xFF;
  data[3] = (accel_bias_reg[1])      & 0xFF;
  data[3] = data[3] | mask_bit[1]; // preserve temperature compensation bit when writing back to accelerometer bias registers
  data[4] = (accel_bias_reg[2] >> 8) & 0xFF;
  data[5] = (accel_bias_reg[2])      & 0xFF;
  data[5] = data[5] | mask_bit[2]; // preserve temperature compensation bit when writing back to accelerometer bias registers
 
// Apparently this is not working for the acceleration biases in the MPU-9250
// Are we handling the temperature correction bit properly?
// Push accelerometer biases to hardware registers
  writeByte(MPU9250_ADDRESS, XA_OFFSET_H, data[0]);
  writeByte(MPU9250_ADDRESS, XA_OFFSET_L, data[1]);
  writeByte(MPU9250_ADDRESS, YA_OFFSET_H, data[2]);
  writeByte(MPU9250_ADDRESS, YA_OFFSET_L, data[3]);
  writeByte(MPU9250_ADDRESS, ZA_OFFSET_H, data[4]);
  writeByte(MPU9250_ADDRESS, ZA_OFFSET_L, data[5]);

// Output scaled accelerometer biases for display in the main program
   dest2[0] = (float)accel_bias[0]/(float)accelsensitivity; 
   dest2[1] = (float)accel_bias[1]/(float)accelsensitivity;
   dest2[2] = (float)accel_bias[2]/(float)accelsensitivity;
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of
// “5123” equals 51.23 DegC.
int32_t bmp280_compensate_T(int32_t adc_T)
{
  int32_t var1, var2, T;
  var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
  t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
  return T;
}


// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8
//fractional bits).
//Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
uint32_t bmp280_compensate_P(int32_t adc_P)
{
  long long var1, var2, p;
  var1 = ((long long)t_fine) - 128000;
  var2 = var1 * var1 * (long long)dig_P6;
  var2 = var2 + ((var1*(long long)dig_P5)<<17);
  var2 = var2 + (((long long)dig_P4)<<35);
  var1 = ((var1 * var1 * (long long)dig_P3)>>8) + ((var1 * (long long)dig_P2)<<12);
  var1 = (((((long long)1)<<47)+var1))*((long long)dig_P1)>>33;
  if(var1 == 0)
  {
    return 0;
    // avoid exception caused by division by zero
  }
  p = 1048576 - adc_P;
  p = (((p<<31) - var2)*3125)/var1;
  var1 = (((long long)dig_P9) * (p>>13) * (p>>13)) >> 25;
  var2 = (((long long)dig_P8) * p)>> 19;
  p = ((p + var1 + var2) >> 8) + (((long long)dig_P7)<<4);
  return (uint32_t)p;
}

int32_t readBMP280Temperature()
{
  uint8_t rawData[3];  // 20-bit pressure register data stored here
  readBytes(BMP280_ADDRESS, BMP280_TEMP_MSB, 3, &rawData[0]);  
  return (int32_t) (((int32_t) rawData[0] << 16 | (int32_t) rawData[1] << 8 | rawData[2]) >> 4);
}

int32_t readBMP280Pressure()
{
  uint8_t rawData[3];  // 20-bit pressure register data stored here
  readBytes(BMP280_ADDRESS, BMP280_PRESS_MSB, 3, &rawData[0]);  
  return (int32_t) (((int32_t) rawData[0] << 16 | (int32_t) rawData[1] << 8 | rawData[2]) >> 4);
}


void BMP280Init()
{
  // Configure the BMP280
  // Set T and P oversampling rates and sensor mode
  writeByte(BMP280_ADDRESS, BMP280_CTRL_MEAS, BMP280Tosr << 5 | BMP280Posr << 2 | BMP280Mode);
  // Set standby time interval in normal mode and bandwidth
  writeByte(BMP280_ADDRESS, BMP280_CONFIG, BMP280SBy << 5 | BMP280IIRFilter << 2);
  // Read and store calibration data
  uint8_t calib[24];
  readBytes(BMP280_ADDRESS, BMP280_CALIB00, 24, &calib[0]);
  dig_T1 = (uint16_t)(((uint16_t) calib[1] << 8) | calib[0]);
  dig_T2 = ( int16_t)((( int16_t) calib[3] << 8) | calib[2]);
  dig_T3 = ( int16_t)((( int16_t) calib[5] << 8) | calib[4]);
  dig_P1 = (uint16_t)(((uint16_t) calib[7] << 8) | calib[6]);
  dig_P2 = ( int16_t)((( int16_t) calib[9] << 8) | calib[8]);
  dig_P3 = ( int16_t)((( int16_t) calib[11] << 8) | calib[10]);
  dig_P4 = ( int16_t)((( int16_t) calib[13] << 8) | calib[12]);
  dig_P5 = ( int16_t)((( int16_t) calib[15] << 8) | calib[14]);
  dig_P6 = ( int16_t)((( int16_t) calib[17] << 8) | calib[16]);
  dig_P7 = ( int16_t)((( int16_t) calib[19] << 8) | calib[18]);
  dig_P8 = ( int16_t)((( int16_t) calib[21] << 8) | calib[20]);
  dig_P9 = ( int16_t)((( int16_t) calib[23] << 8) | calib[22]);
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
