#include "Modbus_slave.h"
#include "FFTConfiguration.h"
#include "Conductor.h"

extern Conductor conductor;


IUmodbus::IUmodbus(HardwareSerial *SelectSerial, uint8_t RE_ENABLE_PIN, uint8_t DE_ENABLE_PIN)
{
    reEnablePin = RE_ENABLE_PIN;                            //Tx_pin = RE -active low enable
    deEnablePin = DE_ENABLE_PIN;                          //Tx1_pin = DE -active high enable
    m_port = SelectSerial;
    if(setupDebugMode){
        debugPrint("DEBUG INIT");
    }
}

/**
 * @brief 
 * setup HardwareSerial parameters, slaveID from File
 */
bool IUmodbus::setupModbusDevice(JsonVariant &config){
    
    bool success = true;  // success if at least "ssid" and "psk" found
    //const char *value;
    // JsonVariant subconfig = config;
    // subconfig.printTo(Serial);

    // Read the file and configure the Modbus Slave
    if(DOSFS.exists("/iuconfig/modbusSlave.conf")){
        
        // Read the current configurations and apply
        
        m_id = config["slaveid"].as<uint16_t>();
        m_baud =  config["baudrate"].as<uint32_t>();
        m_databit = config["databit"].as<uint8_t>();
        m_stopbit = config["stopbit"].as<uint8_t>();
        m_parity = config["parity"];

        if (strncmp(m_parity, "NONE", 4) == 0 ){ parity_byte = 0;}
        if(strncmp(m_parity, "EVEN", 4) == 0 ){parity_byte = 1;}
        if(strncmp(m_parity, "ODD", 3) == 0 ){parity_byte =2; }
    
        if (debugMode)
        {
            debugPrint("SLAVE ID  : ",false);debugPrint(m_id);
            debugPrint("BAUD RATE :",false);debugPrint(m_baud);
            debugPrint("DATA BIT  :",false);debugPrint(m_databit);
            debugPrint("PARITY    :",false);debugPrint(m_parity);
        }
        
    }else
    {   
        debugPrint("MODBUS CONFIG INTERNAL :",false);
        debugPrint(iuFlash.readInternalFlash(CONFIG_MODBUS_SLAVE_CONFIG_FLASH_ADDRESS) );
        // USING default modbusSlave configuration
        m_id =      DEFAULT_MODBUS_SLAVEID;
        m_baud =    DEFAULT_MODBUS_BAUD;
        m_databit = DEFAULT_MODBUS_DATABIT;
        m_stopbit = DEFAULT_MODBUS_STOPBIT;
        m_parity =  DEFAULT_MODBUS_PARITY;
        if (debugMode)
        {
            debugPrint("MODBUS CONFIGS NOT AVAILABLE USING DEFAULT");
        }
        success = false;
        conductor.modbusStreamingMode = false; 
    }

    begin(m_baud,m_databit,m_stopbit,m_parity);
    configure(m_id,TOTAL_REGISTER_SIZE+1);
    conductor.modbusStreamingMode = true;   // Set the Streaming mode as MODBUS

        
    return success;
}

/**
 * Set the modbus slave ID for the unit 
 * acceptable range of slave ID is 1-127 
 * If not in range then set default i.e Slave ID=1
 * return should be SlaveID
 **/
void IUmodbus::setslaveID(uint8_t _slaveID)
{
    if (_slaveID < MODBUS_SLAVEID_MIN || _slaveID >= MODBUS_SLAVEID_MAX)
    {
        slaveID = MODBUS_DEFAULT_SLAVE_ID;
        debugPrint("Failed to set slaveID");
    
        return;
    }
    else
        slaveID = _slaveID;
        return ;
}

void IUmodbus::begin(uint32_t baudRate)
{
    baud = baudRate;
    m_port->begin(baudRate, SERIAL_8N1);
}

/**
 *set the serial communication parameters
 *serial port & baudrate
 * all baud rates defined in micro's
 * add debug prints while merging
 **/

void IUmodbus::begin(uint32_t baudRate, int data_bits, int stop_bits, const char *parity)
{
    char serialConfig[11];
    uint32_t config;
    if ((baudRate >= 1200 && baudRate <= 115200) && (data_bits >= 5 && data_bits <= 8) && (stop_bits == 1 || stop_bits == 2) && (strncmp(parity, "N", 1) == 0 || strncmp(parity, "E", 1) == 0 || strncmp(parity, "O", 1) == 0))
    {
        snprintf(serialConfig, 11, "%s%d%c%d", "SERIAL_", data_bits, parity[0], stop_bits);
        //debugPrint("MODBUS DEBUG CONFIGS 1:",false);debugPrint(serialConfig);
        if (strcmp(serialConfig, "SERIAL_7N1") == 0)
        {
            config = SERIAL_7N1;
        }
        else if (strcmp(serialConfig, "SERIAL_8N1") == 0)
        {
            config = SERIAL_8N1;
        }
        else if (strcmp(serialConfig, "SERIAL_7N2") == 0)
        {
            config = SERIAL_7N2;
        }
        else if (strcmp(serialConfig, "SERIAL_8N2") == 0)
        {
            config = SERIAL_8N2;
        }
        else if (strcmp(serialConfig, "SERIAL_7E1") == 0)
        {
            config = SERIAL_7E1;
        }
        else if (strcmp(serialConfig, "SERIAL_8E1") == 0)
        {
            config = SERIAL_8E1;
        }
        else if (strcmp(serialConfig, "SERIAL_7E2") == 0)
        {
            config = SERIAL_7E2;
        }
        else if (strcmp(serialConfig, "SERIAL_8E2") == 0)
        {
            config = SERIAL_8E2;
        }
        else if (strcmp(serialConfig, "SERIAL_7O1") == 0)
        {
            config = SERIAL_7O1;
        }
        else if (strcmp(serialConfig, "SERIAL_8O1") == 0)
        {
            config = SERIAL_8O1;
        }
        else if (strcmp(serialConfig, "SERIAL_7O2") == 0)
        {
            config = SERIAL_7O2;
        }
        else if (strcmp(serialConfig, "SERIAL_8O2") == 0)
        {
            config = SERIAL_8O2;
        }
        
    }
    else
    {   debugPrint("MODBUS DEBUG : USING DEFAULT PORT CONFIGS");    
        if (strcmp(serialConfig, "SERIAL_8N1") == 0)
        {
            config = SERIAL_8N1;
        }
    }

    baud = baudRate;
    m_port->begin(baudRate, config);
}

unsigned int IUmodbus::modbus_update(unsigned int *holdingRegs)
{
    unsigned char buffer = 0;
    unsigned char overflow = 0;
    if(!m_port->available()){
        modbusConnectionStatus = false;
        if(debugMode){
            //debugPrint("MODBUS DEBUG : NO Query from Master");
        }
    }
    while (m_port->available())
    {
        // The maximum number of bytes is limited to the Serial4 buffer size of 128 bytes
        // If more bytes is received than the BUFFER_SIZE the overflow flag will be set and the
        // Serial4 buffer will be red untill all the data is cleared from the receive buffer.
        if (overflow){
            m_port->read();
        }else
        {
            if (buffer == BUFFER_SIZE)
                overflow = 1;
            frame[buffer] = m_port->read();
            buffer++;
        }
        delayMicroseconds(T1_5); // inter character time out
    }

    // If an overflow occurred increment the errorCount
    // variable and return to the main sketch without
    // responding to the request i.e. force a timeout
    if (overflow)
        return errorCount++;

    // The minimum request packet is 8 bytes for function 3 & 16
    if (buffer > 6)
    {
        unsigned char id = frame[0];

        broadcastFlag = 0;

        if (id == 0)
            broadcastFlag = 1;

        if (id == slaveID || broadcastFlag) // if the recieved ID matches the slaveID or broadcasting id (0), continue
        {
            unsigned int crc = ((frame[buffer - 2] << 8) | frame[buffer - 1]); // combine the crc Low & High bytes
            if (calculateCRC(buffer - 2) == crc)                               // if the calculated crc matches the recieved crc continue
            {
                function = frame[1];
                unsigned int startingAddress = ((frame[2] << 8) | frame[3]); // combine the starting address bytes
                unsigned int no_of_registers = ((frame[4] << 8) | frame[5]); // combine the number of register bytes
                unsigned int maxData = startingAddress + no_of_registers;
                unsigned char index;
                unsigned char address;
                unsigned int crc16;

                // broadcasting is not supported for function 3
                if (!broadcastFlag && (function == 3))
                {
                    if (startingAddress < holdingRegsSize) // check exception 2 ILLEGAL DATA ADDRESS
                    {
                        if (maxData <= holdingRegsSize) // check exception 3 ILLEGAL DATA VALUE
                        {
                            unsigned char noOfBytes = no_of_registers * 2;
                            unsigned char responseFrameSize = 5 + noOfBytes; // ID, function, noOfBytes, (dataLo + dataHi) * number of registers, crcLo, crcHi
                            frame[0] = slaveID;
                            frame[1] = function;
                            frame[2] = noOfBytes;
                            address = 3; // PDU starts at the 4th byte
                            unsigned int temp;

                            for (index = startingAddress; index < maxData; index++)
                            {
                                temp = holdingRegs[index];
                                frame[address] = temp >> 8; // split the register into 2 bytes
                                address++;
                                frame[address] = temp & 0xFF;
                                address++;
                            }

                            crc16 = calculateCRC(responseFrameSize - 2);
                            frame[responseFrameSize - 2] = crc16 >> 8; // split crc into 2 bytes
                            frame[responseFrameSize - 1] = crc16 & 0xFF;
                            sendPacket(responseFrameSize);
                        }
                        else
                            exceptionResponse(3); // exception 3 ILLEGAL DATA VALUE
                    }
                    else
                        exceptionResponse(2); // exception 2 ILLEGAL DATA ADDRESS
                }
                else if (function == 6)
                {
                    if (startingAddress < holdingRegsSize) // check exception 2 ILLEGAL DATA ADDRESS
                    {
                        unsigned int startingAddress = ((frame[2] << 8) | frame[3]);
                        unsigned int regStatus = ((frame[4] << 8) | frame[5]);
                        unsigned char responseFrameSize = 8;

                        holdingRegs[startingAddress] = regStatus;

                        crc16 = calculateCRC(responseFrameSize - 2);
                        frame[responseFrameSize - 2] = crc16 >> 8; // split crc into 2 bytes
                        frame[responseFrameSize - 1] = crc16 & 0xFF;
                        sendPacket(responseFrameSize);
                    }
                    else
                        exceptionResponse(2); // exception 2 ILLEGAL DATA ADDRESS
                }
                else if (function == 16)
                {
                    // check if the recieved number of bytes matches the calculated bytes minus the request bytes
                    // id + function + (2 * address bytes) + (2 * no of register bytes) + byte count + (2 * CRC bytes) = 9 bytes
                    if (frame[6] == (buffer - 9))
                    {
                        if (startingAddress < holdingRegsSize) // check exception 2 ILLEGAL DATA ADDRESS
                        {
                            if (maxData <= holdingRegsSize) // check exception 3 ILLEGAL DATA VALUE
                            {
                                address = 7; // start at the 8th byte in the frame

                                for (index = startingAddress; index < maxData; index++)
                                {
                                    holdingRegs[index] = ((frame[address] << 8) | frame[address + 1]);
                                    address += 2;
                                }

                                // only the first 6 bytes are used for CRC calculation
                                crc16 = calculateCRC(6);
                                frame[6] = crc16 >> 8; // split crc into 2 bytes
                                frame[7] = crc16 & 0xFF;

                                // a function 16 response is an echo of the first 6 bytes from the request + 2 crc bytes
                                if (!broadcastFlag) // don't respond if it's a broadcast message
                                    sendPacket(8);
                            }
                            else
                                exceptionResponse(3); // exception 3 ILLEGAL DATA VALUE
                        }
                        else
                            exceptionResponse(2); // exception 2 ILLEGAL DATA ADDRESS
                    }
                    else
                        errorCount++; // corrupted packet
                }
                else
                    exceptionResponse(1); // exception 1 ILLEGAL FUNCTION
            }
            else // checksum failed
                errorCount++;
        } // incorrect id
    }
    else if (buffer > 0 && buffer < 8)
        errorCount++; // corrupted packet
        
    return errorCount;
}

void IUmodbus::exceptionResponse(unsigned char exception)
{
    //debugPrint("MODBUS DEBUG : ",false);debugPrint(exception);

    errorCount++;       // each call to exceptionResponse() will increment the errorCount
    if (!broadcastFlag) // don't respond if its a broadcast message
    {
        frame[0] = slaveID;
        frame[1] = (function | 0x80); // set the MSB bit high, informs the master of an exception
        frame[2] = exception;
        unsigned int crc16 = calculateCRC(3); // ID, function + 0x80, exception code == 3 bytes
        frame[3] = crc16 >> 8;
        frame[4] = crc16 & 0xFF;
        sendPacket(5); // exception response is always 5 bytes ID, function + 0x80, exception code, 2 bytes crc
    }
    modbusConnectionStatus = false;
}
/**
 * check validation for slaveID and all configurable parameter while merging
 */
void IUmodbus::configure(uint8_t _slaveID, unsigned int _holdingRegsSize)
{   
    //debugPrint("CONFIGURE : ID :",false);debugPrint(_slaveID);
    setslaveID(_slaveID);
    //debugPrint("CONFIGGURE : setslaveID");
    uint8_t _REenablePin = reEnablePin;
    if (_REenablePin > 1)
    { // pin 0 & pin 1 are reserved for RX/TX. To disable set txenpin < 2
        reEnablePin = _REenablePin;
        
        pinMode(reEnablePin, OUTPUT);
        pinMode(deEnablePin, OUTPUT);
        digitalWrite(reEnablePin, LOW);     //RE=0,DE=0 read enable (RE- enable,DE-disable )
        digitalWrite(deEnablePin, LOW);
    }

    // Modbus states that a baud rate higher than 19200 must use a fixed 750 us
    // for inter character time out and 1.75 ms for a frame delay.
    // For baud rates below 19200 the timeing is more critical and has to be calculated.
    // E.g. 9600 baud in a 10 bit packet is 960 characters per second
    // In milliseconds this will be 960characters per 1000ms. So for 1 character
    // 1000ms/960characters is 1.04167ms per character and finaly modbus states an
    // intercharacter must be 1.5T or 1.5 times longer than a normal character and thus
    // 1.5T = 1.04167ms * 1.5 = 1.5625ms. A frame delay is 3.5T.
    // Added experimental low latency delays. This makes the implementation
    // non-standard but practically it works with all major modbus master implementations.

    if (baud == 1000000)
    {
        T1_5 = 1;
        T3_5 = 10;
    }
    else if (baud >= 115200)
    {
        T1_5 = 75;
        T3_5 = 175;
    }
    else if (baud > 19200)
    {
        T1_5 = 750;
        T3_5 = 1750;
    }
    else
    {
        T1_5 = 15000000 / baud; // 1T * 1.5 = T1.5
        T3_5 = 35000000 / baud; // 1T * 3.5 = T3.5
    }
    holdingRegsSize = _holdingRegsSize;
    errorCount = 0; // initialize errorCount
   
}

unsigned int IUmodbus::calculateCRC(byte bufferSize)
{
    unsigned int temp, temp2, flag;
    temp = 0xFFFF;
    for (unsigned char i = 0; i < bufferSize; i++)
    {
        temp = temp ^ frame[i];
        for (unsigned char j = 1; j <= 8; j++)
        {
            flag = temp & 0x0001;
            temp >>= 1;
            if (flag)
                temp ^= 0xA001;
        }
    }
    // Reverse byte order.
    temp2 = temp >> 8;
    temp = (temp << 8) | temp2;
    temp &= 0xFFFF;
    return temp; // the returned value is already swopped - crcLo byte is first & crcHi byte is last
}

void IUmodbus::sendPacket(unsigned char bufferSize)
{
    if (reEnablePin > 1){
        digitalWrite(reEnablePin, HIGH);         //RE=1,DE=1 write enable (RE- disable,DE-enable )
        digitalWrite(deEnablePin, HIGH);
    }
        
    for (unsigned char i = 0; i < bufferSize; i++)
    {
        m_port->write(frame[i]);
    }

    m_port->flush();
    modbusConnectionStatus = true;
    // allow a frame delay to indicate end of transmission
    delayMicroseconds(T3_5);

    if (reEnablePin > 1)
        digitalWrite(reEnablePin, LOW);      //RE=0,DE=0 read enable (RE- enable,DE-disable )
        digitalWrite(deEnablePin, LOW);
}

//REPLACE SERIAL to Debug print 
void IUmodbus::configureModbus(JsonVariant &config)
{

    // You can use a String to get an element of a JsonObject
    // No duplication is done.
    File dataFile2 = DOSFS.open("/iuconfig/modbusSlave.conf", "r");
    if (dataFile2.available())
    {
        String configData = dataFile2.readString();
        DynamicJsonBuffer jsonBuffer2;
        JsonObject &root = jsonBuffer2.parseObject(configData);
        
        uint32_t baud = root["modbusSlaveConfig"]["baudrate"];
        uint8_t dataBit = root["modbusSlaveConfig"]["databit"];
        String parity = root["modbusSlaveConfig"][String("parity")];
        uint8_t stopBit = root["modbusSlaveConfig"]["stopbit"];
        uint8_t m_slaveId = root["modbusSlaveConfig"]["slaveid"];
        
    }else
    {
        debugPrint("MODBUS DEBUG : FILE not found");
    }
    
    dataFile2.close();
    
}

/**
 * @brief 
 * 
 * @param data1 - 2 bytes incoming data bytes
 * @return uint16_t 
 */
uint16_t IUmodbus:: getFloatValue(float data1)
{
  constructFloat.data = data1;
  return constructFloat.reg[2];
}
/**
 * @brief This method stores and updates the device configuration like 
 * 1. Serial port configurations parameters
 * 2. Firmware version details
 * 3. 
 * 
 */
void IUmodbus:: storeDeviceConfigParameters(){

 
  m_holdingRegs[SLAVE_ID] = m_id;
  getFloatValue(m_baud);
  m_holdingRegs[BAUDRATE_L] = constructFloat.reg[0];
  m_holdingRegs[BAUDRATE_H] = constructFloat.reg[1];  
  m_holdingRegs[DATA_BIT] = m_databit;
  m_holdingRegs[START_STOP_BIT] = m_stopbit;
  m_holdingRegs[PARITY] = parity_byte;
  m_holdingRegs[FIRMWARE_VERSION_STM32] = STM_FIRMWARE_VERSION;
  m_holdingRegs[FIRMWARE_VERSION_ESP32] = WIFI_FIRMWARE_VERSION;
  m_holdingRegs[CURRENT_SAMPLING_RATE] = FFTConfiguration::currentSamplingRate;
  m_holdingRegs[CURRENT_BLOCK_SIZE] = FFTConfiguration::currentBlockSize;
  m_holdingRegs[LOWER_CUTOFF_FREQ] = FFTConfiguration::currentLowCutOffFrequency;
  m_holdingRegs[HIGHER_CUTOFF_FREQ] = FFTConfiguration::currentHighCutOffFrequency;
 
  if (debugMode)
  {
      //debugPrint("MODBUS DEBUG : Updated the device configuration sucessfully");
  }
    
}
/**
 * @brief 
 * 
 * @param macid : this method updates the BLE macAdddress to holdingRegs
 */
void IUmodbus::updateBLEMACAddress(MacAddress blemac){
  if(blemac ){
        m_holdingRegs[MAC_ID_BLE_1] =  blemac[0]<<8    | blemac[1];                
        m_holdingRegs[MAC_ID_BLE_2] =  blemac[2]<<8    | blemac[3];                
        m_holdingRegs[MAC_ID_BLE_3] =  blemac[4]<<8    | blemac[5];                
    if (debugMode)
    {
       //debugPrint("MODBUS DEBUG : written BLE macAdddress sucessfully");
    }
  }else
  {
      if (debugMode)
      {
          //debugPrint("MODBUS DEBUG : invalid BLE MacAddress received");
      }
      
  }
}
/**
 * @brief 
 * 
 * @param macid : This method writes the wifi macAdddress to the holdingRegs
 */
void IUmodbus::updateWIFIMACAddress( MacAddress wifimac){
  if(wifimac){
    m_holdingRegs[MAC_ID_WIFI_1] = wifimac[0]<<8  | wifimac[1];               
    m_holdingRegs[MAC_ID_WIFI_2] = wifimac[2]<<8  | wifimac[3];               
    m_holdingRegs[MAC_ID_WIFI_3] = wifimac[4]<<8  | wifimac[5];               
    if (debugMode)
    {
        //debugPrint("MODBUS DEBUG : written WIFI macAdddress sucessfully");
    }
  }else
  {
      if (debugMode)
      {
          debugPrint("MODBUS DEBUG : invalid WIFI MacAddress received");
      }
      
  }
   

}
/**
 * @brief 
 * 
 * @param value 
 * @return uint16_t : 2 bytes float output
 */
uint16_t IUmodbus::updateFloatValues(float value){

    constructFloat.data = value;
    return constructFloat.reg[2];
}
/**
 * @brief 
 * 
 * @param groupNo : This indicate that the no of parameters Groups created in the MODBUS Memory mapping
 * @param startAddress : start of holdingRegs address
 * @param endAddress    : end of holdingRegs address
 * @return uint8_t 
 */
uint8_t IUmodbus::updateHoldingRegister(int groupNo, int startAddress, int endAddress,float* features){


switch (groupNo)
  {
    case  modbusGroups::MODBUS_STREAMING_FEATURES:
        for (int index = startAddress; index <= endAddress; index = index + 2)
        {

            updateFloatValues(*features );              // Feature buffer
            m_holdingRegs[index] = constructFloat.reg[0];     // L_BYTE
            m_holdingRegs[index + 1] = constructFloat.reg[1]; // H_BYTE
            features++;
        }
        // statements
        break;
    case modbusGroups::MODBUS_STREAMING_SPECTRAL_FEATURES:
        for (int index = startAddress; index <= endAddress; index = index + 2)
        {
            updateFloatValues(*features  );              // Feature buffer
            m_holdingRegs[index] = constructFloat.reg[0];     // L_BYTE
            m_holdingRegs[index + 1] = constructFloat.reg[1]; // H_BYTE
            features++;
            
        }
        break;
    case modbusGroups::MODBUS_STREAMING_REPORTABLE_DIAGNOSTIC:
        for(int index =startAddress; index <= endAddress; index++){
            m_holdingRegs[index] = int(*features);
            features++;
        }
        break;
           
    default:
        debugPrint("MODBUS DEBUG : Invalid Group No");
        break;
    }

}

uint8_t IUmodbus::clearHoldingRegister(int groupNo, int startAddress, int endAddress){


switch (groupNo)
  {
    case  modbusGroups::MODBUS_STREAMING_FEATURES:
        for (int index = startAddress; index <= endAddress; index = index + 2)
        {

            m_holdingRegs[index] = 0x00;     // L_BYTE
            m_holdingRegs[index + 1] =0x00; // H_BYTE
            
        }
        // statements
        break;
    case modbusGroups::MODBUS_STREAMING_SPECTRAL_FEATURES:
        for (int index = startAddress; index <= endAddress; index = index + 2)
        {
            m_holdingRegs[index] = 0x00;     // L_BYTE
            m_holdingRegs[index + 1] = 0x00; // H_BYTE
            
        }
        break;
           
    default:
        debugPrint("MODBUS DEBUG : Invalid Group No to cleared");
        break;
    }

}
