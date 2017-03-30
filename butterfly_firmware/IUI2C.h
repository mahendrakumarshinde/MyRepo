/*
  Infinite Uptime Firmware

  Update:
    03/03/2017

  Component:
    Name:
      I2C Computer bus
    Description:
      I2C computer bus for Butterfly

   Comments:
     Butterfly board use STM32L4 chip and a specific Arduino core have been develop for it.
     See Thomas Roell (aka GumpyOldPizza) github: https://github.com/GrumpyOldPizza/arduino-STM32L4
*/

#ifndef IUI2C_H
#define IUI2C_H

#include <Arduino.h>
#include "Wire.h"
#include "IUUtilities.h"
#include "IUABCInterface.h"


/**
 *
 */
class IUI2C : public IUABCInterface
{
  public:
    // Data collection Mode - Modes of Operation String Constants
    const String START_COLLECTION = "IUCMD_START";
    const String START_CONFIRM = "IUOK_START";
    const String END_COLLECTION = "IUCMD_END";
    const String END_CONFIRM = "IUOK_END";
    static const uint32_t defaultBaudRate = 115200;
    static const uint32_t defaultClockRate = 400000; // frequency at 400 kHz
    static constexpr HardwareSerial *port = &Serial;
    // Constructors, getters and setters
    IUI2C();
    virtual ~IUI2C() {}
    virtual HardwareSerial* getPort() { return port; }
    void setClockRate( uint32_t clockRate);
    uint32_t getClockRate() { return m_clockRate; }
    byte getReadError() { return m_readError; }
    bool getReadFlag() { return m_readFlag; }
    void setReadFlag(bool val) { m_readFlag = val; }
    String getErrorMessage() { return m_errorMessage; }
    void setErrorMessage(String errorMessage) { m_errorMessage = errorMessage; }
    void resetErrorMessage() { m_errorMessage = "ALL_OK"; }
    void silence() { m_silent = true; }
    void unsilence() { m_silent = false; }
    bool isSilent() { return m_silent; }
    bool isReadError() { return m_readError != 0x00; }
    void resetReadError() { m_readError = 0; }
    void resetBuffer();
    String getBuffer() { return m_wireBuffer; }
    // Methods
    void activate();
    bool scanDevices();
    bool checkComponentWhoAmI(String componentName, uint8_t address, uint8_t whoAmI, uint8_t iShouldBe);
    bool checkIfStartCollection();
    bool checkIfEndCollection();
    void updateBuffer();
    void printBuffer();
    // I2C read / write methods
    void writeByte(uint8_t address, uint8_t subAddress, uint8_t data);
    void writeByte(uint8_t address, uint8_t subAddress, uint8_t data, void(*callback)(uint8_t wireStatus));
    uint8_t readByte(uint8_t address, uint8_t subAddress);
    uint8_t readByte(uint8_t address, uint8_t subAddress, void(*callback)(uint8_t wireStatus));
    void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t *destination);
    void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t *destination, void(*callback)(uint8_t wireStatus));
    

  protected:
    // Error Handling variables
    uint32_t m_clockRate;
    bool m_readFlag;
    bool m_silent;
    String m_wireBuffer;
    byte m_readError;
    String m_errorMessage;
};

#endif // IUI2C_H
