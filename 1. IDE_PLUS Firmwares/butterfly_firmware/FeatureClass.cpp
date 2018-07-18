#include "FeatureClass.h"


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
                 Feature::slideOption sliding, bool isFFT) :
    m_isFFT(isFFT),
    m_sectionCount(sectionCount),
    m_sectionSize(sectionSize),
    m_sliding(sliding)
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
    // Publish the current section if has been entirely recorded
    if (m_fillingIndex >= (m_recordIndex + 1) * m_sectionSize) {
        m_published[m_recordIndex] = true;
        for (uint8_t j = 0; j < m_receiverCount; ++j) {
            m_acknowledged[m_recordIndex][j] = false;
        }
        m_recordIndex = (m_recordIndex + 1) % m_sectionCount;
        newFullSection = true;
    }
    // Filling Index restart from 0 if needed
    m_fillingIndex %= m_totalSize;
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
 */
bool Feature::isReadyToRecord(uint8_t sectionCount)
{
    uint8_t k;
    if (m_receiverCount == 0) {
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
    for (uint8_t i = idx; i < idx + sectionCount; ++i)
    {
        k = i % m_sectionCount;
        // Acknowledge for current receiver
        m_acknowledged[k][receiverIdx] = true;
        // Check other receiver acknowledgement to maybe unpublish the section
        unpublish = true;
        for (uint8_t j = 0; j < m_receiverCount; ++j)
        {
            unpublish &= m_acknowledged[k][j];
        }
        if (unpublish)
        {
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
 * @param sectionCount The number of sectiong to write into destination.
 * @return the number of chars written.
 */
uint16_t Feature::sendToBuffer(char *destination, uint16_t startIndex,
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
