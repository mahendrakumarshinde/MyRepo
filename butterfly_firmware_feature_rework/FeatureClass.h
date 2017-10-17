#ifndef FEATURECLASS_H
#define FEATURECLASS_H

#include <Arduino.h>
#include <ArduinoJson.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include "Keywords.h"
#include "Logger.h"


/* =============================================================================
    Generic Feature buffers
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
 * available to record new data).
 * Acknowledgement is done on a per-receiver basis (partial acknowledgement),
 * and then a global acknowledgement, which is a check of each partial
 * acknowledgement, can be performed.
 */
class Feature
{
    public:
        /* TODO - For now, only slideOption::FIXED is implemented => need to
        implement slideOption::ROLLING */
        enum slideOption : uint8_t {FIXED,
                                    ROLLING};
        static const uint8_t maxSectionCount = 8;
        static const uint8_t maxReceiverCount = 5;
        Feature(const char* name, uint8_t sectionCount=2,
                uint16_t sectionSize=1, slideOption sliding=FIXED);
        virtual ~Feature() {}
        virtual void reset();
        virtual uint8_t addReceiver(uint8_t receiverId);
        /***** Feature designation *****/
        virtual char* getName() { return m_name; }
        virtual bool isNamed(const char* name)
            { return strcmp(m_name, name) == 0; }
        virtual bool isScalar() { return m_sectionSize == 1; }
        /***** Physical metadata *****/
        virtual void setSamplingRate(uint16_t rate) { m_samplingRate = rate; }
        virtual uint16_t getSamplingRate() { return m_samplingRate; }
        virtual void setResolution(uint16_t res) { m_resolution = res; }
        virtual uint16_t getResolution() { return m_resolution; }
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
        virtual void setSensorName(const char* name)
            { strcpy(m_sensorName, name); }
        virtual char* getSensorName() { return m_sensorName; }
        virtual bool isComputedFeature() { return m_computerId != 0; }
        virtual void setComputerId(uint8_t id) { m_computerId = id; }
        virtual uint8_t getComputerId() { return m_computerId; }
        virtual uint8_t getReceiverCount() { return m_receiverCount; }
        virtual uint8_t getReceiverId(uint8_t idx)
            { return m_receiversId[idx]; }
        /***** Buffer core *****/
        // Accessors to values
        virtual uint16_t getSectionSize() {return m_sectionSize; }
        virtual q15_t* getNextQ15Values() { return NULL; }
        virtual float* getNextFloatValues() { return NULL; }
        virtual q31_t* getNextQ31Values() { return NULL; }
        virtual void addQ15Value(q15_t value) {}
        virtual void addFloatValue(float value) {}
        virtual void addQ31Value(q31_t value) {}
        // Tracking the buffer state
        virtual bool isReadyToCompute(uint8_t receiverIndex,
                                      uint8_t sectionCount=1);
        virtual bool isReadyToRecord(uint8_t sectionCount=1);
        virtual void partialAcknowledge(uint8_t receiverIdx,
                                        uint8_t sectionCount=1);
        virtual void acknowledgeIfReady();
        virtual void incrementFillingIndex();
        /***** Communication *****/
        void stream(HardwareSerial *port);
        /***** Debugging *****/
        void exposeConfig();
        void exposeCounters();


    protected:
        /***** Feature designation *****/
        char m_name[4];
        /***** Physical metadata *****/
        uint16_t m_samplingRate;
        uint16_t m_resolution;
        /***** Configuration variables *****/
        bool m_active;
        bool m_opStateEnabled;
        bool m_streamingEnabled;
        /***** OperationState and Thresholds *****/
        OperationState::option m_operationState;
        // Normal, warning and danger thresholds
        float m_thresholds[OperationState::COUNT - 1];
        /***** Computers tracking *****/
        char m_sensorName[4];
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
        uint8_t m_computeIndex;
        bool m_published[maxSectionCount];
        bool m_acknowledged[maxSectionCount];
        bool m_partialAcknowledged[maxSectionCount][maxReceiverCount];
        // Lock a section to prevent both recording and computation, useful when
        // streaming the section content for example, to garantee data
        // consistency at section level
        bool m_locked[maxSectionCount];
        void m_specializedStream(HardwareSerial *port, uint8_t sectionIdx) {}
};


/* =============================================================================
    Format specific buffers
============================================================================= */

/**
 * A Feature which values are float numbers
 *
 * Values should be an array of size = sectionCount * sectionCount
 */
class FloatFeature : public Feature
{
    public:
        FloatFeature(const char* name, uint8_t sectionCount=2,
                     uint16_t sectionSize=1, float *values=NULL,
                     Feature::slideOption sliding=Feature::FIXED);
        virtual ~FloatFeature() {}
        virtual float* getNextFloatValues();
        virtual void addFloatValue(float value);
        /***** OperationState and Thresholds *****/
        virtual float getValueToCompareToThresholds();

    protected:
        float *m_values;
        void m_specializedStream(HardwareSerial *port, uint8_t sectionIdx);
};


/**
 * A Feature which values are q15_t numbers
 *
 * Values should be an array of size = sectionCount * sectionCount
 */
class Q15Feature : public Feature
{
    public:
        Q15Feature(const char* name, uint8_t sectionCount=2,
                   uint16_t sectionSize=1, q15_t *values=NULL,
                   Feature::slideOption sliding=Feature::FIXED);
        virtual ~Q15Feature() {}
        virtual q15_t* getNextQ15Values();
        virtual void addQ15Value(q15_t value);

    protected:
        q15_t *m_values;
        void m_specializedStream(HardwareSerial *port, uint8_t sectionIdx);
};


/**
 * A Feature which values are q31_t numbers
 *
 * Values should be an array of size = sectionCount * sectionCount
 */
class Q31Feature : public Feature
{
    public:
        Q31Feature(const char* name, uint8_t sectionCount=2,
                   uint16_t sectionSize=1, q31_t *values=NULL,
                   Feature::slideOption sliding=Feature::FIXED);
        virtual ~Q31Feature() {}
        virtual q31_t* getNextQ31Values();
        virtual void addQ31Value(q31_t value);

    protected:
        q31_t *m_values;
        void m_specializedStream(HardwareSerial *port, uint8_t sectionIdx);
};

#endif // FEATURECLASS_H
