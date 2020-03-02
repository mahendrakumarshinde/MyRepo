#ifndef MODBUS_REGISTERS_H
#define MODBUS_REGISTERS_H

enum
{
   // GRUOP - 1 Streaming Features

   OP_STATE = 0,                                //start  - 40001   
   SIGNAL_ENERGY_L = 2,                         //40003
   SIGNAL_ENERGY_H,
   VELOCITY_RMS_X_L,                            //40005
   VELOCITY_RMS_X_H,
   VELOCITY_RMS_Y_L,                            //40007
   VELOCITY_RMS_Y_H,
   VELOCITY_RMS_Z_L,                            //40009
   VELOCITY_RMS_Z_H,
   TEMPERATURE_L,                               //40011
   TEMPERATURE_H,
   AUDIO_L,                                     //40013
   AUDIO_H,

   WIFI_RSSI_L,                                 //40015
   WIFI_RSSI_H,

   TIMESTAMP_LB_L,                              //40017
   TIMESTAMP_LB_H,
   TIMESTAMP_HB_L,                              //40019
   TIMESTAMP_HB_H,
   ACCEL_MAIN_FREQ_X_L,                         //40021
   ACCEL_MAIN_FREQ_X_H,
   ACCEL_MAIN_FREQ_Y_L,                         //40023
   ACCEL_MAIN_FREQ_Y_H,
   ACCEL_MAIN_FREQ_Z_L,                         //40025
   ACCEL_MAIN_FREQ_Z_H,                         
   
   
   // GRUOP - 2 Spectral Features (Max 13 )
   // Data Storage mechanism  - (Key ,value)
   
   FINGERPRINT_KEY_1_L  = 50,                     //START-40051       
   FINGERPRINT_KEY_1_H,
   FINGERPRINT_1_L,  
   FINGERPRINT_1_H,
   
   FINGERPRINT_KEY_2_L,
   FINGERPRINT_KEY_2_H,
   FINGERPRINT_2_L,
   FINGERPRINT_2_H,
   
   FINGERPRINT_KEY_3_L,
   FINGERPRINT_KEY_3_H,
   FINGERPRINT_3_L,
   FINGERPRINT_3_H,

   FINGERPRINT_KEY_4_L,
   FINGERPRINT_KEY_4_H,
   FINGERPRINT_4_L,
   FINGERPRINT_4_H,

   FINGERPRINT_KEY_5_L,
   FINGERPRINT_KEY_5_H,
   FINGERPRINT_5_L,
   FINGERPRINT_5_H,

   FINGERPRINT_KEY_6_L,
   FINGERPRINT_KEY_6_H,
   FINGERPRINT_6_L,
   FINGERPRINT_6_H,
   
   FINGERPRINT_KEY_7_L,
   FINGERPRINT_KEY_7_H,
   FINGERPRINT_7_L,
   FINGERPRINT_7_H,
   
   FINGERPRINT_KEY_8_L,
   FINGERPRINT_KEY_8_H,
   FINGERPRINT_8_L,
   FINGERPRINT_8_H,
   
   FINGERPRINT_KEY_9_L,
   FINGERPRINT_KEY_9_H,
   FINGERPRINT_9_L,
   FINGERPRINT_9_H,
   
   FINGERPRINT_KEY_10_L,
   FINGERPRINT_KEY_10_H,
   FINGERPRINT_10_L,
   FINGERPRINT_10_H,
   
   FINGERPRINT_KEY_11_L,
   FINGERPRINT_KEY_11_H,
   FINGERPRINT_11_L,
   FINGERPRINT_11_H,
   
   FINGERPRINT_KEY_12_L,
   FINGERPRINT_KEY_12_H,
   FINGERPRINT_12_L,
   FINGERPRINT_12_H,
   
   FINGERPRINT_KEY_13_L,
   FINGERPRINT_KEY_13_H,
   FINGERPRINT_13_L,                                  //END-40088
   FINGERPRINT_13_H,                                  

   // GRUOP - 3 One Time Configurable Device Parameters

   MAC_ID_BLE_1 = 110,                                //START-40110  Ex-MAC-94:54:93:43:38:EA  MAC1-0X94
   MAC_ID_BLE_2,                                      
   MAC_ID_BLE_3,                                      
   
   MAC_ID_WIFI_1,                                     //40104
   MAC_ID_WIFI_2,
   MAC_ID_WIFI_3,
   
   FIRMWARE_VERSION_STM32,                            //INT 40107
   FIRMWARE_VERSION_ESP32,                            //INT 40108
   CURRENT_SAMPLING_RATE,                             //40109
   CURRENT_BLOCK_SIZE,                                //40110
   LOWER_CUTOFF_FREQ,                                //40111
   HIGHER_CUTOFF_FREQ,                              //END - 40113
   
   // GROUP 4 - MODBUS SLAVE Communication Parameters

   SLAVE_ID = 150,                                    //START-40151
   DATA_BIT,
   BAUDRATE_L = 152,
   BAUDRATE_H,

   PARITY = 154,                                      //40155
   START_STOP_BIT,
   FLOW_CONTROL,
   TOTAL_ERRORS,
   TOTAL_REGISTER_SIZE,                               // end-458
   
};

#endif
