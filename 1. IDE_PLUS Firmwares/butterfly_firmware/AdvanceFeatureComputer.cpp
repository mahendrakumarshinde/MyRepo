#include "AdvanceFeatureComputer.h"
#include "FeatureComputer.h"
#include "Conductor.h"


extern Conductor conductor;

/**
 * @brief phase struct is for the storing the real and imag data
 * 
 */
struct phase axisX;
struct phase axisY;
struct phase axisZ;
struct phase Axis1;
struct phase Axis2;
uint16_t m_freq_index;

/**
 * @brief The getFFTIndex() function will return the index.Based on this index the FFT coefficient(complex data) will copy  
 * 
 * @param inputFrequency - RPM 
 * @param resolution - SamplingRate/BlockSize
 * @return uint16_t - index for the fft coefficient
 */

uint16_t PhaseAngleComputer::getFFTIndex(int inputFrequency ,float resolution){
    float freq_Index = inputFrequency/resolution;              //if (freq  % resolution < (resolution/2)) else ((freq // resolution) + 1) ???

  freq_index = floor(freq_Index);
  if(freq_index % 2 == 0)
        freq_index = freq_index;
    else
        freq_index = freq_index -1 ;

   m_freq_index =freq_index ;
   debugPrint("FFT_index : ");
    debugPrint(m_freq_index);
   return m_freq_index;
}
/**
 * @brief This function will return the calculated index for the fft coefficient
 * 
 * @return uint16_t - index for the fft coefficient
 */
uint16_t PhaseAngleComputer::get_index(){

    return m_freq_index;
}
/**
 * @brief This function will copy the complex data from the fft buffer into the respective axis buffer.
 * Input data is in q15_t format. before storing the data it will convert into q15ToFloat.
 * 
 * @param realdata - real part of the fft data (index)
 * @param imgdata - imag part of the fft data(index +1)
 * @param m_Id - Based on the fft computation is done the m_Id is passed 
 */
void PhaseAngleComputer::getComplexData( q15_t realdata, q15_t imgdata,int m_Id){

    float m_realdata = q15ToFloat(realdata);
    float m_imgdata = q15ToFloat(imgdata);
    if(m_Id == 30){
        axisX.rel = m_realdata;
        axisX.img = m_imgdata;
    }
    else if(m_Id == 31){
        axisY.rel = m_realdata;
        axisY.img = m_imgdata;
    }
    else if(m_Id == 32){
        axisZ.rel = m_realdata;
        axisZ.img = m_imgdata;
    }
}

/**
 * @brief - This function will return the quadrant of the signal.
 * the quadrant is depend on the real and imag data present.
 * If signal contains the both value are 0 then the function will return 0.
 * 
 * @param img - imag data input
 * @param real - real data input
 * @return uint8_t - return the uint8_t 
 */
uint8_t PhaseAngleComputer::getQuadrant(float img, float real){

  if (real > 0.0 && img > 0.0){
        return QUADRANT_1;
  } else if (real < 0.0 && img >= 0.0) {
        return QUADRANT_2;
  }else if (real < 0.0 && img < 0.0) {
        return QUADRANT_3;
  }else if (real > 0.0 && img < 0.0) {
        return QUADRANT_4;
  }else if(real ==0.0 && img > 0.0 ){            //+90 (pi/2)
        return POSITIVE_IMG;
  }else if(real ==0.0 && img < 0.0){              // -90(3pi/2)
        return NEGATIVE_IMG;
  }else if (real > 0.0 && img ==0 ){              // 0 (2pi)
        return POSITIVE_REAL;
  }else if(real < 0.0 && img ==0 ){               //180 (pi)
        return NEGATIVE_REAL;
  }else {debugPrint("signal lies on the origin "); return 0; }
}

/**
 * @brief The function will get the complex data from the given two axis.
 *  Complex Division of two axis data and based on the results of complex division check for the quadrant
 *  rel_component =(((s1.rel)*(s2.rel))+((s1.img)*(s2.img)))/(pow(s2.rel,2)+pow(s2.img,2));
 * Img_component =(((s2.rel)*(s1.img))-((s1.rel)*(s2.img)))/(pow(s2.rel,2)+pow(s2.img,2));   //s1-Axis1, s2-Axis2
 * @param axis1 - 1st input axis  
 * @param axis2 - 2nd input axis
 * @return float - return the phase difference between two axis(i.e axis1 and axis2).
 */

float PhaseAngleComputer::computePhaseDiff(char axis1 ,char axis2){
 
    if(axis1 =='X'){
        Axis1.rel =axisX.rel;
        Axis1.img = axisX.img;
    }else if(axis1 =='Y'){
        Axis1.rel = axisY.rel;
        Axis1.img = axisY.img;
    }else if(axis1 =='Z'){
        Axis1.rel = axisZ.rel;
        Axis1.img = axisZ.img;
    }
     if(axis2 =='X'){
        Axis2.rel =axisX.rel;
        Axis2.img = axisX.img;
    }else if(axis2 =='Y'){
        Axis2.rel = axisY.rel;
        Axis2.img = axisY.img;
    }else if(axis2 =='Z'){
        Axis2.rel = axisZ.rel;
        Axis2.img = axisZ.img;
    }
    debugPrint("axis 1: ",false);
    debugPrint(axis1);
    debugPrint("axis 2: ",false);
    debugPrint(axis2);
    float rel_component = (((Axis1.rel)*(Axis2.rel))+((Axis1.img)*(Axis2.img)))/(pow(Axis2.rel,2)+pow(Axis2.img,2));
    float Img_component = (((Axis2.rel)*(Axis1.img))-((Axis1.rel)*(Axis2.img)))/(pow(Axis2.rel,2)+pow(Axis2.img,2));
    debugPrint("Division of complex : ",false );
    debugPrint(rel_component ,false);
    debugPrint(" i",false);
    debugPrint(Img_component);
    uint8_t Quadrant = getQuadrant(Img_component,rel_component);
    phase_difference = getAngle(Img_component,rel_component, Quadrant);
 
    return phase_difference;
}

/**
 * @brief - It will do the actual computation of the phase.
 * compute the tan inverse of imag/real data. and based on the input quadrant the angle will adjust between 0 to 180 deg.
 * 
 * @param signalImg - imag data input
 * @param signalrel - real data input
 * @param quadrant - signal lies on the quadrant
 * @return float - return the computed phase angle.
 */

float PhaseAngleComputer::getAngle(float signalImg,float signalrel, uint8_t quadrant ){
    
    switch (quadrant)
    {
    case QUADRANT_1 :
        phase = (atan(signalImg/signalrel))*RAD_TO_DEGREE;
        break;
    case QUADRANT_2 :
        phase = (atan(signalImg/signalrel))*RAD_TO_DEGREE;
        phase = 180 + phase;
        break;
    case QUADRANT_3 :
        phase = (atan(signalImg/signalrel))*RAD_TO_DEGREE;
        phase = -180 + phase;
        break;
    case QUADRANT_4 :
        phase = (atan(signalImg/signalrel))*RAD_TO_DEGREE;
        break;
    case POSITIVE_IMG :
        phase = 90.0;
        break;
    case NEGATIVE_IMG :
        phase = -90.0;
        break;
    case POSITIVE_REAL :
        phase = 0.0;
        break;
    case NEGATIVE_REAL :
        phase = 180.0;
        break;
    default:
        break;
    }

    return phase;

}