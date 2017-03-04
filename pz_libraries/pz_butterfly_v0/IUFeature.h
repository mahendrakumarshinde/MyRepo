#ifndef IUFEATURECONFIG_H
#define IUFEATURECONFIG_H

#include <Arduino.h>
#include <arm_math.h>

class IUFeature
{
    public:
        IUFeature(uint8_t id);
        void setId(uint8_t id) { m_id = id; }
        uint8_t getId() { return m_id; }
        void setName(String name) { m_name = name; }
        String getName() { return m_name; }
        // Feature computation, source and destination
        void setComputeFunction(float (*computeFunction) ()) { m_computeFunction = computeFunction; }
        void setSource(uint16_t sourceSize, q15_t *source);
        void setDestination(uint16_t destinationSize, q15_t *destination);
        bool activate();
        // Thresholds and state
        void setThresholds(float normal, float warning, float danger);
        void getThreshold(uint8_t index) { return m_tresholds[index]}
        uint8_t getThresholdState();
        // Run
        float receive(q15_t value);
        float compute();
        bool stream(Stream *port);

    private:
        uint8_t m_id;
        String m_name;
        bool m_active;
        float (*m_computeFunction);
        uint16_t m_sourceSize;
        q15_t *m_source;
        uint16_t m_destinationSize;
        flaot *m_destination;
        // Thresholds and state
        float m_thresholds[3]; // Normal, warning and danger thresholds
        uint8_t m_state; // possible states are 0: not cutting, 1: normal, 2: warning, 3: danger
        uint8_t m_highestDangerLevel; // The most critical state ever measured

};


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

float default_compute() { return 0; }


#endif // IUFEATURECONFIG_H
