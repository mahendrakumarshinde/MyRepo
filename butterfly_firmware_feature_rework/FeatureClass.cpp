#include "FeatureClass.h"


/* =============================================================================
    Generic Feature buffers
============================================================================= */

Feature::Feature(const char* name, uint8_t sectionCount, uint16_t sectionSize,
                 Feature::slideOption sliding) :
    m_samplingRate(0),
    m_resolution(1),
    m_active(false),
    m_opStateEnabled(false),
    m_streamingEnabled(false),
    m_operationState(OperationState::IDLE),
    m_computerId(0),
    m_receiverCount(0),
    m_sectionCount(sectionCount),
    m_sectionSize(sectionSize),
    m_sliding(sliding)
{
    strcpy(m_name, name);
    strcpy(m_sensorName, "---");
    for (uint8_t i = 0; i < OperationState::COUNT - 1; ++i)
    {
        m_thresholds[i] = 0;
    }
    m_totalSize = m_sectionCount * m_sectionSize;
    reset();
}

/**
 * Reset the buffer to an "empty" state, as if just created
 */
void Feature::reset()
{
    m_fillingIndex = 0;
    m_recordIndex = 0;
    for (uint8_t i = 0; i < maxReceiverCount; ++i)
    {
        m_computeIndex[i] = 0;
    }
    // All sections are set as ready to record: not locked, not published and
    // acknowledged by all receivers.
    for (uint8_t j = 0; j < maxSectionCount; ++j)
    {
        m_locked[j] = false;
        m_published[j] = false;
        for (uint8_t k = 0; k < maxReceiverCount; ++k)
        {
            m_acknowledged[j][k] = true;
        }
    }
}

/**
 * Return the receiver index and increment the buffer receiver count by 1
 */
uint8_t Feature::addReceiver(uint8_t receiverId)
{
    m_receiversId[m_receiverCount] = receiverId;
    m_receiverCount++;
    return m_receiverCount - 1;
}


/***** Configuration *****/

/**
 * Deactivate the feature computation
 *
 * Also deactivate the streaming and the OperationState computation.
 */
void Feature::deactivate()
{
    m_active = false;
    m_opStateEnabled = false;
    m_streamingEnabled = false;
}


/***** OperationState and Thresholds *****/

/**
 * Determine the state using getValueToCompareToThresholds and the thresholds
 */
void Feature::updateOperationState()
{
    if (m_opStateEnabled)
    {
        float value = getValueToCompareToThresholds();
        if (value > m_thresholds[2])
        {
            m_operationState = OperationState::DANGER;
            return;
        }
        else if (value > m_thresholds[1])
        {
            m_operationState = OperationState::WARNING;
            return;
        }
        else if (value > m_thresholds[0])
        {
            m_operationState = OperationState::NORMAL;
            return;
        }
    }
    m_operationState = OperationState::IDLE;
}


void Feature::setThresholds(float normalVal, float warningVal, float dangerVal)
{
    m_thresholds[0] = normalVal;
    m_thresholds[1] = warningVal;
    m_thresholds[2] = dangerVal;
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
    for (uint8_t i = m_recordIndex; i < m_recordIndex + sectionCount; ++i)
    {
        k = i % m_sectionCount;
        if (m_locked[k] || m_published[k])
        {
            return false;
        }
        for (uint8_t j = 0; j < m_receiverCount; ++j)
        {
            if (!m_acknowledged[k][j])
            {
                return false;
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
bool Feature::isReadyToCompute(uint8_t receiverIdx, uint8_t sectionCount)
{
    uint8_t k;
    uint8_t idx = m_computeIndex[receiverIdx];
    for (uint8_t i = idx; i < idx + sectionCount; ++i)
    {
        k = i % m_sectionCount;
        if (m_locked[k] || !m_published[k] || m_acknowledged[k][receiverIdx])
        {
            return false;
        }
    }
    return true;
}

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
    // Publish the current section if has been entirely recorded
    if (m_fillingIndex >= (m_recordIndex + 1) * m_sectionSize)
    {
        m_published[m_recordIndex] = true;
        for (uint8_t j = 0; j < m_receiverCount; ++j)
        {
            m_acknowledged[m_recordIndex][j] = false;
        }
        if (loopDebugMode && highVerbosity)
        {
            debugPrint(getName(), false);
            debugPrint(F(" publish section="), false);
            debugPrint(m_recordIndex, false);
            debugPrint(F(", idx="), false);
            debugPrint(m_fillingIndex);
        }
        m_recordIndex = (m_recordIndex + 1) % m_sectionCount;
        // Update the operation state to reflect the latest published section
        updateOperationState();
    }
    // Filling Index restart from 0 if needed
    m_fillingIndex %= m_totalSize;
}

/**
 * Acknowledge one or several sections of the buffer for a given receiver Idx
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
            if (loopDebugMode && highVerbosity)
            {
                debugPrint(getName(), false);
                debugPrint(F(" fully acknowledge section="), false);
                debugPrint(k);
            }
        }
    }
    m_computeIndex[receiverIdx] = (idx + sectionCount) % m_sectionCount;
}


/***** Communication *****/

/**
 * Send the data from latest recorded section to given Serial
 */
void Feature::stream(HardwareSerial *port)
{
    // Lock and print the last recorded section (= recordIndex - 1)
    uint8_t k = (m_sectionCount + m_recordIndex - 1) % m_sectionCount;
    m_locked[k] = true;
    m_specializedStream(port, k);
    m_locked[k] = false;
}

/***** Debugging *****/

/**
 * Print the feature name and configuration (active, computer & receivers)
 */
void Feature::exposeConfig()
{
    #ifdef DEBUGMODE
    debugPrint(m_name, false);
    debugPrint(F(" config:"));
    debugPrint(F("  active: "), false);
    debugPrint(m_active);
    debugPrint(F("  streaming: "), false);
    debugPrint(m_streamingEnabled);
    debugPrint(F("  Computer id: "), false);
    debugPrint(m_computerId);

    debugPrint(F("  "), false);
    debugPrint(m_receiverCount, false);
    debugPrint(F(" receivers: "), false);
    for (uint8_t i = 0; i < m_receiverCount; ++i)
    {
        debugPrint(m_receiversId[i], false);
        debugPrint(F(", "), false);
    }
    debugPrint("");
    #endif
}

/**
 * Print the buffer counters and state booleans
 */
void Feature::exposeCounters()
{
    #ifdef DEBUGMODE
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


/* =============================================================================
    Format specific buffers
============================================================================= */

/***** Float Buffer *****/

FloatFeature::FloatFeature(const char* name, uint8_t sectionCount,
                           uint16_t sectionSize, float *values,
                           Feature::slideOption sliding) :
    Feature(name, sectionCount, sectionSize, sliding)
{
    m_values = values;
}

/**
 * Return a pointer to the start of the section
 */
float* FloatFeature::getNextFloatValues(uint8_t receiverIdx)
{
    uint16_t startIdx = (uint16_t) m_computeIndex[receiverIdx] * m_sectionSize;
    return &m_values[startIdx];
}

/**
 * Add a value to the buffer and increment the filling index
 */
void FloatFeature::addFloatValue(float value)
{
    m_values[m_fillingIndex] = value;
    incrementFillingIndex();
    if (loopDebugMode && showIntermediaryResults && isComputedFeature())
    {
        debugPrint(getName(), false);
        debugPrint(": ", false);
        debugPrint(value);
    }
}

/**
 * By default, return the mean value of the latest recorded section
 */
float FloatFeature::getValueToCompareToThresholds()
{
    uint8_t k = (m_sectionCount + m_recordIndex - 1) % m_sectionCount;
    float total = 0;
    for (uint16_t i = k * m_sectionSize; i < (k + 1) * m_sectionSize; ++i)
    {
        total += m_values[i];
    }
    return total / (float) m_sectionSize;
}

/**
 * Stream the content of the section at sectionIdx
 */
void FloatFeature::m_specializedStream(HardwareSerial *port, uint8_t sectionIdx)
{
    for (uint16_t i = sectionIdx * m_sectionSize;
         i < (sectionIdx + 1) * m_sectionSize; ++i)
    {
        port->print(",");
        port->print(m_values[i] * m_resolution);
    }
}


/***** Q15 Buffer *****/

Q15Feature::Q15Feature(const char* name, uint8_t sectionCount,
                       uint16_t sectionSize, q15_t *values,
                       Feature::slideOption sliding) :
    Feature(name, sectionCount, sectionSize, sliding)
{
    m_values = values;
}

/**
 * Return a pointer to the start of the section
 */
q15_t* Q15Feature::getNextQ15Values(uint8_t receiverIdx)
{
    uint16_t startIdx = (uint16_t) m_computeIndex[receiverIdx] * m_sectionSize;
    return &m_values[startIdx];
}

/**
 * Add a value to the buffer and increment the filling index
 */
void Q15Feature::addQ15Value(q15_t value)
{
    m_values[m_fillingIndex] = value;
    incrementFillingIndex();
    if (loopDebugMode && showIntermediaryResults && isComputedFeature())
    {
        debugPrint(getName(), false);
        debugPrint(": ", false);
        debugPrint(value);
    }
}


/***** Q31 Buffer *****/

Q31Feature::Q31Feature(const char* name, uint8_t sectionCount,
                       uint16_t sectionSize, q31_t *values,
                       Feature::slideOption sliding) :
    Feature(name, sectionCount, sectionSize, sliding)
{
    m_values = values;
}

/**
 * Return a pointer to the start of the section
 */
q31_t* Q31Feature::getNextQ31Values(uint8_t receiverIdx)
{
    uint16_t startIdx = (uint16_t) m_computeIndex[receiverIdx] * m_sectionSize;
    return &m_values[startIdx];
}

/**
 * Add a value to the buffer and increment the filling index
 */
void Q31Feature::addQ31Value(q31_t value)
{
    m_values[m_fillingIndex] = value;
    incrementFillingIndex();
    if (loopDebugMode && showIntermediaryResults && isComputedFeature())
    {
        debugPrint(getName(), false);
        debugPrint(": ", false);
        debugPrint(value);
    }
}
