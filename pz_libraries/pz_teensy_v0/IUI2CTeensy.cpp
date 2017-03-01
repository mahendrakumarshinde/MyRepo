#include "IUI2CTeensy.h"

IUI2CTeensy::IUI2CTeensy() : m_readFlag(true), m_silent(false), m_wireBuffer("")
{
  port = &Serial;
  resetErrorMessage();
}

/**
* Alternative constructor
* @param silent set to true to silence the serial output
* @param port a point to a Serial
*/
IUI2CTeensy::IUI2CTeensy(bool silent, Stream *serial_port) : m_readFlag(true), m_silent(silent), m_wireBuffer("")
{
  port = serial_port;
  resetErrorMessage();
}

IUI2CTeensy::~IUI2CTeensy()
{
    //dtor
}

/**
 * Activate and setup I2C
 * Setup for Master mode, pins 18/19, external pullups, 400kHz for Teensy 3.1
 */
void IUI2CTeensy::activate()
{
  Wire.begin(I2C_MASTER, 0x00, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  delay(10);
  Serial.begin(115200); //TODO Change Serial to port-> when not using Teensy anymore
}


/**
* Scan for I2C devices on the bus
* @return false if no device found else true
*/
bool IUI2CTeensy::scanDevices()
{
  // scan for i2c devices
  if (!m_silent) port->println("Scanning...");

  byte error, address;
  int nDevices = 0;
  for (address = 1; address < 129; address++ )
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
        if (address < 16) port->print("0");
        port->print(address, HEX);
        port->println("  !");
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

// I2C read/write functions

/**
 * Write a byte to given address and sub-address
 */
void IUI2CTeensy::writeByte(uint8_t address, uint8_t subAddress, uint8_t data)
{
  Wire.beginTransmission(address);  // Initialize the Tx buffer
  Wire.write(subAddress);           // Put slave register address in Tx buffer
  Wire.write(data);                 // Put data in Tx buffer
  Wire.endTransmission();           // Send the Tx buffer
}

/**
 * Read a single byte and return it
 * if an error occurs, m_readError and m_errorMessage are updated (accessible via getters)
 * @param address where to read the byte from
 * @param subAddress where to read the byte from
 */
uint8_t IUI2CTeensy::readByte(uint8_t address, uint8_t subAddress)
{
  uint8_t data = 0; // `data` will store the register data
  Wire.beginTransmission(address);         // Initialize the Tx buffer
  Wire.write(subAddress);                   // Put slave register address in Tx buffer
  m_readError = Wire.endTransmission(I2C_NOSTOP, 1000); // Send the Tx buffer, but send a restart to keep connection alive
  if (m_readError == 0x00)
  {
    Wire.requestFrom(address, (size_t) 1);   // Read one byte from slave register address
    data = Wire.read();                      // Fill Rx buffer with result
  }
  else
  {
    m_errorMessage = "I2CERR";
    if (!m_silent)
    {
      port->print("Error Code:");
      port->println(m_readError);
    }
  }
  return data;                             // Return data read from slave register
}

/**
 * Read several bytes and store them in destination array
 * @param address where to read the byte from
 * @param subAddress where to read the byte from
 * @param the number of byte to read
 */
void IUI2CTeensy::readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t *destination)
{
  if (m_readFlag) // To restrain I2S ISR for accel read from accessing I2C bus while temperature sensor is being read
  {
    m_readFlag = false;
    Wire.beginTransmission(address);   // Initialize the Tx buffer
    Wire.write(subAddress);            // Put slave register address in Tx buffer
    m_readError = Wire.endTransmission(I2C_NOSTOP, 1000); // Send the Tx buffer, but send a restart to keep connection alive
    if (m_readError == 0x00)
    {
      Wire.requestFrom(address, (size_t) count);  // Read bytes from slave register address
      uint8_t i = 0;
      while (Wire.available()) {
        destination[i++] = Wire.read();
      }         // Put read results in the Rx buffer
    }
    else
    {
      m_errorMessage = "I2CERR";
    }
    m_readFlag = true;
  }
}

/* ------------- Hardwire Serial for DATA_COLLECTION commands ------------- */

/**
 * Read port (serial) and write the output in the wire buffer
 */
void IUI2CTeensy::updateBuffer()
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
void IUI2CTeensy::resetBuffer()
{
  m_wireBuffer = "";
}

/**
 * Print the wire buffer to port (serial)
 */
void IUI2CTeensy::printBuffer()
{
  port->println(m_wireBuffer);
}

/**
 * Check if we should start data collection, (ie enter collection mode)
 */
bool IUI2CTeensy::checkIfStartCollection()
{
  // Check the received info; iff data collection request, change the mode
  if (m_wireBuffer.indexOf(START_COLLECTION) > -1)
  {
    port->print(START_CONFIRM);
    return true;
  }
  return false;
}

/**
 * Check if we should end data collection, (ie return run mode)
 */
bool IUI2CTeensy::checkIfEndCollection()
{
  // Check the received info; iff data collection request, change the mode
  if (m_wireBuffer == END_COLLECTION)
  {
    port->print(END_CONFIRM);
    return true;
  }
  return false;
}

