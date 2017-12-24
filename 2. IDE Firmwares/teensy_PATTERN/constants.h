/*

Infinite Uptime BLE Module Firmware

Constants including register addresses and Hamming window presets.

*/

//==============================================================================
//======================= Register Addresses and Enums =========================
//==============================================================================

// See MS5637-02BA03 Low Voltage Barometric Pressure Sensor Data Sheet http://www.meas-spec.com/downloads/MS5637-02BA03.pdf
#define MS5637_RESET      0x1E
#define MS5637_CONVERT_D1 0x40
#define MS5637_CONVERT_D2 0x50
#define MS5637_ADC_READ   0x00

#define BLEport Serial1

// DEPRECATED: tons of BMX055 registers

// MPU-9250 accel register constants
#define SELF_TEST_X_ACCEL 0x0D
#define SELF_TEST_Y_ACCEL 0x0E
#define SELF_TEST_Z_ACCEL 0x0F

#define SMPLRT_DIV       0x19
#define CONFIG           0x1A
#define GYRO_CONFIG      0x1B
#define ACCEL_CONFIG     0x1C
#define ACCEL_CONFIG2    0x1D
#define LP_ACCEL_ODR     0x1E
#define WOM_THR          0x1F

#define MOT_DUR          0x20  // Duration counter threshold for motion interrupt generation, 1 kHz rate, LSB = 1 ms
#define ZMOT_THR         0x21  // Zero-motion detection threshold bits [7:0]
#define ZRMOT_DUR        0x22  // Duration counter threshold for zero motion interrupt generation, 16 Hz rate, LSB = 64 ms

#define FIFO_EN          0x23
#define I2C_MST_CTRL     0x24
#define I2C_SLV0_ADDR    0x25
#define I2C_SLV0_REG     0x26
#define I2C_SLV0_CTRL    0x27
#define I2C_SLV1_ADDR    0x28
#define I2C_SLV1_REG     0x29
#define I2C_SLV1_CTRL    0x2A
#define I2C_SLV2_ADDR    0x2B
#define I2C_SLV2_REG     0x2C
#define I2C_SLV2_CTRL    0x2D
#define I2C_SLV3_ADDR    0x2E
#define I2C_SLV3_REG     0x2F
#define I2C_SLV3_CTRL    0x30
#define I2C_SLV4_ADDR    0x31
#define I2C_SLV4_REG     0x32
#define I2C_SLV4_DO      0x33
#define I2C_SLV4_CTRL    0x34
#define I2C_SLV4_DI      0x35
#define I2C_MST_STATUS   0x36
#define INT_PIN_CFG      0x37
#define INT_ENABLE       0x38
#define DMP_INT_STATUS   0x39  // Check DMP interrupt
#define INT_STATUS       0x3A
#define ACCEL_XOUT_H     0x3B
#define ACCEL_XOUT_L     0x3C
#define ACCEL_YOUT_H     0x3D
#define ACCEL_YOUT_L     0x3E
#define ACCEL_ZOUT_H     0x3F
#define ACCEL_ZOUT_L     0x40
#define MOT_DETECT_STATUS 0x61
#define I2C_SLV0_DO      0x63
#define I2C_SLV1_DO      0x64
#define I2C_SLV2_DO      0x65
#define I2C_SLV3_DO      0x66
#define I2C_MST_DELAY_CTRL 0x67
#define SIGNAL_PATH_RESET  0x68
#define MOT_DETECT_CTRL  0x69
#define USER_CTRL        0x6A  // Bit 7 enable DMP, bit 3 reset DMP
#define PWR_MGMT_1       0x6B // Device defaults to the SLEEP mode
#define PWR_MGMT_2       0x6C
#define DMP_BANK         0x6D  // Activates a specific bank in the DMP
#define DMP_RW_PNT       0x6E  // Set read/write pointer to a specific start address in specified DMP bank
#define DMP_REG          0x6F  // Register in DMP from which to read or to which to write
#define DMP_REG_1        0x70
#define DMP_REG_2        0x71
#define FIFO_COUNTH      0x72
#define FIFO_COUNTL      0x73
#define FIFO_R_W         0x74
#define WHO_AM_I_MPU9250 0x75 // Should return 0x71
#define XA_OFFSET_H      0x77
#define XA_OFFSET_L      0x78
#define YA_OFFSET_H      0x7A
#define YA_OFFSET_L      0x7B
#define ZA_OFFSET_H      0x7D
#define ZA_OFFSET_L      0x7E

#if ADO
#define MPU9250_ADDRESS 0x69  // Device address when ADO = 1
#else
#define MPU9250_ADDRESS 0x68  // Device address when ADO = 0
#endif

#define AHRS true         // set to false for basic data read
#define SerialDebug true   // set to true to get Serial output for debugging


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

// DEPRECATED
// #define BMX055_ACC_ADDRESS  0x18   // Address of BMX055 accelerometer

#define BMX055_GYRO_ADDRESS 0x68   // Address of BMX055 gyroscope
#define BMX055_MAG_ADDRESS  0x10   // Address of BMX055 magnetometer
#define MS5637_ADDRESS      0x76   // Address of MS5637 altimeter
#define EM7180_ADDRESS      0x28   // Address of the EM7180 SENtral sensor hub
#define M24512DFM_DATA_ADDRESS   0x50   // Address of the 500 page M24512DFM EEPROM data buffer, 1024 bits (128 8-bit bytes) per page
#define M24512DFM_IDPAGE_ADDRESS 0x58   // Address of the single M24512DFM lockable EEPROM ID page

// Set initial input parameters
// define MPU ACC full scale options
// CHANGE: new constants for MPU
#define AFS_2G  0x0
#define AFS_4G  0x1
#define AFS_8G  0x2
#define AFS_16G 0x3


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

// BMP280 registers
#define BMP280_TEMP_XLSB  0xFC
#define BMP280_TEMP_LSB   0xFB
#define BMP280_TEMP_MSB   0xFA
#define BMP280_PRESS_XLSB 0xF9
#define BMP280_PRESS_LSB  0xF8
#define BMP280_PRESS_MSB  0xF7
#define BMP280_CONFIG     0xF5
#define BMP280_CTRL_MEAS  0xF4
#define BMP280_STATUS     0xF3
#define BMP280_RESET      0xE0
#define BMP280_ID         0xD0  // should be 0x58
#define BMP280_CALIB00    0x88
#define BMP280_ADDRESS    0x76  //SDO = GND  0x77 if SDO = VCC      // Address of BMP280 altimeter 

enum BMP280Posr {
  P_OSR_00 = 0,  // no op
  P_OSR_01,
  P_OSR_02,
  P_OSR_04,
  P_OSR_08,
  P_OSR_16
};

enum BMP280Tosr {
  T_OSR_00 = 0,  // no op
  T_OSR_01,
  T_OSR_02,
  T_OSR_04,
  T_OSR_08,
  T_OSR_16
};

enum BMP280IIRFilter {
  full = 0,  // bandwidth at full sample rate
  BW0_223ODR,
  BW0_092ODR,
  BW0_042ODR,
  BW0_021ODR // bandwidth at 0.021 x sample rate
};

enum BMP280Mode {
  Sleep = 0,
  forced,
  forced2,
  normal
};

enum BMP280SBy {
  t_00_5ms = 0,
  t_62_5ms,
  t_125ms,
  t_250ms,
  t_500ms,
  t_1000ms,
  t_2000ms,
  t_4000ms,
};

//============================= User defined Enums =============================
// Operation mode enum
enum OpMode {
  RUN              = 0,
  CHARGING         = 1,
  DATA_COLLECTION  = 2
};

// Operation state enum
enum OpState {
  NOT_CUTTING     = 0,
  NORMAL_CUTTING  = 1,
  WARNING_CUTTING = 2,
  BAD_CUTTING     = 3
};

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

//==============================================================================
//========================== Hamming Window Presets ============================
//==============================================================================

int magsize_512 = 257;
float hamming_K_512 = 1.8519;         // 1/0.5400
float inverse_wlen_512 = 1/512.0;
q15_t hamming_window_512 [512] = {
  2621,   2621,   2624,   2631,   2641,   2650,   2660,   2677,   2693,   2713,
  2736,   2759,   2785,   2811,   2844,   2877,   2909,   2949,   2988,   3027,
  3073,   3119,   3168,   3217,   3270,   3325,   3381,   3440,   3502,   3565,
  3630,   3699,   3768,   3840,   3915,   3991,   4069,   4148,   4230,   4315,
  4400,   4489,   4580,   4672,   4764,   4862,   4961,   5059,   5160,   5265,
  5370,   5478,   5586,   5698,   5813,   5927,   6042,   6160,   6281,   6402,
  6527,   6651,   6776,   6907,   7035,   7166,   7300,   7435,   7572,   7710,
  7847,   7988,   8133,   8277,   8421,   8568,   8716,   8863,   9014,   9168,
  9319,   9476,   9630,   9787,   9945,  10105,  10266,  10426,  10590,  10754,
  10918,  11082,  11249,  11416,  11586,  11757,  11927,  12097,  12268,  12442,
  12615,  12792,  12966,  13143,  13320,  13497,  13674,  13854,  14031,  14211,
  14391,  14571,  14755,  14935,  15119,  15299,  15482,  15666,  15849,  16033,
  16216,  16400,  16587,  16770,  16954,  17140,  17324,  17511,  17694,  17878,
  18064,  18248,  18435,  18618,  18802,  18989,  19172,  19356,  19539,  19723,
  19906,  20090,  20270,  20453,  20634,  20817,  20997,  21177,  21358,  21535,
  21715,  21892,  22069,  22246,  22423,  22596,  22773,  22947,  23121,  23291,
  23461,  23632,  23802,  23973,  24140,  24307,  24471,  24634,  24798,  24962,
  25123,  25283,  25444,  25601,  25758,  25912,  26070,  26220,  26374,  26525,
  26673,  26820,  26968,  27112,  27256,  27400,  27541,  27679,  27816,  27954,
  28088,  28223,  28354,  28481,  28613,  28737,  28862,  28986,  29107,  29229,
  29347,  29461,  29576,  29691,  29802,  29910,  30018,  30123,  30228,  30330,
  30428,  30526,  30624,  30716,  30808,  30900,  30988,  31073,  31159,  31241,
  31319,  31398,  31473,  31549,  31621,  31689,  31758,  31824,  31886,  31948,
  32007,  32063,  32119,  32171,  32220,  32269,  32315,  32361,  32400,  32440,
  32479,  32512,  32545,  32577,  32604,  32630,  32653,  32676,  32695,  32712,
  32728,  32738,  32748,  32758,  32764,  32767,  32767,  32767,  32764,  32758,
  32748,  32738,  32728,  32712,  32695,  32676,  32653,  32630,  32604,  32577,
  32545,  32512,  32479,  32440,  32400,  32361,  32315,  32269,  32220,  32171,
  32119,  32063,  32007,  31948,  31886,  31824,  31758,  31689,  31621,  31549,
  31473,  31398,  31319,  31241,  31159,  31073,  30988,  30900,  30808,  30716,
  30624,  30526,  30428,  30330,  30228,  30123,  30018,  29910,  29802,  29691,
  29576,  29461,  29347,  29229,  29107,  28986,  28862,  28737,  28613,  28481,
  28354,  28223,  28088,  27954,  27816,  27679,  27541,  27400,  27256,  27112,
  26968,  26820,  26673,  26525,  26374,  26220,  26070,  25912,  25758,  25601,
  25444,  25283,  25123,  24962,  24798,  24634,  24471,  24307,  24140,  23973,
  23802,  23632,  23461,  23291,  23121,  22947,  22773,  22596,  22423,  22246,
  22069,  21892,  21715,  21535,  21358,  21177,  20997,  20817,  20634,  20453,
  20270,  20090,  19906,  19723,  19539,  19356,  19172,  18989,  18802,  18618,
  18435,  18248,  18064,  17878,  17694,  17511,  17324,  17140,  16954,  16770,
  16587,  16400,  16216,  16033,  15849,  15666,  15482,  15299,  15119,  14935,
  14755,  14571,  14391,  14211,  14031,  13854,  13674,  13497,  13320,  13143,
  12966,  12792,  12615,  12442,  12268,  12097,  11927,  11757,  11586,  11416,
  11249,  11082,  10918,  10754,  10590,  10426,  10266,  10105,   9945,   9787,
  9630,   9476,   9319,   9168,   9014,   8863,   8716,   8568,   8421,   8277,
  8133,   7988,   7847,   7710,   7572,   7435,   7300,   7166,   7035,   6907,
  6776,   6651,   6527,   6402,   6281,   6160,   6042,   5927,   5813,   5698,
  5586,   5478,   5370,   5265,   5160,   5059,   4961,   4862,   4764,   4672,
  4580,   4489,   4400,   4315,   4230,   4148,   4069,   3991,   3915,   3840,
  3768,   3699,   3630,   3565,   3502,   3440,   3381,   3325,   3270,   3217,
  3168,   3119,   3073,   3027,   2988,   2949,   2909,   2877,   2844,   2811,
  2785,   2759,   2736,   2713,   2693,   2677,   2660,   2650,   2641,   2631,
  2624,   2621
};

int magsize_2048 = 1025;
float hamming_K_2048 = 1.8519;        // 1/0.5400
float inverse_wlen_2048 = 1/2048.0;
q15_t hamming_window_2048 [2048] = {
  2621,   2621,   2621,   2621,   2621,   2624,   2624,   2624,   2624,   2627,
  2627,   2631,   2631,   2634,   2634,   2637,   2641,   2641,   2644,   2647,
  2650,   2654,   2654,   2657,   2660,   2667,   2670,   2673,   2677,   2680,
  2683,   2690,   2693,   2700,   2703,   2706,   2713,   2719,   2723,   2729,
  2736,   2739,   2745,   2752,   2759,   2765,   2772,   2778,   2785,   2791,
  2798,   2804,   2811,   2821,   2827,   2834,   2844,   2850,   2860,   2867,
  2877,   2883,   2893,   2903,   2909,   2919,   2929,   2939,   2949,   2958,
  2968,   2978,   2988,   2998,   3008,   3017,   3027,   3040,   3050,   3060,
  3073,   3083,   3096,   3106,   3119,   3132,   3142,   3155,   3168,   3178,
  3191,   3204,   3217,   3230,   3244,   3257,   3270,   3283,   3296,   3312,
  3325,   3339,   3352,   3368,   3381,   3398,   3411,   3427,   3440,   3457,
  3470,   3486,   3502,   3519,   3532,   3548,   3565,   3581,   3597,   3614,
  3630,   3647,   3663,   3683,   3699,   3715,   3732,   3751,   3768,   3787,
  3804,   3824,   3840,   3860,   3876,   3896,   3915,   3932,   3951,   3971,
  3991,   4010,   4030,   4050,   4069,   4089,   4109,   4128,   4148,   4168,
  4191,   4210,   4230,   4253,   4272,   4292,   4315,   4335,   4358,   4381,
  4400,   4423,   4446,   4466,   4489,   4512,   4535,   4558,   4580,   4603,
  4626,   4649,   4672,   4695,   4718,   4741,   4764,   4790,   4813,   4836,
  4862,   4885,   4911,   4934,   4961,   4984,   5010,   5033,   5059,   5085,
  5111,   5134,   5160,   5187,   5213,   5239,   5265,   5292,   5318,   5344,
  5370,   5396,   5423,   5452,   5478,   5505,   5534,   5560,   5586,   5616,
  5642,   5672,   5698,   5727,   5754,   5783,   5813,   5839,   5868,   5898,
  5927,   5953,   5983,   6012,   6042,   6071,   6101,   6130,   6160,   6189,
  6219,   6252,   6281,   6311,   6340,   6373,   6402,   6432,   6465,   6494,
  6527,   6556,   6589,   6619,   6651,   6681,   6714,   6746,   6776,   6809,
  6841,   6874,   6907,   6940,   6969,   7002,   7035,   7068,   7100,   7133,
  7166,   7202,   7235,   7267,   7300,   7333,   7369,   7402,   7435,   7471,
  7503,   7536,   7572,   7605,   7641,   7674,   7710,   7743,   7779,   7815,
  7847,   7883,   7920,   7956,   7988,   8024,   8060,   8096,   8133,   8169,
  8205,   8241,   8277,   8313,   8349,   8385,   8421,   8457,   8493,   8529,
  8568,   8604,   8640,   8676,   8716,   8752,   8791,   8827,   8863,   8903,
  8939,   8978,   9014,   9053,   9089,   9129,   9168,   9204,   9243,   9283,
  9319,   9358,   9397,   9437,   9476,   9512,   9551,   9591,   9630,   9669,
  9709,   9748,   9787,   9827,   9866,   9905,   9945,   9984,  10023,  10066,
  10105,  10144,  10184,  10223,  10266,  10305,  10344,  10387,  10426,  10466,
  10508,  10548,  10590,  10629,  10672,  10711,  10754,  10793,  10836,  10875,
  10918,  10957,  11000,  11042,  11082,  11124,  11167,  11206,  11249,  11291,
  11334,  11377,  11416,  11458,  11501,  11544,  11586,  11629,  11671,  11714,
  11757,  11799,  11842,  11884,  11927,  11970,  12012,  12055,  12097,  12140,
  12183,  12225,  12268,  12314,  12356,  12399,  12442,  12487,  12530,  12573,
  12615,  12661,  12704,  12746,  12792,  12835,  12877,  12923,  12966,  13008,
  13054,  13097,  13143,  13185,  13231,  13274,  13320,  13362,  13408,  13451,
  13497,  13539,  13585,  13631,  13674,  13719,  13762,  13808,  13854,  13896,
  13942,  13988,  14031,  14077,  14123,  14165,  14211,  14257,  14303,  14345,
  14391,  14437,  14483,  14526,  14571,  14617,  14663,  14709,  14755,  14798,
  14843,  14889,  14935,  14981,  15027,  15073,  15119,  15161,  15207,  15253,
  15299,  15345,  15391,  15437,  15482,  15528,  15574,  15620,  15666,  15712,
  15758,  15804,  15849,  15895,  15941,  15987,  16033,  16079,  16125,  16171,
  16216,  16262,  16308,  16354,  16400,  16446,  16492,  16541,  16587,  16633,
  16678,  16724,  16770,  16816,  16862,  16908,  16954,  17000,  17045,  17095,
  17140,  17186,  17232,  17278,  17324,  17370,  17416,  17462,  17511,  17557,
  17602,  17648,  17694,  17740,  17786,  17832,  17878,  17927,  17973,  18019,
  18064,  18110,  18156,  18202,  18248,  18294,  18343,  18389,  18435,  18481,
  18527,  18572,  18618,  18664,  18710,  18756,  18802,  18848,  18897,  18943,
  18989,  19034,  19080,  19126,  19172,  19218,  19264,  19310,  19356,  19401,
  19447,  19493,  19539,  19585,  19631,  19677,  19723,  19768,  19814,  19860,
  19906,  19952,  19998,  20044,  20090,  20135,  20181,  20227,  20270,  20316,
  20362,  20407,  20453,  20499,  20545,  20591,  20634,  20679,  20725,  20771,
  20817,  20863,  20905,  20951,  20997,  21043,  21086,  21132,  21177,  21223,
  21266,  21312,  21358,  21400,  21446,  21492,  21535,  21581,  21626,  21669,
  21715,  21757,  21803,  21849,  21892,  21938,  21980,  22026,  22069,  22115,
  22157,  22203,  22246,  22292,  22334,  22380,  22423,  22465,  22511,  22554,
  22596,  22642,  22685,  22727,  22773,  22816,  22858,  22901,  22947,  22990,
  23032,  23075,  23121,  23163,  23206,  23248,  23291,  23334,  23376,  23419,
  23461,  23504,  23547,  23589,  23632,  23674,  23717,  23760,  23802,  23845,
  23887,  23930,  23973,  24012,  24054,  24097,  24140,  24182,  24222,  24264,
  24307,  24346,  24389,  24431,  24471,  24513,  24553,  24595,  24634,  24677,
  24716,  24759,  24798,  24841,  24880,  24923,  24962,  25001,  25044,  25083,
  25123,  25165,  25205,  25244,  25283,  25323,  25365,  25405,  25444,  25483,
  25522,  25562,  25601,  25640,  25680,  25719,  25758,  25798,  25837,  25876,
  25912,  25952,  25991,  26030,  26070,  26106,  26145,  26184,  26220,  26260,
  26299,  26335,  26374,  26411,  26450,  26486,  26525,  26561,  26597,  26637,
  26673,  26712,  26748,  26784,  26820,  26859,  26895,  26932,  26968,  27004,
  27040,  27076,  27112,  27148,  27184,  27220,  27256,  27292,  27328,  27364,
  27400,  27433,  27469,  27505,  27541,  27574,  27610,  27646,  27679,  27715,
  27747,  27783,  27816,  27852,  27885,  27918,  27954,  27987,  28019,  28055,
  28088,  28121,  28154,  28187,  28223,  28255,  28288,  28321,  28354,  28386,
  28419,  28449,  28481,  28514,  28547,  28580,  28613,  28642,  28675,  28708,
  28737,  28770,  28799,  28832,  28862,  28894,  28924,  28957,  28986,  29016,
  29048,  29078,  29107,  29137,  29170,  29199,  29229,  29258,  29288,  29317,
  29347,  29376,  29406,  29435,  29461,  29491,  29520,  29550,  29576,  29605,
  29635,  29661,  29691,  29717,  29746,  29773,  29802,  29828,  29854,  29884,
  29910,  29936,  29966,  29992,  30018,  30044,  30071,  30097,  30123,  30149,
  30176,  30202,  30228,  30254,  30277,  30303,  30330,  30356,  30379,  30405,
  30428,  30454,  30477,  30503,  30526,  30552,  30575,  30598,  30624,  30647,
  30670,  30693,  30716,  30739,  30762,  30785,  30808,  30831,  30854,  30877,
  30900,  30923,  30942,  30965,  30988,  31008,  31031,  31054,  31073,  31096,
  31116,  31136,  31159,  31178,  31198,  31221,  31241,  31260,  31280,  31299,
  31319,  31339,  31358,  31378,  31398,  31417,  31437,  31457,  31473,  31493,
  31512,  31529,  31549,  31565,  31585,  31601,  31621,  31637,  31657,  31673,
  31689,  31706,  31725,  31742,  31758,  31775,  31791,  31807,  31824,  31840,
  31857,  31870,  31886,  31902,  31919,  31932,  31948,  31961,  31978,  31991,
  32007,  32020,  32037,  32050,  32063,  32076,  32092,  32106,  32119,  32132,
  32145,  32158,  32171,  32184,  32197,  32210,  32220,  32233,  32246,  32256,
  32269,  32283,  32292,  32305,  32315,  32328,  32338,  32348,  32361,  32371,
  32381,  32391,  32400,  32410,  32420,  32430,  32440,  32450,  32459,  32469,
  32479,  32486,  32496,  32505,  32512,  32522,  32528,  32538,  32545,  32555,
  32561,  32568,  32577,  32584,  32591,  32597,  32604,  32610,  32617,  32623,
  32630,  32636,  32643,  32650,  32653,  32659,  32666,  32669,  32676,  32682,
  32686,  32689,  32695,  32699,  32705,  32709,  32712,  32715,  32718,  32722,
  32728,  32731,  32735,  32735,  32738,  32741,  32745,  32748,  32748,  32751,
  32754,  32754,  32758,  32758,  32761,  32761,  32764,  32764,  32764,  32764,
  32767,  32767,  32767,  32767,  32767,  32767,  32767,  32767,  32767,  32764,
  32764,  32764,  32764,  32761,  32761,  32758,  32758,  32754,  32754,  32751,
  32748,  32748,  32745,  32741,  32738,  32735,  32735,  32731,  32728,  32722,
  32718,  32715,  32712,  32709,  32705,  32699,  32695,  32689,  32686,  32682,
  32676,  32669,  32666,  32659,  32653,  32650,  32643,  32636,  32630,  32623,
  32617,  32610,  32604,  32597,  32591,  32584,  32577,  32568,  32561,  32555,
  32545,  32538,  32528,  32522,  32512,  32505,  32496,  32486,  32479,  32469,
  32459,  32450,  32440,  32430,  32420,  32410,  32400,  32391,  32381,  32371,
  32361,  32348,  32338,  32328,  32315,  32305,  32292,  32283,  32269,  32256,
  32246,  32233,  32220,  32210,  32197,  32184,  32171,  32158,  32145,  32132,
  32119,  32106,  32092,  32076,  32063,  32050,  32037,  32020,  32007,  31991,
  31978,  31961,  31948,  31932,  31919,  31902,  31886,  31870,  31857,  31840,
  31824,  31807,  31791,  31775,  31758,  31742,  31725,  31706,  31689,  31673,
  31657,  31637,  31621,  31601,  31585,  31565,  31549,  31529,  31512,  31493,
  31473,  31457,  31437,  31417,  31398,  31378,  31358,  31339,  31319,  31299,
  31280,  31260,  31241,  31221,  31198,  31178,  31159,  31136,  31116,  31096,
  31073,  31054,  31031,  31008,  30988,  30965,  30942,  30923,  30900,  30877,
  30854,  30831,  30808,  30785,  30762,  30739,  30716,  30693,  30670,  30647,
  30624,  30598,  30575,  30552,  30526,  30503,  30477,  30454,  30428,  30405,
  30379,  30356,  30330,  30303,  30277,  30254,  30228,  30202,  30176,  30149,
  30123,  30097,  30071,  30044,  30018,  29992,  29966,  29936,  29910,  29884,
  29854,  29828,  29802,  29773,  29746,  29717,  29691,  29661,  29635,  29605,
  29576,  29550,  29520,  29491,  29461,  29435,  29406,  29376,  29347,  29317,
  29288,  29258,  29229,  29199,  29170,  29137,  29107,  29078,  29048,  29016,
  28986,  28957,  28924,  28894,  28862,  28832,  28799,  28770,  28737,  28708,
  28675,  28642,  28613,  28580,  28547,  28514,  28481,  28449,  28419,  28386,
  28354,  28321,  28288,  28255,  28223,  28187,  28154,  28121,  28088,  28055,
  28019,  27987,  27954,  27918,  27885,  27852,  27816,  27783,  27747,  27715,
  27679,  27646,  27610,  27574,  27541,  27505,  27469,  27433,  27400,  27364,
  27328,  27292,  27256,  27220,  27184,  27148,  27112,  27076,  27040,  27004,
  26968,  26932,  26895,  26859,  26820,  26784,  26748,  26712,  26673,  26637,
  26597,  26561,  26525,  26486,  26450,  26411,  26374,  26335,  26299,  26260,
  26220,  26184,  26145,  26106,  26070,  26030,  25991,  25952,  25912,  25876,
  25837,  25798,  25758,  25719,  25680,  25640,  25601,  25562,  25522,  25483,
  25444,  25405,  25365,  25323,  25283,  25244,  25205,  25165,  25123,  25083,
  25044,  25001,  24962,  24923,  24880,  24841,  24798,  24759,  24716,  24677,
  24634,  24595,  24553,  24513,  24471,  24431,  24389,  24346,  24307,  24264,
  24222,  24182,  24140,  24097,  24054,  24012,  23973,  23930,  23887,  23845,
  23802,  23760,  23717,  23674,  23632,  23589,  23547,  23504,  23461,  23419,
  23376,  23334,  23291,  23248,  23206,  23163,  23121,  23075,  23032,  22990,
  22947,  22901,  22858,  22816,  22773,  22727,  22685,  22642,  22596,  22554,
  22511,  22465,  22423,  22380,  22334,  22292,  22246,  22203,  22157,  22115,
  22069,  22026,  21980,  21938,  21892,  21849,  21803,  21757,  21715,  21669,
  21626,  21581,  21535,  21492,  21446,  21400,  21358,  21312,  21266,  21223,
  21177,  21132,  21086,  21043,  20997,  20951,  20905,  20863,  20817,  20771,
  20725,  20679,  20634,  20591,  20545,  20499,  20453,  20407,  20362,  20316,
  20270,  20227,  20181,  20135,  20090,  20044,  19998,  19952,  19906,  19860,
  19814,  19768,  19723,  19677,  19631,  19585,  19539,  19493,  19447,  19401,
  19356,  19310,  19264,  19218,  19172,  19126,  19080,  19034,  18989,  18943,
  18897,  18848,  18802,  18756,  18710,  18664,  18618,  18572,  18527,  18481,
  18435,  18389,  18343,  18294,  18248,  18202,  18156,  18110,  18064,  18019,
  17973,  17927,  17878,  17832,  17786,  17740,  17694,  17648,  17602,  17557,
  17511,  17462,  17416,  17370,  17324,  17278,  17232,  17186,  17140,  17095,
  17045,  17000,  16954,  16908,  16862,  16816,  16770,  16724,  16678,  16633,
  16587,  16541,  16492,  16446,  16400,  16354,  16308,  16262,  16216,  16171,
  16125,  16079,  16033,  15987,  15941,  15895,  15849,  15804,  15758,  15712,
  15666,  15620,  15574,  15528,  15482,  15437,  15391,  15345,  15299,  15253,
  15207,  15161,  15119,  15073,  15027,  14981,  14935,  14889,  14843,  14798,
  14755,  14709,  14663,  14617,  14571,  14526,  14483,  14437,  14391,  14345,
  14303,  14257,  14211,  14165,  14123,  14077,  14031,  13988,  13942,  13896,
  13854,  13808,  13762,  13719,  13674,  13631,  13585,  13539,  13497,  13451,
  13408,  13362,  13320,  13274,  13231,  13185,  13143,  13097,  13054,  13008,
  12966,  12923,  12877,  12835,  12792,  12746,  12704,  12661,  12615,  12573,
  12530,  12487,  12442,  12399,  12356,  12314,  12268,  12225,  12183,  12140,
  12097,  12055,  12012,  11970,  11927,  11884,  11842,  11799,  11757,  11714,
  11671,  11629,  11586,  11544,  11501,  11458,  11416,  11377,  11334,  11291,
  11249,  11206,  11167,  11124,  11082,  11042,  11000,  10957,  10918,  10875,
  10836,  10793,  10754,  10711,  10672,  10629,  10590,  10548,  10508,  10466,
  10426,  10387,  10344,  10305,  10266,  10223,  10184,  10144,  10105,  10066,
  10023,   9984,   9945,   9905,   9866,   9827,   9787,   9748,   9709,   9669,
  9630,   9591,   9551,   9512,   9476,   9437,   9397,   9358,   9319,   9283,
  9243,   9204,   9168,   9129,   9089,   9053,   9014,   8978,   8939,   8903,
  8863,   8827,   8791,   8752,   8716,   8676,   8640,   8604,   8568,   8529,
  8493,   8457,   8421,   8385,   8349,   8313,   8277,   8241,   8205,   8169,
  8133,   8096,   8060,   8024,   7988,   7956,   7920,   7883,   7847,   7815,
  7779,   7743,   7710,   7674,   7641,   7605,   7572,   7536,   7503,   7471,
  7435,   7402,   7369,   7333,   7300,   7267,   7235,   7202,   7166,   7133,
  7100,   7068,   7035,   7002,   6969,   6940,   6907,   6874,   6841,   6809,
  6776,   6746,   6714,   6681,   6651,   6619,   6589,   6556,   6527,   6494,
  6465,   6432,   6402,   6373,   6340,   6311,   6281,   6252,   6219,   6189,
  6160,   6130,   6101,   6071,   6042,   6012,   5983,   5953,   5927,   5898,
  5868,   5839,   5813,   5783,   5754,   5727,   5698,   5672,   5642,   5616,
  5586,   5560,   5534,   5505,   5478,   5452,   5423,   5396,   5370,   5344,
  5318,   5292,   5265,   5239,   5213,   5187,   5160,   5134,   5111,   5085,
  5059,   5033,   5010,   4984,   4961,   4934,   4911,   4885,   4862,   4836,
  4813,   4790,   4764,   4741,   4718,   4695,   4672,   4649,   4626,   4603,
  4580,   4558,   4535,   4512,   4489,   4466,   4446,   4423,   4400,   4381,
  4358,   4335,   4315,   4292,   4272,   4253,   4230,   4210,   4191,   4168,
  4148,   4128,   4109,   4089,   4069,   4050,   4030,   4010,   3991,   3971,
  3951,   3932,   3915,   3896,   3876,   3860,   3840,   3824,   3804,   3787,
  3768,   3751,   3732,   3715,   3699,   3683,   3663,   3647,   3630,   3614,
  3597,   3581,   3565,   3548,   3532,   3519,   3502,   3486,   3470,   3457,
  3440,   3427,   3411,   3398,   3381,   3368,   3352,   3339,   3325,   3312,
  3296,   3283,   3270,   3257,   3244,   3230,   3217,   3204,   3191,   3178,
  3168,   3155,   3142,   3132,   3119,   3106,   3096,   3083,   3073,   3060,
  3050,   3040,   3027,   3017,   3008,   2998,   2988,   2978,   2968,   2958,
  2949,   2939,   2929,   2919,   2909,   2903,   2893,   2883,   2877,   2867,
  2860,   2850,   2844,   2834,   2827,   2821,   2811,   2804,   2798,   2791,
  2785,   2778,   2772,   2765,   2759,   2752,   2745,   2739,   2736,   2729,
  2723,   2719,   2713,   2706,   2703,   2700,   2693,   2690,   2683,   2680,
  2677,   2673,   2670,   2667,   2660,   2657,   2654,   2654,   2650,   2647,
  2644,   2641,   2641,   2637,   2634,   2634,   2631,   2631,   2627,   2627,
  2624,   2624,   2624,   2624,   2621,   2621,   2621,   2621
};
