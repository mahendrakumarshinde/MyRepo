#ifndef TEST_FEATURECLASS_H_INCLUDED
#define TEST_FEATURECLASS_H_INCLUDED


#include <ArduinoUnit.h>
#include "../FeatureClass.h"
#include "../FeatureComputer.h"


/**
 * Test Feature attributes after instanciation
 */
test(FeatureClass__initialization)
{
    uint8_t initialFeatureCount = Feature::instanceCount;
    Feature feature("TST", 2, 10);
    FeatureComputer comp1(1);
    FeatureComputer comp2(2);
    feature.addReceiver(comp1);
    feature.addReceiver(comp2);

    assertTrue(feature.isNamed("TST"));
    assertEqual(feature.getReceiverCount(), 2);
    assertEqual(feature.getReceiverIndex(comp1), 0);
    assertEqual(feature.getReceiverIndex(comp2), 1);
    assertEqual(feature.getSectionSize(), 10);

    // Instance registration
    assertEqual(Feature::instanceCount - initialFeatureCount, 1);
    Feature *foundFeature = Feature::getInstanceByName("TST");
    assertTrue(foundFeature != NULL);
    assertTrue(foundFeature->isNamed("TST"));

    assertEqual(feature.getResolution(), 1);
    assertFalse(feature.isRequired());
    assertFalse(feature.isComputedFeature());
}


/**
 * Test how Feature buffer state changes to track recording / computation
 */
test(FeatureClass__buffer_state_tracking)
{
    /***** Feature without receivers *****/
    Feature feature0("001", 2, 10);
    feature0.isReadyToRecord();
    for (uint8_t i = 0; i < 10; ++i)
    {
        feature0.incrementFillingIndex();
    }
    assertTrue(feature0.isReadyToRecord());
    for (uint8_t i = 0; i < 10; ++i)
    {
        feature0.incrementFillingIndex();
    }
    assertTrue(feature0.isReadyToRecord());
    for (uint8_t i = 0; i < 10; ++i)
    {
        feature0.incrementFillingIndex();
    }
    assertTrue(feature0.isReadyToRecord());


    /***** Feature with receivers *****/
    Feature feature("TST", 2, 10);
    FeatureComputer comp1(1);
    FeatureComputer comp2(2);
    feature.addReceiver(comp1);
    feature.addReceiver(comp2);

    // At initialization
    assertTrue(feature.isReadyToRecord());
    assertTrue(feature.isReadyToRecord(2));
    assertFalse(feature.isReadyToCompute(0));
    assertFalse(feature.isReadyToCompute(0, 2));
    assertFalse(feature.isReadyToCompute(comp1));
    assertFalse(feature.isReadyToCompute(comp1, 2));
    assertFalse(feature.isReadyToCompute(1));
    assertFalse(feature.isReadyToCompute(1, 2));
    assertFalse(feature.isReadyToCompute(comp2));
    assertFalse(feature.isReadyToCompute(comp2, 2));

    // Increment index (current section is 0)
    for (uint8_t i = 0; i < 9; ++i)
    {
        feature.incrementFillingIndex();
    }
    assertFalse(feature.isReadyToCompute(0));
    assertFalse(feature.isReadyToCompute(1));

    // Increment index and publish 1st section
    feature.incrementFillingIndex();
    assertTrue(feature.isReadyToCompute(0));  // Compute 1st section
    assertTrue(feature.isReadyToCompute(1));  // Compute 1st section
    assertTrue(feature.isReadyToRecord());  // Record in 2nd section
    assertFalse(feature.isReadyToRecord(2));  // 2nd section ready to record,
    // but not the one after (which is actually the 1st section)

    // Acknowledge 1st section for 1st receiver
    feature.acknowledge(0);  // Acknowledge for 1st receiver
    assertFalse(feature.isReadyToCompute(0));  // Compute 2nd section not ready
    // for 1st receiver
    assertTrue(feature.isReadyToCompute(1));   // Compute 1st section ready for
    // 2nd receiver
    assertFalse(feature.isReadyToRecord(2));  // 2nd section ready to record,
    // but not the one after (which is actually the 1st section)

    // Acknowledge 1st section for 2nd receiver
    feature.acknowledge(1);  // Acknowledge for 2nd receiver
    assertFalse(feature.isReadyToCompute(0));  // Compute 2nd section not ready
    // for 1st receiver
    assertFalse(feature.isReadyToCompute(1));   // Compute 1st section not ready
    // for 2nd receiver
    assertTrue(feature.isReadyToRecord(2));  // 2nd and 1st sections ready to
    // record, since 1st section has been fully acknowledged


    // Fill next 2 sections at once (first 2nd section then 1st section)
    for (uint8_t i = 0; i < 20; ++i)
    {
        feature.incrementFillingIndex();
    }
    // No section available for recording
    assertFalse(feature.isReadyToRecord());
    // Both receivers can compute using up to 2 sections
    assertTrue(feature.isReadyToCompute(0, 2));
    assertTrue(feature.isReadyToCompute(1, 2));

    // Acknowledge the 2 sections at once
    feature.acknowledge(0, 2);  // Acknowledge for 1st receiver
    feature.acknowledge(1, 2);  // Acknowledge for 1st receiver
    assertTrue(feature.isReadyToRecord());

    // Reset everything to initial state
    feature.reset();
    assertTrue(feature.isReadyToRecord());
    assertTrue(feature.isReadyToRecord(2));
    assertFalse(feature.isReadyToCompute(0));
    assertFalse(feature.isReadyToCompute(0, 2));
    assertFalse(feature.isReadyToCompute(1));
    assertFalse(feature.isReadyToCompute(1, 2));
}


/**
 * Test functions specific to Feature subclasses (number format specific)
 */
test(FeatureClass__number_format_features)
{
    float floatValues[20];
    FeatureTemplate<float> floatFeature ("FLT", 2, 10, floatValues);
    q15_t q15_tValues[20];
    FeatureTemplate<q15_t> q15Feature("Q15", 2, 10, q15_tValues);
    q31_t q31_tValues[20];
    FeatureTemplate<q31_t> q31Feature("Q31", 2, 10, q31_tValues);
    FeatureComputer comp1(1);
    floatFeature.addReceiver(comp1);
    q15Feature.addReceiver(comp1);
    q31Feature.addReceiver(comp1);


    // Check that adding value increment the filling index:
    for (uint8_t i = 0; i < 10; ++i)
    {
        floatFeature.addValue(0.1 + (float) i);
        q15Feature.addValue((q15_t) (10 + i));
        q31Feature.addValue((q31_t) (20 + i));
    }
    // Float
    assertTrue(floatFeature.isReadyToCompute(0));  // Compute 1st section
    assertTrue(floatFeature.isReadyToRecord());  // Record in 2nd section
    assertFalse(floatFeature.isReadyToRecord(2));  // 2nd section ready to
    // record, but not the one after (which is actually the 1st section)
    // Get next values should return a pointer to the next section to compute
    assertEqual(floatValues[0], 0.1);
    assertEqual(floatFeature.getNextFloatValuesToCompute(0)[0], 0.1);
    assertEqual(floatValues[1], 1.1);
    assertEqual(floatFeature.getNextFloatValuesToCompute(0)[1], 1.1);
    // Q15
    assertTrue(q15Feature.isReadyToCompute(0));  // Compute 1st section
    assertTrue(q15Feature.isReadyToRecord());  // Record in 2nd section
    assertFalse(q15Feature.isReadyToRecord(2));  // 2nd section ready to record,
    // but not the one after (which is actually the 1st section)
    // Get next values should return a pointer to the next section to compute
    assertEqual(q15Feature.getNextQ15ValuesToCompute(0)[0], 10);
    assertEqual(q15Feature.getNextQ15ValuesToCompute(0)[1], 11);
    // Q31
    assertTrue(q31Feature.isReadyToCompute(0));  // Compute 1st section
    assertTrue(q31Feature.isReadyToRecord());  // Record in 2nd section
    assertFalse(q31Feature.isReadyToRecord(2));  // 2nd section ready to record,
    // but not the one after (which is actually the 1st section)
    // Get next values should return a pointer to the next section to compute
    assertEqual(q31Feature.getNextQ31ValuesToCompute(0)[0], 20);
    assertEqual(q31Feature.getNextQ31ValuesToCompute(0)[4], 24);

    // Value to compare to threshold should be the mean of last fully recorded
    // section
    float value = floatFeature.getValueToCompareToThresholds();
    assertEqual(round(10 * value), 46);
}


#endif // TEST_FEATURECLASS_H_INCLUDED
