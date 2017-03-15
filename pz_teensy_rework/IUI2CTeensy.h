#ifndef IUI2CTEENSY_H
#define IUI2CTEENSY_H

#include <Arduino.h>
#include <i2c_t3.h>

class IUI2CTeensy
{
  public:
    // Data collection Mode - Modes of Operation String Constants
    const String START_COLLECTION = "IUCMD_START";
    const String START_CONFIRM = "IUOK_START";
    const String END_COLLECTION = "IUCMD_END";
    const String END_CONFIRM = "IUOK_END";
    IUI2CTeensy();
    IUI2CTeensy(bool silent, Stream *port);
    virtual ~IUI2CTeensy();
    
    Stream *port;
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

    void activate();
    bool scanDevices();
    void writeByte(uint8_t address, uint8_t subAddress, uint8_t data);
    uint8_t readByte(uint8_t address, uint8_t subAddress);
    void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t *destination);
    bool checkIfStartCollection();
    bool checkIfEndCollection();
    void updateBuffer();
    void printBuffer();

  private:
    // Error Handling variables
    bool m_readFlag;
    bool m_silent;
    String m_wireBuffer;
    byte m_readError;
    String m_errorMessage;
};

#endif // IUI2CTEENSY_H
