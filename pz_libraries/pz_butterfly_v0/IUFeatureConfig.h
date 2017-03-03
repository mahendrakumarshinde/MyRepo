#ifndef IUFEATURECONFIG_H
#define IUFEATURECONFIG_H

#include <Arduino.h>

class IUFeature
{
    public:
        IUFeature();
        IUFeature(String name, float (*computeFunction) ());
        void setName(String name) { m_name = name; }
        String getName() { return m_name; }
        float getNormalThreshold() { return m_NormalThreshold; }
        void setNormalThreshold(float val) { m_NormalThreshold = val; }
        float getWarningThreshold() { return m_WarningThreshold; }
        void setWarningThreshold(float val) { m_WarningThreshold = val; }
        float getDangerThreshold() { return m_DangerThreshold; }
        void setDangerThreshold(float val) { m_DangerThreshold = val; }
        void setThresholds(float normal, float warning, float danger);
        void setComputeFunction(float (*computeFunction) ()) { m_computeFunction = computeFunction; }
        float compute();
        uint8_t getThresholdState();

    private:
        String m_name;
        float m_value;
        uint8_t m_state; // possible states are 0: not cutting, 1: normal, 2: warning, 3: danger
        uint8_t m_highestDangerLevel; // The most critical state ever measured
        float m_normalThreshold;
        float m_warningThreshold;
        float m_dangerThreshold;
        float (*m_computeFunction) ();
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
