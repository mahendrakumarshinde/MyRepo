#include "IUBMD350.h"
#include "Conductor.h"

/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUBMD350::IUBMD350(HardwareSerial *serialPort, char *charBuffer,
                   uint16_t bufferSize, PROTOCOL_OPTIONS protocol,
                   uint32_t rate, char stopChar,
                   uint16_t dataReceptionTimeout, uint8_t resetPin,
                   uint8_t atCmdPin) :
    IUSerial(serialPort, charBuffer, bufferSize, protocol, rate, stopChar,
             dataReceptionTimeout),
    m_resetPin(resetPin),
    m_atCmdPin(atCmdPin),
    m_ATCmdEnabled(false),
    m_beaconEnabled(IUBMD350::defaultBeaconEnabled),
    m_beaconAdInterval(IUBMD350::defaultbeaconAdInterval)
{
    resetTxBuffer();
    m_serialTxEmpty = true;
    m_lastSerialTxEmptied = 0;
}


/* =============================================================================
    Bluetooth throughput control
============================================================================= */

void IUBMD350::resetTxBuffer()
{
    for (uint16_t i = 0; i < TX_BUFFER_LENGTH; i++) {
        m_txBuffer[i] = 0;
    }
    m_txHead = 0;
    m_txTail = 0;
}

size_t IUBMD350::write(const char c)
{
    if (c == 0) {
        return 0;
    }
    if (((m_txTail + 1) % TX_BUFFER_LENGTH) != m_txHead) {
        m_txBuffer[m_txTail] = c;
        m_txTail = (m_txTail + 1) % TX_BUFFER_LENGTH;
        return 1;
    } else {
        return 0;
    }
}

size_t IUBMD350::write(const char *msg)
{
    size_t len = strlen(msg);
    size_t counter = 0;
    for (size_t i = 0; i < len; i++) {
        counter += write(msg[i]);
    }
    return counter;
}

void IUBMD350::bleTransmit()
{
    uint32_t now = millis();
    if (!m_serialTxEmpty && port->availableForWrite() == SERIAL_TX_MAX_AVAILABLE) {
        m_serialTxEmpty = true;
        m_lastSerialTxEmptied = now;
    }
    if (m_txHead == m_txTail) {
        return;
    }
    size_t counter = 0;
    if (m_serialTxEmpty && now - m_lastSerialTxEmptied > BLE_TX_DELAY) {
        uint16_t i0;
        uint16_t i1;
        if (m_txTail > m_txHead) {
            i0 = min(m_txHead + BLE_MTU_LEN, m_txTail);
            i1 = i0;
        } else {
            i0 = m_txHead + BLE_MTU_LEN;
            if (i0 < TX_BUFFER_LENGTH) {
                i1 = i0;
            } else {
                i1 = min(i0 % TX_BUFFER_LENGTH, m_txTail);
                i0 = TX_BUFFER_LENGTH;
            }
        }
        for (size_t i = m_txHead; i < i0; i++) {
            counter += port->write(m_txBuffer[i]);
        }
        if (i0 != i1) {
            for (size_t i = 0; i < i1; i++) {
                counter += port->write(m_txBuffer[i]);
            }
        }
        m_txHead = i1;
        if (counter > 0) {
            m_serialTxEmpty = false;
        }
    }
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUBMD350::setupHardware()
{
    begin();
    // Configure pins and port
    pinMode(m_atCmdPin, OUTPUT);
    pinMode(m_resetPin, OUTPUT);
    doFullConfig();
    if(isBLEAvailable == true && setupDebugMode){
        char BMDVersion[20],bootVersion[20],protocolversion[20],hardwareinfo[20];
        enterATCommandInterface();
        getBMDwareInfo(BMDVersion,bootVersion,protocolversion,hardwareinfo,20,20,20,20);
        debugPrint("BMDVersion : ",false);debugPrint(BMDVersion);
        debugPrint("bootVersion : ",false);debugPrint(bootVersion);
        debugPrint("protocolVersion : ",false);debugPrint(protocolversion);
        debugPrint("hardwareInfo :",false);debugPrint(hardwareinfo); 
        exitATCommandInterface();
    }
}

/**
 * Reset the device
 *
 * Required after when entering / exiting AT Command Interface Mode
 */
void IUBMD350::softReset()
{
    digitalWrite(m_resetPin, LOW); // reset BMD-350
    delay(1000); // wait a while
    digitalWrite(m_resetPin, HIGH); // restart BMD-350
    blePowerStatus = true;
}
/**
 * @brief 
 * BLE ON and OFF fuction pass used to turn on/off the BLE module
 * @param button 
 * @return true 
 * @return false 
 */
bool IUBMD350::bleButton(bool button)
{ 
    // Configure pins and port
    pinMode(m_atCmdPin, OUTPUT);
    pinMode(m_resetPin, OUTPUT);
    if(button == true)
    {
        digitalWrite(m_resetPin, HIGH); //BLE ON
        debugPrint("BLE turn ON",true);
        return blePowerStatus = true;
    }
    else if (button == false)
    {
        digitalWrite(m_resetPin, LOW); //ble OFF
        debugPrint("BLE turn OFF",true);
        return blePowerStatus = false;
    }
}
/**
 * Manage component power modes
 */
void IUBMD350::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:
        case PowerMode::ENHANCED:
        case PowerMode::REGULAR:
        case PowerMode::LOW_1:
        case PowerMode::LOW_2:
            enterATCommandInterface();
            setTxPowers(defaultTxPower);
            exitATCommandInterface();
            break;
        case PowerMode::SLEEP:
        case PowerMode::DEEP_SLEEP:
        case PowerMode::SUSPEND:
            enterATCommandInterface();
            setTxPowers(txPowerOption::DBm30);
            exitATCommandInterface();
            break;
        default:
            if (debugMode) {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
    }
}


/* =============================================================================
    AT Command Interface
============================================================================= */

/**
 * Go into AT Command Interface Mode
 *
 * AT Command Interface is a mode where commands can be issued to the BMDWare
 * via the BLE module port (Serial1). These commands allows to configure or
 * query info about configuration.
 * UART Pass-Through needs to be configured when in AT Command Interface Mode,
 * but AT Mode needs to be exited to use UART Pass-Through.
 */
void IUBMD350::enterATCommandInterface(uint8_t retry)
{
    if (m_ATCmdEnabled) {
        return; // Already in AT Command mode
    }
    digitalWrite(m_atCmdPin, LOW);
    delay(10);
    softReset();
    //delay(3000); // Wait for power cycle to complete
    delay(5000);
    m_ATCmdEnabled = true;
    if (setupDebugMode) {
        debugPrint(F("Entered AT Command Interface mode"));
    }
}

/**
 * Exit AT Command interface (see enterATCommandInterface doc)
 */
void IUBMD350::exitATCommandInterface()
{
    if (!m_ATCmdEnabled) {
        return; // Already out of AT Command mode
    }
    digitalWrite(m_atCmdPin, HIGH);
    delay(10);
    softReset();
    //delay(3000); // Wait for power cycle to complete
    delay(5000);
    m_ATCmdEnabled = false;
    if (setupDebugMode) {
        debugPrint(F("Exited AT Command Interface mode"));
    }
}

/**
 * Send a configuration command to the BMDWare in AT Mode
 *
 * Note that sending configuration commands can take a non-negligible amount of
 * time and requires the use of delay: it is not suitable for use in callbacks.
 * @param command  The command to send, without the at$ prefix and
 *  without return carriage "\r" (see datasheet)
 * @param result  A pointer to a string to store the message returned by the
 *  BMDWare
 * @param responseLength  The expected length of result
 * @return -1 if the command failed, or the number N of char of the
 *  response. Note that it is possible that N <> responseLength.
 */

int IUBMD350::sendATCommand(String cmd, char *response, uint8_t responseLength)
{
    if (!m_ATCmdEnabled) {
        if (setupDebugMode) {
            debugPrint("BLE: Cannot send AT commands when not in AT mode");
        }
        return -1;
    }
    if(cmd == "AT")
    {
        port->write("at");
    }
    else
    {
        port->write("at$");
        int charCount = cmd.length();
        for (int i = 0; i < charCount; ++i) {
            port->write(cmd[i]);
        }
    }
    port->write('\r');
    port->flush();
    uint8_t maxI = 20;
    uint8_t i = 0;
    while (!port->available() && i < maxI) {
        delay(100);
        i++;
    }
    if (i == maxI) {
        if (setupDebugMode) {
            debugPrint("AT Command '" + cmd + "' failed");
        }
        return -1;
    }
    int respCount = 0;
    for (uint8_t j = 0; j < responseLength; ++j) {
        if (!port->available()) {
            debugPrint("No data from BMD, exiting sendATCommand() !");
            delay(100);
            break;
        }
        response[j] = port->read();
        respCount++;
        // end of response is carriage or line return
        if (response[j] == '\r' || response[j] == '\n') {
            response[j] = 0; // Replace with end of string
            if (setupDebugMode) {
                debugPrint("AT Command '" + cmd + "' Ok");
                debugPrint("AT Response ",false); debugPrint(response,true);
            }
            // Serial.println("AT Command '" + cmd + "' Ok");
            // Serial.print("AT Response "); Serial.println(response);
            // Serial.print("response Len:");Serial.print(responseLength);Serial.print("response Rx L:");Serial.println(j);
            break;
        }
    }
    // Check that port buffer is empty
    while (port->available()) {
        port->read();
    }
    return respCount;
}


/* =============================================================================
    Bluetooth Configuration
============================================================================= */

void IUBMD350::doFullConfig()
{
    port->flush();
    delay(1000);
    enterATCommandInterface();
    checkBmdComm();
    queryDeviceName();
    if(isBLEAvailable == true){
        //configureBeacon(m_beaconEnabled, m_beaconAdInterval);
        configureBeacon(false, m_beaconAdInterval); //beacon off during setup on at last
        configureUARTPassthrough();
        setPowerMode(PowerMode::REGULAR);
        
    }
    else
    {
        if (setupDebugMode) {
            debugPrint("BLE Hardware is not present",true);
        }
    }
    exitATCommandInterface();
    
}

/***** Device Name *****/

/**
 * Send an AT command to set the device name (must be in AT mode)
 *
 * @param name  The new device name, from 1 to 8 ASCII char
 * @return  True if successfully set, else false
 */
void IUBMD350::setDeviceName(char *deviceName)
{
    strcpy(m_deviceName, deviceName);
    char response[3];
    memset(response,'\0',sizeof(response));
    String cmd = "name ";
    sendATCommand((String) (cmd + m_deviceName), response, 3);
    if(strcmp(response, "OK") != 0)
    {
        if (setupDebugMode)
        {
            debugPrint(F("Failed to set device name, response was: "), false);
            debugPrint(response);
        }
        bmdCommErrCode |= 0x01;
        bmdCommErrMsgRetry = DEV_DIAG_MSG_RTRY;  // Send message multipple time, so its not missed
        memset(m_deviceName,'\0',sizeof(m_deviceName));
        strncpy(m_deviceName, deviceName,8);
        m_deviceName[8] = '\0';
        sendATCommand((String) (cmd + m_deviceName), response, 3);    
        if (strcmp(response, "OK") != 0 && setupDebugMode)
        {
            debugPrint(F("Failed to re-set device name, response was: "), false);
            debugPrint(response);
        }
    }
    else if (strcmp(response, "OK") == 0 && setupDebugMode)
    {
        debugPrint(F("set device name, response was: "), false);
        debugPrint(response);
    }
}

/**
 * Check BLE communication with basic commands
 *
 */
bool IUBMD350::checkBmdComm()
{
    char response[20];
    memset(response,'\0',sizeof(response));
    int retries = 4;
    char current_attempt[5];
    int respLen;
    for(int i=0; i<retries; i++) {
        respLen = sendATCommand("AT", response, 3);
        if(respLen > 0 && (!strcmp(response,"OK")))
        {
            if (setupDebugMode)  { 
                debugPrint("AT ", false); debugPrint(response, true); 
            }
            respLen = sendATCommand("name?", response, 9);
            if (respLen > 0) {
                if (setupDebugMode)  { 
                    debugPrint("bmd name", false); debugPrint(response, true); 
                }
            }
            else
            {
                bmdCommErrCode |= 0x10;
                bmdCommErrMsgRetry = DEV_DIAG_MSG_RTRY; // Send message 3 times
            }
            respLen = sendATCommand("mac?", response, 20);
            if (respLen > 0) {
                if (setupDebugMode)  { 
                    debugPrint("mac add", false); debugPrint(response, true); 
                }
            }
            else
            {
                bmdCommErrCode |= 0x20;
                bmdCommErrMsgRetry = DEV_DIAG_MSG_RTRY; // Send message 3 times
            }
            return true;
        }
        else {
            if (setupDebugMode)  {
                debugPrint("BMD Fails-Restarting BMD with AT Command"); 
            }
            if(i >= (retries-1)) { // All retries completed and failed for AT command
                bmdCommErrCode |= 0x08;
                bmdCommErrMsgRetry = DEV_DIAG_MSG_RTRY; // Send message multiple times
                return false;
            }
            else {
                respLen = sendATCommand("devrst", response, 3);
                delay(3000);
                respLen = sendATCommand("restart", response, 3);
                delay(3000);
            }
        }        
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
    char response[32];
    memset(response,'\0',sizeof(response));
    //int retries = 5;
    int retries = 7;
    char current_attempt[5];
    int respLen;
    int respLen_mac;
    char deviceInfo[256];
 #ifdef DEVIDFIX_TESTSTUB
    /*** For testing Mode 2: DeviceID:WIFI, APP-NO ***/ 
    uint8_t flagval1 = *(uint8_t*)(CONFIG_DEV_ID_ADDRESS);
    conductor.flagval2 = *(uint8_t*)(CONFIG_DEV_ID_ADDRESS+6);
    //Serial.print("Flag1: ");Serial.print(flagval1);Serial.print(" Flag2: ");Serial.println((char)flagval2);
    if(conductor.flagval2 == 5)
        conductor.flagval2 = 4;
 #endif
    for(int i=0; i<retries; i++) {
        respLen = sendATCommand("name?", response, 9);
#ifdef DEVIDFIX_TESTSTUB
        if (respLen > 0 && (conductor.flagval2 == '1' || conductor.flagval2 == '3' || conductor.flagval2 == 'F')) {
#else
        if (respLen > 0) {
#endif
            strcpy(m_deviceName, response);
            if (setupDebugMode)  { debugPrint("Device name", false); debugPrint(m_deviceName, true); }
            int mac_Response = iuBluetooth.sendATCommand("mac?", response, 20);
            if(debugMode){ debugPrint("BLE MAC ID:",false);debugPrint(response,true); }
            if( mac_Response < 0 || (response[0] == '0' && response[1] == '0')){
                conductor.setDeviceIdMode(false);
            }
            else {
#ifdef DEVIDFIX_TESTSTUB
                if(conductor.flagval2 == '1' || conductor.flagval2 == 'F')
                    conductor.setDeviceIdMode(true);
                /*************   TEST STUB CODE ******************************/
                else if(conductor.flagval2 == '3') // Mode 3 testing
                {
                    conductor.setDeviceIdMode(false);
                }
#else
                conductor.setDeviceIdMode(true);
#endif
            }
            break;
        } else {
            if (setupDebugMode)  {
                debugPrint("Attempt ", false); debugPrint(itoa(i, current_attempt, 10));
                debugPrint(F(" - Failed to query device name: no response"));
            }
            if(respLen == -1)
            {
                if (setupDebugMode)  { debugPrint("NOT AT Command Mode !"); }                    
                //Serial.println("NOT AT Command Mode !");
            }
            if (i >= 5)
            {   
                if (setupDebugMode)
                {
                    debugPrint("All Attemps failed, No BLE chip present");
                }
                
                #if 0
                respLen = sendATCommand("devrst", response, 3);
                delay(3000);
                respLen = sendATCommand("restart", response, 3);
                delay(3000);
                respLen = sendATCommand("name?", response, 9);
                delay(100);
                respLen_mac = sendATCommand("mac?", response, 20);
                if ((respLen > 0 || respLen_mac > 0) && conductor.flagval2 == '5') {
                    strcpy(m_deviceName, response);
                    if (setupDebugMode)  { 
                        debugPrint("Device name", false); debugPrint(m_deviceName, true); 
                        debugPrint("Mode 1 : BMD Ok on restart-Device ID:BMD APP:BMD");
                    }
                    Serial.println("DEV RESTART READ OK !");
                    bmdCommErrCode |= 0x04; // Send debug/diag message for informing after reset BMD worked
                    bmdCommErrMsgRetry = DEV_DIAG_MSG_RTRY; // Send message multiple time, not to miss on mqtt
                    conductor.setDeviceIdMode(true);
                }
                else 
                #endif

                #ifdef DEVIDFIX_TESTSTUB
                { // get bmd mac failed...
                    if(conductor.flagval2 == '2')
                        conductor.setDeviceIdMode(false);
                    /*************   TEST STUB CODE ******************************/
                    else if(conductor.flagval2 == '4') // Mode 3 testing
                        conductor.setDeviceIdMode(true);
                }
                #else
                conductor.setDeviceIdMode(false);
                #endif
                break;
                /* code */
            }
            
        }           
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
    //int retries = 5;
    int retries = 7;
    char current_attempt[5];

    char response[3];
    memset(response,'\0',sizeof(response));
    // Baud Rate
    String cmd = String("ubr ") + String(baudRate);

    for (int i = 0; i < retries; i++) {
        sendATCommand(cmd, response, 3);
        if (setupDebugMode && strcmp(response, "OK") != 0)
        {
            debugPrint("Attempt ", false); debugPrint(itoa(i, current_attempt, 10), true);
            debugPrint(F("Failed to set UART baud rate:\n  Command was: "),false);
            debugPrint(cmd);
            debugPrint(F("  Response was: "), false);
            debugPrint(response);
        }
        else 
        {
            debugPrint("Set UART baud rate");
            break;
        }
    }
    
    // Flow Control
    if (UART_FLOW_CONTROL) { cmd += "ufc 01"; }
    else { cmd = "ufc 00"; }
    for (int i = 0; i < retries; i++) {
        sendATCommand(cmd, response, 3);
        if (setupDebugMode && strcmp(response, "OK") != 0)
        {
            debugPrint("Attempt ", false); debugPrint(itoa(i, current_attempt, 10), true);
            debugPrint(F("Failed to configure UART flow control:\n  Command was: "),false);
            debugPrint(cmd);
            debugPrint(F("  Response was: "), false);
            debugPrint(response);
        }
        else 
        {
            debugPrint("Configured UART flow control");
            break;
        }
    }
    
    // Parity
    if (UART_PARITY) { cmd += "upar 01"; }
    else { cmd = "upar 00"; }
    for (int i = 0; i < retries; i++) {
        sendATCommand(cmd, response, 3);
        if (setupDebugMode && strcmp(response, "OK") != 0)
        {
            debugPrint("Attempt ", false); debugPrint(itoa(i, current_attempt, 10), true);
            debugPrint(F("Failed to configure UART parity:\n  Command was: "),false);
            debugPrint(cmd);
            debugPrint(F("  Response was: "), false);
            debugPrint(response);
        }
        else 
        {
            debugPrint("Configured UART parity", true);
            break;
        }
    }
    
    // enable / disable UART PassThrough
    if (UART_ENABLED) { cmd = "uen 01"; }
    else { cmd = "uen 00"; }
    for (int i = 0; i < retries; i++) {
        sendATCommand(cmd, response, 3);
        if (setupDebugMode && strcmp(response, "OK") != 0)
        {
            debugPrint("Attempt ", false); debugPrint(itoa(i, current_attempt, 10), true);
            debugPrint(F("Failed to enable UART:\n  Command was: "), false);
            debugPrint(cmd);
            debugPrint(F("  Response was: "), false);
            debugPrint(response);
        }
        else 
        {
            debugPrint("Enabled UART");
            break;
        }
    }
   
}


/**
 * Set Beacon configuration
 *
 * @param enabled  If true, enable Beacon, else disable the Beacon
 * @param adInterval  Beacon advertisement interval in ms - valid values range
 *  from 50ms to 4000ms.
 * @return True if configuration was successfully set, else false
 */
void IUBMD350::configureBeacon(bool enabled, uint16_t adInterval)
{
    char response[3];
    memset(response,'\0',sizeof(response));
    String cmd;
    // Set attributes
    m_beaconEnabled = enabled;
    m_beaconAdInterval = adInterval;
    // enable / disable Beacon
    if (enabled) { 
        debugPrint("beacon enable",false);
        cmd = "ben 01"; 
    }
    else { 
        debugPrint("beacon disable",false);
        cmd = "ben 00"; 
    }
    
    //int retries = 5;
    int retries = 7;
    char current_attempt[5];
    for (int i=0; i<retries; i++) {
        sendATCommand(cmd, response, 3);
        if (setupDebugMode && strcmp(response, "OK") != 0)
        {
            debugPrint("Attempt ", false); debugPrint(itoa(i, current_attempt, 10), false);
            debugPrint(F(" - Failed to enable / disable Beacon, response was: "), false);
            debugPrint(response);
        } else {
            debugPrint("Beacon configured", true);
            break;
        }
    }


    // Set Ad Interval
    cmd = "badint " + String(adInterval, HEX);
    for(int i=0; i<retries; i++) {
        sendATCommand(cmd, response, 3);
        if (setupDebugMode && strcmp(response, "OK") != 0)
        {
            debugPrint("Attempt ", false); debugPrint(itoa(i, current_attempt, 10), false);
            debugPrint(F(" - Failed to set Ad Interval, response was: "), false);
            debugPrint(response);
        } else {
            debugPrint("Beacon interval configured");
            break;
        }
    }
    

    // connectable advertisement interval
    cmd = "cadint " + String(20, HEX);
    for(int i=0; i<retries; i++) {
        sendATCommand(cmd, response, 3);
        if (setupDebugMode && strcmp(response, "OK") != 0)
        {
            debugPrint("Attempt ", false); debugPrint(itoa(i, current_attempt, 10), false);
            debugPrint(F(" - Failed to set Connectable Ad Interval, response was: "), false);
            debugPrint(response);
        } else {
            debugPrint("Connectable advertising interval configured");
            break;
        }
    }

}


/**
 * Set Beacon UUID
 *
 * @param UUID  Universal Unique Identifier as a char array of 32 hex digits
 *  (UUID is 16byte = 128bit long number)
 * @param major  UUID Major Number as a char array of 4 hex digits (16bit)
 * @param minor  UUID Minor Number as a char array of 4 hex digits (16bit)
 * @return True if configuration was successfully set, else false
 */
void IUBMD350::setBeaconUUID(char *UUID, char *major, char *minor)
{
    bool success = true;
    char response[3];
    memset(response,'\0',sizeof(response));
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

void IUBMD350::bleBeaconSetting(bool BeaconSet)
{
    // Configure BLE beacon ON and OFF for resolving ble stuck issue
    // pinMode(m_atCmdPin, OUTPUT);
    // pinMode(m_resetPin, OUTPUT);
    // begin();
    enterATCommandInterface();
    configureBeacon(BeaconSet, m_beaconAdInterval);
    exitATCommandInterface();
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
    memset(response,'\0',sizeof(response));
    //int retries = 5;
    int retries = 7;
    char current_attempt[5];

    // Connected Power
    String cmd = String("ctxpwr ") + String(txPower, HEX);
    for (int i = 0; i < retries; i++) {
        sendATCommand(cmd, response, 3);
        if (setupDebugMode && strcmp(response, "OK") != 0)
        {   
            debugPrint("Attempt ", false); debugPrint(itoa(i, current_attempt, 10));
            debugPrint(F("Failed to set Connected Tx Power:\n  Command was: "), false);
            debugPrint(cmd);
            debugPrint(F("  Response was: "), false);
            debugPrint(response);
        }
        else 
        {
            debugPrint("Connected Tx Power configured");
            break;
        }
    }
    
    // Beacon Power
    cmd = String("btxpwr ") + String(txPower, HEX); 
    for (int i = 0; i < retries; i++) {
        sendATCommand(cmd, response, 3);
        if (setupDebugMode && strcmp(response, "OK") != 0)
        {
            debugPrint("Attempt ", false); debugPrint(itoa(i, current_attempt, 10));
            debugPrint(F("Failed to set Beacon Tx Power:\n  Command was: "), false);
            debugPrint(cmd);
            debugPrint(F("  Response was: "), false);
            debugPrint(response);
        }
        else 
        {
            debugPrint("Beacon power configured");
            break;
        }
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
    #ifdef IUDEBUG_ANY
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
    #ifdef IUDEBUG_ANY
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
    #ifdef IUDEBUG_ANY
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
 * @param  char*  Pointers to char arrays to get the resulting info
 * @param  len1, len2, len3, len4  Length of arrays
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
