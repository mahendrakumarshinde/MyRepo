#ifndef IUFEATUREHANDLER_H
#define IUFEATUREHANDLER_H

#include "IUUtilities.h"
#include "IUFeatures.h"

/**
 * A class to hold create, configure, activate and hold features
 * In current implementation, max possible number of features is fixed to 10,
 * change static class member 'maxFeatureCount' if more is needed.
 */
class IUFeatureHandler
{
  public:
    // Arrays have to be initialized with a fixed size,
    // so we chose max_size = max number of features per collections
    static const uint8_t maxFeatureCount = 10;
    IUFeatureHandler();
    virtual ~IUFeatureHandler();
    void resetFeatures;
    bool addFeature(IUABCFeature *feature);
    IUABCFeature getFeature(uint8_t index);
    uint8_t getFeatureCount() { return m_featureCount; }

  protected:
    IUABCFeature *m_features[MAX_SIZE];
    uint8_t m_featureCount; // Dynamic

};

#endif // IUFEATUREHANDLER_H





/* ===================================== Feature Instanciations ======================================== */
/*
extern IUMultiQ15SourceDataCollectionFeature showRecordFFT;
extern IUMultiQ15SourceFeature accelerationEnergy;
extern IUMultiQ15SourceFeature velocityX;
extern IUMultiQ15SourceFeature velocityY;
extern IUMultiQ15SourceFeature velocityZ;
extern IUSingleFloatSourceFeature temperature;
extern IUSingleFloatSourceFeature audioDB;
*/






/* ===================================== Feature Instanciations ======================================== */

/*
uint16_t accelSourceSize[3] = {512, 512, 512};
uint16_t velocitySourceSize[3] = {1, 1, 512};

showRecordFFT = IUMultiQ15SourceDataCollectionFeature(0, "accel_data");
showRecordFFT.prepareSource(3, accelSourceSize);
showRecordFFT.setDefaultDataTransform();
showRecordFFT.activate();

accelerationEnergy = IUMultiQ15SourceFeature(1, "acceleration_energy");
accelerationEnergy.prepareSource(3, accelSourceSize);
accelerationEnergy.prepareSendingQueue(1);
accelerationEnergy.setComputeFunction(computeSignalEnergy);
accelerationEnergy.activate();

velocityX = IUMultiQ15SourceFeature(2, "velocity_x");
VelocityX.prepareSource(3, velocitySourceSize);
VelocityX.prepareSendingQueue(1);
VelocityX.setComputeFunction(computeVelocity);
VelocityX.activate();

velocityY = IUMultiQ15SourceFeature(3, "velocity_y");
velocityY.prepareSource(3, velocitySourceSize);
velocityY.prepareSendingQueue(1);
velocityY.setComputeFunction(computeVelocity);
velocityY.activate();

velocityZ = IUMultiQ15SourceFeature(4, "velocity_z");
velocityZ.prepareSource(3, velocitySourceSize);
velocityZ.prepareSendingQueue(1);
velocityZ.setComputeFunction(computeVelocity);
velocityZ.activate();

temperature = IUSingleFloatSourceFeature(5, "current_temperature");
temperature.prepareSource(1);
temperature.prepareSendingQueue(1);
temperature.setDefaultComputeFunction();
temperature.activate();

audioDB = IUSingleFloatSourceFeature(6, "audio_db");
audioDB.prepareSource(1);
audioDB.prepareSendingQueue(1);
audioDB.setDefaultComputeFunction();
audioDB.activate();
*/











