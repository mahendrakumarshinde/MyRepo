//#include "Wire.h"
#include <i2c_t3.h>
#include <SPI.h>

/* I2S digital audio */
#include <i2s.h>

// Accelerometer registers
#define BMX055_ACC_WHOAMI        0x00   // should return 0xFA
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
#define BMX055_ACC_FIFO_STATUS   0x0E
#define BMX055_ACC_PMU_RANGE     0x0F
#define BMX055_ACC_PMU_BW        0x10
#define BMX055_ACC_PMU_LPW       0x11
#define BMX055_ACC_PMU_LOW_POWER 0x12
#define BMX055_ACC_D_HBW         0x13
#define BMX055_ACC_BGW_SOFTRESET 0x14
#define BMX055_ACC_INT_EN_0      0x16
#define BMX055_ACC_INT_EN_1      0x17
#define BMX055_ACC_INT_EN_2      0x18
#define BMX055_ACC_INT_MAP_0     0x19
#define BMX055_ACC_INT_MAP_1     0x1A
#define BMX055_ACC_INT_MAP_2     0x1B
#define BMX055_ACC_INT_SRC       0x1E
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
#define BMX055_ACC_PMU_SELF_TEST 0x32
#define BMX055_ACC_TRIM_NVM_CTRL 0x33
#define BMX055_ACC_BGW_SPI3_WDT  0x34
#define BMX055_ACC_OFC_CTRL      0x36
#define BMX055_ACC_OFC_SETTING   0x37
#define BMX055_ACC_OFC_OFFSET_X  0x38
#define BMX055_ACC_OFC_OFFSET_Y  0x39
#define BMX055_ACC_OFC_OFFSET_Z  0x3A
#define BMX055_ACC_TRIM_GPO      0x3B
#define BMX055_ACC_TRIM_GP1      0x3C
#define BMX055_ACC_FIFO_CONFIG_1 0x3E
#define BMX055_ACC_FIFO_DATA     0x3F

#define EM7180_AlgorithmStatus    0x38
#define EM7180_AlgorithmControl   0x54
#define EM7180_PassThruStatus     0x9E
#define EM7180_PassThruControl    0xA0

#define BMX055_ACC_ADDRESS  0x18   // Address of BMX055 accelerometer
//#define MS5637_ADDRESS      0x76   // Address of MS5637 altimeter
#define EM7180_ADDRESS      0x28   // Address of the EM7180 SENtral sensor hub
#define M24512DFM_DATA_ADDRESS   0x50   // Address of the 500 page M24512DFM EEPROM data buffer, 1024 bits (128 8-bit bytes) per page
#define M24512DFM_IDPAGE_ADDRESS 0x58   // Address of the single M24512DFM lockable EEPROM ID page

#define SerialDebug true  // set to true to get Serial output for debugging

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

// Specify sensor full scale
uint8_t Ascale = AFS_2G;           // set accel full scale
uint8_t ACCBW  = 0x08 & ABW_1000Hz;  // Choose bandwidth for accelerometer

// Pin definitions
int myLed     = 13;  // LED on the Teensy 3.1

// BMX055 variables
int16_t accelCount[3];  // Stores the 16-bit signed accelerometer sensor output
float accelBias[3] = {0, 0, 0}; // Bias corrections for gyro, accelerometer, mag


float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest sensor data values
float aRes; //resolution of accelerometer
int count;
int delt_t;

long init_time;

bool passThru = true;

int datai = 0;
long last_wakeup;
long this_wakeup;
long dt;

// set 48kHz sampling rate
#define CLOCK_TYPE                  (I2S_CLOCK_48K_INTERNAL)

// allocate  data buffer
#define bufferSize 8000*3
const uint16_t target_sample = 8000;
const uint16_t inter = 48000/target_sample;
uint32_t subsample_counter = 0;
uint32_t accel_counter = 0;
unsigned char audio_buffer[bufferSize];
uint32_t nTX = 0;
uint32_t nRX = 0;
const uint32_t intvl = 100;
uint32_t totCount = 0;
uint32_t localCount = 0;
//unit32_t nStep = 10;
float accel_x[intvl];
float accel_y[intvl];
float accel_z[intvl];
float thres = 100;
float thres2 = 750;
float thres3 = 1500;
float ave_x=0;
float ave_y=0;
float ave_z=0;
float ene=0;
float state=0;
float stateprev=0;
boolean over = false;

boolean silent = true;
unsigned char bytes[4];

// extract the 24bit INMP441 audio data from 32bit sample
void extractdata_inplace(int32_t  *pBuf) {
  // set highest bit to zero, then bitshift right 7 times
  // do not omit the first part (!)
  pBuf[0] = (pBuf[0] & 0x7fffffff) >>7;
}


/* --------------------- Direct I2S Receive, we get callback to read 2 words from the FIFO ----- */

void i2s_rx_callback( int32_t *pBuf )
{
  // Downsampling routine; only take every 6th sample.
  if (subsample_counter != inter-2){
    subsample_counter ++;
    return;
  } else{
    subsample_counter = 0;
    accel_counter++;
  }
  
  // perform the data extraction for single channel
  extractdata_inplace(&pBuf[0]);

  if (accel_counter == 8){
    readAccelData(accelCount);
    
    ax = (float)accelCount[0] * aRes + accelBias[0]; 
    ay = (float)accelCount[1] * aRes + accelBias[1];
    az = (float)accelCount[2] * aRes + accelBias[2];

    byte* axb = (byte *) &ax;
    byte* ayb = (byte *) &ay;
    byte* azb = (byte *) &az;
    accel_counter = 0;

    // Feature Calculation
    
    accel_x[localCount] = ax;
    accel_y[localCount] = ay;
    accel_z[localCount] = az;
    totCount = totCount + 1;
    localCount = localCount + 1;
    localCount = localCount%intvl;
    if(totCount > intvl){
      totCount = intvl + 1;
      over = true;
    }
    if (over){
      // Calculate the mean only if we have at least 100 data points
      ave_x=0;
      ave_y=0;
      ave_z=0;
      for (int i=0;i<intvl;i++){
        ave_x = ave_x + accel_x[i];
        ave_y = ave_y + accel_y[i];
        ave_z = ave_z  + accel_z[i];
      }
      ave_x = ave_x/intvl;
      ave_y = ave_y/intvl;
      ave_z = ave_z/intvl;
      ene=0; //initialize the energy feature
      for (int i=0;i<intvl;i++){ 
        ene = ene + (accel_x[i]-ave_x)*(accel_x[i]-ave_x) + (accel_y[i]-ave_y)*(accel_y[i]-ave_y) + (accel_z[i]-ave_z)*(accel_z[i]-ave_z);
      }
      ene = ene*intvl;
      Serial.println(ene);

      if(ene < thres){
        // blue: normal cutting
        digitalWrite(22, LOW);
        digitalWrite(21, HIGH);
        digitalWrite(20, LOW);
        state=0;
      }
      else if(ene < thres2){
        // green: normal cutting
        digitalWrite(22, HIGH);
        digitalWrite(21, HIGH);
        digitalWrite(20, LOW);
        state=1;
      }
      else if(ene < thres3){
        // yellow: warning
        digitalWrite(22, HIGH);
        digitalWrite(21, LOW);
        digitalWrite(20, LOW);
        state=2;
      }
      else{
        // red: bad cutting
        digitalWrite(22, HIGH);
        digitalWrite(21, LOW);
        digitalWrite(20, HIGH);
        state=3;
      }

      
      if (state==stateprev)
      {        
      }
      else
      {
        if (state==0){
          Serial2.print("\nIdle!\n");
        }
        if (state==1){
          Serial2.print("\nNormal Cutting\n");
        }
        if (state==2){
          Serial2.print("\nWarning!\n");
        }
        if (state==3){
          Serial2.print("\nDanger!\n");
        }
        stateprev=state;
      }
      
    }
      
  }
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
  Serial2.begin(9600);
  //Serial.println("in da loop");

  delay(1000);
  I2Cscan(); // should detect SENtral at 0x28
  
  // Set up the SENtral as sensor bus in normal operating mode

  // ld pass thru
  // Put EM7180 SENtral into pass-through mode
  SENtralPassThroughMode();

  I2Cscan(); // should see all the devices on the I2C bus including two from the EEPROM (ID page and data pages)
  // LED G=20, R=21, B=22
  pinMode(20, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(22, OUTPUT);
  // Set up the interrupt pin, its set as active high, push-pull
  pinMode(myLed, OUTPUT);
  // NOTE: I have no idea what this does, but turning this on basically kills BLE. disabled for now.
  //digitalWrite(myLed, HIGH);

  // Read the BMX-055 WHO_AM_I registers, this is a good test of communication
  //Serial.println("BMX055 accelerometer...");
  byte c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_WHOAMI);  // Read ACC WHO_AM_I register for BMX055
  //Serial.print("BMX055 ACC"); Serial.print(" I AM 0x"); Serial.print(c, HEX); Serial.print(" I should be 0x"); Serial.println(0xFA, HEX);

  if (c == 0xFA) // WHO_AM_I should always be ACC = 0xFA, GYRO = 0x0F, MAG = 0x32
  {
    //Serial.println("BMX055 is online...");

    initBMX055();
    //Serial.println("BMX055 initialized for active data mode...."); // Initialize device for active mode read of acclerometer, gyroscope, and temperature

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
  
  // fill the buffer with something to see if the RX callback is activated at all
  audio_buffer[0] = 0x42424242;
  
  delay(5000);
  // start the I2S RX
  I2SRx0.start();
  //I2STx0.start();
  if(!silent) Serial.println( "Started I2S RX" );
}

void loop()
{

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
  Serial.println("SENtral in standby mode");
  // Place SENtral in pass-through mode
  writeByte(EM7180_ADDRESS, EM7180_PassThruControl, 0x01);
  if (readByte(EM7180_ADDRESS, EM7180_PassThruStatus) & 0x01) {
    Serial.println("SENtral in pass-through mode");
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

// simple function to scan for I2C devices on the bus
void I2Cscan()
{
  // scan for i2c devices
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

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
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      Serial.print("Unknow error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
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
