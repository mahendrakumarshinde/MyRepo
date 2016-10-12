/* 
 by: Zankar Bapat
 date: Oct 12, 2016
 
 This sketch uses SDA/SCL on pins 0/2, respectively, and it uses the ESP8285.
 SDA and SCL should have 4K7 pull-up resistors (to 3.3V).
 
 Hardware setup:
 SDA ----------------------- 0
 SCL ----------------------- 2
 
 Following Sensors are on the board wth the respective addresses
 0x28 EM7180
 0x42 CAM M8Q -- GPS Sensor
 0x50 M24512 EEPROM
 0x58 M24512 EEPROM
 0x68 MPU9250 -- Accelerometer, Gyro and Magnetometer
 0x76 MS5637  -- Pressure Sensor

 Cloud data link: https://data.sparkfun.com/streams/v0O0vgyjyVfpr554alR3
*/
#include "Wire.h"   
#include <ESP8266WiFi.h>
#include <Phant.h>
#include <ArduinoOTA.h>
#include "constants.h"

#define myLed 15

//=============================== Variable Declaration ==============================
String webString="";     // String to display
// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 15000;              // interval at which to read sensor
uint8_t *mac;  // To find mac address of the WiFi Module
String MAC_ADDR = "AB:CD:EF:01:23:45";

// Phant Address and security keys
const char PhantHost[] = "data.sparkfun.com";
const char PublicKey[] = "v0O0vgyjyVfpr554alR3";
const char PrivateKey[] = "aPdPG2656RukM00Vp9jP";

// MPU9250 variables
uint8_t Ascale = AFS_2G;  // set accel full scale
int16_t accelData[3] = {0, 0, 0};  // Stores the 16-bit signed accelerometer sensor output
//Error Handling
byte i2c_read_error;
bool readFlag = true;
String error_message = "ALL_OK";

float feature_arr[6]; // Feature Array

//=============================== Function Declaration ==============================
void    initMPU9250(); // Initialize the MPU9250 IMU
void    initWifi(); // Initiate the WiFi module
int     post_to_phant();  // Function to post data to Phant cloud
void    SENtralPassThroughMode(); // Initialize EM7180
void    I2Cscan();  // Scan devices on I2C line
void    readAccelData(int16_t * destination); // Read data from accelerometer
void    calc_features();  // Calculate the feature values
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

  // Get some information abut the ESP8285

//  uint32_t freeheap = ESP.getFreeHeap();
//  Serial.print("Free Heap Size = "); Serial.println(freeheap);
//  uint32_t chipID = ESP.getChipId();
//  Serial.print("ESP8285 chip ID = "); Serial.println(chipID);
//  uint32_t flashChipID = ESP.getFlashChipId();
//  Serial.print("ESP8285 flash chip ID = "); Serial.println(flashChipID);
//  uint32_t flashChipSize = ESP.getFlashChipSize();
//  Serial.print("ESP8285 flash chip size = "); Serial.print(flashChipSize); Serial.println(" bytes");
//  uint32_t flashChipSpeed = ESP.getFlashChipSpeed();
//  Serial.print("ESP8285 flash chip speed = "); Serial.print(flashChipSpeed); Serial.println(" Hz");
//  uint32_t getVcc = ESP.getVcc();
//  Serial.print("ESP8285 supply voltage = "); Serial.print(getVcc); Serial.println(" volts");

  initWifi();
//  MAC_ADDR = String(String(mac[5], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[0], HEX));
//  Serial.print("MAC Address: ");
//  Serial.println(MAC_ADDR);
}
 

void loop()
{  
  if(previousMillis+interval < millis())
  {
    Serial.println("Posting Data");
    if(error_message.equals("ALL_OK"))
    {
      readAccelData(accelData); // Reading Accel Data
      calc_features();
    }
    
    if(post_to_phant())
    {
      previousMillis = millis();
    }
    else
    {
      Serial.println("Data not posted to Phant");
    }
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
  Serial.print("MPU-9250 initialized successfully.\n\n\n");
}

void initWifi() {
  const char* ssid     = "GANAPATI";          //"SkyDeckWireless";
  const char* password = "HP3PARUSERS2015";   //"moonshot";
 // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");

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

int post_to_phant()
{
  Phant phant(PhantHost, PublicKey, PrivateKey);  // Phant object declaration
  //Adding data to phant
  phant.add("mac_addr", MAC_ADDR);
  phant.add("bat", 98);
  phant.add("feature1", feature_arr[0]);
  phant.add("feature2", feature_arr[1]);
  phant.add("feature3", feature_arr[2]);
  phant.add("feature4", feature_arr[3]);
  phant.add("feature5", feature_arr[4]);
  phant.add("feature6", feature_arr[5]);

  WiFiClient client;  // HTTP Client object
  const int httpPort = 80;  // HTTP default port

  if(!client.connect(PhantHost, httpPort))
  {
    Serial.println("Not connected to client");
    return 0;
  }
  String phant_post = phant.post(); // Post request to Phant server
  client.print(phant_post);
  while(client.available()) // Read the response from the server
  {
    String response = client.readStringUntil('\r');
    Serial.print("Server Response: ");
    Serial.println(response);
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
  uint8_t rawData[6];  // x/y/z accel register data stored here
  readBytes(MPU9250_ADDRESS, ACCEL_XOUT_H, 6, &rawData[0]);  // Read the six raw data registers into data array
  destination[0] = ((int16_t)rawData[0] << 8) | rawData[1] ;  // Turn the MSB and LSB into a signed 16-bit value
  destination[1] = ((int16_t)rawData[2] << 8) | rawData[3] ;
  destination[2] = ((int16_t)rawData[4] << 8) | rawData[5] ;
  //Serial.printf("Accel data read: %d, %d, %d", destination[0], destination[1], destination[2]);
}

void calc_features()
{
  feature_arr[0] = (float)accelData[0]; // Raw Accel Data along X
  feature_arr[1] = (float)accelData[1]; // Raw Accel Data along Y
  feature_arr[2] = (float)accelData[2]; // Raw Accel Data along Z
  feature_arr[3] = (float)accelData[0] * 9.8 / 16384;  // Accel in m/s2 along X
  feature_arr[4] = (float)accelData[1] * 9.8 / 16384;  // Accel in m/s2 along Y
  feature_arr[5] = (float)accelData[2] * 9.8 / 16384;  // Accel in m/s2 along Z
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
