#ifndef IUFEATURECONFIG_H
#define IUFEATURECONFIG_H

#include <Arduino.h>
#include <arm_math.h>
#include <MD_CirQueue.h>
#include "IUUtilities.h"


/**
 * Abstract base class for features
 */
class IUABCFeature
{
    public:
        IUABCFeature() {}
        virtual ~IUABCFeature() {}
        void setId(uint8_t id) { m_id = id; }
        uint8_t getId() { return m_id; }
        void setName(String name) { m_name = name; }
        String getName() { return m_name; }
        // Feature computation, source, sending queue and receivers
        virtual void setComputeFunction(float (*computeFunction) ()) = 0; // Pure virtual
        virtual void prepareSource(uint16_t sourceSize) = 0; // Pure virtual
        void prepareSendingQueue(uint16_t sendingQueueSize);
        bool activate();
        bool isActive() { return m_active; }
        void enableFeatureCheck() { m_checkFeature = true; }
        void disableFeatureCheck() { m_checkFeature = false; }
        bool isFeatureCheckActive() { return m_checkFeature; }
        // Thresholds and state
        void setThresholds(float normal, float warning, float danger);
        void getThreshold(uint8_t index) { return m_tresholds[index]}
        // Run
        virtual bool receive() = 0; // Pure virtual
        virtual float compute() = 0; // Pure virtual
        uint8_t getThresholdState();
        float peekNextValue();
        bool stream(Stream *port);

    private:
        uint8_t m_id;
        String m_name;
        bool m_active;
        bool m_checkFeature;
        float (*m_computeFunction) ();
        uint16_t m_sourceSize;
        q15_t *m_source;
        virtual void newSource() = 0; // Pure virtual
        uint16_t m_sourceCounter;
        bool m_computeNow[2];
        bool m_recordNow[2];
        uint8_t m_computeIndex;   // Computation buffer index
        uint8_t m_recordIndex;    // Data recording buffer index
        // Using a queue for sending, so that we always have the n latest results
        uint8_t m_sendingQueueSize; // MD_Queue size
        MD_CirQueue *m_sendingQueue;
        // Thresholds and state
        float m_thresholds[3]; // Normal, warning and danger thresholds
        uint8_t m_state; // possible states are 0: not cutting, 1: normal, 2: warning, 3: danger
        uint8_t m_highestDangerLevel; // The most critical state ever measured
};

/**
 * Feature with a single source, with number in Q15 format
 */
class IUSingleQ15SourceFeature : IUABCFeature
{
    public:
        IUSingleQ15SourceFeature(uint8_t id, String name);
        virtual ~IUSingleQ15SourceFeature();
        // Feature computation, source, sending queue and receivers
        virtual void setComputeFunction(float (*computeFunction) (uint16_t, q15_t*));
        virtual void prepareSource(uint16_t sourceSize);
        void setReceivers(uint8_t receiverCount, IUABCFeature *receivers)
        // Run
        virtual bool receive(q15_t value);
        virtual float compute();

    private:
        float (*m_computeFunction) (uint16_t sourceSize, q15_t* source);
        uint16_t m_sourceSize;
        q15_t *m_source;
        virtual void newSource(uint16_t *sourceSize);
        uint16_t m_sourceCounter;
        bool m_computeNow[2];
        bool m_recordNow[2];
        uint8_t m_computeIndex;   // Computation buffer index
        uint8_t m_recordIndex;    // Data recording buffer index
        // Using a queue for sending, so that we always have the n latest results
        uint8_t m_sendingQueueSize; // MD_Queue size
        MD_CirQueue *m_sendingQueue;
        uint8_t m_receiverCount;
        IUABCFeature *m_receivers;
        
};


/**
 * Feature with a single source, with number in float format
 */
class IUSingleFloatSourceFeature : public IUSingleQ15SourceFeature
{
    public:
        // Feature computation, source, sending queue and receivers
        virtual void setComputeFunction(float (*computeFunction) (uint16_t, float*));
        // Run
        virtual bool receive(float value);

    private:
        float (*m_computeFunction) (uint16_t sourceSize, float* source);
        float *m_source;
        virtual void newSource(uint16_t *sourceSize);
}


/**
 * Feature with multiple sources, with number in Q15 format
 */
class IUMultiQ15SourceFeature : public IUABCFeature
{
    public:
        IUMultiSourceFeature(uint8_t id, String name="");
        ~IUMultiQ15SourceFeature();
        virtual void setComputeFunction(float (*computeFunction) (uint8_t, uint16_t*, q15_t*));
        virtual void prepareSource(uint8_t sourceCount, uint16_t *sourceSize);
        // Run
        virtual bool receive(uint8_t sourceIndex, q15_t value);
        virtual float compute();

    private:
        float (*m_computeFunction) (uint8_t sourceCount, uint16_t* sourceSize, q15_t* source);
        uint8_t m_sourceCount;
        uint16_t *m_sourceSize;
        q15_t *m_source;
        void newSource(uint8_t sourceCount, uint16_t *sourceSize);
        uint16_t *m_sourceCounter;
        bool *m_computeNow;
        bool *m_recordNow;
        uint8_t *m_computeIndex;   // Computation buffer index
        uint8_t *m_recordIndex;    // Data recording buffer index
};


/**
 * Feature with multiple sources, with number in float format
 */
class IUMultiFloatSourceFeature : public IUMultiQ15SourceFeature
{
    public:
        // Feature computation, source, sending queue and receivers
        virtual void setComputeFunction(float (*computeFunction) (uint8_t, uint16_t*, q15_t*));
        // Run
        virtual bool receive(float value);

    private:
        float (*m_computeFunction) (uint16_t sourceSize, float* source);
        float *m_source;
        virtual void newSource(uint16_t *sourceSize);
}


class IUFeatureCollection
{
    public:
        // Arrays have to be initialized with a fixed size,
        // so we chose max_size = max number of features per collections
        static const uint8_t MAX_SIZE = 10;
        IUFeatureCollection();
        bool addFeature(IUFeature feature);
        bool addFeature(String name, float (*computeFunction) ());
        IUFeature getFeature(uint8_t index);
        uint8_t getSize() { return m_size; }

    private:
        IUFeature m_features[MAX_SIZE];
        uint8_t m_size; // Dynamic


};

float default_compute(q15_t* source) { return source[0]; }

/* ========================== Feature Definition ============================= */
IUMultiSourceFeature accelerationEnergy = IUMultiSourceFeature(1, "acceleration_energy");
uint16_t sourceSize[3] = {512, 512, 512}
accelerationEnergy.prepareSource(3, sourceSize);
accelerationEnergy.setComputeFunction(computeSignalEnergy);

IUMultiSourceFeature velocityX = IUMultiSourceFeature(2, "velocity_x");
uint16_t sourceSize[3] = {1, 1, 512}
VelocityX.prepareSource(3, sourceSize);
VelocityX.setComputeFunction(computeVelocity);

IUMultiSourceFeature velocityY = IUMultiSourceFeature(3, "velocity_y");
uint16_t sourceSize[3] = {1, 1, 512}
VelocityX.prepareSource(3, sourceSize);
VelocityX.setComputeFunction(computeVelocity);

IUMultiSourceFeature velocityZ = IUMultiSourceFeature(4, "velocity_z");
uint16_t sourceSize[3] = {1, 1, 512}
VelocityX.prepareSource(3, sourceSize);
VelocityX.setComputeFunction(computeVelocity);

IUFeature temperature = IUFeature(5, "current_temperature");
temperature.prepareSource(1);
// The temperature compute function is the default one

IUMultiSourceFeature audioDB = IUMultiSourceFeature(6, "current_temperature");
audioDB.prepareSource(1);



#endif // IUFEATURECONFIG_H
