#include "IUI2C.h"

IUI2C::IUI2C() :
  IUABCInterface(),
  m_readFlag(true),
  m_silent(false),
  m_wireBuffer("")
{
  Wire.begin(TWI_PINS_20_21); // set master mode on pins 21/20
  setClockRate(defaultClockRate);
  m_baudRate = IUI2C::defaultBaudRate;
  port->begin(m_baudRate);
  delay(4000);
  int intPin   =   7; 
  pinMode(intPin,  INPUT);
}

/**
 * Set the I2C frequency
 * This includes a delay.
 */
void IUI2C::setClockRate( uint32_t clockRate)
{
  m_clockRate = clockRate;
  Wire.setClock(m_clockRate);
  delay(2000);
}

void IUI2C::activate()
{
  // Nothing to do
}

/**
* Scan components connected to the computer bus
* @return false if no device found else true
*/
bool IUI2C::scanDevices()
{
  // scan for i2c devices
  if (!m_silent) port->println("Scanning...");

  byte error, address;
  int nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Wire.endTransmission to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {
      if (!m_silent) {
        port->print("I2C device found at address 0x");
        if (address < 16) { port->print("0"); }
        port->print(address, HEX);
        port->println(" !");
      }
      nDevices++;
    }
    else if (error == 4)
    {
      if (!m_silent) {
        port->print("Unknown error at address 0x");
        if (address < 16) port->print("0");
        port->println(address, HEX);
      }
    }
  }
  if (nDevices == 0) {
    if (!m_silent) port->println("No I2C devices found\n");
    return false;
  }
  if (!m_silent) port->println("done\n");
  return true;
}

/**
 * Read the WHO_AM_I register (this is a good test of communication)
 * @return true if the WHO_AM_I register read is correct, else false
 */
bool IUI2C::checkComponentWhoAmI(String componentName, uint8_t address, uint8_t whoAmI, uint8_t iShouldBe)
{
  byte c = readByte(address, whoAmI);  // Read ACC WHO_AM_I register for componant
  if (!m_silent)
  {
    port->print(componentName);
    port->print(" I AM 0x");
    port->print(c, HEX);
    port->print(" I should be 0x");
    port->println(iShouldBe, HEX);
    port->flush();
  }
  if (c != iShouldBe)
  {
    port->print("Could not connect to ");
    port->print(componentName);
    port->print(": 0x");
    port->println(c, HEX);
    port->flush();
    return false;
  }
  return true;
}

/* ========================= I2C read/write functions ========================= */
//Speed should be around 100kB/s


/**
 * Write a byte to given address and sub-address
 */
bool IUI2C::writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
{
  uint8_t temp[2];
  temp[0] = subAddress;
  temp[1] = data;
  m_readError = Wire.transfer(address, &temp[0], 2, NULL, 0);
  if (isReadError())
  {
    if (!m_silent)
    {
      port->print("Error Code:");
      port->println(m_readError);
    }
    m_errorMessage = "I2CERR";
    resetReadError();
    return false;
  }
  return true;
}

/**
 * Write a byte to given address and sub-address
 */
bool IUI2C::writeByte(uint8_t address, uint8_t subAddress, uint8_t data, void(*callback)(uint8_t wireStatus))
{
  uint8_t temp[2];
  temp[0] = subAddress;
  temp[1] = data;
  bool success = Wire.transfer(address, &temp[0], 2, NULL, 0, true, callback);
  if (!success)
  {
    if (callbackDebugMode) { debugPrint(F("I2CERR")); }
  }
  return success;
}

/**
 * Read a single byte and return it
 * @param address where to read the byte from
 * @param subAddress where to read the byte from
 */
uint8_t IUI2C::readByte(uint8_t address, uint8_t subAddress)
{
  uint8_t temp[1];
  m_readError = Wire.transfer(address, &subAddress, 1, &temp[0], 1);
  if (isReadError())
  {
    if (!m_silent)
    {
      port->print("Error Code:");
      port->println(m_readError);
    }
    m_errorMessage = "I2CERR";
    resetReadError();
    return 0;
  }
  return temp[0];
}

/**
 * Read a single byte and return it
 * @param address where to read the byte from
 * @param subAddress where to read the byte from
 */
uint8_t IUI2C::readByte(uint8_t address, uint8_t subAddress, void(*callback)(uint8_t wireStatus))
{
  uint8_t temp[1];
  if (!Wire.transfer(address, &subAddress, 1, &temp[0], 1, true, callback))
  {
    if (callbackDebugMode) { debugPrint(F("I2CERR")); }
    return 0;
  }
  return temp[0];
}

/**
 * Read several bytes and store them in destination array
 * @param address where to read the byte from
 * @param subAddress where to read the byte from
 * @param the number of byte to read
 */
bool IUI2C::readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t *destination)
{
  if (!m_readFlag) // To restrain multiple simultoneous read accesses to I2C bus
  {
    return false;
  }
  m_readFlag = false;
  m_readError = Wire.transfer(address, &subAddress, 1, destination, count);
  bool success = (!isReadError());
  if (!success)
  {
    if (!m_silent)
    {
      port->print("Error Code:");
      port->println(m_readError);
    }
    m_errorMessage = "I2CERR";
    resetReadError();
  }
  m_readFlag = true;
  return success;
}

/**
 * Read several bytes and store them in destination array
 * @param address where to read the byte from
 * @param subAddress where to read the byte from
 * @param the number of byte to read
 */
bool IUI2C::readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t *destination, void(*callback)(uint8_t wireStatus))
{
  if (!m_readFlag) // To restrain multiple simultoneous read accesses to I2C bus
  {
    return false;
  }
  m_readFlag = false;
  bool success = Wire.transfer(address, &subAddress, 1, destination, count, true, callback);
  if (!success)
  {
    if (callbackDebugMode) { debugPrint(F("I2CERR")); }
  }
  m_readFlag = true;
  return success;
}

/* ------------- Hardwire Serial for DATA_COLLECTION commands ------------- */

/**
 * Read port (serial) and write the output in the wire buffer
 */
void IUI2C::updateBuffer()
{
  while (port->available() > 0) {
    char characterRead = port->read();
    if (characterRead != '\n')
      m_wireBuffer.concat(characterRead);
  }
}

/**
 * Empty the wire buffer
 */
void IUI2C::resetBuffer()
{
  m_wireBuffer = "";
}

/**
 * Print the wire buffer to port (serial)
 */
void IUI2C::printBuffer()
{
  port->println(m_wireBuffer);
}

/**
 * Check if we should start data collection, (ie enter collection mode)
 */
bool IUI2C::checkIfStartCollection()
{
  // Check the received info; iff data collection request, change the mode
  if (m_wireBuffer.indexOf(START_COLLECTION) > -1)
  {
    port->println(START_CONFIRM);
    return true;
  }
  return false;
}

/**
 * Check if we should end data collection, (ie return run mode)
 */
bool IUI2C::checkIfEndCollection()
{
  // Check the received info; if data collection request, change the mode
  if (m_wireBuffer == END_COLLECTION)
  {
    port->println(END_CONFIRM);
    return true;
  }
  return false;
}

