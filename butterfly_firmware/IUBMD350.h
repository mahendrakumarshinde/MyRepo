/*
  Infinite Uptime Firmware

  Update:
    03/03/2017

  Component:
    Name:
      BMD-350
    Description:
      Bluetooth Low Energy for Butterfly board
      BMD-350 UART connected to Serial 2 (pins 30/31) on Butterfly
*/

#ifndef IUBMD350_H
#define IUBMD350_H

#include <Arduino.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>
#include "IUI2C.h"
#include "IUABCInterface.h"

class IUBMD350 : public IUABCInterface
{
  public:
    struct BooleanSetting
    {
      String name;
      String command;
      bool state;
    };

    static constexpr HardwareSerial *port = &Serial1;
    static const uint8_t bufferSize       = 19;
    // Device configuration
    static const uint8_t resetPin         = 38;     // BMD-350 reset pin active LOW
    static const uint8_t ATCmdPin         = 26;     // toggle pin for AT Command Interface
    // Default Config
    static const int8_t defaultUARTTxPower = 4;
    static const int8_t defaultBeaconTxPower = 4;
    static const uint32_t defaultBaudRate = 57600;      // transmission rate through Serial1 to the BLE module
    static const uint32_t defaultBLEBaudRate = 57600;   // transmission rate through bluetooth (from BLE module to BLE receiver)
    static const bool defaultFlowControlEnable = false;
    static const bool defaultParityEnable = false;
    static const uint8_t booleanSettingCount = 3;
    static BooleanSetting noSetting;

    //Constructors, getters and setters
    IUBMD350(IUI2C *iuI2C);
    virtual ~IUBMD350() {}
    virtual HardwareSerial* getPort() { return port; }
    // Hardware and power management
    void softReset();
    virtual void wakeUp();
    virtual void sleep();
    virtual void suspend();
    // TODO Need to set / use password? If yes, see BMDWare datasheet
    //Set Password AT Command (1 to 19 byte alphanumeric): at$password
    void enterATCommandInterface();
    void exitATCommandInterface();
    bool isATCommandInterfaceEnabled() { return m_ATCmdEnabled; }
    int sendATCommand(String cmd, char *response, uint8_t responseLength);
    void getModuleInfo(char *BMDVersion, char *bootVersion, char *protocolVersion, char *hardwareInfo,
                       uint8_t len1 = 11, uint8_t len2 = 21, uint8_t len3 = 3, uint8_t len4 = 13);
    bool setDeviceName(char *deviceName);
    bool queryDeviceName();
    char* getDeviceName() { return m_deviceName; }
    bool setTxPower(int8_t txPower, bool beacon=false);
    bool queryTxPower(bool beacon=false);
    int8_t getBeaconTxPower() { return m_beaconTxPower; }
    int8_t getUARTTxPower() { return m_UARTTxPower; }
    bool setUUIDInfo(char *UUID, char *major, char *minor);
    bool queryUUIDInfo();
    char* getUUID() { return m_UUID; }
    char* getMajorNumber() { return m_majorNumber; }
    char* getMinorNumber() { return m_minorNumber; }
    bool setUARTSettings(uint32_t baudRate, bool flowControl, bool parity, bool passThrough);
    bool queryUARTSettings();
    uint32_t getUARTBaudRate() { return m_UARTBaudRate; }
    bool inUARTFlowControlEnabled() { return m_UARTFlowControl; }
    bool isUARTParityEnabled() { return m_UARTParity; }
    bool isUARTEnabled() { return m_UARTMode; }




    bool setBLEBaudRate(uint32_t baudRate);
    uint32_t getBLEBaudRate() { return m_baudRate; }
    uint32_t queryBLEBaudRate();
    BooleanSetting getBooleanSettings(String settingName);
    bool setBooleanSettings(String settingName, bool enable);
    bool queryBooleanSettingState(String settingName);




    uint16_t getDataReceptionTimeout() { return m_dataReceptionTimeout; }
    void setDataReceptionTimeout(uint16_t dataRecTimeout) { m_dataReceptionTimeout = dataRecTimeout; }
    char* getBuffer() { return m_buffer; }
    // Methods
    void activate();
    bool checkDataReceptionTimeout();
    bool readToBuffer();
    void printFigures(uint16_t buffSize, q15_t *buff);
    void printFigures(uint16_t buffSize, q15_t *buff, float (*transform)(int16_t));
    // Public members
    BooleanSetting settings[booleanSettingCount];

    // Diagnostic Functions
    void exposeInfo();

  protected:
    // Main configuration
    IUI2C *m_iuI2C;
    bool m_ATCmdEnabled;
    char m_deviceName[9];     // max 8 chars + 1 char end of string
    char m_UUID[33];          // 32 hex digits + 1 char end of string
    char m_majorNumber[5];    // 4 hex digits + 1 char end of string
    char m_minorNumber[5];    // 4 hex digits + 1 char end of string
    // Power Management
    powerMode m_status
    int8_t m_beaconTxPower;
    int8_t m_UARTTxPower;
    // UART Configuration
    uint32_t m_UARTBaudRate;
    bool m_UARTFlowControl;
    bool m_UARTParity;
    bool m_UARTMode;
    // Communication
    char m_buffer[bufferSize];  // BLE Data buffer
    uint8_t m_bufferIndex; // BLE buffer Index
    // Data reception robustness variables: Buffer is cleansed if now - lastReadTime > dataReceptionTimeout
    uint16_t m_dataReceptionTimeout; // ms
    uint32_t m_lastReadTime;

};

#endif // IUBMD350_H

