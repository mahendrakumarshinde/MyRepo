
**1. Read Temperature and Pressure at once?**

**2. What do we do when ill-configured? Nothing? Send something special in heartbeat?**
- What to do when no feature are streaming? Use atLeastOneStreamingFeature

**3. Check and fix resolution computation for each sensor**

**4. Formerly, had issue processing the USB instructions in EXPERIMENT mode. Is it working now?**
See Conductor.acquireData
/* TODO: In EXPERIMENT mode, need to process I2C data here
otherwise they are not properly processed. Can we fix
this ? */
processInstructionsFromI2C();

## BONUS ##

**00. Improve Features.h / Features.cpp**
- Move declaration + instanciation of sensors into sensor files
- Use namespace FeatureUtility

**01. Add configuration options in FFT features to set up freq_low_bound and freq_high_bound**

**02. Add RPM feature**

**03. Add geolocation features**
CAM-M8Q GNSS => the feature format is not a base type, it is GNSSLocation type
How to handle the value propagation through the features / featureComputers?

**04. function conductor.resetDataAcquisition wasn't working. Is it now?**

**05. To save power: deactivateFeature and activateFeature => Also activate/deactivate sensors automatically?**

**22. Info about RAM selection (among other)**
https://github.com/GrumpyOldPizza/arduino-STM32L4/blob/master/variants/STM32L432KC-NUCLEO/linker_scripts/STM32L432KC_FLASH.ld

