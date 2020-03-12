#ifndef IUBMD350_H
#define IUBMD350_H

#include <Arduino.h>

#include <IUSerial.h>
#include "Component.h"


/**
 * BLE chip
 *
 * Component:
 *   Name:
 *   BMD-350
 * Description:
 *   Bluetooth Low Energy for Butterfly board
 *   BMD-350 UART connected to Serial 2 (pins 30/31) on Butterfly
 */
class IUBMD350 : public IUSerial, public Component
{
    public:
        /***** Preset values and default settings *****/
        // UART default configuration - Must match Serial configuration
        static const bool UART_ENABLED = true;
        static const bool UART_FLOW_CONTROL = false;
        static const bool UART_PARITY = false;
        bool blePowerStatus = false;
        // Beacon default configuration
        static const bool defaultBeaconEnabled = true;
        // Ad Interval valid values range from 50ms to 4000ms
        static const uint16_t defaultbeaconAdInterval = 100;
        // Available tx Power settings
        enum txPowerOption : uint8_t {DB4   = 4,    // 4 DB
                                      DB0   = 0,    // 0 DB
                                      DBm4  = 252,  // -4 DB
                                      DBm16 = 240,  // -4 DB
                                      DBm30 = 226}; // -30 DB
        static const txPowerOption defaultTxPower = txPowerOption::DBm4;
        /***** Bluetooth throughput control *****/
        static const uint16_t TX_BUFFER_LENGTH = 12000;
        static const uint16_t SERIAL_TX_MAX_AVAILABLE = 64;
        static const uint16_t BLE_MTU_LEN = 20;
        static const uint32_t BLE_TX_DELAY = 25;
        /***** Constructors & destructor *****/
        IUBMD350(HardwareSerial *serialPort, char *charBuffer,
                 uint16_t bufferSize, PROTOCOL_OPTIONS protocol, uint32_t rate,
                 char stopChar, uint16_t dataReceptionTimeout, uint8_t resetPin,
                 uint8_t atCmdPin);
        virtual ~IUBMD350() {}
        /***** Hardware and power management *****/
        virtual void setupHardware();
        void softReset();
        bool bleButton(bool); // use for ble ON and OFF by passing true to ON and false to OFF
        virtual void setPowerMode(PowerMode::option pMode);
        /***** Bluetooth throughput control *****/
        void resetTxBuffer();
        virtual size_t write(const char c);
        virtual size_t write(const char *msg);
        void bleTransmit();
        /***** AT Command Interface *****/
        void enterATCommandInterface(uint8_t retry=1);
        void exitATCommandInterface();
        int sendATCommand(String cmd, char *response, uint8_t responseLength);
        /***** Bluetooth Configuration *****/
        virtual void doFullConfig();
        // Device name
        void setDeviceName(char *deviceName);
        void queryDeviceName();
        char* getDeviceName() { return m_deviceName; }
        /* TODO Need to set / use password? If yes, see BMDWare datasheet
        Set Password AT Command (1 to 19 byte alphanumeric): at$password */
        // Beacon and UART Passthrough
        void configureUARTPassthrough();
        void bleBeaconSetting(bool BeaconSet); //Beacon on and off
        void configureBeacon(bool enabled, uint16_t adInterval);
        void setBeaconUUID(char *UUID, char *major, char *minor);
        // Tx Powers
        void setTxPowers(txPowerOption txPower);
        void queryTxPowers();
        txPowerOption getConnectedTxPower() { return m_connectedTxPower; }
        txPowerOption getBeaconTxPower() { return m_beaconTxPower; }
        /***** Debugging *****/
        void printUARTConfiguration();
        void printBeaconConfiguration();
        void exposeInfo();
        void getBMDwareInfo(char *BMDVersion, char *bootVersion,
                            char *protocolVersion, char *hardwareInfo,
                            uint8_t len1 = 11, uint8_t len2 = 21,
                            uint8_t len3 = 3, uint8_t len4 = 13);
        //Bluetooth Available
        bool isBLEAvailable = true;
    protected:
        /***** Pin definition *****/
        uint8_t m_resetPin;  // BMD-350 reset pin active LOW
        uint8_t m_atCmdPin;  // Toggle pin for AT Command
        /***** Bluetooth throughput control *****/
        char m_txBuffer[TX_BUFFER_LENGTH];
        uint16_t m_txHead;
        uint16_t m_txTail;
        bool m_serialTxEmpty;
        uint32_t m_lastSerialTxEmptied;
        /***** Bluetooth Configuration *****/
        bool m_ATCmdEnabled;  // AT Command Interface
        char m_deviceName[9];  // max 8 chars + 1 char end of string
        bool m_beaconEnabled;
        uint16_t m_beaconAdInterval;
        txPowerOption m_beaconTxPower;
        txPowerOption m_connectedTxPower;
        
};

#endif // IUBMD350_H

