#ifndef FEATURECLASS_H
#define FEATURECLASS_H

#include <Arduino.h>
#include <ArduinoJson.h>
/* CMSIS-DSP library for RFFT */
#include <arm_math.h>

#include "Keywords.h"
#include "Logger.h"


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
        virtual void stream(HardwareSerial *port);
        /***** Debugging *****/
        virtual void exposeConfig();
        virtual void exposeCounters();


    protected:
        /***** Instance registry *****/
        uint8_t m_instanceIdx;
        /***** Feature designation *****/
        char m_name[4];
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
        uint8_t m_computeIndex[maxReceiverCount];
        bool m_published[maxSectionCount];
        bool m_acknowledged[maxSectionCount][maxReceiverCount];
        // Lock a section to prevent both recording and computation, useful when
        // streaming the section content for example, to garantee data
        // consistency at section level
        bool m_locked[maxSectionCount];
        virtual void m_specializedStream(HardwareSerial *port,
                                         uint8_t sectionIdx) {}
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
                                         uint8_t sectionIdx);
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
                                         uint8_t sectionIdx);
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
                                         uint8_t sectionIdx);
};


/* =============================================================================
    Instanciations
============================================================================= */

/***** Battery load *****/

extern float batteryLoadValues[2];
extern FloatFeature batteryLoad;


/***** Accelerometer Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) q15_t accelerationXValues[1024];
extern __attribute__((section(".noinit2"))) q15_t accelerationYValues[1024];
extern __attribute__((section(".noinit2"))) q15_t accelerationZValues[1024];
extern Q15Feature accelerationX;
extern Q15Feature accelerationY;
extern Q15Feature accelerationZ;


// 128 sample long accel features
extern __attribute__((section(".noinit2"))) float accelRMS128XValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128YValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128ZValues[8];
extern __attribute__((section(".noinit2"))) float accelRMS128TotalValues[8];
extern FloatFeature accelRMS128X;
extern FloatFeature accelRMS128Y;
extern FloatFeature accelRMS128Z;
extern FloatFeature accelRMS128Total;

// 512 sample long accel features
extern __attribute__((section(".noinit2"))) float accelRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512ZValues[2];
extern __attribute__((section(".noinit2"))) float accelRMS512TotalValues[2];
extern FloatFeature accelRMS512X;
extern FloatFeature accelRMS512Y;
extern FloatFeature accelRMS512Z;
extern FloatFeature accelRMS512Total;

// FFT feature from 512 sample long accel data
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTXValues[300];
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTYValues[300];
extern __attribute__((section(".noinit2"))) q15_t accelReducedFFTZValues[300];
extern Q15Feature accelReducedFFTX;
extern Q15Feature accelReducedFFTY;
extern Q15Feature accelReducedFFTZ;

// Acceleration main Frequency features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float accelMainFreqXValues[2];
extern __attribute__((section(".noinit2"))) float accelMainFreqYValues[2];
extern __attribute__((section(".noinit2"))) float accelMainFreqZValues[2];
extern FloatFeature accelMainFreqX;
extern FloatFeature accelMainFreqY;
extern FloatFeature accelMainFreqZ;

// Velocity features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float velRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float velRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float velRMS512ZValues[2];
extern FloatFeature velRMS512X;
extern FloatFeature velRMS512Y;
extern FloatFeature velRMS512Z;

// Displacements features from 512 sample long accel data
extern __attribute__((section(".noinit2"))) float dispRMS512XValues[2];
extern __attribute__((section(".noinit2"))) float dispRMS512YValues[2];
extern __attribute__((section(".noinit2"))) float dispRMS512ZValues[2];
extern FloatFeature dispRMS512X;
extern FloatFeature dispRMS512Y;
extern FloatFeature dispRMS512Z;


/***** Gyroscope Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) q15_t tiltXValues[2];
extern __attribute__((section(".noinit2"))) q15_t tiltYValues[2];
extern __attribute__((section(".noinit2"))) q15_t tiltZValues[2];
extern Q15Feature tiltX;
extern Q15Feature tiltY;
extern Q15Feature tiltZ;


/***** Magnetometer Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) q15_t magneticXValues[2];
extern __attribute__((section(".noinit2"))) q15_t magneticYValues[2];
extern __attribute__((section(".noinit2"))) q15_t magneticZValues[2];
extern Q15Feature magneticX;
extern Q15Feature magneticY;
extern Q15Feature magneticZ;


/***** Barometer Features *****/

// Sensor data
extern __attribute__((section(".noinit2"))) float temperatureValues[2];
extern FloatFeature temperature;

extern __attribute__((section(".noinit2"))) float pressureValues[2];
extern FloatFeature pressure;


/***** Audio Features *****/

// Sensor data
extern q15_t audioValues[8192];
extern Q15Feature audio;

// 2048 sample long features
extern __attribute__((section(".noinit2"))) float audioDB2048Values[4];
extern FloatFeature audioDB2048;

// 4096 sample long features
extern __attribute__((section(".noinit2"))) float audioDB4096Values[2];
extern FloatFeature audioDB4096;


/***** GNSS Feature *****/


/***** RTD Temperature features *****/

#ifdef RTD_DAUGHTER_BOARD // Optionnal hardware
extern __attribute__((section(".noinit2"))) float rtdTempValues[8];
extern FloatFeature rtdTemp;
#endif // RTD_DAUGHTER_BOARD


#endif // FEATURECLASS_H
