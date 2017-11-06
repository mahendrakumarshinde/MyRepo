#ifndef TEST_FEATURECLASS_H_INCLUDED
#define TEST_FEATURECLASS_H_INCLUDED


#include <ArduinoUnit.h>
#include "../FeatureClass.h"


/**
 * Test Feature attributes after instanciation
 */
test(FeatureClass__initialization)
{
    /* Feature(const char* name, uint8_t sectionCount=2,
               uint16_t sectionSize=1, slideOption sliding=FIXED) */
    Feature feature("TST", 2, 10);
    feature.addReceiver(1);
    feature.addReceiver(2);

    assertTrue(feature.isNamed("TST"));
    assertEqual(feature.getReceiverId(0), 1);
    assertEqual(feature.getReceiverId(1), 2);
    assertEqual(feature.getSectionSize(), 10);

    assertFalse(feature.isScalar());
    // Also test when isScalar should be true.
    assertTrue(Feature("ABC, 2, 1").isScalar());

    assertEqual(feature.getResolution(), 1);
    assertFalse(feature.isStreaming());
    assertFalse(feature.opStateEnabled());
    assertFalse(feature.isActive());
    assertEqual(feature.getOperationState(), OperationState::IDLE);
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
    feature.addReceiver(1);
    feature.addReceiver(2);

    // At initialization
    assertTrue(feature.isReadyToRecord());
    assertTrue(feature.isReadyToRecord(2));
    assertFalse(feature.isReadyToCompute(0));
    assertFalse(feature.isReadyToCompute(0, 2));
    assertFalse(feature.isReadyToCompute(1));
    assertFalse(feature.isReadyToCompute(1, 2));

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
    FloatFeature floatFeature ("FLT", 2, 10, floatValues);
    q15_t q15_tValues[20];
    Q15Feature q15Feature("Q15", 2, 10, q15_tValues);
    q31_t q31_tValues[20];
    Q31Feature q31Feature("Q31", 2, 10, q31_tValues);
    floatFeature.addReceiver(1);
    q15Feature.addReceiver(1);
    q31Feature.addReceiver(1);


    // Check that adding value increment the filling index:
    for (uint8_t i = 0; i < 10; ++i)
    {
        floatFeature.addFloatValue(0.1 + (float) i);
        q15Feature.addQ15Value(10 + i);
        q31Feature.addQ31Value(20 + i);
    }
    // Float
    assertTrue(floatFeature.isReadyToCompute(0));  // Compute 1st section
    assertTrue(floatFeature.isReadyToRecord());  // Record in 2nd section
    assertFalse(floatFeature.isReadyToRecord(2));  // 2nd section ready to
    // record, but not the one after (which is actually the 1st section)
    // Get next values should return a pointer to the next section to compute
    assertEqual(floatValues[0], 0.1);
    assertEqual(floatFeature.getNextFloatValues(0)[0], 0.1);
    assertEqual(floatValues[1], 1.1);
    assertEqual(floatFeature.getNextFloatValues(0)[1], 1.1);
    // Q15
    assertTrue(q15Feature.isReadyToCompute(0));  // Compute 1st section
    assertTrue(q15Feature.isReadyToRecord());  // Record in 2nd section
    assertFalse(q15Feature.isReadyToRecord(2));  // 2nd section ready to record,
    // but not the one after (which is actually the 1st section)
    // Get next values should return a pointer to the next section to compute
    assertEqual(q15Feature.getNextQ15Values(0)[0], 10);
    assertEqual(q15Feature.getNextQ15Values(0)[1], 11);
    // Q31
    assertTrue(q31Feature.isReadyToCompute(0));  // Compute 1st section
    assertTrue(q31Feature.isReadyToRecord());  // Record in 2nd section
    assertFalse(q31Feature.isReadyToRecord(2));  // 2nd section ready to record,
    // but not the one after (which is actually the 1st section)
    // Get next values should return a pointer to the next section to compute
    assertEqual(q31Feature.getNextQ31Values(0)[0], 20);
    assertEqual(q31Feature.getNextQ31Values(0)[4], 24);

    // Value to compare to threshold should be the mean of last fully recorded
    // section
    float value = floatFeature.getValueToCompareToThresholds();
    assertEqual(round(10 * value), 46);
}


/**
 * Test functions specific to Feature subclasses (number format specific)
 */
test(FeatureClass__FloatFeatureOperationState)
{
    /***** Scalar feature (section size = 1) *****/
    float scalarValues[4];
    FloatFeature scalarFeature("SCL", 4, 1, scalarValues);
    // Initial values
    assertFalse(scalarFeature.opStateEnabled());
    assertEqual(scalarFeature.getOperationState(), OperationState::IDLE);
    for (uint8_t i = 0; i < OperationState::COUNT - 1; i++)
    {
        assertEqual(scalarFeature.getThreshold(i), 0);
    }
    // Update thresholds
    scalarFeature.enableOperationState();
    scalarFeature.setThresholds(1.1, 2.2, 3.3);
    assertTrue(scalarFeature.opStateEnabled());
    assertEqual(round(scalarFeature.getThreshold(0) * 10), 11);
    assertEqual(round(scalarFeature.getThreshold(1) * 10), 22);
    assertEqual(round(scalarFeature.getThreshold(2) * 10), 33);
    // update Operation State
    scalarFeature.addFloatValue(0.5);
    assertEqual(round(scalarFeature.getValueToCompareToThresholds() * 10), 5);
    assertEqual(scalarFeature.getOperationState(), OperationState::IDLE);
    scalarFeature.addFloatValue(1.5);
    assertEqual(round(scalarFeature.getValueToCompareToThresholds() * 10), 15);
    assertEqual(scalarFeature.getOperationState(), OperationState::NORMAL);
    scalarFeature.addFloatValue(2.5);
    assertEqual(round(scalarFeature.getValueToCompareToThresholds() * 10), 25);
    assertEqual(scalarFeature.getOperationState(), OperationState::WARNING);
    scalarFeature.addFloatValue(3.5);
    assertEqual(round(scalarFeature.getValueToCompareToThresholds() * 10), 35);
    assertEqual(scalarFeature.getOperationState(), OperationState::DANGER);

    /***** Array feature (section size > 1) *****/
    float arrayValues[40];
    FloatFeature arrayFeature("ARR", 4, 10, arrayValues);
    // Initial values
    assertFalse(arrayFeature.opStateEnabled());
    assertEqual(arrayFeature.getOperationState(), OperationState::IDLE);
    for (uint8_t i = 0; i < OperationState::COUNT - 1; i++)
    {
        assertEqual(arrayFeature.getThreshold(i), 0);
    }
    // Update thresholds
    arrayFeature.enableOperationState();
    arrayFeature.setThresholds(2.5, 3.0, 3.5);
    assertTrue(arrayFeature.opStateEnabled());
    assertEqual(round(arrayFeature.getThreshold(0) * 10), 25);
    assertEqual(round(arrayFeature.getThreshold(1) * 10), 30);
    assertEqual(round(arrayFeature.getThreshold(2) * 10), 35);
    // update Operation State
    for (float i = 0; i < 10; ++i)
    {
        arrayFeature.addFloatValue(0.5 * i);
    }
    assertEqual(round(arrayFeature.getValueToCompareToThresholds() * 100), 225);
    assertEqual(arrayFeature.getOperationState(), OperationState::IDLE);
    for (float i = 0; i < 10; ++i)
    {
        arrayFeature.addFloatValue(0.5 * (i + 1));
    }
    assertEqual(round(arrayFeature.getValueToCompareToThresholds() * 100), 275);
    assertEqual(arrayFeature.getOperationState(), OperationState::NORMAL);
    for (float i = 0; i < 10; ++i)
    {
        arrayFeature.addFloatValue(0.5 * (i + 2));
    }
    assertEqual(round(arrayFeature.getValueToCompareToThresholds() * 100), 325);
    assertEqual(arrayFeature.getOperationState(), OperationState::WARNING);
    for (float i = 0; i < 10; ++i)
    {
        arrayFeature.addFloatValue(0.5 * (i + 3));
    }
    assertEqual(round(arrayFeature.getValueToCompareToThresholds() * 100), 375);
    assertEqual(arrayFeature.getOperationState(), OperationState::DANGER);
}

#endif // TEST_FEATURECLASS_H_INCLUDED
