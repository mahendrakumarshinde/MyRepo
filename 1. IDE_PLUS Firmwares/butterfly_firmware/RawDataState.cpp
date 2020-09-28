#include "RawDataState.h"

namespace RawDataState {
    bool startRawDataCollection = false;
    bool XCollected = false;
    bool YCollected = false;
    bool ZCollected = false;
    bool startRawDataTransmission = false;
    bool rawDataTransmissionInProgress = false;
    char rawAccelerationX[16400];
    char rawAccelerationY[16400];
    char rawAccelerationZ[16400];
}

namespace FeatureStates {

    bool isFeatureStreamComplete = false;
    bool isISRActive = false;
    bool isISRDisabled = false;
    int isrCount =0;


}

namespace featureDestinations {
    float buff[basicfeatures::COUNT];
}