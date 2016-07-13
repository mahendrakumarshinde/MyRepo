/*
  Infinite Uptime BLE Module Firmware
*/

//#include "Wire.h"
#include <i2c_t3.h>
#include <SPI.h>

/* I2S digital audio */
#include <i2s.h>

/* CMSIS-DSP library for RFFT */
#define ARM_MATH_CM4
#include <arm_math.h>

/* Register addresses and hamming window presets  */
#include "constants.h"

//==============================================================================
//====================== Module Configuration Variables ========================
//==============================================================================

#define CLOCK_TYPE         (I2S_CLOCK_48K_INTERNAL)     // I2S clock
bool statusLED = true;                                  // Status LED ON/OFF
String MAC_ADDRESS = "20:91:48:42:4A:F4";
// Reduce RUN frequency if needed.
const uint16_t AUDIO_FREQ_RUN = 8000;
const uint16_t AUDIO_FREQ_DATA = 8000;

uint16_t TARGET_AUDIO_SAMPLE = AUDIO_FREQ_RUN;          // Audio frequency
const uint16_t TARGET_ACCEL_SAMPLE = 1000;              // Accel frequency
const uint32_t RESTING_INTERVAL = 0;                    // Inter-batch gap

// NOTE: Do NOT change the following variables unless you know what you're doing
const uint16_t RUN_ACCEL_AUDIO_RATIO = AUDIO_FREQ_RUN / TARGET_ACCEL_SAMPLE;
uint16_t ACCEL_COUNTER_TARGET = TARGET_AUDIO_SAMPLE / TARGET_ACCEL_SAMPLE;
uint16_t DOWNSAMPLE_MAX = 48000 / TARGET_AUDIO_SAMPLE;
uint32_t subsample_counter = 0;
uint32_t accel_counter = 0;

//==============================================================================
//====================== Feature Computation Variables =========================
//==============================================================================

// TIME DOMAIN FEATURES
// NOTE: Intervals are checked based on AUDIO, not ACCEL. Read vibration energy
//       code to understand the sampling mechanism.

// Vbiration Energy
const uint32_t ENERGY_INTERVAL_ACCEL = 64; //128
const uint32_t ENERGY_INTERVAL_AUDIO = ENERGY_INTERVAL_ACCEL * RUN_ACCEL_AUDIO_RATIO;

// Mean Crossing Rate
const uint32_t MCR_INTERVAL_ACCEL = 128;
const uint32_t MCR_INTERVAL_AUDIO = MCR_INTERVAL_ACCEL * RUN_ACCEL_AUDIO_RATIO;

// FREQUENCY DOMAIN FEATURES
// NOTE: Most of these variables are for accessing calculation results.
//       Read any of frequency domain feature calculation flow to understand.

/* Deprecated, but keeping this just in case we will need again.
  // Spectral Flux
  double sp_flux_x = 0;
  double sp_flux_y = 0;
  double sp_flux_z = 0;
*/

// Spectral Centroid
double sp_cent_x = 0;
double sp_cent_y = 0;
double sp_cent_z = 0;

// Spectral Flatness
double sp_flat_x = 0;
double sp_flat_y = 0;
double sp_flat_z = 0;

/* Deprecated, but keeping this just in case we will need again.
  // Spectral Crest
  float sp_crest_x = 0;
  float sp_crest_y = 0;
  float sp_crest_z = 0;
*/

// Spectral Spread ACCEL
double sp_spread_x = 0;
double sp_spread_y = 0;
double sp_spread_z = 0;

// Spectral Spread AUDIO
float sp_spread_aud = 0;

/*
  By default, only energy feature is turned on.

  0: Signal Energy
  1: Mean Crossing Rate
  2: Spectral Centroid
  3: Spectral Flatness
  4: Accel Spectral Spread
  5: Audio Spectral Spread

  CANCELLED: Spectral Flux
  CANCELLED: Spectral Crest
*/
const uint8_t NUM_FEATURES = 6;    // Total number of features
const uint8_t NUM_TD_FEATURES = 2; // Total number of frequency domain features
int chosen_features = 0;

float feature_energy();
float feature_mcr();
float feature_spectral_centroid();
float feature_spectral_flatness();
float feature_spectral_spread_accel();
float feature_spectral_spread_audio();
void calculate_spectral_centroid(int axis);
void calculate_spectral_flatness(int axis);
void calculate_spectral_spread_accel(int axis);
void calculate_spectral_spread_audio(int axis);

typedef float (* FeatureFuncPtr) (); // this is a typedef to feature functions
FeatureFuncPtr features[NUM_FEATURES] = {feature_energy,
                                         feature_mcr,
                                         //feature_spectral_flux,
                                         feature_spectral_centroid,
                                         feature_spectral_flatness,
                                         //feature_spectral_crest,
                                         feature_spectral_spread_accel,
                                         feature_spectral_spread_audio
                                        };

typedef void (* FeatureCalcPtr) (int); // this is a typedef to FD calculation functions
FeatureCalcPtr calc_features[NUM_FEATURES] = {NULL,
                                              NULL,
                                              //calculate_spectral_flux,
                                              calculate_spectral_centroid,
                                              calculate_spectral_flatness,
                                              //calculate_spectral_crest,
                                              calculate_spectral_spread_accel,
                                              calculate_spectral_spread_audio
                                             };

// NOTE: DO NOT CHANGE THIS UNLESS YOU KNOW WHAT YOU'RE DOING
const uint16_t MAX_INTERVAL_ACCEL = 512;
const uint16_t MAX_INTERVAL_AUDIO = 4086;

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

// Array of intervals for every features; currently, every FD feature is expected
// to have half-second interval. Add new variables and change this when you want
// smaller interval for frequency domain features.
uint32_t featureIntervals[NUM_FEATURES] = {ENERGY_INTERVAL_AUDIO,
                                           MCR_INTERVAL_AUDIO,
                                           //MAX_INTERVAL_AUDIO,
                                           MAX_INTERVAL_AUDIO,
                                           MAX_INTERVAL_AUDIO,
                                           //MAX_INTERVAL_AUDIO,
                                           MAX_INTERVAL_AUDIO,
                                           MAX_INTERVAL_AUDIO
                                          };

// Array of thresholds; add new array and change code accordingly if you want
// more states.
float featureNormalThreshold[NUM_FEATURES] = {30,
                                              10000,
                                              //10000,
                                              60,
                                              40,
                                              //10000,
                                              200,
                                              500
                                             };
float featureWarningThreshold[NUM_FEATURES] = {600,
                                               50000,
                                               //10000,
                                               70,
                                               50,
                                               //10000,
                                               205,
                                               1000
                                              };
float featureDangerThreshold[NUM_FEATURES] = {1200,
                                              100000,
                                              //100000,
                                              80,
                                              60,
                                              //100000,
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
q15_t accel_x_batch[2][MAX_INTERVAL_ACCEL];
q15_t accel_y_batch[2][MAX_INTERVAL_ACCEL];
q15_t accel_z_batch[2][MAX_INTERVAL_ACCEL];

// Audio Buffers
q15_t audio_batch[2][MAX_INTERVAL_AUDIO];

// RFFT Variables
const uint16_t ACCEL_NFFT = 512;
const uint16_t AUDIO_NFFT = 2048;

const uint8_t ACCEL_RESCALE = 8;
const uint8_t AUDIO_RESCALE = 10;

// RFFT Output and Magnitude Buffers
q15_t rfft_audio_buffer [AUDIO_NFFT * 2];
q15_t rfft_accel_buffer [ACCEL_NFFT * 2];

//TODO: May be able to use just one instance. Check later.
arm_rfft_instance_q15 audio_rfft_instance;
arm_cfft_radix4_instance_q15 audio_cfft_instance;

arm_rfft_instance_q15 accel_rfft_instance;
arm_cfft_radix4_instance_q15 accel_cfft_instance;

//==============================================================================
//============================ Internal Variables ==============================
//==============================================================================


// Specify sensor full scale
uint8_t Ascale = AFS_2G;           // set accel full scale
uint8_t ACCBW  = 0x08 & ABW_1000Hz;  // Choose bandwidth for accelerometer

// Pin definitions
int myLed       = 13;  // LED on the Teensy 3.1/3.2
int chargerCHG  = 2;   // CHG pin to detect charging status

// BMX055 variables
int16_t accelCount[3];  // Stores the 16-bit signed accelerometer sensor output
float accelBias[3] = {0, 0, 0}; // Bias corrections for gyro, accelerometer, mag

float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest sensor data values
float aRes; //resolution of accelerometer
int delt_t;

long init_time;

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
unsigned char bytes[4];

// Two-way Communication Protocol Constants
const String THRESH_START_FLAG = "THBG";
const String THRESH_SUCCESS = "THRESH_OK";
const String THRESH_FAIL = "THRESH_FAIL";

const String FEATURE_SET_START_FLAG = "FEBG";
const String FEATURE_SET_SUCCESS = "FEATURE_SET_OK";
const String FEATURE_SET_FAIL = "FEATURE_SET_FAIL";
const uint8_t FEATURE_SET_BYTELEN = (((NUM_FEATURES - 1) / 8) / 4 + 1) * 4;

// Hard-wired communication buffers
String wireBuffer = ""; // Serial only expects string literals
char characterRead; // Holder variable for newline check

// Two-way Communication Variables
char bleBuffer[19] = "abcdefghijklmnopqr";
byte bleFeaturesBuffer[(NUM_FEATURES - 1) / 8 + 1] = {B00000000};
uint8_t bleBufferIndex = 0;
uint8_t bleBytesRead = 0;
uint16_t bleFeatureIndex = 0;
int featureIndex = 0;
int newThres = 0;
int newThres2 = 0;
int newThres3 = 0;
boolean didThresChange = false;
int args_assigned = 0;
int args_assigned2 = 0;
boolean rubbish = false;
int date = 0;
int dateset = 0;
double dateyear = 0;
int dateyear1 = 0;

// THRESH
uint16_t bleFeatureType = 0;
float bleFeatureValue = 0;

// FEATURE_SET
uint32_t largestBatchSize = 0;

// Communication state enum
enum CommState {
  COMM_IDLE           = 0,
  THRESH              = 1,
  FEATURE_SET         = 2
};

CommState currComm = COMM_IDLE;

//Battery state
int bat = 100;

//Regular data transmit variables
boolean datasendtime = false;
int datasendlimit = 1500;
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
int bluesleeplimit = 10000;
int bluetimerstart = 0;

//Special feature variables
boolean buffcounter = false;
float paddedbuff[5];

/*==============================================================================
  ====================== Feature Computation Caller ============================
  ==============================================================================*/
void compute_features() {
  // Turn the boolean off immediately.
  compute_feature_now[buffer_compute_index] = false;
  record_feature_now[buffer_compute_index] = false;
  highest_danger_level = 0;

  for (int i = 0; i < NUM_TD_FEATURES; i++) {
    // Compute time domain feature only when corresponding bit is turned on.
    if ((enabledFeatures[i / 8] << (i % 8)) & B10000000) {
      feature_value[i] = features[i]();
      current_danger_level = threshold_feature(i, feature_value[i]);
      // TODO: Variable is separated for possible feature value output.
      //       AKA, when danger level is 2, output feature index, etc.
      highest_danger_level = max(highest_danger_level, current_danger_level);
    }
  }

  if (audioFDCompute) {
    audio_rfft();
    for (int i = NUM_TD_FEATURES; i < NUM_FEATURES; i++) {
      // Compute audio frequency domain feature only when the feature IS audio FD
      // And the bit is turned on for enabledFeatures.
      if ((enabledFeatures[i / 8] << (i % 8)) & (audioFDFeatures[i / 8] << (i % 8)) & B10000000) {
        feature_value[i] = features[i]();
        current_danger_level = threshold_feature(i, feature_value[i]);
        // TODO: Variable is separated for possible feature value output.
        //       AKA, when danger level is 2, output feature index, etc.
        //highest_danger_level = max(highest_danger_level, current_danger_level); //UNCOMMENT
      }
    }
  }
  if (accelFDCompute) {
    accel_rfft();
    for (int i = NUM_TD_FEATURES; i < NUM_FEATURES; i++) {
      // Compute accel frequency domain feature only when the feature IS accel FD
      // And the bit is turned on for enabledFeatures.
      if ((enabledFeatures[i / 8] << (i % 8)) & (accelFDFeatures[i / 8] << (i % 8)) & B10000000) {
        feature_value[i] = features[i]();
        current_danger_level = threshold_feature(i, feature_value[i]);
        //highest_danger_level = max(highest_danger_level, current_danger_level); //UNCOMMENT
      }
    }
  }
  // Reflect highest danger level if differs from previous state.
  LEDColors c;
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

  // Allow recording for old index buffer.
  record_feature_now[buffer_compute_index] = true;
  // Flip computation index AFTER all computations are done.
  buffer_compute_index = buffer_compute_index == 0 ? 1 : 0;
}

// Thresholding helper function to reduce code redundancy
uint8_t threshold_feature(int index, float featureVal) {
  if (featureVal < featureNormalThreshold[index]) {
    // level 0: not cutting
    return 0;
  }
  else if (featureVal < featureWarningThreshold[index]) {
    // level 1: normal cutting
    return 1;
  }
  else if (featureVal < featureDangerThreshold[index]) {
    // level 2: warning cutting
    return 2;
  }
  else {
    // level 3: bad cutting
    return 3;
  }
}

// Single instance of RFFT calculation on audio data.
// ORIGINAL TIME SERIES DATA WILL BE MODIFIED! CALCULATE TIME DOMAIN FEATURES FIRST
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

  /* DEBUGGING ROUTINE; FREQUENCY CHECKER
      TODO: REMOVE THIS WHEN DONE
    q15_t joj;
    uint32_t ind;
    arm_max_q15(audio_batch[buffer_compute_index],
              magsize_2048,
              &joj,
              &ind);
    Serial.println(joj*2);
    Serial.println((ind+1)*TARGET_AUDIO_SAMPLE / (double)AUDIO_NFFT);
  */

  for (int i = NUM_TD_FEATURES; i < NUM_FEATURES; i++) {
    // Update audio frequency domain feature variables only when the feature IS
    // audio FD and the bit is turned on for enabledFeatures.
    if ((enabledFeatures[i / 8] << (i % 8)) & (audioFDFeatures[i / 8] << (i % 8)) & B10000000) {
      calc_features[i](0);
    }
  }

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

  for (int i = NUM_TD_FEATURES; i < NUM_FEATURES; i++) {
    // Update audio frequency domain feature variables only when the feature IS
    // audio FD and the bit is turned on for enabledFeatures.
    if ((enabledFeatures[i / 8] << (i % 8)) & (audioFDFeatures[i / 8] << (i % 8)) & B10000000) {
      calc_features[i](0);
    }
  }
}

// Single instance of RFFT calculation on accel data.
// ORIGINAL TIME SERIES DATA WILL BE MODIFIED! CALCULATE TIME DOMAIN FEATURES FIRST
void accel_rfft() {
  // TODO: Figure out a way to update every frequency domain feature for accel
  // without using multiple buffers
  // NOTE: Internally update all frequency domain variables

  // Apply Hamming window in-place
  arm_mult_q15(accel_x_batch[buffer_compute_index],
               hamming_window_512,
               accel_x_batch[buffer_compute_index],
               ACCEL_NFFT);
  arm_mult_q15(accel_y_batch[buffer_compute_index],
               hamming_window_512,
               accel_y_batch[buffer_compute_index],
               ACCEL_NFFT);
  arm_mult_q15(accel_z_batch[buffer_compute_index],
               hamming_window_512,
               accel_z_batch[buffer_compute_index],
               ACCEL_NFFT);

  // First FFT, X axis
  arm_rfft_init_q15(&accel_rfft_instance,
                    &accel_cfft_instance,
                    ACCEL_NFFT,
                    0,
                    1);

  arm_rfft_q15(&accel_rfft_instance,
               accel_x_batch[buffer_compute_index],
               rfft_accel_buffer);

  arm_shift_q15(rfft_accel_buffer,
                ACCEL_RESCALE,
                rfft_accel_buffer,
                ACCEL_NFFT * 2);

  // NOTE: OUTPUT FORMAT IS IN 2.14
  arm_cmplx_mag_q15(rfft_accel_buffer,
                    accel_x_batch[buffer_compute_index],
                    ACCEL_NFFT);

  arm_q15_to_float(accel_x_batch[buffer_compute_index],
                   (float*)rfft_accel_buffer,
                   magsize_512);

  // Div by K
  arm_scale_f32((float*)rfft_accel_buffer,
                hamming_K_512,
                (float*)rfft_accel_buffer,
                magsize_512);

  // Div by wlen
  arm_scale_f32((float*)rfft_accel_buffer,
                inverse_wlen_512,
                (float*)rfft_accel_buffer,
                magsize_512);

  // Fix 2.14 format from cmplx_mag
  arm_scale_f32((float*)rfft_accel_buffer,
                2.0,
                (float*)rfft_accel_buffer,
                magsize_512);

  // Correction of the DC & Nyquist component, including Nyquist point.
  arm_scale_f32(&((float*)rfft_accel_buffer)[1],
                2.0,
                &((float*)rfft_accel_buffer)[1],
                magsize_512 - 2);

  /* DEBUGGING ROUTINE; FFT BIN CHECKER
    TODO: Remove this when done
    float sum = 0;
    for (int i=0; i<magsize_512; i++){
    Serial.print(((float*)rfft_accel_buffer)[i], 8);
    sum += ((float*)rfft_accel_buffer)[i];
    Serial.print(" ");
    }
    Serial.println();
    Serial.println(sum, 8);
  */

  for (int i = NUM_TD_FEATURES; i < NUM_FEATURES; i++) {
    // Update accel frequency domain feature variables only when the feature IS
    // accel FD and the bit is turned on for enabledFeatures.
    if ((enabledFeatures[i / 8] << (i % 8)) & (accelFDFeatures[i / 8] << (i % 8)) & B10000000) {
      calc_features[i](0);
    }
  }

  // Second FFT, Y axis
  arm_rfft_init_q15(&accel_rfft_instance,
                    &accel_cfft_instance,
                    ACCEL_NFFT,
                    0,
                    1);

  arm_rfft_q15(&accel_rfft_instance,
               accel_y_batch[buffer_compute_index],
               rfft_accel_buffer);

  arm_shift_q15(rfft_accel_buffer,
                ACCEL_RESCALE,
                rfft_accel_buffer,
                ACCEL_NFFT * 2);

  // NOTE: OUTPUT FORMAT IS IN 2.14
  arm_cmplx_mag_q15(rfft_accel_buffer,
                    accel_y_batch[buffer_compute_index],
                    ACCEL_NFFT);

  arm_q15_to_float(accel_y_batch[buffer_compute_index],
                   (float*)rfft_accel_buffer,
                   magsize_512);

  // Div by K
  arm_scale_f32((float*)rfft_accel_buffer,
                hamming_K_512,
                (float*)rfft_accel_buffer,
                magsize_512);

  // Div by wlen
  arm_scale_f32((float*)rfft_accel_buffer,
                inverse_wlen_512,
                (float*)rfft_accel_buffer,
                magsize_512);

  // Correction of the DC & Nyquist component, including Nyquist point.
  arm_scale_f32(&((float*)rfft_accel_buffer)[1],
                2.0,
                &((float*)rfft_accel_buffer)[1],
                magsize_512 - 2);

  // Fix 2.14 format from cmplx_mag
  arm_scale_f32((float*)rfft_accel_buffer,
                2.0,
                (float*)rfft_accel_buffer,
                magsize_512);

  for (int i = NUM_TD_FEATURES; i < NUM_FEATURES; i++) {
    // Update accel frequency domain feature variables only when the feature IS
    // accel FD and the bit is turned on for enabledFeatures.
    if ((enabledFeatures[i / 8] << (i % 8)) & (accelFDFeatures[i / 8] << (i % 8)) & B10000000) {
      calc_features[i](1);
    }
  }

  // Third FFT, Z axis
  arm_rfft_init_q15(&accel_rfft_instance,
                    &accel_cfft_instance,
                    ACCEL_NFFT,
                    0,
                    1);

  arm_rfft_q15(&accel_rfft_instance,
               accel_z_batch[buffer_compute_index],
               rfft_accel_buffer);

  arm_shift_q15(rfft_accel_buffer,
                ACCEL_RESCALE,
                rfft_accel_buffer,
                ACCEL_NFFT * 2);

  // NOTE: OUTPUT FORMAT IS IN 2.14
  arm_cmplx_mag_q15(rfft_accel_buffer,
                    accel_z_batch[buffer_compute_index],
                    ACCEL_NFFT);

  arm_q15_to_float(accel_z_batch[buffer_compute_index],
                   (float*)rfft_accel_buffer,
                   magsize_512);

  // Div by K
  arm_scale_f32((float*)rfft_accel_buffer,
                hamming_K_512,
                (float*)rfft_accel_buffer,
                magsize_512);

  // Div by wlen
  arm_scale_f32((float*)rfft_accel_buffer,
                inverse_wlen_512,
                (float*)rfft_accel_buffer,
                magsize_512);

  // Fix 2.14 format from cmplx_mag
  arm_scale_f32((float*)rfft_accel_buffer,
                2.0,
                (float*)rfft_accel_buffer,
                magsize_512);

  // Correction of the DC & Nyquist component, including Nyquist point.
  arm_scale_f32(&((float*)rfft_accel_buffer)[1],
                2.0,
                &((float*)rfft_accel_buffer)[1],
                magsize_512 - 2);

  for (int i = NUM_TD_FEATURES; i < NUM_FEATURES; i++) {
    // Update accel frequency domain feature variables only when the feature IS
    // accel FD and the bit is turned on for enabledFeatures.
    if ((enabledFeatures[i / 8] << (i % 8)) & (accelFDFeatures[i / 8] << (i % 8)) & B10000000) {
      calc_features[i](2);
    }
  }
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
float feature_energy () {
  // Take q15_t buffer as float buffer for computation
  float* compBuffer = (float*)&rfft_accel_buffer;
  // Reduce re-computation of variables
  int zind = ENERGY_INTERVAL_ACCEL * 2;

  // Copy from accel batches to compBuffer
  arm_q15_to_float(&accel_x_batch[buffer_compute_index][featureBatchSize / ACCEL_COUNTER_TARGET - ENERGY_INTERVAL_ACCEL],
                   compBuffer,
                   ENERGY_INTERVAL_ACCEL);
  arm_q15_to_float(&accel_y_batch[buffer_compute_index][featureBatchSize / ACCEL_COUNTER_TARGET - ENERGY_INTERVAL_ACCEL],
                   &compBuffer[ENERGY_INTERVAL_ACCEL],
                   ENERGY_INTERVAL_ACCEL);
  arm_q15_to_float(&accel_z_batch[buffer_compute_index][featureBatchSize / ACCEL_COUNTER_TARGET - ENERGY_INTERVAL_ACCEL],
                   &compBuffer[zind],
                   ENERGY_INTERVAL_ACCEL);

  // Scale all entries by aRes AND 2^15; the entries were NOT in q1.15 format.
  arm_scale_f32(compBuffer, aRes * 32768, compBuffer, ENERGY_INTERVAL_ACCEL * 3);

  // Apply accel biases
  arm_offset_f32(compBuffer, accelBias[0], compBuffer, ENERGY_INTERVAL_ACCEL);
  arm_offset_f32(&compBuffer[ENERGY_INTERVAL_ACCEL], accelBias[1], &compBuffer[ENERGY_INTERVAL_ACCEL], ENERGY_INTERVAL_ACCEL);
  arm_offset_f32(&compBuffer[zind], accelBias[2], &compBuffer[zind], ENERGY_INTERVAL_ACCEL);

  float ave_x = 0;
  float ave_y = 0;
  float ave_z = 0;

  // Calculate average
  arm_mean_f32(compBuffer, ENERGY_INTERVAL_ACCEL, &ave_x);
  arm_mean_f32(&compBuffer[ENERGY_INTERVAL_ACCEL], ENERGY_INTERVAL_ACCEL, &ave_y);
  arm_mean_f32(&compBuffer[zind], ENERGY_INTERVAL_ACCEL, &ave_z);

  // Calculate energy.
  // NOTE: Using CMSIS-DSP functions to perform the following operation performed 2840 microseconds,
  //       while just using simple loop performed 2730 microseconds. Keep the latter.
  float ene = 0;
  for (int i = 0; i < ENERGY_INTERVAL_ACCEL; i++) {
    ene += sq(compBuffer[i] - ave_x)
           +  sq(compBuffer[ENERGY_INTERVAL_ACCEL + i] - ave_y)
           +  sq(compBuffer[zind + i] - ave_z);
  }
  ene *= ENERGY_INTERVAL_ACCEL;

  return ene;
}

// Mean Crossing Rate ACCEL, index 1
float feature_mcr () {
  //TODO: Reduce redundant work; aka float conversion and bias application

  // Take q15_t buffer as float buffer for computation
  float* compBuffer = (float*)&rfft_accel_buffer;
  // Reduce re-computation of variables
  int zind = MCR_INTERVAL_ACCEL * 2;

  // Copy from accel batches to compBuffer
  arm_q15_to_float(&accel_x_batch[buffer_compute_index][featureBatchSize / ACCEL_COUNTER_TARGET - MCR_INTERVAL_ACCEL],
                   compBuffer,
                   MCR_INTERVAL_ACCEL);
  arm_q15_to_float(&accel_y_batch[buffer_compute_index][featureBatchSize / ACCEL_COUNTER_TARGET - MCR_INTERVAL_ACCEL],
                   &compBuffer[MCR_INTERVAL_ACCEL],
                   MCR_INTERVAL_ACCEL);
  arm_q15_to_float(&accel_z_batch[buffer_compute_index][featureBatchSize / ACCEL_COUNTER_TARGET - MCR_INTERVAL_ACCEL],
                   &compBuffer[zind],
                   MCR_INTERVAL_ACCEL);

  // Scale all entries by aRes AND 2^15; the entries were NOT in q1.15 format.
  arm_scale_f32(compBuffer, aRes * 32768, compBuffer, MCR_INTERVAL_ACCEL * 3);

  // Apply accel biases
  arm_offset_f32(compBuffer, accelBias[0], compBuffer, MCR_INTERVAL_ACCEL);
  arm_offset_f32(&compBuffer[MCR_INTERVAL_ACCEL], accelBias[1], &compBuffer[MCR_INTERVAL_ACCEL], MCR_INTERVAL_ACCEL);
  arm_offset_f32(&compBuffer[zind], accelBias[2], &compBuffer[zind], MCR_INTERVAL_ACCEL);

  float max_r = 0;
  float max_temp = 0;
  float max_x = 0;
  float max_y = 0;
  float max_z = 0;
  float min_x = 0;
  float min_y = 0;
  float min_z = 0;
  uint32_t index = 0;

  // Calculate average
  arm_max_f32(compBuffer, 64, &max_x, &index);
  arm_max_f32(&compBuffer[MCR_INTERVAL_ACCEL], 64, &max_y, &index);
  arm_max_f32(&compBuffer[zind], 64, &max_z, &index);
  arm_min_f32(compBuffer, 64, &min_x, &index);
  arm_min_f32(&compBuffer[MCR_INTERVAL_ACCEL], 64, &min_y, &index);
  arm_min_f32(&compBuffer[zind], 64, &min_z, &index);

  min_x = abs(min_x); min_y = abs(min_y); min_y = abs(min_y);
  max_x = abs(max_x); max_y = abs(max_y); max_y = abs(max_y);
  max_x = max(max_x, min_x);
  max_y = max(max_y, min_y);
  max_z = max(max_z, min_z);

  max_temp = max(max_x, max_y);
  max_r = max(max_temp, max_z);
  Serial.print("max value = "); Serial.println(max_r);
  return max_r*100;
}


// FREQUENCY DOMAIN FEATURES

/* CANCELLED
  // Spectral Flux ACCEL, index 2
  void calculate_spectral_flux(int axis){
  float* buff = (float*) rfft_accel_buffer;

  double logSum = 0;
  double sum = 0;

  for (int i=0; i<magsize_512; i++){
    sum += buff[i];
    logSum += log(buff[i]);
  }

  double flux = exp((logSum / magsize_512)) / (sum / magsize_512);

  switch(axis){
    case 0: sp_flux_x = flux; break;
    case 1: sp_flux_y = flux; break;
    case 2: sp_flux_z = flux; break;
  }
  }

  float feature_spectral_flux(){
  return (float)sp_flux_x;
  }
*/

// Spectral Centroid ACCEL, index 2
void calculate_spectral_centroid(int axis) {
  // TODO: Could use a LOT of CMSIS-DSP functions. Maybe optimize it later.
  float* buff = (float*) rfft_accel_buffer;

  float centroid = 0;
  float sp_centroid_fake = 0;

  for (int i = 0; i < 50; i++) {
    sp_centroid_fake += sq(buff[i]);
  }
  sp_centroid_fake = sp_centroid_fake * float(1000000); //if it needs to be scaled!
  centroid = sp_centroid_fake;

  switch (axis) {
    case 0: sp_cent_x = centroid; break;
    case 1: sp_cent_y = centroid; break;
    case 2: sp_cent_z = centroid; break;
  }
}

float feature_spectral_centroid() {
  return (float) sqrt(sq(sp_cent_x) + sq(sp_cent_y) + sq(sp_cent_z));
}

// Spectral Flatness ACCEL, index 3
void calculate_spectral_flatness(int axis) {
  float* buff = (float*) rfft_accel_buffer;

  float flatness = 0;
  float sp_flatness_fake = 0;

  for (int i = 50; i < 100; i++) {
    sp_flatness_fake += sq(buff[i]);
  }
  sp_flatness_fake = sp_flatness_fake * float(1000000); //if it needs to be scaled!
  flatness = sp_flatness_fake;

  switch (axis) {
    case 0: sp_flat_x = flatness; break;
    case 1: sp_flat_y = flatness; break;
    case 2: sp_flat_z = flatness; break;
  }
}

float feature_spectral_flatness() {
  return (float) sqrt(sq(sp_flat_x) + sq(sp_flat_y) + sq(sp_flat_z));
}

/* CANCELLED
  // Spectral Crest ACCEL, index 5
  void calculate_spectral_crest(int axis){
  float* buff = (float*) rfft_accel_buffer;

  float sqSum = 0;
  float maxVal = 0;
  uint32_t maxInd = 0;
  arm_dot_prod_f32(buff, buff, magsize_512, &sqSum);
  arm_max_f32(buff, magsize_512, &maxVal, &maxInd);

  float crest = maxVal / sqSum;

  switch(axis){
    case 0: sp_crest_x = crest; break;
    case 1: sp_crest_y = crest; break;
    case 2: sp_crest_z = crest; break;
  }
  }

  float feature_spectral_crest(){
  return sp_crest_x;
  }
*/

// Spectral Spread ACCEL, index 4
void calculate_spectral_spread_accel(int axis) {
  // TODO: Could use more CMSIS-DSP functions. Maybe optimize it later.
  float* buff = (float*) rfft_accel_buffer;

  float sp_spread_accel = 0;
  float sp_spread_accel_fake = 0;

  for (int i = 100; i < 240; i++) {
    sp_spread_accel_fake += sq(buff[i]);
  }
  sp_spread_accel_fake = sp_spread_accel_fake * float(1000000); //if it needs to be scaled!
  sp_spread_accel = sp_spread_accel_fake;

  switch (axis) {
    case 0: sp_spread_x = sp_spread_accel; break;
    case 1: sp_spread_y = sp_spread_accel; break;
    case 2: sp_spread_z = sp_spread_accel; break;
  }
}

float feature_spectral_spread_accel() {
  return (float) sqrt(sq(sp_spread_x) + sq(sp_spread_y) + sq(sp_spread_z)) / (float)100;
}

// Spectral Spread AUDIO, index 5
void calculate_spectral_spread_audio(int axis) {
  float* buff = (float*) rfft_audio_buffer;

  sp_spread_aud = 0;
  float sp_spread_aud_fake = 0;

  for (int i = 0; i < 150; i++) {
    sp_spread_aud_fake += sq(buff[i]);
  }
  sp_spread_aud_fake = sp_spread_aud_fake * float(1000000); //if it needs to be scaled!

  sp_spread_aud = sp_spread_aud_fake;
}

float feature_spectral_spread_audio() {
  //NOTE: Audio Spectral Spread has reversed thresholds; keep as negative values. //deprecated
  return (float) sp_spread_aud;
}


//==============================================================================
//================================= Main Code ==================================
//==============================================================================

// extract the 24bit INMP441 audio data from 32bit sample
void extractdata_inplace(int32_t  *pBuf) {
  // set highest bit to zero, then bitshift right 7 times
  // do not omit the first part (!)
  pBuf[0] = (pBuf[0] & 0x7fffffff) >> 7;
}


#define bufferSize 8000*3
const uint16_t target_sample = 8000;
const uint16_t inter = 48000 / target_sample;
/*uint32_t subsample_counter = 0;*/
/*uint32_t accel_counter = 0;*/
unsigned char audio_buffer[bufferSize];
uint32_t nTX = 0;
uint32_t nRX = 0;
const uint32_t intvl = 100;
uint32_t totCount = 0;
uint32_t localCount = 0;
float accel_x[intvl];
float accel_y[intvl];
float accel_z[intvl];
float thres = 100;
float thres2 = 750;
float thres3 = 1500;
float ave_x = 0;
float ave_y = 0;
float ave_z = 0;
float ene = 0; //vibration sugnal energy value
float state = 0;
float stateprev = 0;
boolean over = false;
/*int greenPin = 27;*/
/*int redPin = 29;*/
/*int bluePin = 28;*/
int greenPinState = HIGH; //boot color; GREEN PIN
int redPinState = HIGH; //boot color; RED PIN
int bluePinState = HIGH; //boot color; BLUE PIN
int bluestart = 0; //timer to know how long it has been idle
/*int blue = 0; //toggle to know how if idle*/
int millisnow = 0;
int bluesleep = 5000; //threshold for sleep mode
/*int sleepmode = 0; //initialize to not in sleep mode*/
/*int currentmillis = 0; //battery monitoring*/
/*int prevmillis = 0; //battery monitoring*/
int batlimit = 1000; //battery monitoring
int overallcounter = 0;
/*int newThres = 0;*/
/*int newThres2 = 0;*/
/*int newThres3 = 0;*/
/*int args_assigned = 0;*/
/*boolean silent = false;*/
/*unsigned char bytes[4];*/

/* --- Direct I2S Receive, we get callback to read 2 words from the FIFO --- */

//void i2s_rx_callback( int32_t *pBuf )
//{
//  //digitalWrite(28,HIGH);
//  //Serial.print(accel_counter);
//  // Downsampling routine; only take every 6th sample.
//  Serial.print("Starting callback...\n");
//  if (subsample_counter != inter - 2) {
//    subsample_counter ++;
//    Serial.printf("Subsample counter: Value: %d\n", subsample_counter);
//    return;
//  } else {
//    subsample_counter = 0;
//    accel_counter++;
//    Serial.printf("Accel counter: Value: %d\n", accel_counter);
//  }
//  extractdata_inplace(&pBuf[0]);
//
//  if (accel_counter == 8) {
//    //Perform accelerometer data extraction
//    readAccelData(accelCount);
//    getAres();
//    ax = (float)accelCount[0] * aRes;// + accelBias[0];
//    ay = (float)accelCount[1] * aRes;// + accelBias[1];
//    az = (float)accelCount[2] * aRes;// + accelBias[2];
//
//    byte* axb = (byte *) &ax;
//    byte* ayb = (byte *) &ay;
//    byte* azb = (byte *) &az;
//    accel_counter = 0;
//    //Serial.print("ax="); Serial.print(ax); Serial.print("\n");
//    // Feature Calculation
//
//    accel_x[localCount] = ax;
//    accel_y[localCount] = ay;
//    accel_z[localCount] = az;
//    totCount = totCount + 1;
//    localCount = localCount + 1;
//    localCount = localCount % intvl;
//    if (totCount > intvl) {
//      totCount = intvl + 1;
//      over = true;
//    }
//    if (over) {
//      // Calculate the mean only if we have at least 100 data points
//      ave_x = 0;
//      ave_y = 0;
//      ave_z = 0;
//      for (int i = 0; i < intvl; i++) {
//        ave_x = ave_x + accel_x[i];
//        ave_y = ave_y + accel_y[i];
//        ave_z = ave_z  + accel_z[i];
//      }
//      ave_x = ave_x / intvl;
//      ave_y = ave_y / intvl;
//      ave_z = ave_z / intvl;
//      ene = 0; //initialize the energy feature
//      for (int i = 0; i < intvl; i++) {
//        ene = ene + (accel_x[i] - ave_x) * (accel_x[i] - ave_x) + (accel_y[i] - ave_y) * (accel_y[i] - ave_y) + (accel_z[i] - ave_z) * (accel_z[i] - ave_z);
//      }
//      ene = ene * intvl; //Signal energy integrated
//
//      if (ene < thres) {
//
//        // blue: normal cutting
//
//        if (blue == 0) {
//          bluestart = millis();
//          blue = 1;
//        }
//        if (blue == 1) {
//          millisnow = millis();
//          if (millisnow - bluestart >= bluesleeplimit) { //sleep mode
//            sleepmode = true;
//            greenPinState = HIGH;
//            redPinState = HIGH;
//            bluePinState = HIGH;
//            digitalWrite(greenPin, greenPinState);
//            digitalWrite(redPin, redPinState);
//            digitalWrite(bluePin, bluePinState);
//            blue = 0;
//          }
//        }
//
//        if (sleepmode == 0) {
//          bluePinState = LOW;
//          redPinState = HIGH;
//          greenPinState = LOW;
//        }
//        digitalWrite(greenPin, greenPinState);
//        digitalWrite(redPin, redPinState);
//        digitalWrite(bluePin, bluePinState);
//        currState = NOT_CUTTING;
//      }
//      else if (ene < thres2) {
//        blue = 0;
//        sleepmode = 0;
//        // green: normal cutting
//
//        bluePinState = HIGH;
//        redPinState = HIGH;
//        greenPinState = LOW;
//        digitalWrite(greenPin, greenPinState);
//        digitalWrite(redPin, redPinState);
//        digitalWrite(bluePin, bluePinState);
//        currState = NORMAL_CUTTING;
//      }
//      else if (ene < thres3) {
//        blue = 0;
//        sleepmode = 0;
//        // yellow: warning
//
//        bluePinState = HIGH;
//        redPinState = LOW;
//        greenPinState = LOW;
//        digitalWrite(greenPin, greenPinState);
//        digitalWrite(redPin, redPinState);
//        digitalWrite(bluePin, bluePinState);
//        currState = WARNING_CUTTING;
//      }
//      else {
//        // red: bad cutting
//        blue = 0;
//        sleepmode = 0;
//        bluePinState = HIGH;
//        redPinState = LOW;
//        greenPinState = HIGH;
//        digitalWrite(greenPin, greenPinState);
//        digitalWrite(redPin, redPinState);
//        digitalWrite(bluePin, bluePinState);
//        currState = BAD_CUTTING;
//      }
//    }
//  }
//}

void i2s_rx_callback( int32_t *pBuf )
{
  // Don't do anything if CHARGING
  if (currMode == CHARGING) {
    //Serial.print("Returning on charge\n");
    return;
  }

  if (currMode == RUN) {
    // If feature computation is lagging behind, reset all sampling routine and
    // Don't run any data collection.
    // TODO: Whenever this happens and STFT features are turned on, reset STFT
    // TODO: May want to do timing checks to save STFT resets
    if (audioSamplingCount == 0 && !record_feature_now[buffer_record_index]) {
      //Serial.print("Returning on other\n");
      return;
    }

    // Filled out the current buffer; switch buffer index
    if (audioSamplingCount == featureBatchSize) {
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
  if (subsample_counter != DOWNSAMPLE_MAX - 2) {
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

        accel_x_batch[buffer_record_index][accelSamplingCount] = accelCount[0];
        accel_y_batch[buffer_record_index][accelSamplingCount] = accelCount[1];
        accel_z_batch[buffer_record_index][accelSamplingCount] = accelCount[2];
        accelSamplingCount ++;

        accel_counter = 0;
      }
    }
  } else if (currMode == DATA_COLLECTION) {
    //Serial.print("currMode=DATA COLLECTION\n");
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

/* --- Direct I2S Transmit, we get callback to put 2 words into the FIFO --- */

void i2s_tx_callback( int32_t *pBuf )
{
  // This function won't be used at all; leave it empty.
}

/* --- Time functions --- */

double gettimestamp() {
  Serial.print("gettimestamp called : ");
  Serial.println(dateyear);
  return dateyear;
}

/* ----------------------- begin -------------------- */

void setup()
{

  //  Wire.begin();
  //  TWBR = 12;  // 400 kbit/sec I2C speed for Pro Mini
  // Setup for Master mode, pins 18/19, external pullups, 400kHz for Teensy 3.1
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  delay(1000);

  Serial.begin(115200);
  delay(1000);

  I2Cscan(); // should detect SENtral at 0x28

  // Set up the SENtral as sensor bus in normal operating mode

  // ld pass thru
  // Put EM7180 SENtral into pass-through mode
  SENtralPassThroughMode();

  // should see all the devices on the I2C bus including two from the EEPROM (ID page and data pages)
  I2Cscan();

  if (statusLED) {
    pinMode(27, OUTPUT);
    pinMode(28, OUTPUT);
    pinMode(29, OUTPUT);
  }

  // Read the BMX-055 WHO_AM_I registers, this is a good test of communication
  //Serial.println("BMX055 accelerometer...");

  //DEPRECATED
  /*byte c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_WHOAMI);  // Read ACC WHO_AM_I register for BMX055
    //Serial.print("BMX055 ACC"); Serial.print(" I AM 0x"); Serial.print(c, HEX); Serial.print(" I should be 0x"); Serial.println(0xFA, HEX);

    if (c == 0xFA) // WHO_AM_I should always be ACC = 0xFA, GYRO = 0x0F, MAG = 0x32
    {
    //Serial.println("BMX055 is online...");

    initBMX055();

    // get sensor resolutions, only need to do this once
    getAres();

    fastcompaccelBMX055(accelBias);
    //Serial.println("accel biases (mg)"); Serial.println(1000.*accelBias[0]); Serial.println(1000.*accelBias[1]); Serial.println(1000.*accelBias[2]);
    }*/
  byte c = readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250); // Read WHO_AM_I register on MPU
  if (c == 0x71) { //WHO_AM_I should always be 0x71 for the entire chip
    initMPU9250();

    getAres();

    // Leaving this out for now (will allow 1g bias temporarily)
    /*fastcompaccelMPU9250(accelBias);*/
  }
  else
  {
    Serial.print("Could not connect to MPU9250: 0x");
    Serial.println(c, HEX);
    while (1) ; // Loop forever if communication doesn't happen
  }

  init_time = millis();
  //Serial.print(init_time);

  // << nothing before the first delay will be printed to the serial
  delay(1500);

  if (!silent) {
    Serial.print("Pin configuration setting: ");
    Serial.println( I2S_PIN_PATTERN , HEX );
    Serial.println( "Initializing." );
  }

  if (!silent) Serial.println( "Initialized I2C Codec" );

  // prepare I2S RX with interrupts
  I2SRx0.begin( CLOCK_TYPE, i2s_rx_callback );
  if (!silent) Serial.println( "Initialized I2S RX without DMA" );

  // I2STx0.begin( CLOCK_TYPE, i2s_tx_callback );
  // Serial.println( "Initialized I2S TX without DMA" );

  delay(5000);
  // start the I2S RX
  I2SRx0.start();
  //I2STx0.start();
  if (!silent) Serial.println( "Started I2S RX" );

  //For Battery Monitoring//
  analogReference(EXTERNAL);
  analogReadResolution(12);
  analogReadAveraging(32);
  bat = getInputVoltage();

  // Calculate feature size and which ones are selected
  for (int i = 0; i < NUM_FEATURES; i++) {
    // Consider only when corresponding bit is turned on.
    if ((enabledFeatures[i / 8] << (i % 8)) & B10000000) {
      FeatureID[chosen_features] = i;
      chosen_features++;
    }
  }

  // Enable LiPo charger's CHG read.
  pinMode(chargerCHG, INPUT_PULLUP);

  // Make sure to initialize BLEport at the end of setup
  BLEport.begin(9600);
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
  if (bitRead(USB0_OTGSTAT, 5)) {
    //if (1==1) {
  //      // Disconnected; this flag will briefly come up when USB gets disconnected.
  //      // Immediately put into RUN mode.
      if (currMode != RUN) { //If disconnected and NOT in RUN mode, set to run mode with idle state initialized
          BLEport.begin(9600);
          BLEport.flush();
          resetSampling(true);
          currMode = RUN;
          LEDColors c = BLUE_NOOP;
          changeStatusLED(c);
  //
  //        // Reset state
          currState = NOT_CUTTING;
      }
  } else {
  //      // Connected
      if (currMode == RUN) {
        // NOTE: Assume the battery will start charging when USB is connected.
        if (digitalRead(chargerCHG) == 1) { //If connected, charging and in RUN mode, set to CHARGING mode
          //BLEport.flush();
          //BLEport.end();
          //LEDColors c = PURPLE_CHARGE;
          //changeStatusLED(c);
          //currMode = CHARGING;
        } else {
          if (didThresChange) { //Weirdly, the logic for chargerCHG switches once ble data is received!
            //BLEport.flush();
            //BLEport.end();
            //LEDColors c = PURPLE_CHARGE;
            //changeStatusLED(c);
            //currMode = CHARGING;
          }
       }
      } else if (currMode == CHARGING) { //If already in charging mode
        // CHARGE mode and battery is NOT charging.
        // Display white LED to notify charge completion.
        if (digitalRead(chargerCHG) == 0) {
          if (didThresChange) {//Weirdly, the logic for chargerCHG switches once ble data is received!
            //LEDColors c = PURPLE_CHARGE;
            //changeStatusLED(c);
          }
          else {
            //LEDColors c = WHITE_NONE;
            //changeStatusLED(c);
          }
  
        } else {
          if (didThresChange) { //Weirdly, the logic for chargerCHG switches once ble data is received!
            //LEDColors c = WHITE_NONE;
            //changeStatusLED(c);
          }
          else {
            //LEDColors c = PURPLE_CHARGE;
            //changeStatusLED(c);
          }
        }
      }
  }
  /*Serial.print("USB connection check complete. Starting BLE\n");*/

  /* ----------- Two-way Communication via BLE, only on RUN mode. ----------- */

  if (currMode == RUN) {
    while (BLEport.available() > 0) {
      bleBuffer[bleBufferIndex] = BLEport.read();
      bleBufferIndex++;

      if (bleBufferIndex > 18) {
        Serial.println(bleBuffer);
        if (bleBuffer[0] == '0') {
          args_assigned = sscanf(bleBuffer, "%d-%d-%d-%d", &bleFeatureIndex, &newThres, &newThres2, &newThres3);
          didThresChange = true;
          bleBufferIndex = 0;
          if (bleFeatureIndex > 5 || newThres > 9999 || newThres2 > 9999 || newThres3 > 9999 || newThres < 1 || newThres2 < 1 || newThres3 < 1) {
            //Serial.println("Rubbish data");
            rubbish = true;
          }
          if (!rubbish) {
            featureNormalThreshold[bleFeatureIndex] = newThres;
            featureWarningThreshold[bleFeatureIndex] = newThres2;
            featureDangerThreshold[bleFeatureIndex] = newThres3;
            newThres = 0;
            newThres2 = 0;
            newThres3 = 0;
          }
          rubbish = false;
        }

        // receive the timestamp data from the hub
        if (bleBuffer[0] == '1') {
          args_assigned2 = sscanf(bleBuffer, "%d:%d.%d", &date, &dateset, &dateyear1);
          dateyear = double(dateset) + double(dateyear1) / double(1000000);
          Serial.println(dateyear);
        }
        bleBufferIndex = 0;
      }
    }
  }
  
  /*Serial.print("BLE complete.\n");*/

  //  if (currMode == RUN) {
  //    bleBufferIndex = 0;
  //    while (BLEport.available() > 0) {
  //      bleBuffer[bleBufferIndex] = BLEport.read();
  //      Serial.println(char(bleBuffer[bleBufferIndex]));
  //      bleBufferIndex ++;
  //      // NOTE: BLE communication may come in 8 bytes, not 4 bytes, due to
  //      //       interrupt delays. When it goes OVER 4 bytes, just cut it at 4.
  //      if (bleBufferIndex == 4) {
  //        break;
  //      }
  //      Serial.println(bleBufferIndex);
  //    }
  //    if (bleBufferIndex == 4) {
  //      //Two-way Communication Protocol
  //      switch (currComm) {
  //        case COMM_IDLE:
  //          if (String(bleBuffer) == THRESH_START_FLAG) {
  //            currComm = THRESH;
  //          } else if (String(bleBuffer) == FEATURE_SET_START_FLAG) {
  //            currComm = FEATURE_SET;
  //          }
  //          break;
  //        case THRESH:
  //          if (bleBytesRead == 0) {
  //            bleFeatureIndex = (*(uint16_t *)(&bleBuffer[0]));
  //            bleFeatureType = (*(uint16_t *)(&bleBuffer[2]));
  //
  //            // Unexpected input; reset communication
  //            if (bleFeatureType != 0 && bleFeatureType != 1) {
  //              BLEport.print(THRESH_FAIL);
  //              BLEport.flush();
  //              bleBytesRead = 0;
  //              bleFeatureIndex = 0;
  //              currComm = COMM_IDLE;
  //              break;
  //            }
  //            if (bleFeatureIndex < 0 || bleFeatureIndex >= NUM_FEATURES) {
  //              BLEport.print(THRESH_FAIL);
  //              BLEport.flush();
  //              bleBytesRead = 0;
  //              bleFeatureIndex = 0;
  //              currComm = COMM_IDLE;
  //              break;
  //            }
  //
  //            Serial.println(bleFeatureIndex);
  //            Serial.println(bleFeatureType);
  //            bleBytesRead = 4;
  //          } else if (bleBytesRead == 4) {
  //            // THRESH communication completed.
  //            if (bleFeatureType == 0) {
  //              featureWarningThreshold[bleFeatureIndex] = (*(float *)(&bleBuffer[0]));
  //            } else if (bleFeatureType == 1) {
  //              featureDangerThreshold[bleFeatureIndex] = (*(float *)(&bleBuffer[0]));
  //            }
  //            // Transmit THRESH_SUCCESS
  //            BLEport.print(THRESH_SUCCESS);
  //            BLEport.flush();
  //
  //            // Reset Variables
  //            bleBytesRead = 0;
  //            bleFeatureIndex = 0;
  //            currComm = COMM_IDLE;
  //          }
  //          break;
  //        case FEATURE_SET:
  //          if (bleBytesRead < FEATURE_SET_BYTELEN) {
  //            bleBufferIndex = 0;
  //            while (bleFeatureIndex < ((NUM_FEATURES - 1) / 8 + 1) && bleBufferIndex < 4) {
  //              bleFeaturesBuffer[bleFeatureIndex] = bleBuffer[bleBufferIndex];
  //
  //              bleFeatureIndex ++;
  //              bleBufferIndex ++;
  //            }
  //            bleBytesRead += 4;
  //          } else if (bleBytesRead == FEATURE_SET_BYTELEN) {
  //            // FEATURE_SET communication completed.
  //            BLEport.print(FEATURE_SET_SUCCESS);
  //            BLEport.flush();
  //
  //            // Copy over to enabledFeatures
  //            for (int i = 0; i < (NUM_FEATURES - 1) / 8 + 1; i++) {
  //              enabledFeatures[i] = bleFeaturesBuffer[i];
  //            }
  //
  //            // Set the FFT flags according to the new feature set
  //            for (int i = NUM_TD_FEATURES; i < NUM_FEATURES; i++) {
  //              if ((enabledFeatures[i / 8] << (i % 8)) & (audioFDFeatures[i / 8] << (i % 8)) & B10000000) {
  //                audioFDCompute = true;
  //              }
  //            }
  //            for (int i = NUM_TD_FEATURES; i < NUM_FEATURES; i++) {
  //              if ((enabledFeatures[i / 8] << (i % 8)) & (accelFDFeatures[i / 8] << (i % 8)) & B10000000) {
  //                accelFDCompute = true;
  //              }
  //            }
  //
  //            largestBatchSize = 0;
  //            for (int i = 0; i < NUM_FEATURES; i++) {
  //              // Consider batch size only when corresponding bit is turned on.
  //              if ((enabledFeatures[i / 8] << (i % 8)) & B10000000) {
  //                largestBatchSize = max(largestBatchSize, featureIntervals[i]);
  //              }
  //            }
  //
  //            featureBatchSize = largestBatchSize;
  //            resetSampling(true);
  //
  //            // Reset Variables
  //            bleBytesRead = 0;
  //            bleFeatureIndex = 0;
  //            currComm = COMM_IDLE;
  //          } else {
  //            //NOTE: this should not happen if all BLE transmissions are 4 bytes aligned.
  //            BLEport.print(FEATURE_SET_FAIL);
  //            BLEport.flush();
  //            bleBytesRead = 0;
  //            bleFeatureIndex = 0;
  //            currComm = COMM_IDLE;
  //          }
  //          break;
  //      }
  //    } else {
  //      //Incorrect bytes received; invalidate current communication.
  //      switch (currComm) {
  //        case THRESH:
  //          BLEport.print(THRESH_FAIL);
  //          break;
  //        case FEATURE_SET:
  //          BLEport.print(FEATURE_SET_FAIL);
  //          break;
  //      }
  //      BLEport.flush();
  //      bleBytesRead = 0;
  //      bleFeatureIndex = 0;
  //      currComm = COMM_IDLE;
  //    }
  //  }

  /* ------------- Hardwire Serial for DATA_COLLECTION commands ------------- */
  while (Serial.available() > 0) {
    characterRead = Serial.read();
    if (characterRead != '\n')
      wireBuffer.concat(characterRead);
  }
  if (wireBuffer.length() > 0) {
    //if (currMode == CHARGING) {
    if (currMode == RUN) {
      // Check the received info; iff data collection request, change the mode
      if (wireBuffer == START_COLLECTION) {
        LEDColors c = CYAN_DATA;
        changeStatusLED(c);

        Serial.print(START_CONFIRM);
        resetSampling(false);
        currMode = DATA_COLLECTION;
      }
    } else if (currMode == DATA_COLLECTION) {
      // Check the received info; iff data collection finished, change the mode
      if (wireBuffer == END_COLLECTION) {
        //LEDColors c = PURPLE_CHARGE;
        LEDColors c = BLUE_NOOP;
        changeStatusLED(c);

        Serial.print(END_CONFIRM);
        //currMode = CHARGING;
        currMode = RUN;
      }
    }
    // Clear wire buffer
    wireBuffer = "";
  }

  // After handling all state / mode changes, check if features need to be computed.
  if (currMode == RUN && compute_feature_now[buffer_compute_index]) {
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
    TARGET_AUDIO_SAMPLE = AUDIO_FREQ_RUN;
  } else {
    TARGET_AUDIO_SAMPLE = AUDIO_FREQ_DATA;
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
    // CHANGE: MPU data is signed 16 bit
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

// DEPRECATED
/*void readAccelData(int16_t * destination)
  {
  uint8_t rawData[6];  // x/y/z accel register data stored here
  readBytes(BMX055_ACC_ADDRESS, BMX055_ACC_D_X_LSB, 6, &rawData[0]);       // Read the six raw data registers into data array
  if ((rawData[0] & 0x01) && (rawData[2] & 0x01) && (rawData[4] & 0x01)) { // Check that all 3 axes have new data
    destination[0] = (int16_t) (((int16_t)rawData[1] << 8) | rawData[0]) >> 4;  // Turn the MSB and LSB into a signed 12-bit value
    destination[1] = (int16_t) (((int16_t)rawData[3] << 8) | rawData[2]) >> 4;
    destination[2] = (int16_t) (((int16_t)rawData[5] << 8) | rawData[4]) >> 4;
  }
  }*/

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

  // Configure Gyro and Thermometer
  // Disable FSYNC and set thermometer and gyro bandwidth to 41 and 42 Hz, respectively;
  // minimum delay time for this setting is 5.9 ms, which means sensor fusion update rates cannot
  // be higher than 1 / 0.0059 = 170 Hz
  // DLPF_CFG = bits 2:0 = 011; this limits the sample rate to 1000 Hz for both
  // With the MPU9250, it is possible to get gyro sample rates of 32 kHz (!), 8 kHz, or 1 kHz

  //DISABLED FOR NOW
  /*writeByte(MPU9250_ADDRESS, CONFIG, 0x03);*/

  // Set sample rate = gyroscope output rate/(1 + SMPLRT_DIV)
  writeByte(MPU9250_ADDRESS, SMPLRT_DIV, 0x00);  // Use a 200 Hz rate; a rate consistent with the filter update rate
  // determined inset in CONFIG above

  // DISABLED FOR NOW
  // Set gyroscope full scale range
  // Range selects FS_SEL and AFS_SEL are 0 - 3, so 2-bit values are left-shifted into positions 4:3

  /*uint8_t c = readByte(MPU9250_ADDRESS, GYRO_CONFIG); // get current GYRO_CONFIG register value*/

  // c = c & ~0xE0; // Clear self-test bits [7:5]

  /*c = c & ~0x02; // Clear Fchoice bits [1:0]
    c = c & ~0x18; // Clear AFS bits [4:3]
    c = c | Gscale << 3; // Set full scale range for the gyro*/

  // c =| 0x00; // Set Fchoice for the gyro to 11 by writing its inverse to bits 1:0 of GYRO_CONFIG

  /*writeByte(MPU9250_ADDRESS, GYRO_CONFIG, c ); // Write new GYRO_CONFIG value to register*/

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

// DEPRECATED
/*void initBMX055()
  {
  // start with all sensors in default mode with all registers reset
  writeByte(MPU9250_ADDRESS,  BMX055_ACC_BGW_SOFTRESET, 0xB6);  // reset accelerometer
  delay(1000); // Wait for all registers to reset

  // Configure accelerometer
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_PMU_RANGE, Ascale & 0x0F); // Set accelerometer full range
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_PMU_BW, ACCBW & 0x0F);     // Set accelerometer bandwidth
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_D_HBW, 0x00);              // Use filtered data

  // //   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_EN_1, 0x10);           // Enable ACC data ready interrupt
  // //   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_OUT_CTRL, 0x04);       // Set interrupts push-pull, active high for INT1 and INT2
  // //   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_MAP_1, 0x02);        // Define INT1 (intACC1) as ACC data ready interrupt
  // //   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_INT_MAP_1, 0x80);          // Define INT2 (intACC2) as ACC data ready interrupt

  // //   writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_BGW_SPI3_WDT, 0x06);       // Set watchdog timer for 50 ms

  // // Configure Gyro
  // // start by resetting gyro, better not since it ends up in sleep mode?!
  // // writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_BGW_SOFTRESET, 0xB6); // reset gyro
  // // delay(100);
  // // Three power modes, 0x00 Normal,
  // // set bit 7 to 1 for suspend mode, set bit 5 to 1 for deep suspend mode
  // // sleep duration in fast-power up from suspend mode is set by bits 1 - 3
  // // 000 for 2 ms, 111 for 20 ms, etc.
  // //  writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_LPM1, 0x00);  // set GYRO normal mode
  // //  set GYRO sleep duration for fast power-up mode to 20 ms, for duty cycle of 50%
  // //  writeByte(BMX055_ACC_ADDRESS, BMX055_GYRO_LPM1, 0x0E);
  // // set bit 7 to 1 for fast-power-up mode,  gyro goes quickly to normal mode upon wake up
  // // can set external wake-up interrupts on bits 5 and 4
  // // auto-sleep wake duration set in bits 2-0, 001 4 ms, 111 40 ms
  // //  writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_LPM2, 0x00);  // set GYRO normal mode
  // // set gyro to fast wake up mode, will sleep for 20 ms then run normally for 20 ms
  // // and collect data for an effective ODR of 50 Hz, other duty cycles are possible but there
  // // is a minimum wake duration determined by the bandwidth duration, e.g.,  > 10 ms for 23Hz gyro bandwidth
  // //  writeByte(BMX055_ACC_ADDRESS, BMX055_GYRO_LPM2, 0x87);

  // writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_RANGE, Gscale);  // set GYRO FS range
  // writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_BW, GODRBW);     // set GYRO ODR and Bandwidth

  // // writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_EN_0, 0x80);  // enable data ready interrupt
  // // writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_EN_1, 0x04);  // select push-pull, active high interrupts
  // // writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_INT_MAP_1, 0x80); // select INT3 (intGYRO1) as GYRO data ready interrupt

  // // writeByte(BMX055_GYRO_ADDRESS, BMX055_GYRO_BGW_SPI3_WDT, 0x06); // Enable watchdog timer for I2C with 50 ms window


  // // Configure magnetometer
  // writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL1, 0x82);  // Softreset magnetometer, ends up in sleep mode
  // delay(100);
  // writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL1, 0x01); // Wake up magnetometer
  // delay(100);

  // writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL2, MODR << 3); // Normal mode
  // //writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_PWR_CNTL2, MODR << 3 | 0x02); // Forced mode

  // //writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_INT_EN_2, 0x84); // Enable data ready pin interrupt, active high

  // // Set up four standard configurations for the magnetometer
  // switch (Mmode)
  // {
  //   case lowPower:
  //     // Low-power
  //     writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x01);  // 3 repetitions (oversampling)
  //     writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x02);  // 3 repetitions (oversampling)
  //     break;
  //   case Regular:
  //     // Regular
  //     writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x04);  //  9 repetitions (oversampling)
  //     writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x16);  // 15 repetitions (oversampling)
  //     break;
  //   case enhancedRegular:
  //     // Enhanced Regular
  //     writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x07);  // 15 repetitions (oversampling)
  //     writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x22);  // 27 repetitions (oversampling)
  //     break;
  //   case highAccuracy:
  //     // High Accuracy
  //     writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_XY, 0x17);  // 47 repetitions (oversampling)
  //     writeByte(BMX055_MAG_ADDRESS, BMX055_MAG_REP_Z,  0x51);  // 83 repetitions (oversampling)
  //     break;
  // }
  }*/

//DEPRECATED
/*void fastcompaccelBMX055(float * dest1)
  {
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_CTRL, 0x80); // set all accel offset compensation registers to zero
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_SETTING, 0x20);  // set offset targets to 0, 0, and +1 g for x, y, z axes
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_CTRL, 0x20); // calculate x-axis offset

  byte c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_CTRL);
  while (!(c & 0x10)) {  // check if fast calibration complete
    c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_CTRL);
    delay(10);
  }
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_CTRL, 0x40); // calculate y-axis offset

  c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_CTRL);
  while (!(c & 0x10)) {  // check if fast calibration complete
    c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_CTRL);
    delay(10);
  }
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_CTRL, 0x60); // calculate z-axis offset

  c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_CTRL);
  while (!(c & 0x10)) {  // check if fast calibration complete
    c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_CTRL);
    delay(10);
  }

  int8_t compx = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_OFFSET_X);
  int8_t compy = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_OFFSET_Y);
  int8_t compz = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_OFC_OFFSET_Z);

  dest1[0] = (float) compx / 128.; // accleration bias in g
  dest1[1] = (float) compy / 128.; // accleration bias in g
  dest1[2] = (float) compz / 128.; // accleration bias in g
  }*/


// I2C communication with the M24512DFM EEPROM is a little different from I2C communication with the usual motion sensor
// since the address is defined by two bytes


// I2C communication with the MS5637 is a little different from that with the MPU9250 and most other sensors
// For the MS5637, we write commands, and the MS5637 sends data in response, rather than directly reading
// MS5637 registers

//void MS5637Reset()
//{
//  Wire.beginTransmission(MS5637_ADDRESS);  // Initialize the Tx buffer
//  Wire.write(MS5637_RESET);                // Put reset command in Tx buffer
//  Wire.endTransmission();                  // Send the Tx buffer
//}

//void MS5637PromRead(uint16_t * destination)
//{
//  uint8_t data[2] = {0, 0};
//  for (uint8_t ii = 0; ii < 7; ii++) {
//    Wire.beginTransmission(MS5637_ADDRESS);  // Initialize the Tx buffer
//    Wire.write(0xA0 | ii << 1);              // Put PROM address in Tx buffer
//    Wire.endTransmission(I2C_NOSTOP);        // Send the Tx buffer, but send a restart to keep connection alive
//    uint8_t i = 0;
//    Wire.requestFrom(MS5637_ADDRESS, 2);   // Read two bytes from slave PROM address
//    while (Wire.available()) {
//      data[i++] = Wire.read();
//    }               // Put read results in the Rx buffer
//    destination[ii] = (uint16_t) (((uint16_t) data[0] << 8) | data[1]); // construct PROM data for return to main program
//  }
//}

//uint32_t MS5637Read(uint8_t CMD, uint8_t OSR)  // temperature data read
//{
//  uint8_t data[3] = {0, 0, 0};
//  Wire.beginTransmission(MS5637_ADDRESS);  // Initialize the Tx buffer
//  Wire.write(CMD | OSR);                  // Put pressure conversion command in Tx buffer
//  Wire.endTransmission(I2C_NOSTOP);        // Send the Tx buffer, but send a restart to keep connection alive
//
//  switch (OSR)
//  {
//    case ADC_256: delay(1); break;  // delay for conversion to complete
//    case ADC_512: delay(3); break;
//    case ADC_1024: delay(4); break;
//    case ADC_2048: delay(6); break;
//    case ADC_4096: delay(10); break;
//    case ADC_8192: delay(20); break;
//  }
//
//  Wire.beginTransmission(MS5637_ADDRESS);  // Initialize the Tx buffer
//  Wire.write(0x00);                        // Put ADC read command in Tx buffer
//  Wire.endTransmission(I2C_NOSTOP);        // Send the Tx buffer, but send a restart to keep connection alive
//  uint8_t i = 0;
//  Wire.requestFrom(MS5637_ADDRESS, 3);     // Read three bytes from slave PROM address
//  while (Wire.available()) {
//    data[i++] = Wire.read();
//  }               // Put read results in the Rx buffer
//  return (uint32_t) (((uint32_t) data[0] << 16) | (uint32_t) data[1] << 8 | data[2]); // construct PROM data for return to main program
//}



//unsigned char MS5637checkCRC(uint16_t * n_prom)  // calculate checksum from PROM register contents
//{
//  int cnt;
//  unsigned int n_rem = 0;
//  unsigned char n_bit;
//
//  n_prom[0] = ((n_prom[0]) & 0x0FFF);  // replace CRC byte by 0 for checksum calculation
//  n_prom[7] = 0;
//  for (cnt = 0; cnt < 16; cnt++)
//  {
//    if (cnt % 2 == 1) n_rem ^= (unsigned short) ((n_prom[cnt >> 1]) & 0x00FF);
//    else         n_rem ^= (unsigned short)  (n_prom[cnt >> 1] >> 8);
//    for (n_bit = 8; n_bit > 0; n_bit--)
//    {
//      if (n_rem & 0x8000)    n_rem = (n_rem << 1) ^ 0x3000;
//      else                  n_rem = (n_rem << 1);
//    }
//  }
//  n_rem = ((n_rem >> 12) & 0x000F);
//  return (n_rem ^ 0x00);
//}

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
