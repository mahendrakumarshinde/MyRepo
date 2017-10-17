
**1. Read Temperature and Pressure at once?**

**2. What do we do when ill-configured? Nothing? Send something special in heartbeat?**

**3. Check and fix resolution computation for each sensor**

**4. Formerly, had issue processing the USB instructions in EXPERIMENT mode. Is it working now?**
See Conductor.acquireData
/* TODO: In EXPERIMENT mode, need to process I2C data here
otherwise they are not properly processed. Can we fix
this ? */
processInstructionsFromI2C();

**5. butterfly_firmware is not working on branch feature_rework_from_scratch => do not commit it in Master**

## BONUS ##

**01. Add configuration options in FFT features to set up freq_low_bound and freq_high_bound**

**02. I2S was previously using q15_t (16 most significant bits) to reduce memory usage. Do that again?**
=> Or maybe just engineer, present a more useful feature for Sound data

**03. Add geolocation features**
CAM-M8Q GNSS => the feature format is not a base type, it is GNSSLocation type
How to handle the value propagation through the features / featureComputers?

**04. function conductor.resetDataAcquisition wasn't working. Is it now?**

**05. To save power: deactivateFeature and activateFeature => Also activate/deactivate sensors automatically?**

**06. Low power modes and configurations: When exiting suspend mode, some sensors need to rewrite configurations**
=> check and fix.

**07. Still get MAGERR, but I haven't yet really understood why it happens sometimes and sometimes not**

**08. Use Device Functions**
In functions, need to program:
- Sensors On / Off
- Features
- data send Period
- features enable for Operation sate

Outside of function, keep configurability for:
- Feature threshold
- Features FFT filtering
- Full scale range

