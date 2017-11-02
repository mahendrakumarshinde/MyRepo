#ifndef TEST_FEATUREPROFILE_H_INCLUDED
#define TEST_FEATUREPROFILE_H_INCLUDED


#include <ArduinoUnit.h>
#include "../FeatureProfile.h"


test(FeatureProfile__test)
{
    FeatureProfile profile("PROF01", 500);

    /***** Values after initialization *****/
    assertTrue(profile.isNamed("PROF01"));
    assertEqual(profile.getFeatureCount(), 0);
    assertFalse(profile.isActive());

    /***** Add features *****/
    Feature feature1("001", 2, 1);
    Feature feature2("002", 2, 1);
    profile.addFeature(&feature1);
    profile.addFeature(&feature2);
    assertEqual(profile.getFeatureCount(), 2);
    assertTrue(profile.getFeature(0)->isNamed("001"));
    assertTrue(profile.getFeature(1)->isNamed("002"));

    /***** Data send Period *****/
//    assertTrue(profile.isDataSendTime());
//    // 2nd successive call should be false
//    assertFalse(profile.isDataSendTime());
//    delay(500);
//    assertTrue(profile.isDataSendTime());

    /***** Data send Period *****/
//    profile.stream(&Serial, OperationState::IDLE, 0);
}

#endif // TEST_FEATUREPROFILE_H_INCLUDED
