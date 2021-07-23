/*
    The file tracks the state of raw data request
 */

#ifndef RAW_DATA_STATE_H
#define RAW_DATA_STATE_H

namespace RawDataState {
    extern bool startRawDataCollection;
    extern bool XCollected;
    extern bool YCollected;
    extern bool ZCollected;
    extern bool startRawDataTransmission;
    extern bool rawDataTransmissionInProgress;
    // Raw data storage buffers. TODO: This is temporary for v1.1.3,
    // sizes of these buffers should be optimized in later releases and 
    // the extra freed up space should be used to create dedicated raw data buffers
    // of datatype q15_t.
    extern char rawAccelerationX[16400];
    extern char rawAccelerationY[16400];
    extern char rawAccelerationZ[16400];
}

namespace FeatureStates {

    extern bool isFeatureStreamComplete;
    extern bool isISRActive;
    extern bool isISRDisabled;
    extern double isr_startTime;
    extern double isr_stopTime;
    extern double elapsedTime;
    extern int isrCount;
    extern int opStateStatusFlag; 
    extern int m_currentStreamingMode;
}

namespace featureDestinations {

    enum basicfeatures : int {accelRMS512Total     = 0,
                                  velRMS512X    = 1,
                                  velRMS512Y    = 2,
                                  velRMS512Z    = 3,
                                  temperature   = 4,
                                  audio         = 5,
                                  accelRMS512X  = 6,
                                  accelRMS512Y  = 7,
                                  accelRMS512Z  = 8,
                                  dispRMS512X   = 9,
                                  dispRMS512Y   = 10,
                                  dispRMS512Z   = 11,
                                  rpm           =12,
                                  COUNT         = 13};
    extern float buff[basicfeatures::COUNT];
}
#endif // RAW_DATA_STATE_H
