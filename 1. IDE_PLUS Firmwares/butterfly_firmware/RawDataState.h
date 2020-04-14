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
    extern char rawAccelerationX[8208];
    extern char rawAccelerationY[8208];
    extern char rawAccelerationZ[16400];
}

namespace FeatureStates {

    extern bool isFeatureStreamComplete;
    extern bool isISRActive;
    extern bool isISRDisabled;
    extern int isrCount; 
}

#endif // RAW_DATA_STATE_H
