/*
  Infinite Uptime Firmware

  Update:
    03/03/2017

  Component:
    Name:
      BMD-350
    Description:
      Bluetooth Low Energy for Butterfly board
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
    static const uint8_t bufferSize = 19;
    // Device configuration
    static const uint8_t resetPin         = 38;     // BMD-350 reset pin active LOW
    static const uint8_t ATCmdPin         = 26;     // toggle pin for AT Command Interface
    // Default Config
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
    void resetDevice();
    // TODO Need to set / use password? If yes, see BMDWare datasheet
    //Set Password AT Command (1 to 19 byte alphanumeric): at$password
    void enterATCommandInterface();
    void exitATCommandInterface();
    bool isATCommandInterfaceEnabled() { return m_ATCmdEnabled; }
    bool sendATCommand(String cmd, String *result);
    void getModuleInfo(String *BMDVersion, String *bootVersion, String *protocolVersion, String *hardwareInfo);
    bool setDeviceName(String name);
    String getDeviceName() { return m_deviceName; }
    String queryDeviceName();
    bool setUUIDInfo(String UUID, String major, String minor);
    bool queryUUIDInfo();
    String getUUID() { return m_UUID; }
    String getMajorNumber() { return m_majorNumber; }
    String getMinorNumber() { return m_minorNumber; }
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

  protected:
    IUI2C *m_iuI2C;
    bool m_ATCmdEnabled;
    String m_deviceName;
    String m_UUID;
    String m_majorNumber;
    String m_minorNumber;
    uint32_t m_BLEBaudRate;
    char m_buffer[bufferSize];  // BLE Data buffer
    uint8_t m_bufferIndex; // BLE buffer Index
    //Data reception robustness variables: Buffer is cleansed if now - lastReadTime > dataReceptionTimeout
    uint16_t m_dataReceptionTimeout; // ms
    uint32_t m_lastReadTime;

};

#endif // IUBMD350_H

