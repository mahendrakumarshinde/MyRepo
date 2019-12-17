#ifndef UNITTESTUTILITIES_H_INCLUDED
#define UNITTESTUTILITIES_H_INCLUDED


#include <Arduino.h>
#include <ArduinoUnit.h>


class DummyHardwareSerial : public HardwareSerial
{
    public:
        /***** HardwareSerial behavior mimicking *****/
        static const uint8_t length = 255;
        DummyHardwareSerial() : m_begun(false) { resetBuffers(); }
        virtual void begin(unsigned long baudrate) {
            m_begun = true;
            m_baudRate = baudrate;
        }
        virtual void begin(unsigned long baudrate, uint16_t config) {
            begin(baudrate);
        }
        virtual void end() { m_begun = false; }
        virtual int available(void) { return m_rxTail - m_rxHead; }
        virtual int peek(void) { return (int) m_rxBuffer[m_rxHead]; }
        virtual int read(void) {
            if (available()) return (int) m_rxBuffer[m_rxHead++];
        }
        virtual void flush(void) { resetBuffers(); }
        virtual size_t write(uint8_t c) {
            if (m_txTail - m_txHead < length) {
                m_txBuffer[m_txTail++] = c;
            }
        }
        using Print::write; // pull in write(str) and write(buf, size)
        virtual operator bool() { return true; };
        /***** Test utilities *****/
        bool begun() { return m_begun; }
        uint32_t getBaudRate() { return m_baudRate; }
        void resetBuffers() {
            m_rxHead = m_rxTail = 0;
            m_txHead = m_txTail = 0;
            for (uint8_t i = 0; i < length; ++i)
            {
                m_rxBuffer[i] = 0;
                m_txBuffer[i] = 0;
            }
        }
        uint8_t getTxBuffer(char *outputBuffer, uint8_t maxLen) {
            uint8_t len = min(m_txTail - m_txHead, maxLen);
            strncpy(outputBuffer, m_txBuffer,  len);
            return len;
        }
        String getTxBuffer() {
            return String(m_txBuffer);
        }
        uint8_t setRxBuffer(const char *inputBuffer, uint8_t len) {
            m_rxHead = 0;
            m_rxTail = min(len, length);
            strncpy(m_rxBuffer, inputBuffer, m_rxTail);
            return m_rxTail;
        }
        uint8_t setRxBuffer(String inputBuffer, uint8_t len) {
            return setRxBuffer(inputBuffer.c_str(), len);
        }
        void putTxIntoRx() {
            m_rxHead = 0;
            m_rxTail = m_txTail - m_txHead;
            for (uint8_t i = 0; i < m_rxTail; ++i )
            {
                m_rxBuffer[i] = m_txBuffer[m_txHead + i];
            }
        }

    private:
        bool m_begun;
        uint32_t m_baudRate;
        char m_rxBuffer[length];
        uint8_t m_rxHead;
        uint8_t m_rxTail;
        char m_txBuffer[length];
        uint8_t m_txHead;
        uint8_t m_txTail;
};

test(DummyHardwareSerial_test)
{
    DummyHardwareSerial dhs;
    assertEqual(dhs.available(), 0);
    assertEqual(dhs.getTxBuffer().length(), 0);
    assertFalse(dhs.begun());

    dhs.begin(115200);
    assertTrue(dhs.begun());
    assertEqual(dhs.getBaudRate(), 115200);
    dhs.end();
    assertFalse(dhs.begun());


    // Rx buffer setting
    char buffer[13] = "Hello World!";
    dhs.setRxBuffer(buffer, 3);
    assertEqual(dhs.available(), 3);
    assertEqual(dhs.read(), 'H');
    assertEqual(dhs.available(), 2);
    assertEqual(dhs.read(), 'e');
    assertEqual(dhs.available(), 1);
    assertEqual(dhs.read(), 'l');
    assertEqual(dhs.available(), 0);

    dhs.setRxBuffer(buffer, 13);
    assertEqual(dhs.available(), 13);
    char c;
    while(dhs.available())
    {
        c = dhs.read();
    }
    assertEqual(c, 0);

    // Writting to Tx buffer
//    HardwareSerial *dhsPtr = &dhs;
    dhs.write("This is a test", 14);
    assertEqual(dhs.getTxBuffer().length(), 14);
    assertEqual(strncmp(dhs.getTxBuffer().c_str(), "This is a test", 14), 0);

    // Transferring Rx buffer to Tx buffer
    dhs.putTxIntoRx();
    assertEqual(dhs.available(), 14);
    assertEqual(dhs.read(), 'T');
    assertEqual(dhs.available(), 13);
    assertEqual(dhs.read(), 'h');
    assertEqual(dhs.available(), 12);
    assertEqual(dhs.read(), 'i');
    assertEqual(dhs.available(), 11);
}

#endif // UNITTESTUTILITIES_H_INCLUDED
