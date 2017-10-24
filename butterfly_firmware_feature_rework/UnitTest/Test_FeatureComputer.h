#ifndef TEST_FEATURECOMPUTER_H_INCLUDED
#define TEST_FEATURECOMPUTER_H_INCLUDED

#include <ArduinoUnit.h>
#include "../Utilities.h"
#include "../FeatureComputer.h"
#include "UnitTestData.h"

/**
 * Test FeatureComputer attribute after instanciation
 */
test(FeatureComputer__instanciation)
{
    Feature source("SRC", 8, 10);
    Feature destination("DST", 2, 1);
//    FeatureComputer(uint8_t id, uint8_t destinationCount=0,
//                    Feature *destination0=NULL)
    FeatureComputer computer(1, 1, &destination);
    // Add source, with computation over 2 sections at a time
    computer.addSource(&source, 2);

    assertEqual(computer.getId(), 1);
    assertFalse(computer.isActive());
    assertEqual(computer.getSourceCount(), 1);
    assertEqual(computer.getSource(0)->getName(), source.getName());
    assertEqual(computer.getDestinationCount(), 1);
    assertEqual(computer.getDestination(0)->getName(), destination.getName());

    assertEqual(computer.getId(), 1);
}

/**
 * Test operation of FeatureComputer with single source and destination
 */
test(FeatureComputer__simple_operation)
{
    Feature source("SRC", 8, 10);
    Feature destination("DST", 2, 1);
//    FeatureComputer(uint8_t id, uint8_t destinationCount=0,
//                    Feature *destination0=NULL)
    FeatureComputer computer(1, 1, &destination);
    // Add source, with computation over 2 sections at a time
    computer.addSource(&source, 2);

    // Source is not ready initially
    assertFalse(computer.compute());

    // Fill out 1st section of the source
    for (uint8_t i = 0; i < 10; ++i)
    {
        source.incrementFillingIndex();
    }
    // Still no compute since the computer need 2 sections
    assertFalse(computer.compute());

    // Fill out 2nd section of the source
    for (uint8_t i = 0; i < 10; ++i)
    {
        source.incrementFillingIndex();
    }
    // Now compute is possible
    assertTrue(computer.compute());

    // Trying to compute a 2nd time fails since source sections have been
    // acknowledged
    assertFalse(computer.compute());

    // Fill out 3rd section of the source
    for (uint8_t i = 0; i < 10; ++i)
    {
        source.incrementFillingIndex();
    }
    // No compute because only 3rd section is ready (both 1st and 2nd sections
    // have been acknowledged)
    assertFalse(computer.compute());

    // Reset and refill source and test destination readiness
    source.reset();
    for (uint8_t i = 0; i < 20; ++i)
    {
        source.incrementFillingIndex();
    }

    // Fill out 1st section of the destination
    destination.incrementFillingIndex();
    // Compute is possible since 2nd section of the destination is free
    assertTrue(computer.compute());

    source.reset();
    for (uint8_t i = 0; i < 20; ++i)
    {
        source.incrementFillingIndex();
    }

    // Fill out 2nd section of the destination
    destination.incrementFillingIndex();
    // Compute is now impossible since no free section in the destination
    assertFalse(computer.compute());
}


/**
 * Test operation of FeatureComputer with multiple sources and destinations
 */
test(FeatureComputer__complex_operation)
{
    // Featurecomputers can take different number of sources and destinations,
    // and they can all have different buffer section counts and sizes.
    Feature source1("S01", 8, 10);
    Feature source2("S02", 4, 5);
    Feature destination1("D01", 2, 1);
    Feature destination2("D02", 2, 2);
    Feature destination3("D03", 4, 1);

    FeatureComputer computer(1, 3, &destination1, &destination2, &destination3);
    // Note that sources can be used with different number of sections at a time
    // and that it is not necessary that the buffers of each source be comsumed
    // at the same rate.
    computer.addSource(&source1, 4);
    computer.addSource(&source2, 1);

    // The computer need 4 sections of source1 + 1 section of source2
    // Fill out 1st 3 sections of source1
    for (uint8_t i = 0; i < 30; ++i)
    {
        source1.incrementFillingIndex();
    }
    assertFalse(computer.compute());  // No compute
    // Fill out 1st section of source2
    for (uint8_t i = 0; i < 5; ++i)
    {
        source2.incrementFillingIndex();
    }
    assertFalse(computer.compute());  // No compute
    // Fill out 4th section of source1
    for (uint8_t i = 0; i < 10; ++i)
    {
        source1.incrementFillingIndex();
    }
    assertTrue(computer.compute());  // Compute !

    // Compute is done on sections 1 to 4 of source1 and section 1 of source2,
    // cannot recompute until new sections are filled.
    assertFalse(computer.compute());  // No nompute


    // Reset and refill sources and test destinations readiness
    source1.reset();
    for (uint8_t i = 0; i < 40; ++i)
    {
        source1.incrementFillingIndex();
    }
    source2.reset();
    for (uint8_t i = 0; i < 5; ++i)
    {
        source2.incrementFillingIndex();
    }

    for (uint8_t i = 0; i < 4; ++i)
    {
        destination2.incrementFillingIndex();
    }
    // destination2 is full => cannot compute
    assertFalse(computer.compute());  // No nompute
}


/**
 * Test SignalRMScomputer computation results
 */
test(FeatureComputer__signal_rms)
{
    // This test works only if testSampleCount >= 512
    assertMoreOrEqual(testSampleCount, 512);

    Q15Feature source("SRC", 8, 128, sharedTestArray);

    float rms128Values[8];
    FloatFeature rms128("SR7", 8, 1, rms128Values);
    float rms512Values[2];
    FloatFeature rms512("SR9", 2, 1, rms512Values);

    SignalRMSComputer rms128Computer(1, &rms128, false, false);
    SignalRMSComputer rms512Computer(1, &rms512, false, false);
    rms128Computer.addSource(&source, 1);
    rms512Computer.addSource(&source, 4);

    /***** Conditions: removeMean = false, normalize = false *****/
    for (uint16_t i = 0; i < 512; ++i)
    {
        source.addQ15Value(q15TestData[i]);
    }
    assertTrue(rms128Computer.compute());
    assertTrue(rms512Computer.compute());
    assertEqual(round(rms128Values[0]), 91630);
    assertEqual(round(rms512Values[0]), 182370);


    /***** Conditions: removeMean = true, normalize = false *****/
    // Reset source and destinations to reuse them
    source.reset();
    rms128.reset();
    rms512.reset();
    rms128Computer.setRemoveMean(true);
    rms512Computer.setRemoveMean(true);

    for (uint16_t i = 0; i < 512; ++i)
    {
        source.addQ15Value(q15TestData[i]);
    }
    assertTrue(rms128Computer.compute());
    assertTrue(rms512Computer.compute());
    assertEqual(round(rms128Values[0]), 18266);
    assertEqual(round(rms512Values[0]), 36422);


    /***** Conditions: removeMean = false, normalize = true *****/
    // Reset source and destinations to reuse them
    source.reset();
    rms128.reset();
    rms512.reset();
    rms128Computer.setRemoveMean(false);
    rms512Computer.setRemoveMean(false);
    rms128Computer.setNormalize(true);
    rms512Computer.setNormalize(true);

    for (uint16_t i = 0; i < 512; ++i)
    {
        source.addQ15Value(q15TestData[i]);
    }
    assertTrue(rms128Computer.compute());
    assertTrue(rms512Computer.compute());
    assertEqual(round(rms128Values[0]), 8099);
    assertEqual(round(rms512Values[0]), 8060);


    /***** Conditions: removeMean = true, normalize = true *****/
    // Reset source and destinations to reuse them
    source.reset();
    rms128.reset();
    rms512.reset();
    rms128Computer.setRemoveMean(true);
    rms512Computer.setRemoveMean(true);

    for (uint16_t i = 0; i < 512; ++i)
    {
        source.addQ15Value(q15TestData[i]);
    }
    assertTrue(rms128Computer.compute());
    assertTrue(rms512Computer.compute());
    assertEqual(round(rms128Values[0]), 1615);
    assertEqual(round(rms512Values[0]), 1610);

}


/**
 * Test SectionSumComputer computation results
 */
test(FeatureComputer__section_sum)
{
    float source1Values[8];
    FloatFeature source1("S01", 4, 2, source1Values);
    float source2Values[8];
    FloatFeature source2("S02", 4, 2, source2Values);

    float dest1Values[4];
    FloatFeature dest1("D01", 4, 1, dest1Values);
    float dest2Values[2];
    FloatFeature dest2("D02", 2, 1, dest2Values);

    // Sum over 1 section and over multiple sections
    SectionSumComputer sumComputer(1, 2, &dest1, &dest2);
    sumComputer.addSource(&source1, 1);  // Sum over 1 section
    sumComputer.addSource(&source2, 2);  // Sum over 2 sections at once

    // Without averaging / normalize = false (ie sum value)
    sumComputer.setNormalize(false);
    for (uint16_t i = 0; i < 2; ++i)  // Need to fill 1 section
    {
        source1.addFloatValue(i + 1.5);
    }
    for (uint16_t i = 0; i < 4; ++i)  // Need to fill 2 sections
    {
        source2.addFloatValue(2 * i);
    }
    assertTrue(sumComputer.compute());
    assertEqual(round(dest1Values[0] * 100), 400);
    assertEqual(round(dest2Values[0] * 100), 1200);


    // With averaging / normalize = true (ie mean value)
    source1.reset();
    source2.reset();
    dest1.reset();
    dest2.reset();
    sumComputer.setNormalize(true);
    for (uint16_t i = 0; i < 2; ++i)  // Need to fill 1 section
    {
        source1.addFloatValue(i + 1.5);
    }
    for (uint16_t i = 0; i < 4; ++i)  // Need to fill 2 sections
    {
        source2.addFloatValue(2 * i);
    }
    assertTrue(sumComputer.compute());
    assertEqual(round(dest1Values[0] * 100), 200);
    assertEqual(round(dest2Values[0] * 100), 300);
}


/**
 * Test MultiSourceSumComputer computation results
 */
test(FeatureComputer__multi_source_sum)
{
    float source1Values[8];
    FloatFeature source1("S01", 4, 2, source1Values);
    float source2Values[8];
    FloatFeature source2("S02", 4, 2, source2Values);

    float dest1Values[8];
    FloatFeature dest1("D01", 4, 2, dest1Values);
    float dest2Values[8];
    FloatFeature dest2("D02", 4, 2, dest2Values);

    MultiSourceSumComputer sumComputer(1, &dest1);  // Test over single section
    sumComputer.addSource(&source1, 1);  // Sum over 1 section
    sumComputer.addSource(&source2, 1);  // Sum over 2 sections at once
    MultiSourceSumComputer sumComputer2(2, &dest2);  // Test over 2 sections
    sumComputer2.addSource(&source1, 2);  // Sum over 1 section
    sumComputer2.addSource(&source2, 2);  // Sum over 2 sections at once

    // Without averaging / normalize = false (ie sum value)
    sumComputer.setNormalize(false);
    sumComputer2.setNormalize(false);
    for (uint16_t i = 0; i < 4; ++i)
    {
        source1.addFloatValue(i + 1.5);
        source2.addFloatValue(2 * i);
    }
    assertTrue(sumComputer.compute());
    assertTrue(sumComputer2.compute());
    assertEqual(round(dest1Values[0] * 100), 150);
    assertEqual(round(dest1Values[1] * 100), 450);
    assertEqual(round(dest2Values[0] * 100), 150);
    assertEqual(round(dest2Values[1] * 100), 450);
    assertEqual(round(dest2Values[2] * 100), 750);
    assertEqual(round(dest2Values[3] * 100), 1050);

    // With averaging / normalize = true (ie mean value)
    source1.reset();
    source2.reset();
    dest1.reset();
    dest2.reset();
    sumComputer.setNormalize(true);
    sumComputer2.setNormalize(true);
    for (uint16_t i = 0; i < 4; ++i)  // Need to fill 1 section
    {
        source1.addFloatValue(i + 1.5);
        source2.addFloatValue(2 * i);
    }
    assertTrue(sumComputer.compute());
    assertTrue(sumComputer2.compute());
    assertEqual(round(dest1Values[0] * 100), 75);
    assertEqual(round(dest1Values[1] * 100), 225);
    assertEqual(round(dest2Values[0] * 100), 75);
    assertEqual(round(dest2Values[1] * 100), 225);
    assertEqual(round(dest2Values[2] * 100), 375);
    assertEqual(round(dest2Values[3] * 100), 525);
}


/**
 * Test Q15FFTComputer computation results
 */
test(FeatureComputer__q15_fft)
{

}


/**
 * Test AudioDBComputer computation results
 */
test(FeatureComputer__audio_db)
{

}

#endif // TEST_FEATURECOMPUTER_H_INCLUDED
