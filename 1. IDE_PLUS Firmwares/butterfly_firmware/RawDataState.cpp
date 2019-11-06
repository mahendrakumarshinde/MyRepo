#include "RawDataState.h"

namespace RawDataState {
    bool startRawDataCollection = false;
    bool XCollected = false;
    bool YCollected = false;
    bool ZCollected = false;
    bool startRawDataTransmission = false;
    bool rawDataTransmissionInProgress = false;
    char rawAccelerationX[15000];
    char rawAccelerationY[15000];
    char rawAccelerationZ[15000];
}

namespace FeatureStates {

    bool isFeatureStreamComplete = false;
    bool isISRActive = false;
    bool isISRDisabled = false;
    int isrCount =0;


}