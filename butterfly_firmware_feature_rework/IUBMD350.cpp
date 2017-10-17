#include "IUBMD350.h"


/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUBMD350::IUBMD350(HardwareSerial *serialPort, uint32_t rate,
                   uint16_t dataReceptionTimeout) :
    IUSerial(InterfaceType::INT_BLE, serialPort, rate, 500, ';',
             dataReceptionTimeout),
    m_ATCmdEnabled(false),
    m_beaconEnabled(IUBMD350::defaultBeaconEnabled),
    m_beaconAdInterval(IUBMD350::defaultbeaconAdInterval)
{
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUBMD350::setupHardware()
{
    port->begin(baudRate);
    delay(10);
    // Configure pins and port
    pinMode(ATCmdPin, OUTPUT);
    pinMode(resetPin, OUTPUT);
    delay(100);
    // Beacon and UART configuration
    enterATCommandInterface();
    queryDeviceName();
    configureBeacon(m_beaconEnabled, m_beaconAdInterval);
    configureUARTPassthrough();
    wakeUp();
    exitATCommandInterface();
}

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
    IUSerial::wakeUp();
    enterATCommandInterface();
    setTxPowers(defaultTxPower);
    exitATCommandInterface();
}

/**
 * Switch to SLEEP power mode
 *
 * SLEEP mode consist of disabled Beacon and enabled UART settings
 */
void IUBMD350::sleep()
{
    IUSerial::sleep();
    enterATCommandInterface();
    setTxPowers(defaultTxPower);
    exitATCommandInterface();
}

/**
 * Switch to SUSPEND power mode
 *
 * SLEEP mode consist of disabled Beacon and UART modes
 */
void IUBMD350::suspend()
{
    IUSerial::suspend();
    enterATCommandInterface();
    setTxPowers(txPowerOption::DBm30);
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


/***** Device Name *****/

/**
 * Send an AT command to set the device name (must be in AT mode)
 *
 * @param name  the new device name, from 1 to 8 ASCII char
 * @return      true if successfully set, else false
 */
void IUBMD350::setDeviceName(char *deviceName)
{
    strcpy(m_deviceName, deviceName);
    char response[3];
    String cmd = "name ";
    sendATCommand((String) (cmd + m_deviceName), response, 3);
    if (strcmp(response, "OK") != 0 && setupDebugMode)
    {
        debugPrint(F("Failed to set device name, response was: "), false);
        debugPrint(response);
    }
}

/**
 * Query BLE configuration to get the device name
 *
 * Also update the device name saved at the class level (accessible via
 * getDeviceName)
 */
void IUBMD350::queryDeviceName()
{
    char response[9];
    int respLen = sendATCommand("name?", response, 9);
    if (respLen > 0)
    {
        strcpy(m_deviceName, response);
    }
    else if (setupDebugMode)
    {
        debugPrint(F("Failed to query device name: no response"));
    }
}


/***** Beacon and UART Passthrough *****/

/**
 * Configure the UART Passthrough mode (UART - BLE bridge)
 *
 * @return  true if configuration was successfully set, else false
 */
void IUBMD350::configureUARTPassthrough()
{
    char response[3];
    // Baud Rate
    String cmd = String("ubr ") + String(baudRate);
    sendATCommand(cmd, response, 3);
    if (setupDebugMode && strcmp(response, "OK") != 0)
    {
        debugPrint(F("Failed to set UART baud rate:\n  Command was: "),
                   false);
        debugPrint(cmd);
        debugPrint(F("  Response was: "), false);
        debugPrint(response);
    }
    // Flow Control
    if (UART_FLOW_CONTROL) { cmd += "ufc 01"; }
    else { cmd = "ufc 00"; }
    sendATCommand(cmd, response, 3);
    if (setupDebugMode && strcmp(response, "OK") != 0)
    {
        debugPrint(F("Failed to configure UART flow control:\n  Command was: "),
                   false);
        debugPrint(cmd);
        debugPrint(F("  Response was: "), false);
        debugPrint(response);
    }
    // Parity
    if (UART_PARITY) { cmd += "upar 01"; }
    else { cmd = "upar 00"; }
    sendATCommand(cmd, response, 3);
    if (setupDebugMode && strcmp(response, "OK") != 0)
    {
        debugPrint(F("Failed to configure UART parity:\n  Command was: "),
                   false);
        debugPrint(cmd);
        debugPrint(F("  Response was: "), false);
        debugPrint(response);
    }
    // enable / disable UART PassThrough
    if (UART_ENABLED) { cmd = "uen 01"; }
    else { cmd = "uen 00"; }
    sendATCommand(cmd, response, 3);
    if (setupDebugMode && strcmp(response, "OK") != 0)
    {
        debugPrint(F("Failed to enable UART:\n  Command was: "), false);
        debugPrint(cmd);
        debugPrint(F("  Response was: "), false);
        debugPrint(response);
    }
}


/**
 * Set Beacon configuration
 *
 * @param enabled     If true, enable Beacon, else disable the Beacon
 * @param adInterval  Beacon advertisement interval in ms - valid values range
 *  from 50ms to 4000ms.
 * @return            true if configuration was successfully set, else false
 */
void IUBMD350::configureBeacon(bool enabled, uint16_t adInterval)
{
    char response[3];
    String cmd;
    // Set attributes
    m_beaconEnabled = enabled;
    m_beaconAdInterval = adInterval;
    // enable / disable Beacon
    if (enabled) { cmd = "ben 01"; }
    else { cmd = "ben 00"; }
    sendATCommand(cmd, response, 3);
    if (setupDebugMode && strcmp(response, "OK") != 0)
    {
        debugPrint(F("Failed to enable / disable Beacon, response was: "), false);
        debugPrint(response);
    }
    // Set Ad Interval
    cmd = "badint " + String(adInterval, HEX);
    sendATCommand(cmd, response, 3);
    if (setupDebugMode && strcmp(response, "OK") != 0)
    {
        debugPrint(F("Failed to set Ad Interval, response was: "), false);
        debugPrint(response);
    }
}


/**
 * Set Beacon UUID
 *
 * @param UUID   Universal Unique Identifier as a char array of 32 hex digits
 *  (UUID is 16byte = 128bit long number)
 * @param major  UUID Major Number as a char array of 4 hex digits (16bit)
 * @param minor  UUID Minor Number as a char array of 4 hex digits (16bit)
 * @return       true if configuration was successfully set, else false
 */
void IUBMD350::setBeaconUUID(char *UUID, char *major, char *minor)
{
    bool success = true;
    char response[3];
    String cmd;
    // Set UUID
    cmd = "buuid ";
    sendATCommand((String) (cmd + UUID), response, 3);
    success &= (strcmp(response, "OK") == 0);
    // Set major number
    cmd = "bmjid ";
    sendATCommand((String) (cmd + major), response, 3);
    success &= (strcmp(response, "OK") == 0);
    // Set minor number
    cmd = "bmnid ";
    sendATCommand((String) (cmd + minor), response, 3);
    success &= (strcmp(response, "OK") == 0);
    if ((!success) && setupDebugMode)
    {
        debugPrint("Failed to configure BLE beacon UUID");
    }
}


/***** Tx Powers *****/

/**
 * Set the connected and beacon TX powers via AT Command
 *
 * @return True if the queries succeeded else false
 */
void IUBMD350::setTxPowers(txPowerOption txPower)
{
    m_connectedTxPower = txPower;
    m_beaconTxPower = txPower;
    char response[3];
    // Connected Power
    String cmd = String("ctxpwr ") + String(txPower, HEX);
    sendATCommand(cmd, response, 3);
    if (setupDebugMode && strcmp(response, "OK") != 0)
    {
        debugPrint(F("Failed to set Connected Tx Power:\n  Command was: "), false);
        debugPrint(cmd);
        debugPrint(F("  Response was: "), false);
        debugPrint(response);
    }
    // Beacon Power
    cmd = String("btxpwr ") + String(txPower, HEX);
    sendATCommand(cmd, response, 3);
    if (setupDebugMode && strcmp(response, "OK") != 0)
    {
        debugPrint(F("Failed to set Beacon Tx Power:\n  Command was: "), false);
        debugPrint(cmd);
        debugPrint(F("  Response was: "), false);
        debugPrint(response);
    }
}

/**
 * Query the connected and beacon TX power from BLE registers via AT Command
 *
 * @return True if the queries succeeded else false
 */
void IUBMD350::queryTxPowers()
{
    char txPowHex[3];
    int respLen = sendATCommand("ctxpwr?", txPowHex, 3);
    if (respLen > 0)
    {
        m_connectedTxPower = (txPowerOption) strtol(txPowHex, NULL, 16);
    }
    else if (setupDebugMode)
    {
        debugPrint(F("Failed to query connected Tx Power"));
    }
    respLen = sendATCommand("btxpwr?", txPowHex, 3) > 0;
    if (respLen > 0)
    {
        m_beaconTxPower = (txPowerOption) strtol(txPowHex, NULL, 16);
    }
    else if (setupDebugMode)
    {
        debugPrint(F("Failed to query beacon Tx Power"));
    }
}


/* =============================================================================
    Debugging
============================================================================= */

/**
 * Query UART configuration from registers and print it (must be in AT mode)
 */
void IUBMD350::printUARTConfiguration()
{
    #ifdef DEBUGMODE
    // UART enabled / disabled
    char enabled[3];
    sendATCommand("uen?", enabled, 3);
    // Get Tx Power
    char txPowHex[3];
    sendATCommand("ctxpwr?", txPowHex, 3);
    // Baud Rate
    char baudRate[8];
    sendATCommand("ubr?", baudRate, 8);
    // Flow Control
    char flowControl[3];
    sendATCommand("ufc?", flowControl, 3);
    // Parity
    char parity[3];
    sendATCommand("upar?", parity, 3);
    // Print everything
    debugPrint(F("  UART config:"));
    debugPrint(F("    PassThrough enabled: "), false); debugPrint(enabled);
    debugPrint(F("    Connected TxPower (dB): "), false);
    debugPrint(strtol(txPowHex, NULL, 16));
    debugPrint(F("    Baud rate: "), false); debugPrint(baudRate);
    debugPrint(F("    Flow Control enabled: "), false); debugPrint(flowControl);
    debugPrint(F("    Parity enabled: "), false); debugPrint(parity);
    #endif
}



/**
 * Query beacon configuration from registers and print it (must be in AT mode)
 */
void IUBMD350::printBeaconConfiguration()
{
    #ifdef DEBUGMODE
    int respLen = 0;
    // Beacon enabled / disabled
    char enabled[3];
    sendATCommand("ben?", enabled, 3);
    // Get Tx Power
    char txPowHex[3];
    sendATCommand("btxpwr?", txPowHex, 3);
    // Ad Interval
    char adInt[4];
    sendATCommand("badint?", adInt, 4);
    // UUID
    char uuid[33];
    sendATCommand("buuid?", uuid, 33);
    // Major number
    char major[5];
    sendATCommand("bmjid?", major, 5);
    // Minor number
    char minor[5];
    sendATCommand("bmnid?", minor, 5);
    // Print everything
    debugPrint(F("  Beacon config:"));
    debugPrint(F("    Enabled: "), false); debugPrint(enabled);
    debugPrint(F("    TxPower (dB): "), false);
    debugPrint(strtol(txPowHex, NULL, 16));
    debugPrint(F("    Ad Interval (ms): "), false);
    debugPrint(strtol(adInt, NULL, 16));
    debugPrint(F("    UUID: "), false); debugPrint(uuid);
    debugPrint(F("    Major number: "), false); debugPrint(major);
    debugPrint(F("    Minor number: "), false); debugPrint(minor);
    #endif
}


void IUBMD350::exposeInfo()
{
    #ifdef DEBUGMODE
    debugPrint(F("BLE Config: "));
    debugPrint(F("  Device name: ")); debugPrint(m_deviceName);
    enterATCommandInterface();
    printUARTConfiguration();
    printBeaconConfiguration();
    exitATCommandInterface();
    #endif
}


/***** BMDware info *****/

/**
 * Get BLE Module info and stream them through given port
 *
 * @param  pointers to char arrays to get the resulting info
 * @param  length of arrays
 */
void IUBMD350::getBMDwareInfo(char *BMDVersion, char *bootVersion,
                              char *protocolVersion, char *hardwareInfo,
                              uint8_t len1, uint8_t len2, uint8_t len3,
                              uint8_t len4)
{
    sendATCommand("ver?", BMDVersion, len1);
    sendATCommand("blver?", bootVersion, len2);
    sendATCommand("pver?", protocolVersion, len3);
    sendATCommand("hwinfo?", hardwareInfo, len4);
}


/* =============================================================================
    Instanciation
============================================================================= */

IUBMD350 iuBluetooth(&Serial1, 57600, 2000);
