# Infinite Uptime

## Main BLE Firmware
###Modes of Operation
####Enum `OpMode`
* `RUN`:  
  Regular operation mode. Can enter this mode by simply disconnecting the USB. Will transmit via BLE when internal state changes.
  * Internal states, Enum `OpState` implements this:  
    `NOT_CUTTING = 0`: No cutting  
    `NORMAL_CUTTING = 1`: Normal cutting  
    `BAD_CUTTING = 2`: Bad cutting
* `CHARGING`:  
  Charging mode. Can enter this mode by simply connecting the USB. The module will only listen to Serial (via USB), for entering `DATA_COLLECTION` mode. `i2s_rx_callback( int32_t *pBuf )` is forced to return immediately during this mode, therefore no data collection nor feature calculation.

* `DATA_COLLECTION`:  
  Data collection mode. Can enter this mode ONLY from `CHARGING` mode. Commands to send over Serial is:
  * `IUCMD_START`: Enters into `DATA_COLLECTION` mode from `CHARGING` mode. The module will print `IUOK_START` and then dump raw data bytes.
  * `IUCMD_END`: Enters into `CHARGING` mode from `DATA_COLLECTION` mode. The module will print `IUOK_END`.

###Multi-Feature Handling
####Buffer Switching
* Sampling is prioritized; `i2s_rx_callback` will be called regardless of feature calculations. To enable this, two seprate buffers are kept: while feature is calculated on buffer 0, sampling routine will continue collecting data on buffer 1 and vice versa. Feature calculation will only trigger if fully populated buffer is available. If feature calculation lags behind for some reason and sampling routine no longer has a free buffer to populate, sampling will hault until a buffer is free to use.
* TODO: Obviously this method is VERY memory expensive. Memory saving measures must be taken.

####Highest Danger Level
* Every feature will have its own thresholds, and will each produce a danager level (0, 1, 2). Highest danger level is taken from all the features to set the `OpState`.
* TODO: We may want to also transmit feature indices / feature value that have high danger levels, for identifying malfunction types.

###LED Color Tags
####Enum `LEDColors`
* `RED_BAD`: `RUN` mode. `BAD_CUTTING` state.
* `BLUE_NOOP`: `RUN` mode. `NOT_CUTTING` state.
* `GREEN_OK`: `RUN` mode. `NORMAL_CUTTING` state.
* `ORANGE_CHARG`: `CHARGE` mode, when battery is being charged.
* `PURPLE_ERR`: Error during initialization (`I2Cscan()`)
* `CYAN_DATA`: `DATA_COLLECTION` mode.
* `WHITE_NONE`: Turned on during initialization, or during `CHARGE` mode when battery is fully charged.

###Configuration Varaibles
  * `statusLED`: LED on/off
  * `CLOCK_TYPE`: internal clock rate for I2S microphone (No real need to change this)
  * `target_audio_sample`: audio sampling rate
  * `target_accel_sample`: accel sampling rate
  * `energy_interval`: batch size for energy calculation
  * `rest_count`: in-between sample gaps. 400 means 400 samples will be ignored until energy is re-calculated. Default is 0; no waiting in between energy calculation.

###Two-way Communication Protocol
* Two-way Communication via BLE is only valid on `RUN` mode. First of all, Get the number of bytes (characters) available for reading from the serial port. There are only 19 characters. Then, parse them into four decimal integers separated by three dash lines. i.e. `&bleFeatureIndex-&newThres-&newThres2-&newThres3`

* Set "Rubbish data" when there are more than 6 features or thresholds are over their limits.

###Flowchart for visualizing two main functions: `i2s_rx_callback` and `loop`.

* ![i2s_rx_callback](flowchart_callback.png)

* ![loop](flowchart_loop.png)
