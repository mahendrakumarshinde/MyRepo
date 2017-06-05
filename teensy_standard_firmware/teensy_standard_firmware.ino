/*
  Infinite Uptime BLE Module Firmware Updated 24-12-2016 v33

  TIME DOMAIN FEATURES
  NOTE: Intervals are checked based on AUDIO, not ACCEL. Read vibration energy
  code to understand the sampling mechanism.

  FREQUENCY DOMAIN FEATURES
  NOTE: Most of these variables are for accessing calculation results.
  Read any of frequency domain feature calculation flow to understand.

*/

#include <i2c_t3.h>

#include <i2s.h>  /* I2S digital audio */

/* CMSIS-DSP library for RFFT */
#define ARM_MATH_CM4
#include <arm_math.h>

/* Register addresses and hamming window presets  */
#include "constants.h"

//====================== Module Configuration Variables ========================
#define CLOCK_TYPE         (I2S_CLOCK_48K_INTERNAL)     // I2S clock
bool statusLED = true;                                  // Status LED ON/OFF
String MAC_ADDRESS = "88:4A:EA:69:DE:A8";

// Reduce RUN frequency if needed.
const uint16_t AUDIO_FREQ = 8000;     // Audio frequency set to 8000 Hz
const uint16_t ACCEL_FREQ = 1000;     // Accel frequency set to 1000 Hz
const uint32_t RESTING_INTERVAL = 0;  // Inter-batch gap
//const uint16_t RUN_ACCEL_AUDIO_RATIO = AUDIO_FREQ / ACCEL_FREQ;
// Vibration Energy
const uint32_t ENERGY_INTERVAL_ACCEL = 128; //All data in buffer
// Mean Crossing Rate
const uint32_t MCR_INTERVAL_ACCEL = 64; //All data in buffer

const uint8_t NUM_FEATURES = 6;    // Total number of features
const uint8_t NUM_TD_FEATURES = 2; // Total number of frequency domain features
const uint16_t MAX_INTERVAL_ACCEL = 512;  //Number of Acceleration readings per cycle
const uint16_t MAX_INTERVAL_AUDIO = 4096; //Number of Audio readings per cycle
const byte audioFDFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B00000100}; // Audio feature enabling variable
const byte accelFDFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B00111000}; // Accel feature enabling variable
// RFFT Variables
const uint16_t ACCEL_NFFT = 512;  // Accel FFT Index
const uint16_t AUDIO_NFFT = 2048; // Audio FFT Index

//const uint8_t ACCEL_RESCALE = 8;
const uint8_t AUDIO_RESCALE = 10; // Scaling factor in Audio fft function

// Data collection Mode - Modes of Operation String Constants
const String START_COLLECTION = "IUCMD_START";
const String START_CONFIRM = "IUOK_START";
const String END_COLLECTION = "IUCMD_END";
const String END_CONFIRM = "IUOK_END";

// Pin definitions
const int chargerCHG  = 2;  // CHG pin to detect charging status
const int greenPin = 27;    // Green LED Pin (Active Low)
const int redPin = 29;      // Red LED Pin (Active Low)
const int bluePin = 28;     // Blue LED Pin (Active Low)
//=============================================================================
uint16_t TARGET_AUDIO_SAMPLE = AUDIO_FREQ;          // Target Audio frequency may change in data collection mode
uint16_t TARGET_ACCEL_SAMPLE = ACCEL_FREQ;          // Target Accel frequency may change in data collection mode

// NOTE: Do NOT change the following variables unless you know what you're doing
uint16_t ACCEL_COUNTER_TARGET = TARGET_AUDIO_SAMPLE / TARGET_ACCEL_SAMPLE;  // Changing ratio for data collection mode
uint32_t DOWNSAMPLE_MAX = 48000 / TARGET_AUDIO_SAMPLE;  // Down-sampling ratio
uint32_t subsample_counter = 0;
uint32_t accel_counter = 0;

//====================== Feature Computation Variables =========================
// FFT frequency detection indexes
int max_index_X = 0;  // Frequency Index for X axis
int max_index_Y = 0;  // Frequency Index for Y axis
int max_index_Z = 0;  // Frequency Index for Z axis

int max_index_X_2 = 0;  // 2nd max Frequency Index for X axis
int max_index_Y_2 = 0;  // 2nd max Frequency Index for Y axis
int max_index_Z_2 = 0;  // 2nd max Frequency Index for Z axis

int max_index_X_3 = 0;  // 3rd max Frequency Index for X axis
int max_index_Y_3 = 0;  // 3rd max Frequency Index for Y axis
int max_index_Z_3 = 0;  // 3rd max Frequency Index for Z axis

int max_index = 0;      // Global Frequency Index
int max_index_2 = 0;    // 2nd global Frequency Index
int max_index_3 = 0;    // 3rd global Frequency Index

int chosen_features = 0;  // Number of chosen features

// Byte representing which features are enabled. Feature index 0 is enabled by default.
byte enabledFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B11111100};   // To enable selected features only - Currently not used
int FeatureID[NUM_FEATURES];

// NOTE: These variables are responsible for "buffer-switching" mechanism.
//       Change these only if you know what you're doing.
bool compute_feature_now[2] = {false, false};
bool record_feature_now[2] = {true, true};
uint8_t buffer_compute_index = 0;   // Computation buffer index
uint8_t buffer_record_index = 0;    // Data recording buffer index

uint32_t featureBatchSize = MAX_INTERVAL_AUDIO; // Batch size for a feature -  Can be replaced with MAX_INTERVAL_AUDIO

uint32_t audioSamplingCount = 0;    // Audio sample index for recording data
uint32_t accelSamplingCount = 0;    // Accel sample index for recording data
uint32_t restCount = 0;             // Resting interval - Can be removed
uint8_t highest_danger_level = 0;   // Highest danger level indication for setting up the color indication
uint8_t current_danger_level = 0;   // Current danger level of the feature
float feature_value[NUM_FEATURES];  // Feature value array to besent over cloud

// Acceleration Buffers
q15_t accel_x_batch[2][MAX_INTERVAL_ACCEL]; // Accel buffer along X axis - 2 buffers namely for recording and computation simultaneously
q15_t accel_y_batch[2][MAX_INTERVAL_ACCEL]; // Accel buffer along Y axis - 2 buffers namely for recording and computation simultaneously
q15_t accel_z_batch[2][MAX_INTERVAL_ACCEL]; // Accel buffer along Z axis - 2 buffers namely for recording and computation simultaneously

q15_t accel_x_buff[MAX_INTERVAL_ACCEL]; // Accel buff along X axis for fft calculation
q15_t accel_y_buff[MAX_INTERVAL_ACCEL]; // Accel buff along Y axis for fft calculation
q15_t accel_z_buff[MAX_INTERVAL_ACCEL]; // Accel buff along Z axis for fft calculation

// Audio Buffers
q15_t audio_batch[2][MAX_INTERVAL_AUDIO]; // Audio buffer - 2 buffers namely for recording and computation simultaneously

// RFFT Output and Magnitude Buffers
q15_t rfft_audio_buffer [AUDIO_NFFT * 2];
q15_t rfft_accel_buffer [ACCEL_NFFT * 2];

//TODO: May be able to use just one instance. Check later.
arm_rfft_instance_q15 audio_rfft_instance;
arm_cfft_radix4_instance_q15 audio_cfft_instance;

arm_rfft_instance_q15 accel_rfft_instance;
arm_cfft_radix4_instance_q15 accel_cfft_instance;

//============================ Internal Variables ==============================
// Specify sensor full scale
uint8_t Ascale = AFS_2G;           // set accel full scale
uint8_t ACCBW  = 0x08 & ABW_1000Hz;  // Choose bandwidth for accelerometer

// MPU9250 variables
int16_t accelCount[3];  // Stores the 16-bit signed accelerometer sensor output
float accelBias[3] = {0, 0, 0}; // Bias corrections for gyro, accelerometer, mag

float ax, ay, az; // variables to hold latest sensor data values
float aRes; //resolution of accelerometer

OpMode currMode = RUN;            // Mode of operation
OpState currState = NOT_CUTTING;  // Current state on feature calculation

boolean silent = false; // Silent flag - Used to print statements when not silent

// Hard-wired communication buffers
String wireBuffer = ""; // Serial only expects string literals

// Two-way Communication Variables
char bleBuffer[19] = "abcdefghijklmnopqr";  // BLE Data buffer
uint8_t bleBufferIndex = 0; // BLE buffer Index

// Timestamp variables
int date = 0;
int dateset = 0;
double dateyear = 0;
double bleDateyear = 0;
int dateyear1 = 0;
int parametertag = 0;

//Battery state
int bat = 100;  // Battery status

//Regular data transmit variables
boolean datasendtime = false;
int datasendlimit = 3000;
int currentmillis = 0;
int prevmillis = 0;
int currenttime = 0;
int prevtime = 0;

//Feature selection parameters
int feature0check = 1;
int feature1check = 0;
int feature2check = 0;
int feature3check = 0;
int feature4check = 0;
int feature5check = 0;

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

bool recordmode = false;

//----------------------- Temperature calculation ------------------------
// Specify BMP280 configuration
uint8_t BMP280Posr = P_OSR_00, BMP280Tosr = T_OSR_01, BMP280Mode = normal, BMP280IIRFilter = BW0_042ODR, BMP280SBy = t_62_5ms;     // set pressure amd temperature output data rate
// t_fine carries fine temperature as global value for BMP280
int32_t t_fine;
// BMP280 compensation parameters
uint16_t dig_T1, dig_P1;
int16_t  dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
double BMP280Temperature, BMP280Pressure; // stores MS5637 pressures sensor pressure and temperature

// Array of thresholds; add new array and change code accordingly if you want
// more states.
float featureNormalThreshold[NUM_FEATURES] = {30,   // Feature Energy
                                              0.05, // Velocity X
                                              0.05, // Velocity Y
                                              0.05, // Velocity Z
                                              200,  // Temperature
                                              500   // AudioDB
                                             };
float featureWarningThreshold[NUM_FEATURES] = {600, // Feature Energy
                                               1.2, // Velocity X
                                               1.2, // Velocity Y
                                               1.2, // Velocity Z
                                               205, // Temperature
                                               1000 // AudioDB
                                              };
float featureDangerThreshold[NUM_FEATURES] = {1200, // Feature Energy
                                              1.8,  // Velocity X
                                              1.8,  // Velocity Y
                                              1.8,  // Velocity Z
                                              210,  // Temperature
                                              1500  // AudioDB
                                             };

//------------------------ Function Declaration --------------------------
float feature_energy();     // Function to calculate Energy
float velocityX();          // Function to calculate Velocity along X axis
float velocityY();          // Function to calculate Velocity along Y axis
float velocityZ();          // Function to calculate Velocity along Z axis
float currentTemperature(); // Function to extract current temperature
float audioDB();            // Function to calculate audio data in DB
void changeStatusLED(LEDColors color); // Function to change LED color

typedef float (* FeatureFuncPtr) (); // this is a typedef to feature functions
FeatureFuncPtr features[NUM_FEATURES] = {feature_energy,
                                         velocityX,
                                         velocityY,
                                         velocityZ,
                                         currentTemperature,
                                         audioDB
                                        };

//============================ Error Handling =================================
byte i2c_read_error;
bool readFlag = true;
String error_message = "ALL_OK";
//======================= Feature Computation Caller ============================
// Function to send raw data to Android app for FFT calculation
void show_record_fft(char rec_axis)
{
  BLEport.print("REC,");
  BLEport.print(MAC_ADDRESS);
  if (rec_axis == 'X')
  {
    BLEport.print(",X");
    for (int i = 0; i < MAX_INTERVAL_ACCEL; i++) {
      BLEport.print(",");
      BLEport.print(LSB_to_ms2(accel_x_batch[buffer_compute_index][i]));
      BLEport.flush();
    }
  }
  else if (rec_axis == 'Y')
  {
    BLEport.print(",Y");
    for (int i = 0; i < MAX_INTERVAL_ACCEL; i++) {
      BLEport.print(",");
      BLEport.print(LSB_to_ms2(accel_y_batch[buffer_compute_index][i]));
      BLEport.flush();
    }
  }
  else if (rec_axis == 'Z')
  {
    BLEport.print(",Z");
    for (int i = 0; i < MAX_INTERVAL_ACCEL; i++) {
      BLEport.print(",");
      BLEport.print(LSB_to_ms2(accel_z_batch[buffer_compute_index][i]));
      BLEport.flush();
    }
  }
  BLEport.print(";");
  BLEport.flush();
}
// Function to convert raw accel data to m/s2
float LSB_to_ms2(int16_t accelLSB)
{
  return ((float)accelLSB * aRes * 9.8);
}

// Function to compute all the feature values to be stored on cloud
void compute_features() {
  // Turn the boolean off immediately.
  compute_feature_now[buffer_compute_index] = false;
  record_feature_now[buffer_compute_index] = false;
  highest_danger_level = 0;

  feature_value[0] = feature_energy();
  if (feature0check == 1) {
    current_danger_level = threshold_feature(0, feature_value[0]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  accel_rfft();

  feature_value[1] = velocityX();
  if (feature1check == 1) {
    current_danger_level = threshold_feature(1, feature_value[1]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  feature_value[2] = velocityY();
  if (feature2check == 1) {
    current_danger_level = threshold_feature(2, feature_value[2]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  feature_value[3] = velocityZ();
  if (feature3check == 1) {
    current_danger_level = threshold_feature(3, feature_value[3]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  feature_value[4] = currentTemperature();
  if (feature4check == 1) {
    current_danger_level = threshold_feature(4, feature_value[4]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  feature_value[5] = audioDB();
  if (feature5check == 1) {
    current_danger_level = threshold_feature(5, feature_value[5]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }


  // Reflect highest danger level if differs from previous state.
  LEDColors c;
  if (!recordmode) {
    switch (highest_danger_level) {
      case 0:
        if (sleepmode) {
          // Do nothing
        }
        else {
          if (currState != NOT_CUTTING || datasendtime) {
            currState = NOT_CUTTING;
            BLEport.print(MAC_ADDRESS);
            BLEport.print(",00,");
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
            //Serial.println(gettimestamp());
            BLEport.print(";");
            BLEport.flush();
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
            //sleepmode = true;
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
      case 1:
        if (currState != NORMAL_CUTTING || datasendtime) {
          currState = NORMAL_CUTTING;
          BLEport.print(MAC_ADDRESS);
          BLEport.print(",01,");
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
        c = GREEN_OK;
        changeStatusLED(c);
        datasendtime = false;
        blue = 0;
        sleepmode = false;
        break;
      case 2:
        if (currState != WARNING_CUTTING || datasendtime) {
          currState = WARNING_CUTTING;
          BLEport.print(MAC_ADDRESS);
          BLEport.print(",02,");
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
        c = ORANGE_WARNING;
        changeStatusLED(c);
        datasendtime = false;
        blue = 0;
        sleepmode = false;
        break;
      case 3:
        if (currState != BAD_CUTTING || datasendtime) {
          currState = BAD_CUTTING;
          BLEport.print(MAC_ADDRESS);
          BLEport.print(",03,");
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

// Thresholding helper function to reduce code redundancy
uint8_t threshold_feature(int index, float featureVal) {
  if (featureVal < featureNormalThreshold[index]) {
    return 0; // level 0: not cutting
  }
  else if (featureVal < featureWarningThreshold[index]) {
    return 1; // level 1: normal cutting
  }
  else if (featureVal < featureDangerThreshold[index]) {
    return 2;  // level 2: warning cutting
  }
  else {
    return 3;  // level 3: bad cutting
  }
}


// Single instance of RFFT calculation on audio data.
void audio_rfft() {
  // Apply Hamming window in-place
  arm_mult_q15(audio_batch[buffer_compute_index],
               hamming_window_2048,
               audio_batch[buffer_compute_index],
               AUDIO_NFFT);

  arm_mult_q15(&audio_batch[buffer_compute_index][AUDIO_NFFT],
               hamming_window_2048,
               &audio_batch[buffer_compute_index][AUDIO_NFFT],
               AUDIO_NFFT);


  // TODO: Computation time verification
  // Since input vector is 2048, perform FFT twice.
  // First FFT, 0 to 2047
  arm_rfft_init_q15(&audio_rfft_instance,
                    &audio_cfft_instance,
                    AUDIO_NFFT,
                    0,
                    1);

  arm_rfft_q15(&audio_rfft_instance,
               audio_batch[buffer_compute_index],
               rfft_audio_buffer);

  arm_shift_q15(rfft_audio_buffer,
                AUDIO_RESCALE,
                rfft_audio_buffer,
                AUDIO_NFFT * 2);

  // NOTE: OUTPUT FORMAT IS IN 2.14
  arm_cmplx_mag_q15(rfft_audio_buffer,
                    audio_batch[buffer_compute_index],
                    AUDIO_NFFT);

  arm_q15_to_float(audio_batch[buffer_compute_index],
                   (float*)rfft_audio_buffer,
                   magsize_2048);

  // TODO: REDUCE VECTOR CALCULATION BY COMING UP WITH SINGLE FACTOR

  // Div by K
  arm_scale_f32((float*)rfft_audio_buffer,
                hamming_K_2048,
                (float*)rfft_audio_buffer,
                magsize_2048);

  // Div by wlen
  arm_scale_f32((float*)rfft_audio_buffer,
                inverse_wlen_2048,
                (float*)rfft_audio_buffer,
                magsize_2048);

  // Fix 2.14 format from cmplx_mag
  arm_scale_f32((float*)rfft_audio_buffer,
                2.0,
                (float*)rfft_audio_buffer,
                magsize_2048);

  // Correction of the DC & Nyquist component, including Nyquist point.
  arm_scale_f32(&((float*)rfft_audio_buffer)[1],
                2.0,
                &((float*)rfft_audio_buffer)[1],
                magsize_2048 - 2);


  // Second FFT, 2048 to 4095
  arm_rfft_init_q15(&audio_rfft_instance,
                    &audio_cfft_instance,
                    AUDIO_NFFT,
                    0,
                    1);

  arm_rfft_q15(&audio_rfft_instance,
               &audio_batch[buffer_compute_index][AUDIO_NFFT],
               rfft_audio_buffer);

  arm_shift_q15(rfft_audio_buffer,
                AUDIO_RESCALE,
                rfft_audio_buffer,
                AUDIO_NFFT * 2);

  // NOTE: OUTPUT FORMAT IS IN 2.14
  arm_cmplx_mag_q15(rfft_audio_buffer,
                    audio_batch[buffer_compute_index],
                    AUDIO_NFFT);

  arm_q15_to_float(audio_batch[buffer_compute_index],
                   (float*)rfft_audio_buffer,
                   magsize_2048);

  // TODO: REDUCE VECTOR CALCULATION BY COMING UP WITH SINGLE FACTOR

  // Div by K
  arm_scale_f32((float*)rfft_audio_buffer,
                hamming_K_2048,
                (float*)rfft_audio_buffer,
                magsize_2048);

  // Div by wlen
  arm_scale_f32((float*)rfft_audio_buffer,
                inverse_wlen_2048,
                (float*)rfft_audio_buffer,
                magsize_2048);

  // Fix 2.14 format from cmplx_mag
  arm_scale_f32((float*)rfft_audio_buffer,
                2.0,
                (float*)rfft_audio_buffer,
                magsize_2048);

  // Correction of the DC & Nyquist component, including Nyquist point.
  arm_scale_f32(&((float*)rfft_audio_buffer)[1],
                2.0,
                &((float*)rfft_audio_buffer)[1],
                magsize_2048 - 2);
}

// Single instance of RFFT calculation on accel data.
void accel_rfft() {
  // Apply Hamming window in-place
  arm_mult_q15(accel_x_batch[buffer_compute_index],
               hamming_window_512,
               accel_x_buff,
               ACCEL_NFFT);
  arm_mult_q15(accel_y_batch[buffer_compute_index],
               hamming_window_512,
               accel_y_buff,
               ACCEL_NFFT);
  arm_mult_q15(accel_z_batch[buffer_compute_index],
               hamming_window_512,
               accel_z_buff,
               ACCEL_NFFT);

  // First FFT, X axis
  arm_rfft_init_q15(&accel_rfft_instance,
                    &accel_cfft_instance,
                    ACCEL_NFFT,
                    0,
                    1);

  arm_rfft_q15(&accel_rfft_instance,
               accel_x_buff,
               rfft_accel_buffer);

  float magnitude = 0;
  int df = 2; // 1000.0 / 512.0;
  
  float flatness_x(0), flatness_x_2(0), flatness_x_3(0);
  max_index_X = 0;
  max_index_X_2 = 0;
  max_index_X_3 = 0;
  
  for (int ind_x = 2; ind_x < 256; ind_x++)
  {
    magnitude = sqrt(sq(rfft_accel_buffer[2 * ind_x]) + sq(rfft_accel_buffer[2 * ind_x + 1]));
    //Serial.printf("rfft: %f, flat: %f\n", rfft_accel_buffer[ind_x], flatness_x);
    if (magnitude > flatness_x) {
      flatness_x_3 = flatness_x_2;
      flatness_x_2 = flatness_x;
      flatness_x = magnitude;
      max_index_X_3 = max_index_X_2;
      max_index_X_2 = max_index_X;
      max_index_X = ind_x * df;
      
    }
    else if (magnitude > flatness_x_2)
    {
      flatness_x_3 = flatness_x_2;
      flatness_x_2 = magnitude;
      max_index_X_3 = max_index_X_2;
      max_index_X_2 = ind_x * df;
      
    }
    else if (magnitude > flatness_x_3)
    {
      flatness_x_3 = magnitude;
      max_index_X_3 = ind_x * df;
      
    }
  }
  /*
  Serial.print("Frequencies in X are: ");
  Serial.print(max_index_X); Serial.print(", "); 
  Serial.print(max_index_X_2); Serial.print(", ");
  Serial.println(max_index_X_3);
  */

  // Second FFT, Y axis
  arm_rfft_init_q15(&accel_rfft_instance,
                    &accel_cfft_instance,
                    ACCEL_NFFT,
                    0,
                    1);

  arm_rfft_q15(&accel_rfft_instance,
               accel_y_buff,
               rfft_accel_buffer);

  float flatness_y(0), flatness_y_2(0), flatness_y_3(0);
  max_index_Y = 0;
  max_index_Y_2 = 0;
  max_index_Y_3 = 0;
  
  for (int ind_y = 2; ind_y < 256; ind_y++)
  {
    magnitude = sqrt(sq(rfft_accel_buffer[2 * ind_y]) + sq(rfft_accel_buffer[2 * ind_y + 1]));
    //Serial.printf("rfft: %f, flat: %f\n", rfft_accel_buffer[ind_y], flatness_y);
    if (magnitude > flatness_y) {
      flatness_y_3 = flatness_y_2;
      flatness_y_2 = flatness_y;
      flatness_y = magnitude;
      max_index_Y_3 = max_index_Y_2;
      max_index_Y_2 = max_index_Y;
      max_index_Y = ind_y * df;
      
    }
    else if (magnitude > flatness_y_2)
    {
      flatness_y_3 = flatness_y_2;
      flatness_y_2 = magnitude;
      max_index_Y_3 = max_index_Y_2;
      max_index_Y_2 = ind_y * df;
      
    }
    else if (magnitude > flatness_y_3)
    {
      flatness_y_3 = magnitude;
      max_index_Y_3 = ind_y * df;
      
    }
  }
  /*
  Serial.print("Frequencies in Y are: ");
  Serial.print(max_index_Y); Serial.print(", "); 
  Serial.print(max_index_Y_2); Serial.print(", ");
  Serial.println(max_index_Y_3);
  */

  // Third FFT, Z axis
  arm_rfft_init_q15(&accel_rfft_instance,
                    &accel_cfft_instance,
                    ACCEL_NFFT,
                    0,
                    1);

  arm_rfft_q15(&accel_rfft_instance,
               accel_z_buff,
               rfft_accel_buffer);

  
  float flatness_z(0), flatness_z_2(0), flatness_z_3(0);
  max_index_Z = 0;
  max_index_Z_2 = 0;
  max_index_Z_3 = 0;
  
  for (int ind_z = 2; ind_z < 256; ind_z++)
  {
    magnitude = sqrt(sq(rfft_accel_buffer[2 * ind_z]) + sq(rfft_accel_buffer[2 * ind_z + 1]));
    //Serial.printf("rfft: %f, flat: %f\n", rfft_accel_buffer[ind_z], flatness_z);
    if (magnitude > flatness_z) {
      flatness_z_3 = flatness_z_2;
      flatness_z_2 = flatness_z;
      flatness_z = magnitude;
      max_index_Z_3 = max_index_Z_2;
      max_index_Z_2 = max_index_Z;
      max_index_Z = ind_z * df;
      
    }
    else if (magnitude > flatness_z_2)
    {
      flatness_z_3 = flatness_z_2;
      flatness_z_2 = magnitude;
      max_index_Z_3 = max_index_Z_2;
      max_index_Z_2 = ind_z * df;
      
    }
    else if (magnitude > flatness_z_3)
    {
      flatness_z_3 = magnitude;
      max_index_Z_3 = ind_z * df;
      
    }
  }
  /*
  Serial.print("Frequencies in Z are: ");
  Serial.print(max_index_Z); Serial.print(", "); 
  Serial.print(max_index_Z_2); Serial.print(", ");
  Serial.println(max_index_Z_3);
  */

  if (flatness_z > flatness_y && flatness_z > flatness_x)
  {
    max_index = max_index_Z;
    max_index_2 = max_index_Z_2;
    max_index_3 = max_index_Z_3;
  }
  else if (flatness_y > flatness_z && flatness_y > flatness_x)
  {
    max_index = max_index_Y;
    max_index_2 = max_index_Y_2;
    max_index_3 = max_index_Y_3;
  }
  else
  {
    max_index = max_index_X;
    max_index_2 = max_index_X_2;
    max_index_3 = max_index_X_3;
  }
  /*
  Serial.print("Global frequencies are: ");
  Serial.print(max_index); Serial.print(", "); 
  Serial.print(max_index_2); Serial.print(", ");
  Serial.println(max_index_3);
  */
}



/*====================== Feature Computation Functions =========================*/

// TIME DOMAIN FEATURES

// Signal Energy ACCEL, index 0
// Simple loops  : 4200 microseconds
// All CMSIS-DSP : 2840 microseconds
// Partial       : 2730 microseconds (current)

/**
 * 
 */
float press_energy(uint16_t startInd, uint32_t buffSize)
{
  float compBuffer[3][buffSize];

  // Copy from accel batches to compBuffer
  arm_q15_to_float(&accel_x_batch[buffer_compute_index][startInd],
                   compBuffer[0],
                   buffSize);
  arm_q15_to_float(&accel_y_batch[buffer_compute_index][startInd],
                   compBuffer[1],
                   buffSize);
  arm_q15_to_float(&accel_z_batch[buffer_compute_index][startInd],
                   compBuffer[2],
                   buffSize);
                   
  float energy[3] = {0, 0, 0};
  for (uint16_t i=0; i < 3; i++)
  {
    // Convert to m/s2
    arm_scale_f32(compBuffer[i], aRes * 9.8 * 32768, compBuffer[i], buffSize);
    // Remove accel biases
    //arm_offset_f32(compBuffer[i], accelBias[i], compBuffer[i], buffSize);
    
    for (uint16_t j = 0; j < buffSize; j++)
    {
      energy[i] += sq(compBuffer[i][j]);
    }
    energy[i] /= buffSize;
  }
  return (energy[0] + energy[1] + energy[2]); //Total energy on all 3 axes
}

/**
 * Return the max absolute value
 * 
 * eg: max_abs_accel(accel_x_batch[buffer_compute_index], MAX_INTERVAL_ACCEL)
 */
float max_abs_accel(q15_t *accel_batch, int16_t batchSize)
{
  float max_val(0), val(0);
  for (uint16_t i = 0; i < batchSize; ++i)
  {
    val = abs(accel_batch[i]);
    if (val > max_val)
    {
      max_val = val;
    }
  }
  return max_val;
}

float feature_energy () {
  float ene = press_energy(0, MAX_INTERVAL_ACCEL);
  //Serial.println(timestamp[buffer_compute_index]);
  //Serial.println(ene, DEC);
  return ene;
}
// Function to calculate Accel RMS value for RMS velovity calculation along X axis
float calcAccelRMSx()
{
  float accelRMS = 0.0, accelAVG_2 = 0.0, accelAVG = 0.0;
  for (int i = 0; i < MAX_INTERVAL_ACCEL; i++)
  {
    accelAVG += (float)accel_x_batch[buffer_compute_index][i] * aRes * 9.8;
    accelAVG_2 += sq((float)accel_x_batch[buffer_compute_index][i] * aRes * 9.8);
  }
  float scaling_factor = 1 + 0.00266 * max_index_X + 0.000106 * sq(max_index_X);
  accelRMS = scaling_factor * sqrt(accelAVG_2 / MAX_INTERVAL_ACCEL - sq(accelAVG / MAX_INTERVAL_ACCEL));
  //    Serial.printf("Accel_RMS_X: %f\n",accelRMS);
  return accelRMS;
}
// Function to calculate RMS velocity along X axis
float velocityX()
{
  if (feature_value[0] < featureNormalThreshold[0]) {
    return 0; // level 0: not cutting
  }
  else {
    float vel_rms_x = calcAccelRMSx() * 1000 / (2 * 3.14159 * max_index_X);
    //  Serial.printf("Vel_RMS_X: %f\n", vel_rms_x);
    return vel_rms_x;
  }
}
// Function to calculate Accel RMS value for RMS velovity calculation along Y axis
float calcAccelRMSy()
{
  float accelRMS = 0.0, accelAVG_2 = 0.0, accelAVG = 0.0;
  for (int i = 0; i < MAX_INTERVAL_ACCEL; i++)
  {
    accelAVG += (float)accel_y_batch[buffer_compute_index][i] * aRes * 9.8;
    accelAVG_2 += sq((float)accel_y_batch[buffer_compute_index][i] * aRes * 9.8);
  }
  float scaling_factor = 1 + 0.00266 * max_index_Y + 0.000106 * sq(max_index_Y);
  accelRMS = scaling_factor * sqrt(accelAVG_2 / MAX_INTERVAL_ACCEL - sq(accelAVG / MAX_INTERVAL_ACCEL));
  //  Serial.printf("Accel_RMS_Y: %f\n",accelRMS);
  return accelRMS;
}
// Function to calculate RMS velocity along Y axis
float velocityY()
{
  if (feature_value[0] < featureNormalThreshold[0]) {
    return 0; // level 0: not cutting
  }
  else {
    float vel_rms_y = calcAccelRMSy() * 1000 / (2 * 3.14159 * max_index_Y);
    //  Serial.printf("Vel_RMS_Y: %f\n", vel_rms_y);
    return vel_rms_y;
  }
}
// Function to calculate Accel RMS value for RMS velovity calculation along Z axis
float calcAccelRMSz()
{
  float accelRMS = 0.0, accelAVG_2 = 0.0, accelAVG = 0.0;
  for (int i = 0; i < MAX_INTERVAL_ACCEL; i++)
  {
    accelAVG += (float)accel_z_batch[buffer_compute_index][i] * aRes * 9.8;
    accelAVG_2 += sq((float)accel_z_batch[buffer_compute_index][i] * aRes * 9.8);
  }
  float scaling_factor = 1 + 0.00266 * max_index_Z + 0.000106 * sq(max_index_Z);
  accelRMS = scaling_factor * sqrt(accelAVG_2 / MAX_INTERVAL_ACCEL - sq(accelAVG / MAX_INTERVAL_ACCEL));

  //  Serial.printf("Accel_RMS_Z: %f\n", accelRMS);
  //  Serial.printf("Scale_Z: %f\n",scaling_factor);
  return accelRMS;
}
// Function to calculate RMS velocity along Z axis
float velocityZ()
{
  if (feature_value[0] < featureNormalThreshold[0]) {
    return 0; // level 0: not cutting
  }
  else {
    float vel_rms_z = calcAccelRMSz() * 1000 / (2 * 3.14 * max_index_Z);
    //  Serial.printf("Vel_RMS_Z: %f\n", vel_rms_z);
    return vel_rms_z;
  }
}

// Function to estimate current temperature from BMP280 Sensor
float currentTemperature() // Index 4
{
  int32_t rawTemp = readBMP280Temperature();
  if (i2c_read_error == 0x00)
  {
    BMP280Temperature = (float) bmp280_compensate_T(rawTemp) / 100.;
  }
  return BMP280Temperature;
}

// Function to calculate Audio data in DB
float audioDB()
{
  float audioDB_val = 0.0;
  float aud_data = 0.0;
  for (int i = 0; i < MAX_INTERVAL_AUDIO; i++)
  {
    aud_data = (float) abs(audio_batch[buffer_compute_index][i]);// / 16777215.0;    //16777215 = 0xffffff - 24 bit max value
    //    Serial.printf("AudioData: %f\n", aud_data);
    if (aud_data > 0)
    {
      audioDB_val += (20 * log10(aud_data));
    }
  }
  audioDB_val /= MAX_INTERVAL_AUDIO;
  //  Serial.printf("AudioDB: %f\n", audioDB_val);
  return (0.0095 * audioDB_val * audioDB_val + 0.06 * audioDB_val + 59.91);
}

//==============================================================================
//================================= Main Code ==================================
//==============================================================================

// extract the 24bit INMP441 audio data from 32bit sample
void extractdata_inplace(int32_t  *pBuf) {
  if (pBuf[0] < 0)  // Check if the data is negative
  {
    pBuf[0] = pBuf[0] >> 8;
    pBuf[0] |= 0xff000000;
  }
  else  // Data is positive
  {
    pBuf[0] = pBuf[0] >> 8;
  }
}
/* --- Direct I2S Receive, we get callback to read 2 words from the FIFO --- */
void i2s_rx_callback( int32_t *pBuf )
{
  //  Serial.print("I2S RX : ");
  //  Serial.println(error_message);
  if (error_message.equals("ALL_OK"))
  {
    // Don't do anything if CHARGING
    if (currMode == CHARGING) {
      //Serial.print("Returning on charge\n");
      return;
    }

    if (currMode == RUN)
    {
      if (audioSamplingCount == 0 && !record_feature_now[buffer_record_index])
      {
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
    } else {
      subsample_counter = 0;
      accel_counter++;
    }

    if (currMode == RUN) {
      //Serial.print("currMode=RUN\n");
      if (restCount < RESTING_INTERVAL) {
        restCount ++;
      }
      else {
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
          if (readFlag)
          {
            accel_x_batch[buffer_record_index][accelSamplingCount] = accelCount[0];
            accel_y_batch[buffer_record_index][accelSamplingCount] = accelCount[1];
            accel_z_batch[buffer_record_index][accelSamplingCount] = accelCount[2];
            accelSamplingCount ++;
          }
          accel_counter = 0;
        }
      }
    } else if (currMode == DATA_COLLECTION) {
      // perform the data extraction for single channel mic
      extractdata_inplace(&pBuf[0]);

      // Dump sound data first
      Serial.write((pBuf[0] >> 16) & 0xFF);
      Serial.write((pBuf[0] >> 8) & 0xFF);
      Serial.write(pBuf[0] & 0xFF);

      if (accel_counter == ACCEL_COUNTER_TARGET) {
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
    else {
      //Serial.print("DIDNT FIND A MODE");
      //Serial.print(currMode);
    }
  }
}

/* --- Time functions --- */
double gettimestamp() {
  return dateyear;
}

/* ----------------------- begin -------------------- */

void setup()
{
  if (statusLED) {
    pinMode(greenPin, OUTPUT);
    pinMode(redPin, OUTPUT);
    pinMode(bluePin, OUTPUT);
  }

  // Enable LiPo charger's CHG read.
  pinMode(chargerCHG, INPUT_PULLUP);

  // Make sure to initialize BLEport at the end of setup
  BLEport.begin(9600);

  // Setup for Master mode, pins 18/19, external pullups, 400kHz for Teensy 3.1
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  delay(10);

  Serial.begin(115200);
  delay(10);

  I2Cscan(); // should detect SENtral at 0x28

  // Set up the SENtral as sensor bus in normal operating mode
  // ld pass thru
  // Put EM7180 SENtral into pass-through mode
  SENtralPassThroughMode();

  // should see all the devices on the I2C bus including two from the EEPROM (ID page and data pages)
  I2Cscan();

  error_message = "ALL_OK"; //ZAB: Do not change this flow as error_message need to be set here

  byte c = readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250); // Read WHO_AM_I register on MPU
  if (c == 0x71) { //WHO_AM_I should always be 0x71 for the entire chip
    initMPU9250();  // Initialize MPU9250 Accelerometer sensor
    getAres();  // Set the aRes value depending on the accel resolution selected
  }
  else
  {
    Serial.print("Could not connect to MPU9250: 0x");
    Serial.println(c, HEX);
    error_message = "MPUERR";
    //while (1) ; // Loop forever if communication doesn't happen
  }
  delay(15);
  if (!silent) {
    Serial.print("Pin configuration setting: ");
    Serial.println( I2S_PIN_PATTERN , HEX );
    Serial.println( "Initializing." );
  }

  // Pressure and Temperature sensor
  // Read the WHO_AM_I register of the BMP280 this is a good test of communication
  byte f = readByte(BMP280_ADDRESS, BMP280_ID);  // Read WHO_AM_I register for BMP280
  if (!silent)
  {
    Serial.print("BMP280 ");
    Serial.print("I AM ");
    Serial.print(f, HEX);
    Serial.print(" I should be ");
    Serial.println(0x58, HEX);
    Serial.println(" ");
  }
  delay(10);

  writeByte(BMP280_ADDRESS, BMP280_RESET, 0xB6); // reset BMP280 before initilization
  delay(100);
  if (f == 0x58)
  {
    BMP280Init(); // Initialize BMP280 altimeter
    Serial.println("BMP280 Initialised");
  }
  else
  {
    error_message = "BMPERR";
    Serial.print("Could not connect to BMP280: 0x");
    Serial.println(f, HEX);
    //  while (1) ; // Loop forever if communication doesn't happen
  }

  if (!silent) Serial.println( "Initialized I2C Codec" );
  if (error_message.equals("ALL_OK"))
  {
    I2SRx0.begin( CLOCK_TYPE, i2s_rx_callback );  // prepare I2S RX with interrupts
    if (!silent) Serial.println( "Initialized I2S RX without DMA" );
    delay(50);
    I2SRx0.start(); // start the I2S RX
    if (!silent) Serial.println( "Started I2S RX" );
  }
  else {
    Serial.println(error_message);
  }
  //For Battery Monitoring//
  analogReference(EXTERNAL);
  analogReadResolution(12);
  analogReadAveraging(32);
  bat = getInputVoltage();  //Determine the battery status

  // Calculate feature size and which ones are selected
  for (int i = 0; i < NUM_FEATURES; i++) {
    // Consider only when corresponding bit is turned on.
    if ((enabledFeatures[i / 8] << (i % 8)) & B10000000) {
      FeatureID[chosen_features] = i;
      chosen_features++;
    }
  }
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
  dateyear = bleDateyear + (currenttime - prevtime) / 1000.0;

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
  if (bitRead(USB0_OTGSTAT, 5)) {
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

      if (bleBufferIndex > 18)
      {
        Serial.println(bleBuffer);
        if (bleBuffer[0] == '0')  // Set Thresholds
        {
          if (bleBuffer[4] == '-' && bleBuffer[9] == '-' && bleBuffer[14] == '-') {
            int bleFeatureIndex = 0, newThres = 0, newThres2 = 0, newThres3 = 0;
            sscanf(bleBuffer, "%d-%d-%d-%d", &bleFeatureIndex, &newThres, &newThres2, &newThres3);
            if (bleFeatureIndex == 1 || bleFeatureIndex == 2 || bleFeatureIndex == 3) {
              featureNormalThreshold[bleFeatureIndex] = (float)newThres / 100.;
              featureWarningThreshold[bleFeatureIndex] = (float)newThres2 / 100.;
              featureDangerThreshold[bleFeatureIndex] = (float)newThres3 / 100.;
            } else {
              featureNormalThreshold[bleFeatureIndex] = (float)newThres;
              featureWarningThreshold[bleFeatureIndex] = (float)newThres2;
              featureDangerThreshold[bleFeatureIndex] = (float)newThres3;
            }
            bleBufferIndex = 0;
          }
        }
        else if (bleBuffer[0] == '1')      // receive the timestamp data from the hub
        {
          if (bleBuffer[1] == ':' && bleBuffer[12] == '.') {
            sscanf(bleBuffer, "%d:%d.%d", &date, &dateset, &dateyear1);
            bleDateyear = double(dateset) + double(dateyear1) / double(1000000);
            prevtime = millis();
          }
        }
        else if (bleBuffer[0] == '2')      // Wireless parameter setting
        {
          if (bleBuffer[1] == ':' && bleBuffer[7] == '-' && bleBuffer[13] == '-') {
            sscanf(bleBuffer, "%d:%d-%d-%d", &parametertag, &datasendlimit, &bluesleeplimit, &datareceptiontimeout);
            Serial.print("Data send limit is : ");
            Serial.println(datasendlimit);
          }
        }
        else if (bleBuffer[0] == '3')      // Record button pressed - go into record mode to record FFTs
        {
          if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0') {
            Serial.print("Time to record data and send FFTs");
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
        }
        else if (bleBuffer[0] == '4')      // Stop button pressed - go out of record mode back into RUN mode
        {
          if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0') {
            Serial.print("Stop recording and sending FFTs");
          }
        }
        else if (bleBuffer[0] == '5')
        {
          if (bleBuffer[7] == '0' && bleBuffer[9] == '0' && bleBuffer[11] == '0' && bleBuffer[13] == '0' && bleBuffer[15] == '0' && bleBuffer[17] == '0') {
            BLEport.print("HB,");
            BLEport.print(MAC_ADDRESS);
            BLEport.print(",");
            BLEport.print(error_message);
            BLEport.print(";");
            BLEport.flush();
          }
        }
        else if (bleBuffer[0] == '6')
        {
          if (bleBuffer[7] == ':' && bleBuffer[9] == '.' && bleBuffer[11] == '.' && bleBuffer[13] == '.' && bleBuffer[15] == '.' && bleBuffer[17] == '.') {
            sscanf(bleBuffer, "%d:%d.%d.%d.%d.%d.%d", &parametertag, &feature0check, &feature1check, &feature2check, &feature3check, &feature4check, &feature5check);
          }
        }
        bleBufferIndex = 0;
      }
    }
  }
  /* ------------- Hardwire Serial for DATA_COLLECTION commands ------------- */
  while (Serial.available() > 0) {
    char characterRead = Serial.read();
    if (characterRead != '\n')
      wireBuffer.concat(characterRead);
  }
  if (wireBuffer.length() > 0)
  {
    if (currMode == RUN)
    {
      // Check the received info; iff data collection request, change the mode
      if (wireBuffer.indexOf(START_COLLECTION) > -1) {
        LEDColors c = CYAN_DATA;
        changeStatusLED(c);
        Serial.print(START_CONFIRM);
        resetSampling(false);
        currMode = DATA_COLLECTION;
      }
    }
    else if (currMode == DATA_COLLECTION)
    {
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
        int rgblocation = wireBuffer.indexOf("rgb");
        int Red = wireBuffer.charAt(rgblocation + 7) - 48;
        int Green = wireBuffer.charAt(rgblocation + 8) - 48;
        int Blue = wireBuffer.charAt(rgblocation + 9) - 48;
        if (Green == 1) {
          ledOn(greenPin);
        }
        else {
          ledOff(greenPin);
        }
        if (Red == 1) {
          ledOn(redPin);
        }
        else {
          ledOff(redPin);
        }
        if (Blue == 1) {
          ledOn(bluePin);
        }
        else {
          ledOff(bluePin);
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
      if (wireBuffer == END_COLLECTION)
      {
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
    wireBuffer = "";    // Clear wire buffer
  }
  //Serial.println(error_message);
  // After handling all state / mode changes, check if features need to be computed.
  if (currMode == RUN && compute_feature_now[buffer_compute_index] && error_message.equals("ALL_OK")) {
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

// Function that reflects LEDColors into statusLED.
// Will not have effect if statusLED is turned off.
void ledOn(int pin)
{
  digitalWrite(pin, LOW);
}
void ledOff(int pin)
{
  digitalWrite(pin, HIGH);
}

void changeStatusLED(LEDColors color) {
  if (!statusLED) return;
  switch (color) {
    case RED_BAD:
      ledOn(redPin);    // R = 1
      ledOff(greenPin); // G = 0
      ledOff(bluePin);  // B = 0
      break;
    case BLUE_NOOP:
      ledOff(redPin);   // R = 0
      ledOff(greenPin); // G = 0
      ledOn(bluePin);   // B = 1
      break;
    case GREEN_OK:
      ledOff(redPin);   // R = 0
      ledOn(greenPin);  // G = 1
      ledOff(bluePin);  // B = 0
      break;
    case ORANGE_WARNING:
      ledOn(redPin);    // R = 1
      ledOn(greenPin);  // G = 1
      ledOff(bluePin);  // B = 0
      break;
    case PURPLE_CHARGE:
      ledOn(redPin);    // R = 1
      ledOff(greenPin); // G = 0
      ledOn(bluePin);   // B = 1
      break;
    case CYAN_DATA:
      ledOff(redPin);   // R = 0
      ledOn(greenPin);  // G = 1
      ledOn(bluePin);   // B = 1
      break;
    case WHITE_NONE:
      ledOn(redPin);    // R = 1
      ledOn(greenPin);  // G = 1
      ledOn(bluePin);   // B = 1
      break;
    case SLEEP_MODE:
      ledOff(redPin);   // R = 0
      ledOff(greenPin); // G = 0
      ledOff(bluePin);  // B = 0
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
  // Verify standby status
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
  var2 = var2 + ((var1 * (long long)dig_P5) << 17);
  var2 = var2 + (((long long)dig_P4) << 35);
  var1 = ((var1 * var1 * (long long)dig_P3) >> 8) + ((var1 * (long long)dig_P2) << 12);
  var1 = (((((long long)1) << 47) + var1)) * ((long long)dig_P1) >> 33;
  if (var1 == 0)
  {
    return 0;
    // avoid exception caused by division by zero
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((long long)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((long long)dig_P8) * p) >> 19;
  p = ((p + var1 + var2) >> 8) + (((long long)dig_P7) << 4);
  return (uint32_t)p;
}

int32_t readBMP280Temperature()
{
  uint8_t rawData[3];  // 20-bit temperature register data stored here
  readBytes(BMP280_ADDRESS, BMP280_TEMP_MSB, 3, &rawData[0]);
  if (i2c_read_error == 0x00)
  {
    return (int32_t) (((int32_t) rawData[0] << 16 | (int32_t) rawData[1] << 8 | rawData[2]) >> 4);
  }
  else
  {
    Serial.println("Skip temperature read");
    Serial.println(i2c_read_error, HEX);
    i2c_read_error = 0;
  }
  return 0;
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
  uint8_t data = 0; // `data` will store the register data
  Wire.beginTransmission(address);         // Initialize the Tx buffer
  Wire.write(subAddress);                   // Put slave register address in Tx buffer
  i2c_read_error = Wire.endTransmission(I2C_NOSTOP, 1000); // Send the Tx buffer, but send a restart to keep connection alive
  if (i2c_read_error == 0x00)
  {
    Wire.requestFrom(address, (size_t) 1);   // Read one byte from slave register address
    data = Wire.read();                      // Fill Rx buffer with result
  }
  else
  {
    error_message = "I2CERR";
    Serial.print("Error Code:");
    Serial.println(i2c_read_error);
  }
  return data;                             // Return data read from slave register
}

void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest)
{
  if (readFlag) // To restrain I2S ISR for accel read from accessing I2C bus while temperature sensor is being read
  {
    readFlag = false;
    Wire.beginTransmission(address);   // Initialize the Tx buffer
    Wire.write(subAddress);            // Put slave register address in Tx buffer
    i2c_read_error = Wire.endTransmission(I2C_NOSTOP, 1000); // Send the Tx buffer, but send a restart to keep connection alive
    if (i2c_read_error == 0x00)
    {
      Wire.requestFrom(address, (size_t) count);  // Read bytes from slave register address
      uint8_t i = 0;
      while (Wire.available()) {
        dest[i++] = Wire.read();
      }         // Put read results in the Rx buffer
    }
    else
    {
      error_message = "I2CERR";
    }
    readFlag = true;
  }
}
