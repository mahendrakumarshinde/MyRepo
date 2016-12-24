#define ACCEL_BUFF_SIZE  50
#define AUDIO_BUFF_SIZE  50
#define FEATURE_SIZE 20
#define pi 3.14159

// aRes to be changed according to the sensor
#define aRes 1024 // Accelerometer resolution for sensitivity

enum feature_name
{
  ASE = 0,  // Acceleration Energy
  VLX,      // Velocity X
  VLY,      // Velocity Y
  VLZ,      // Velocity Z
  TMP,      // Temperature
  AAD,      // Audio_dB
  LAT,      // Lattitude
  LON,      // Longitude
  AAX,      // Accel_RMS_X
  AAY,      // Accel_RMS_Y
  AAZ,      // Accel_RMS_Z
  RPM,      // Frequency
  RAX,      // Accel_X buffer
  RAY,      // Accel_Y buffer
  RAZ,      // Accel_Z buffer
  RAD,      // Audio buffer
  FFT       // Accel_FFT
};

// Struct feature to configure feature timings with feature value
uint16_t MaxCount[FEATURE_SIZE];
uint16_t Count[FEATURE_SIZE];

int16_t accel_data[3] = {0, 0, 0};  // Stores the 16-bit signed accelerometer sensor output
int16_t accel_x_buff[ACCEL_BUFF_SIZE]; // Accel buffer along X axis
int16_t accel_y_buff[ACCEL_BUFF_SIZE]; // Accel buffer along Y axis
int16_t accel_z_buff[ACCEL_BUFF_SIZE]; // Accel buffer along Z axis
int avg_x = 0, avg_y = 0, avg_z = 0;  // Average value for 1 iteration

int16_t audio_buff[AUDIO_BUFF_SIZE]; // Audio buffer - 2 buffers namely for recording and computation simultaneously

uint16_t max_index_x = 1, max_index_y = 1, max_index_z = 1;

// GPS functionality
String gps_data = "";
String lat_str = "", lon_str = "";
char ns_indicator = 'a', ew_indicator = 'a';


//-----------------Function Declaration-------------------
float signal_energy();              // Calculates the acceleration energy along all the axes
float velocity(char axis);          // Calculate the velocity along the parameter axis
float acceleration_rms(char axis);  // Calculate the RMS acceleration along the parameter axis
float get_temperature();            // Gets the temperature value in degree celcius
float audio_db();                   // Calculates the audio data in dB
void gps_data();                    // Parse the GPS string and gets the GPS co-ordinates in terms of lattitue and longitude

void timer_isr()  // Timer interrupt service routine to be executed at 1 ms interval
{
  Count[ASE]++;
  Count[VLX]++;
  Count[VLY]++;
  Count[VLZ]++;
  Count[TMP]++;
  Count[AAD]++;
  Count[LAT]++;
  Count[LON]++;
  Count[AAX]++;
  Count[AAY]++;
  Count[AAZ]++;
  Count[RPM]++;
  Count[RAX]++;
  Count[RAY]++;
  Count[RAZ]++;
  Count[RAD]++;
  Count[FAX]++;
  Count[FAY]++;
  Count[FAZ]++;

  if(Count[ASE] >= MaxCount[ASE])
  {
    // Signal Energy function call
    Count[ASE] = 0;
  }
  if(Count[VLX] >= MaxCount[VLX])
  {
    // Velocity X function call
    Count[VLX] = 0;
  }
  if(Count[VLY] >= MaxCount[VLY])
  {
    // Velocity Y function call
    Count[VLY] = 0;
  }
  if(Count[VLZ] >= MaxCount[VLZ])
  {
    // Velocity Z function call
    Count[VLZ] = 0;
  }
  if(Count[TMP] >= MaxCount[TMP])
  {
    // Temperature function call
    Count[TMP] = 0;
  }
  if(Count[AAD] >= MaxCount[AAD])
  {
    // Velocity X function call
    Count[AAD] = 0;
  }
  if(Count[LAT] >= MaxCount[LAT])
  {
    // GPS function call
    Count[LAT] = 0;
  }
  if(Count[LON] >= MaxCount[LON])
  {
    // GPS function call
    Count[LON] = 0;
  }
  if(Count[AAX] >= MaxCount[AAX])
  {
    // Acceleration RMS X function call
    Count[AAX] = 0;
  }
  if(Count[AAY] >= MaxCount[AAY])
  {
    // Acceleration RMS Y function call
    Count[AAY] = 0;
  }
  if(Count[AAZ] >= MaxCount[AAZ])
  {
    // Acceleration RMS Z function call
    Count[AAZ] = 0;
  }
  if(Count[RPM] >= MaxCount[RPM])
  {
    // RPM function call
    Count[RPM] = 0;
  }
  if(Count[RAX] >= MaxCount[RAX])
  {
    // Acceleration X function call
    Count[RAX] = 0;
  }
  if(Count[RAY] >= MaxCount[RAY])
  {
    // Acceleration Y function call
    Count[RAY] = 0;
  }
  if(Count[RAZ] >= MaxCount[RAZ])
  {
    // Acceleration Z function call
    Count[RAZ] = 0;
  }
  if(Count[RAD] >= MaxCount[RAD])
  {
    // Audio buffer function call
    Count[RAD] = 0;
  }
  if(Count[FAX] >= MaxCount[FAX])
  {
    // Acceleration X FFT function call
    Count[FAX] = 0;
  }
  if(Count[FAY] >= MaxCount[FAY])
  {
    // Acceleration Y FFT function call
    Count[FAY] = 0;
  }
  if(Count[FAZ] >= MaxCount[FAZ])
  {
    // Acceleration Z FFT function call
    Count[FAZ] = 0;
  }
  
}

void i2c_isr()
{
  // Code to read acceleration data
}

void ble_isr()
{
  // ISR to receive data over BLE and set the parameters accordingly
}

void setup() {
  // put your setup code here, to run once:
  MaxCount[ASE] = 1000; // Default value for execution at 1 second rate
  MaxCount[VLX] = 1000; // Default value for execution at 1 second rate
  MaxCount[VLY] = 1000; // Default value for execution at 1 second rate
  MaxCount[VLZ] = 1000; // Default value for execution at 1 second rate
  MaxCount[TMP] = 1000; // Default value for execution at 1 second rate
  MaxCount[AAD] = 1000; // Default value for execution at 1 second rate
  MaxCount[LAT] = 1000; // Default value for execution at 1 second rate
  MaxCount[LON] = 1000; // Default value for execution at 1 second rate
  MaxCount[AAX] = 1000; // Default value for execution at 1 second rate
  MaxCount[AAY] = 1000; // Default value for execution at 1 second rate
  MaxCount[AAZ] = 1000; // Default value for execution at 1 second rate
  MaxCount[RPM] = 1000; // Default value for execution at 1 second rate
  MaxCount[RAX] = 1000; // Default value for execution at 1 second rate
  MaxCount[RAY] = 1000; // Default value for execution at 1 second rate
  MaxCount[RAZ] = 1000; // Default value for execution at 1 second rate
  MaxCount[RAD] = 1000; // Default value for execution at 1 second rate
  MaxCount[FAX] = 1000; // Default value for execution at 1 second rate
  MaxCount[FAY] = 1000; // Default value for execution at 1 second rate
  MaxCount[FAZ] = 1000; // Default value for execution at 1 second rate

  // Initializing the counter value
  Count[ASE] = 0;
  Count[VLX] = 0;
  Count[VLY] = 0;
  Count[VLZ] = 0;
  Count[TMP] = 0;
  Count[AAD] = 0;
  Count[LAT] = 0;
  Count[LON] = 0;
  Count[AAX] = 0;
  Count[AAY] = 0;
  Count[AAZ] = 0;
  Count[RPM] = 0;
  Count[RAX] = 0;
  Count[RAY] = 0;
  Count[RAZ] = 0;
  Count[RAD] = 0;
  Count[FAX] = 0;
  Count[FAY] = 0;
  Count[FAZ] = 0;
  
}

void loop() {
  // put your main code here, to run repeatedly:

}

// Calculates the acceleration energy along all the axes
float signal_energy()
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

// Calculate the velocity along the parameter axis
float velocity(char axis)
{
  uint16_t max_index = 1;
  float accelRMS = acceleration_rms(axis);
  switch(axis)
  {
    case 'X': max_index = max_index_x; break;
    case 'Y': max_index = max_index_y; break;
    case 'Z': max_index = max_index_z; break;
  }

  float vel_rms = accelRMS * 1000 / (2 * pi * max_index); // accelRMS * freq /(2 pi f)
  Serial.print("Vel_RMS = ");
  Serial.println(vel_rms);
  Serial.println(max_index);
  return vel_rms;
}

// Calculate the RMS acceleration along the parameter axis
float acceleration_rms(char axis)
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
  switch(axis)
  {
    case 'X': max_index_x = max_index; break;
    case 'Y': max_index_y = max_index; break;
    case 'Z': max_index_z = max_index; break;
  }
  return accelRMS;
}

// Gets the temperature value in degree celcius
float get_temperature()
{
  // Logic depends on the sensor you used
}

// Calculates the audio data in dB
float audio_db()
{
  float audioDB_val = 0.0;
  float aud_data = 0.0;
  for (int i = 0; i < AUDIO_BUFF_SIZE; i++)
  {
    aud_data = (float) abs(audio_buff[i]);// / 16777215.0;    //16777215 = 0xffffff - 24 bit max value
    //    Serial.printf("AudioData: %f\n", aud_data);
    if (aud_data > 0)
    {
      audioDB_val += (20 * log10(aud_data));
    }
  }
  audioDB_val /= MAX_INTERVAL_AUDIO;
  //  Serial.printf("AudioDB: %f\n", audioDB_val);
  return audioDB_val;
}

// Change the logic according to new sensor
// Parse the GPS string and gets the GPS co-ordinates in terms of lattitue and longitude
void gps_data()
{
//  Serial.print("Read GPS Data: ");
//  Serial.println(millis());
//  
//  uint8_t rx_data[32];
//  readBytes(CAMM8Q_ADDRESS, CAMM8Q_I2C_BYTES, 2, &rx_data[0]);
//
//  uint16_t scount = ( ((uint16_t) rx_data[0] << 8) | rx_data[1]);
//  Serial.println(scount);
//  Serial.print("Size of RX_DATA: "); Serial.println(sizeof(rx_data));
//  Serial.println(i2c_read_error);
//  
//  if(scount > 0) {
//    gps_data = "";
//    while(scount) {
//      uint8_t rx_count = ((scount > 32) ? 32 : scount);
//      readBytes(CAMM8Q_ADDRESS, CAMM8Q_I2C_DATA,  rx_count, &rx_data[0]);
//      
//      for(uint8_t ii = 0; ii < rx_count; ii++) 
//      {
//        char aChar = rx_data[ii];
//        if(isValid(aChar))
//        {
//          gps_data += String(aChar);
//        }
//      }
//      scount -= rx_count;
//    }
//    Serial.println(gps_data);
//    parseGGA(gps_data);
//    Serial.println();
//  }
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

float LSB_to_ms2(int16_t accelLSB)
{
  return ((float)accelLSB / aRes * 9.8);
}
