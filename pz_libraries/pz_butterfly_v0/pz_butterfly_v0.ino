/*
  Infinite Uptime BLE Module Firmware Updated 24-12-2016 v33

  TIME DOMAIN FEATURES
  NOTE: Intervals are checked based on AUDIO, not ACCEL. Read vibration energy
  code to understand the sampling mechanism.

  FREQUENCY DOMAIN FEATURES
  NOTE: Most of these variables are for accessing calculation results.
  Read any of frequency domain feature calculation flow to understand.

  TIMESTAMP HANDLING
  Timestamp are handled by the BLE componant. Current time can be sent from Bluetooth
  Hub to the board (accessible via iuBLE.getHubDatetime()). Current datetime (accessible 
  via iuBLE.getDatetime)is then approximated using hub datetime and millis function.

*/

// TIME DOMAIN FEATURES

// Signal Energy ACCEL, index 0
// Simple loops  : 4200 microseconds
// All CMSIS-DSP : 2840 microseconds
// Partial       : 2730 microseconds (current)

/* Register addresses and hamming window presets  */
#include "constants.h"

/*Load functionnalities*/
#include "IURGBLed.h"
#include "IUBLE.h"
#include "IUI2C.h"
#include "IUBattery.h"
#include "IUBMX055.h"
#include "IUBMP280.h"
#include "IUI2S.h"

//====================== Instanciate device classes ========================
//Always initialize the computer bus first
IUI2C       iuI2C; 
IUBLE       iuBLE(iuI2C);
IUBattery   iuBat;
IURGBLed    iuRGBLed(iuI2C, iuBLE);
IUBMX055    iuBMX055(iuI2C, iuBLE);
IUBMP280    iuBMP280(iuI2C, iuBLE);
IUI2S       iuI2S(iuI2C, iuBLE);


//====================== Module Configuration Variables ========================
String MAC_ADDRESS = "88:4A:EA:69:DF:8C";

// Reduce RUN frequency if needed.
const uint32_t RESTING_INTERVAL = 0;  // Inter-batch gap
//const uint16_t RUN_ACCEL_AUDIO_RATIO = AUDIO_FREQ / ACCEL_FREQ;

const uint8_t NUM_FEATURES = 6;    // Total number of features
const uint8_t NUM_TD_FEATURES = 2; // Total number of frequency domain features
const byte audioFDFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B00000100}; // Audio feature enabling variable
const byte accelFDFeatures[(NUM_FEATURES - 1) / 8 + 1] = {B00111000}; // Accel feature enabling variable


//============================ Internal Variables ==============================


OpMode currMode = RUN;            // Mode of operation
OpState currState = NOT_CUTTING;  // Current state on feature calculation

//Regular data transmit variables
boolean dataSendTime = false;

//Sleep mode variables
int blue = 0;
boolean sleepmode = false;
int bluemillisnow = 0;
int bluesleeplimit = 60000;
int bluetimerstart = 0;

bool recordmode = false;



//====================== Feature Computation Variables =========================

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

uint8_t highest_danger_level = 0;   // Highest danger level indication for setting up the color indication
uint8_t current_danger_level = 0;   // Current danger level of the feature
float feature_value[NUM_FEATURES];  // Feature value array to besent over cloud
/*
 * Features currently are:
 * - Acceleration energy
 * - velocity on X axis
 * - velocity on Y axis
 * - velocity on Z axis
 * - current temperature
 * - audio DB
 */

//Feature selection parameters
int feature0check = 1;
int feature1check = 0;
int feature2check = 0;
int feature3check = 0;
int feature4check = 0;
int feature5check = 0;


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


//==============================================================================
//================================= Main Code ==================================
//==============================================================================

/* ------------------------------------ Init all counters ---------------------------------------- */

uint32_t featureBatchSize = iuI2S.MAX_INTERVAL; // Batch size for a feature -  Can be replaced with MAX_INTERVAL_AUDIO

uint32_t audioSamplingCount = 0;    // Audio sample index for recording data
uint32_t accelSamplingCount = 0;    // Accel sample index for recording data
uint32_t restCount = 0;             // Resting interval - Can be removed

// NOTE: Do NOT change the following variables unless you know what you're doing
uint16_t ACCEL_COUNTER_TARGET = iuI2S.getTargetSample() / iuBMX055.getTargetSample();  // Changing ratio for data collection mode
uint32_t subsample_counter = 0;
uint32_t accel_counter = 0;



/* ------------------------------------ begin ---------------------------------------- */

void setup()
{
  // Setup I2C: Always init computer bus first
  iuI2C.activate(400000, 115200); // Set I2C frequency at 400 kHz and Baud rate at 115200
  iuBLE.activate(); // Initialize BLE port
  iuBat.activate(); // Battery
  iuRGBLed.activate();
  
  iuI2C.port->println( "Initializing." );

  /* SENtral EM7180 is interfacing MPU9250 with the main CPU.
   It needs to be set to Pass-Through mode so that the MPU9250 can
   be accessed directly.*/
  iuI2C.scanDevices(); // should detect SENtral at 0x28
  iuBMX055.setSENtralToPassThroughMode(); // Put EM7180 SENtral into pass-through mode
  iuI2C.scanDevices(); // should see all the devices on the I2C bus including two from the EEPROM (ID page and data pages)
  iuI2C.resetErrorMessage(); //ZAB: Do not change this flow as error_message need to be set here

  // Wake up and configure each sensor
  iuBMX055.wakeUp(); // Accelerometer
  iuBMP280.wakeUp(); // Pressure and Temperature sensor
  iuI2S.wakeUp(); // Audio  
  
  iuBat.updateStatus();  // Determine the battery status

  iuI2S.begin(i2s_rx_callback);

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
  iuBat.updateStatus(); // Battery voltage
  //Timer to send data regularly//
  if (iuBLE.isDataSendTime())
  {
    dataSendTime = true;
  }

  //Robustness for data reception//
  iuBLE.autocleanBuffer();

  /* -------------------------- USB Connection Check ----------------------- */
  if (bitRead(USB0_OTGSTAT, 5)) {
    //      // Disconnected; this flag will briefly come up when USB gets disconnected.
    //      // Immediately put into RUN mode.
    if (currMode != RUN) { //If disconnected and NOT in RUN mode, set to run mode with idle state initialized
      iuBLE.port->begin(9600);
      iuBLE.port->flush();
      resetSampling(true);
      currMode = RUN;
      iuRGBLed.changeColor(iuRGBLed.BLUE_NOOP);
      currState = NOT_CUTTING;
    }
  }
  /*Serial.print("USB connection check complete. Starting BLE\n");*/
  /* ----------- Two-way Communication via BLE, only on RUN mode. ----------- */
  if (currMode == RUN)
  {
    // Read the BLE into the buffer and process instructions
    iuBLE.readToBuffer(processInstructions);
  }
  
  /* ------------- Hardwire Serial for DATA_COLLECTION commands ------------- */
  iuI2C.updateBuffer();
  
  if (iuI2C.getBuffer().length() > 0)
  {
    if (currMode == RUN)
    {
      if(iuI2C.checkIfStartCollection()) // if data collection request, change the mode
      {
        resetSampling(false);
        currMode = DATA_COLLECTION;
        iuRGBLed.changeColor(iuRGBLed.CYAN_DATA);
      }
    }
    else if (currMode == DATA_COLLECTION)
    {
      iuI2C.printBuffer();
      
      iuRGBLed.updateFromI2C(); // Check for LED color update
      iuBMX055.updateAccelRangeFromI2C(); // check for accel range update
      bool accUpdate = iuBMX055.updateSamplingRateFromI2C(); // check for accel sampling rate update
      bool audioUpdate = iuI2S.updateSamplingRateFromI2C(); // check for accel sampling rate update
      
      if (accUpdate || audioUpdate) //Accelerometer or Audio sampling rate changed
      {
        ACCEL_COUNTER_TARGET = iuI2S.getTargetSample() / iuBMX055.getTargetSample();
        accel_counter = 0;
        subsample_counter = 0;
      }

      if (iuI2C.checkIfEndCollection()) //if data collection finished, change the mode
      {
        currMode = RUN;
        iuRGBLed.changeColor(iuRGBLed.BLUE_NOOP);
        iuBMX055.setScale(iuBMX055.AFS_2G);
        iuBMX055.resetTargetSample();
        iuI2S.resetTargetSample();

        //Sampling and Counter reset
        ACCEL_COUNTER_TARGET = iuI2S.getTargetSample() / iuBMX055.getTargetSample();
        accel_counter = 0;
        subsample_counter = 0;
      }
    } // end of currMode == DATA_COLLECTION
    iuI2C.resetBuffer();;    // Clear wire buffer
  }
  // After handling all state / mode changes, check if features need to be computed.
  if (currMode == RUN && compute_feature_now[buffer_compute_index] && iuI2C.getErrorMessage().equals("ALL_OK"))
  {
    compute_features();
  }
}

//===================================================================================================================
//====== Utility functions
//===================================================================================================================


/**
 *--- Direct I2S Receive, we get callback to read 2 words from the FIFO --- 
 * We use the I2S callback to do both audio and acceleration data collection. The audio device (I2S)
 * is sort of giving the drumbeat for both audio and acceleration sampling. Note that acceleration can 
 * be downsampled.
 */
void i2s_rx_callback( int32_t *pBuf )
{
  if (iuI2C.getErrorMessage().equals("ALL_OK"))
  {
    if (currMode == CHARGING) { // Don't do anything if CHARGING
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
    if (subsample_counter != iuI2S.getDownsampleMax() - 2)
    {
      subsample_counter ++;
      return;
    } else {
      subsample_counter = 0;
      accel_counter++;
    }

    if (currMode == RUN) {
      if (restCount < RESTING_INTERVAL) {
        restCount ++;
      }
      else {
        // perform the data extraction and put data in batch for later FFT processing
        //Audio data for single channel mic
        iuI2S.extractDataInPlace(&pBuf[0]);
        iuI2S.pushDataToBatch(buffer_record_index, audioSamplingCount, pBuf);
        audioSamplingCount ++;
        // Accel data
        if (accel_counter == ACCEL_COUNTER_TARGET) {
          iuBMX055.readAccelData();
          if (iuI2C.getReadFlag())
          {
            iuBMX055.pushDataToBatch(buffer_record_index, accelSamplingCount);
            accelSamplingCount ++;
          }
          accel_counter = 0;
        }
      }
    } else if (currMode == DATA_COLLECTION) {
      //Extract and send data, audio first then accel if needed
      iuI2S.extractDataInPlace(&pBuf[0]); // Single channel mic
      iuI2S.dumpDataThroughI2C(pBuf);
      if (accel_counter == ACCEL_COUNTER_TARGET) {
        iuBMX055.readAccelData();
        iuBMX055.dumpDataThroughI2C();
        accel_counter = 0;
      }
    }
  }
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

/**
 * Process the instructions sent over BlueTooth to adjust the configuration
 * Concretely, scan the content of blebuffer, a char array (resulting from readToBuffer)
 */
void processInstructions(char *blebuffer)
{
  //iuI2C.port->println(blebuffer);
  switch(blebuffer[0])
  {
    case 0: // Set Thresholds
      if (blebuffer[4] == '-' && blebuffer[9] == '-' && blebuffer[14] == '-')
      {
        int bleFeatureIndex = 0, newThres = 0, newThres2 = 0, newThres3 = 0;
        sscanf(blebuffer, "%d-%d-%d-%d", &bleFeatureIndex, &newThres, &newThres2, &newThres3);
        if (bleFeatureIndex == 1 || bleFeatureIndex == 2 || bleFeatureIndex == 3) {
          featureNormalThreshold[bleFeatureIndex] = (float)newThres / 100.;
          featureWarningThreshold[bleFeatureIndex] = (float)newThres2 / 100.;
          featureDangerThreshold[bleFeatureIndex] = (float)newThres3 / 100.;
        } else {
          featureNormalThreshold[bleFeatureIndex] = (float)newThres;
          featureWarningThreshold[bleFeatureIndex] = (float)newThres2;
          featureDangerThreshold[bleFeatureIndex] = (float)newThres3;
        }
      }
      break;
     
    case 1: // Receive the timestamp data from the hub
      iuBLE.updateHubDatetime();
      break;
     
    case 2: // Bluetooth parameter setting
      iuBLE.updateDataSendingReceptionTime(&bluesleeplimit);
      break;
     
    case 3: // Record button pressed - go into record mode to record FFTs
      if (blebuffer[7] == '0' && blebuffer[9] == '0' && blebuffer[11] == '0' && blebuffer[13] == '0' && blebuffer[15] == '0' && blebuffer[17] == '0') {
        iuI2C.port->print("Time to record data and send FFTs");
        recordmode = true;
        iuBMX055.showRecordFFT(buffer_compute_index, MAC_ADDRESS);
        recordmode = false;
      }
      break;
     
    case 4: // Stop button pressed - go out of record mode back into RUN mode
      if (blebuffer[7] == '0' && blebuffer[9] == '0' && blebuffer[11] == '0' && blebuffer[13] == '0' && blebuffer[15] == '0' && blebuffer[17] == '0') {
        iuI2C.port->print("Stop recording and sending FFTs");
      }
      break;
     
    case 5:
      if (blebuffer[7] == '0' && blebuffer[9] == '0' && blebuffer[11] == '0' && blebuffer[13] == '0' && blebuffer[15] == '0' && blebuffer[17] == '0')
      {
        iuBLE.port->print("HB,");
        iuBLE.port->print(MAC_ADDRESS);
        iuBLE.port->print(",");
        iuBLE.port->print(iuI2C.getErrorMessage());
        iuBLE.port->print(";");
        iuBLE.port->flush();
      }
      break;
     
    case 6:
      if (blebuffer[7] == ':' && blebuffer[9] == '.' && blebuffer[11] == '.' && blebuffer[13] == '.' && blebuffer[15] == '.' && blebuffer[17] == '.')
      {
        int parametertag = 0;
        sscanf(blebuffer, "%d:%d.%d.%d.%d.%d.%d", &parametertag, &feature0check, &feature1check, &feature2check, &feature3check, &feature4check, &feature5check);
      }
      break;
  }
}

/**
 * Reset the data collection routine
 * NB: Need to call this before entering RUN and DATA_COLLECTION modes
 */
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
    iuI2S.resetTargetSample();
  } else {
    iuI2S.resetTargetSample();
  }

  ACCEL_COUNTER_TARGET = iuI2S.getTargetSample() / iuBMX055.getTargetSample();
}

//======================= Feature Computation Caller ============================


// Function to compute all the feature values to be stored on cloud
void compute_features() {
  // Turn the boolean off immediately.
  compute_feature_now[buffer_compute_index] = false;
  record_feature_now[buffer_compute_index] = false;
  highest_danger_level = 0;

  feature_value[0] = iuBMX055.computeEnergy(buffer_compute_index, featureBatchSize, ACCEL_COUNTER_TARGET);
  if (feature0check == 1) {
    current_danger_level = threshold_feature(0, feature_value[0]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  iuBMX055.computeAccelRFFT(buffer_compute_index, iuBMX055.X, hamming_window_512);
  iuBMX055.computeAccelRFFT(buffer_compute_index, iuBMX055.Y, hamming_window_512);
  iuBMX055.computeAccelRFFT(buffer_compute_index, iuBMX055.Z, hamming_window_512);

  feature_value[1] = iuBMX055.computeVelocity(buffer_compute_index, iuBMX055.X, featureNormalThreshold[1]);
  
  if (feature1check == 1) {
    current_danger_level = threshold_feature(1, feature_value[1]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  feature_value[2] = iuBMX055.computeVelocity(buffer_compute_index, iuBMX055.Y, featureNormalThreshold[2]);
  if (feature2check == 1) {
    current_danger_level = threshold_feature(2, feature_value[2]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  feature_value[3] = iuBMX055.computeVelocity(buffer_compute_index, iuBMX055.Z, featureNormalThreshold[3]);
  if (feature3check == 1) {
    current_danger_level = threshold_feature(3, feature_value[3]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  feature_value[4] = iuBMP280.getCurrentTemperature();
  if (feature4check == 1) {
    current_danger_level = threshold_feature(4, feature_value[4]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  feature_value[5] = iuI2S.getAudioDB(buffer_compute_index);
  if (feature5check == 1) {
    current_danger_level = threshold_feature(5, feature_value[5]);
    highest_danger_level = max(highest_danger_level, current_danger_level);
  }

  
  // Reflect highest danger level if differs from previous state.
  if (!recordmode) {
    double datetime = iuBLE.getDatetime();
    switch (highest_danger_level) {
      case 0:
        if (sleepmode) {
          // Do nothing
        }
        else {
          if (currState != NOT_CUTTING || dataSendTime) {
            currState = NOT_CUTTING;
            iuBLE.port->print(MAC_ADDRESS);
            iuBLE.port->print(",00,");
            iuBLE.port->print(iuBat.getStatus());
            for (int i = 0; i < chosen_features; i++) {
              iuBLE.port->print(",");
              iuBLE.port->print("000");
              iuBLE.port->print(FeatureID[i] + 1);
              iuBLE.port->print(",");
              iuBLE.port->print(feature_value[i]);
            }
            iuBLE.port->print(",");
            iuBLE.port->print(datetime);
            iuBLE.port->print(";");
            iuBLE.port->flush();
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
          iuRGBLed.changeColor(iuRGBLed.SLEEP_MODE);
        }
        else {
          iuRGBLed.changeColor(iuRGBLed.BLUE_NOOP);
        }
        dataSendTime = false;
        break;
      case 1:
        if (currState != NORMAL_CUTTING || dataSendTime) {
          currState = NORMAL_CUTTING;
          iuBLE.port->print(MAC_ADDRESS);
          iuBLE.port->print(",01,");
          iuBLE.port->print(iuBat.getStatus());
          for (int i = 0; i < chosen_features; i++) {
            iuBLE.port->print(",");
            iuBLE.port->print("000");
            iuBLE.port->print(FeatureID[i] + 1);
            iuBLE.port->print(",");
            iuBLE.port->print(feature_value[i]);
          }
          iuBLE.port->print(",");
          iuBLE.port->print(datetime);
          iuBLE.port->print(";");
          iuBLE.port->flush();
        }
        iuRGBLed.changeColor(iuRGBLed.GREEN_OK);
        dataSendTime = false;
        blue = 0;
        sleepmode = false;
        break;
      case 2:
        if (currState != WARNING_CUTTING || dataSendTime) {
          currState = WARNING_CUTTING;
          iuBLE.port->print(MAC_ADDRESS);
          iuBLE.port->print(",02,");
          iuBLE.port->print(iuBat.getStatus());
          for (int i = 0; i < chosen_features; i++) {
            iuBLE.port->print(",");
            iuBLE.port->print("000");
            iuBLE.port->print(FeatureID[i] + 1);
            iuBLE.port->print(",");
            iuBLE.port->print(feature_value[i]);
          }
          iuBLE.port->print(",");
          iuBLE.port->print(datetime);
          iuBLE.port->print(";");
          iuBLE.port->flush();
        }
        iuRGBLed.changeColor(iuRGBLed.ORANGE_WARNING);
        dataSendTime = false;
        blue = 0;
        sleepmode = false;
        break;
      case 3:
        if (currState != BAD_CUTTING || dataSendTime) {
          currState = BAD_CUTTING;
          iuBLE.port->print(MAC_ADDRESS);
          iuBLE.port->print(",03,");
          iuBLE.port->print(iuBat.getStatus());
          for (int i = 0; i < chosen_features; i++) {
            iuBLE.port->print(",");
            iuBLE.port->print("000");
            iuBLE.port->print(FeatureID[i] + 1);
            iuBLE.port->print(",");
            iuBLE.port->print(feature_value[i]);
          }
          iuBLE.port->print(",");
          iuBLE.port->print(datetime);
          iuBLE.port->print(";");
          iuBLE.port->flush();
        }
        iuRGBLed.changeColor(iuRGBLed.RED_BAD);
        dataSendTime = false;
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


