#include "RawDataState.h"

namespace RawDataState {
    bool startRawDataCollection = false;
    bool XCollected = false;
    bool YCollected = false;
    bool ZCollected = false;
    bool startRawDataTransmission = false;
    bool rawDataTransmissionInProgress = false;
    char rawAccelerationX[8208];
    char rawAccelerationY[8208];
    char rawAccelerationZ[8208];
}

namespace FeatureStates {

    bool isFeatureStreamComplete = false;
    bool isISRActive = false;
    bool isISRDisabled = false;
    int isrCount =0;


}