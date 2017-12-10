#ifndef TEST_FEATUREGROUP_H_INCLUDED
#define TEST_FEATUREGROUP_H_INCLUDED


#include <ArduinoUnit.h>
#include "../FeatureGroup.h"


test(FeatureGroup__test)
{
    FeatureGroup group("PROF01", 500);

    /***** Values after initialization *****/
    assertTrue(group.isNamed("PROF01"));
    assertEqual(group.getFeatureCount(), 0);
    assertFalse(group.isActive());

    /***** Add features *****/
    Feature feature1("001", 2, 1);
    Feature feature2("002", 2, 1);
    group.addFeature(&feature1);
    group.addFeature(&feature2);
    assertEqual(group.getFeatureCount(), 2);
    assertTrue(group.getFeature(0)->isNamed("001"));
    assertTrue(group.getFeature(1)->isNamed("002"));

    /***** Data send Period *****/
    assertTrue(group.isDataSendTime());
    // 2nd successive call should be false
    assertFalse(group.isDataSendTime());
    delay(600);
    assertTrue(group.isDataSendTime());

    /***** Data send Period *****/
//    group.stream(&Serial, OperationState::IDLE, 0);
}

#endif // TEST_FEATUREGROUP_H_INCLUDED
