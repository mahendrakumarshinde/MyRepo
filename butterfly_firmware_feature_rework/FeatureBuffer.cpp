#include "FeatureBuffer.h"


/* =============================================================================
    Generic Feature buffers
============================================================================= */

FeatureBuffer::FeatureBuffer(const char* name, uint8_t sectionCount,
                             uint16_t sectionSize,
                             FeatureBuffer::slideOption sliding) :
    m_samplingRate(0),
    m_resolution(0),
    m_active(false),
    m_opStateEnabled(false),
    m_streamingEnabled(false),
    m_computerId(0),
    m_receiverCount(0),
    m_sectionCount(sectionCount),
    m_sectionSize(sectionSize),
    m_sliding(sliding)
{
    strcpy(m_name, name);
    m_totalSize = m_sectionCount * m_sectionSize;
    reset();
}

/**
 * Reset the buffer to an "empty" state, as if just created
 */
void FeatureBuffer::reset()
{
    m_fillingIndex = 0;
    m_recordIndex = 0;
    m_computeIndex = 0;
    for (uint8_t i = 0; i < maxSectionCount; ++i)
    {
        m_published[i] = false;
        m_acknowledged[i] = true;
        for (uint8_t j = 0; j < maxReceiverCount; ++j)
        {
            m_partialAcknowledged[i][j] = false;
        }
    }
}

/**
 * Return the receiver index and increment the buffer receiver count by 1
 */
uint8_t FeatureBuffer::addReceiver(uint8_t receiverId)
{
    if (m_receiverCount >= maxReceiverCount)
    {
        raiseException("Too many receivers");
    }
    m_receiversId[m_receiverCount] = receiverId;
    m_receiverCount++;
    return m_receiverCount - 1;
}


/***** Configuration *****/

void FeatureBuffer::setState(OperationState::option opState)
{
    m_operationState = opState;
    m_highestOpState = max(m_highestOpState, m_operationState);
}


/***** OperationState and Thresholds *****/

/**
 * Deactivate the OperationState computation for this feature
 *
 * Also reset the OperationState to IDLE (lowest state)
 */
void FeatureBuffer::disableOperationState()
{
    m_opStateEnabled = false;
    m_operationState = OperationState::IDLE;
}


void FeatureBuffer::setThresholds(float normalVal, float warningVal,
                                  float dangerVal)
{
    m_thresholds[0] = normalVal;
    m_thresholds[1] = warningVal;
    m_thresholds[2] = dangerVal;
}


/***** Tracking the buffer state *****/

/**
 * Check whether the section(s) of the buffer are ready for computation
 *
 * Ready for computation means published and not yet acknowledged by this
 * receiver.
 */
bool FeatureBuffer::isReadyToCompute(uint8_t receiverIndex,
                                     uint8_t sectionCount)
{
    bool ready = true;
    uint8_t k;
    for (uint8_t i = m_computeIndex; i < m_computeIndex + sectionCount; ++i)
    {
        k = i % maxSectionCount;
        ready &= (m_published[k] && !m_partialAcknowledged[k][receiverIndex]);
    }
    return ready;
}

/**
 * Check if the section(s) of the buffer are ready for recording
 *
 * Ready for recording means acknowledged.
 */
bool FeatureBuffer::isReadyToRecord(uint8_t sectionCount)
{
    bool ready = true;
    for (uint8_t i = m_recordIndex; i < m_recordIndex + sectionCount; ++i)
    {
        ready &= m_acknowledged[i % maxSectionCount];
    }
    return ready;
}

/**
 * Acknowledge one or several sections of the buffer for a given receiver Idx
 */
void FeatureBuffer::partialAcknowledge(uint8_t receiverIdx,
                                       uint8_t sectionCount)
{
    for (uint8_t i = m_computeIndex; i < m_computeIndex + sectionCount; ++i)
    {
        m_partialAcknowledged[i % maxSectionCount][receiverIdx] = true;
    }
}

/**
 * Publish next buffer part (one or several sections)  if recording is done
 *
 * Next buffer part to publish is determined based on fillingIndex and
 * recordIndex.
 * Publishing a section set its "published" and "acknowledged" booleans to true
 * and false respectively.
 * The index of current recording section is then appropriately moved forward.
 */
void FeatureBuffer::publishIfReady(uint8_t sectionCount)
{
    uint16_t nextIndex = (m_recordIndex + sectionCount) * m_sectionSize;
    nextIndex = nextIndex % m_totalSize;
    if (m_fillingIndex >= nextIndex)
    {
        uint8_t k = 0;
        for (uint8_t i = m_recordIndex; i < m_recordIndex + sectionCount; ++i)
        {
            k = i % maxSectionCount;
            m_published[k] = true;
            m_acknowledged[k] = false;
        }
        m_recordIndex = (m_recordIndex + sectionCount) % maxSectionCount;
    }
}

/**
 * Acknowledge the next part (one or several sections) of the buffer
 *
 * Acknowledging a section set its "acknowledged" and "published" boleans to
 * true and false respectively.
 * The index of the current computed section is appropriately moved forward.
 */
void FeatureBuffer::acknowledgeIfReady(uint8_t sectionCount)
{
    uint8_t k = 0;
    for (uint8_t i = m_computeIndex; i < m_computeIndex + sectionCount; ++i)
    {
        k = i % maxSectionCount;
        for (uint8_t j = 0; j < maxReceiverCount; ++j)
        {
            if (!m_partialAcknowledged[k][j])
            {
                // Not ready because some sections are not totally acknowledged
                return;
            }
        }
    }
    // If all partial acknowledgement are OK, move forward
    for (uint8_t i = m_computeIndex; i < m_computeIndex + sectionCount; ++i)
    {
        k = i % maxSectionCount;
        m_acknowledged[k] = true;
        m_published[k] = false;
    }
    m_computeIndex += sectionCount;
    if (sectionCount >= maxSectionCount)
    {
        m_computeIndex -= maxSectionCount;
    }
}

/**
 * Increment the index used to record data in the buffer
 */
void FeatureBuffer::incrementFillingIndex()
{
    m_fillingIndex++;
    if (m_fillingIndex == m_totalSize)
    {
        m_fillingIndex = 0;
    }
}


/***** Debugging *****/

/**
 * Print the feature name and configuration (active, computer & receivers)
 */
void FeatureBuffer::exposeConfig()
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
void FeatureBuffer::exposeCounters()
{
    #ifdef DEBUGMODE
    debugPrint(m_name, false);
    debugPrint(F(" counters:"));
    debugPrint(F("  filling idx: "), false);
    debugPrint(m_fillingIndex);
    debugPrint(F("  record idx: "), false);
    debugPrint(m_recordIndex);
    debugPrint(F("  Compute idx: "), false);
    debugPrint(m_computeIndex);

    debugPrint(F("  "), false);
    debugPrint(m_sectionCount, false);
    debugPrint(F(" sections:"));
    for (uint8_t i = 0; i < m_receiverCount; ++i)
    {
        debugPrint(F("  "), false);
        debugPrint(i, false);
        debugPrint(F(": published "), false);
        debugPrint(m_published[i], false);
        debugPrint(F(", acknowledged "), false);
        debugPrint(m_acknowledged[i]);
        //TODO also print m_partialAcknowledged[i][maxReceiverCount]?
    }
    #endif
}


/* =============================================================================
    Format specific buffers
============================================================================= */

/***** Float Buffer *****/

FloatFeature::FloatFeature(const char* name, uint8_t sectionCount,
                           uint16_t sectionSize, float *values,
                           FeatureBuffer::slideOption sliding) :
    FeatureBuffer(name, sectionCount, sectionCount, sliding)
{
    m_values = values;
}

/**
 * Return a pointer to the start of the section
 */
float* FloatFeature::getNextFloatValues()
{
    uint16_t startIndex = (uint16_t) m_computeIndex * m_sectionSize;
    return &m_values[startIndex];
}

/**
 * Add a value to the buffer and increment the filling index
 */
void FloatFeature::addFloatValue(float value)
{
    m_values[m_fillingIndex] = value;
    incrementFillingIndex();
}


/***** Q15 Buffer *****/

Q15Feature::Q15Feature(const char* name, uint8_t sectionCount,
                       uint16_t sectionSize, q15_t *values,
                       FeatureBuffer::slideOption sliding) :
    FeatureBuffer(name, sectionCount, sectionCount, sliding)
{
    m_values = values;
}

/**
 * Return a pointer to the start of the section
 */
q15_t* Q15Feature::getNextQ15Values()
{
    uint16_t startIndex = (uint16_t) m_computeIndex * m_sectionSize;
    return &m_values[startIndex];
}

/**
 * Add a value to the buffer and increment the filling index
 */
void Q15Feature::addQ15Value(q15_t value)
{
    m_values[m_fillingIndex] = value;
    incrementFillingIndex();
}


/***** Q31 Buffer *****/

Q31Feature::Q31Feature(const char* name, uint8_t sectionCount,
                       uint16_t sectionSize, q31_t *values,
                       FeatureBuffer::slideOption sliding) :
    FeatureBuffer(name, sectionCount, sectionCount, sliding)
{
    m_values = values;
}

/**
 * Return a pointer to the start of the section
 */
q31_t* Q31Feature::getNextQ31Values()
{
    uint16_t startIndex = (uint16_t) m_computeIndex * m_sectionSize;
    return &m_values[startIndex];
}

/**
 * Add a value to the buffer and increment the filling index
 */
void Q31Feature::addQ31Value(q31_t value)
{
    m_values[m_fillingIndex] = value;
    incrementFillingIndex();
}
