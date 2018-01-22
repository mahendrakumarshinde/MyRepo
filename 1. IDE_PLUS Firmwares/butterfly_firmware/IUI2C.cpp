#include "IUI2C.h"


/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUI2C::IUI2C() :
    Component(),
    m_readFlag(true),
    m_errorFlag(false)
{
}


/* =============================================================================
    Hardware and power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUI2C::setupHardware()
{
    Wire.begin(TWI_PINS_20_21); // set master mode on pins 21/20
    Wire.setClock(CLOCK_RATE);
    delay(2000);
    wakeUp();
}


/* =============================================================================
    Communication with components
============================================================================= */

/***** Base functions *****/
//Speed should be around 100kB/s


/**
 * Write a byte to given address and sub-address
 */
bool IUI2C::writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
{
    uint8_t temp[2];
    temp[0] = subAddress;
    temp[1] = data;
    byte errorCode = Wire.transfer(address, &temp[0], 2, NULL, 0);
    if (errorCode != 0x00)
    {
        m_errorFlag = true;
        if (debugMode)
        {
            debugPrint("I2C error Code:", false);
            debugPrint(errorCode);
        }
        return false;
    }
    return true;
}

/**
 * Write a byte to given address and sub-address
 */
bool IUI2C::writeByte(uint8_t address, uint8_t subAddress, uint8_t data,
                      void(*callback)(uint8_t wireStatus))
{
    uint8_t temp[2];
    temp[0] = subAddress;
    temp[1] = data;
    bool success = Wire.transfer(address, &temp[0], 2, NULL, 0, true, callback);
    if (!success)
    {
        if (callbackDebugMode)
        {
            debugPrint(F("I2C error"));
        }
    }
    return success;
}

/**
 * Read a single byte and return it
 *
 * @param address Where to read the byte from
 * @param subAddress Where to read the byte from
 */
uint8_t IUI2C::readByte(uint8_t address, uint8_t subAddress)
{
    uint8_t temp[1];
    byte errorCode = Wire.transfer(address, &subAddress, 1, &temp[0], 1);
    if (isError())
    {
        m_errorFlag = true;
        if (debugMode)
        {
            debugPrint("I2C error code:", false);
            debugPrint(errorCode);
        }
        return 0;
    }
    return temp[0];
}

/**
 * Read a single byte and return it
 *
 * @param address Where to read the byte from
 * @param subAddress Where to read the byte from
 */
uint8_t IUI2C::readByte(uint8_t address, uint8_t subAddress,
                        void(*callback)(uint8_t wireStatus))
{
    uint8_t temp[1];
    if (!Wire.transfer(address, &subAddress, 1, &temp[0], 1, true, callback))
    {
        if (callbackDebugMode)
        {
            debugPrint(F("I2C error"));
        }
        return 0;
    }
    return temp[0];
}

/**
 * Read several bytes and store them in destination array
 *
 * @param address Where to read the byte from
 * @param subAddress Where to read the byte from
 * @param The number of byte to read
 */
bool IUI2C::readBytes(uint8_t address, uint8_t subAddress, uint8_t count,
                      uint8_t *destination)
{
    if (!m_readFlag)
    {
        // Restrain multiple simultoneous read accesses to I2C bus
        return false;
    }
    m_readFlag = false;
    byte errorCode = Wire.transfer(address, &subAddress, 1, destination, count);
    m_readFlag = true;
    if (errorCode != 0x00)
    {
        m_errorFlag = true;
        if (debugMode)
        {
            debugPrint("I2C error code:", false);
            debugPrint(errorCode);
        }
        return false;
    }
    return true;
}

/**
 * Read several bytes and store them in destination array
 *
 * Note that the callback function MUST call endReadOperation to free the I2C.
 *
 * @param address Where to read the byte from
 * @param subAddress Where to read the byte from
 * @param The number of byte to read
 */
bool IUI2C::readBytes(uint8_t address, uint8_t subAddress, uint8_t count,
                      uint8_t *destination, void(*callback)(uint8_t wireStatus))
{
    if (!m_readFlag)
    {
        // Restrain multiple simultoneous read accesses to I2C bus
        return false;
    }
    m_readFlag = false;
    bool success = Wire.transfer(address, &subAddress, 1, destination, count,
                                 true, callback);
    if (!success)
    {
        if (callbackDebugMode)
        {
            debugPrint(F("I2C error"));
        }
    }
    return success;
}


/***** Detection and identification *****/

/**
* Scan components connected to the computer bus
*
* @return False if no device found else true
*/
bool IUI2C::scanDevices()
{
    if (setupDebugMode)
    {
        debugPrint(F("I2C Scanning..."));
    }
    byte error, address;
    int nDevices = 0;
    for (address = 1; address < 127; address++ )
    {
        // The i2c_scanner uses the return value of the Wire.endTransmission to
        // see if a device did acknowledge to the address.
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        if (error == 0)
        {
            if (setupDebugMode)
            {
                debugPrint(F("  Device found at address 0x"), false);
                if (address < 16) debugPrint(F("0"), false);
                debugPrint(String(address, HEX));
            }
            nDevices++;
        }
        else if (error == 4)
        {
            if (setupDebugMode)
            {
                debugPrint("Error at address 0x", false);
                if (address < 16) debugPrint("0", false);
                debugPrint(address, HEX);
            }
        }
    }
    if (nDevices == 0)
    {
        if (setupDebugMode)
        {
            debugPrint("No I2C devices found\n");
        }
        return false;
    }
    return true;
}

/**
 * Read the WHO_AM_I register (this is a good test of communication)
 *
 * @return True if the WHO_AM_I register read is correct, else false
 */
bool IUI2C::checkComponentWhoAmI(String componentName, uint8_t address,
                                 uint8_t whoAmI, uint8_t iShouldBe)
{
    byte c = readByte(address, whoAmI);  // Read WHO_AM_I register for component
    if (setupDebugMode)
    {
        debugPrint(componentName, false);
        debugPrint(F(": I AM 0x"), false);
        debugPrint(c, false);
        debugPrint(F(", I should be 0x"), false);
        debugPrint(iShouldBe);
    }
    if (c != iShouldBe)
    {
        if (setupDebugMode)
        {
            debugPrint(F("Could not connect to "), false);
            debugPrint(componentName, false);
            debugPrint(F(": 0x"), false);
            debugPrint(String(c, HEX));
        }
        return false;
    }
    return true;
}

/* =============================================================================
    Instanciation
============================================================================= */

IUI2C iuI2C = IUI2C();
