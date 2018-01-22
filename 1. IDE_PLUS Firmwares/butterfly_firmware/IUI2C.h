#ifndef IUI2C_H
#define IUI2C_H

#include <Arduino.h>
#include "Wire.h"
#include "Utilities.h"
#include "Component.h"


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
class IUI2C : public Component
{
    public:
        /***** Default settings *****/
        static const uint32_t CLOCK_RATE = 400000; // 400 kHz
        /***** Constructors & desctructors *****/
        IUI2C();
        virtual ~IUI2C() {}
        /***** Hardware and power management *****/
        virtual void setupHardware();
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
        void endReadOperation() { m_readFlag = true; }
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
