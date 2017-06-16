## Commentaire pour JD sur les features ##

Avantage d'etre en pull => Chaque producer ne stocke ses donnees qu'une fois et les processors (=features) les utilisent
=> gain de memoire
=> On cree toutes les features des le debut, et apres on ne fait que les activer / desactiver



## BLE, WiFi ##

### Android App / Hub to device ###

Command length has a max size, that is used to define the size of the receiving buffer on the device:
=> Max cmd length = 50 char = 50 bytes

Message from the app / cloud to the device are formated as follow:
hex_msg_length,hex_cmd_id,comma_separated_args;
=> the message length in Hexadecimal is used to assert that we haven't missed some of the message

See next section to check how command should be acknowledged.

eg: set thresholds for feature_id 1 to (5, 10, 15) - set_threshold command has id 12
10,12,1,5,10,15;
(note that 10(HEX) = 16(DEC))

### Device to Android App / Hub ###


timestamp,MAC_address,msg_type,rest_of_message;

The message type can be:
- Command acknowledgement: rest_of_message = hex_cmd_id
- raw data sending: rest_of_message = data_id,sample_length,comma_separated_samples
- FFT sending: rest_of_message = data_id,sample_length,comma_separated_coeffs (=> TODO: determine how to make profit of the FFT coeffs sparsity)
- feature sending: rest_of_message = feature_count,[feature_id,feature_value] * feature_count

eg:
- Command acknowledgement: 1492145778.80,94:54:93:0E:7D:D1,ACK,12;
- raw data sending: 1492145778.80,94:54:93:0E:7D:D1,RAW,AX,512,10.05,11.2,10.8,...,28;
- FFT sending: 1492145778.80,94:54:93:0E:7D:D1,FFT,AX,256,80,72,20,,,,,,12,45,6,,,;
- feature sending: 1492145778.80,94:54:93:0E:7D:D1,FEA,4,01,42.0,02,3.0,03,32,04,89;


### Wired command only ###
Some commands can only be sent via Serial, since there are meant to work while the device is wired (eg: )
For human readability and 







**The following should be stored in the our database**



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

