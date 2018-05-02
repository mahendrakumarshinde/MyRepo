#ifndef FEATURECLASS_H
#define FEATURECLASS_H

#include <Arduino.h>
#include <ArduinoJson.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include <IUDebugger.h>


/* =============================================================================
    Feature Operation States
============================================================================= */

/**
 * Operation states describe the production status
 *
 * The state is determined by features values and user-defined thresholds.
 */
namespace OperationState
{
    enum option : uint8_t {IDLE    = 0,
                           NORMAL  = 1,
                           WARNING = 2,
                           DANGER  = 3,
                           COUNT   = 4};
}


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
        /***** Core *****/
        Feature(const char* name, uint8_t sectionCount=2,
                uint16_t sectionSize=1, slideOption sliding=FIXED);
        virtual ~Feature();
        virtual void reset();
        virtual uint8_t addReceiver(uint8_t receiverId);
        /***** Feature designation *****/
        virtual char* getName() { return m_name; }
        virtual bool isNamed(const char* name)
            { return strcmp(m_name, name) == 0; }
        virtual bool isScalar() { return m_sectionSize == 1; }
        static Feature *getInstanceByName(const char *name);
        /***** Physical metadata *****/
        virtual void setSamplingRate(uint16_t rate) { m_samplingRate = rate; }
        virtual uint16_t getSamplingRate() { return m_samplingRate; }
        virtual void setResolution(float res) { m_resolution = res; }
        virtual float getResolution() { return m_resolution; }
        /***** Configuration *****/
        virtual bool isStreaming() { return m_streamingEnabled; }
        virtual void enableStreaming() { m_streamingEnabled = true; }
        virtual void disableStreaming() { m_streamingEnabled = false; }
        virtual bool opStateEnabled() { return m_opStateEnabled; }
        virtual void enableOperationState() { m_opStateEnabled = true; }
        virtual void disableOperationState() { m_opStateEnabled = false; }
        virtual void activate() { m_active = true; }
        virtual void deactivate();
        virtual bool isActive() { return m_active; }
        virtual void configure(JsonVariant &config);
        /***** OperationState and Thresholds *****/
        virtual OperationState::option getOperationState()
            { return m_operationState; }
        virtual void updateOperationState();
        virtual void setThreshold(uint8_t idx, float value)
            { m_thresholds[idx] = value; }
        virtual void setThresholds(float normalVal, float warningVal,
                                   float dangerVal);
        virtual float getThreshold(uint8_t idx) { return m_thresholds[idx]; }
        virtual float getValueToCompareToThresholds()
            { return m_thresholds[0] - 1;}
        /***** Computers tracking *****/
        virtual bool isComputedFeature() { return m_computerId != 0; }
        virtual void setComputerId(uint8_t id) { m_computerId = id; }
        virtual uint8_t getComputerId() { return m_computerId; }
        virtual uint8_t getReceiverCount() { return m_receiverCount; }
        virtual uint8_t getReceiverId(uint8_t idx)
            { return m_receiversId[idx]; }
        /***** Buffer core *****/
        // Accessors to values
        virtual uint16_t getSectionSize() {return m_sectionSize; }
        virtual q15_t* getNextQ15Values(uint8_t receiverIdx) { return NULL; }
        virtual float* getNextFloatValues(uint8_t receiverIdx) { return NULL; }
        virtual q31_t* getNextQ31Values(uint8_t receiverIdx) { return NULL; }
        virtual void addQ15Value(q15_t value) {}
        virtual void addFloatValue(float value) {}
        virtual void addQ31Value(q31_t value) {}
        // Tracking the buffer state
        virtual bool isReadyToRecord(uint8_t sectionCount=1);
        virtual bool isReadyToCompute(uint8_t receiverIdx,
                                      uint8_t sectionCount=1);
        virtual void incrementFillingIndex();
        virtual void acknowledge(uint8_t receiverIdx, uint8_t sectionCount=1);
        /***** Communication *****/
        virtual void stream(HardwareSerial *port, uint8_t sectionCount=1);
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
        /***** Physical metadata *****/
        uint16_t m_samplingRate;
        float m_resolution;
        /***** Configuration variables *****/
        bool m_active;
        bool m_opStateEnabled;
        bool m_streamingEnabled;
        /***** OperationState and Thresholds *****/
        OperationState::option m_operationState;
        // Normal, warning and danger thresholds
        float m_thresholds[OperationState::COUNT - 1];
        /***** Computers tracking *****/
        uint8_t m_computerId;
        uint8_t m_receiverCount;
        uint8_t m_receiversId[maxReceiverCount];
        /***** Buffer core *****/
        uint8_t m_sectionCount;
        uint16_t m_sectionSize;
        slideOption m_sliding;
        uint16_t m_totalSize;
        // Tracking the buffer state
        uint16_t m_fillingIndex;
        uint8_t m_recordIndex;
        uint8_t m_computeIndex[maxReceiverCount];
        bool m_published[maxSectionCount];
        bool m_acknowledged[maxSectionCount][maxReceiverCount];
        // Lock a section to prevent both recording and computation, useful when
        // streaming the section content for example, to garantee data
        // consistency at section level
        bool m_locked[maxSectionCount];
        virtual void m_specializedStream(HardwareSerial *port,
                                         uint8_t sectionIdx,
                                         uint8_t sectionCount=1) {}
        virtual uint16_t m_specializedBufferStream(uint8_t sectionIdx,
            char *destination, uint16_t startIndex, uint8_t sectionCount=1) {}
};


/* =============================================================================
    Format specific features
============================================================================= */

/**
 * A Feature which values are float numbers
 *
 * Values should be an array of size = sectionCount * sectionSize
 */
class FloatFeature : public Feature
{
    public:
        FloatFeature(const char* name, uint8_t sectionCount=2,
                     uint16_t sectionSize=1, float *values=NULL,
                     Feature::slideOption sliding=Feature::FIXED);
        virtual ~FloatFeature() {}
        virtual float* getNextFloatValues(uint8_t receiverIdx);
        virtual void addFloatValue(float value);
        /***** OperationState and Thresholds *****/
        virtual float getValueToCompareToThresholds();

    protected:
        float *m_values;
        virtual void m_specializedStream(HardwareSerial *port,
                                         uint8_t sectionIdx,
                                         uint8_t sectionCount=1);
        virtual uint16_t m_specializedBufferStream(uint8_t sectionIdx,
            char *destination, uint16_t startIndex, uint8_t sectionCount=1);
};


/**
 * A Feature which values are q15_t numbers
 *
 * Values should be an array of size = sectionCount * sectionSize
 *
 * The parameter "isFFT" gives meaning to the feature content. If true, the
 * feature contains FFT coeffs, organised as follow:
 *  - 3 * i: Index of the Fourrier coefficient
 *  - 3 * i + 1: Real part the Fourrier coefficient
 *  - 3 * i + 2: Imaginary part the Fourrier coefficient
 * That means that if the FFT feature must hold 50 Fourrier coefficients, its
 * sectionSize should be 150.
 * If the feature contains FFT coef, the resolution will naturally only apply
 * to the real and imaginary parts of the coefficients, not to the indexes.
 */
class Q15Feature : public Feature
{
    public:
        Q15Feature(const char* name, uint8_t sectionCount=2,
                   uint16_t sectionSize=1, q15_t *values=NULL,
                   Feature::slideOption sliding=Feature::FIXED,
                   bool isFFT=false);
        virtual ~Q15Feature() {}
        virtual q15_t* getNextQ15Values(uint8_t receiverIdx);
        virtual void addQ15Value(q15_t value);

    protected:
        q15_t *m_values;
        bool m_isFFT;
        virtual void m_specializedStream(HardwareSerial *port,
                                         uint8_t sectionIdx,
                                         uint8_t sectionCount=1);
        virtual uint16_t m_specializedBufferStream(uint8_t sectionIdx,
            char *destination, uint16_t startIndex, uint8_t sectionCount=1);
};


/**
 * A Feature which values are q31_t numbers
 *
 * Values should be an array of size = sectionCount * sectionCount
 *
 * The parameter "isFFT" gives meaning to the feature content. If true, the
 * feature contains FFT coeffs, organised as follow:
 *  - 3 * i: Index of the Fourrier coefficient
 *  - 3 * i + 1: Real part the Fourrier coefficient
 *  - 3 * i + 2: Imaginary part the Fourrier coefficient
 * That means that if the FFT feature must hold 50 Fourrier coefficients, its
 * sectionSize should be 150.
 * If the feature contains FFT coef, the resolution will naturally only apply
 * to the real and imaginary parts of the coefficients, not to the indexes.
 */
class Q31Feature : public Feature
{
    public:
        Q31Feature(const char* name, uint8_t sectionCount=2,
                   uint16_t sectionSize=1, q31_t *values=NULL,
                   Feature::slideOption sliding=Feature::FIXED,
                   bool isFFT=false);
        virtual ~Q31Feature() {}
        virtual q31_t* getNextQ31Values(uint8_t receiverIdx);
        virtual void addQ31Value(q31_t value);

    protected:
        q31_t *m_values;
        bool m_isFFT;
        virtual void m_specializedStream(HardwareSerial *port,
                                         uint8_t sectionIdx,
                                         uint8_t sectionCount=1);
        virtual uint16_t m_specializedBufferStream(uint8_t sectionIdx,
            char *destination, uint16_t startIndex, uint8_t sectionCount=1);
};


#endif // FEATURECLASS_H
