#include "IUESP8285.h"

/* =============================================================================
    Constructor & desctructors
============================================================================= */

IUESP8285::IUESP8285(HardwareSerial *serialPort, char *charBuffer,
                     uint16_t bufferSize, PROTOCOL_OPTIONS protocol,
                     uint32_t rate, char stopChar,
                     uint16_t dataReceptionTimeout) :
    IUSerial(serialPort, charBuffer, bufferSize, protocol, rate, stopChar,
             dataReceptionTimeout)
{
    m_credentialValidator.setTimeout(wifiConfigReceptionTimeout);
    m_staticConfigValidator.setTimeout(wifiConfigReceptionTimeout);
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
    setPowerMode(PowerMode::REGULAR);
}

/**
 * Manage component power modes
 */
void IUESP8285::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;
    manageAutoSleep();
}


/* =============================================================================
    WiFi Maintenance functions
============================================================================= */

/**
 * Send commands to WiFi chip to manage its sleeping cycle.
 *
 * When not connected, WiFi cycle through connection attempts / sleeping phase.
 * When connected, WiFi use light-sleep mode to maintain connection to AP at a
 * lower energy cost.
 */
void IUESP8285::manageAutoSleep()
{
    uint32_t now = millis();
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:
        case PowerMode::ENHANCED:
            sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
            break;
        case PowerMode::REGULAR:
        case PowerMode::LOW_1:
        case PowerMode::LOW_2:
            Serial.println("here 1");
            if (m_connected)
            {
                sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
                m_awakeTimerStart = now;
            }
            else if (m_sleeping)  // Not connected, already sleeping
            {
                if (now - m_sleepTimerStart > m_autoSleepDuration)
                {
                    sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
                    m_awakeTimerStart = now;  // Reset auto-sleep start timer
                }
                else
                {
                    sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
                }
            }
            else  // Not connected and not sleeping
            {
                if (now - m_awakeTimerStart > m_autoSleepDelay)
                {
                    sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
                    m_sleepTimerStart = now;  // Reset auto-sleep start timer
                }
                else
                {
                    sendMSPCommand(MSPCommand::WIFI_WAKE_UP);
                }
            }
            break;
        case PowerMode::SLEEP:
        case PowerMode::DEEP_SLEEP:
        case PowerMode::SUSPEND:
            // TODO Implement sleep duration
            sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
            sendMSPCommand(MSPCommand::WIFI_DEEP_SLEEP);
    }
    if ((uint8_t) m_powerMode > (uint8_t) PowerMode::SLEEP && !m_sleeping)
    {
        sendWiFiCredentials();
        sendMSPCommand(MSPCommand::ASK_WIFI_MAC);
    }
}


/* =============================================================================
    Network Configuration
============================================================================= */

void IUESP8285::setSSID(char *ssid, uint8_t length)
{
    Serial.print("here 61: ");
    Serial.println(ssid);
    if (m_credentialValidator.hasTimedOut())
    {
        Serial.println("here 62");
        m_credentialValidator.reset();
    }
    uint8_t charCount = min(wifiCredentialLength, length);
    strncpy(m_ssid, ssid, charCount);
    for (uint8_t i = charCount; i < wifiCredentialLength; ++i)
    {
        m_ssid[i] = 0;
    }
    Serial.print("here 63");
    Serial.println(m_ssid);
    m_credentialValidator.receivedMessage(0);
    Serial.print("here 64: ");
    Serial.println(m_credentialValidator.completed());
}

void IUESP8285::setPassword(char *psk, uint8_t length)
{
    Serial.print("here 71: ");
    Serial.println(psk);
    if (m_credentialValidator.hasTimedOut())
    {
        Serial.println("here 72: ");
        m_credentialValidator.reset();
    }
    uint8_t charCount = min(wifiCredentialLength, length);
    strncpy(m_psk, psk, charCount);
    for (uint8_t i = charCount; i < wifiCredentialLength; ++i)
    {
        m_psk[i] = 0;
    }
    Serial.print("here 73: ");
    Serial.println(m_psk);
    m_credentialValidator.receivedMessage(1);
    Serial.print("here 74: ");
    Serial.println(m_credentialValidator.completed());
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
        case MSPCommand::RECEIVE_WIFI_MAC:
            Serial.println("RECEIVE_WIFI_MAC");
            m_macAddress = mspReadMacAddress();
            break;
        case MSPCommand::WIFI_CONFIRM_NEW_CREDENTIALS:
            Serial.println("WIFI_CONFIRM_NEW_CREDENTIALS");
            m_credentialReceptionConfirmed = true;
            break;
        case MSPCommand::WIFI_CONFIRM_NEW_STATIC_CONFIG:
            Serial.println("WIFI_CONFIRM_NEW_STATIC_CONFIG");
            m_staticConfigReceptionConfirmed = true;
            break;
        case MSPCommand::WIFI_ALERT_CONNECTED:
            Serial.println("WIFI_ALERT_CONNECTED");
            m_connected = true;
            m_working = false;
            m_awakeTimerStart = millis();
            break;
        case MSPCommand::WIFI_ALERT_DISCONNECTED:
            Serial.println("WIFI_ALERT_DISCONNECTED");
            m_connected = false;
            m_working = false;
            break;
        case MSPCommand::WIFI_ALERT_NO_SAVED_CREDENTIALS:
            Serial.println("WIFI_ALERT_NO_SAVED_CREDENTIALS");
            m_working = false;
            break;
        case MSPCommand::WIFI_ALERT_SLEEPING:
            Serial.println("WIFI_ALERT_SLEEPING");
            m_sleeping = true;
            break;
        case MSPCommand::WIFI_ALERT_AWAKE:
            Serial.println("WIFI_ALERT_AWAKE");
            m_sleeping = false;
            break;
        case MSPCommand::WIFI_REQUEST_ACTION:
            Serial.println("WIFI_REQUEST_ACTION");
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
void IUESP8285::sendWiFiCredentials()
{
    Serial.println("here 50");
    if (m_credentialValidator.completed())
    {
        Serial.println("here 51");
        sendMSPCommand(MSPCommand::WIFI_RECEIVE_SSID, m_ssid);
        sendMSPCommand(MSPCommand::WIFI_RECEIVE_PASSWORD, m_psk);
        m_credentialSent = true;
        m_credentialReceptionConfirmed = false;
        m_working = true;
    }
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
void IUESP8285::sendStaticConfig()
{
    if (m_staticConfigValidator.completed())
    {
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_STATIC_IP, m_staticIp);
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_GATEWAY, m_staticGateway);
        mspSendIPAddress(MSPCommand::WIFI_RECEIVE_SUBNET, m_staticSubnet);
        m_staticConfigSent = true;
        m_staticConfigReceptionConfirmed = false;
        m_working = true;
    }
}

/**
 *
 */
void IUESP8285::forgetStaticConfig()
{
    sendMSPCommand(MSPCommand::WIFI_FORGET_STATIC_CONFIG);
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
