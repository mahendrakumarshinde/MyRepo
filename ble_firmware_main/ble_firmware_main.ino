/*

Infinite Uptime BLE Module Firmware

*/

//#include "Wire.h"
#include <i2c_t3.h>
#include <SPI.h>

/* I2S digital audio */
#include <i2s.h>

// See MS5637-02BA03 Low Voltage Barometric Pressure Sensor Data Sheet http://www.meas-spec.com/downloads/MS5637-02BA03.pdf
#define MS5637_RESET      0x1E
#define MS5637_CONVERT_D1 0x40
#define MS5637_CONVERT_D2 0x50
#define MS5637_ADC_READ   0x00

// BMX055 data sheet http://ae-bst.resource.bosch.com/media/products/dokumente/bmx055/BST-BMX055-DS000-01v2.pdf
// The BMX055 is a conglomeration of three separate motion sensors packaged together but
// addressed and communicated with separately by design
// Accelerometer registers
#define BMX055_ACC_WHOAMI        0x00   // should return 0xFA
//#define BMX055_ACC_Reserved    0x01
#define BMX055_ACC_D_X_LSB       0x02
#define BMX055_ACC_D_X_MSB       0x03
#define BMX055_ACC_D_Y_LSB       0x04
#define BMX055_ACC_D_Y_MSB       0x05
#define BMX055_ACC_D_Z_LSB       0x06
#define BMX055_ACC_D_Z_MSB       0x07
#define BMX055_ACC_D_TEMP        0x08
#define BMX055_ACC_INT_STATUS_0  0x09
#define BMX055_ACC_INT_STATUS_1  0x0A
#define BMX055_ACC_INT_STATUS_2  0x0B
#define BMX055_ACC_INT_STATUS_3  0x0C
//#define BMX055_ACC_Reserved    0x0D
#define BMX055_ACC_FIFO_STATUS   0x0E
#define BMX055_ACC_PMU_RANGE     0x0F
#define BMX055_ACC_PMU_BW        0x10
#define BMX055_ACC_PMU_LPW       0x11
#define BMX055_ACC_PMU_LOW_POWER 0x12
#define BMX055_ACC_D_HBW         0x13
#define BMX055_ACC_BGW_SOFTRESET 0x14
//#define BMX055_ACC_Reserved    0x15
#define BMX055_ACC_INT_EN_0      0x16
#define BMX055_ACC_INT_EN_1      0x17
#define BMX055_ACC_INT_EN_2      0x18
#define BMX055_ACC_INT_MAP_0     0x19
#define BMX055_ACC_INT_MAP_1     0x1A
#define BMX055_ACC_INT_MAP_2     0x1B
//#define BMX055_ACC_Reserved    0x1C
//#define BMX055_ACC_Reserved    0x1D
#define BMX055_ACC_INT_SRC       0x1E
//#define BMX055_ACC_Reserved    0x1F
#define BMX055_ACC_INT_OUT_CTRL  0x20
#define BMX055_ACC_INT_RST_LATCH 0x21
#define BMX055_ACC_INT_0         0x22
#define BMX055_ACC_INT_1         0x23
#define BMX055_ACC_INT_2         0x24
#define BMX055_ACC_INT_3         0x25
#define BMX055_ACC_INT_4         0x26
#define BMX055_ACC_INT_5         0x27
#define BMX055_ACC_INT_6         0x28
#define BMX055_ACC_INT_7         0x29
#define BMX055_ACC_INT_8         0x2A
#define BMX055_ACC_INT_9         0x2B
#define BMX055_ACC_INT_A         0x2C
#define BMX055_ACC_INT_B         0x2D
#define BMX055_ACC_INT_C         0x2E
#define BMX055_ACC_INT_D         0x2F
#define BMX055_ACC_FIFO_CONFIG_0 0x30
//#define BMX055_ACC_Reserved    0x31
#define BMX055_ACC_PMU_SELF_TEST 0x32
#define BMX055_ACC_TRIM_NVM_CTRL 0x33
#define BMX055_ACC_BGW_SPI3_WDT  0x34
//#define BMX055_ACC_Reserved    0x35
#define BMX055_ACC_OFC_CTRL      0x36
#define BMX055_ACC_OFC_SETTING   0x37
#define BMX055_ACC_OFC_OFFSET_X  0x38
#define BMX055_ACC_OFC_OFFSET_Y  0x39
#define BMX055_ACC_OFC_OFFSET_Z  0x3A
#define BMX055_ACC_TRIM_GPO      0x3B
#define BMX055_ACC_TRIM_GP1      0x3C
//#define BMX055_ACC_Reserved    0x3D
#define BMX055_ACC_FIFO_CONFIG_1 0x3E
#define BMX055_ACC_FIFO_DATA     0x3F

// BMX055 Gyroscope Registers
#define BMX055_GYRO_WHOAMI           0x00  // should return 0x0F
//#define BMX055_GYRO_Reserved       0x01
#define BMX055_GYRO_RATE_X_LSB       0x02
#define BMX055_GYRO_RATE_X_MSB       0x03
#define BMX055_GYRO_RATE_Y_LSB       0x04
#define BMX055_GYRO_RATE_Y_MSB       0x05
#define BMX055_GYRO_RATE_Z_LSB       0x06
#define BMX055_GYRO_RATE_Z_MSB       0x07
//#define BMX055_GYRO_Reserved       0x08
#define BMX055_GYRO_INT_STATUS_0  0x09
#define BMX055_GYRO_INT_STATUS_1  0x0A
#define BMX055_GYRO_INT_STATUS_2  0x0B
#define BMX055_GYRO_INT_STATUS_3  0x0C
//#define BMX055_GYRO_Reserved    0x0D
#define BMX055_GYRO_FIFO_STATUS   0x0E
#define BMX055_GYRO_RANGE         0x0F
#define BMX055_GYRO_BW            0x10
#define BMX055_GYRO_LPM1          0x11
#define BMX055_GYRO_LPM2          0x12
#define BMX055_GYRO_RATE_HBW      0x13
#define BMX055_GYRO_BGW_SOFTRESET 0x14
#define BMX055_GYRO_INT_EN_0      0x15
#define BMX055_GYRO_INT_EN_1      0x16
#define BMX055_GYRO_INT_MAP_0     0x17
#define BMX055_GYRO_INT_MAP_1     0x18
#define BMX055_GYRO_INT_MAP_2     0x19
#define BMX055_GYRO_INT_SRC_1     0x1A
#define BMX055_GYRO_INT_SRC_2     0x1B
#define BMX055_GYRO_INT_SRC_3     0x1C
//#define BMX055_GYRO_Reserved    0x1D
#define BMX055_GYRO_FIFO_EN       0x1E
//#define BMX055_GYRO_Reserved    0x1F
//#define BMX055_GYRO_Reserved    0x20
#define BMX055_GYRO_INT_RST_LATCH 0x21
#define BMX055_GYRO_HIGH_TH_X     0x22
#define BMX055_GYRO_HIGH_DUR_X    0x23
#define BMX055_GYRO_HIGH_TH_Y     0x24
#define BMX055_GYRO_HIGH_DUR_Y    0x25
#define BMX055_GYRO_HIGH_TH_Z     0x26
#define BMX055_GYRO_HIGH_DUR_Z    0x27
//#define BMX055_GYRO_Reserved    0x28
//#define BMX055_GYRO_Reserved    0x29
//#define BMX055_GYRO_Reserved    0x2A
#define BMX055_GYRO_SOC           0x31
#define BMX055_GYRO_A_FOC         0x32
#define BMX055_GYRO_TRIM_NVM_CTRL 0x33
#define BMX055_GYRO_BGW_SPI3_WDT  0x34
//#define BMX055_GYRO_Reserved    0x35
#define BMX055_GYRO_OFC1          0x36
#define BMX055_GYRO_OFC2          0x37
#define BMX055_GYRO_OFC3          0x38
#define BMX055_GYRO_OFC4          0x39
#define BMX055_GYRO_TRIM_GP0      0x3A
#define BMX055_GYRO_TRIM_GP1      0x3B
#define BMX055_GYRO_BIST          0x3C
#define BMX055_GYRO_FIFO_CONFIG_0 0x3D
#define BMX055_GYRO_FIFO_CONFIG_1 0x3E

// BMX055 magnetometer registers
#define BMX055_MAG_WHOAMI         0x40  // should return 0x32
#define BMX055_MAG_Reserved       0x41
#define BMX055_MAG_XOUT_LSB       0x42
#define BMX055_MAG_XOUT_MSB       0x43
#define BMX055_MAG_YOUT_LSB       0x44
#define BMX055_MAG_YOUT_MSB       0x45
#define BMX055_MAG_ZOUT_LSB       0x46
#define BMX055_MAG_ZOUT_MSB       0x47
#define BMX055_MAG_ROUT_LSB       0x48
#define BMX055_MAG_ROUT_MSB       0x49
#define BMX055_MAG_INT_STATUS     0x4A
#define BMX055_MAG_PWR_CNTL1      0x4B
#define BMX055_MAG_PWR_CNTL2      0x4C
#define BMX055_MAG_INT_EN_1       0x4D
#define BMX055_MAG_INT_EN_2       0x4E
#define BMX055_MAG_LOW_THS        0x4F
#define BMX055_MAG_HIGH_THS       0x50
#define BMX055_MAG_REP_XY         0x51
#define BMX055_MAG_REP_Z          0x52
/* Trim Extended Registers */
#define BMM050_DIG_X1             0x5D // needed for magnetic field calculation
#define BMM050_DIG_Y1             0x5E
#define BMM050_DIG_Z4_LSB         0x62
#define BMM050_DIG_Z4_MSB         0x63
#define BMM050_DIG_X2             0x64
#define BMM050_DIG_Y2             0x65
#define BMM050_DIG_Z2_LSB         0x68
#define BMM050_DIG_Z2_MSB         0x69
#define BMM050_DIG_Z1_LSB         0x6A
#define BMM050_DIG_Z1_MSB         0x6B
#define BMM050_DIG_XYZ1_LSB       0x6C
#define BMM050_DIG_XYZ1_MSB       0x6D
#define BMM050_DIG_Z3_LSB         0x6E
#define BMM050_DIG_Z3_MSB         0x6F
#define BMM050_DIG_XY2            0x70
#define BMM050_DIG_XY1            0x71

// EM7180 SENtral register map
// see http://www.emdeveloper.com/downloads/7180/EMSentral_EM7180_Register_Map_v1_3.pdf
//
#define EM7180_QX                 0x00  // this is a 32-bit normalized floating point number read from registers 0x00-03
#define EM7180_QY                 0x04  // this is a 32-bit normalized floating point number read from registers 0x04-07
#define EM7180_QZ                 0x08  // this is a 32-bit normalized floating point number read from registers 0x08-0B
#define EM7180_QW                 0x0C  // this is a 32-bit normalized floating point number read from registers 0x0C-0F
#define EM7180_QTIME              0x10  // this is a 16-bit unsigned integer read from registers 0x10-11
#define EM7180_MX                 0x12  // int16_t from registers 0x12-13
#define EM7180_MY                 0x14  // int16_t from registers 0x14-15
#define EM7180_MZ                 0x16  // int16_t from registers 0x16-17
#define EM7180_MTIME              0x18  // uint16_t from registers 0x18-19
#define EM7180_AX                 0x1A  // int16_t from registers 0x1A-1B
#define EM7180_AY                 0x1C  // int16_t from registers 0x1C-1D
#define EM7180_AZ                 0x1E  // int16_t from registers 0x1E-1F
#define EM7180_ATIME              0x20  // uint16_t from registers 0x20-21
#define EM7180_GX                 0x22  // int16_t from registers 0x22-23
#define EM7180_GY                 0x24  // int16_t from registers 0x24-25
#define EM7180_GZ                 0x26  // int16_t from registers 0x26-27
#define EM7180_GTIME              0x28  // uint16_t from registers 0x28-29
#define EM7180_QRateDivisor       0x32  // uint8_t
#define EM7180_EnableEvents       0x33
#define EM7180_HostControl        0x34
#define EM7180_EventStatus        0x35
#define EM7180_SensorStatus       0x36
#define EM7180_SentralStatus      0x37
#define EM7180_AlgorithmStatus    0x38
#define EM7180_FeatureFlags       0x39
#define EM7180_ParamAcknowledge   0x3A
#define EM7180_SavedParamByte0    0x3B
#define EM7180_SavedParamByte1    0x3C
#define EM7180_SavedParamByte2    0x3D
#define EM7180_SavedParamByte3    0x3E
#define EM7180_ActualMagRate      0x45
#define EM7180_ActualAccelRate    0x46
#define EM7180_ActualGyroRate     0x47
#define EM7180_ErrorRegister      0x50
#define EM7180_AlgorithmControl   0x54
#define EM7180_MagRate            0x55
#define EM7180_AccelRate          0x56
#define EM7180_GyroRate           0x57
#define EM7180_LoadParamByte0     0x60
#define EM7180_LoadParamByte1     0x61
#define EM7180_LoadParamByte2     0x62
#define EM7180_LoadParamByte3     0x63
#define EM7180_ParamRequest       0x64
#define EM7180_ROMVersion1        0x70
#define EM7180_ROMVersion2        0x71
#define EM7180_RAMVersion1        0x72
#define EM7180_RAMVersion2        0x73
#define EM7180_ProductID          0x90
#define EM7180_RevisionID         0x91
#define EM7180_RunStatus          0x92
#define EM7180_UploadAddress      0x94 // uint16_t registers 0x94 (MSB)-5(LSB)
#define EM7180_UploadData         0x96
#define EM7180_CRCHost            0x97  // uint32_t from registers 0x97-9A
#define EM7180_ResetRequest       0x9B
#define EM7180_PassThruStatus     0x9E
#define EM7180_PassThruControl    0xA0

// Using the Teensy Mini Add-On board, BMX055 SDO1 = SDO2 = CSB3 = GND as designed
// Seven-bit BMX055 device addresses are ACC = 0x18, GYRO = 0x68, MAG = 0x10
#define BMX055_ACC_ADDRESS  0x18   // Address of BMX055 accelerometer
#define BMX055_GYRO_ADDRESS 0x68   // Address of BMX055 gyroscope
#define BMX055_MAG_ADDRESS  0x10   // Address of BMX055 magnetometer
#define MS5637_ADDRESS      0x76   // Address of MS5637 altimeter
#define EM7180_ADDRESS      0x28   // Address of the EM7180 SENtral sensor hub
#define M24512DFM_DATA_ADDRESS   0x50   // Address of the 500 page M24512DFM EEPROM data buffer, 1024 bits (128 8-bit bytes) per page
#define M24512DFM_IDPAGE_ADDRESS 0x58   // Address of the single M24512DFM lockable EEPROM ID page

#define SerialDebug false  // set to true to get Serial output for debugging

// Set initial input parameters
// define X055 ACC full scale options
#define AFS_2G  0x03
#define AFS_4G  0x05
#define AFS_8G  0x08
#define AFS_16G 0x0C

enum ACCBW {    // define BMX055 accelerometer bandwidths
  ABW_8Hz,      // 7.81 Hz,  64 ms update time
  ABW_16Hz,     // 15.63 Hz, 32 ms update time
  ABW_31Hz,     // 31.25 Hz, 16 ms update time
  ABW_63Hz,     // 62.5  Hz,  8 ms update time
  ABW_125Hz,    // 125   Hz,  4 ms update time
  ABW_250Hz,    // 250   Hz,  2 ms update time
  ABW_500Hz,    // 500   Hz,  1 ms update time
  ABW_1000Hz     // 1000  Hz,  0.5 ms update time
};

enum Gscale {
  GFS_2000DPS = 0,
  GFS_1000DPS,
  GFS_500DPS,
  GFS_250DPS,
  GFS_125DPS
};

enum GODRBW {
  G_2000Hz523Hz = 0, // 2000 Hz ODR and unfiltered (bandwidth 523Hz)
  G_2000Hz230Hz,
  G_1000Hz116Hz,
  G_400Hz47Hz,
  G_200Hz23Hz,
  G_100Hz12Hz,
  G_200Hz64Hz,
  G_100Hz32Hz  // 100 Hz ODR and 32 Hz bandwidth
};

enum MODR {
  MODR_10Hz = 0,   // 10 Hz ODR
  MODR_2Hz     ,   // 2 Hz ODR
  MODR_6Hz     ,   // 6 Hz ODR
  MODR_8Hz     ,   // 8 Hz ODR
  MODR_15Hz    ,   // 15 Hz ODR
  MODR_20Hz    ,   // 20 Hz ODR
  MODR_25Hz    ,   // 25 Hz ODR
  MODR_30Hz        // 30 Hz ODR
};

enum Mmode {
  lowPower         = 0,   // rms noise ~1.0 microTesla, 0.17 mA power
  Regular             ,   // rms noise ~0.6 microTesla, 0.5 mA power
  enhancedRegular     ,   // rms noise ~0.5 microTesla, 0.8 mA power
  highAccuracy            // rms noise ~0.3 microTesla, 4.9 mA power
};

// MS5637 pressure sensor sample rates
#define ADC_256  0x00 // define pressure and temperature conversion rates
#define ADC_512  0x02
#define ADC_1024 0x04
#define ADC_2048 0x06
#define ADC_4096 0x08
#define ADC_8192 0x0A
#define ADC_D1   0x40
#define ADC_D2   0x50

//==============================================================================
//====================== Module Configuration Variables ========================
//==============================================================================

#define CLOCK_TYPE         (I2S_CLOCK_48K_INTERNAL)     // I2S clock
bool statusLED = true;                                  // Status LED ON/OFF
const uint16_t TARGET_AUDIO_SAMPLE = 8000;              // Audio frequency
const uint16_t TARGET_ACCEL_SAMPLE = 1000;              // Accel frequency
const uint32_t RESTING_INTERVAL = 0;                    // Inter-batch gap

// NOTE: Do NOT change the following variables
const uint16_t ACCEL_COUNTER_TARGET = TARGET_AUDIO_SAMPLE/TARGET_ACCEL_SAMPLE;
const uint16_t DOWNSAMPLE_MAX = 48000/TARGET_AUDIO_SAMPLE;
uint32_t subsample_counter = 0;
uint32_t accel_counter = 0;

//==============================================================================
//====================== Feature Computation Variables =========================
//==============================================================================

// Energy Variables
const uint32_t ENERGY_INTERVAL_ACCEL = 100;
const uint32_t ENERGY_INTERVAL_AUDIO = ENERGY_INTERVAL_ACCEL * ACCEL_COUNTER_TARGET;
float ave_x = 0;
float ave_y = 0;
float ave_z = 0;
float ene = 0;

/*
  By default, only energy feature is turned on.

  0: Energy
*/
const uint8_t NUM_FEATURES = 1;

typedef float (* FeatureFuncPtr) (); // this is a typedef to feature functions
FeatureFuncPtr features[NUM_FEATURES] = {feature_energy};

const uint32_t MAX_INTERVAL = ENERGY_INTERVAL_AUDIO;
// This bitwise boolean is saved backwards.
// 0th feature is the least significant bit.
byte enabledFeatures[NUM_FEATURES/8 + 1] = {B00000001};
uint32_t featureIntervals[NUM_FEATURES]= {ENERGY_INTERVAL_AUDIO};
float featureWarningThreshold[NUM_FEATURES] = {100};
float featureDangerThreshold[NUM_FEATURES] = {1000};

bool compute_feature_now[2] = {false, false};
bool record_feature_now[2] = {true, true};
uint8_t buffer_compute_index = 0;
uint8_t buffer_record_index = 0;

uint32_t featureBatchSize = ENERGY_INTERVAL_AUDIO;
uint32_t audioSamplingCount = 0;
uint32_t accelSamplingCount = 0;
uint32_t restCount = 0;
uint8_t highest_danger_level = 0;
uint8_t current_danger_level = 0;
float feature_value = 0;

// Acceleration Buffers
float accel_x_batch[2][MAX_INTERVAL / ACCEL_COUNTER_TARGET];
float accel_y_batch[2][MAX_INTERVAL / ACCEL_COUNTER_TARGET];
float accel_z_batch[2][MAX_INTERVAL / ACCEL_COUNTER_TARGET];

// Audio Buffers
int32_t audio_batch[2][MAX_INTERVAL];

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

// TODO: This may not be necessary
int datai = 0;
long last_wakeup;
long this_wakeup;
long dt;

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
  NOT_CUTTING    = 0,
  NORMAL_CUTTING = 1,
  BAD_CUTTING    = 2
};

OpState currState = NOT_CUTTING;

// LED color code enum
enum LEDColors {
  RED_BAD         , // L H H
  BLUE_NOOP       , // H H L
  GREEN_OK        , // H L H
  ORANGE_CHARG    , // L L H
  PURPLE_ERR      , // L H L
  CYAN_DATA       , // H L L
  WHITE_NONE        // L L L
};

// Explicit function declaration for enum argument
void changeStatusLED(LEDColors color);

boolean silent = true;
unsigned char bytes[4];

// String for Serial communication
String bleBuffer = "";
String wireBuffer = "";
char characterRead;


/*==============================================================================
  ====================== Feature Computation Caller ============================
==============================================================================*/
void compute_features(){
  // Turn the boolean off immediately.
  compute_feature_now[buffer_compute_index] = false;
  record_feature_now[buffer_compute_index] = false;
  highest_danger_level = 0;

  for (int i=0; i < NUM_FEATURES; i++){
    // Compute feature only when corresponding bit is turned on.
    if ((enabledFeatures[i/8] >> (i%8)) & 1){
      feature_value = features[i]();
      current_danger_level = threshold_feature(i, feature_value);
      // TODO: Variable is separated for possible feature value output.
      //       AKA, when danger level is 2, output feature index, etc.
      highest_danger_level = max(highest_danger_level, current_danger_level);
    }
  }

  // Reflect highest danger level if differs from previous state.
  LEDColors c;
  switch (highest_danger_level){
    case 0:
      if (currState != NOT_CUTTING){
        currState = NOT_CUTTING;
        Serial2.print(0);
        Serial2.flush();
      }
      c = BLUE_NOOP;
      changeStatusLED(c);
      break;
    case 1:
      if (currState != NORMAL_CUTTING){
        currState = NORMAL_CUTTING;
        Serial2.print(1);
        Serial2.flush();
      }
      c = GREEN_OK;
      changeStatusLED(c);
      break;
    case 2:
      if (currState != BAD_CUTTING){
        currState = BAD_CUTTING;
        Serial2.print(2);
        Serial2.flush();
      }
      c = RED_BAD;
      changeStatusLED(c);
      break;
  }

  // Allow recording for old index buffer.
  record_feature_now[buffer_compute_index] = true;
  // Flip computation index AFTER all computations are done.
  buffer_compute_index = buffer_compute_index == 0 ? 1 : 0;
}

// Thresholding helper function to reduce code redundancy
uint8_t threshold_feature(int index, float featureVal){
  if(featureVal < featureWarningThreshold[index]){
    // level 0: not cutting
    return 0;
  }
  else if(featureVal < featureDangerThreshold[index]){
    // level 1: normal cutting
    return 1;
  }
  else{
    // level 2: bad cutting
    return 2;
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

// Energy, index 0
float feature_energy (){
  ave_x=0;
  ave_y=0;
  ave_z=0;
  for (int i=featureBatchSize/ACCEL_COUNTER_TARGET - ENERGY_INTERVAL_ACCEL; i<featureBatchSize/ACCEL_COUNTER_TARGET; i++){
    ave_x += accel_x_batch[buffer_compute_index][i];
    ave_y += accel_y_batch[buffer_compute_index][i];
    ave_z += accel_z_batch[buffer_compute_index][i];
  }
  ave_x /= ENERGY_INTERVAL_ACCEL;
  ave_y /= ENERGY_INTERVAL_ACCEL;
  ave_z /= ENERGY_INTERVAL_ACCEL;

  ene = 0; //initialize the energy feature
  for (int i=featureBatchSize/ACCEL_COUNTER_TARGET - ENERGY_INTERVAL_ACCEL; i<featureBatchSize/ACCEL_COUNTER_TARGET; i++){
    ene += sq(accel_x_batch[buffer_compute_index][i]-ave_x)
          +  sq(accel_y_batch[buffer_compute_index][i]-ave_y)
          +  sq(accel_z_batch[buffer_compute_index][i]-ave_z);
  }
  ene *= ENERGY_INTERVAL_ACCEL;

  return ene;
}

//==============================================================================
//================================= Main Code ==================================
//==============================================================================

// extract the 24bit INMP441 audio data from 32bit sample
void extractdata_inplace(int32_t  *pBuf) {
  // set highest bit to zero, then bitshift right 7 times
  // do not omit the first part (!)
  pBuf[0] = (pBuf[0] & 0x7fffffff) >>7;
}


/* --- Direct I2S Receive, we get callback to read 2 words from the FIFO --- */

void i2s_rx_callback( int32_t *pBuf )
{
  // Don't do anything if CHARGING
  if (currMode == CHARGING) {
    return;
  }

  if (currMode == RUN){
    // If feature computation is lagging behind, reset all sampling routine and
    // Don't run any data collection.
    if (audioSamplingCount == 0 && !record_feature_now[buffer_record_index]){
      return;
    }

    // Filled out the current buffer; switch buffer index
    if (audioSamplingCount == featureBatchSize){
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
  if (subsample_counter != DOWNSAMPLE_MAX-2){
    subsample_counter ++;
    return;
  } else{
    subsample_counter = 0;
    accel_counter++;
  }

  if (currMode == RUN){
    if (restCount < RESTING_INTERVAL){
      restCount ++;
    } else{
      // perform the data extraction for single channel mic
      extractdata_inplace(&pBuf[0]);

      // Save audio sample into buffer
      audio_batch[buffer_record_index][audioSamplingCount] = pBuf[0];
      audioSamplingCount ++;

      if (accel_counter == ACCEL_COUNTER_TARGET){
        readAccelData(accelCount);

        ax = (float)accelCount[0] * aRes + accelBias[0];
        ay = (float)accelCount[1] * aRes + accelBias[1];
        az = (float)accelCount[2] * aRes + accelBias[2];

        accel_x_batch[buffer_record_index][accelSamplingCount] = ax;
        accel_y_batch[buffer_record_index][accelSamplingCount] = ay;
        accel_z_batch[buffer_record_index][accelSamplingCount] = az;
        accelSamplingCount ++;

        accel_counter = 0;
      }
    }
  } else if (currMode == DATA_COLLECTION){
    // perform the data extraction for single channel mic
    extractdata_inplace(&pBuf[0]);

    // Dump sound data first
    Serial.write((pBuf[0] >> 16) & 0xFF);
    Serial.write((pBuf[0] >> 8) & 0xFF);
    Serial.write(pBuf[0] & 0xFF);

    if (accel_counter == ACCEL_COUNTER_TARGET){
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

/* --- Direct I2S Transmit, we get callback to put 2 words into the FIFO --- */

void i2s_tx_callback( int32_t *pBuf )
{
  // This function won't be used at all; leave it empty.
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

  // LED G=20, R=21, B=22
  if (statusLED){
    pinMode(20, OUTPUT);
    pinMode(21, OUTPUT);
    pinMode(22, OUTPUT);
  }

  // Read the BMX-055 WHO_AM_I registers, this is a good test of communication
  //Serial.println("BMX055 accelerometer...");
  byte c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_WHOAMI);  // Read ACC WHO_AM_I register for BMX055
  //Serial.print("BMX055 ACC"); Serial.print(" I AM 0x"); Serial.print(c, HEX); Serial.print(" I should be 0x"); Serial.println(0xFA, HEX);

  if (c == 0xFA) // WHO_AM_I should always be ACC = 0xFA, GYRO = 0x0F, MAG = 0x32
  {
    //Serial.println("BMX055 is online...");

    initBMX055();
    //Serial.println("BMX055 initialized for active data mode...."); // Initialize device for active mode read of acclerometer, gyroscope, and temperature


    // // Reset the MS5637 pressure sensor
    // MS5637Reset();
    // delay(100);
    // Serial.println("MS5637 pressure sensor reset...");
    // // Read PROM data from MS5637 pressure sensor
    // MS5637PromRead(Pcal);
    // Serial.println("PROM data read:");
    // Serial.print("C0 = "); Serial.println(Pcal[0]);
    // unsigned char refCRC = Pcal[0] >> 12;
    // Serial.print("C1 = "); Serial.println(Pcal[1]);
    // Serial.print("C2 = "); Serial.println(Pcal[2]);
    // Serial.print("C3 = "); Serial.println(Pcal[3]);
    // Serial.print("C4 = "); Serial.println(Pcal[4]);
    // Serial.print("C5 = "); Serial.println(Pcal[5]);
    // Serial.print("C6 = "); Serial.println(Pcal[6]);

    // nCRC = MS5637checkCRC(Pcal);  //calculate checksum to ensure integrity of MS5637 calibration data
    // Serial.print("Checksum = "); Serial.print(nCRC); Serial.print(" , should be "); Serial.println(refCRC);

    // delay(1000);

    // get sensor resolutions, only need to do this once
    getAres();

    fastcompaccelBMX055(accelBias);
    //Serial.println("accel biases (mg)"); Serial.println(1000.*accelBias[0]); Serial.println(1000.*accelBias[1]); Serial.println(1000.*accelBias[2]);
  }
  else
  {
    //Serial.print("Could not connect to BMX055: 0x");
    //Serial.println(c, HEX);
    while (1) ; // Loop forever if communication doesn't happen
  }
  last_wakeup = micros();

  init_time = millis();
  //Serial.print(init_time);

  // << nothing before the first delay will be printed to the serial
  delay(1500);

  if(!silent){
    Serial.print("Pin configuration setting: ");
    Serial.println( I2S_PIN_PATTERN , HEX );
    Serial.println( "Initializing." );
  }

  if(!silent) Serial.println( "Initialized I2C Codec" );

  // prepare I2S RX with interrupts
  I2SRx0.begin( CLOCK_TYPE, i2s_rx_callback );
  if(!silent) Serial.println( "Initialized I2S RX without DMA" );

  // I2STx0.begin( CLOCK_TYPE, i2s_tx_callback );
  // Serial.println( "Initialized I2S TX without DMA" );

  delay(5000);
  // start the I2S RX
  I2SRx0.start();
  //I2STx0.start();
  if(!silent) Serial.println( "Started I2S RX" );

  // Make sure to initialize Serial2 at the end of setup
  Serial2.begin(9600);

  // Enable LiPo charger's CHG read.
  pinMode(chargerCHG, INPUT_PULLUP);
}

void loop()
{
  // USB Connection Check
  if (bitRead(USB0_OTGSTAT,5)){
    // Disconnected; this flag will briefly come up when USB gets disconnected.
    // Immediately put into RUN mode.
    if (currMode != RUN){
      resetSampling();
      currMode = RUN;

      LEDColors c = BLUE_NOOP;
      changeStatusLED(c);

      // Reset state
      currState = NOT_CUTTING;
    }
  } else{
    // Connected
    if (currMode == RUN){
      // NOTE: Assume the battery will start charging when USB is connected.
      if (digitalRead(chargerCHG) == 0){
        LEDColors c = ORANGE_CHARG;
        changeStatusLED(c);

        currMode = CHARGING;
      }
    } else if (currMode == CHARGING){
      // CHARGE mode and battery is NOT charging.
      // Display white LED to notify charge completion.
      if (digitalRead(chargerCHG) == 1){
        LEDColors c = WHITE_NONE;
        changeStatusLED(c);
      } else{
        LEDColors c = ORANGE_CHARG;
        changeStatusLED(c);
      }
    }
  }

  // Two-way Communication via BLE, only on RUN mode.
  // TODO: Currently assuming the entire batch will arrive atomically. This may
  //       not be true.
  // TODO: CONFIRMED! Data still arrives 4 bytes by 4 bytes; may not happen in single loop()
  while(Serial2.available() > 0){
    characterRead = Serial2.read();
    bleBuffer.concat(characterRead);
  }
  if (bleBuffer.length() > 0){
    if (currMode == RUN){
      //TODO: Two-way Communication Protocol
    }
    Serial.println(bleBuffer);
    // Clear BLE buffer
    bleBuffer = "";
  }

  // Listen for Serial input from RPi, only on CHARGING/DATA_COLLECTION modes.
  while(Serial.available() > 0){
    characterRead = Serial.read();
    if (characterRead != '\n')
      wireBuffer.concat(characterRead);
  }
  if (wireBuffer.length() > 0){
    if (currMode == CHARGING){
      // Check the received info; iff data collection request, change the mode
      if (wireBuffer == START_COLLECTION){
        LEDColors c = CYAN_DATA;
        changeStatusLED(c);

        Serial.print(START_CONFIRM);
        resetSampling();
        currMode = DATA_COLLECTION;
      }
    } else if (currMode == DATA_COLLECTION){
      // Check the received info; iff data collection finished, change the mode
      if (wireBuffer == END_COLLECTION){
        LEDColors c = ORANGE_CHARG;
        changeStatusLED(c);

        Serial.print(END_CONFIRM);
        currMode = CHARGING;
      }
    }
    // Clear wire buffer
    wireBuffer = "";
  }

  // After handling all state / mode changes, check if features need to be computed.
  if (currMode == RUN && compute_feature_now[buffer_compute_index]){
    compute_features();
  }
}

// Function for resetting the data collection routine.
// Need to call this before entering RUN and DATA_COLLECTION modes
void resetSampling(){
  // Reset downsampling variables
  subsample_counter = 0;
  accel_counter = 0;

  // Reset feature variables
  audioSamplingCount = 0;
  accelSamplingCount = 0;
  ave_x = 0;
  ave_y = 0;
  ave_z = 0;
  ene = 0;
  restCount = 0;

  // Reset buffer management variables
  buffer_record_index = 0;
  buffer_compute_index = 0;
  compute_feature_now[0] = false;
  compute_feature_now[1] = false;
  record_feature_now[0] = true;
  record_feature_now[1] = true;
}

// Function that reflects LEDColors into statusLED.
// Will not have effect if statusLED is turned off.
void changeStatusLED(LEDColors color){
  if (!statusLED) return;
  switch (color){
    case RED_BAD:
      digitalWrite(21, LOW);
      digitalWrite(20, HIGH);
      digitalWrite(22, HIGH);
      break;
    case BLUE_NOOP:
      digitalWrite(21, HIGH);
      digitalWrite(20, HIGH);
      digitalWrite(22, LOW);
      break;
    case GREEN_OK:
      digitalWrite(21, HIGH);
      digitalWrite(20, LOW);
      digitalWrite(22, HIGH);
      break;
    case ORANGE_CHARG:
      digitalWrite(21, LOW);
      digitalWrite(20, LOW);
      digitalWrite(22, HIGH);
      break;
    case PURPLE_ERR:
      digitalWrite(21, LOW);
      digitalWrite(20, HIGH);
      digitalWrite(22, LOW);
      break;
    case CYAN_DATA:
      digitalWrite(21, HIGH);
      digitalWrite(20, LOW);
      digitalWrite(22, LOW);
      break;
    case WHITE_NONE:
      digitalWrite(21, LOW);
      digitalWrite(20, LOW);
      digitalWrite(22, LOW);
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
    case AFS_2G:
      aRes = 2.0 / 2048.0;
      break;
    case AFS_4G:
      aRes = 4.0 / 2048.0;
      break;
    case AFS_8G:
      aRes = 8.0 / 2048.0;
      break;
    case AFS_16G:
      aRes = 16.0 / 2048.0;
      break;
  }
}

void readAccelData(int16_t * destination)
{
  uint8_t rawData[6];  // x/y/z accel register data stored here
  readBytes(BMX055_ACC_ADDRESS, BMX055_ACC_D_X_LSB, 6, &rawData[0]);       // Read the six raw data registers into data array
  if ((rawData[0] & 0x01) && (rawData[2] & 0x01) && (rawData[4] & 0x01)) { // Check that all 3 axes have new data
    destination[0] = (int16_t) (((int16_t)rawData[1] << 8) | rawData[0]) >> 4;  // Turn the MSB and LSB into a signed 12-bit value
    destination[1] = (int16_t) (((int16_t)rawData[3] << 8) | rawData[2]) >> 4;
    destination[2] = (int16_t) (((int16_t)rawData[5] << 8) | rawData[4]) >> 4;
  }
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
  if(!silent) Serial.println("SENtral in standby mode");
  // Place SENtral in pass-through mode
  writeByte(EM7180_ADDRESS, EM7180_PassThruControl, 0x01);
  if (readByte(EM7180_ADDRESS, EM7180_PassThruStatus) & 0x01) {
    if(!silent) Serial.println("SENtral in pass-through mode");
  }
  else {
    Serial.println("ERROR! SENtral not in pass-through mode!");
  }

}


void initBMX055()
{
  // start with all sensors in default mode with all registers reset
  writeByte(BMX055_ACC_ADDRESS,  BMX055_ACC_BGW_SOFTRESET, 0xB6);  // reset accelerometer
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
}

void fastcompaccelBMX055(float * dest1)
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
}


// I2C communication with the M24512DFM EEPROM is a little different from I2C communication with the usual motion sensor
// since the address is defined by two bytes


// I2C communication with the MS5637 is a little different from that with the MPU9250 and most other sensors
// For the MS5637, we write commands, and the MS5637 sends data in response, rather than directly reading
// MS5637 registers

void MS5637Reset()
{
  Wire.beginTransmission(MS5637_ADDRESS);  // Initialize the Tx buffer
  Wire.write(MS5637_RESET);                // Put reset command in Tx buffer
  Wire.endTransmission();                  // Send the Tx buffer
}

void MS5637PromRead(uint16_t * destination)
{
  uint8_t data[2] = {0, 0};
  for (uint8_t ii = 0; ii < 7; ii++) {
    Wire.beginTransmission(MS5637_ADDRESS);  // Initialize the Tx buffer
    Wire.write(0xA0 | ii << 1);              // Put PROM address in Tx buffer
    Wire.endTransmission(I2C_NOSTOP);        // Send the Tx buffer, but send a restart to keep connection alive
    uint8_t i = 0;
    Wire.requestFrom(MS5637_ADDRESS, 2);   // Read two bytes from slave PROM address
    while (Wire.available()) {
      data[i++] = Wire.read();
    }               // Put read results in the Rx buffer
    destination[ii] = (uint16_t) (((uint16_t) data[0] << 8) | data[1]); // construct PROM data for return to main program
  }
}

uint32_t MS5637Read(uint8_t CMD, uint8_t OSR)  // temperature data read
{
  uint8_t data[3] = {0, 0, 0};
  Wire.beginTransmission(MS5637_ADDRESS);  // Initialize the Tx buffer
  Wire.write(CMD | OSR);                  // Put pressure conversion command in Tx buffer
  Wire.endTransmission(I2C_NOSTOP);        // Send the Tx buffer, but send a restart to keep connection alive

  switch (OSR)
  {
    case ADC_256: delay(1); break;  // delay for conversion to complete
    case ADC_512: delay(3); break;
    case ADC_1024: delay(4); break;
    case ADC_2048: delay(6); break;
    case ADC_4096: delay(10); break;
    case ADC_8192: delay(20); break;
  }

  Wire.beginTransmission(MS5637_ADDRESS);  // Initialize the Tx buffer
  Wire.write(0x00);                        // Put ADC read command in Tx buffer
  Wire.endTransmission(I2C_NOSTOP);        // Send the Tx buffer, but send a restart to keep connection alive
  uint8_t i = 0;
  Wire.requestFrom(MS5637_ADDRESS, 3);     // Read three bytes from slave PROM address
  while (Wire.available()) {
    data[i++] = Wire.read();
  }               // Put read results in the Rx buffer
  return (uint32_t) (((uint32_t) data[0] << 16) | (uint32_t) data[1] << 8 | data[2]); // construct PROM data for return to main program
}



unsigned char MS5637checkCRC(uint16_t * n_prom)  // calculate checksum from PROM register contents
{
  int cnt;
  unsigned int n_rem = 0;
  unsigned char n_bit;

  n_prom[0] = ((n_prom[0]) & 0x0FFF);  // replace CRC byte by 0 for checksum calculation
  n_prom[7] = 0;
  for (cnt = 0; cnt < 16; cnt++)
  {
    if (cnt % 2 == 1) n_rem ^= (unsigned short) ((n_prom[cnt >> 1]) & 0x00FF);
    else         n_rem ^= (unsigned short)  (n_prom[cnt >> 1] >> 8);
    for (n_bit = 8; n_bit > 0; n_bit--)
    {
      if (n_rem & 0x8000)    n_rem = (n_rem << 1) ^ 0x3000;
      else                  n_rem = (n_rem << 1);
    }
  }
  n_rem = ((n_rem >> 12) & 0x000F);
  return (n_rem ^ 0x00);
}

// simple function to scan for I2C devices on the bus
void I2Cscan()
{
  // scan for i2c devices
  byte error, address;
  int nDevices;

  if(!silent) Serial.println("Scanning...");

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
      if(!silent){
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
      if(!silent){
        Serial.print("Unknow error at address 0x");
        if (address < 16)
          Serial.print("0");
        Serial.println(address, HEX);
      }
    }
  }
  if (nDevices == 0){
    LEDColors c = PURPLE_ERR;
    changeStatusLED(c);
    if(!silent) Serial.println("No I2C devices found\n");
  }
  else{
    if(!silent) Serial.println("done\n");
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
