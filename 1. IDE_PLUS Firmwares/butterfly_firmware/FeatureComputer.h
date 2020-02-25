#ifndef FEATURECOMPUTER_H
#define FEATURECOMPUTER_H

#include "FeatureClass.h"
#include "FeatureUtilities.h"
#include "DiagnosticFingerPrint.h"
#include "RawDataState.h"

/* =============================================================================
 *  Motor Scaling Global Variable
 *  
 *==============================================================================*/

extern float motorScalingFactor ;
extern  bool computationDone;

//extern char FingerprintMessage[500];
/* =============================================================================
    Feature Computer Base Class
============================================================================= */

class FeatureComputer
{
    public:
        static const uint8_t maxSourceCount = 10;
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
        virtual bool addSource(Feature *source, uint8_t sectionCount);
        virtual void deleteAllSources();
        virtual uint8_t getSourceCount() { return m_sourceCount; }
        virtual Feature* getSource(uint8_t idx) { return m_sources[idx]; }
        virtual uint8_t getDestinationCount() { return m_destinationCount; }
        virtual Feature* getDestination(uint8_t idx)
            { return m_destinations[idx]; }
        virtual void updateSectionCount(int sectionCount) {} // to be implemented in FFTComputer, for configuring blockSize
        virtual void updateFrequencyLimits(int lowCutFrequency, int highCutFrequency) {} // to be implemented in FFTComputer, for configuring the frequency limits
        /***** Computation *****/
        virtual bool compute();
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
        uint8_t m_sectionCount[maxSourceCount];
        // Destination buffers
        uint8_t m_destinationCount;
        Feature *m_destinations[maxDestinationCount];
        /***** Computation *****/
        bool m_computeLast;
        bool m_sourceReadyForStateComputation[maxSourceCount];  // This is to be used only by FeatureStateComputer, however, values have to be updated in FeatureComputer::compute(), hence declared here
        virtual void m_specializedCompute() {}
};


/* =============================================================================
    Feature State Computer
============================================================================= */

/**
 * Feature State (Operation State)
 *
 * Sources:
 *      - Any features: The last section average value (x resolution) of each
 *          will be compared to the computer thresholds to get the feature
 *          state. The global state is the max of each feature state.
 * Destinations:
 *      - Feature state: an int in [0, 3]
 */
class FeatureStateComputer: public FeatureComputer
{
    public:

        FeatureStateComputer(uint8_t id, FeatureTemplate<q15_t> *featureState) :
            FeatureComputer(id, 1, featureState) { }
        /***** Sources and destinations *****/
        virtual bool addSource(Feature *source, uint8_t sectionCount,
                               bool active=true);
        /***** Configuration *****/
        bool addOpStateFeature(Feature *feature, float lowThreshold,
                               float medThreshold, float highThreshold,
                               uint8_t sectionCount=1, bool active=true);
        void setThresholds(uint8_t idx, float low, float med, float high);
        virtual void configure(JsonVariant &config);



    protected:
        /***** Configuration *****/
        bool m_activeOpStateFeatures[maxSourceCount];
        float m_lowThresholds[maxSourceCount];
        float m_medThresholds[maxSourceCount];
        float m_highThresholds[maxSourceCount];
        /***** Computation *****/
        virtual void m_specializedCompute();
        float m_getSectionAverage(Feature *feature);
};

/* =============================================================================
    Feature Computers
============================================================================= */

/**
 * Mean / Average
 *
 * Sources:
 *      - Input data: Float data buffer
 * Destinations:
 *      - Output RMS: Float data buffer
 */
class AverageComputer: public FeatureComputer
{
    public:
        AverageComputer(uint8_t id, FeatureTemplate<float> *input);
        /***** Configuration *****/

    protected:
        virtual void m_specializedCompute();
};


/**
 * Signal RMS for Q15 or Q31 values
 *
 * Sources:
 *      - Signal data: Q15 or Q31 feature
 * Destinations:
 *      - Signal RMS: Float feature
 */
template<typename T>
class SignalRMSComputer: public FeatureComputer
{
    public:
        SignalRMSComputer(uint8_t id, FeatureTemplate<float> *rms,
                          bool removeMean=false, bool normalize=false,
                          bool squared=false, float calibrationScaling=1.) :
            FeatureComputer(id, 1, rms),
            m_removeMean(removeMean),
            m_normalize(normalize),
            m_squared(squared),
            m_calibrationScaling(calibrationScaling)
        {
            // Constructor
        }
        /***** Configuration *****/
        virtual void configure(JsonVariant &config)
        {
            FeatureComputer::configure(config);
            JsonVariant my_config = config["NODC"];
            if (my_config.success()) {
                setRemoveMean(my_config.as<int>() > 0);
            }
            my_config = config["NORM"];
            if (my_config.success()) {
                setNormalize(my_config.as<int>() > 0);
            }
            my_config = config["SQR"];
            if (my_config.success()) {
                setSquaredOutput(my_config.as<int>() > 0);
            }
        }
        void setRemoveMean(bool value) { m_removeMean = value; }
        void setNormalize(bool value) { m_normalize = value; }
        void setSquaredOutput(bool value) { m_squared = value; }
        void setCalibrationScaling(float val) { m_calibrationScaling = val; }

    protected:
        bool m_removeMean;  // Remove mean before computing Signal Energy ?
        bool m_normalize;  // Divide by source sectionSize ?
        bool m_squared;  // Output is (RMS)^2 (or just RMS)
        float m_calibrationScaling;  // Scaling factor


        /**
         * Compute the total energy, power and RMS of a signal
         *
         * NB: Since the signal energy is squared, the resulting scaling factor
         * will be squared as well.
         * - Signal energy: sum((x(t)- x_mean)^2 * dt) for t in [0, dt, 2 *dt,
         * ..., T] where x_mean = Mean(x) if removeMean is true else 0,
         * dt = 1 / samplingFreq and T = sampleCount / samplingFreq.
         * Signal power: signal energy divided by Period
         * Signal RMS: square root of signal power
         */
        virtual void m_specializedCompute()
    {
        float resolution = m_sources[0]->getResolution();
        T *values = (T*) m_sources[0]->getNextValuesToCompute(this);
        uint16_t sectionSize = m_sources[0]->getSectionSize();
        uint16_t totalSize = m_sectionCount[0] * sectionSize;
        float total = 0;
        T avg = 0;
        if (m_removeMean) {
            getMean(values, (uint32_t) totalSize, &avg);
        }
        for (int i = 0; i < totalSize; ++i) {
            total += sq((values[i] - avg));
        }
        if (m_normalize) {
            total = total / (float) totalSize;
        }
        if (!m_squared) {
            total = sqrt(total);
        }
        m_destinations[0]->setSamplingRate(m_sources[0]->getSamplingRate());
        if (m_squared) {
            m_destinations[0]->setResolution(sq(resolution));
        } else {
            m_destinations[0]->setResolution(resolution);
        }
        m_destinations[0]->addValue(total * m_calibrationScaling);
        if (featureDebugMode)
        {
            debugPrint(millis(), false);
            debugPrint(" -> ", false);
            debugPrint(m_destinations[0]->getName(), false);
            debugPrint(": ", false);
            if (m_squared) {
                debugPrint(total * sq(resolution));
            } else {
                debugPrint(total * resolution);
            }
        }
    }
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
                           FeatureTemplate<float> *destination0=NULL,
                           FeatureTemplate<float> *destination1=NULL,
                           FeatureTemplate<float> *destination2=NULL,
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
        MultiSourceSumComputer(uint8_t id,
                               FeatureTemplate<float> *destination0,
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
 * FFT Computers
 *
 * Sources:
 *      - A Q15 or Q31 buffer
 * Destinations:
 *      - Reduced FFT values: A FFT q15 or q31 feature
 *      - Main frequency: A float feature
 *      - Integrated RMS: A float feature
 *      - Double-integrated RMS: A float feature
 */
template<typename T>
class FFTComputer: public FeatureComputer,public DiagnosticEngine
{
    public:
        FFTComputer(uint8_t id,
                    FeatureTemplate<T> *reducedFFT,
                    FeatureTemplate<float> *mainFrequency,
                    FeatureTemplate<float> *integralRMS,
                    FeatureTemplate<float> *doubleIntegralRMS,
                    T *allocatedFFTSpace,
                    uint16_t lowCutFrequency=5,
                    uint16_t highCutFrequency=1660,   // 500
                    float minAgitationRMS=0.1,
                    float calibrationScaling1=1.,
                    float calibrationScaling2=1.,
                    bool useCalibrationMethod=false) :
            FeatureComputer(id, 4, reducedFFT, mainFrequency, integralRMS,
                            doubleIntegralRMS),
            m_lowCutFrequency(lowCutFrequency),
            m_highCutFrequency(highCutFrequency),
            m_minAgitationRMS(minAgitationRMS),
            m_calibrationScaling1(calibrationScaling1),
            m_calibrationScaling2(calibrationScaling2),
            m_useCalibrationMethod(useCalibrationMethod)
        {
            m_allocatedFFTSpace = allocatedFFTSpace;
            m_computeLast = true;
        }
        /***** Configuration *****/
        virtual void configure(JsonVariant &config)
        {
            FeatureComputer::configure(config);
            JsonVariant freq = config["FREQ"][0];
            if (freq.success()) {
                setLowCutFrequency((uint16_t) (freq.as<int>()));
            }
            freq = config["FREQ"][1];
            if (freq.success()) {
                setHighCutFrequency((uint16_t) (freq.as<int>()));
            }
        }
        void setLowCutFrequency(uint16_t value) { m_lowCutFrequency = value; }
        void setHighCutFrequency(uint16_t value) { m_highCutFrequency = value; }
        void setMinAgitationRMS(float value) { m_minAgitationRMS = value; }
        void setCalibrationScaling1(float val) { m_calibrationScaling1 = val; }
        void setCalibrationScaling2(float val) { m_calibrationScaling2 = val; }
        void updateSectionCount(int sectionCount) {
            m_sectionCount[0] = sectionCount; // update the required sectionCount for A0[X|Y|Z] 
        }
        void updateFrequencyLimits(int lowCutFrequency, int highCutFrequency) {
            // update the frequency limits
            m_lowCutFrequency = lowCutFrequency;
            m_highCutFrequency = highCutFrequency;
        }
        
        /***** File Logging *****/
        int saveFFTCount = 20;
        bool fileLogging = false;  // toggle logging for FFT data

        int FFTComputerID = 30;
        File FFTInput[3], FFTOuput[3];
        bool isFFTOpened[3] = {false, false, false};
        char directions[4] = {'X', 'Y', 'Z', 0};
        char fftInputFile[20];
        char fftOutputFile[20];
        int fft_direction;
        bool FFTParamsSaved = false;
        enum buffer { accFFT, velFFT, velRMS, dispFFT, dispRMS };        

        void logFFTParams(File *FFTInput, uint16_t samplingRate, uint16_t blockSize, float fftResolution) {
            //Save the samplingRate of sensor and the blockSize along with fftResolution
            if(fileLogging && !FFTParamsSaved) {
                FFTInput->println("***************************");
                FFTInput->print("Sampling Rate: "); FFTInput->println(samplingRate);
                FFTInput->print("Block Size: "); FFTInput->println(blockSize);
                FFTInput->print("FFT Resolution: "); FFTInput->println(fftResolution, 4);
                FFTInput->println("***************************");
                FFTInput->flush();
                FFTParamsSaved = true;
            }
            
        } 

        void logFFTInput(File *FFTInput, q15_t *values, uint16_t sampleCount) {
            if(fileLogging && saveFFTCount > 0) {
                FFTInput->print("Timestamp -> "); FFTInput->println(millis());
                for (uint16_t i=0; i< sampleCount; ++i) {
                    FFTInput->print(float(values[i])*9.8/8192.0, 6);
                    if(i < (sampleCount-1)) FFTInput->print(",");
                }
                FFTInput->println("");
                FFTInput->println("-------------------------------------");
                FFTInput->flush();
                debugPrint("FFT: Saved INPUT for FFTComputer:" , false);  debugPrint(directions[fft_direction]);
            }            
        }

        void logFFTOutput(File *FFTOutput, buffer value_type, void *values, uint16_t sampleCount, bool flushFile) {
            if(fileLogging && saveFFTCount > 0) {
                q15_t *fft_buffer;
                float *rms_value; 

                switch(value_type) {
                    case accFFT:
                    {
                        FFTOutput->print("Acceleration FFT: Timestamp -> ");
                        fft_buffer = (q15_t*) values;                 
                        break;
                    }
                    case velFFT:
                    {
                        FFTOutput->print("Velocity FFT: Timestamp -> ");
                        fft_buffer = (q15_t*) values;                 
                        break;
                    }
                    case velRMS: 
                    {
                        FFTOutput->print("Velocity RMS: Timestamp -> ");
                        rms_value = (float*) values;                 
                        break;
                    }
                    case dispFFT:
                    {
                        FFTOutput->print("Displacement FFT: Timestamp -> ");
                        fft_buffer = (q15_t*) values;                 
                        break;
                    }
                    case dispRMS:
                    {
                        FFTOutput->print("Displacement RMS: Timestamp -> ");
                        rms_value = (float*) values;                 
                        break;
                    }                
                }
                FFTOutput->println(millis());
                for(uint16_t i=0; i<sampleCount; ++i) {
                    if(value_type == accFFT || value_type == velFFT || value_type == dispFFT)
                        FFTOutput->print(fft_buffer[i]);
                    else  // rms_value
                        FFTOutput->print(rms_value[i]);
                    if(i < (sampleCount-1))  FFTOutput->print(",");
                }
                FFTOutput->println("");
                if(flushFile) {
                    FFTOutput->println("===============================================================================");
                    FFTOutput->flush();
                } else {
                    FFTOutput->println("-------------------------------------");
                }
                debugPrint("FFT: Saved FFT output: ", false); debugPrint(value_type, false);
                debugPrint(" for FFTComputer:" , false);  debugPrint(directions[fft_direction]);
            }
        }

    protected:
        T *m_allocatedFFTSpace;
        T *velocityFFT;
        bool m_useCalibrationMethod;
        uint16_t m_lowCutFrequency;
        uint16_t m_highCutFrequency;
        float m_minAgitationRMS;
        bool m_calibrationScaling1;  // Scaling factor for 1st derivative RMS
        bool m_calibrationScaling2;  // Scaling factor for 2nd derivative RMS
        virtual void m_specializedCompute()
    {
        // File logging
        fft_direction = m_id % FFTComputerID; 

        if(fileLogging && !isFFTOpened[fft_direction]) {
            sprintf(fftInputFile, "FFTInput%c.txt", directions[fft_direction]);
            sprintf(fftOutputFile, "FFTOuput%c.txt", directions[fft_direction]);
            FFTInput[fft_direction] = DOSFS.open(fftInputFile, "w");
            FFTOuput[fft_direction] = DOSFS.open(fftOutputFile, "w");
            if (FFTInput[fft_direction] && FFTOuput[fft_direction]) {
                debugPrint("FFT: opened files: ", false); debugPrint(fftInputFile, false); debugPrint(" ", false); debugPrint(fftOutputFile);
                isFFTOpened[fft_direction] = true;
            }
        }

        // 0. Preparation
        uint16_t samplingRate = m_sources[0]->getSamplingRate();
        float resolution = m_sources[0]->getResolution();         // using resolution of 2^15 
        uint16_t sampleCount = m_sources[0]->getSectionSize() * m_sectionCount[0];
        float df = (float) samplingRate / (float) sampleCount;
        T *values = (T*) m_sources[0]->getNextValuesToCompute(this);
        for (uint8_t i = 0; i < m_destinationCount; ++i) {
            m_destinations[i]->setSamplingRate(samplingRate);
        }

        m_destinations[0]->setResolution(resolution);
        m_destinations[1]->setResolution(1);
        m_destinations[2]->setResolution(resolution);
        m_destinations[3]->setResolution(resolution);

        logFFTParams(&FFTInput[fft_direction], samplingRate, sampleCount, df);
        logFFTInput(&FFTInput[fft_direction], values, sampleCount);

        // Save the raw data 
        if(m_id == 30 && RawDataState::startRawDataCollection && !RawDataState::XCollected) {
            memcpy(RawDataState::rawAccelerationX, (q15_t*)values, FFTConfiguration::currentBlockSize * 2);
            if(loopDebugMode) {
                debugPrint("Raw data request: collected X");
            }
            RawDataState::XCollected = true;
        }
        if(m_id == 31 && RawDataState::startRawDataCollection && !RawDataState::YCollected) {
            memcpy(RawDataState::rawAccelerationY, (q15_t*)values, FFTConfiguration::currentBlockSize * 2);
            if(loopDebugMode) {
                debugPrint("Raw data request: collected Y");
            }
            RawDataState::YCollected = true;
        }
        if(m_id == 32 && RawDataState::startRawDataCollection && !RawDataState::ZCollected) {
            memcpy(RawDataState::rawAccelerationZ, (q15_t*)values, FFTConfiguration::currentBlockSize * 2);
            if(loopDebugMode) {
                debugPrint("Raw data request: collected Z");
            }
            RawDataState::ZCollected = true;
        }

        // 1. Compute FFT and get amplitudes
        uint32_t amplitudeCount = sampleCount / 2 + 1;
        T amplitudes[amplitudeCount];
        //T amplitudesCopy[amplitudeCount];
        T newAccelAmplitudes[sampleCount];
        char* fingerprintResult_X;
        char* fingerprintResult_Y;
        char* fingerprintResult_Z;
        //q15_t fingerprintResult;
        RFFT::computeRFFT(values, m_allocatedFFTSpace, sampleCount, false);
        RFFTAmplitudes::getAmplitudes(m_allocatedFFTSpace, sampleCount,
                                      amplitudes);

        logFFTOutput(&FFTOuput[fft_direction], accFFT, (void*) amplitudes, amplitudeCount, false);

        // -----------------------Start -------------------------------------------
        //Raw Data
       /* Serial.print("RAW DATA :");Serial.print("[");
        for(uint16_t i = 2; i < sampleCount ;i++){
          Serial.print(*values + i);Serial.print(",");  
        }
        Serial.println("]");
        
      //++  float newAccelMean = RFFTAmplitudes::getAccelerationMean(values,sampleCount);
      //Serial.print("New Mean:");Serial.println(newAccelMean);  
      //rmove mean from new AccclAmplitudes
      //++  RFFTAmplitudes::removeNewAccelMean(values,sampleCount,newAccelMean,newAccelAmplitudes);   // returns the newAccerationAmplitudes by removing mean form raw accel amplitudes
        
    /*   Serial.println("Accel FFT Amplitudes :");
        Serial.print("[");
        for(int i=0;i<amplitudeCount;i++){

          Serial.print(amplitudes[i]);Serial.print(",");
          
        }
        Serial.println("]");
    */
     //  float arms = RFFTAmplitudes::getRMS(amplitudes, sampleCount, true);

      // Serial.print("ARMS :");Serial.println(arms*resolution);
      // Serial.print("Resolution Value :");Serial.println(resolution,6); 
       static int direction = 0;
       static bool fingerprintSet = false; 
        
       if(direction >2){
        direction = 0;
          
       }
      // Serial.print("Axis ID :");Serial.println(direction);
    
       //Serial.println("********************** Dingerprint On Acceleration Amplitude ***************************************");     
    /*    
       if(direction == 0) {
          fingerprintResult_X =  DiagnosticEngine::m_specializedCompute (direction,amplitudes,resolution);
         // fingerprintResult = DiagnosticEngine:: m_specializedCompute(direction, speedMultiplierX*multiplierX, bandValueX,amplitudes);
         //FingerprintMessage = fingerprintResult;
       }
       if(direction ==1){
           fingerprintResult_Y = DiagnosticEngine::m_specializedCompute (direction, amplitudes,resolution);
          //fingerprintResult = DiagnosticEngine:: m_specializedCompute(direction, speedMultiplierY*multiplierY, bandValueX,amplitudes);
        //FingerprintMessage += fingerprintResult;
       }
       if(direction == 2){
          fingerprintResult_Z = DiagnosticEngine::m_specializedCompute (direction, amplitudes,resolution);
          //fingerprintResult = DiagnosticEngine:: m_specializedCompute(direction,speedMultiplierZ*multiplierZ, bandValueZ,amplitudes);
          //FingerprintMessage += fingerprintResult;
         //size_t messageSize = malloc(strlen(fingerprintResult_X)+strlen(fingerprintResult_Y)+ strlen(fingerprintResult_Z) + 1);
    
       }
     */
      // Serial.print(fingerprintResult_X);Serial.print("\t");Serial.print(fingerprintResult_Y);Serial.print("\t");
      // Serial.println(fingerprintResult_Z);
      // direction++;
        // T newamplitudesCopy[amplitudeCount];
        // float newFloatAmplitudesCopy[amplitudeCount];
        // float new_df = (float) samplingRate / (float) sampleCount;        
        // copyArray(amplitudes, newamplitudesCopy, amplitudeCount);

      /*  Serial.println("Accel FFT Amplitudes Copy :");
        Serial.print("[");
        for(int i=0;i<amplitudeCount;i++){

          Serial.print(newamplitudesCopy[i]);Serial.print(",");
            
          //newamplitudesCopy[i] = float(newamplitudesCopy[i]/32768.0);
          //newFloatAmplitudesCopy[i] = newamplitudesCopy[i];
                   
        }
        Serial.println("]");
      */
       // Serial.print("Resolution :");Serial.println(new_df);
       // Serial.println("New Amplitude after processing ...");  
       // Serial.print("[");
       
       /* Get the Velocity Amplitudes 
        * converting all the q15_t values to float 
        * applying the diagnostic fingerprints on float amplitudes
        * ScalingFactor/ Formula : float(accel amplitudes)/(2*pi*i*frequencyResolution)*sensorResolution/sqrt(2);  
        * @newFloatAmplitudesCopy  return the velocity fft amplitudes. 
        */
        
        // float q15_amplitudes;
        
        // for(int i=1;i<amplitudeCount;i++){
        //   q15_amplitudes =  (((float)newamplitudesCopy[i])/128.0*1000); 
        //   //Serial.print(q15_amplitudes);Serial.print(",");
           
        //   newFloatAmplitudesCopy[i] =  q15_amplitudes/(2*3.14*i*new_df)*resolution*(FFTConfiguration::currentBlockSize/2)/1.414 ;       // ((float(newamplitudesCopy[i])/32768.0)/(2*3.14*i*new_df))* 1000;
          
        //   //Serial.print(newFloatAmplitudesCopy[i],4);Serial.print(",");
          
        // }
          //Serial.println("]");
        
        float agitation = RFFTAmplitudes::getRMS(amplitudes, sampleCount, true);
        bool isInMotion = (agitation * resolution) > m_minAgitationRMS ;

        if (!isInMotion && featureDebugMode) {
            debugPrint(F("Device is still - Freq, vel & disp defaulted to 0."));
        }
        // 2. Keep the K max coefficients (K = reducedLength)
        uint16_t reducedLength = m_destinations[0]->getSectionSize() / 3;
        T maxVal;
        uint32_t maxIdx;
        // If board is still, freq is defaulted to 0.
        bool mainFreqSaved = false;
        float freq(0);
        if (!isInMotion) {
            m_destinations[1]->addValue(freq);
            mainFreqSaved = true;
        }
        T amplitudesCopy[amplitudeCount];
        copyArray(amplitudes, amplitudesCopy, amplitudeCount);
        for (uint16_t i = 0; i < reducedLength; ++i) {
            getMax(amplitudesCopy, amplitudeCount, &maxVal, &maxIdx);
            amplitudesCopy[maxIdx] = 0;
            m_destinations[0]->addValue((q31_t) maxIdx);
            m_destinations[0]->addValue(m_allocatedFFTSpace[2 * maxIdx]);
            m_destinations[0]->addValue(m_allocatedFFTSpace[2 * maxIdx + 1]);
            if (!mainFreqSaved) {
                freq = df * (float) maxIdx;
                if (freq > m_lowCutFrequency && freq < m_highCutFrequency) {
                    m_destinations[1]->addValue(freq);
                    mainFreqSaved = true;
                }
            }
        }
              
        if (featureDebugMode) {
            debugPrint(millis(), false);
            debugPrint(F(" -> "), false);
            debugPrint(m_destinations[0]->getName(), false);
            debugPrint(F(": computed"));
            debugPrint(millis(), false);
            debugPrint(F(" -> "), false);
            debugPrint(m_destinations[1]->getName(), false);
            debugPrint(F(": "), false);
            debugPrint(freq);
        }
        float integratedRMS1;
        if (isInMotion) {
            // 3. 1st integration in frequency domain
            T scaling1 = (T) RFFTAmplitudes::getRescalingFactorForIntegral(
                amplitudes, sampleCount, samplingRate);
            RFFTAmplitudes::filterAndIntegrate(
                amplitudes, sampleCount, samplingRate, m_lowCutFrequency,
                m_highCutFrequency, scaling1, false);

            logFFTOutput(&FFTOuput[fft_direction], velFFT,(void*) amplitudes, amplitudeCount, false);

          
            /***************************** Applying Diagnostic fingerprints on computated velocity fft amplitude *************************/ 
            //  Serial.print("Axis ID :");Serial.println(direction);
           /* Serial.println("Velocity FFT Amplitudes :");
              Serial.print("[");
                for(int i=0;i<amplitudeCount;i++){
          
                  Serial.print(amplitudes[i]);Serial.print(",");
                    
               }
              Serial.println("]"); 
           */           
    
             if(direction == 0) {
                fingerprintResult_X =  DiagnosticEngine::m_specializedCompute (direction,(const q15_t*)amplitudes,amplitudeCount,resolution,scaling1);  // resolution
                // debugPrint("X", false);debugPrint(fingerprintResult_X, true);
             }
             if(direction ==1){
                fingerprintResult_Y = DiagnosticEngine::m_specializedCompute (direction, (const q15_t*)amplitudes,amplitudeCount,resolution, scaling1);
                // debugPrint("Y", false);debugPrint(fingerprintResult_Y, true);

             }
             if(direction == 2){
                fingerprintResult_Z = DiagnosticEngine::m_specializedCompute (direction, (const q15_t*)amplitudes,amplitudeCount,resolution,scaling1);
                // debugPrint("Z", false);debugPrint(fingerprintResult_Z, true);
             }
            
            direction++; 
                
      
            if( m_useCalibrationMethod == false ) {// || UsageMode::CALIBRATION == 0) {

              //++integratedRMS1 = RFFTAmplitudes::getNewAccelRMS(newAccelAmplitudes,sampleCount);
              //Serial.print("integratedRMS1 before Curve Fit :");Serial.println(integratedRMS1);
              
             //++ integratedRMS1 = abs((integratedRMS1))*(65.4/(maxFreqIndex) + 8.70 - 0.0139 *(maxFreqIndex))* motorScalingFactor ;
             //Serial.print("integratedRMS1 After Curve Fit :");Serial.println(integratedRMS1*resolution);
                          
            }else {

              integratedRMS1 = RFFTAmplitudes::getRMS(amplitudes,sampleCount);
              integratedRMS1 *= 1000 / ((float) scaling1) * m_calibrationScaling1;  // remove base noise 0.2
              integratedRMS1 *= motorScalingFactor;
              
              //Serial.print("integratedRMS1 Original :");Serial.println(integratedRMS1*resolution);
              //Serial.print("Integrated RMS calibration Scaling:");Serial.println(m_calibrationScaling1);
            }
            
            m_destinations[2]->addValue(integratedRMS1);
            
            logFFTOutput(&FFTOuput[fft_direction], velRMS, (void*) &integratedRMS1, 1, false);

            if (featureDebugMode) {
                debugPrint(millis(), false);
                debugPrint(F(" -> "), false);
                debugPrint(m_destinations[2]->getName(), false);
                debugPrint(": ", false);
                debugPrint(integratedRMS1 * resolution);
            }
          
            //Serial.print(m_destinations[2]->getName());Serial.print("\t");Serial.println(integratedRMS1*resolution);
            
            // 4. 2nd integration in frequency domain
            T scaling2 = (T) RFFTAmplitudes::getRescalingFactorForIntegral(
                amplitudes, sampleCount, samplingRate);
            RFFTAmplitudes::filterAndIntegrate(
                amplitudes, sampleCount, samplingRate, m_lowCutFrequency,
                m_highCutFrequency, scaling2, false);
            float integratedRMS2 = RFFTAmplitudes::getRMS(amplitudes,
                                                          sampleCount);
            
            logFFTOutput(&FFTOuput[fft_direction], dispFFT, (void*) &amplitudes, amplitudeCount, false);

            integratedRMS2 *= 1000 / ((float) scaling1 * (float) scaling2) * m_calibrationScaling2;
            m_destinations[3]->addValue(integratedRMS2); 

            logFFTOutput(&FFTOuput[fft_direction], dispRMS, (void*) &integratedRMS2, 1, true);

            if (featureDebugMode) {
                debugPrint(millis(), false);
                debugPrint(F(" -> "), false);
                debugPrint(m_destinations[3]->getName(), false);
                debugPrint(": ", false);
                debugPrint(integratedRMS2 * resolution);
            }
        } else {
            m_destinations[2]->addValue(0.0);
            m_destinations[3]->addValue(0.0);
            if (featureDebugMode) {
                debugPrint(millis(), false);
                debugPrint(F(" -> "), false);
                debugPrint(m_destinations[2]->getName(), false);
                debugPrint(F(": 0"));
                debugPrint(millis(), false);
                debugPrint(F(" -> "), false);
                debugPrint(m_destinations[3]->getName(), false);
                debugPrint(F(": 0"));
            }
        }
        --saveFFTCount;
        if(saveFFTCount == 0) {
            if(fileLogging && !isFFTOpened[fft_direction]) {
                FFTInput[fft_direction].close();
                FFTOuput[fft_direction].close();
                debugPrint("FFT: closed files: ", false); debugPrint(fftInputFile, false); debugPrint(" ", false); debugPrint(fftOutputFile);
            }
        }
    }
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
        AudioDBComputer(uint8_t id, FeatureTemplate<float> *audioDB,
                        float calibrationScaling=1., float calibrationOffset=0.);
        void setCalibrationScaling(float val) { m_calibrationScaling = val; }
        void setCalibrationOffset(float val) { m_calibrationOffset = val; }
        float dBresult;
    protected:
        virtual void m_specializedCompute();
        float m_calibrationScaling;  // Scaling factor
        float m_calibrationOffset;  // Offset
};


#endif // FEATURECOMPUTER_H
