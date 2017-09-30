#ifndef IUI2C_H
#define IUI2C_H

#include <Arduino.h>
#include "Wire.h"
#include "Utilities.h"
#include "IUComponent.h"


namespace IUComponent
{
    /**
     * IU implementation of the board controller I2C
     *
     * Component:
     *   Name:
     *     I2C Computer bus
     *   Description:
     *     I2C computer bus for Butterfly
     *
     *  Comments:
     *    Butterfly board use STM32L4 chip and a specific Arduino core have been
     *    developed for it. See Thomas Roell (aka GumpyOldPizza) github:
     *    https://github.com/GrumpyOldPizza/arduino-STM32L4
     */
    class IUI2C : public ABCComponent
    {
        public:
            /***** Default settings *****/
            static const uint32_t defaultBaudRate = 115200;
            static const uint32_t defaultClockRate = 400000; // 400 kHz
            static constexpr HardwareSerial *port = &Serial;
            /***** Constructors & desctructors *****/
            IUI2C();
            virtual ~IUI2C() {}
            /***** Hardware and power management *****/
            virtual void wakeUp();
            virtual void sleep();
            virtual void suspend();
            void setClockRate( uint32_t clockRate);
            uint32_t getClockRate() { return m_clockRate; }
            /***** Communication with components *****/
            // Base functions
            bool writeByte(uint8_t address, uint8_t subAddress, uint8_t data);
            bool writeByte(uint8_t address, uint8_t subAddress, uint8_t data,
                           void(*callback)(uint8_t wireStatus));
            uint8_t readByte(uint8_t address, uint8_t subAddress);
            uint8_t readByte(uint8_t address, uint8_t subAddress,
                             void(*callback)(uint8_t wireStatus));
            bool readBytes(uint8_t address, uint8_t subAddress, uint8_t count,
                           uint8_t *destination);
            bool readBytes(uint8_t address, uint8_t subAddress, uint8_t count,
                           uint8_t *destination,
                           void(*callback)(uint8_t wireStatus));
            // Detection and identification
            bool scanDevices();
            bool checkComponentWhoAmI(String componentName, uint8_t address,
                                      uint8_t whoAmI, uint8_t iShouldBe);
            // Error handling
            byte getReadError() { return m_readError; }
            bool isReadError() { return m_readError != 0x00; }
            void resetReadError() { m_readError = 0; }
            /***** Communications through USB *****/
            // Baud Rate
            virtual void setBaudRate(uint32_t baudRate);
            virtual uint32_t getBaudRate() { return m_baudRate; }
            // Buffer handling
            void updateBuffer();
            void resetBuffer();
            String getBuffer() { return m_wireBuffer; }
            void printBuffer();
            // Send error message
            String getErrorMessage() { return m_errorMessage; }
            void setErrorMessage(String errorMessage)
                { m_errorMessage = errorMessage; }
            void resetErrorMessage() { m_errorMessage = "ALL_OK"; }
            // Verbosity setting
            void silence() { m_silent = true; }
            void unsilence() { m_silent = false; }
            bool isSilent() { return m_silent; }

        protected:
            /***** Hardware and power management *****/
            uint32_t m_clockRate;
            /***** Communication with components *****/
            bool m_readFlag;
            byte m_readError;
            /***** Communications through USB *****/
            bool m_begun;
            uint32_t m_baudRate;
            String m_wireBuffer;
            bool m_silent;
            String m_errorMessage;
    };

    /***** Instanciation *****/
    extern IUI2C iuI2C;

};

#endif // IUI2C_H
