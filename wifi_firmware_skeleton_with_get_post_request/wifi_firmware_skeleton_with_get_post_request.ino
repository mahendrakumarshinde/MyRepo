/* 
 by: Zankar Bapat
 date: Oct 12, 2016
 
 This sketch uses SDA/SCL on pins 0/2, respectively, and it uses the ESP8285.
 SDA and SCL should have 4K7 pull-up resistors (to 3.3V).
 
 Hardware setup:
 SDA ----------------------- 0
 SCL ----------------------- 2
*/
#include <ESP8266WiFi.h>
#include "Wire.h"   
#include "constants.h"

#define myLed 15

//=============================== Variable Declaration ==============================
String webString="";     // String to display
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previous_millis = 0;        // will store last temp was read
const long interval = 15000;              // interval at which to read sensor
unsigned long current_time = 0;

String MAC_ADDR = "AB:CD:EF:01:23:11";
uint8_t battery = 29;
float feature_arr[6]; // Feature Array
//Iteration variables for testing to be deleted at the final version
uint8_t count = 0;
uint8_t no_of_iterations = 1;

// MPU9250 variables
uint8_t Ascale = AFS_2G;  // set accel full scale
int16_t accel_data[3] = {0, 0, 0};  // Stores the 16-bit signed accelerometer sensor output

//Error Handling
byte i2c_read_error;
bool read_flag = true;
String error_message = "ALL_OK";


// Post Request data
String db_name = "hubapp3";
const int http_port = 80;  // HTTP default port
const char iu_url[] = "infinite-uptime-1232.appspot.com";

// GPS functionality
String gps_data = "",gps_data2 = "";
String lat_str = "", lon_str = "";
char ns_indicator = 'a', ew_indicator = 'a';

bool gps_flag = false;  // To enable or disable the gps functionality
bool wifi_flag = true;
bool get_flag = false; // request type flag -- GET = true & POST = false

//=============================== Function Declaration ==============================
void    initMPU9250(); // Initialize the MPU9250 IMU
void    initWifi(); // Initiate the WiFi module
int     sendDataToCloud();  // Function to post data to Phant cloud
void    SENtralPassThroughMode(); // Initialize EM7180
void    I2Cscan();  // Scan devices on I2C line
void    readAccelData(int16_t * destination); // Read data from accelerometer
void    calcFeatures();  // Calculate the feature values
void    writeByte(uint8_t address, uint8_t subAddress, uint8_t data);
uint8_t readByte(uint8_t address, uint8_t subAddress);
void    readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest);
//===================================================================================

void setup()
{
  Serial.begin(115200);
  delay(2000);

  pinMode(myLed, OUTPUT);
  digitalWrite(myLed, HIGH);
  
  Wire.begin(0,2); // SDA (0), SCL (2) on ESP8285
  Wire.setClock(400000); // choose 400 kHz I2C rate

  I2Cscan();  // Should detect EM7180 at 0x28
  SENtralPassThroughMode();
  I2Cscan();  // Should detect other devices on I2C

  byte c = readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250); // Read WHO_AM_I register on MPU
  if (c == 0x71) { //WHO_AM_I should always be 0x71 for the entire chip
    initMPU9250();  // Initialize MPU9250 Accelerometer sensor
    //Serial.println("MPU9250 initialized successfully");
  }
  else
  {
    Serial.print("Could not connect to MPU9250: 0x");
    Serial.println(c, HEX);
    error_message = "MPUERR";
    //while (1) ; // Loop forever if communication doesn't happen
  }
  
  if(wifi_flag)
  {
    initWifi();
  //  MAC_ADDR = WiFi.macAddress();
    Serial.print("MAC Address: ");
    Serial.println(MAC_ADDR);
  }

  count = 0;

  Serial.print("Current Time = ");
  current_time = getCurrentTime();
  Serial.println(current_time);
}
 

void loop()
{  
  if((previous_millis + interval < millis()) && (count < no_of_iterations))
  {
    if(gps_flag)
    {
      gpsDataReady();
    }
    if(wifi_flag)
    {
      Serial.println("Posting Data");
      if(error_message.equals("ALL_OK"))
      {
        readAccelData(accel_data); // Reading Accel Data
        calcFeatures();
      }
      
      if(sendDataToCloud())
      {
        previous_millis = millis();
      }
      else
      {
        Serial.println("Data not posted to cloud");
      }
    }
    Serial.println(count);
    count++;
  }
  if(count == no_of_iterations)
  {
    if(wifi_flag)
    {
      WiFi.disconnect();
      Serial.println("WiFi disconnected");
    }
    count++;
  }
}

//=====================================================================================================
//================================ User Defined Functions==============================================
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
  Serial.print("MPU-9250 initialized successfully.\n");
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
  
  if(scount > 0 && scount < 1000) {
    gps_data = "";
    gps_data2 = "";
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
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
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

void readAccelData(int16_t * destination)
{
  uint8_t raw_data[6];  // x/y/z accel register data stored here
  readBytes(MPU9250_ADDRESS, ACCEL_XOUT_H, 6, &raw_data[0]);  // Read the six raw data registers into data array
  destination[0] = ((int16_t)raw_data[0] << 8) | raw_data[1] ;  // Turn the MSB and LSB into a signed 16-bit value
  destination[1] = ((int16_t)raw_data[2] << 8) | raw_data[3] ;
  destination[2] = ((int16_t)raw_data[4] << 8) | raw_data[5] ;
  //Serial.printf("Accel data read: %d, %d, %d", destination[0], destination[1], destination[2]);
}

void calcFeatures()
{
  feature_arr[0] = (float)accel_data[0]; // Raw Accel Data along X
  feature_arr[1] = (float)accel_data[1]; // Raw Accel Data along Y
  feature_arr[2] = (float)accel_data[2]; // Raw Accel Data along Z
  feature_arr[3] = (float)accel_data[0] * 9.8 / 16384;  // Accel in m/s2 along X
  feature_arr[4] = (float)accel_data[1] * 9.8 / 16384;  // Accel in m/s2 along Y
  feature_arr[5] = (float)accel_data[2] * 9.8 / 16384;  // Accel in m/s2 along Z
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
  if(c >= 0x20 && c <= 0x7D)
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

