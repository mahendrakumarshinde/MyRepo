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
    enum txPowerOption : uint8_t {DB4   = 4,    // 4 DB
                                  DB0   = 0,    // 0 DB
                                  DBm4  = 252,  // -4 DB
                                  DBm16 = 240,  // -4 DB
                                  DBm30 = 226}; // -30 DB
    static const uint8_t bufferSize       = 19;
    // Device configuration
    static constexpr HardwareSerial *port = &Serial1;
    static const uint32_t defaultBaudRate = 57600;  // transmission rate through Serial1 to the BLE module
    static const uint8_t resetPin         = 38;     // BMD-350 reset pin active LOW
    static const uint8_t ATCmdPin         = 26;     // toggle pin for AT Command Interface
    // Beacon default configuration
    static const bool defaultBeaconEnabled = true;
    static const txPowerOption defaultBeaconTxPower = txPowerOption::DBm4;
    static const uint16_t defaultbeaconAdInterval = 100; // Valid values range from 50ms to 4000ms
    // UART default configuration
    static const bool defaultUARTEnabled = true;
    static const uint32_t defaultUARTBaudRate = 57600;
    static const bool defaultUARTFlowControl = false;
    static const bool defaultUARTParity = false;
    // Default connected Tx Power
    static const txPowerOption defaultConnectedTxPower = txPowerOption::DB4;

    // Constructors & destructor
    IUBMD350(IUI2C *iuI2C);
    virtual ~IUBMD350() {}
    virtual void setBaudRate(uint32_t baudRate);
    // Hardware and power management
    void softReset();
    virtual void wakeUp();
    virtual void sleep();
    virtual void suspend();
    // Configuration via AT Command
      void enterATCommandInterface();
      void exitATCommandInterface();
      bool isATCommandInterfaceEnabled() { return m_ATCmdEnabled; }
      int sendATCommand(String cmd, char *response, uint8_t responseLength);
      // BMDware info
      void getModuleInfo(char *BMDVersion, char *bootVersion,
                         char *protocolVersion, char *hardwareInfo,
                         uint8_t len1 = 11, uint8_t len2 = 21,
                         uint8_t len3 = 3, uint8_t len4 = 13);
      // Device name
      bool setDeviceName(char *deviceName);
      bool queryDeviceName();
      char* getDeviceName() { return m_deviceName; }
      // TODO Need to set / use password? If yes, see BMDWare datasheet
      //Set Password AT Command (1 to 19 byte alphanumeric): at$password
      // Beacon configuration
      bool configureBeacon(txPowerOption txPower, bool enabled, uint16_t adInterval,
                           char *UUID, char *major, char *minor);
      bool queryBeaconConfiguration();
      bool isBeaconEnabled() { return m_beaconEnabled; }
      txPowerOption getBeaconTxPower() { return m_beaconTxPower; }
      char* getUUID() { return m_beaconUUID; }
      char* getMajorNumber() { return m_beaconMajorNumber; }
      char* getMinorNumber() { return m_beaconMinorNumber; }
      // UART configuration
      bool configureUART(txPowerOption txPower, bool enabled, uint32_t baudRate,
                         bool flowControl, bool parity);
      bool queryUARTConfiguration();
      bool isUARTEnabled() { return m_UARTEnabled; }
      uint32_t getUARTBaudRate() { return m_UARTBaudRate; }
      bool inUARTFlowControlEnabled() { return m_UARTFlowControl; }
      bool isUARTParityEnabled() { return m_UARTParity; }
      // Connected Power
      bool setConnectedTxPower(txPowerOption txPower);
      bool queryConnectedTxPower();
      txPowerOption getConnectedTxPower() { return m_connectedTxPower; }
    // Communication methods
    void setDataReceptionTimeout(uint16_t dataRecTimeout) { m_dataReceptionTimeout = dataRecTimeout; }
    uint16_t getDataReceptionTimeout() { return m_dataReceptionTimeout; }
    bool checkDataReceptionTimeout();
    bool readToBuffer();
    char* getBuffer() { return m_buffer; }
    // Diagnostic Functions
    void exposeInfo();

  protected:
    // Main configuration
    IUI2C *m_iuI2C;
    bool m_ATCmdEnabled;
    char m_deviceName[9];           // max 8 chars + 1 char end of string
    // Beacon configuration
    txPowerOption m_beaconTxPower;
    bool m_beaconEnabled;
    uint16_t m_beaconAdInterval;
    char m_beaconUUID[33];          // 32 hex digits + 1 char end of string
    char m_beaconMajorNumber[5];    // 4 hex digits + 1 char end of string
    char m_beaconMinorNumber[5];    // 4 hex digits + 1 char end of string
    // UART Configuration
    bool m_UARTEnabled;
    uint32_t m_UARTBaudRate;
    bool m_UARTFlowControl;
    bool m_UARTParity;
    // Connected Power
    txPowerOption m_connectedTxPower;
    // Communication
    char m_buffer[bufferSize];      // BLE Data buffer
    uint8_t m_bufferIndex;          // BLE buffer Index
    // Data reception robustness variables: Buffer is cleansed if now - lastReadTime > dataReceptionTimeout
    uint16_t m_dataReceptionTimeout; // ms
    uint32_t m_lastReadTime;

};

#endif // IUBMD350_H

