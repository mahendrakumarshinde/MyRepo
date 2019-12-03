#ifndef IUI2C_H
#define IUI2C_H

#include <Arduino.h>
#include <Wire.h>
#include <IUDebugger.h>


/**
 * IU implementation of the board controller I2C Computer bus
 *
 * IU STM32L4 chips uses a specific Arduino core. See Thomas Roell (aka
 * GumpyOldPizza) github: https://github.com/GrumpyOldPizza/arduino-STM32L4
 */
class IUI2C
{
    public:
        /***** Default settings *****/
        static const uint32_t CLOCK_RATE = 1000000; // 400 kHz
        /***** Core *****/
        IUI2C();
        virtual ~IUI2C() {}
        virtual void begin();
        /***** Communication with components *****/
        // Base functions
        bool writeByte(uint8_t address, uint8_t subAddress, uint8_t data);
        bool writeByte(uint8_t address, uint8_t subAddress, uint8_t data,
                       void(*callback)(uint8_t wireStatus));
        bool writeWord(uint8_t address, uint8_t subAddress, uint16_t data);
        uint8_t readByte(uint8_t address, uint8_t subAddress);
        uint8_t readByte(uint8_t address, uint8_t subAddress,
                         void(*callback)(uint8_t wireStatus));
        bool readBytes(uint8_t address, uint8_t subAddress, uint8_t count,
                       uint8_t *destination);
        bool readBytes(uint8_t address, uint8_t subAddress, uint8_t count,
                       uint8_t *destination,
                       void(*callback)(uint8_t wireStatus));
        void releaseReadLock() { m_readFlag = true; }
        // Detection and identification
        bool scanDevices();
        bool checkComponentWhoAmI(String componentName, uint8_t address,
                                  uint8_t whoAmI, uint8_t iShouldBe);
        // Status
        bool isError() { return m_errorFlag; }
        void resetErrorFlag() { m_errorFlag = false; }

    protected:
        /***** Communication with components *****/
        bool m_readFlag;
        bool m_errorFlag;
};


/***** Instanciation *****/

extern IUI2C iuI2C;

#endif // IUI2C_H
