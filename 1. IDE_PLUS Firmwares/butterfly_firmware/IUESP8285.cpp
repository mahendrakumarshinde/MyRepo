#include "IUESP8285.h"

/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUESP8285::IUESP8285(HardwareSerial *serialPort, char *charBuffer,
                     uint16_t bufferSize, PROTOCOL_OPTIONS protocol,
                     uint32_t rate, uint16_t dataReceptionTimeout) :
    IUSerial(serialPort, charBuffer, bufferSize, protocol, rate, ';',
             dataReceptionTimeout),
    m_connected(false),
    m_sleeping(false),
    m_lastConnectedTime(0),
    m_sleepStartTime(0),
    m_autoSleepDelay(defaultAutoSleepDelay),
    m_autoSleepDuration(defaultAutoSleepDuration),
    m_working(false)
{
}


/* =============================================================================
    Hardware & power management
============================================================================= */

/**
 * Set up the component and finalize the object initialization
 */
void IUESP8285::setupHardware()
{
    // Check if UART is being driven by something else (eg: FTDI connnector)
    // used to flash the ESP8285
    // TODO Test the following
//    pinMode(UART_TX_PIN, INPUT_PULLDOWN);
//    delay(100);
//    if(digitalRead(UART_TX_PIN))
//    {
//        // UART driven by something else
//        // TODO How does this behave when there is a battery?
//        IUSerial::suspend();
//        return;
//    }
    begin();
    hardReset();
    wakeUp();
}

/**
 * Switch to ACTIVE power mode
 */
void IUESP8285::wakeUp()
{
    Component::wakeUp();
    manageAutoSleep();
}

/**
 * Switch to ECONOMY power mode
 *
 * WiFi cycle through connection attempts / sleeping phase if not connected.
 * When connected however, this mode is in practise the same as wakeUp.
 */
void IUESP8285::lowPower()
{
    Component::lowPower();
    manageAutoSleep();
}

/**
 * Switch to SUSPEND power mode
 */
void IUESP8285::suspend()
{
    // TODO Implement sleep duration
    Component::suspend();
    manageAutoSleep();
}


/* =============================================================================
    WiFi Maintenance functions
============================================================================= */

/**
 *
 */
void IUESP8285::manageAutoSleep()
{
    uint32_t now = millis();
    switch (m_powerMode)
    {
        case PowerMode::ACTIVE:
            sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
            break;
        case PowerMode::ECONOMY:
            if (m_connected)
            {
                sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
            }
            else if (m_sleeping)  // Not connected, already sleeping
            {
                if (now - m_sleepStartTime > m_autoSleepDuration)
                {
                    sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
                }
                else
                {
                    sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
                }
            }
            else  // Not connected and not sleeping
            {
                if (now - m_lastConnectedTime > m_autoSleepDelay)
                {
                    sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
                }
                else
                {
                    sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
                }
            }
            break;
        case PowerMode::SUSPEND:
            sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unmanaged power mode "), false);
                debugPrint(m_powerMode);
            }
            sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
            break;
    }
}


/* =============================================================================
    Inbound communication
============================================================================= */

/**
 *
 */
bool IUESP8285::processMessage()
{
    bool commandFound = true;
    switch (m_mspCommand)
    {
        case MSPCommand::WIFI_ALERT_CONNECTED:
            m_connected = true;
            m_working = false;
            m_lastConnectedTime = millis();
            break;
        case MSPCommand::WIFI_ALERT_DISCONNECTED:
            m_connected = false;
            m_working = false;
            break;
        case MSPCommand::WIFI_ALERT_NO_SAVED_CREDENTIALS:
            m_working = false;
            break;
        case MSPCommand::WIFI_ALERT_AWAKE:
            m_sleeping = false;
            break;
        case MSPCommand::WIFI_ALERT_SLEEPING:
            m_sleeping = true;
            m_sleepStartTime = millis();
            break;
        case MSPCommand::WIFI_REQUEST_ACTION:
            manageAutoSleep();
            break;
        default:
            commandFound = false;
            break;
    }
    return commandFound;
}


/* =============================================================================
    Outbound communication
============================================================================= */

/**
 *
 */
void IUESP8285::setSSID(char *ssid)
{
    sendMSPCommand(MSPCommand::WIFI_RECEIVE_SSID, ssid);
    m_working = true;
}

/**
 *
 */
void IUESP8285::setPassword(char *password)
{
    sendMSPCommand(MSPCommand::WIFI_RECEIVE_PASSWORD, password);
    m_working = true;
}

/**
 *
 */
void IUESP8285::forgetCredentials()
{
    sendMSPCommand(MSPCommand::WIFI_FORGET_CREDENTIALS);
    m_working = true;
}

/**
 *
 */
void IUESP8285::connect()
{
    sendMSPCommand(MSPCommand::WIFI_CONNECT);
    m_working = true;
}

/**
 *
 */
void IUESP8285::disconnect()
{
    sendMSPCommand(MSPCommand::WIFI_DISCONNECT);
    m_working = true;
}


/* =============================================================================
    Instanciation
============================================================================= */

char iuWiFiBuffer[500] = "";

#ifdef EXTERNAL_WIFI
    IUSerial iuWiFi(StreamingMode::WIFI, &Serial3, iuWiFiBuffer, 500,
                    115200, ';', 500);
#else
    IUESP8285 iuWiFi(&Serial3, iuWiFiBuffer, 500, IUSerial::LEGACY_PROTOCOL,
                     115200, 100);
#endif
