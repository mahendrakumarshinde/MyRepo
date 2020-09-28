#include "FeatureGroup.h"
#include "RawDataState.h"
//#include "IULSM6DSM.h"

//extern Feature feature;
/* =============================================================================
    Constructors & destructor
============================================================================= */

uint8_t FeatureGroup::instanceCount = 0;

FeatureGroup *FeatureGroup::instances[FeatureGroup::MAX_INSTANCE_COUNT] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

FeatureGroup::FeatureGroup(const char *name, uint16_t dataSendPeriod) :
    m_active(false),
    m_dataSendPeriod(dataSendPeriod),
    m_bufferIndex(0),
    m_bufferStartTime(0)
{
    strcpy(m_name, name);
    m_charBufferNode = NULL;
    reset();
    m_lastSentTime[0] = 0;
    m_lastSentTime[1] = 0;
    // Instance registration
    m_instanceIdx = instanceCount;
    instances[m_instanceIdx] = this;
    instanceCount++;
}

FeatureGroup::~FeatureGroup()
{
    instances[m_instanceIdx] = NULL;
    for (uint8_t i = m_instanceIdx + 1; i < instanceCount; ++i)
    {
        instances[i]->m_instanceIdx--;
        instances[i -1] = instances[i];
    }
    instances[instanceCount] = NULL;
    instanceCount--;
}

FeatureGroup *FeatureGroup::getInstanceByName(const char *name)
{
    for (uint8_t i = 0; i < instanceCount; ++i)
    {
        if (instances[i] != NULL)
        {
            if(instances[i]->isNamed(name))
            {
                return instances[i];
            }
        }
    }
    return NULL;
}


/***** Core *****/

/**
 * Remove all the features from the group
 */
void FeatureGroup::reset()
{
    m_featureCount = 0;
    for (uint8_t i = 0; i < maxFeatureCount; ++i)
    {
        m_features[i] = NULL;
    }
}

/**
 * Append a feature to the group
 */
void FeatureGroup::addFeature(Feature *feature)
{
    if (m_featureCount >= maxFeatureCount) {
        if (debugMode) {
            debugPrint(F("FeatureGroup: too many features"));
        }
    }
    m_features[m_featureCount] = feature;
    m_featureCount++;
}


/* =============================================================================
    Time management
============================================================================= */

/**
 * Set the data send interval.
 */
void FeatureGroup::setDataSendPeriod(uint16_t dataSendPeriod)
{
    m_dataSendPeriod = dataSendPeriod;
    if (debugMode) {
        debugPrint(F("Group "), false);
        debugPrint(m_name, false);
        debugPrint(F(": data send period set to "), false);
        debugPrint(m_dataSendPeriod);
    }
}

/**
 * Check if it is time to send data based on m_dataSendPeriod.
 *
 * NB: Data streaming is always disabled when in "sleep" operation mode.
 */
bool FeatureGroup::isDataSendTime(uint8_t idx)
{
    // Timer to send data regularly
    uint32_t now = millis();
    if (now - m_lastSentTime[idx] > m_dataSendPeriod)
    {
        m_lastSentTime[idx] = now;
        return true;
    }
    return false;
}


/* =============================================================================
    Communication
============================================================================= */

/**
 * Tell whether the feature group and its features fulfill the conditions to
 * stream the next message.
 *
 * FeatureGroups only stream when the following conditions are fullfilled:
 *  - The feature group has some features and is active.
 *  - All features have been fully computed (feature.hasBeenFilledOnce returns
 *    true for each feature).
 *  - There is no data error in any of the features
 *    (feature.latestSectionsHaveDataError returns false for each feature).
 *  - If all the previous conditions are met, the last one is that enough time
 *    must have passed since the last message sent for the given port index
 *    (FeatureGroup.isDataSendTime(portIdx) returns true).
 *
 */
bool FeatureGroup::isReadyToStream(uint8_t portIdx)
{
    // Only stream if group is active and has some features
    if (m_featureCount == 0 || !m_active) {
        return false;
    }
    // Only stream if all the features have been fully computed and have no data
    // error.
    for (uint8_t i = 0; i < m_featureCount; i++) {
        if (!m_features[i]->hasBeenFilledOnce() ||
            m_features[i]->latestSectionsHaveDataError(1))
        {
            return false;
        }
    }
    if (!isDataSendTime(portIdx)) {
        return false;
    }
    return true;

}

/**
 * Sends the values of the group features through given serial.
 *
 * Use the new feature data message specification and the MSP protocol.
 */
void FeatureGroup::MSPstream(IUSerial *iuSerial, MacAddress mac,
                             double timestamp, bool sendMACAddress,
                             uint8_t portIdx)
{
    // TODO: Reimplement to stream via MSP

//    if (!isReadyToStream(portIdx)) {
//        return;  // Not ready to stream
//    }
//    strlen(m_name) + 1 + 17 + 1
//
//
//
//    iuSerial.port->print(m_name);  // char count = strlen(m_name)
//    if (sendMACAddress)
//    {
//        iuSerial.port->print(",");  // char count = 1
//        iuSerial.port->print(mac);  // char count = 17
//    }
//    for (uint8_t i = 0; i < m_featureCount; ++i)
//    {
//        if (m_features[i] != NULL)
//        {
//            iuSerial.port->print(",");  // char count = 1
//            iuSerial.port->print(m_features[i]->getName());  // char count = nameLength
//            m_features[i]->stream(port);
//        }
//    }
//    iuSerial.port->print(",");
//    iuSerial.port->print(timestamp);
//    iuSerial.port->print(";");
//    if (loopDebugMode)  // Add a line break to ease readability in debug mode
//    {
//        port->println("");
//    }
}


/**
 * Sends the values of the group features through given serial.
 */
void FeatureGroup::legacyStream(IUSerial *iuSerial, MacAddress mac,
                                uint8_t opState, float batteryLoad,
                                double timestamp, bool sendName,
                                uint8_t portIdx)
{
    if (!isReadyToStream(portIdx)) {
        return;  // Not ready to stream
    }
    if (sendName) {
        iuSerial->write(m_name);
        iuSerial->write(',');
    }
    iuSerial->write(mac.toString().c_str());
    iuSerial->write(",0");
    iuSerial->write(String((uint8_t) opState, DEC).c_str());
    iuSerial->write(',');
    iuSerial->write(String((int) round(batteryLoad), DEC).c_str());
    for (uint8_t i = 0; i < m_featureCount; ++i) {
        iuSerial->write(",000");
        iuSerial->write(String(i + 1, DEC).c_str());
        if (m_features[i] != NULL) {
            m_features[i]->stream(iuSerial);
            //Serial.print(m_features[i]->getName());Serial.print(",");
        }
    }
    iuSerial->write(',');
    iuSerial->write(String(timestamp, 2).c_str());
    iuSerial->write(';'); 
    if (featureDebugMode) {
        debugPrint(millis(), false);
        debugPrint(F(" -> "), false);
        debugPrint(m_name, false);
        debugPrint(F(": "), false);
        for (uint8_t i = 0; i < m_featureCount; ++i) {
            debugPrint(",000", false);
            debugPrint(i + 1, false);
            if (m_features[i] != NULL) {
                debugPrint("val", false);
            }
        }
        debugPrint("");
    }

}

///**
// * Buffers the feature then send them through iuSerial in one shot.
// *
// * Can use either the IUSerial MSP protocol or the IUSerial Legacy protocol.
// */
void FeatureGroup::bufferAndStream(
   IUSerial *iuSerial, IUSerial::PROTOCOL_OPTIONS protocol, MacAddress mac,
   uint8_t opState, float batteryLoad, double timestamp,
   bool sendName, uint8_t portIdx)
{
   if (!isReadyToStream(portIdx)) {
       return;  // Not ready to stream
   }
   //Serial.print("isISRDisabled : ");Serial.println(FeatureStates::isISRDisabled);
   uint32_t now = millis();
//    Serial.print("m_bufferStartTime : ");Serial.println(m_bufferStartTime);
//    Serial.print("maxBufferDelay :");Serial.println(now - m_bufferStartTime);
//    Serial.print("maxBufferMargin :");Serial.println(maxBufferSize - m_bufferIndex);
   if (m_bufferStartTime == 0 ||
       now - m_bufferStartTime > maxBufferDelay ||
       maxBufferSize - m_bufferIndex < maxBufferMargin) {
       // Send current data
       m_featureBuffer[m_bufferIndex] = 0;
       if (protocol == IUSerial::LEGACY_PROTOCOL) {
           iuSerial->port->print(m_featureBuffer);
           if (loopDebugMode)
           {
               iuSerial->port->println("");
           }
       } else if (protocol == IUSerial::MS_PROTOCOL /* && FeatureStates::isISRDisabled */) {
            iuSerial->sendMSPCommand(MSPCommand::PUBLISH_FEATURE,
                                    m_featureBuffer);
            //Serial.print(" m_featureBuffer : ");Serial.println(m_featureBuffer);
            //Serial.println("ISR attachInterrupt !!!");
            //isFeatureStreamComplete = true;
            // if (FeatureStates::isISRActive != true && FeatureStates::isISRDisabled)
            // {   
            //     FeatureStates::isFeatureStreamComplete = true;   // publication completed
            //     FeatureStates::isISRActive = true;
            //     //Serial.println("isISRActive is true");
            // }
            
            
       }
       // Reset buffer
       m_bufferIndex = 0;
       m_bufferStartTime = now;
       for (uint16_t i = 0; i < maxBufferSize; ++i) {
           m_featureBuffer[i] = 0;
       }
   }
   if (sendName) {
       // Always send for Legacy protocol, send once for MSP
       if (protocol == IUSerial::LEGACY_PROTOCOL || m_bufferIndex == 0) {
           strcat(m_featureBuffer, m_name);
           m_bufferIndex += strlen(m_name);
           m_featureBuffer[m_bufferIndex++] = ',';
       }
   }
   strncat(m_featureBuffer, mac.toString().c_str(), 17);
   m_bufferIndex += 17;
   m_featureBuffer[m_bufferIndex++] = ',';
   m_featureBuffer[m_bufferIndex++] = '0';
   m_featureBuffer[m_bufferIndex++] = opState + 48;
   m_featureBuffer[m_bufferIndex++] = ',';
   String stringBattery((int) round(batteryLoad));
   strcat(m_featureBuffer, stringBattery.c_str());
   m_bufferIndex += stringBattery.length();
   for (uint8_t i = 0; i < m_featureCount; ++i) {
       m_featureBuffer[m_bufferIndex++] = ',';
       m_featureBuffer[m_bufferIndex++] = '0';
       m_featureBuffer[m_bufferIndex++] = '0';
       m_featureBuffer[m_bufferIndex++] = '0';
       m_featureBuffer[m_bufferIndex++] = i + 49;
       if(i >= 9){
           m_featureBuffer[m_bufferIndex - 2] = '1';
           m_featureBuffer[m_bufferIndex - 1] = i + 39;
           //m_bufferIndex++;
       }
       if (m_features[i] != NULL) {
           m_bufferIndex += m_features[i]->sendToBuffer(m_featureBuffer,
                                                        m_bufferIndex);
           }
   }
   m_featureBuffer[m_bufferIndex++] = ',';
   String stringTS = String(timestamp, 2);
   strcat(m_featureBuffer, stringTS.c_str());
   m_bufferIndex += stringTS.length();
   m_featureBuffer[m_bufferIndex++] = ';';
    // Serial.print("ISR Disabled Time : ");Serial.println(m_features);
    //Serial.print("Time to publised : ");Serial.println(isr_featureData_publish);
    //Serial.print("Feature Buffers : ");Serial.println(m_featureBuffer); 
    //attachInterrupt(digitalPinToInterrupt(IULSM6DSM::INT1_PIN),dataAcquisitionISR,RISING);
    
}

/**
 *
 */
void FeatureGroup::bufferAndQueue(
    CharBufferSendingQueue *sendingQueue, IUSerial::PROTOCOL_OPTIONS protocol,
    MacAddress mac, uint8_t opState, float batteryLoad, double timestamp,
    bool sendName, uint8_t portIdx)
{
    if (!isReadyToStream(portIdx)) {
        return;  // Not ready to stream
    }
    uint32_t now = millis();
    if (m_bufferStartTime == 0 ||
        now - m_bufferStartTime > maxBufferDelay ||
        CharBufferNode::bufferSize - m_bufferIndex < maxBufferMargin) {
        if (m_charBufferNode) {
          //Serial.print("SENDCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC");
        // Send current data
            m_charBufferNode->buffer[m_bufferIndex] = 0;
            sendingQueue->finishedWriting(m_charBufferNode->idx);
        }
        // Get new buffer and reset counter
        m_charBufferNode = sendingQueue->getNextBufferToWrite();
        sendingQueue->startedWriting(m_charBufferNode->idx);
        m_bufferIndex = 0;
        m_bufferStartTime = now;
    }
    if (sendName) {
        // Always send for Legacy protocol, send once for MSP
        if (protocol == IUSerial::LEGACY_PROTOCOL || m_bufferIndex == 0) {
          //Serial.print("LEAGACY$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$");
            strcat(m_charBufferNode->buffer, m_name);
            m_bufferIndex += strlen(m_name);
            m_charBufferNode->buffer[m_bufferIndex++] = ',';
        }
    }
    strncat(m_charBufferNode->buffer, mac.toString().c_str(), 17);
    m_bufferIndex += 17;
    m_charBufferNode->buffer[m_bufferIndex++] = ',';
    m_charBufferNode->buffer[m_bufferIndex++] = '0';
    m_charBufferNode->buffer[m_bufferIndex++] = opState + 48;
    m_charBufferNode->buffer[m_bufferIndex++] = ',';
    String stringBattery((int) round(batteryLoad));
    strcat(m_charBufferNode->buffer, stringBattery.c_str());
    m_bufferIndex += stringBattery.length();
    for (uint8_t i = 0; i < m_featureCount; ++i) {
        m_charBufferNode->buffer[m_bufferIndex++] = ',';
        m_charBufferNode->buffer[m_bufferIndex++] = '0';
        m_charBufferNode->buffer[m_bufferIndex++] = '0';
        m_charBufferNode->buffer[m_bufferIndex++] = '0';
        m_charBufferNode->buffer[m_bufferIndex++] = i + 49;
        if (m_features[i] != NULL) {
            m_bufferIndex += m_features[i]->sendToBuffer(
                m_charBufferNode->buffer, m_bufferIndex);
        }
    }
    m_charBufferNode->buffer[m_bufferIndex++] = ',';
    String stringTS = String(timestamp, 2);
    strcat(m_charBufferNode->buffer, stringTS.c_str());
    m_bufferIndex += stringTS.length();
    m_charBufferNode->buffer[m_bufferIndex++] = ';';
}

