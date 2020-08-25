
#ifndef STM32_MODBUS_SLAVE_H
#define STM32_MODBUS_SLAVE_H

#include "RegModbus.h"
#include "Arduino.h"
#include "ArduinoJson.h"
#include "FS.h"
#include <IUSerial.h>

// #include "Modbus_reg.h"


#define MODBUS_DEFAULT_BAUD_RATE 115200

#define MODBUS_MAX_BUFFER 255 //not used -Max Buffer size of UART port of stm32
#define MODBUS_DEFAULT_SLAVE_ID 1
#define MODBUS_MAX_WR_READ_REGISTERS 125

#define BUFFER_SIZE 255

#define MODBUS_SLAVEID_MIN 1
#define MODBUS_SLAVEID_MAX 128
/**
 * RS485 chip
 *
 * Component:
 *   Name:
 *   MAX MAX13487E
 * Description:
 *   Half-Duplex RS-485-/RS-422 
 *	Compatible Transceiver with AutoDirection Control

 */
 union
{
  uint16_t reg[2];
  float data;

} constructFloat;

union {
  char str[20];
  char mac[20];

} macAdddress;

namespace modbusGroups {
	enum option : uint8_t {
        MODBUS_STREAMING_FEATURES = 1,
		MODBUS_STREAMING_SPECTRAL_FEATURES = 2,
		COUNT = 2
	};
}
enum 
{	 MB_EX_ILLEGAL_FUNCTION = 0x01, // Function Code not Supported
	 MB_EX_ILLEGAL_ADDRESS = 0x02,  // Output Address not exists
	 MB_EX_ILLEGAL_VALUE = 0x03,	// Output Value not in Range
	 MB_EX_SLAVE_FAILURE = 0x04,	// Slave Deivece Fails to process request
		
};

class IUmodbus //: public IUSerial
{
public:
	IUmodbus(HardwareSerial *SelectSerial, uint8_t RE_ENABLE_PIN, uint8_t DE_ENABLE_PIN);
	bool setupModbusDevice(JsonVariant &config);
	void setslaveID(uint8_t _slaveID);
	uint8_t getslaveID(){return slaveID;};
	void begin(uint32_t baudRate);
	void begin(uint32_t baudRate, int data_bits, int stop_bits, const char *parity);
	void configure(uint8_t _slaveID,
				   unsigned int _holdingRegsSize);
	unsigned int modbus_update(unsigned int *holdingRegs);
    uint16_t getFloatValue (float data1);
	void configureModbus(JsonVariant &config);
	void storeDeviceConfigParameters();
	void updateBLEMACAddress(MacAddress macid);
	void updateWIFIMACAddress(MacAddress macid);
	uint16_t updateFloatValues(float value);
	uint8_t updateHoldingRegister(int groupNo, int startAddress, int endAddress,float* features);	
	uint8_t clearHoldingRegister(int groupNo, int startAddress, int endAddress);

	unsigned int m_holdingRegs[TOTAL_REGISTER_SIZE + 1];
	uint8_t m_id ;
    uint32_t m_baud ;
    uint8_t m_databit;
    uint8_t m_stopbit ;
    const char* m_parity ;
	uint8_t parity_byte;
	int modbusUpdateInterval = 1000;
	int lastModbusUpdateTime = 0;
	uint16_t WIFI_FIRMWARE_VERSION;
	uint16_t STM_FIRMWARE_VERSION;
	bool modbusConnectionStatus = false;

protected:
	HardwareSerial *m_port;
	uint32_t baud;
	void exceptionResponse(unsigned char exception);
	unsigned int calculateCRC(byte bufferSize);
	void sendPacket(unsigned char bufferSize);
	char serialConfig[10];
	unsigned char frame[BUFFER_SIZE];
	unsigned int T1_5;			  // inter character time out
	unsigned int T3_5;			  // frame delay
	unsigned int holdingRegsSize; // size of the register array
	unsigned char broadcastFlag;
	uint8_t slaveID;
	unsigned char function;
	uint8_t reEnablePin;    //RE -Active low enable pin 
	uint8_t deEnablePin;  	//DE -Active high enable pin
	unsigned int errorCount;

};
#endif
