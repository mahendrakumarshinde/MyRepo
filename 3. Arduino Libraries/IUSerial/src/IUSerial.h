#ifndef IUSERIAL_H
#define IUSERIAL_H

#include <Arduino.h>
#include <IPAddress.h>
#include <MacAddress.h>

#include "MSPCommands.h"
#include "IUDebugger.h"


/**
 * Custom implementation of Serial port (UART)
 *
 * Class implements function to read data from Serial using a protocol.
 * Protocol are implemented by default and child class can define their custom
 * protocol.
 */
class IUSerial
{
    public:
        /***** Public constants *****/
        HardwareSerial *port;
        const uint32_t baudRate;
        // MS_PROTOCOL: MultiWii Serial Protocol. Note that current
        // implementation skips the variable type in the header.
        enum PROTOCOL_OPTIONS : uint8_t {LEGACY_PROTOCOL,
                                         MS_PROTOCOL,
                                         CUSTOM_PROTOCOL};
        enum MSP_STATE : uint8_t {MSP_IDLE,
                                  MSP_HEADER_START,
                                  MSP_HEADER_M,
                                  MSP_HEADER_ARROW,
                                  MSP_HEADER_SIZE_1,
                                  MSP_HEADER_SIZE_2,
                                  MSP_HEADER_CMD};

        /***** Message processing callback *****/
        typedef void (*newMsgCBSignature)(IUSerial *iuSerial);

        /***** Core *****/
        IUSerial(HardwareSerial *serialPort, char *charBuffer,
                 uint16_t bufferSize, PROTOCOL_OPTIONS protocol, uint32_t rate,
                 char stopChar, uint16_t dataReceptionTimeout);
        virtual ~IUSerial() {}
        virtual void begin();
        PROTOCOL_OPTIONS getProtocol() { return m_protocol; }
        void setOnNewMessageCallback(newMsgCBSignature cb)
            { m_newMessageCB = cb; }
        /***** Communication *****/
        virtual void resetBuffer();
        virtual bool readUptoOneMessage();
        virtual bool readMessages();
        virtual bool hasNewMessage() { return m_newMessage; }
        virtual char* getBuffer() { return m_buffer; }
        virtual size_t write(const char c) { return port->write(c); }
        virtual size_t write(const char *msg) { return port->write(msg); }
        // The length of the buffer, including the null terminating char
        virtual uint16_t getCurrentBufferLength() { return m_bufferIndex; }
        virtual bool processMessage() { return false; }
        /***** Logging functionnality *****/
        virtual void log(const char* msg);
        /***** MSP commands *****/
        virtual MSPCommand::command getMspCommand() { return m_mspCommand; }
        virtual void sendMSPCommand(MSPCommand::command cmd);
        virtual void sendMSPCommand(MSPCommand::command cmd, const char* cmdMsg,
                                    uint16_t cmdSize);
        virtual void sendMSPCommand(MSPCommand::command cmd,
                                    const char* cmdMsg);
        virtual void sendLongMSPCommand(MSPCommand::command cmd,
                                        uint32_t microTimeout,
                                        const char* cmdMsg, uint16_t cmdSize);
        // Send MSP command by chunks: note that you must already know the
        // message length when starting the command.
        virtual void startLiveMSPCommand(MSPCommand::command cmd,
                                         uint16_t cmdSize);
        virtual void streamLiveMSPMessage(char c);
        virtual void streamLiveMSPMessage(const char* msg, size_t length);
        virtual void streamLiveMSPMessage(const char* msg)
            { streamLiveMSPMessage(msg, strlen(msg)); }
        virtual void streamLiveMSPMessage(String msg)
            { streamLiveMSPMessage(msg.c_str()); }
        virtual bool endLiveMSPCommand();
        /***** Convenience MSP functions *****/
        virtual void mspSendMacAddress(MSPCommand::command cmd, MacAddress mac);
        virtual void mspSendIPAddress(MSPCommand::command cmd, IPAddress ip);
        virtual MacAddress mspReadMacAddress();
        virtual IPAddress mspReadIPAddress();

    protected:
        /***** Communication *****/
        char *m_buffer;
        uint16_t m_bufferSize;
        bool m_newMessage;
        uint16_t m_bufferIndex;
        /***** Message processing callback *****/
        newMsgCBSignature m_newMessageCB = NULL;
        /***** Logging functionnality *****/
        virtual void m_customProtocolLog(const char *msg) {
            if (debugMode) {
                debugPrint("Custom protocol log not implemented");
            }
        }
        /***** Data reception robustness variables *****/
        // Buffer is emptied if now - lastReadTime > dataReceptionTimeout
        uint16_t m_dataReceptionTimeout;  // in ms
        uint32_t m_lastReadTime;  // in ms
        /***** Protocol selection *****/
        PROTOCOL_OPTIONS m_protocol;
        /***** Legacy Protocol *****/
        virtual bool readCharLegacyProtocol();
        char m_stopChar;
        /***** MSP (Multiwii Serial Protocol) *****/
        virtual bool readCharMsp();
        virtual size_t sendMspCommandHeader(uint16_t cmdSize,
                                            MSPCommand::command cmd);
        virtual size_t sendMspCommandTail();
        virtual size_t mspChecksumAndSend(uint8_t b);
        MSP_STATE m_mspState = MSP_IDLE;
        MSPCommand::command m_mspCommand = MSPCommand::NONE;
        // 2 bytes command size: manually handled as MSB (no union because bit
        // order is implementation specific)
        uint16_t m_mspDataSize;
        uint8_t m_mspDataSizeByte0;
        uint8_t m_mspDataSizeByte1;
        uint8_t m_mspChecksumIn = 0;
        uint8_t m_mspChecksumOut = 0;
        // Live MSP command
        uint16_t m_expectedLiveMspCmdSize = 0;
        uint16_t m_actualLiveMspCmdSize = 0;
        /***** Custom Protocol *****/
        virtual bool readCharCustomProtocol();
};

#endif // IUSERIAL_H
