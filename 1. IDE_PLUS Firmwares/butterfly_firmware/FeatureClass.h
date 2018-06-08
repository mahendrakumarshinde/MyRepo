#ifndef FEATURECLASS_H
#define FEATURECLASS_H

#include <Arduino.h>
#include <ArduinoJson.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include <IUDebugger.h>
#include <IUSerial.h>


/* =============================================================================
    Forward declarations
============================================================================= */

class FeatureComputer;


/* =============================================================================
    Generic Features
============================================================================= */

/**
 * A Feature has a buffer of values, and the methods to interact with it.
 *
 * Features handle per section recording on their buffer, publication and
 * acknowledgement.
 * Buffers are split into multiple sections (2 by default). Note that the total
 * length of the buffer should be divisible by maxSectionCount. Each section
 * represents a consistent set of values that are recorded together.
 * Note that if a computation uses consecutive sections, there is no in-built
 * garanty that these sections have been recorded consecutively, without time
 * gap.
 * Buffers have a mechanism to publish a section (signaling that it has been
 * entirely filled with records and is ready to be computed) and acknowledge it
 * (all the necessary computations have been done and the section is now
 * available to record new data). Acknowledgement is done on a per-receiver
 * basis.
 *
 *The parameter "isFFT" gives meaning to the feature content. If true, the
 * feature contains FFT coeffs, organised as follow:
 *  - 3 * i: Index of the Fourrier coefficient
 *  - 3 * i + 1: Real part the Fourrier coefficient
 *  - 3 * i + 2: Imaginary part the Fourrier coefficient
 * That means that if the FFT feature must hold 50 Fourrier coefficients, its
 * sectionSize should be 150.
 * If the feature contains FFT coef, the resolution will naturally only apply
 * to the real and imaginary parts of the coefficients, not to the indexes.
 */
class Feature
{
    public:
        static const uint8_t nameLength = 3;
        /* TODO - For now, only slideOption::FIXED is implemented => need to
        implement slideOption::ROLLING */
        enum slideOption : uint8_t {FIXED,
                                    ROLLING};
        static const uint8_t maxSectionCount = 8;
        static const uint8_t maxReceiverCount = 5;
        /***** Feature instance registry *****/
        static const uint8_t MAX_INSTANCE_COUNT = 60;
        static uint8_t instanceCount;
        static Feature *instances[MAX_INSTANCE_COUNT];

        /***** Callback signatures *****/
        // NB: this callbacks should be kept short as they be called outside of
        // the main loop
        typedef void (*onNewValueSignature)(Feature *feature);
        typedef void (*onNewRecordedSectionSignature)(Feature *feature);

        /***** Core *****/
        Feature(const char* name, uint8_t sectionCount,
                uint16_t sectionSize, slideOption sliding=FIXED,
                bool isFFT=false);
        virtual ~Feature();
        virtual char* getName() { return m_name; }
        virtual bool isNamed(const char* name) {
            return strcmp(m_name, name) == 0;
        }
        static Feature *getInstanceByName(const char *name);
        /***** Configuration *****/
        virtual bool isRequired() { return (m_alwaysRequired || m_required); }
        virtual void setRequired(bool required) { m_required = required; }
        virtual void setAlwaysRequired(bool required)
            { m_alwaysRequired = required; }
        virtual void configure(JsonVariant &config) { }
        virtual void setOnNewValueCallback(onNewValueSignature onNewValue)
            { m_onNewValue = onNewValue; }
        virtual void setOnNewRecordedSectionCallback(
                onNewRecordedSectionSignature onNewRecordedSection)
            { m_onNewRecordedSection = onNewRecordedSection; }
        /***** Physical metadata *****/
        virtual void setSamplingRate(uint16_t rate) { m_samplingRate = rate; }
        virtual uint16_t getSamplingRate() { return m_samplingRate; }
        virtual void setResolution(float res) { m_resolution = res; }
        virtual float getResolution() { return m_resolution; }
        /***** Receivers tracking *****/
        virtual void addReceiver(FeatureComputer *receiver);
        virtual bool removeReceiver(FeatureComputer *receiver);
        virtual uint8_t getReceiverCount() { return m_receiverCount; }
        virtual FeatureComputer* getReceiver(uint8_t idx)
            { return m_receivers[idx]; }
        virtual int getReceiverIndex(FeatureComputer *receiver);
        /***** Computers tracking *****/
        virtual bool isComputedFeature() { return m_computer != NULL; }
        virtual void setComputer(FeatureComputer *cptr) { m_computer = cptr; }
        virtual void removeComputer() { m_computer = NULL; }
        virtual FeatureComputer* getComputer() { return m_computer; }
        /***** Buffer core *****/
        // Reset all counters
        virtual void reset();
        // Filling the buffer
        virtual void addValue(q15_t value) {}
        virtual void addValue(float value) {}
        virtual void addValue(q31_t value) {}
        virtual void incrementFillingIndex();
        // Accessors to values
        virtual uint16_t getSectionSize() { return m_sectionSize; }
        virtual void* getNextValuesToCompute(FeatureComputer *receiver)
            { return NULL; }
        virtual float* getNextFloatValuesToCompute(FeatureComputer *receiver)
            { return NULL; }
        virtual q15_t* getNextQ15ValuesToCompute(FeatureComputer *receiver)
            { return NULL; }
        virtual q31_t* getNextQ31ValuesToCompute(FeatureComputer *receiver)
            { return NULL; }
        virtual void* getLastRecordedValues(uint8_t sectionCount=1)
            { return NULL; }
        virtual float* getLastRecordedFloatValues(uint8_t sectionCount=1)
            { return NULL; }
        virtual q15_t* getLastRecordedQ15Values(uint8_t sectionCount=1)
            { return NULL; }
        virtual q31_t* getLastRecordedQ31Values(uint8_t sectionCount=1)
            { return NULL; }
        // Tracking the buffer state
        virtual bool isReadyToRecord(uint8_t sectionCount=1);
        virtual bool isReadyToCompute(uint8_t receiverIdx,
                                      uint8_t sectionCount);
        virtual bool isReadyToCompute(FeatureComputer *receiver,
                                      uint8_t sectionCount=1);
        virtual void acknowledge(uint8_t receiverIdx,
                                 uint8_t sectionCount=1);
        virtual void acknowledge(FeatureComputer *receiver,
                                 uint8_t sectionCount=1);
        /***** Communication *****/
        virtual void stream(IUSerial *ser, uint8_t sectionCount=1);
        virtual uint16_t sendToBuffer(char *destination, uint16_t startIndex,
                                      uint8_t sectionCount=1);
        /***** Debugging *****/
        virtual void exposeConfig();
        virtual void exposeCounters();


    protected:
        /***** Instance registry *****/
        uint8_t m_instanceIdx;
        /***** Feature designation *****/
        char m_name[nameLength + 1];
        /***** Configuration variables *****/
        bool m_required = false;
        bool m_alwaysRequired = false;
        onNewValueSignature m_onNewValue = NULL;
        onNewRecordedSectionSignature m_onNewRecordedSection = NULL;
        /***** Physical metadata *****/
        uint16_t m_samplingRate = 0;
        float m_resolution = 1;
        bool m_isFFT;
        /***** Receivers and computers tracking *****/
        uint8_t m_receiverCount = 0;
        FeatureComputer *m_receivers[maxReceiverCount];
        FeatureComputer *m_computer = NULL;
        /***** Buffer core *****/
        uint8_t m_sectionCount;
        uint16_t m_sectionSize;
        slideOption m_sliding;
        uint16_t m_totalSize;
        // Tracking the buffer state
        uint16_t m_fillingIndex = 0;
        uint8_t m_recordIndex = 0;
        uint8_t m_computeIndex[maxReceiverCount];
        bool m_published[maxSectionCount];
        bool m_acknowledged[maxSectionCount][maxReceiverCount];
        // Lock a section to prevent both recording and computation, useful when
        // streaming the section content for example, to garantee data
        // consistency at section level
        bool m_locked[maxSectionCount];
        virtual void m_specializedStream(IUSerial *ser,
                                         uint8_t sectionIdx,
                                         uint8_t sectionCount=1) {}
        virtual uint16_t m_specializedBufferStream(uint8_t sectionIdx,
            char *destination, uint16_t startIndex, uint8_t sectionCount=1) {}
};


/* =============================================================================
    Format specific features
============================================================================= */

/**
 * A template class for Feature with a specific number format.
 *
 * Values should be an array of size = sectionCount * sectionSize
 */
template <typename T>
class FeatureTemplate : public Feature
{
    public:
        FeatureTemplate(const char* name, uint8_t sectionCount,
                        uint16_t sectionSize, T *values,
                        Feature::slideOption sliding=Feature::FIXED,
                        bool isFFT=false) :
            Feature(name, sectionCount, sectionSize, sliding, isFFT),
            m_values(values) { }
        virtual ~FeatureTemplate() {}

        /**
         * Add a value to the buffer and increment the filling index
         */
        virtual void addValue(T value) {
            m_values[m_fillingIndex] = value;
            incrementFillingIndex();
        }

        /**
         * Return a pointer to the current section start for the receiver.
         */
        virtual void* getNextValuesToCompute(FeatureComputer *receiver) {
            int idx = getReceiverIndex(receiver);
            if (idx < 0) {
                return NULL;
            } else {
                uint16_t startIdx = (uint16_t) m_computeIndex[uint8_t(idx)] *
                                    m_sectionSize;
                return &m_values[startIdx];
            }
        }
        virtual float* getNextFloatValuesToCompute(FeatureComputer *receiver)
            { return NULL; }
        virtual q15_t* getNextQ15ValuesToCompute(FeatureComputer *receiver)
            { return NULL; }
        virtual q31_t* getNextQ31ValuesToCompute(FeatureComputer *receiver)
            { return NULL; }


        /**
         * Return a pointer to the start of the k last recorded section.
         */
        virtual void* getLastRecordedValues(uint8_t sectionCount=1) {
            uint8_t sIdx = (m_sectionCount + m_recordIndex - sectionCount) %
                            m_sectionCount;
            return &m_values[sIdx * m_sectionSize];
        }
        virtual float* getLastRecordedFloatValues(uint8_t sectionCount=1)
            { return NULL; }
        virtual q15_t* getLastRecordedQ15Values(uint8_t sectionCount=1)
            { return NULL; }
        virtual q31_t* getLastRecordedQ31Values(uint8_t sectionCount=1)
            { return NULL; }

    protected:
        T *m_values;

        virtual void m_specializedStream(IUSerial *ser,
                                         uint8_t sectionIdx,
                                         uint8_t sectionCount=1) {
            uint8_t sIdx = 0;
            for (uint8_t k = sectionIdx; k < sectionIdx + sectionCount; k++) {
                sIdx = k % m_sectionCount;
                if (m_isFFT) {
                    for (uint16_t i = sIdx * m_sectionSize / 3;
                         i < (sIdx + 1) * m_sectionSize / 3; ++i) {
                        ser->write(',');
                        ser->write(String((uint16_t) m_values[3 * i], DEC).c_str());
                        ser->write(',');
                        ser->write(String(((float) m_values[3 * i + 1]) *
                                    m_resolution, 2).c_str());
                        ser->write(',');
                        ser->write(String(((float) m_values[3 * i + 2]) *
                                    m_resolution, 2).c_str());
                    }
                } else {
                    for (uint16_t i = sIdx * m_sectionSize;
                         i < (sIdx + 1) * m_sectionSize; ++i) {
                        ser->write(',');
                        ser->write(String(((float) m_values[i]) * m_resolution, 2).c_str());
                    }
                }
            }
        }

        virtual uint16_t m_specializedBufferStream(uint8_t sectionIdx,
                char *destination, uint16_t startIndex, uint8_t sectionCount=1)
        {
            uint8_t sIdx = 0;
            String strVal = "";
            uint8_t floatLen = 5;
            uint16_t destIndex = startIndex;
            for (uint8_t k = sectionIdx; k < sectionIdx + sectionCount; k++) {
                sIdx = k % m_sectionCount;
                if (m_isFFT) {
                    for (uint16_t i = sIdx * m_sectionSize / 3;
                         i < (sIdx + 1) * m_sectionSize / 3; ++i) {
                        destination[destIndex++] = ',';
                        strcat(destination,
                               String((int) m_values[3 * i]).c_str());
                        if (m_values[3 * i] < 10) { destIndex++; }
                        else if (m_values[3 * i] < 100) { destIndex += 2; }
                        else { destIndex += 3; }
                        destination[destIndex++] = ',';
                        strVal = String(((float) m_values[3 * i + 1]) *
                                        m_resolution, 2);
                        strncat(destination, strVal.c_str(), floatLen);
                        destIndex += min((uint16_t) strVal.length(), floatLen);
                        destination[destIndex++] = ',';
                        strVal = String(((float) m_values[3 * i + 2]) *
                                        m_resolution, 2);
                        strcat(destination, strVal.c_str());
                        destIndex += strVal.length();
                    }
                } else {
                    for (uint16_t i = sIdx * m_sectionSize;
                         i < (sIdx + 1) * m_sectionSize; ++i) {
                        destination[destIndex++] = ',';
                        strVal = String(((float) m_values[i]) *
                                        m_resolution, 2);
                        strncat(destination, strVal.c_str(), floatLen);
                        destIndex += min((uint16_t) strVal.length(), floatLen);
                    }
                }
            }
            return destIndex - startIndex;
        }
};

template <>
inline float* FeatureTemplate<float>::getNextFloatValuesToCompute(FeatureComputer *receiver)
{
    return (float*) getNextValuesToCompute(receiver);
}

template <>
inline float* FeatureTemplate<float>::getLastRecordedFloatValues(uint8_t sectionCount)
{
    return (float*) getLastRecordedValues(sectionCount);
}

template <>
inline q15_t* FeatureTemplate<q15_t>::getNextQ15ValuesToCompute(FeatureComputer *receiver)
{
    return (q15_t*) getNextValuesToCompute(receiver);
}

template <>
inline q15_t* FeatureTemplate<q15_t>::getLastRecordedQ15Values(uint8_t sectionCount)
{
    return (q15_t*) getLastRecordedValues(sectionCount);
}

template <>
inline q31_t* FeatureTemplate<q31_t>::getNextQ31ValuesToCompute(FeatureComputer *receiver)
{
    return (q31_t*) getNextValuesToCompute(receiver);
}

template <>
inline q31_t* FeatureTemplate<q31_t>::getLastRecordedQ31Values(uint8_t sectionCount)
{
    return (q31_t*) getLastRecordedValues(sectionCount);
}

#endif // FEATURECLASS_H
