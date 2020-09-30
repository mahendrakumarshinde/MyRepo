#ifndef ADVANCE_FEATURE_COMPUTATION_H
#define ADVANCE_FEATURE_COMPUTATION_H
/**
 * @file AdvanceFeatureComputer.h
 * @author Sidramayya Swami
 * @brief This library will use for compute the phase difference between two axis.
 * @version 0.1
 * @date 2020-09-25
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "Arduino.h"
#include "ArduinoJson.h"
#include "FS.h"
#include <IUSerial.h>
#include "arm_math.h"

 struct phase
{
  float rel;
  float img;
};

#define RAD_TO_DEGREE 180/PI;

class AdvanceFeatureComputer
{
   public :
       //char* getAdvanceFeatureName(){ return m_advanceFeatureName;}              //  -return feature name
       //float* getAdvanceFeatureValue() {return m_advanceFeatureValue;}     //- return value based on index
       //getAdvanceFeatureCount();  //will return total size of buffer or return number of element filled 
       //bool addAdvanceFeatureName(const char* advanceFeature);               //- save/insert feature name
       //bool addAdvanceFeatureValue(float advanceFeatureValue[]); //-save/insert feature value
       //uint8_t m_advanceFeatureCount;
       //static const uint8_t max_advanceFeature_count = 20;
       //const char* m_advanceFeatureName[max_advanceFeature_count];
       //float m_advanceFeatureValue[max_advanceFeature_count];
   protected:




};

class PhaseAngleComputer : public AdvanceFeatureComputer
{
    public:
        enum quadrantOption : uint8_t {QUADRANT_1 = 1,
                                     QUADRANT_2,
                                     QUADRANT_3,
                                     QUADRANT_4,
                                     POSITIVE_IMG,
                                     NEGATIVE_IMG,
                                     POSITIVE_REAL,
                                     NEGATIVE_REAL
                                  };
        void getPhaseAngle();
        uint16_t getFFTIndex(int inputFrequency ,float resolution);
        float computePhaseDiff( char axis1 , char axis2);
        uint8_t getQuadrant(float img, float real);
        float getAngle(float signalImg,float signalrel, uint8_t quadrant );
        uint16_t get_index(); //{return m_freq_index;}
        void getComplexData( q15_t realdata, q15_t imgdata,int m_id);
        
        int freq_index ;
        float phase_difference;
        float phase;
        static const uint8_t total_max_IDs = 10;   //max number of featurename in configs
        float phase_output[total_max_IDs];
        size_t totalPhaseIds;   
    protected :
    
    
};
#endif
