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
       //getAdvanceFeatureName();  -return feature name
       //getAdvanceFeatureValue(); - return value based on index
       //getAdvanceFeatureCount();  //will return total size of buffer or return number of element filled 
       //addAdvanceFeatureName(); - save/insert feature name
       //addAdvanceFeatureValue(); -save/insert feature value
       static const uint8_t max_advanceFeature_count = 20;
       char* AdvanceFeatureName[max_advanceFeature_count];
       float AdvanceFeatureValue[max_advanceFeature_count];
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
    protected :
    
    
};
#endif
