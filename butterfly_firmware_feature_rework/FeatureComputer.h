#ifndef FEATURECOMPUTER_H
#define FEATURECOMPUTER_H

#include "FeatureClass.h"
//#include "Utilities.h"


/* =============================================================================
    Feature Computer Base Class
============================================================================= */

class FeatureComputer
{
    public:
        static const uint8_t maxSourceCount = 5;
        static const uint8_t maxDestinationCount = 5;
        FeatureComputer(uint8_t id, uint8_t destinationCount=0,
                        Feature *destination0=NULL, Feature *destination1=NULL,
                        Feature *destination2=NULL, Feature *destination3=NULL,
                        Feature *destination4=NULL);
        virtual ~FeatureComputer() {}
        virtual uint8_t getId() { return m_id; }
        /***** Configuration *****/
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
        /***** Debugging *****/
        void exposeConfig();

    protected:
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
                          bool removeMean=false, bool normalize=false);
        // Change parameters
        void setRemoveMean(bool value) { m_removeMean = value; }
        void setNormalize(bool value) { m_normalize = value; }

    protected:
        virtual void m_specializedCompute();
        bool m_removeMean;  // Remove mean before computing Signal Energy ?
        bool m_normalize;  // Divide by source sectionSize ?
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
                           bool normalize=false);
        // Change parameters
        void setNormalize(bool value) { m_normalize = value; }

    protected:
        virtual void m_specializedCompute();
        bool m_normalize;  // Return the average instead of the sum
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
                               bool normalize=false);
        // Change parameters
        void setNormalize(bool value) { m_normalize = value; }

    protected:
        virtual void m_specializedCompute();
        bool m_normalize;  // Return the average instead of the sum
};


/**
 * FFT for Q15 values
 *
 * Sources:
 *      - A Q15 buffer
 * Destinations:
 *      - Reduced FFT values: A Q15 buffer
 *      - Integrated values: A Q15 buffer
 *      - Double-integrated values: A Q15 buffer
 */
class Q15FFTComputer: public FeatureComputer
{
    public:
        Q15FFTComputer(uint8_t id, Feature *reducedFFT=NULL,
                       Feature *integralRMS=NULL,
                       Feature *integralPeakToPeak=NULL,
                       Feature *doubleIntegralRMS=NULL,
                       Feature *doubleIntegralPeakToPeak=NULL,
                       q15_t *allocatedFFTSpace=NULL,
                       uint8_t outputFFTIntegration=1,
                       bool computeReducedFFT=false,
                       bool computeIntegral=true,
                       bool computeDoubleIntegral=false);
        // Change parameters
        void setOutputFFTIntegration(uint8_t value)
                { m_outputFFTIntegration = value; }
        void setComputeReducedFFT(bool value) { m_computeReducedFFT = value; }
        void setComputeIntegral(bool value) { m_computeIntegral = value; }
        void setComputeDoubleIntegral(bool value)
                { m_computeDoubleIntegral = value; }

    protected:
        virtual void m_specializedCompute();
        //arm_rfft_instance_q15 m_rfftInstance;
        q15_t *m_allocatedFFTSpace;
        uint8_t m_outputFFTIntegration;
        bool m_computeReducedFFT;
        bool m_computeIntegral;
        bool m_computeDoubleIntegral;

};


/**
 * FFT for Q31 values
 *
 * Sources:
 *      - 1 Q31 buffers
 * Destinations:
 *      - 1 Q31 buffer, with section size twice as large as the source section
 *          size
 */
/*
class Q31FFTComputer: public FeatureComputer
{
    public:
        Q31FFTComputer(uint8_t id=0, Feature *destination0=NULL);

    protected:
        virtual void m_specializedCompute();
        arm_rfft_instance_q31 m_rfftInstance;
};
*/


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
        AudioDBComputer(uint8_t id, Feature *audioDB=NULL);

    protected:
        virtual void m_specializedCompute();
};


#endif // FEATURECOMPUTER_H
