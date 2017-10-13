#include "FeatureClass.h"


/* =============================================================================
    Generic Feature buffers
============================================================================= */

Feature::Feature(const char* name, uint8_t sectionCount, uint16_t sectionSize,
                 Feature::slideOption sliding) :
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
    strcpy(m_sensorName, "---");
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
    m_computeIndex = 0;
    for (uint8_t i = 0; i < maxSectionCount; ++i)
    {
        m_locked[i] = false;
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
uint8_t Feature::addReceiver(uint8_t receiverId)
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

/**
 * Deactivate the feature computation
 *
 * Also deactivate the streaming and the OperationState computation.
 */
void Feature::deactivate()
{
    m_active = false;
    m_opStateEnabled = false;
}


/***** OperationState and Thresholds *****/

void Feature::setThresholds(float normalVal, float warningVal, float dangerVal)
{
    m_thresholds[0] = normalVal;
    m_thresholds[1] = warningVal;
    m_thresholds[2] = dangerVal;
}


/***** Tracking the buffer state *****/

/**
 * Check whether the section(s) of the buffer are ready for computation
 *
 * Ready for computation means not locked, published and not yet acknowledged
 * by this receiver.
 */
bool Feature::isReadyToCompute(uint8_t receiverIndex, uint8_t sectionCount)
{
    uint8_t k;
    for (uint8_t i = m_computeIndex; i < m_computeIndex + sectionCount; ++i)
    {
        k = i % maxSectionCount;
        if (m_locked[k] || m_partialAcknowledged[k][receiverIndex] ||
            !m_published[k])
        {
            return false;
        }
    }
    return true;
}

/**
 * Check if the section(s) of the buffer are ready for recording
 *
 * Ready for recording means acknowledged and not locked.
 */
bool Feature::isReadyToRecord(uint8_t sectionCount)
{
    uint8_t k;
    for (uint8_t i = m_recordIndex; i < m_recordIndex + sectionCount; ++i)
    {
        k = i % maxSectionCount;
        if(m_locked[k] || !m_acknowledged[k])
        {
            return false;
        }
    }
    return true;
}

/**
 * Acknowledge one or several sections of the buffer for a given receiver Idx
 */
void Feature::partialAcknowledge(uint8_t receiverIdx, uint8_t sectionCount)
{
    for (uint8_t i = m_computeIndex; i < m_computeIndex + sectionCount; ++i)
    {
        m_partialAcknowledged[i % maxSectionCount][receiverIdx] = true;
    }
}

/**
 * Acknowledge the next sections if all partial acknowledgement are OK
 *
 * Function will check sections one by one starting at computeIndex and, if all
 * partialAcknowledged are true, will acknowledge the section. It stops at the
 * first non-completely ackwoledged section.
 * This function should be called immediately after the receivers of the feature
 * have run.
 *
 * Acknowledging a section set its "acknowledged" and "published" boleans to
 * true and false respectively. computeIndex (index of the current computed
 * section) is then appropriately moved forward.
 */
void Feature::acknowledgeIfReady()
{
    uint8_t k;
    for (uint8_t i = m_computeIndex; i < m_computeIndex + maxSectionCount; ++i)
    {
        k = i % maxSectionCount;
        for (uint8_t j = 0; j < maxReceiverCount; ++j)
        {
            if (!m_partialAcknowledged[k][j])
            {
                // Not ready because some sections are not totally acknowledged
                break;
            }
        }
        // All partial acknowledgement OK, so move forward
        for (uint8_t j = 0; j < maxReceiverCount; ++j)
        {
            m_partialAcknowledged[k][j] = false;
        }
        m_acknowledged[k] = true;
        m_published[k] = false;
        m_computeIndex = (i + 1) % maxSectionCount;
    }

}

/**
 * Increment the index used to record data in the buffer
 *
 * Also publish the sections if it has been entirely recorded.
 * Publishing a section set its "published" and "acknowledged" booleans to true.
 * and false respectively. recordIndex (index of current recording section) is
 * then appropriately moved forward.
 */
void Feature::incrementFillingIndex()
{
    m_fillingIndex++;
    // Publish the current section if has been entirely recorded
    if (m_fillingIndex >= (m_recordIndex + 1) * m_sectionSize)
    {
        m_published[m_recordIndex] = true;
        m_acknowledged[m_recordIndex] = false;
        m_recordIndex = (m_recordIndex + 1) % maxSectionCount;
    }
    // Filling Index restart from 0 if needed
    m_fillingIndex %= m_totalSize;
}


/***** Communication *****/

/**
 * Send the data from latest recorded section to given Serial
 */
void Feature::stream(HardwareSerial *port)
{
    uint8_t k = (maxSectionCount + m_recordIndex - 1) % maxSectionCount;
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
                           Feature::slideOption sliding) :
    Feature(name, sectionCount, sectionCount, sliding)
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

/**
 * Compute the feature operation State
 */
OperationState::option FloatFeature::getOperationState()
{
    if (m_opStateEnabled)
    {
        float value = getNextFloatValues()[0];
        if (value > m_thresholds[2])
        {
            return OperationState::DANGER;
        }
        else if (value > m_thresholds[1])
        {
            return OperationState::WARNING;
        }
        else if (value > m_thresholds[0])
        {
            return OperationState::NORMAL;
        }
    }
    return OperationState::IDLE;
}

void FloatFeature::m_specializedStream(HardwareSerial *port, uint8_t sectionIdx)
{
    for (uint8_t i = sectionIdx * m_sectionSize;
         i < (sectionIdx + 1) * m_sectionSize; ++i)
    {
        port->print(",");
        port->print(m_values[i]);
    }
}


/***** Q15 Buffer *****/

Q15Feature::Q15Feature(const char* name, uint8_t sectionCount,
                       uint16_t sectionSize, q15_t *values,
                       Feature::slideOption sliding) :
    Feature(name, sectionCount, sectionCount, sliding)
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

/**
 * Compute the feature operation State
 */
OperationState::option Q15Feature::getOperationState()
{
    if (m_opStateEnabled)
    {
        float value = (float) getNextQ15Values()[0];
        if (value > m_thresholds[2])
        {
            return OperationState::DANGER;
        }
        else if (value > m_thresholds[1])
        {
            return OperationState::WARNING;
        }
        else if (value > m_thresholds[0])
        {
            return OperationState::NORMAL;
        }
    }
    return OperationState::IDLE;
}

void Q15Feature::m_specializedStream(HardwareSerial *port, uint8_t sectionIdx)
{
    for (uint8_t i = sectionIdx * m_sectionSize;
         i < (sectionIdx + 1) * m_sectionSize; ++i)
    {
        port->print(",");
        port->print(m_values[i]);
    }
}


/***** Q31 Buffer *****/

Q31Feature::Q31Feature(const char* name, uint8_t sectionCount,
                       uint16_t sectionSize, q31_t *values,
                       Feature::slideOption sliding) :
    Feature(name, sectionCount, sectionCount, sliding)
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

/**
 * Compute the feature operation State
 */
OperationState::option Q31Feature::getOperationState()
{
    if (m_opStateEnabled)
    {
        float value = (float) getNextQ31Values()[0];
        if (value > m_thresholds[2])
        {
            return OperationState::DANGER;
        }
        else if (value > m_thresholds[1])
        {
            return OperationState::WARNING;
        }
        else if (value > m_thresholds[0])
        {
            return OperationState::NORMAL;
        }
    }
    return OperationState::IDLE;
}

void Q31Feature::m_specializedStream(HardwareSerial *port, uint8_t sectionIdx)
{
    for (uint8_t i = sectionIdx * m_sectionSize;
         i < (sectionIdx + 1) * m_sectionSize; ++i)
    {
        port->print(",");
        port->print(m_values[i]);
    }
}
