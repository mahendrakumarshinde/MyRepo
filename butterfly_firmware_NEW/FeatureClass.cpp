#include "FeatureClass.h"


/* =============================================================================
    Generic features
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
    // Instance registration
    if (debugMode)
    {
        Feature *existingFeature = getInstanceByName(name);
        if (existingFeature != NULL)
        {
            debugPrint(F("WARNING - Duplicate feature name "), false);
            debugPrint(name);
        }
        if (instanceCount >= MAX_INSTANCE_COUNT)
        {
            raiseException("Max feature count exceeded");
        }
    }
    m_instanceIdx = instanceCount;
    instances[m_instanceIdx] = this;
    instanceCount++;
}

Feature::~Feature()
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


/***** Feature designation *****/

Feature *Feature::getInstanceByName(const char *name)
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


/**
 * Applied the given config to the feature.
 */
bool Feature::configure(JsonVariant &config)
{
    JsonVariant my_config = config[m_name];
    if (!my_config)
    {
        return false;
    }
    JsonVariant value = my_config["OPS"];
    if (value)
    {
        if (value.as<int>())
        {
            enableOperationState();
        }
        else
        {
            disableOperationState();
        }
    }
    // Thresholds setting
    for (uint8_t i = 0; i < 3; ++i)
    {
        value = my_config["TRH"][i];
        if (value.success())
        {
            setThreshold(i, value);
        }
    }
    if (debugMode)
    {
        debugPrint(F("Configured feature "), false);
        debugPrint(m_name);
    }
    return true;
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
    if (m_receiverCount == 0)
    {
        for (uint8_t i = m_recordIndex; i < m_recordIndex + sectionCount; ++i)
        {
            if (m_locked[k])
            {
                return false;
            }
            m_published[k] = false;
        }
    }
    else
    {
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
    Format specific features
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
    return m_resolution * total / (float) m_sectionSize;
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
}


/* =============================================================================
    Instanciations
============================================================================= */

/***** Battery load *****/

float batteryLoadValues[2];
FloatFeature batteryLoad("BAT", 2, 1, batteryLoadValues);


/***** Accelerometer Features *****/

// Sensor data
__attribute__((section(".noinit2"))) q15_t accelerationXValues[1024];
__attribute__((section(".noinit2"))) q15_t accelerationYValues[1024];
__attribute__((section(".noinit2"))) q15_t accelerationZValues[1024];
Q15Feature accelerationX("A0X", 8, 128, accelerationXValues);
Q15Feature accelerationY("A0Y", 8, 128, accelerationYValues);
Q15Feature accelerationZ("A0Z", 8, 128, accelerationZValues);


// 128 sample long accel features
__attribute__((section(".noinit2"))) float accelRMS128XValues[8];
__attribute__((section(".noinit2"))) float accelRMS128YValues[8];
__attribute__((section(".noinit2"))) float accelRMS128ZValues[8];
__attribute__((section(".noinit2"))) float accelRMS128TotalValues[8];
FloatFeature accelRMS128X("A7X", 2, 4, accelRMS128XValues);
FloatFeature accelRMS128Y("A7Y", 2, 4, accelRMS128YValues);
FloatFeature accelRMS128Z("A7Z", 2, 4, accelRMS128ZValues);
FloatFeature accelRMS128Total("A73", 2, 4, accelRMS128TotalValues);


// 512 sample long accel features
__attribute__((section(".noinit2"))) float accelRMS512XValues[2];
__attribute__((section(".noinit2"))) float accelRMS512YValues[2];
__attribute__((section(".noinit2"))) float accelRMS512ZValues[2];
__attribute__((section(".noinit2"))) float accelRMS512TotalValues[2];
FloatFeature accelRMS512X("A9X", 2, 1, accelRMS512XValues);
FloatFeature accelRMS512Y("A9Y", 2, 1, accelRMS512YValues);
FloatFeature accelRMS512Z("A9Z", 2, 1, accelRMS512ZValues);
FloatFeature accelRMS512Total("A93", 2, 1, accelRMS512TotalValues);


// FFT feature from 512 sample long accel data
__attribute__((section(".noinit2"))) q15_t accelReducedFFTXValues[300];
__attribute__((section(".noinit2"))) q15_t accelReducedFFTYValues[300];
__attribute__((section(".noinit2"))) q15_t accelReducedFFTZValues[300];
Q15Feature accelReducedFFTX("FAX", 2, 150, accelReducedFFTXValues);
Q15Feature accelReducedFFTY("FAY", 2, 150, accelReducedFFTYValues);
Q15Feature accelReducedFFTZ("FAZ", 2, 150, accelReducedFFTZValues);

// Velocity features from 512 sample long accel data
__attribute__((section(".noinit2"))) float velRMS512XValues[2];
__attribute__((section(".noinit2"))) float velRMS512YValues[2];
__attribute__((section(".noinit2"))) float velRMS512ZValues[2];
FloatFeature velRMS512X("VAX", 2, 1, velRMS512XValues);
FloatFeature velRMS512Y("VAY", 2, 1, velRMS512YValues);
FloatFeature velRMS512Z("VAZ", 2, 1, velRMS512ZValues);

// Displacements features from 512 sample long accel data
__attribute__((section(".noinit2"))) float dispRMS512XValues[2];
__attribute__((section(".noinit2"))) float dispRMS512YValues[2];
__attribute__((section(".noinit2"))) float dispRMS512ZValues[2];
FloatFeature dispRMS512X("DAX", 2, 1, dispRMS512XValues);
FloatFeature dispRMS512Y("DAX", 2, 1, dispRMS512YValues);
FloatFeature dispRMS512Z("DAZ", 2, 1, dispRMS512ZValues);


/***** Gyroscope Features *****/

// Sensor data
__attribute__((section(".noinit2"))) q15_t tiltXValues[2];
__attribute__((section(".noinit2"))) q15_t tiltYValues[2];
__attribute__((section(".noinit2"))) q15_t tiltZValues[2];
Q15Feature tiltX("T0X", 2, 1, tiltXValues);
Q15Feature tiltY("T0Y", 2, 1, tiltYValues);
Q15Feature tiltZ("T0Z", 2, 1, tiltZValues);


/***** Magnetometer Features *****/

// Sensor data
__attribute__((section(".noinit2"))) q15_t magneticXValues[2];
__attribute__((section(".noinit2"))) q15_t magneticYValues[2];
__attribute__((section(".noinit2"))) q15_t magneticZValues[2];
Q15Feature magneticX("M0X", 2, 1, magneticXValues);
Q15Feature magneticY("M0Y", 2, 1, magneticYValues);
Q15Feature magneticZ("M0Z", 2, 1, magneticZValues);


/***** Barometer Features *****/

// Sensor data
__attribute__((section(".noinit2"))) float temperatureValues[2];
__attribute__((section(".noinit2"))) float pressureValues[2];
FloatFeature temperature("TMP", 2, 1, temperatureValues);
FloatFeature pressure("PRS", 2, 1, pressureValues);


/***** Audio Features *****/

// Sensor data
q15_t audioValues[8192];
Q15Feature audio("SND", 2, 2048, audioValues);

// 2048 sample long features
__attribute__((section(".noinit2"))) float audioDB2048Values[4];
FloatFeature audioDB2048("S11", 4, 1, audioDB2048Values);

// 4096 sample long features
__attribute__((section(".noinit2"))) float audioDB4096Values[2];
FloatFeature audioDB4096("S12", 2, 1, audioDB4096Values);


/***** GNSS Feature *****/


/***** RTD Temperature features *****/

#ifdef RTD_DAUGHTER_BOARD // Optionnal hardware
__attribute__((section(".noinit2"))) float rtdTempValues[8];
FloatFeature rtdTemp("RTD", 2, 4, rtdTempValues);
#endif // RTD_DAUGHTER_BOARD
