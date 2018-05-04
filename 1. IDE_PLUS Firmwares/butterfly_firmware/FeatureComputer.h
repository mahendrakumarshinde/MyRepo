#ifndef FEATURECOMPUTER_H
#define FEATURECOMPUTER_H

#include "FeatureClass.h"
#include "Utilities.h"


/* =============================================================================
    Feature Computer Base Class
============================================================================= */

class FeatureComputer
{
    public:
        static const uint8_t maxSourceCount = 5;
        static const uint8_t maxDestinationCount = 5;
        /***** Instance registry *****/
        static const uint8_t MAX_INSTANCE_COUNT = 20;
        static uint8_t instanceCount;
        static FeatureComputer *instances[MAX_INSTANCE_COUNT];
        /***** Core *****/
        FeatureComputer(uint8_t id, uint8_t destinationCount=0,
                        Feature *destination0=NULL, Feature *destination1=NULL,
                        Feature *destination2=NULL, Feature *destination3=NULL,
                        Feature *destination4=NULL);
        virtual ~FeatureComputer();
        virtual uint8_t getId() { return m_id; }
        static FeatureComputer *getInstanceById(const uint8_t id);
        /***** Configuration *****/
        virtual void configure(JsonVariant &config) {}
        virtual void activate() { m_active = true; }
        virtual void deactivate() { m_active = false; }
        virtual bool isActive() { return m_active; }
        /***** Sources and destinations *****/
        virtual void addSource(Feature *source, uint8_t sectionCount);
        virtual uint8_t getSourceCount() { return m_sourceCount; }
        virtual Feature* getSource(uint8_t idx) { return m_sources[idx]; }
        virtual uint8_t getDestinationCount() { return m_destinationCount; }
        virtual Feature* getDestination(uint8_t idx)
            { return m_destinations[idx]; }
        /***** Computation *****/
        virtual bool compute();
        virtual void acknowledgeSectionToSources();
        /***** Debugging *****/
        virtual void exposeConfig();

    protected:
        /***** Instance registry *****/
        uint8_t m_instanceIdx;
        /***** Configuration *****/
        uint8_t m_id;
        bool m_active;
        /***** Sources and destinations *****/
        // Source buffers
        uint8_t m_sourceCount;
        Feature *m_sources[maxSourceCount];
        uint8_t m_indexesAsReceiver[maxSourceCount];
        uint8_t m_sectionCount[maxSourceCount];
        // Destination buffers
        uint8_t m_destinationCount;
        Feature *m_destinations[maxDestinationCount];
        /***** Computation *****/
        virtual void m_specializedCompute() {}
};


/* =============================================================================
    Feature Computers
============================================================================= */

/**
 * Signal RMS
 *
 * Sources:
 *      - Signal data: Q15 data buffer (with sampling rate and resolution)
 * Destinations:
 *      - Signal RMS: Float data buffer
 */
class SignalRMSComputer: public FeatureComputer
{
    public:
        SignalRMSComputer(uint8_t id, Feature *rms=NULL,
                          bool removeMean=false, bool normalize=false,
                          bool squared=false, float calibrationScaling=1.);
        /***** Configuration *****/
        virtual void configure(JsonVariant &config);
        void setRemoveMean(bool value) { m_removeMean = value; }
        void setNormalize(bool value) { m_normalize = value; }
        void setSquaredOutput(bool value) { m_squared = value; }
        void setCalibrationScaling(float val) { m_calibrationScaling = val; }

    protected:
        virtual void m_specializedCompute();
        bool m_removeMean;  // Remove mean before computing Signal Energy ?
        bool m_normalize;  // Divide by source sectionSize ?
        bool m_squared;  // Output is (RMS)^2 (or just RMS)
        float m_calibrationScaling;  // Scaling factor
};


/**
 * Sum several sections, source by source
 *
 * Sources:
 *      - 1 or several Float buffers: it is expected that the sources are
 *      compatible, ie they have the same sampling rate, resolution, number of
 *      sections and the same section size.
 * Destinations:
 *      - As many Float buffers as there are sources
 */
class SectionSumComputer: public FeatureComputer
{
    public:
        SectionSumComputer(uint8_t id, uint8_t destinationCount=0,
                           Feature *destination0=NULL,
                           Feature *destination1=NULL,
                           Feature *destination2=NULL,
                           bool normalize=false, bool rmsInput=false);
        /***** Configuration *****/
        virtual void configure(JsonVariant &config);
        void setNormalize(bool value) { m_normalize = value; }
        void setRMSInput(bool value) { m_rmsInput = value; }

    protected:
        virtual void m_specializedCompute();
        bool m_normalize;  // Return the average instead of the sum
        bool m_rmsInput;  // Return the average instead of the sum
};


/**
 * Sum several sources, section by section
 *
 * Sources:
 *      - Several Float buffers, with the same section size and number of
 *      sections to compute at once (the computer will use these parameters from
 *      only the 1st source only and assume they are the same for all)
 * Destinations:
 *      - 1 Float buffer
 */
class MultiSourceSumComputer: public FeatureComputer
{
    public:
        MultiSourceSumComputer(uint8_t id, Feature *destination0=NULL,
                               bool normalize=false, bool rmsInput=false);
        /***** Configuration *****/
        virtual void configure(JsonVariant &config);
        void setNormalize(bool value) { m_normalize = value; }
        void setRMSInput(bool value) { m_rmsInput = value; }

    protected:
        virtual void m_specializedCompute();
        bool m_normalize;  // Return the average instead of the sum
        bool m_rmsInput;  // Return the average instead of the sum
};


/**
 * FFT for Q15 values
 *
 * Sources:
 *      - A Q15 buffer
 * Destinations:
 *      - Reduced FFT values: A FFTFeature
 *      - Integrated RMS: A Q15 buffer
 *      - Double-integrated RMS: A Q15 buffer
 */
class Q15FFTComputer: public FeatureComputer
{
    public:
        Q15FFTComputer(uint8_t id,
                       Feature *reducedFFT=NULL,
                       Feature *mainFrequency=NULL,
                       Feature *integralRMS=NULL,
                       Feature *doubleIntegralRMS=NULL,
                       q15_t *allocatedFFTSpace=NULL,
                       uint16_t lowCutFrequency=5,
                       uint16_t highCutFrequency=500,
                       float minAgitationRMS=0.1,
                       float calibrationScaling1=1.,
                       float calibrationScaling2=1.);
        /***** Configuration *****/
        virtual void configure(JsonVariant &config);
        void setLowCutFrequency(uint16_t value) { m_lowCutFrequency = value; }
        void setHighCutFrequency(uint16_t value) { m_highCutFrequency = value; }
        void setMinAgitationRMS(float value) { m_minAgitationRMS = value; }
        void setCalibrationScaling1(float val) { m_calibrationScaling1 = val; }
        void setCalibrationScaling2(float val) { m_calibrationScaling2 = val; }

    protected:
        virtual void m_specializedCompute();
        q15_t *m_allocatedFFTSpace;
        uint16_t m_lowCutFrequency;
        uint16_t m_highCutFrequency;
        float m_minAgitationRMS;
        bool m_calibrationScaling1;  // Scaling factor for 1st derivative RMS
        bool m_calibrationScaling2;  // Scaling factor for 2nd derivative RMS
};


/**
 * Audio decibel computer for Q15 values
 *
 * Sources:
 *      - A Q31 buffer
 * Destinations:
 *      - Audio dB value: a float buffer
 */
class AudioDBComputer: public FeatureComputer
{
    public:
        AudioDBComputer(uint8_t id, Feature *audioDB=NULL,
                        float calibrationScaling=1.);
        void setCalibrationScaling(float val) { m_calibrationScaling = val; }

    protected:
        virtual void m_specializedCompute();
        float m_calibrationScaling;  // Scaling factor
};


#endif // FEATURECOMPUTER_H
