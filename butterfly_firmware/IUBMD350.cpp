#include "IUBMD350.h"


IUBMD350::BooleanSetting IUBMD350::noSetting = {"", "", false};

IUBMD350::IUBMD350(IUI2C *iuI2C) :
  IUABCInterface(),
  m_iuI2C(iuI2C),
  m_bufferIndex(0),
  m_dataReceptionTimeout(2000),
  m_lastReadTime(0)
{
  // Init Settings
  settings[0] = {"flowControl", "ufc", false};
  settings[1] = {"parity", "upar", false};
  settings[2] = {"UARTPassThrough", "uen", false};
  // Fill out the buffer with meaningless data
  for (int i=0; i < bufferSize; i++)
  {
    m_buffer[i] = 'a';
  }
  // Make configuration pins writable
  pinMode(ATCmdPin, OUTPUT);
  pinMode(resetPin, OUTPUT);
  delay(100);
  // Initial Configuration
  m_baudRate = defaultBaudRate;
  port->begin(defaultBaudRate);
  enterATCommandInterface();
  queryDeviceName();
  queryUUIDInfo();
  queryBLEBaudRate();
  if(m_BLEBaudRate != defaultBLEBaudRate)
  {
    setBLEBaudRate(defaultBLEBaudRate);
  }
  setBooleanSettings("flowControl", false);
  setBooleanSettings("parity", false);
  setBooleanSettings("UARTPassThrough", true);
  exitATCommandInterface();
}

/**
 * Reset the device
 * Required after when entering / exiting AT Command Interface Mode
 */
void IUBMD350::resetDevice()
{
  digitalWrite(resetPin, LOW); // reset BMD-350
  delay(100); // wait a while
  digitalWrite(resetPin, HIGH); // restart BMD-350
}

/**
 * Go into AT Command Interface Mode or exit it
 * AT Command Interface is a mode where commands can be issued to the BMDWare via
 * the BLE module port (Serial1). These commands allows to configure or query info
 * about configuration.
 * UART Pass-Through needs to be configured when in AT Command Interface Mode, but
 * AT Mode needs to be exited to use UART Pass-Through.
 */
void IUBMD350::enterATCommandInterface()
{
  m_ATCmdEnabled = true;
  digitalWrite(ATCmdPin, LOW);
  delay(100);
  resetDevice();
  delay(3000);
  // hold ATMD pin LOW for at least 2.5 seconds
  // If you do not hold it, the AT Mode will not be fully configured and will not work
  if (!m_iuI2C->isSilent())
  {
    m_iuI2C->port->println("Entered AT Command Interface mode");
  }
}

/**
 * See enterATCommandInterface
 */
void IUBMD350::exitATCommandInterface()
{
  m_ATCmdEnabled = false;
  digitalWrite(ATCmdPin, HIGH);
  delay(100);
  resetDevice();
  if (!m_iuI2C->isSilent())
  {
    m_iuI2C->port->println("Exited AT Command Interface mode");
  }
}

/**
 * Send a configuration command to the BMDWare in AT Mode
 * NB: Please not that sending configuration commands can take a non-negligible of time
 *     and requires the use of delay: it is not suitable for use in callbacks.
 * @param command   the command to send, without the at$ prefix and
                    without return carriage "\r" (see datasheet)
 * @param result    a pointer to a string to store the message returned by the BMDWare
 * @return  true if currently in AT Mode and the command was sent
            false if currently not in AT Mode, and the command was canceled.
 */
bool IUBMD350::sendATCommand(String cmd, String *result)
{
  if (!m_ATCmdEnabled)
  {
    if (setupDebugMode) { debugPrint("BLE: Cannot send AT commands when not in AT mode"); }
    return false;
  }
  port->write("at$");
  int charCount = cmd.length();
  for (int i = 0; i < charCount; i++)
  {
    port->write(cmd[i]);
  }
  port->write('\r');
  uint8_t i = 0;
  while (!port->available() && i <= 50)
  {
    delay(100); 
    i++;
  }
  if (i == 50)
  {
    if (setupDebugMode) { debugPrint("AT Command '" + cmd + "' failed"); }
    return false;
  }
  while (port->available())
  {
    result += port->read();
  }
  return true;
}

/**
 * Get BLE Module info and stream them through given port
 * @param  pointers to strings to get the resulting info
 */
void IUBMD350::getModuleInfo(String *BMDVersion, String *bootVersion, String *protocolVersion, String *hardwareInfo)
{
  sendATCommand("ver?", BMDVersion);
  sendATCommand("blver?", bootVersion);
  sendATCommand("pver?", protocolVersion);
  sendATCommand("hwinfo?", hardwareInfo);
}

/**
 * Send an AT command to set the device name (must be in AT mode)
 * @param name  the new device name, from 1 to 8 ASCII char
 * @return      true if successfully set, else false
 */
bool IUBMD350::setDeviceName(String name)
{
  queryDeviceName();
  if (m_deviceName == name)
  {
    return true; // Nothing to do, already OK
  }
  m_deviceName = name;
  String result = "";
  bool success = sendATCommand((String) ("name " + m_deviceName), &result);
  if (success && result == "OK")
  {
    return true;
  }
  if (setupDebugMode) { debugPrint("Failed to set device name"); }
  return false;
}

/**
 * Query BLE configuration to get the device name
 * Also update the device name saved at the class level (accessible via getDeviceName)
 */
String IUBMD350::queryDeviceName()
{
  String result = "";
  if (sendATCommand("name?", &result))
  {
    m_deviceName = result;
    return m_deviceName;
  }
  return ""; // failed
}

/**
 * Set the UUID and the Major and Minor numbers for the device (see bluetooth Beacon)
 *
 * @param UUID    Universal Unique Identiifier as a String of 32 hex digit (UUID is 16byte (128bit long number)
 * @param major   UUID Major Number as a String of 4 hex digits (16bit long number)
 * @param minor   UUID Minor Number as a String of 4 hex digits (16bit long number)
 * @return      true if successfully set, else false
 */
bool IUBMD350::setUUIDInfo(String UUID, String major, String minor)
{
  m_UUID = UUID;
  m_majorNumber = major;
  m_minorNumber = minor;
  String results[3] = {"", "", ""};
  bool success = sendATCommand((String) ("buuid " + m_UUID), &results[0]);
  success &= sendATCommand((String) ("bmjid " + m_majorNumber), &results[1]);
  success &= sendATCommand((String) ("bmnid " + m_minorNumber), &results[2]);
  if (success && results[0] == "OK" && results[1] == "OK" && results[2] == "OK")
  {
    return true;
  }
  if (setupDebugMode) { debugPrint("Failed to set BLE UUID, major and / or minor numbers"); }
  return false;
}

/**
 * Query BLE configuration to get the UUID, major and minor numbers
 * Also update the UUID, major and minor saved at the class level (accessible via getters)
 */
bool IUBMD350::queryUUIDInfo()
{
  String results[3] = {"", "", ""};
  if (sendATCommand("buuid?", &results[0]) &&
      sendATCommand("bmjid?", &results[1]) &&
      sendATCommand("bmnid?", &results[2]))
  {
    m_UUID = results[0];
    m_majorNumber = results[1];
    m_minorNumber = results[2];
    return true;
  }
  return false;
}

/**
 * Send an AT command to set the given baudRate (must be in AT mode)
 * @return      true if successfully set, else false
 */
bool IUBMD350::setBLEBaudRate(uint32_t baudRate)
{
  queryBLEBaudRate();
  if (m_BLEBaudRate == baudRate)
  {
    return true; // Nothing to do, already OK
  }
  m_BLEBaudRate = baudRate;
  String result = "";
  bool success = sendATCommand((String) ("ubr " + m_BLEBaudRate), &result);
  if (success && result == "OK")
  {
    return true;
  }
  if (setupDebugMode) { debugPrint("Failed to set BLE Baud Rate"); }
  return false;
}

/**
 * Query BLE configuration to get the current baud rate
 * Also update the baudrate saved at the class level (accessible via getBaudRate)
 */
uint32_t IUBMD350::queryBLEBaudRate()
{
  String result = "";
  if (sendATCommand("ubr?", &result))
  {
    m_BLEBaudRate = (uint32_t) result.toInt();
    return m_BLEBaudRate;
  }
  return 0; // failed
}

/**
 * Return the setting (name, command and state) from its name
 * @param settingName   the name of the setting to get
 * @return              the setting (name + command + state)
 */
IUBMD350::BooleanSetting IUBMD350::getBooleanSettings(String settingName)
{
  for (int i = 0; i < booleanSettingCount; i++)
  {
    if (settings[i].name == settingName)
    {
      return settings[i];
    }
  }
  if (setupDebugMode) { debugPrint("BLE setting '" + settingName + "' not found"); }
  return noSetting;
}

/**
 * Send an AT command to enable / disable the desired setting
 * @return      true if successfully set, else false
 */
bool IUBMD350::setBooleanSettings(String settingName, bool enable)
{
  queryBooleanSettingState(settingName);
  BooleanSetting conf = getBooleanSettings(settingName);
  if (conf.state == enable)
  {
    return true; // Nothing to do, already OK
  }
  if (conf.name == "")
  {
    return false; // Fail if no setting found
  }
  conf.state = enable;
  String result = "";
  bool success = false;
  if (enable)
  {
    success = sendATCommand((String) (conf.command + " 01"), &result);
  }
  else
  {
    success = sendATCommand((String) (conf.command + " 00"), &result);
  }
  if (success && result == "OK")
  {
    return true;
  }
  if (setupDebugMode) { debugPrint("Failed to set BLE " + conf.name); }
  return false;
}

/**
 * Query BLE configuration to get the setting current state
 * Also update the setting saved at the class level (accessible via getBooleanSetting(name).state)
 */
bool IUBMD350::queryBooleanSettingState(String settingName)
{
  BooleanSetting conf = getBooleanSettings(settingName);
  if (conf.name == "")
  {
    return false; // return false if failed if no setting found
  }
  String result = "";
  if (sendATCommand((String) (conf.command + "?"), &result))
  {
    conf.state = (result == "01");
    return conf.state;
  }
  return false; // return false if failed
}


/**
 * Ready the device for bluetooth streaming
 */
void IUBMD350::activate()
{
  if (m_ATCmdEnabled) // Make sure AT Command Interface is disabled
  {
    exitATCommandInterface();
  }
}

/**
 * Reset the buffer if more time than m_dataReceptionTimeout passed since last reception.
 * @return true if a timeout happened and that the buffer was reset, else false.
 */
bool IUBMD350::checkDataReceptionTimeout()
{
  if (m_bufferIndex > 0 && (millis() -  m_lastReadTime > m_dataReceptionTimeout))
  {
    m_bufferIndex = 0;
    return true;
  }
  return false;
}

/**
 * Read available incoming bytes to fill the buffer
 * @param processBuffer a function to process buffer
 * @return true if the buffer is full and ready to be read, else false
 * NB1: if there are a lot of incoming data, the buffer should be
 * filled and processed several times.
 * NB2: a data reception timeout check is performed before reading (see
 * checkDataReceptionTimeout).
 */
bool IUBMD350::readToBuffer()
{
  if (m_bufferIndex == bufferSize)
  {
      m_bufferIndex = 0; // reset the buffer if it was previously filled
  }
  checkDataReceptionTimeout();
  while (port->available() > 0)
  {
    m_buffer[m_bufferIndex] = port->read();
    m_bufferIndex++;
    m_lastReadTime = millis();
    if (m_bufferIndex == bufferSize)
    {
      return true; // buffer is full => return true to warn it's ready to be processed
    }
  }
  return false;
}


/**
 * Print given buffer then flushes
 * @param buff the buffer
 * @param buffSize the buffer size
 */
void IUBMD350::printFigures(uint16_t buffSize, q15_t *buff)
{
  for (int i = 0; i < buffSize; i++)
  {
    port->print(buff[i]);
    port->print(",");
    port->flush();
  }
}

/**
 * Print given buffer then flushes
 * @param buff the buffer
 * @param buffSize the buffer size
 * @param transform a transfo to apply to each buffer element before printing
 */
void IUBMD350::printFigures(uint16_t buffSize, q15_t *buff, float (*transform) (int16_t))
{
    for (int i = 0; i < buffSize; i++)
    {
      port->print(transform(buff[i]));
      port->print(",");
      port->flush();
    }
}
