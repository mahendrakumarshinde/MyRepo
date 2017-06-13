#include "IUBMD350.h"


IUBMD350::BooleanSetting IUBMD350::noSetting = {"", "", false};


/* ============================ Constructors, destructor, getters, setters ============================ */

IUBMD350::IUBMD350(IUI2C *iuI2C) :
  IUABCInterface(),
  m_iuI2C(iuI2C),
  m_bufferIndex(0),
  m_dataReceptionTimeout(2000),
  m_lastReadTime(0),
  m_ATCmdEnabled(false),
  m_UARTTxPower(defaultUARTTxPower),
  m_beaconTxPower(defaultBeaconTxPower)
{
  // Init Settings
  settings[0] = {"flowControl", "ufc", false};
  settings[1] = {"parity", "upar", false};
  settings[2] = {"UARTPassThrough", "uen", false};
  // Fill out the buffer with meaningless data
  for (int i = 0; i < bufferSize; i++)
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
  if (m_BLEBaudRate != defaultBLEBaudRate)
  {
    setBLEBaudRate(defaultBLEBaudRate);
  }
  setTxPower(defaultUARTTxPower);
  setTxPower(defaultBeaconTxPower, true);
  setBooleanSettings("flowControl", false);
  setBooleanSettings("parity", false);
  setBooleanSettings("UARTPassThrough", true);
  exitATCommandInterface();
}


/* ============================  Hardware & power management methods ============================ */

/**
 * Switch to ACTIVE power mode
 */
void IUBMD350::wakeUp()
{
  m_powerMode = powerMode::ACTIVE;
}

/**
 * Switch to SLEEP power mode
 */
void IUBMD350::sleep()
{
  m_powerMode = powerMode::SLEEP;
}

/**
 * Switch to SUSPEND power mode
 */
void IUBMD350::suspend()
{
  m_powerMode = powerMode::SUSPEND;
}

/**
   Reset the device
   Required after when entering / exiting AT Command Interface Mode
*/
void IUBMD350::softReset()
{
  digitalWrite(resetPin, LOW); // reset BMD-350
  delay(100); // wait a while
  digitalWrite(resetPin, HIGH); // restart BMD-350
}

/**
   Go into AT Command Interface Mode or exit it
   AT Command Interface is a mode where commands can be issued to the BMDWare via
   the BLE module port (Serial1). These commands allows to configure or query info
   about configuration.
   UART Pass-Through needs to be configured when in AT Command Interface Mode, but
   AT Mode needs to be exited to use UART Pass-Through.
*/
void IUBMD350::enterATCommandInterface()
{
  digitalWrite(ATCmdPin, LOW);
  delay(100);
  softReset();
  // hold ATMD pin LOW for at least 2.5s. If not, AT Mode will not work
  delay(2500);
  m_ATCmdEnabled = true;
  if (setupDebugMode)
  {
    debugPrint(F("Entered AT Command Interface mode"));
  }
  delay(10);
}

/**
   See enterATCommandInterface
*/
void IUBMD350::exitATCommandInterface()
{
  m_ATCmdEnabled = false;
  digitalWrite(ATCmdPin, HIGH);
  delay(100);
  softReset();
  if (setupDebugMode)
  {
    debugPrint(F("Exited AT Command Interface mode"));
  }
}

/**
   Send a configuration command to the BMDWare in AT Mode
   NB: Please not that sending configuration commands can take a non-negligible of time
       and requires the use of delay: it is not suitable for use in callbacks.
   @param command     the command to send, without the at$ prefix and
                      without return carriage "\r" (see datasheet)
   @param result      a pointer to a string to store the message returned by the BMDWare
   @param resultLen   the expected length of result
   @return            -1 if the command failed, or the number N of char of the response
                      Please not that we can have N <> resultLen
*/
int IUBMD350::sendATCommand(String cmd, char *response, uint8_t responseLength)
{
  if (!m_ATCmdEnabled)
  {
    if (setupDebugMode) {
      debugPrint("BLE: Cannot send AT commands when not in AT mode");
    }
    return -1;
  }
  port->write("at$");
  int charCount = cmd.length();
  for (int i = 0; i < charCount; i++)
  {
    port->write(cmd[i]);
  }
  port->write('\r');
  port->flush();
  uint8_t i = 0;
  while (!port->available() && i <= 50)
  {
    delay(100);
    i++;
  }
  if (i == 50)
  {
    if (setupDebugMode) {
      debugPrint("AT Command '" + cmd + "' failed");
    }
    return -1;
  }
  int respCount = 0;
  for (uint8_t j = 0; j < responseLength; ++j)
  {
    if (!port->available())
    {
      break;
    }
    response[j] = port->read();
    respCount++;
    if (response[j] == '\r' || response[j] == '\n') // end of response is carriage or line return
    {
      response[j] = 0; // Replace with end of string
      break;
    }
  }
  // Check that port buffer is empty
  while (port->available())
  {
    port->read();
  }
  return respCount;
}

/**
   Get BLE Module info and stream them through given port
   @param  pointers to char arrays to get the resulting info
   @param  length of arrays
*/
void IUBMD350::getModuleInfo(char *BMDVersion, char *bootVersion, char *protocolVersion, char *hardwareInfo,
                             uint8_t len1, uint8_t len2, uint8_t len3, uint8_t len4)
{
  sendATCommand("ver?", BMDVersion, len1);
  sendATCommand("blver?", bootVersion, len2);
  sendATCommand("pver?", protocolVersion, len3);
  sendATCommand("hwinfo?", hardwareInfo, len4);
}

/**
   Send an AT command to set the device name (must be in AT mode)
   @param name  the new device name, from 1 to 8 ASCII char
   @return      true if successfully set, else false
*/
bool IUBMD350::setDeviceName(char *deviceName)
{
  queryDeviceName();
  if (m_deviceName == deviceName)
  {
    return true; // Nothing to do, already OK
  }
  strcpy(m_deviceName, deviceName);
  char response[3];
  String cmd = "name ";
  sendATCommand((String) (cmd + m_deviceName), response, 3);
  if (strcmp(response, "OK") == 0)
  {
    return true;
  }
  if (setupDebugMode) {
    debugPrint("Failed to set device name");
  }
  return false;
}
/**
   Query BLE configuration to get the device name
   Also update the device name saved at the class level (accessible via getDeviceName)
*/
bool IUBMD350::queryDeviceName()
{
  char response[9];
  int respLen = sendATCommand("name?", response, 9);
  if (respLen > 0)
  {
    strCopyWithAutocompletion(m_deviceName, response, 9, respLen);
    return true;
  }
  return false; // failed
}

/**
 * Set the TX power of the BLE UART and Beacon
 *
 * UART Tx power affects the range at which the device is connectable
 * Beacon Tx Power affects the range at which the device is visible / discoverable
 */
bool IUBMD350::setTxPower(int8_t txPower, bool beacon)
{
  String cmd;
  if (beacon)
  {
    cmd = "ctxpwr ";
  }
  else
  {
    cmd = "btxpwr ";
  }
  switch (txPower)
  {
    case (-4):
      cmd += "FC"; // 256 - 4 = 252
      break;
    case 0:
      cmd += "00";
      break;
    case 4:
      cmd += "04";
      break;
    default:
      cmd += "04"; // Default to max Tx power = max BLE range
  }
  char response[3];
  sendATCommand(cmd, response, 3);
  queryTxPower(beacon);
  if (strcmp(response, "OK") == 0)
  {
    return true;
  }
  if (setupDebugMode) {
    if (beacon)
    {
      debugPrint("Failed to set Beacon Tx Power");
    }
    else
    {
      debugPrint("Failed to set UART Tx Power");
    }
  }
  return false;
}

bool IUBMD350::queryTxPower(bool beacon)
{
  char response[3];
  int respLen;
  if (beacon)
  {
    respLen = sendATCommand("btxpwr?", response, 3);
  }
  else
  {
    respLen = sendATCommand("ctxpwr?", response, 3);
  }
  if (respLen == 0)
  {
    return false; // failed
  }
  int8_t txPower;
  if (strcmp(response, "FC") == 0)
  {
    txPower = -4;
  }
  else if (strcmp(response, "00") == 0)
  {
    txPower = 0;
  }
  else if (strcmp(response, "04") == 0)
  {
    txPower = 4;
  }
  else
  {
    return false;  // Unknown response
  }
  if (beacon)
  {
    m_beaconTxPower = txPower;
  }
  else
  {
    m_UARTTxPower = txPower;
  }
  return true;
}

/**
   Set the UUID and the Major and Minor numbers for the device (see bluetooth Beacon)

   @param UUID    Universal Unique Identiifier as a char array of 32 hex digit (UUID is 16byte (128bit long number)
   @param major   UUID Major Number as a char array of 4 hex digits (16bit long number)
   @param minor   UUID Minor Number as a char array of 4 hex digits (16bit long number)
   @return        true if successfully set, else false
*/
bool IUBMD350::setUUIDInfo(char *UUID, char *major, char *minor)
{
  strcpy(m_UUID, UUID);
  strcpy(m_majorNumber, major);
  strcpy(m_minorNumber, minor);
  bool success[3] = {false, false, false};
  char response[3];
  String cmd = "buuid ";
  if(sendATCommand((String) (cmd + m_UUID), response, 3) > 0)
  {
    success[0] = (strcmp(response, "OK") == 0);
  }
  cmd = "bmjid ";
  if(sendATCommand((String) (cmd + m_majorNumber), response, 3) > 0)
  {
    success[1] = (strcmp(response, "OK") == 0);
  }
  cmd = "bmnid ";
  if(sendATCommand((String) (cmd + m_minorNumber), response, 3) > 0)
  {
    success[2] = (strcmp(response, "OK") == 0);
  }
  if (success[0] && success[1] && success[2])
  {
    return true;
  }
  if (setupDebugMode) {
    debugPrint("Failed to set BLE UUID, major and / or minor numbers");
  }
  return false;
}

/**
   Query BLE configuration to get the UUID, major and minor numbers
   Also update the UUID, major and minor saved at the class level (accessible via getters)
*/
bool IUBMD350::queryUUIDInfo()
{
  char uuid[33];
  int respLen = sendATCommand("buuid?", uuid, 33);
  if (respLen < 1) { return false; }
  strCopyWithAutocompletion(m_UUID, uuid, 33, respLen);

  char number[5];
  respLen = sendATCommand("bmjid?", number, 5);
  if (respLen < 1) { return false; }
  strCopyWithAutocompletion(m_majorNumber, number, 5, respLen);

  respLen = sendATCommand("bmnid?", number, 5);
  if (respLen < 1) { return false; }
  strCopyWithAutocompletion(m_minorNumber, number, 5, respLen);

  return true;
}

/**
   Send an AT command to set the given baudRate (must be in AT mode)
   @return      true if successfully set, else false
*/
bool IUBMD350::setBLEBaudRate(uint32_t baudRate)
{
  queryBLEBaudRate();
  if (m_BLEBaudRate == baudRate)
  {
    return true; // Nothing to do, already OK
  }
  m_BLEBaudRate = baudRate;
  char response[3];
  int respLength = sendATCommand((String) ("ubr " + m_BLEBaudRate), response, 3);
  if (respLength > 0 && strcmp(response, "OK") == 0)
  {
    return true;
  }
  if (setupDebugMode)
  {
    debugPrint("Failed to set BLE Baud Rate");
  }
  return false;
}

/**
   Query BLE configuration to get the current baud rate
   Also update the baudrate saved at the class level (accessible via getBaudRate)
*/
uint32_t IUBMD350::queryBLEBaudRate()
{
  char response[8];
  if (sendATCommand("ubr?", response, 8))
  {
    m_BLEBaudRate = (uint32_t) strtol(response, NULL, 16);
    return m_BLEBaudRate;
  }
  return 0; // failed
}

/**
   Return the setting (name, command and state) from its name
   @param settingName   the name of the setting to get
   @return              the setting (name + command + state)
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
  if (setupDebugMode) {
    debugPrint("BLE setting '" + settingName + "' not found");
  }
  return noSetting;
}

/**
   Send an AT command to enable / disable the desired setting
   @return      true if successfully set, else false
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
  char response[3];
  int respLength = -1;
  if (enable)
  {
    respLength = sendATCommand((String) (conf.command + " 01"), response, 3);
  }
  else
  {
    respLength = sendATCommand((String) (conf.command + " 00"), response, 3);
  }
  if (respLength > 0 && strcmp(response, "OK") == 0)
  {
    return true;
  }
  if (setupDebugMode) {
    debugPrint("Failed to set BLE " + conf.name);
  }
  return false;
}

/**
   Query BLE configuration to get the setting current state
   Also update the setting saved at the class level (accessible via getBooleanSetting(name).state)
*/
bool IUBMD350::queryBooleanSettingState(String settingName)
{
  BooleanSetting conf = getBooleanSettings(settingName);
  if (conf.name == "")
  {
    return false; // return false if failed if no setting found
  }
  char response[3];
  if (sendATCommand((String) (conf.command + "?"), response, 3) > 0)
  {
    conf.state = (strcmp(response, "01") == 0);
    return conf.state;
  }
  return false; // return false if failed
}


/**
   Ready the device for bluetooth streaming
*/
void IUBMD350::activate()
{
  if (m_ATCmdEnabled) // Make sure AT Command Interface is disabled
  {
    exitATCommandInterface();
  }
}

/**
   Reset the buffer if more time than m_dataReceptionTimeout passed since last reception.
   @return true if a timeout happened and that the buffer was reset, else false.
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
   Read available incoming bytes to fill the buffer
   @param processBuffer a function to process buffer
   @return true if the buffer is full and ready to be read, else false
   NB1: if there are a lot of incoming data, the buffer should be
   filled and processed several times.
   NB2: a data reception timeout check is performed before reading (see
   checkDataReceptionTimeout).
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
      if (loopDebugMode)
      {
        debugPrint(F("BLE received: "), false);
        debugPrint(m_buffer);
      }
      return true; // buffer is full => return true to warn it's ready to be processed
    }
  }
  return false;
}


/**
   Print given buffer then flushes
   @param buff the buffer
   @param buffSize the buffer size
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
   Print given buffer then flushes
   @param buff the buffer
   @param buffSize the buffer size
   @param transform a transfo to apply to each buffer element before printing
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


/* ====================== Diagnostic Functions, only active when setupDebugMode = true ====================== */

void IUBMD350::exposeInfo()
{
  #ifdef DEBUGMODE
  debugPrint(F("BLE Config: "));
  debugPrint(F("Device name: "), false); debugPrint(m_deviceName);
  debugPrint(F("UUID: "), false); debugPrint(m_UUID);
  debugPrint(F("Major number: "), false); debugPrint(m_majorNumber);
  debugPrint(F("Minor number: "), false); debugPrint(m_minorNumber);
  debugPrint(F("Baud rate: "), false); debugPrint(m_BLEBaudRate);
  debugPrint(F("Flow Control enabled: "), false); debugPrint(getBooleanSettings("flowControl").state);
  debugPrint(F("Parity enabled: "), false); debugPrint(getBooleanSettings("parity").state);
  debugPrint(F("UARTPassThrough enabled: "), false); debugPrint(getBooleanSettings("UARTPassThrough").state);
  #endif
}
