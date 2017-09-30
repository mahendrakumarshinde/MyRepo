#include "IUBMD350.h"

using namespace IUComponent;

/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUBMD350::IUBMD350(IUI2C *iuI2C) :
    ABCComponent(),
    m_iuI2C(iuI2C),
    m_ATCmdEnabled(false),
    m_bufferIndex(0),
    m_dataReceptionTimeout(2000),
    m_lastReadTime(0),
{
    // Configure pins and port
    pinMode(ATCmdPin, OUTPUT);
    pinMode(resetPin, OUTPUT);
    delay(100);
    setBaudRate(defaultBaudRate);
    // Get initial device name and UUID configuration
    enterATCommandInterface();
    queryDeviceName();
    queryBeaconConfiguration();
    // Then wakeUp => will set up the rest of the config
    wakeUp();
    exitATCommandInterface();
    // Fill out the buffer with meaningless data
    for (int i = 0; i < bufferSize; i++)
    {
        m_buffer[i] = 'a';
    }
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Reset the device
 *
 * Required after when entering / exiting AT Command Interface Mode
 */
void IUBMD350::softReset()
{
    digitalWrite(resetPin, LOW); // reset BMD-350
    delay(100); // wait a while
    digitalWrite(resetPin, HIGH); // restart BMD-350
}

/**
 * Switch to ACTIVE power mode
 *
 * ACTIVE mode consist of default Beacon and UART settings
 */
void IUBMD350::wakeUp()
{
    enterATCommandInterface();
    m_powerMode = PowerMode::ACTIVE;
    configureBeacon(defaultBeaconTxPower, defaultBeaconEnabled,
                    defaultbeaconAdInterval, m_beaconUUID, m_beaconMajorNumber,
                    m_beaconMinorNumber);
    configureUART(defaultConnectedTxPower, defaultUARTEnabled,
                  defaultUARTBaudRate, defaultUARTFlowControl,
                  defaultUARTParity);
    exitATCommandInterface();
}

/**
 * Switch to SLEEP power mode
 *
 * SLEEP mode consist of disabled Beacon and enabled UART settings
 */
void IUBMD350::sleep()
{
    enterATCommandInterface();
    m_powerMode = PowerMode::SLEEP;
    configureBeacon(txPowerOption::DBm30, false, 4000, m_beaconUUID,
                    m_beaconMajorNumber, m_beaconMinorNumber);
    configureUART(defaultConnectedTxPower, defaultUARTEnabled,
                  defaultUARTBaudRate, defaultUARTFlowControl,
                  defaultUARTParity);
    exitATCommandInterface();
}

/**
 * Switch to SUSPEND power mode
 *
 * SLEEP mode consist of disabled Beacon and UART modes
 */
void IUBMD350::suspend()
{
    enterATCommandInterface();
    m_powerMode = PowerMode::SUSPEND;
    configureBeacon(txPowerOption::DBm30, false, 4000, m_beaconUUID,
                    m_beaconMajorNumber, m_beaconMinorNumber);
    configureUART(txPowerOption::DBm30, defaultUARTEnabled, defaultUARTBaudRate,
                  defaultUARTFlowControl, defaultUARTParity);
    exitATCommandInterface();
}


/* =============================================================================
    Bluetooth Configuration
============================================================================= */

/***** AT Command Interface *****/

/**
 * Go into AT Command Interface Mode
 *
 * AT Command Interface is a mode where commands can be issued to the BMDWare
 * via the BLE module port (Serial1). These commands allows to configure or
 * query info about configuration.
 * UART Pass-Through needs to be configured when in AT Command Interface Mode,
 * but AT Mode needs to be exited to use UART Pass-Through.
 */
void IUBMD350::enterATCommandInterface()
{
    if (m_ATCmdEnabled)
    {
        return; // Already in AT Command mode
    }
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
 * Exit AT Command interface (see enterATCommandInterface doc)
 */
void IUBMD350::exitATCommandInterface()
{
    if (!m_ATCmdEnabled)
    {
        return; // Already out of AT Command mode
    }
    digitalWrite(ATCmdPin, HIGH);
    delay(100);
    softReset();
    m_ATCmdEnabled = false;
    if (setupDebugMode)
    {
        debugPrint(F("Exited AT Command Interface mode"));
    }
}

/**
 * Send a configuration command to the BMDWare in AT Mode
 *
 * Note that sending configuration commands can take a non-negligible amount of
 * time and requires the use of delay: it is not suitable for use in callbacks.
 * @param command     the command to send, without the at$ prefix and
 *  without return carriage "\r" (see datasheet)
 * @param result      a pointer to a string to store the message returned by the
 *  BMDWare
 * @param responseLength   the expected length of result
 * @return            -1 if the command failed, or the number N of char of the
 *  response. Note that it is possible that N <> responseLength.
 */
int IUBMD350::sendATCommand(String cmd, char *response, uint8_t responseLength)
{
    if (!m_ATCmdEnabled)
    {
        if (setupDebugMode)
        {
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
        if (setupDebugMode)
        {
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
        // end of response is carriage or line return
        if (response[j] == '\r' || response[j] == '\n')
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


/***** BMDware info *****/

/**
 * Get BLE Module info and stream them through given port
 *
 * @param  pointers to char arrays to get the resulting info
 * @param  length of arrays
 */
void IUBMD350::getModuleInfo(char *BMDVersion, char *bootVersion,
                             char *protocolVersion, char *hardwareInfo,
                             uint8_t len1, uint8_t len2, uint8_t len3,
                             uint8_t len4)
{
    sendATCommand("ver?", BMDVersion, len1);
    sendATCommand("blver?", bootVersion, len2);
    sendATCommand("pver?", protocolVersion, len3);
    sendATCommand("hwinfo?", hardwareInfo, len4);
}


/***** Device Name *****/

/**
 * Send an AT command to set the device name (must be in AT mode)
 *
 * @param name  the new device name, from 1 to 8 ASCII char
 * @return      true if successfully set, else false
 */
bool IUBMD350::setDeviceName(char *deviceName)
{
    strcpy(m_deviceName, deviceName);
    char response[3];
    String cmd = "name ";
    sendATCommand((String) (cmd + m_deviceName), response, 3);
    if (strcmp(response, "OK") == 0)
    {
        return true;
    }
    if (setupDebugMode)
    {
        debugPrint("Failed to set device name");
    }
    return false;
}

/**
 * Query BLE configuration to get the device name
 *
 * Also update the device name saved at the class level (accessible via
 * getDeviceName)
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


/***** Beacon configuration *****/

/**
 * Set Beacon configuration
 *
 * @param txPower     Transmission power for the Beacon
 * @param enabled     If true, enable Beacon, else disable the Beacon
 * @param adInterval  Beacon advertisement interval in ms - valid values range
 *  from 50ms to 4000ms.
 * @param UUID        Universal Unique Identifier as a char array of 32 hex
 *  digit (UUID is 16byte (128bit long number)
 * @param major       UUID Major Number as a char array of 4 hex digits (16bit
 *  long number)
 * @param minor       UUID Minor Number as a char array of 4 hex digits (16bit
 *  long number)
 * @return            true if configuration was successfully set, else false
 */
bool IUBMD350::configureBeacon(txPowerOption txPower, bool enabled,
                               uint16_t adInterval, char *UUID, char *major,
                               char *minor)
{
    bool success = true;
    char response[3];
    String cmd;
    // Set attributes
    m_beaconEnabled = enabled;
    m_beaconTxPower = txPower;
    strcpy(m_beaconUUID, UUID);
    strcpy(m_beaconMajorNumber, major);
    strcpy(m_beaconMinorNumber, minor);
    // Set Tx Power
    cmd = "btxpwr " + String(txPower, HEX);
    sendATCommand(cmd, response, 3);
    success &= (strcmp(response, "OK") == 0);
    // enable / disable Beacon
    if (enabled) { cmd = "ben 01"; }
    else { cmd = "ben 00"; }
    sendATCommand(cmd, response, 3);
    success &= (strcmp(response, "OK") == 0);
    // Set Ad Interval
    cmd = "badint " + String(adInterval, HEX);
    sendATCommand(cmd, response, 3);
    success &= (strcmp(response, "OK") == 0);
    // Set UUID
    cmd = "buuid ";
    sendATCommand((String) (cmd + m_beaconUUID), response, 3);
    success &= (strcmp(response, "OK") == 0);
    // Set major number
    cmd = "bmjid ";
    sendATCommand((String) (cmd + m_beaconMajorNumber), response, 3);
    success &= (strcmp(response, "OK") == 0);
    // Set minor number
    cmd = "bmnid ";
    sendATCommand((String) (cmd + m_beaconMinorNumber), response, 3);
    success &= (strcmp(response, "OK") == 0);
    if ((!success) && setupDebugMode)
    {
        debugPrint("Failed to configure BLE beacon");
    }
    return success;
}

/**
 * Query beacon configuration from registers and update instance attributes
 *
 * @return true if the queries succeeded, else false
 */
bool IUBMD350::queryBeaconConfiguration()
{
    int respLen = 0;
    // Beacon enabled / disabled
    char enabled[3];
    respLen = sendATCommand("ben?", enabled, 3);
    m_beaconEnabled = (strcmp(enabled, "01") == 0);
    // Get Tx Power
    char txPowHex[3];
    respLen = sendATCommand("btxpwr?", txPowHex, 3);
    if (respLen < 1) { return false; }
    m_beaconTxPower = (txPowerOption) strtol(txPowHex, NULL, 16);
    // Ad Interval
    char adInt[4];
    if (sendATCommand("badint?", adInt, 4))
    {
        m_beaconAdInterval = (uint16_t) strtol(adInt, NULL, 16);
    }
    // UUID
    char uuid[33];
    respLen = sendATCommand("buuid?", uuid, 33);
    if (respLen < 1) { return false; }
    strCopyWithAutocompletion(m_beaconUUID, uuid, 33, respLen);
    // Major number
    char number[5];
    respLen = sendATCommand("bmjid?", number, 5);
    if (respLen < 1) { return false; }
    strCopyWithAutocompletion(m_beaconMajorNumber, number, 5, respLen);
    // Minor number
    respLen = sendATCommand("bmnid?", number, 5);
    if (respLen < 1) { return false; }
    strCopyWithAutocompletion(m_beaconMinorNumber, number, 5, respLen);
    return true;
}


/***** UART configuration *****/

/**
 * Set UART configuration
 *
 * @param txPower      Transmission power for the UART
 * @param enabled      If true, enable UART, else disable the UART
 * @param baudRate     The baud rate used for UART communication
 * @param flowControl  Enable flow control ?
 * @param parity       enable parity ?
 * @return             true if configuration was successfully set, else false
 */
bool IUBMD350::configureUART(txPowerOption txPower, bool enabled,
                             uint32_t baudRate, bool flowControl, bool parity)
{
    bool success = true;
    char response[3];
    String cmd;
    // Set attributes
    m_UARTEnabled = enabled;
    m_UARTBaudRate = baudRate;
    m_UARTFlowControl = flowControl;
    m_UARTParity = parity;
    // Set connected Tx Power
    success &= setConnectedTxPower(txPower);
    // enable / disable UART
    if (enabled) { cmd = "uen 01"; }
    else { cmd = "uen 00"; }
    sendATCommand(cmd, response, 3);
    success &= (strcmp(response, "OK") == 0);
    // Baud Rate
    sendATCommand((String) ("ubr " + m_UARTBaudRate), response, 3);
    success &= (strcmp(response, "OK") == 0);
    // Flow Control
    if (flowControl) { cmd += "ufc 01"; }
    else { cmd = "ufc 00"; }
    sendATCommand(cmd, response, 3);
    success &= (strcmp(response, "OK") == 0);
    // Parity
    if (parity) { cmd += "upar 01"; }
    else { cmd = "upar 00"; }
    sendATCommand(cmd, response, 3);
    success &= (strcmp(response, "OK") == 0);
    if ((!success) && setupDebugMode)
    {
        debugPrint("Failed to configure BLE UART");
    }
    return success;
}

/**
 * Query UART configuration from registers and update instance attributes
 *
 * @return true if the queries succeeded, else false
 */
bool IUBMD350::queryUARTConfiguration()
{
    int respLen = 0;
    // UART enabled / disabled
    char enabled[3];
    respLen = sendATCommand("uen?", enabled, 3);
    m_beaconEnabled = (strcmp(enabled, "01") == 0);
    // Get Tx Power
    queryConnectedTxPower();
    // Baud Rate
    char baudRate[8];
    if (sendATCommand("ubr?", baudRate, 8))
    {
        m_UARTBaudRate = (uint32_t) strtol(baudRate, NULL, 16);
    }
    // Flow Control
    char flowControl[3];
    respLen = sendATCommand("ufc?", flowControl, 3);
    m_beaconEnabled = (strcmp(flowControl, "01") == 0);
    // Parity
    char parity[3];
    respLen = sendATCommand("upar?", parity, 3);
    m_beaconEnabled = (strcmp(parity, "01") == 0);
    return true;
}


/***** Connected Power *****/

/**
 * Set the connected TX power
 *
 * The connected Tx Power is also used for UART, when UART is enabled
 */
bool IUBMD350::setConnectedTxPower(txPowerOption txPower)
{
    m_connectedTxPower = txPower;
    String cmd = "ctxpwr " + String(txPower, HEX);
    char response[3];
    sendATCommand(cmd, response, 3);
    if (strcmp(response, "OK") == 0)
    {
        return true;
    }
    if (debugMode)
    {
        debugPrint("Failed to set connected Tx Power");
    }
    return false;
}

/**
 * Query the connected TX power from BLE module registers via AT Command
 *
 * @return True if the query succeeded else false
 */
bool IUBMD350::queryConnectedTxPower()
{
    char txPowHex[3];
    int respLen = sendATCommand("ctxpwr?", txPowHex, 3);
    if (respLen < 1)
    {
        return false;
    }
    m_beaconTxPower = (txPowerOption) strtol(txPowHex, NULL, 16);
    return true;
}


/* =============================================================================
    Bluetooth Configuration
============================================================================= */

void IUBMD350::setBaudRate(uint32_t baudRate)
{
    m_baudRate = baudRate;
    // NB: Do not flush or end, as it blocks the device
    port->begin(m_baudRate);
}

/**
 * Read available incoming bytes to fill the buffer
 *
 * NB1: if there are a lot of incoming data, the buffer should be
 * filled and processed several times.
 * NB2: a data reception timeout check is performed before reading (see
 * checkDataReceptionTimeout).
 * @param processBuffer a function to process buffer
 * @return true if the buffer is full and ready to be read, else false
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
            // Return true to indicate that buffer is full & ready to be
            // processed
            return true;
        }
    }
    return false;
}


/***** Data reception robustness *****/

/**
 * Reset the buffer if too much time passed since last reception
 *
 * Time since last reception is compared to m_dataReceptionTimeout.
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


/* =============================================================================
    Debugging
============================================================================= */

void IUBMD350::exposeInfo()
{
  #ifdef DEBUGMODE
  debugPrint(F("BLE Config: "));
  debugPrint(F("  Device name: ")); debugPrint(m_deviceName);
  debugPrint(F("  Connected Tx Power (dB): "), false);
  debugPrint((int8_t) m_connectedTxPower);
  debugPrint(F("  UART config:"));
  debugPrint(F("    Enabled: "), false); debugPrint(m_UARTEnabled);
  debugPrint(F("    Baud rate: "), false); debugPrint(m_UARTBaudRate);
  debugPrint(F("    Flow Control enabled: "), false);
  debugPrint(m_UARTFlowControl);
  debugPrint(F("    Parity enabled: "), false); debugPrint(m_UARTParity);
  debugPrint(F("UARTPassThrough enabled: "), false); debugPrint(m_UARTEnabled);
  debugPrint(F("  Beacon config:"));
  debugPrint(F("    Enabled: "), false); debugPrint(m_beaconEnabled);
  debugPrint(F("    TxPower (dB): "), false);
  debugPrint((int8_t) m_beaconTxPower);
  debugPrint(F("    UUID: "), false); debugPrint(m_beaconUUID);
  debugPrint(F("    Major number: "), false); debugPrint(m_beaconMajorNumber);
  debugPrint(F("    Minor number: "), false); debugPrint(m_beaconMinorNumber);
  #endif
}


/* =============================================================================
    Instanciation
============================================================================= */

IUBMD350 iuBluetooth(&iuI2C);


/* =============================================================================
    Instanciation
============================================================================= */

IUBMD350 iuBluetooth(&iuI2C);
