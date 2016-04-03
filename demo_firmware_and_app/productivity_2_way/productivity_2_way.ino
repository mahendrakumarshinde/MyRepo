//include communication modules//
#include <i2c_t3.h>
#include <SPI.h>

/* I2S digital audio  */
#include <i2s.h>

// Accelerometer registers
#define BMX055_ACC_WHOAMI        0x00   // should return 0xFA
#define BMX055_ACC_D_X_LSB       0x02
#define BMX055_ACC_D_X_MSB       0x03
#define BMX055_ACC_D_Y_LSB       0x04
#define BMX055_ACC_D_Y_MSB       0x05
#define BMX055_ACC_D_Z_LSB       0x06
#define BMX055_ACC_D_Z_MSB       0x07
#define BMX055_ACC_PMU_RANGE     0x0F
#define BMX055_ACC_PMU_BW        0x10
#define BMX055_ACC_D_HBW         0x13
#define BMX055_ACC_BGW_SOFTRESET 0x14
#define BMX055_ACC_OFC_CTRL      0x36
#define BMX055_ACC_OFC_SETTING   0x37
#define BMX055_ACC_OFC_OFFSET_X  0x38
#define BMX055_ACC_OFC_OFFSET_Y  0x39
#define BMX055_ACC_OFC_OFFSET_Z  0x3A

//Sensor Hub registers
#define EM7180_AlgorithmControl   0x54
#define EM7180_PassThruStatus     0x9E
#define EM7180_PassThruControl    0xA0

#define BMX055_ACC_ADDRESS  0x18   // Address of BMX055 accelerometer
#define EM7180_ADDRESS      0x28   // Address of the EM7180 SENtral sensor hub

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

// Specify sensor full scale
uint8_t Ascale = AFS_2G;           // set accel full scale
uint8_t ACCBW  = 0x08 & ABW_1000Hz;  // Choose bandwidth for accelerometer

// BMX055 variables
int16_t accelCount[3];  // Stores the 16-bit signed accelerometer sensor output
float accelBias[3] = {0, 0, 0}; // Bias corrections for gyro, accelerometer, mag

float ax, ay, az; // variables to hold latest sensor data values
float aRes; //resolution of accelerometer

char bleBuffer[13] = "iuhqiuhqiuhq"; //reading bluetooth data buffer initialization
uint8_t bleBufferIndex = 0; //reading bluetooth data buffer index initialization

// set 48kHz sampling rate
#define CLOCK_TYPE                  (I2S_CLOCK_48K_INTERNAL)

// allocate  data buffer
#define bufferSize 8000*3
const uint16_t target_sample = 8000;
const uint16_t inter = 48000 / target_sample;
uint32_t subsample_counter = 0;
uint32_t accel_counter = 0;
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
int led20State = HIGH; //boot color
int led21State = HIGH; //boot color
int led22State = HIGH; //boot color
int bluestart = 0; //timer to know how long it has been idle
int blue = 0; //toggle to know how if idle
int millisnow = 0;
int bluesleep = 5000; //threshold for sleep mode
int sleepmode = 0; //initialize to not in sleep mode
int currentmillis = 0; //battery monitoring
int prevmillis = 0; //battery monitoring
int batlimit = 1000; //battery monitoring
int overallcounter = 0;
int newThres = 0;
int newThres2 = 0;
int newThres3 = 0;
int args_assigned = 0;
int bat = 0;
boolean silent = true;
unsigned char bytes[4];
String  MAC_ADDRESS = "68:9E:19:07:DE:98";

// extract the 24bit INMP441 audio data from 32bit sample
void extractdata_inplace(int32_t  *pBuf) {
  // set highest bit to zero, then bitshift right 7 times
  // do not omit the first part (!)
  pBuf[0] = (pBuf[0] & 0x7fffffff) >> 7;
}


/* --------------------- Direct I2S Receive, we get callback to read 2 words from the FIFO ----- */

void i2s_rx_callback( int32_t *pBuf )
{
  // Downsampling routine; only take every 6th sample.
  if (subsample_counter != inter - 2) {
    subsample_counter ++;
    return;
  } else {
    subsample_counter = 0;
    accel_counter++;
  }

  // perform the audio data extraction for single channel
  extractdata_inplace(&pBuf[0]);

  if (accel_counter == 8) {
    //Perform accelerometer data extraction
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
    localCount = localCount % intvl;
    if (totCount > intvl) {
      totCount = intvl + 1;
      over = true;
    }
    if (over) {
      // Calculate the mean only if we have at least 100 data points
      ave_x = 0;
      ave_y = 0;
      ave_z = 0;
      for (int i = 0; i < intvl; i++) {
        ave_x = ave_x + accel_x[i];
        ave_y = ave_y + accel_y[i];
        ave_z = ave_z  + accel_z[i];
      }
      ave_x = ave_x / intvl;
      ave_y = ave_y / intvl;
      ave_z = ave_z / intvl;
      ene = 0; //initialize the energy feature
      for (int i = 0; i < intvl; i++) {
        ene = ene + (accel_x[i] - ave_x) * (accel_x[i] - ave_x) + (accel_y[i] - ave_y) * (accel_y[i] - ave_y) + (accel_z[i] - ave_z) * (accel_z[i] - ave_z);
      }
      ene = ene * intvl; //Signal energy integrated

      if (ene < thres) {

        // blue: normal cutting

        if (blue == 0) {
          bluestart = millis();
          blue = 1;
        }
        if (blue == 1) {
          millisnow = millis();
          if (millisnow - bluestart >= bluesleep) { //sleep mode
            sleepmode = 1;
            led22State = HIGH;
            led21State = HIGH;
            led20State = HIGH;
            digitalWrite(22, led22State);
            digitalWrite(21, led21State);
            digitalWrite(20, led20State);
            blue = 0;
          }
        }

        if (sleepmode == 0) {
          led22State = LOW;
          led21State = HIGH;
          led20State = LOW;
        }
        digitalWrite(22, led22State);
        digitalWrite(21, led21State);
        digitalWrite(20, led20State);
        state = 0;
      }
      else if (ene < thres2) {
        blue = 0;
        sleepmode = 0;
        // green: normal cutting

        led22State = HIGH;
        led21State = HIGH;
        led20State = LOW;
        digitalWrite(22, led22State);
        digitalWrite(21, led21State);
        digitalWrite(20, led20State);
        state = 1;
      }
      else if (ene < thres3) {
        blue = 0;
        sleepmode = 0;
        // yellow: warning

        led22State = HIGH;
        led21State = LOW;
        led20State = LOW;

        digitalWrite(22, led22State);
        digitalWrite(21, led21State);
        digitalWrite(20, led20State);
        state = 2;
      }
      else {
        // red: bad cutting
        blue = 0;
        sleepmode = 0;
        led22State = HIGH;
        led21State = LOW;
        led20State = HIGH;
        digitalWrite(22, led22State);
        digitalWrite(21, led21State);
        digitalWrite(20, led20State);
        state = 3;
      }


      if (state == stateprev)
      {
      }
      else
      {
        bat = getInputVoltage();
        if (state == 0) {
          Serial2.print(MAC_ADDRESS);
          Serial2.print(",00,");
          Serial2.print(bat);
          Serial2.print(",0001,");
          Serial2.print(ene);
          Serial2.print(";");
        }
        if (state == 1) {
          Serial2.print(MAC_ADDRESS);
          Serial2.print(",01,");
          Serial2.print(bat);
          Serial2.print(",0001,");
          Serial2.print(ene);
          Serial2.print(";");
        }
        if (state == 2) {
          Serial2.print(MAC_ADDRESS);
          Serial2.print(",02,");
          Serial2.print(bat);
          Serial2.print(",0001,");
          Serial2.print(ene);
          Serial2.print(";");
        }
        if (state == 3) {
          Serial2.print(MAC_ADDRESS);
          Serial2.print(",03,");
          Serial2.print(bat);
          Serial2.print(",0001,");
          Serial2.print(ene);
          Serial2.print(";");
        }
        stateprev = state;
      }

    }
  }

}

/* ----------------------- begin -------------------- */

void setup()
{
  // Setup for Master mode, pins 18/19, external pullups, 400kHz for Teensy 3.1
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  delay(1000);
  Serial.begin(115200);

  delay(1000);
  I2Cscan(); // should detect SENtral at 0x28

  // Put EM7180 SENtral into pass-through mode
  SENtralPassThroughMode();

  I2Cscan(); // should see all the devices on the I2C bus including two from the EEPROM (ID page and data pages)

  // LED G=20, R=21, B=22
  pinMode(20, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(22, OUTPUT);

  byte c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_WHOAMI);  // Read ACC WHO_AM_I register for BMX055

  if (c == 0xFA) // WHO_AM_I should always be ACC = 0xFA, GYRO = 0x0F, MAG = 0x32
  {
    initBMX055();

    getAres(); // get sensor resolutions, only need to do this once

    fastcompaccelBMX055(accelBias);
  }
  else
  {
    while (1) ; // Loop forever if communication doesn't happen
  }

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

  // fill the buffer with something to see if the RX callback is activated at all
  audio_buffer[0] = 0x42424242;

  delay(5000);

  // start the I2S RX
  I2SRx0.start();
  if (!silent) Serial.println( "Started I2S RX" );

  //For Battery Monitoring//
  analogReference(EXTERNAL);
  analogReadResolution(12);
  analogReadAveraging(32);

  Serial2.begin(9600); //This has to be at the end of the Setup to enable two way communication!
}

void loop()
{
  int whichThres = 0;
  bool didThresChange = false;
  while (Serial2.available() > 0) {
    bleBuffer[bleBufferIndex] = Serial2.read();
    //Serial.print(char(bleBuffer[bleBufferIndex])); //Do not comment - good for debugging
    bleBufferIndex++;
  }

  if (bleBufferIndex > 12) {
    args_assigned = sscanf(bleBuffer, "%d-%d-%d", &newThres, &newThres2, &newThres3);
    didThresChange = true;
    bleBufferIndex = 0;
  }
  if (didThresChange) {
    didThresChange = false;
    Serial.print("\nNew low threshold =");
    Serial.print(newThres);
    Serial.print("\n");
    Serial.print("\nNew med threshold =");
    Serial.print(newThres2);
    Serial.print("\n");
    Serial.print("\nNew high threshold =");
    Serial.print(newThres3);
    Serial.print("\n");
    thres = newThres;
    thres2 = newThres2;
    thres3 = newThres3;
    newThres = 0;
    newThres2 = 0;
    newThres3 = 0;
  }

  //Battery status
//  currentmillis = millis();
//  if (currentmillis - prevmillis >= batlimit) {
//    prevmillis = currentmillis;
//    int bat;
//    bat = getInputVoltage();
//  }
}

//Battery Status calculation function
int getInputVoltage() {
  float x = float(analogRead(39))/float(1000);
//    Serial2.print("\nx=");
//    Serial2.print(x,3);
  return int(-261.7 * x  + 484.06);
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
