//======================= Register Addresses and Enums =========================
#define CAMM8Q_ADDRESS         0x42  // I2C address of UBLOX CAM M8Q GPS Module
#define CAMM8Q_I2C_BYTES       0xFD   // Address of CAM M8Q number of data bytes ready register, two bytes
#define CAMM8Q_I2C_DATA        0xFF   // Address of CAM M8Q I2C data buffer

// BMX055 data sheet http://ae-bst.resource.bosch.com/media/products/dokumente/bmx055/BST-BMX055-DS000-01v2.pdf
// The BMX055 is a conglomeration of three separate motion sensors packaged together but
// addressed and communicated with separately by design
// Seven-bit BMX055 device addresses are ACC = 0x18, GYRO = 0x68, MAG = 0x10
#define BMX055_ACC_ADDRESS      0x18   // Address of BMX055 accelerometer
#define BMX055_GYRO_ADDRESS     0x68   // Address of BMX055 gyroscope
#define BMX055_MAG_ADDRESS      0x10   // Address of BMX055 magnetometer

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
#define BMX055_GYRO_WHOAMI       0x00  // should return 0x0F
//#define BMX055_GYRO_Reserved    0x01
#define BMX055_GYRO_RATE_X_LSB   0x02
#define BMX055_GYRO_RATE_X_MSB   0x03
#define BMX055_GYRO_RATE_Y_LSB   0x04
#define BMX055_GYRO_RATE_Y_MSB   0x05
#define BMX055_GYRO_RATE_Z_LSB   0x06
#define BMX055_GYRO_RATE_Z_MSB   0x07
//#define BMX055_GYRO_Reserved    0x08
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


// See MS5637-02BA03 Low Voltage Barometric Pressure Sensor Data Sheet http://www.meas-spec.com/downloads/MS5637-02BA03.pdf
#define MS5637_RESET      0x1E
#define MS5637_CONVERT_D1 0x40
#define MS5637_CONVERT_D2 0x50
#define MS5637_ADC_READ   0x00

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


#define MS5637_ADDRESS      0x76   // Address of MS5637 altimeter
#define EM7180_ADDRESS      0x28   // Address of the EM7180 SENtral sensor hub
#define M24512DFM_DATA_ADDRESS   0x50   // Address of the 500 page M24512DFM EEPROM data buffer, 1024 bits (128 8-bit bytes) per page
#define M24512DFM_IDPAGE_ADDRESS 0x58   // Address of the single M24512DFM lockable EEPROM ID page

// MS5637 pressure sensor sample rates
#define ADC_256  0x00 // define pressure and temperature conversion rates
#define ADC_512  0x02
#define ADC_1024 0x04
#define ADC_2048 0x06
#define ADC_4096 0x08
#define ADC_8192 0x0A
#define ADC_D1   0x40
#define ADC_D2   0x50

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

enum ErrorCodes {
  ALL_OK = 0,
  ACCEL_ERROR,
  I2C_ERROR
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

