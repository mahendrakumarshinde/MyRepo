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

## Feature rework ##

Avantage d'etre en pull => Chaque producer ne stocke ses donnees qu'une fois et les processors (=features) les utilisent
=> gain de memoire
=> On cree toutes les features des le debut, et apres on ne fait que les activer / desactiver


## BMX-055 - Check if the following can be useful ##
- Accelerometer fifo_data 0x3F register might be useful
- Accelerometer EST (equidistant-sampling mode): 1/(2 * bw) = n * t SLEEP where n is an integer


## STM32 - Check in datasheet what are the following ##
– Batch acquisition mode (BAM)
- ultra-low-power brown-out reset (BOR)


## Using preset configuration ##
I started writing those in C++ but then realize that it would be far better if they were stored on the cloud (json, yaml, etc).

```c++
/* ============================= Operation Presets ============================= */

/**
 * Usage Preset are user controlled, they describe how the device is being used
 */
class powerPreset
{
  public:
    enum  option : uint8_t
    {
      ACTIVE  = 0,
      SUSPEND = 2,
      LOW1    = 3,
      LOW2    = 4,
      COUNT   = 5,
    };

    static powerMode::option accelPowerModes[option::COUNT] = 
      {
        powerMode::ACTIVE,
        powerMode::SUSPEND,
        powerMode::ACTIVE,
        powerMode::ACTIVE,
        powerMode::COUNT,
      };
};

/**
 * Usage Preset are user controlled, they describe how the device is being used
 */
class usagePreset
{
  public:
    enum  option : uint8_t
    {
      CALIBRATION   = 0,
      EXPERIMENT    = 1,
      OPERATION     = 2,
      COUNT         = 3,
    };
    
    // Define the related default config
    static powerPreset::option powerPresetDetails[option::COUNT] = 
      {
        powerPreset::FEATURE,
        powerPreset::RAWDATA,
        powerPreset::FEATURE,
        powerPreset::COUNT,
      };
    static acquisitionMode::option acquisitionModeDetails[option::COUNT] = 
      {
        acquisitionMode::WIRED,
        acquisitionMode::WIRED,
        acquisitionMode::BLE,
        acquisitionMode::COUNT,
      };
    static streamingMode::option streamingModeDetails[option::COUNT] = 
      {
        streamingMode::WIRED,
        streamingMode::WIRED,
        streamingMode::BLE,
        streamingMode::COUNT,
      };
};


/* ============================= Machine Presets ============================= */

class machinePreset
{
  public:
    enum  option : uint8_t
    {
      MOTOR   = 0,
      PRESS   = 1,
      COUNT   = 2,
    };
    // TODO => define powerPresets and features for each machine
};
```

