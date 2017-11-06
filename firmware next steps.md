## OTA ##
  1. BLE:
    - Check and test Kris sketches
    - OPTION 1: Use Kris's skecth for OTA using a Dragonfly + NRF52 as host
      * Need to get a Dragonfly with a NRF52832 from Kris?
      * "In order to program the compiled code onto the nRF52 development board, a Nordic nRF52 pca10040 DK board is useful." => Also need to get a DK board?
    - OPTION 2:
      * Use Cassia Hub as host => in that case, I have to replicate "IU_Dragonfly_BLE_OTA_Update_Utility_V0.0" functionnalities in a Python app.
      * We can also have Po program the same for the Android App.
    - Note that *.iap files (firmware image files) path are given by Arduino IDE once the skecth compilation is complete. The file path should be something like "/tmp/arduino_build_XXXXXX".
  2. WiFi:
    - Check Kris sketches
