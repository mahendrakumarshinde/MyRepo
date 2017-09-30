#ifndef FEATURECOMPUTER_H
#define FEATURECOMPUTER_H

#include "FeatureBuffer.h"


/* =============================================================================
    Feature Computer Base Class
============================================================================= */

class FeatureComputer
{
    public:
        static const uint8_t maxSourceCount = 5;
        static const uint8_t maxDestinationCount = 5;
        FeatureComputer(uint8_t id,
                        uint8_t destinationCount=0,
                        FeatureBuffer *destination0=NULL,
                        FeatureBuffer *destination1=NULL,
                        FeatureBuffer *destination2=NULL,
                        FeatureBuffer *destination3=NULL,
                        FeatureBuffer *destination4=NULL);
        virtual ~FeatureComputer() {}
        virtual uint8_t getId() { return m_id; }
        /***** Configuration *****/
        virtual void activate() { m_active = true; }
        virtual void deactivate() { m_active = false; }
        virtual bool isActive() { return m_active; }
        /***** Sources and destinations *****/
        virtual void addSource(FeatureBuffer *source, uint8_t sectionCount,
                               uint8_t skippedSectionCount=0);
        virtual uint8_t getSourceCount() { return m_sourceCount; }
        virtual FeatureBuffer* getSource(uint8_t idx) { return m_sources[idx]; }
        virtual uint8_t getDestinationCount() { return m_destinationCount; }
        virtual FeatureBuffer* getDestination(uint8_t idx)
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
        FeatureBuffer *m_sources[maxSourceCount];
        uint8_t m_indexesAsReceiver[maxSourceCount];
        uint8_t m_sectionCount[maxSourceCount];
        uint8_t m_skippedSectionCount[maxSourceCount];
        // Destination buffers
        uint8_t m_destinationCount;
        FeatureBuffer *m_destinations[maxDestinationCount];
        /***** Computation *****/
        virtual void m_specializedCompute() {}
};


/* =============================================================================
    Feature Computers
============================================================================= */

/**
 * Signal Energy, Power and RMS
 *
 * Sources:
 *      - Signal data: Q15 data buffer (with sampling rate and resolution)
 * Destinations:
 *      - Signal Energy: Float data buffer with sectionSize = 1
 *      - Signal Power: Float data buffer with sectionSize = 1
 *      - Signal RMS: Float data buffer with sectionSize = 1
 */
class SignalEnergyComputer: public FeatureComputer
{
    public:
        SignalEnergyComputer(uint8_t id,
                             FeatureBuffer *destEnergy=NULL,
                             FeatureBuffer *destPower=NULL,
                             FeatureBuffer *destRMS=NULL,
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
                           FeatureBuffer *destination0=NULL,
                           FeatureBuffer *destination1=NULL,
                           FeatureBuffer *destination2=NULL,
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
 *      - Several Float buffers
 * Destinations:
 *      - 1 Float buffer
 */
class MultiSourceSumComputer: public FeatureComputer
{
    public:
        MultiSourceSumComputer(uint8_t id, FeatureBuffer *destination0=NULL,
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
        Q15FFTComputer(uint8_t id,
                       FeatureBuffer *reducedFFT=NULL,
                       FeatureBuffer *integralRMS=NULL,
                       FeatureBuffer *integralPeakToPeak=NULL,
                       FeatureBuffer *doubleIntegralRMS=NULL,
                       FeatureBuffer *doubleIntegralPeakToPeak=NULL,
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
        Q31FFTComputer(uint8_t id=0, FeatureBuffer *destination0=NULL);

    protected:
        virtual void m_specializedCompute();
        arm_rfft_instance_q31 m_rfftInstance;
};
*/


/**
 * Audio decibel computer for Q15 values
 *
 * Sources:
 *      - A Q15 buffer
 * Destinations:
 *      - Audio dB value: a float buffer
 */
class AudioDBComputer: public FeatureComputer
{
    public:
        AudioDBComputer(uint8_t id, FeatureBuffer *audioDB=NULL);

    protected:
        virtual void m_specializedCompute();
};


#endif // FEATURECOMPUTER_H
