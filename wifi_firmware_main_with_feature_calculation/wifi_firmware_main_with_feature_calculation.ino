#include <ESP8266WiFi.h>
#include "Wire.h"   
#include "constants.h"

#define red_led 4
#define green_led 9
#define blue_led 15
#define pi 3.14159
//=============================== Variable Declaration ==============================
// Constant variables store in Program memory uing keyword PROGMEM
#define ACCEL_BUFF_SIZE  500
#define aRes 1024 // Accelerometer resolution for sensitivity
// Post Request data
String db_name = "hubapp3";
#define http_port 80  // HTTP default port
char iu_url[] = "infinite-uptime-1232.appspot.com";

// Generally, you should use "unsigned long" for variables that hold time
unsigned long previous_millis = 0;        // will store last temp was read
unsigned long current_time = 0;
//Iteration variables for testing to be deleted at the final version
uint8_t buffer_index = 0;

// WiFi module Variables
String MAC_ADDR = "";
uint8_t battery = 29;
float feature_arr[6]; // Feature Array

// BMX055 variables
uint8_t Ascale = AFS_2G;  // set accel full scale
uint8_t ACCBW  = 0x08 & ABW_1000Hz;  // Choose bandwidth for accelerometer

int16_t accel_data[3] = {0, 0, 0};  // Stores the 16-bit signed accelerometer sensor output
int16_t accel_x_buff[ACCEL_BUFF_SIZE]; // Accel buffer along X axis
int16_t accel_y_buff[ACCEL_BUFF_SIZE]; // Accel buffer along Y axis
int16_t accel_z_buff[ACCEL_BUFF_SIZE]; // Accel buffer along Z axis
int avg_x = 0, avg_y = 0, avg_z = 0;  // Average value for 1 iteration

//Error Handling
byte i2c_read_error;
bool read_flag = true;
uint8_t error_message = ALL_OK;

// GPS functionality
String gps_data = "";
String lat_str = "", lon_str = "";
char ns_indicator = 'a', ew_indicator = 'a';

bool gps_flag = false;  // To enable or disable the gps functionality
bool wifi_flag = false;
bool get_flag = false; // request type flag -- GET = true & POST = false

//=============================== Function Declaration ==============================
void    initBMX055(); // Initialize the BMX055 IMU
void    initWifi(); // Initiate the WiFi module
int     sendDataToCloud();  // Function to post data to Phant cloud
void    SENtralPassThroughMode(); // Initialize EM7180
void    I2Cscan();  // Scan devices on I2C line
void    readAccelData(int16_t * destination); // Read data from accelerometer
void    calcFeatures();  // Calculate the feature values
void    writeByte(uint8_t address, uint8_t subAddress, uint8_t data);
uint8_t readByte(uint8_t address, uint8_t subAddress);
void    readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest);
//------------------------ Feature value calculation functions --------------------------
float signalEnergy();           // Function to calculate Energy
float velocityCalc(char axis);  // Function to calculate Velocity along required axis
//===================================================================================

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Wire.begin(0,2); // SDA (0), SCL (2) on ESP8285
  Wire.setClock(400000); // choose 400 kHz I2C rate
  delay(10);
  I2Cscan();  // Should detect EM7180 at 0x28
  SENtralPassThroughMode();
  I2Cscan();  // Should detect other devices on I2C
  delay(10);
  // Read the BMX-055 WHO_AM_I registers, this is a good test of communication
  byte c = readByte(BMX055_ACC_ADDRESS, BMX055_ACC_WHOAMI);  // Read ACC WHO_AM_I register for BMX055
  if (c == 0xFA) // WHO_AM_I should always be ACC = 0xFA, GYRO = 0x0F, MAG = 0x32
  {
    // get sensor resolutions, only need to do this once
    initBMX055();
    Serial.println("BMX055 Initialized");
  }
  else
  {
    error_message = ACCEL_ERROR;
  }
  if(wifi_flag)
  {
    initWifi();
    MAC_ADDR = WiFi.macAddress();
    Serial.print("MAC Address: ");
    Serial.println(MAC_ADDR);
  }

  buffer_index = 0;
  previous_millis = millis();
  Serial.print("Current Time = ");
  current_time = getCurrentTime();
  Serial.println(current_time);
}
 

void loop()
{  
  if((previous_millis < millis()) && (error_message == ALL_OK))
  {
    if(gps_flag)
    {
      gpsDataReady();
    }
    //Read data from accelerometer and store in accel buffers
    readAccelData(accel_data);
    accel_x_buff[buffer_index] = accel_data[0];
    accel_y_buff[buffer_index] = accel_data[1];
    accel_z_buff[buffer_index] = accel_data[2];
    avg_x += accel_data[0];
    avg_y += accel_data[1];
    avg_z += accel_data[2];
    buffer_index++;
        
//    Serial.println("Read AccelData");
//    Serial.printf("x = %d, y = %d, z = %d\n", accel_data[0], accel_data[1], accel_data[2]);
//    Serial.println(previous_millis);
    if(buffer_index >= ACCEL_BUFF_SIZE)
    {
      // Calculate feature values and Send data to coud when buffer is full
      avg_x /= ACCEL_BUFF_SIZE;
      avg_y /= ACCEL_BUFF_SIZE;
      avg_z /= ACCEL_BUFF_SIZE;
      
      calcFeatures();
      
      if(sendDataToCloud()) {
        Serial.println("\nData posted to cloud");
        Serial.println(previous_millis);
        Serial.println();
      }
      else {
        Serial.println("Data not posted to cloud");
        Serial.println(previous_millis);
      }
      buffer_index = 0;
      avg_x = 0, avg_y = 0, avg_z = 0;
    }
    previous_millis++;
  }
}

//=====================================================================================================
//================================ User Defined Functions==============================================
void initBMX055()
{
  // start with all sensors in default mode with all registers reset
  writeByte(BMX055_ACC_ADDRESS,  BMX055_ACC_BGW_SOFTRESET, 0xB6);  // reset accelerometer
  delay(1000); // Wait for all registers to reset

  // Configure accelerometer
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_PMU_RANGE, (Ascale & 0x0F)); // Set accelerometer full range
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_PMU_BW, (ACCBW & 0x0F));     // Set accelerometer bandwidth
  writeByte(BMX055_ACC_ADDRESS, BMX055_ACC_D_HBW, 0x00);              // Use filtered data
/* 
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
  // }*/
}

// To calculate Accel Bias
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

void gpsDataReady()
{
  Serial.print("Read GPS Data: ");
  Serial.println(millis());
  
  uint8_t rx_data[32];
  readBytes(CAMM8Q_ADDRESS, CAMM8Q_I2C_BYTES, 2, &rx_data[0]);

  uint16_t scount = ( ((uint16_t) rx_data[0] << 8) | rx_data[1]);
  Serial.println(scount);
  Serial.print("Size of RX_DATA: "); Serial.println(sizeof(rx_data));
  Serial.println(i2c_read_error);
  
  if(scount > 0) {
    gps_data = "";
    //gps_data2 = "";
    while(scount) {
      uint8_t rx_count = ((scount > 32) ? 32 : scount);
      readBytes(CAMM8Q_ADDRESS, CAMM8Q_I2C_DATA,  rx_count, &rx_data[0]);
      
      for(uint8_t ii = 0; ii < rx_count; ii++) 
      {
        char aChar = rx_data[ii];
        if(isValid(aChar))
        {
          gps_data += String(aChar);
        }
//        gps_data2 += String(aChar);
      }
      scount -= rx_count;
    }
      Serial.println(gps_data);
      parseGGA(gps_data);
//    Serial.println("GPS_DATA_2");
//    Serial.println(gps_data2);
//    parseGGA(gps_data2);
    Serial.println();
  }
//  if(digitalRead(gps_int) == HIGH)
//  {
//    Serial.println("GPS pin HIGH");
//  }
//  else
//  {
//    Serial.println("GPS pin LOW");
//  }
}

void initWifi() {
//  const char* ssid     = "SkyDeckWireless";
//  const char* password = "moonshot";
  
//  const char* ssid     = "XianShi";
//  const char* password = "5103326192";

  const char* ssid     = "GANAPATI";
  const char* password = "HP3PARUSERS2015";

//  const char* ssid = "CalVisitor";
//  const char* password = "";
 // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\n\rWorking to connect");

 // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
//  mac = WiFi.macAddress(mac);
  Serial.println("");
  Serial.println("ESP8285 Environmental Data Server");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

unsigned long getCurrentTime() {
  unsigned long current_time = 1234567890;
  // Code to find the current UTC time
  
  return current_time;
}

int sendDataToCloud()
{
  WiFiClient iuclient;  // HTTP Client object
  
  if(iuclient.connect(iu_url, http_port))
  {
    iuclient.flush();
    if(iuclient.connected())
    {
      if(get_flag)  // GET Request code
      {
        String data_string = MAC_ADDR + ",00," + battery;
        data_string += ",0001," + String(feature_arr[0]);
        data_string += ",0002," + String(feature_arr[1]);
        data_string += ",0003," + String(feature_arr[2]);
        data_string += ",0004," + String(feature_arr[3]);
        data_string += ",0005," + String(feature_arr[4]);
        data_string += ",0006," + String(feature_arr[5]);
        data_string += "," + String(current_time + millis()) + ".00";
        
        String get_request = "GET /adddatafromdevice?dbname="+ db_name + "&macaddress=" + MAC_ADDR + "&data=" + data_string+ " HTTP/1.1\r\n";
        iuclient.print(get_request);
        Serial.print(get_request);
        get_request = "Host: " + String(iu_url) +"\r\n";
        iuclient.println(get_request);
        Serial.println(get_request);
      }
      else    // POST Request code
      {
        String post_request = "POST /adddatafromdevice? HTTP/1.1\r\n";
        iuclient.print(post_request);
        Serial.print(post_request);
        post_request = "Host: " + String(iu_url) +"\r\n";
        iuclient.print(post_request);
        Serial.print(post_request);

        String data_string = "dbname="+ db_name + "&macaddress=" + MAC_ADDR + "&data=" + MAC_ADDR + ",00," + battery;
        data_string += ",0001," + String(feature_arr[0]) + ",0002," + String(feature_arr[1]);
        data_string += ",0003," + String(feature_arr[2]) + ",0004," + String(feature_arr[3]);
        data_string += ",0005," + String(feature_arr[4]) + ",0006," + String(feature_arr[5]);
        data_string += "," + String(current_time+millis()) + ".00";

        post_request = "Content-Type: application/x-www-form-urlencoded\r\n";
        iuclient.print(post_request);
        Serial.print(post_request);
        post_request = "Content-Length: " + String(data_string.length()) + "\r\n";
        iuclient.print(post_request);
        Serial.print(post_request);
        iuclient.print("\r\n");
        Serial.print("\r\n");
        
        iuclient.println(data_string);
        Serial.println(data_string);
      }
      while(iuclient.available()) // Read the response from the server
      {
        String response = iuclient.readStringUntil('\r');
        Serial.print("Server Response: ");
        Serial.println(response);
      }
    }
    else
    {
      Serial.print("Client connection closed...");
      iuclient.stop();
    }
  }
  else  {
  Serial.println("Not connected to client");
    return 0;
  }
  return 1;
}

void SENtralPassThroughMode()
{
  // First put SENtral in standby mode
  uint8_t c = readByte(EM7180_ADDRESS, EM7180_AlgorithmControl);
  writeByte(EM7180_ADDRESS, EM7180_AlgorithmControl, c | 0x01);
  // Verify standby status
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
 
// simple function to scan for I2C devices on the bus
void I2Cscan() 
{
    // scan for i2c devices
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for(address = 1; address < 127; address++ ) 
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

// BMX055 Axes data reading function
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

void calcFeatures()
{
  Serial.println("calculate feature values");
  feature_arr[0] = signalEnergy();    // Signal Energy
  feature_arr[1] = velocityCalc('X'); // Velocity along X
  feature_arr[2] = velocityCalc('Y'); // Velocity along Y
  feature_arr[3] = velocityCalc('Z'); // velocity along Z
  // GPS values need to be parsed
  feature_arr[4] = 1234.56;           // Lattitude
  feature_arr[5] = 4567.89;           // Longitude
}
// I2C read/write functions for the BMP280 sensors
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
  Wire.write(subAddress);                  // Put slave register address in Tx buffer
//  Wire.endTransmission(I2C_NOSTOP);        // Send the Tx buffer, but send a restart to keep connection alive
  Wire.endTransmission(false);             // Send the Tx buffer, but send a restart to keep connection alive
  Wire.requestFrom(address, 1);  // Read one byte from slave register address 
//  Wire.requestFrom(address, (size_t) 1);   // Read one byte from slave register address 
  data = Wire.read();                      // Fill Rx buffer with result
  return data;                             // Return data read from slave register
}

void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest)
{  
  Wire.beginTransmission(address);   // Initialize the Tx buffer
  Wire.write(subAddress);            // Put slave register address in Tx buffer
//  Wire.endTransmission(I2C_NOSTOP);  // Send the Tx buffer, but send a restart to keep connection alive
  Wire.endTransmission(false);       // Send the Tx buffer, but send a restart to keep connection alive
  uint8_t i = 0;
  Wire.requestFrom(address, count);  // Read bytes from slave register address 
//        Wire.requestFrom(address, (size_t) count);  // Read bytes from slave register address 
  while (Wire.available()) {
        dest[i++] = Wire.read(); }         // Put read results in the Rx buffer
}

int isValid(char c)
{
  if((c >= 0x20 && c <= 0x7D) || c == '\n')
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void parseGGA(String str_data)
{
  int str_len = str_data.length();
  int gga_index = 0;
  for(int i = 0; i < str_len; i++)
  {
    if(str_data[i] == 'G' && str_data[i+1] == 'G' && str_data[i+2] == 'A')
    {
      gga_index = i;
      i = str_len;
    }
  }
  lat_str = str_data.substring(gga_index+14, gga_index+23);
  lon_str = str_data.substring(gga_index+27, gga_index+37);
  ns_indicator = str_data[gga_index+25];
  ew_indicator = str_data[gga_index+39];

  String temp = str_data.substring(gga_index, gga_index+70);
  Serial.print("Lats: ");
  Serial.println(lat_str);
  Serial.print("Long: ");
  Serial.println(lon_str);
  Serial.printf("N/S: %c\n",ns_indicator);
  Serial.printf("E/W: %c\n",ew_indicator);
  Serial.print("GGA_STR = ");
  Serial.println(temp);
  Serial.println("ZAB");
}
//-----------------Change LED colors----------------------------//
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
  switch (color) {
    case RED_BAD:
      ledOn(red_led);    // R = 1
      ledOff(green_led); // G = 0
      ledOff(blue_led);  // B = 0
      break;
    case BLUE_NOOP:
      ledOff(red_led);   // R = 0
      ledOff(green_led); // G = 0
      ledOn(blue_led);   // B = 1
      break;
    case GREEN_OK:
      ledOff(red_led);   // R = 0
      ledOn(green_led);  // G = 1
      ledOff(blue_led);  // B = 0
      break;
    case ORANGE_WARNING:
      ledOn(red_led);    // R = 1
      ledOn(green_led);  // G = 1
      ledOff(blue_led);  // B = 0
      break;
    case PURPLE_CHARGE:
      ledOn(red_led);    // R = 1
      ledOff(green_led); // G = 0
      ledOn(blue_led);   // B = 1
      break;
    case CYAN_DATA:
      ledOff(red_led);   // R = 0
      ledOn(green_led);  // G = 1
      ledOn(blue_led);   // B = 1
      break;
    case WHITE_NONE:
      ledOn(red_led);    // R = 1
      ledOn(green_led);  // G = 1
      ledOn(blue_led);   // B = 1
      break;
    case SLEEP_MODE:
      ledOff(red_led);   // R = 0
      ledOff(green_led); // G = 0
      ledOff(blue_led);  // B = 0
      break;
  }
}

float signalEnergy()
{
  float ene = 0;
  
  for (int i = 0; i < ACCEL_BUFF_SIZE; i++) {
    ene += sq(LSB_to_ms2(accel_x_buff[i] - (int16_t)avg_x))
           +  sq(LSB_to_ms2(accel_y_buff[i] - (int16_t)avg_y))
           +  sq(LSB_to_ms2(accel_z_buff[i] - (int16_t)avg_z));
  }
  Serial.print("Energy = ");
  Serial.println(ene);
  return ene;
}

float velocityCalc(char axis)
{
  int16_t *buff;
  uint16_t k = 0, n = 0, max_index = 1;
  float max_val = 0;
  float accelRMS = 0.0, accelAVG_2 = 0.0, accelAVG = 0.0;

  switch(axis)
  {
    case 'X': buff = accel_x_buff; break;
    case 'Y': buff = accel_y_buff; break;
    case 'Z': buff = accel_z_buff; break;
  }
  Serial.println("Velocity Calculation begins for " + String(axis) + "-axis");
  
  for(k = 0; k < ACCEL_BUFF_SIZE; k++)
  {
    // Calculation of fft max frequency index
    float X_k = 0;
    for(n = 0; n < ACCEL_BUFF_SIZE; n++)
    {
      X_k += (buff[n] * cos(2 * pi * k * n / ACCEL_BUFF_SIZE)); // Real-FFT calculation
    }
    if(X_k > max_val && k > 1)
    {
      max_val = X_k;
      max_index = k;
    }
  }
  
  for(k = 0; k < ACCEL_BUFF_SIZE; k++)
  {
    // Calculation of velocity
    accelAVG += LSB_to_ms2(buff[k]);
  }
  for(k = 0; k < ACCEL_BUFF_SIZE; k++)
  {
    // Calculation of velocity
    accelAVG_2 += sq(LSB_to_ms2(buff[k]));
  }
  accelRMS = sqrt(accelAVG_2 / ACCEL_BUFF_SIZE - sq(accelAVG / ACCEL_BUFF_SIZE));
  //    Serial.printf("Accel_RMS: %f\n",accelRMS);
 
  float vel_rms = accelRMS * 1000 / (2 * pi * max_index); // accelRMS * freq /(2 pi f)
  Serial.print("Vel_RMS = ");
  Serial.println(vel_rms);
  Serial.println(max_index);
  return vel_rms;  
}

float LSB_to_ms2(int16_t accelLSB)
{
  return ((float)accelLSB / aRes * 9.8);
}
