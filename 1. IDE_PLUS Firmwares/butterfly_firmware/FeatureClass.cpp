#include "FeatureClass.h"
#include "IULSM6DSM.h"
#include "RawDataState.h"
#include "IUKX222.h"
#include "FFTConfiguration.h"
//#include "FeatureGroup.h"
/* =============================================================================
    Core
============================================================================= */

uint8_t Feature::instanceCount = 0;

Feature *Feature::instances[Feature::MAX_INSTANCE_COUNT] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

Feature::Feature(const char* name, uint8_t sectionCount, uint16_t sectionSize,
                 Feature::slideOption sliding, bool isFFT,
                 bool isOverWrittable) :
    m_isFFT(isFFT),
    m_sectionCount(sectionCount),
    m_sectionSize(sectionSize),
    m_sliding(sliding),
    m_overWrittable(isOverWrittable)
{
    strcpy(m_name, name);
    m_totalSize = m_sectionCount * m_sectionSize;
    for (uint8_t i = 0; i < maxReceiverCount; i++) {
        m_receivers[i] = NULL;
    }
    reset();
    // Instance registration
    m_instanceIdx = instanceCount;
    instances[m_instanceIdx] = this;
    instanceCount++;
}

/**
 * Delete from class registry + remove pointers to receivers / computers
 */
Feature::~Feature()
{
    instances[m_instanceIdx] = NULL;
    for (uint8_t i = m_instanceIdx + 1; i < instanceCount; ++i) {
        instances[i]->m_instanceIdx--;
        instances[i -1] = instances[i];
    }
    instances[instanceCount] = NULL;
    instanceCount--;
    for (uint8_t i = 0; i < maxReceiverCount; i++) {
        m_receivers[i] = NULL;
    }
    m_computer = NULL;
}

Feature *Feature::getInstanceByName(const char *name)
{
    for (uint8_t i = 0; i < instanceCount; ++i) {
        if (instances[i] != NULL && instances[i]->isNamed(name)) {
            return instances[i];
        }
    }
    return NULL;
}


/* =============================================================================
    Receivers tracking
============================================================================= */

/**
 * Add the receiver to the Feature receivers list.
 */
void Feature::addReceiver(FeatureComputer *receiver)
{
    if (m_receiverCount < maxReceiverCount) {
        m_receivers[m_receiverCount] = receiver;
        m_computeIndex[m_receiverCount] = m_recordIndex;
        for (uint8_t j = 0; j < maxSectionCount; ++j) {
            m_acknowledged[j][m_receiverCount] = true;
        }
        m_receiverCount++;
    } else if (debugMode) {
        debugPrint(F("Feature can't add receiver (buffer overflow)"));
    }
}

/**
 * Delete the receiver with the corresponding receiverId.
 *
 * @return True if the receiver was found and removed, else false.
 */
bool Feature::removeReceiver(FeatureComputer *receiver)
{
    int idx = getReceiverIndex(receiver);
    if (idx < 0) {
        return false;
    } else {
        for (uint8_t i = uint8_t(idx); i < m_receiverCount - 1; ++i) {
            m_receivers[i] = m_receivers[i + 1];
            m_computeIndex[i] = m_computeIndex[i + 1];
            for (uint8_t j = 0; j < maxSectionCount; ++j) {
                m_acknowledged[j][i] = m_acknowledged[j][i + 1];
            }
        }
        m_receiverCount--;
        m_receivers[m_receiverCount] = NULL;
        m_computeIndex[m_receiverCount] = 0;
        for (uint8_t j = 0; j < maxSectionCount; ++j) {
            m_acknowledged[j][m_receiverCount] = true;
        }
        return true;
    }
}

/**
 * Return the current index of the receiver for the feature.
 *
 * @return The index of the receiver or -1 if the receiver was not found.
 */
int Feature::getReceiverIndex(FeatureComputer *receiver)
{
    for (uint8_t i = 0; i < m_receiverCount; i++) {
        if (m_receivers[i] == receiver) {
            return int(i);
        }
    }
    if (debugMode) { debugPrint(F("Receiver not found")); }
    return -1;
}


/* =============================================================================
    Buffer core
============================================================================= */

/**
 * Reset the buffer to an "empty" state, as if just created
 */
void Feature::reset()
{
    m_filledOnce = false;
    m_fillingIndex = 0;
    m_recordIndex = 0;
    for (uint8_t i = 0; i < maxReceiverCount; ++i) {
        m_computeIndex[i] = 0;
    }
    // All sections are set as ready to record: not locked, not published and
    // acknowledged by all receivers.
    for (uint8_t j = 0; j < maxSectionCount; ++j) {
        m_locked[j] = false;
        m_published[j] = false;
        m_dataError[j] = false;
        for (uint8_t k = 0; k < maxReceiverCount; ++k) {
            m_acknowledged[j][k] = true;
        }
    }
}


/***** Filling the buffer *****/

/**
 * Increment the index used to record data in the buffer
 *
 * Also publish the sections if it has been entirely recorded.
 * Publishing a section set its "published" boolean to true, and the
 * "acknowledged" booleans of all the receivers to false.
 * recordIndex (index of current recording section) is also moved forward.
 */
void Feature::incrementFillingIndex()
{
    m_fillingIndex++;
    bool newFullSection = false;
    bool isrFlag = false;
    static int isrCount;
    // int copyRecorIndex;
    // int copyFillingIndex;
    // int copyReceiverCount;
   #if 0 
    if(strcmp(getName(),"A0X") == 0 ){
        if(m_recordIndex == 31){
            Serial.print("Record Index : ");Serial.println(m_recordIndex);
            Serial.print("Filling Index count :");
            Serial.println(m_fillingIndex);
        }
    }
    #endif
    // Publish the current section if has been entirely recorded
    if (m_fillingIndex >= (m_recordIndex + 1) * m_sectionSize) {   // ( [0:8192] >= ([0:63] + 1)*128 ) 
        m_published[m_recordIndex] = true;                         // publish last filled section
        for (uint8_t j = 0; j < m_receiverCount; ++j) {            // m_receiverCount = 2
            m_acknowledged[m_recordIndex][j] = false;              // ack[0:63][0],qck[0:63][1],ack[0:63][2]
        }                                                           
        
        if(strcmp(getName(),"A0Z") == 0 && (m_recordIndex == FFTConfiguration::currentBlockSize/getSectionSize() -1) /* && m_totalSize >= 4096 && m_fillingIndex >= 4096 */ ){
                // isrCount++;
                // Serial.print("ISR count : ");Serial.println(isrCount);
                //if(isrCount == 10){ 
                    // Disabled the ISR
                    if ( FFTConfiguration::currentSensor == FFTConfiguration::lsmSensor)
                    {
                        detachInterrupt(digitalPinToInterrupt(IULSM6DSM::INT1_PIN));
                    }
                    else
                    {
                        detachInterrupt(digitalPinToInterrupt(IUKX222::INT1_PIN));
                    }
                    //isr_detached_startTime = micros();
                    FeatureStates::isr_stopTime = micros();
                    FeatureStates::elapsedTime = (FeatureStates::isr_stopTime-FeatureStates::isr_startTime)/1000000;
                    debugPrint("Elapsed Time in sec : ",false);
                    debugPrint(FeatureStates::elapsedTime,true);
                    FeatureStates::outputFrequency = round(FFTConfiguration::currentBlockSize/FeatureStates::elapsedTime);
                    debugPrint("Output Frequency in Hz : ",false);
                    debugPrint(FeatureStates::outputFrequency,true);
                    FeatureStates::isISRActive = false;
                    FeatureStates::isISRDisabled = true;
                    // isrFlag = true;
                    //isrCount = 0;
                    // Serial.println("ISR Disabled !!!");
                    
                //}
                
                //FeatureGroup::isFeatureStreamComplete = false;
        }   
            
        m_recordIndex = (m_recordIndex + 1) % m_sectionCount;       // m_recordIndex++
        newFullSection = true;
        // Clear the data error flag for the next section
        m_dataError[m_recordIndex] = false;
        
    }
    // if(isrFlag){
    //     Serial.print("m_fillingIndex : ");
    //     Serial.println(m_fillingIndex);
    //     Serial.print("m_recordIndex : ");
    //     Serial.println(m_recordIndex);
    //}
    // Filling Index restart from 0 if needed       // Starting pointer for data filling Index rest to 0 (m_totalSize = 8192 )
    if (m_fillingIndex >= m_totalSize) {
        m_filledOnce = true;
       #if 0 
        if(m_fillingIndex == 4096 && strcmp(getName(),"A0Z") == 0 ) {
             Serial.print("m_totalSize : ");Serial.println(m_totalSize);
             Serial.print("Feature name : ");Serial.println(getName());
             Serial.print("m_fillingIndex IN : ");
             Serial.println(m_fillingIndex);
             Serial.println("ISR Disabled !!!");
             detachInterrupt(digitalPinToInterrupt(IULSM6DSM::INT1_PIN) );
             FeatureStates::isISRActive = false;
             FeatureStates::isISRDisabled = true;
          }
        #endif

        m_fillingIndex %= m_totalSize;
        //Serial.println("ISR Disabled **********");  // 4096 buffer filled Completely
        #if 0
         if( strcmp(getName(),"A0X") == 0 ) {
            Serial.println("A0X is Filled");
            Serial.print("Total Size X:");
            Serial.println(m_totalSize);
            Serial.print("Section Size X:");
            Serial.println(m_sectionSize);
            Serial.print(" 1 Section Filled :");
            Serial.println(copyFillingIndex);
            Serial.print("Record Index:");
            Serial.println(copyRecorIndex);
            Serial.print("ReceiverCount : X");
            Serial.println(copyReceiverCount);
            Serial.print("current Filling Index:");
            Serial.println(count);
            
            }
        if( strcmp(getName(),"A0Y") == 0){
            Serial.println("A0Y is Filled");
            Serial.print("Total Size Y:");
            Serial.println(m_totalSize);
            Serial.print("Section Size Y:");
            Serial.println(m_sectionSize);
            Serial.print(" 1 Section Filled :");
            Serial.println(copyFillingIndex);
            Serial.print("Record Index:");
            Serial.println(copyRecorIndex);
            Serial.print("ReceiverCount : Y");
            Serial.println(copyReceiverCount);
            Serial.print("current Filling Index:");
            Serial.println(count);
            
        }    
        if( strcmp(getName(),"A0Z")== 0){
            Serial.println("A0Z is Filled");
            Serial.print("Total Size Z:");
            Serial.println(m_totalSize);
            Serial.print("Section Size Z:");
            Serial.println(m_sectionSize);
            Serial.print(" 1 Section Filled :");
            Serial.println(copyFillingIndex);
            Serial.print("Record Index:");
            Serial.println(copyRecorIndex);
            Serial.print("ReceiverCount : Z");
            Serial.println(copyReceiverCount);
            Serial.print("current Filling Index:");
            Serial.println(count);
  
        } 
        if(strcmp(getName(),"A7X") == 0){
            Serial.print("A7X Section Size: ");
            Serial.println(m_sectionSize);
            Serial.print("ReceiverCount A7X: ");
            Serial.println(copyReceiverCount);
        }
        #endif
        //  if(count >= 1000){
        //     //dittached ISR after N-counts
        //     //detachInterrupt(digitalPinToInterrupt(IULSM6DSM::INT1_PIN));
        //     // Stop filling the data into buffer
        // }

        //count++;
        
    }
    // Run the callbacks
    if (m_onNewValue != NULL) {
        m_onNewValue(this);
    }
    if (newFullSection && m_onNewRecordedSection != NULL) {
        m_onNewRecordedSection(this);
    }
}


/***** Tracking the buffer state *****/

/**
 * Check if the section(s) of the buffer are ready for recording
 *
 * Ready for recording means not locked, not already published and acknowledged
 * by all receivers.
 * Features that are overWrittable or that have no receivers are
 * "ready to record" provided that the current recording section is not locked.
 */
bool Feature::isReadyToRecord(uint8_t sectionCount)
{
    uint8_t k;
    if (m_overWrittable || m_receiverCount == 0) {
        for (uint8_t i = m_recordIndex; i < m_recordIndex + sectionCount; ++i) {
            if (m_locked[k]) {
                return false;
            }
            m_published[k] = false;
        }
    } else {
        for (uint8_t i = m_recordIndex; i < m_recordIndex + sectionCount; ++i) {
            k = i % m_sectionCount;
            if (m_locked[k] || m_published[k]) {
                return false;
            }
            for (uint8_t j = 0; j < m_receiverCount; ++j) {
                if (!m_acknowledged[k][j]) {
                    return false;
                }
            }
        }
    }
    return true;
}

/**
 * Check if the next section(s) of the buffer are ready for computation
 *
 * Ready for computation means the sections must be not locked, fully recorded,
 * not yet received by this receiver.
 */
bool Feature::isReadyToCompute(uint8_t receiverIdx, uint8_t sectionCount,
                               bool computeLast)
{
    uint8_t k;
    uint8_t idx = m_computeIndex[receiverIdx];
    for (uint8_t i = idx; i < idx + sectionCount; ++i) {
        k = i % m_sectionCount;
        if (m_locked[k] || !m_published[k] || m_acknowledged[k][receiverIdx]) {
            //digitalWrite(7,HIGH);
            return false;
        }
        if (computeLast) {
            for (uint8_t j = 0; j < m_receiverCount; j++) {
                if (j != receiverIdx && !m_acknowledged[k][j]) {
                    return false;
                }
            }
        }
    }
    return true;
}

/**
 * Check if the next section(s) of the buffer are ready for computation
 *
 * See Feature::isReadyToCompute(uint8_t receiverIdx, uint8_t sectionCount)
 */
bool Feature::isReadyToCompute(FeatureComputer *receiver, uint8_t sectionCount,
                               bool computeLast)
{
    int idx = getReceiverIndex(receiver);
    if (idx < 0) {
        return false;
    } else {
        return isReadyToCompute(uint8_t(idx), sectionCount, computeLast);
    }
}

/**
 * Acknowledge one or several sections of the buffer for a given receiver Idx.
 *
 * Acknowledging a section set its "acknowledged"  bolean to true. If all
 * receivers have acknowledged the section, the "published" boolean of the
 * section is then set to false to allow recording.
 */
void Feature::acknowledge(uint8_t receiverIdx, uint8_t sectionCount)
{
    uint8_t idx = m_computeIndex[receiverIdx];
    bool unpublish;
    uint8_t k;
    for (uint8_t i = idx; i < idx + sectionCount; ++i) {
        k = i % m_sectionCount;
        // Acknowledge for current receiver
        m_acknowledged[k][receiverIdx] = true;
        // Check other receiver acknowledgement to maybe unpublish the section
        unpublish = true;
        for (uint8_t j = 0; j < m_receiverCount; ++j) {
            unpublish &= m_acknowledged[k][j];
        }
        if (unpublish) {
            m_published[k] = false;
        }
    }
    m_computeIndex[receiverIdx] = (idx + sectionCount) % m_sectionCount;
}

/**
 * Acknowledge one or several sections of the buffer for a given receiver.
 *
 * See Feature::acknowledge(uint8_t receiverIdx, uint8_t sectionCount)
 */
void Feature::acknowledge(FeatureComputer *receiver, uint8_t sectionCount)
{
    int idx = getReceiverIndex(receiver);
    if (idx >= 0) {
        acknowledge(uint8_t(idx), sectionCount);
    }
}

/**
 * Flag the section currently being recorded as containing a data error.
 *
 * Note that this function should be called the feature source BEFORE sending
 * new values to the feature (Feature.addValue), because this function flag the
 * current recording section as wrong and sending values to the feature may
 * change the current recording section.
 */
void Feature::flagDataError()
{
    m_dataError[m_recordIndex] = true;
}

/**
 * Tell whether one of the next sections to compute for the receiver contains a
 * data error.
 */
bool Feature::sectionsToComputeHaveDataError(FeatureComputer *receiver,
                                             uint8_t sectionCount)
{
    int idx = getReceiverIndex(receiver);
    if (idx >= 0) {
        uint8_t sIdx = m_computeIndex[uint8_t(idx)];
        for (uint8_t i = sIdx; i < sIdx + sectionCount; i++) {
            if (m_dataError[i % m_sectionCount]) {
                //Serial.println("Data error !");
                return true;
            }
        }
    }
    return false;
}

/**
 * Tell whether one of the N latest recorded sections contains a data error.
 */
bool Feature::latestSectionsHaveDataError(uint8_t sectionCount)
{
    uint8_t sIdx = (m_sectionCount + m_recordIndex - sectionCount);
    for (uint8_t i = sIdx; i < sIdx + sectionCount; i++) {
        if (m_dataError[i % m_sectionCount]) {
            return true;
        }
    }
    return false;
}

/***** Communication *****/

/**
 * Send the data from latest recorded sections to given serial.
 *
 * Lock and print sectionCount sections, ending at last recorded section
 * [recordIndex - sectionCount; recordIndex - 1].
 */
void Feature::stream(IUSerial *ser, uint8_t sectionCount)
{
    uint8_t k = (m_sectionCount + m_recordIndex - sectionCount) % m_sectionCount;
    for (uint8_t i = k; i < k + sectionCount; ++i)
    {
        m_locked[i % m_sectionCount] = true;
    }
    m_specializedStream(ser, k, sectionCount);
    for (uint8_t i = k; i < k + sectionCount; ++i)
    {
        m_locked[i % m_sectionCount] = false;
    }
}

/**
 * Add the data from latest recorded sections to given buffer.
 *
 * Lock and print sectionCount sections, ending at last recorded section
 * [recordIndex - sectionCount; recordIndex - 1].
 * @param destination The destination buffer.
 * @param startIndex The index of the destination to start from buffer.
 * @param sectionCount The number of sections to write into destination.
 * @return the number of chars written.
 */
uint16_t Feature::sendToBuffer(char *destination, uint16_t startIndex,
                               uint8_t sectionCount)
{
    uint8_t k = (m_sectionCount + m_recordIndex - sectionCount) % m_sectionCount;
    for (uint8_t i = k; i < k + sectionCount; ++i)
    {
        m_locked[i % m_sectionCount] = true;
    }
    uint16_t charCount = m_specializedBufferStream(k, destination, startIndex,
                                                   sectionCount);
    for (uint8_t i = k; i < k + sectionCount; ++i)
    {
        m_locked[i % m_sectionCount] = false;
    }
    //Serial.print("charCount : ");Serial.println(charCount);
    return charCount;
}

/**
 * Same function as above, but uses q15_t as destination buffer. 
 * Used to transmit raw binary values rather than strings.
 */
uint16_t Feature::sendToBuffer(q15_t *destination, uint16_t startIndex,
                               uint8_t sectionCount)
{
    uint8_t k = (m_sectionCount + m_recordIndex - sectionCount) %
        m_sectionCount;
    for (uint8_t i = k; i < k + sectionCount; ++i)
    {
            m_locked[i % m_sectionCount] = true;
    }
    uint16_t charCount = m_specializedBufferStream(k, destination, startIndex,
                                                   sectionCount);
    for (uint8_t i = k; i < k + sectionCount; ++i)
    {
        m_locked[i % m_sectionCount] = false;
    }
    return charCount;
}

/***** Debugging *****/

/**
 * Print the feature name and configuration (active, computer & receivers)
 */
void Feature::exposeConfig()
{
    #ifdef IUDEBUG_ANY
    debugPrint(m_name, false);
    debugPrint(F(" config:"));
    debugPrint(F("  reguired: "), false);
    debugPrint(isRequired());
    debugPrint(F("  is computed: "), false);
    debugPrint(isComputedFeature());
    debugPrint(F("  has "), false);
    debugPrint(m_receiverCount, false);
    debugPrint(F(" receivers"));
    #endif
}

/**
 * Print the buffer counters and state booleans
 */
void Feature::exposeCounters()
{
    #ifdef IUDEBUG_ANY
    debugPrint(m_name, false);
    debugPrint(F(" counters:"));
    debugPrint(F("  filling idx: "), false);
    debugPrint(m_fillingIndex);
    debugPrint(F("  record idx: "), false);
    debugPrint(m_recordIndex);
    debugPrint(F("  Compute idx: "), false);
    for (uint8_t j = 0; j < m_receiverCount; ++j)
    {
        debugPrint(m_computeIndex[j], false);
        debugPrint(", ", false);
    }
    debugPrint(F("\n  "), false);
    debugPrint(m_sectionCount, false);
    debugPrint(F(" sections"));
    for (uint8_t i = 0; i < m_sectionCount; ++i)
    {
        debugPrint(F("  "), false);
        debugPrint(i, false);
        debugPrint(F(": published: "), false);
        debugPrint(m_published[i], false);
        debugPrint(F(", acknowledged: "), false);
        for (uint8_t j = 0; j < m_receiverCount; ++j)
        {
            debugPrint(m_acknowledged[i][j], false);
            debugPrint(", ", false);
        }
        debugPrint("");
    }
    #endif
}
