#ifndef IUBMD350_H
#define IUBMD350_H

#include <Arduino.h>

#include "IUSerial.h"
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
        static const uint8_t resetPin = 38; //  BMD-350 reset pin active LOW
        static const uint8_t ATCmdPin = 26; //  toggle pin for AT Command
        // UART default configuration - Must match Serial1 configuration
        static const bool UART_ENABLED = true;
        static const bool UART_FLOW_CONTROL = false;
        static const bool UART_PARITY = false;
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
        /***** Constructors & destructor *****/
        IUBMD350(HardwareSerial *serialPort, char *charBuffer,
                 uint16_t bufferSize, PROTOCOL_OPTIONS protocol,
                 uint32_t rate=57600, uint16_t dataReceptionTimeout=2000);
        virtual ~IUBMD350() {}
        /***** Hardware and power management *****/
        virtual void setupHardware();
        void softReset();
        virtual void wakeUp();
        virtual void sleep();
        virtual void suspend();
        /***** Bluetooth Configuration *****/
        // AT Command Interface
        void enterATCommandInterface();
        void exitATCommandInterface();
        int sendATCommand(String cmd, char *response, uint8_t responseLength);
        // Device name
        void setDeviceName(char *deviceName);
        void queryDeviceName();
        char* getDeviceName() { return m_deviceName; }
        /* TODO Need to set / use password? If yes, see BMDWare datasheet
        Set Password AT Command (1 to 19 byte alphanumeric): at$password */
        // Beacon and UART Passthrough
        void configureUARTPassthrough();
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

    protected:
        /***** Bluetooth Configuration *****/
        bool m_ATCmdEnabled;  // AT Command Interface
        char m_deviceName[9];  // max 8 chars + 1 char end of string
        bool m_beaconEnabled;
        uint16_t m_beaconAdInterval;
        txPowerOption m_beaconTxPower;
        txPowerOption m_connectedTxPower;
};


/***** Instanciation *****/

extern char iuBluetoothBuffer[500];
extern IUBMD350 iuBluetooth;

#endif // IUBMD350_H

