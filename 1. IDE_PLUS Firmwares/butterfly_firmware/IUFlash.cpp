#include "IUFlash.h"
#include "FFTConfiguration.h"
#include "InstancesDragonfly.h"
/* =============================================================================
    IUFSFlash - Flash with file system
============================================================================= */

char IUFSFlash::CONFIG_SUBDIR[IUFSFlash::CONFIG_SUBDIR_LEN] = "/iuconfig";
char IUFSFlash::IUFWBACKUP_SUBDIR[IUFSFlash::CONFIG_SUBDIR_LEN] = "/iuBackupFirmware";
char IUFSFlash::IUFWTMPIMG_SUBDIR[IUFSFlash::CONFIG_SUBDIR_LEN]  = "/iuTempFirmware";
char IUFSFlash::IUFWROLLBACK_SUBDIR[IUFSFlash::CONFIG_SUBDIR_LEN] = "/iuRollbackFirmware";
char IUFSFlash::RULE_SUBDIR[IUFSFlash::CONFIG_SUBDIR_LEN] = "/iuRule";
char IUFSFlash::CONFIG_EXTENSION[IUFSFlash::CONFIG_EXTENSION_LEN] = ".conf";

char IUFSFlash::FNAME_WIFI0[6] = "wifi0";
char IUFSFlash::FNAME_WIFI1[6] = "wifi1";
char IUFSFlash::FNAME_WIFI2[6] = "wifi2";
char IUFSFlash::FNAME_WIFI3[6] = "wifi3";
char IUFSFlash::FNAME_WIFI4[6] = "wifi4";
char IUFSFlash::FNAME_FEATURE[9] = "features";
char IUFSFlash::FNAME_COMPONENT[11] = "components";
char IUFSFlash::FNAME_DEVICE[7] = "device";
char IUFSFlash::FNAME_OP_STATE[8] = "opState";
char IUFSFlash::FNAME_RAW_DATA_ENDPOINT[13] = "fft_endpoint";
char IUFSFlash::FNAME_MQTT_SERVER[12] = "mqtt_server";
char IUFSFlash::FNAME_MQTT_CREDS[11] = "mqtt_creds";
char IUFSFlash::FNAME_FFT[4] = "fft";
char IUFSFlash::FNAME_OTA[4] = "ota";
char IUFSFlash::FNAME_FORCE_OTA[10] = "force_ota";
char IUFSFlash::FNAME_MODBUS_SLAVE[12] = "modbusSlave";
char IUFSFlash::FNAME_FOUT[12] = "fout";
char IUFSFlash::FNAME_HASH[11] = "configHash";
char IUFSFlash::FNAME_DIG[11] = "diagnostic";
char IUFSFlash::FNAME_RPM[4] = "rpm";
char IUFSFlash::FNAME_PHASE[6] = "phase";
/***** Core *****/

void IUFSFlash::begin()
{
    if (!DOSFS.begin())
    {
        if (setupDebugMode)
        {
            debugPrint("Failed to start DOSFS (DOSFS.begin returned false)");
        }
    }
    m_begun = DOSFS.exists(CONFIG_SUBDIR);
    if (!m_begun)
    {
        DOSFS.mkdir(CONFIG_SUBDIR);
        m_begun = DOSFS.exists(CONFIG_SUBDIR);
        if (!m_begun && setupDebugMode)
        {
            debugPrint("Unable to find or create the config directory");
        }
    }
    m_otaDir = DOSFS.exists(IUFWBACKUP_SUBDIR);
    if (!m_otaDir)
    {
        DOSFS.mkdir(IUFWBACKUP_SUBDIR);
        m_otaDir = DOSFS.exists(IUFWBACKUP_SUBDIR);
        if (!m_otaDir && setupDebugMode)
        {
            debugPrint("Unable to find/create the ota_mainbkup directory");
        }
    }
    m_otaDir = DOSFS.exists(IUFWTMPIMG_SUBDIR);
    if (!m_otaDir)
    {
        DOSFS.mkdir(IUFWTMPIMG_SUBDIR);
        m_otaDir = DOSFS.exists(IUFWTMPIMG_SUBDIR);
        if (!m_otaDir && setupDebugMode)
        {
            debugPrint("Unable to find or create the ota_mainimage directory");
        }
    }
    m_otaDir = DOSFS.exists(IUFWROLLBACK_SUBDIR);
    if (!m_otaDir)
    {
        DOSFS.mkdir(IUFWROLLBACK_SUBDIR);
        m_otaDir = DOSFS.exists(IUFWROLLBACK_SUBDIR);
        if (!m_otaDir && setupDebugMode)
        {
            debugPrint("Unable to find or create the ota_rollback directory");
        }
    }
    m_digDir = DOSFS.exists(RULE_SUBDIR);
    if(!m_digDir){
        DOSFS.mkdir(RULE_SUBDIR);
        m_digDir = DOSFS.exists(RULE_SUBDIR);
        if (!m_digDir && setupDebugMode)
        {
            debugPrint("Unable to find or create the iuRule directory");
        }

    }
}


/***** Config read / write functions *****/

/**
 * Read the relevant file to retrieve the stored config.
 *
 * Return the number of read chars. Returns 0 if the stored config type is
 * unknown or if the file is not found.
 */
size_t IUFSFlash::readConfig(storedConfig configType, char *config,
                             size_t maxLength)
{
    File file = openConfigFile(configType, "r");
    size_t readCharCount = 0;
    if (!file) {
        strcpy(config, "");
    }
    else {
        readCharCount = file.readBytes(config, maxLength);
    }
    file.close();
    return readCharCount;
}

/**
 * Store the config in flash by writing the config to the relevant file.
 *
 * Return the number of written chars. Returns 0 if the stored config type is
 * unknown.
 */
size_t IUFSFlash::writeConfig(storedConfig configType, char *config, size_t len)
{
    File file = openConfigFile(configType, "w");
    size_t writtenCharCount = 0;
    if (file)
    {
        writtenCharCount = file.write((uint8_t*) config, len);
    }
    file.close();
    return writtenCharCount;
}

/**
 * Delete the config.
 */
bool IUFSFlash::deleteConfig(storedConfig configType)
{
    if (!available()) {
        return false;
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    getConfigFilename(configType, filepath, MAX_FULL_CONFIG_FPATH_LEN);
    if (DOSFS.exists(filepath)) {
        return DOSFS.remove(filepath);
    } else {
        return true;
    }
}

/***** JSON Config load / save functions *****/

bool IUFSFlash::saveConfigJson(storedConfig configType, JsonVariant &config)
{
    if (!available()) {
        return false;
    }
    File file = openConfigFile(configType, "w");
    if (config.printTo(file) == 0)
    {
        return false;
    }
    file.close();
    if (debugMode)
    {
        debugPrint("Successfully saved config type #", false);
        debugPrint((uint8_t) configType);
    }
    return true;
}
/*
    Following method only updates the fields received in the config message.
    It will not overwrite the previous json string, only add/update the fields
    received in the config message.
 */
bool IUFSFlash::updateConfigJson(storedConfig configType, JsonVariant &config)
{
    if (!available()) {
        return false;
    }
    StaticJsonBuffer<300> storedConfigJsonBuffer;
    const int storedConfigMaxLength = 300;
    char storedConfigJsonString[storedConfigMaxLength];
    int charCount;
    strcpy(storedConfigJsonString, "");
    charCount = readConfig(configType, storedConfigJsonString, storedConfigMaxLength);    
    if (charCount == 0) { strcpy(storedConfigJsonString, "{}"); }
    //storedConfigJsonString[charCount] = '\0'; //DOES NOT WORK, json is created as {}, however no fields can be added in this json
    JsonObject& storedConfigJson = storedConfigJsonBuffer.parse(storedConfigJsonString);
    for(auto kv:(JsonObject&)config) {
        storedConfigJson[kv.key] = kv.value;
    }
    File file = openConfigFile(configType, "w");
    if (storedConfigJson.printTo(file) == 0)
    {
        return false;
    }
    file.close();
    if (debugMode)
    {
        debugPrint("Successfully saved config type #", false);
        debugPrint((uint8_t) configType);
    }
    return true;
}

bool IUFSFlash::validateConfig(storedConfig configType, JsonObject &config, char* validationResultString, char* mac_id, double timestamp, char* messageId)
{
    // Perform validation checks on the config json
    // Return the errors in a json object
    bool validConfig = true;
    StaticJsonBuffer<400> validationResultBuffer;
    JsonObject& validationResult = validationResultBuffer.createObject();
    JsonArray& errorMessages = validationResult.createNestedArray("errorMessages");
    
    switch(configType) {
        case CFG_DEVICE: {
            // Configuration is valid if there is atleast one correct key           

            // Indicate the type of validation
            validationResult["messageType"] = "device-config-ack";

            // Save the updated params
            JsonArray& validParams = validationResult.createNestedArray("validParams");

            // Validation for "DSP" parameter
            if(config.containsKey("DSP")) {
                int newDataSendPeriod = config["DSP"];
                if(newDataSendPeriod < 200) { // DSP cannot be less than 200 milliseconds
                    errorMessages.add("DSP < 200 ms");
                    config.remove("DSP");
                } else {
                    validParams.add("DSP");
                }
            }

            // Validation for "RAW parameter"
            if(config.containsKey("RAW")) {
                int newRawDataPublishPeriod = config["RAW"];
                if(newRawDataPublishPeriod < 30) { // RAW cannot be less than 10 seconds
                    errorMessages.add("RAW < 30 seconds");
                    config.remove("RAW");
                } else {
                    validParams.add("RAW");
                }
            }

            // Device configuration message : {"main":{"GRP":["MOTSTD"],"RAW":1800,"POW":0,"TSL":60,"TOFF":10,"TCY":20,"DSP":512}}
            // TODO: Validation should be added for these keys (GRP, POW, TSL, TOFF, TCY etc.) as well, right now these keys
            // are assumed to be correct and are being added without any validation checks. After adding validation checks, 
            // if the parameter is incorrect it should be removed from "config". The following logic is added as a temporary workaround, 
            // it ensures that the parameters which do not have validation checks will be considered to be valid.
            /* WORKAROUND FOR PARAMS WITHOUT VALIDATION CHECKS*/
            const int numberOfParametersWithValidationChecks = 2;
            const int parameterNameLength = 4; 
            // The following list contains the parameters which have validation checks
            char parametersWithValidationChecks[2][4] = {"DSP", "RAW"};
            // All parameters in "config" excluding parametersWithValidationChecks are to be added to the list of valid params
            for (auto kv:config) {
                bool parameterWithValidationCheck = false;
                for (int i=0; i<numberOfParametersWithValidationChecks; ++i) {
                    if (strcmp(kv.key, parametersWithValidationChecks[i]) == 0) {
                        parameterWithValidationCheck = true;
                        break;
                    }
                }
                if(!parameterWithValidationCheck) {
                    validParams.add(kv.key);
                }
            }   
            /* THIS WORKAROUND CAN BE REMOVED AFTER VALIDATION CHECKS ARE ADDED FOR ALL PARAMETERS */         

            if (validParams.size() == 0) {
                validConfig = false;
            }
            break;
        }
        case CFG_FFT: {
            // Configuration is valid only if both samplingRate and blockSize are present
            // and both are within their acceptable range of values
            validConfig = true;

            // Indicate the type of validation
            validationResult["messageType"] = "fft-config-ack";

            // If the received config matches the current config, report an error
            bool sameBlockSize = false;
            bool notSupportedBlockSize = false;
            bool sameSamplingRate = false;
            bool notSupportedSamplingRate = false;
            bool samegRange = false;
            bool grangepresent = true;
            //Validation for samplingRate field
            if(config.containsKey("samplingRate")) {
                uint16_t samplingRate = config["samplingRate"];
                // Validation for samplingRate
                bool validLSMSamplingRate = true;
                bool validKionixSamplingRate = true;
                //bool validSensor = true;
                for(int i=0; i<FFTConfiguration::LSMsamplingRateOption - 1; ++i) {
                    if( samplingRate < FFTConfiguration::samplingRates[0] || 
                        samplingRate > FFTConfiguration::samplingRates[FFTConfiguration::LSMsamplingRateOption - 1] || 
                        (FFTConfiguration::samplingRates[i] < samplingRate &&
                         samplingRate < FFTConfiguration::samplingRates[i+1]) ) {
                           validLSMSamplingRate = false;
                       }
                }
                for(int i=0; i<FFTConfiguration::KNXsamplingRateOption - 1; ++i) {
                    if( samplingRate < FFTConfiguration::samplingRates2[0] || 
                        samplingRate > FFTConfiguration::samplingRates2[FFTConfiguration::KNXsamplingRateOption - 1] || 
                        (FFTConfiguration::samplingRates2[i] < samplingRate &&
                         samplingRate < FFTConfiguration::samplingRates2[i+1]) ) {
                             validKionixSamplingRate = false;
                       }
                }
                if(config.containsKey("grange")){                                           // g range contains key is not cheked while saving the json because without grange also the jason is valid.
                    uint8_t grange = config["grange"];
                    uint16_t samplingrate = config["samplingRate"];                                   //same g range received validation check not done.
                    bool validLSMgRange = true;
                    bool validKNXgRange = true;
                    grangepresent = false;
                    debugPrint("G_range_received in JSON");
                    if(validLSMSamplingRate){
                    for (int i = 0; i <FFTConfiguration::LSMgRangeOption - 1; ++i){
                        if( grange < FFTConfiguration::LSMgRanges[0] || 
                            grange > FFTConfiguration::LSMgRanges[FFTConfiguration::LSMgRangeOption - 1] || 
                            (FFTConfiguration::LSMgRanges[i] < grange &&
                            grange < FFTConfiguration::LSMgRanges[i+1]) ) {
                            validLSMgRange = false;
                        }   
                    }
                    if(FFTConfiguration::currentLSMgRange == grange ){
                        samegRange = true;
                        debugPrint("LSM same gRange received");
                    }
                    }
                    if(validKionixSamplingRate){
                    for(int i = 0; i<FFTConfiguration::KNXgRangeOption - 1; ++i){
                        if( grange < FFTConfiguration::KNXgRanges[0] || 
                            grange > FFTConfiguration::KNXgRanges[FFTConfiguration::KNXgRangeOption - 1] ||
                            (FFTConfiguration::KNXgRanges[i] < grange &&
                            grange < FFTConfiguration::KNXgRanges[i+1]) ){
                                validKNXgRange = false; 
                            }
                        }
                        if(FFTConfiguration::currentKNXgRange == grange ){
                            samegRange = true;
                            debugPrint("KNX same gRange received");
                        }

                    }

                    if (!validLSMgRange && !validKNXgRange) {
                            validConfig = false;
                            errorMessages.add("Invalid g Range");
                    }
                    else if(validLSMSamplingRate && !validLSMgRange ){
                        validConfig = false ;
                        errorMessages.add("Invalid LSM G Range");
                    }
                    else if(validKionixSamplingRate && !validKNXgRange){
                        validConfig = false;
                        errorMessages.add("Invalid Kionix G Range");
                    }

                    debugPrint("valid G_range_json");debugPrint(validLSMgRange);
                }
                //debugPrint("G_range_NOT_received in JSON");
                if(!iuAccelerometer.lsmPresence && validLSMSamplingRate) {
                     validConfig = false;
                     errorMessages.add("LSM not Present");
                }
                if(!iuAccelerometerKX222.kionixPresence && validKionixSamplingRate) {
                     validConfig = false;
                     errorMessages.add("Kionix not Present");
                }
                // if(config.containsKey("sensor") && validKionixSamplingRate) {
                //         uint16_t sensor = config["sensor"];
                //         if (sensor != FFTConfiguration::kionixSensor){
                //             validSensor = false;
                //         }
                //         else{
                //             validSensor = true;
                //         }    
                // }
                // if(config.containsKey("sensor") && validLSMSamplingRate) {
                //         uint16_t sensor = config["sensor"];
                //         if (sensor != FFTConfiguration::lsmSensor){
                //             validSensor = false;
                //         }
                //         else{
                //             validSensor = true;
                //         }    
                // }
                if (!validLSMSamplingRate && !validKionixSamplingRate) {
                        validConfig = false;
                        errorMessages.add("Invalid samplingRate");
                // } else if (samplingRate == 416) {  // Temporary workaround
                //     validConfig = false;
                //     errorMessages.add("Sampling rate not supported");
                } else if (FFTConfiguration::currentSamplingRate == samplingRate) {
                    sameSamplingRate = true;
                }
                if (samplingRate == 208)
                {
                    notSupportedSamplingRate = true;
                } 
                // else if (!validSensor){
                //     validConfig = false;
                //     errorMessages.add("Invalid Sensor Selection");
                // }
            } else {
                validConfig = false;
                errorMessages.add("Key missing: samplingRate"); 
            }
         
            // Validation for blockSize field
            if(config.containsKey("blockSize")) {
                uint16_t blockSize = config["blockSize"];
                // Validation for samplingRate
                bool validBlockSize = true;
                for(int i=0; i<FFTConfiguration::blockSizeConfigurations; i++) {
                    if( blockSize < FFTConfiguration::blockSizes[0] ||
                        blockSize > FFTConfiguration::blockSizes[FFTConfiguration::blockSizeConfigurations - 1] ||
                        (FFTConfiguration::blockSizes[i] < blockSize &&
                        blockSize < FFTConfiguration::blockSizes[i+1]) ) {
                            validBlockSize = false;
                            break;
                        }
                }
                if (!validBlockSize) {
                        validConfig = false;
                        errorMessages.add("Invalid blockSize");
                } else if (FFTConfiguration::currentBlockSize == blockSize) {
                    sameBlockSize = true;
                }
                if (notSupportedSamplingRate && blockSize > 4096)
                {
                    if(debugMode){
                        debugPrint("SR:208 and BlockSize : 8192 not supported.");
                    }
                    validConfig = false;
                    errorMessages.add("SR:208 and BlockSize : 8192 not supported");
                }
            } else {
                validConfig = false;
                errorMessages.add("Key missing: blockSize"); 
            }
            

            // If the received config matches the current config, report an error
            if(sameBlockSize && sameSamplingRate && grangepresent) {      //if same SR & BS received and gRange is present then false this condition
                validConfig = false;
                errorMessages.add("Same SR & BS received without gRange ");
            }
            else if(sameBlockSize && sameSamplingRate && samegRange){
                validConfig = false;
                errorMessages.add("Same configuration received");
            }
            break;
        }
        case CFG_RPM: {
            validConfig = true;
            validationResult["messageType"] = "rpm-config-ack";
            int lowRPM = config["CONFIG"]["RPM"]["LOW_RPM"];
            int highRPM = config["CONFIG"]["RPM"]["HIGH_RPM"];
            if(highRPM - lowRPM < 0 ){  // negative difference is not allowed 
                validConfig = false;
                errorMessages.add("High RPM cannot be less then LOW RPM");
            }
            break;
        }
        case CFG_HTTP:{
            validConfig = true;
            validationResult["messageType"] = "http-config-ack";
            if( config.measureLength() >= 511 ){
                validConfig = false;
                errorMessages.add("Http config length more");
            }
            break;
        }
        case CFG_MODBUS_SLAVE: {
            // Configuration is valid only if all the parameters are present in the 
            // and All are within their acceptable range of values
            validConfig = true;

            // Indicate the type of validation
            validationResult["messageType"] = "modbusSlave-config-ack";

            // If the received config matches the current config, report an error
            bool sameModbusConfigs = false;
            
            //Validation for samplingRate field
            if(config.containsKey("baudrate") && config.containsKey("databit") && config.containsKey("stopbit") && 
                config.containsKey("parity") &&  config.containsKey("slaveid")  ) {

                uint8_t m_id = config["slaveid"].as<uint16_t>();
                uint32_t m_baud =  config["baudrate"].as<uint32_t>();
                uint8_t m_databit = config["databit"].as<uint8_t>();
                uint8_t m_stopbit = config["stopbit"].as<uint8_t>();
                const char* m_parity = config["parity"];
                if(debugMode){
                    debugPrint("MODBUS DEBUG : ",false);debugPrint(m_databit);
                    debugPrint("MODBUS DEBUG : ",false);debugPrint(m_stopbit);
                    debugPrint("MODBUS DEBUG : ",false);debugPrint(m_parity);    
                }
                if (m_id < 1 || m_id >127){   
                    validConfig = false;
                    errorMessages.add("Invalid slaveid");
                }if ((m_baud < 1200 || m_baud > 115200)){   // Note : Modbus Serial configuration only works between 1200 and 115200bps, complete list is available in app to configure   
                    validConfig = false;
                    errorMessages.add("Invalid baudrate");
                }if (m_databit < 7 || m_databit > 8){     
                    validConfig = false;
                    errorMessages.add("Invalid databit");
                }if (m_stopbit  < 1 || m_stopbit > 2){     
                    validConfig = false;
                    errorMessages.add("Invalid stopbit");
                }if (strncmp(m_parity, "NONE", 4) == 0 || strncmp(m_parity, "EVEN", 4) == 0 || strncmp(m_parity, "ODD", 3) == 0 ){  
                        debugPrint("Valid parity");   
                    }else
                    {
                        validConfig = false;
                        errorMessages.add("Invalid parity");
                    }
                    
                
            } else {
                validConfig = false;
                errorMessages.add("Key missing: modbusSlaveConfig"); 
            }
         
           break;
        }
        case CFG_WIFI0: {

            validConfig = true;

            // Indicate the type of validation
            validationResult["messageType"] = "wifi-config-ack";

            //Validation for Auth Type field
            if(config.containsKey("auth_type")) {
                const char* AuthType = config["auth_type"];
                bool ssid = config.containsKey("ssid");
                bool password = config.containsKey("password");
                bool username = config.containsKey("username");
                bool staticIP = config.containsKey("static");
                bool gateway = config.containsKey("gateway");
                bool subnet = config.containsKey("subnet");
                bool dns1 = config.containsKey("dns1");
                bool dns2 = config.containsKey("dns2");
 
                if (strncmp(AuthType, "NONE", 4) == 0){ 
                    if(!ssid) { errorMessages.add("SSID Not found"); validConfig = false; }
                }else if(strncmp(AuthType, "WPA-PSK", 7) == 0){
                    if(!ssid) { errorMessages.add("SSID Not found"); validConfig = false; }
                    if(!password){ errorMessages.add("Password Not found"); validConfig = false; }
                }else if(strncmp(AuthType, "EAP-PEAP", 8) == 0){
                    if(!ssid) { errorMessages.add("SSID Not found"); validConfig = false; }
                    if(!password){ errorMessages.add("Password Not found"); validConfig = false; }
                    if(!username){ errorMessages.add("Username Not found"); validConfig = false; }
                }else if(strncmp(AuthType, "EAP-TLS", 7) == 0){
                    if(!ssid) { errorMessages.add("SSID Not found"); validConfig = false; }
                    if(!password){ errorMessages.add("Password Not found"); validConfig = false; }
                }else if(strncmp(AuthType, "STATIC-NONE", 11) == 0){
                    if(!ssid){ errorMessages.add("SSID Not found"); validConfig = false; }
                    if(!staticIP){ errorMessages.add("Static IP Not found"); validConfig = false; }
                    if(!gateway){ errorMessages.add("Gateway IP Not found"); validConfig = false; }
                    if(!subnet){ errorMessages.add("Subnet Not found"); validConfig = false; }
                    if(!dns1){ errorMessages.add("DNS 1 Not found"); validConfig = false; }
                    if(!dns2){ errorMessages.add("DNS 2 Not found"); validConfig = false; }
                }else if(strncmp(AuthType, "STATIC-WPA-PSK", 14) == 0){
                    if(!ssid){ errorMessages.add("SSID Not found"); validConfig = false; }
                    if(!password){ errorMessages.add("Password Not found"); validConfig = false; }
                    if(!staticIP){ errorMessages.add("Static IP Not found"); validConfig = false; }
                    if(!gateway){ errorMessages.add("Gateway IP Not found"); validConfig = false; }
                    if(!subnet){ errorMessages.add("Subnet Not found"); validConfig = false; }
                    if(!dns1){ errorMessages.add("DNS 1 Not found"); validConfig = false; }
                    if(!dns2){ errorMessages.add("DNS 2 Not found"); validConfig = false; }
                }else if(strncmp(AuthType, "STATIC-EAP-PEAP", 15) == 0){
                    if(!ssid){ errorMessages.add("SSID Not found"); validConfig = false; }
                    if(!password){ errorMessages.add("Password Not found"); validConfig = false; }
                    if(!username){ errorMessages.add("Username Not found"); validConfig = false; }
                    if(!staticIP){ errorMessages.add("Static IP Not found"); validConfig = false; }
                    if(!gateway){ errorMessages.add("Gateway IP Not found"); validConfig = false; }
                    if(!subnet){ errorMessages.add("Subnet Not found"); validConfig = false; }
                    if(!dns1){ errorMessages.add("DNS 1 Not found"); validConfig = false; }
                    if(!dns2){ errorMessages.add("DNS 2 Not found"); validConfig = false; }
                }else if(strncmp(AuthType, "STATIC-EAP-TLS", 14) == 0 ){
                    if(!ssid){ errorMessages.add("SSID Not found"); validConfig = false; }
                    if(!password){ errorMessages.add("Password Not found"); validConfig = false; }
                    if(!staticIP){ errorMessages.add("Static IP Not found"); validConfig = false; }
                    if(!gateway){ errorMessages.add("Gateway IP Not found"); validConfig = false; }
                    if(!subnet){ errorMessages.add("Subnet Not found"); validConfig = false; }
                    if(!dns1){ errorMessages.add("DNS 1 Not found"); validConfig = false; }
                    if(!dns2){ errorMessages.add("DNS 2 Not found"); validConfig = false; }
                }else{
                    errorMessages.add("Invalid Authentication Type");
                    validConfig = false;
                }
            }
           break; 
        }
    }

    // Construct the validationResult
    validationResult["messageId"] = messageId;
    validationResult["validConfig"] = validConfig;
    validationResult["mac"] = mac_id;
    validationResult["timestamp"] = timestamp;
    validationResult.printTo(validationResultString, 300);
    return validConfig;
}

/***** Utility *****/

/**
 * Find and return the file name of the requested config type.
 *
 * Dest char array pointer should at least be of length
 * MAX_FULL_CONFIG_FPATH_LEN.
 */
size_t IUFSFlash::getConfigFilename(storedConfig configType, char *dest,
                                    size_t len)
{
    char *fname = NULL;
    switch (configType)
    {
        case CFG_WIFI0:
            fname = FNAME_WIFI0;
            break;
        case CFG_WIFI1:
            fname = FNAME_WIFI1;
            break;
        case CFG_WIFI2:
            fname = FNAME_WIFI2;
            break;
        case CFG_WIFI3:
            fname = FNAME_WIFI3;
            break;
        case CFG_WIFI4:
            fname = FNAME_WIFI4;
            break;
        case CFG_FEATURE:
            fname = FNAME_FEATURE;
            break;
        case CFG_COMPONENT:
            fname = FNAME_COMPONENT;
            break;
        case CFG_DEVICE:
            fname = FNAME_DEVICE;
            break;
        case CFG_OP_STATE:
            fname = FNAME_OP_STATE;
            break;
        case CFG_RAW_DATA_ENDPOINT:
            fname = FNAME_RAW_DATA_ENDPOINT;
            break;
        case CFG_MQTT_SERVER:
            fname = FNAME_MQTT_SERVER;
            break;
        case CFG_MQTT_CREDS:
            fname = FNAME_MQTT_CREDS;
            break;
        case CFG_FFT:
            fname = FNAME_FFT;
            break;
        case CFG_OTA:
            fname = FNAME_OTA;
            break;
        case CFG_FORCE_OTA:
            fname = FNAME_FORCE_OTA;
            break;
        case CFG_MODBUS_SLAVE:
            fname = FNAME_MODBUS_SLAVE;
            break;
        case CFG_FOUT:
            fname = FNAME_FOUT;
            break;
        case CFG_HASH:
            fname = FNAME_HASH;
            break;
        case CFG_DIG:
            fname = FNAME_DIG;
            break;
        case CFG_RPM:
            fname = FNAME_RPM;
            break;
        case CFG_PHASE:
            fname = FNAME_PHASE;
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unknown config type "), false);
                debugPrint(configType);
            }
            return 0;
    }
    if (fname == NULL)
    {
        return 0;
    }
    if(fname == FNAME_DIG ){
        return snprintf(dest, len, "%s/%s%s", RULE_SUBDIR, fname,
                    CONFIG_EXTENSION);
    }
    else { 
        return snprintf(dest, len, "%s/%s%s", CONFIG_SUBDIR, fname,
                    CONFIG_EXTENSION);
    }
}

File IUFSFlash::openConfigFile(storedConfig configType,
                                       const char* mode)
{
    if (!available())
    {
        return File();
    }
    char filepath[MAX_FULL_CONFIG_FPATH_LEN];
    size_t fpathLen = getConfigFilename(configType, filepath,
                                        MAX_FULL_CONFIG_FPATH_LEN);
    if (fpathLen == 0 || filepath == NULL)
    {
        if (debugMode)
        {
            debugPrint("Couldn't find the file name for config ", false);
            debugPrint(configType);
        }
        return File();
    }
    return DOSFS.open(filepath, mode);
}

/***Read Write Internal Flag***/

String IUFSFlash::readInternalFlash(uint32_t address)
{
    uint16_t MaxConfigLenth = 512;
    uint8_t type;
    uint16_t length;
    uint8_t result[MaxConfigLenth];
    char resultConfig[MaxConfigLenth];
    stm32l4_flash_unlock();
    memset(result,'\0',sizeof(result));
    type = *(uint8_t*)(address );
    length = *(uint8_t*)(address + 1) + *(uint8_t*)(address + 2);
    delay(1000);
    if(length < MaxConfigLenth && length > 0 )
    {
        for (int i = 0 ; i < length; i++){
            result[i] = *(uint8_t*)(address + i + 3);
        }
    }
    sprintf(resultConfig,"%s",(char*)result);
    stm32l4_flash_lock();
    return resultConfig;
}
/*Internal flash configuration packet format*/
/*--|Precense| Size 16-bit |<--Mqtt or Http config...expected that length is MAX of 510 Bytes>|*/
/*--|   01   |  FF  |  FF  |<---------------------Mqtt/http config json--------------------->|*/
void IUFSFlash::writeInternalFlash(uint8_t type, uint32_t address, uint16_t dataLength, const uint8_t* data)
{
  const uint8_t maxDataperByte = 255;
  uint16_t dataSize;
  char allData[2048];
  dataSize = sizeof(type)+sizeof(dataLength)+dataLength;

  allData[0] = type;
  if(dataLength <= maxDataperByte){
    allData[1] = dataLength;
    allData[2] = 0;
  }else{
    allData[1] = maxDataperByte;
    allData[2] = dataLength - maxDataperByte;
  }
  sprintf(&allData[3],"%s",data);
  for(int i=dataSize;i < 4;i++){
    allData[i] = 0x00;   // Fill next 4 bytes with 00
  }
  for(int i=dataSize + 4;i<sizeof(allData);i++){
    allData[i] = 0xFF;   // Fill remaining data with FF
  }
    if(loopDebugMode){
        debugPrint("Data : ",false);debugPrint((char*)data);
        debugPrint("All Data : ",false);debugPrint(allData); //debugPrint may not work if length is grater than 255 because NULL in next Byte
    }   
  for(int i=0; i<2 ;i++){  
    stm32l4_flash_unlock();
    stm32l4_flash_erase(address, 2048);
    stm32l4_flash_program(address, (const uint8_t*)allData,sizeof(allData));
    stm32l4_flash_lock();
  }
}

bool IUFSFlash::checkConfig(uint32_t address)
{
    uint8_t presence;
    stm32l4_flash_unlock();
    presence = *(uint8_t*)(address );
    stm32l4_flash_lock();
    if(presence == 0x01)
    {
        return true;
    }
    return false;
}
// To clear Internal flash of 2k
void IUFSFlash::clearInternalFlash(uint32_t address)
{
    stm32l4_flash_unlock();
    stm32l4_flash_erase(address, 2048);
    stm32l4_flash_lock();
}
/* =============================================================================
    IUSPIFlash - Flash accessible via SPI
============================================================================= */

/***** Core *****/

uint8_t IUSPIFlash::ID_BYTES[IUSPIFlash::ID_BYTE_COUNT] = {0xEF, 0x40, 0x14};

IUSPIFlash::IUSPIFlash(SPIClass *spiPtr, uint8_t csPin, SPISettings settings) :
    IUFlash(),
    m_csPin(csPin),
    m_spiSettings(settings),
    m_busy(false)
{
    m_SPI = spiPtr;
}

/**
 * Component hard reset
 *
 * The spec says "the device will take approximately tRST=30 microseconds to
 * reset.
 */
void IUSPIFlash::hardReset()
{
    beginTransaction();
    m_SPI->write(CMD_RESET_DEVICE);
    endTransaction();
    delayMicroseconds(50);
    while(readStatus() & STATUS_WIP);
    {
        // Wait for the hard reset to finish
    }
}

/**
 *
 */
void IUSPIFlash::begin()
{
    pinMode(m_csPin, OUTPUT);
    digitalWrite(m_csPin, HIGH);
    m_SPI->begin();
    m_begun = true;
}


/***** Config read / write functions *****/

/**
 *
 */
size_t IUSPIFlash::readConfig(storedConfig configType, char *config,
                              size_t maxLength)
{
    // TODO Implement
    return 0;
}

/**
 *
 */
size_t IUSPIFlash::writeConfig(storedConfig configType, char *config,
                               size_t len)
{
    // TODO Implement
    return 0;
}

/**
 * Delete the config.
 */
bool IUSPIFlash::deleteConfig(storedConfig configType)
{
    // TODO Implement
    return false;
}


/***** JSON Config load / save functions *****/

bool IUSPIFlash::saveConfigJson(storedConfig configType, JsonVariant &config)
{
    // TODO Implement
    return false;
}


/***** SPI utilities *****/

/**
 * Begin the SPI transaction and select the flash memory chip (with SS/CS pin)
 *
 * CS pin is set to LOW to select the flash memory chip.
 */
void IUSPIFlash::beginTransaction(bool waitIfBusy)
{
    if (waitIfBusy)
    {
        waitForAvailability();
    }
    m_SPI->beginTransaction(m_spiSettings);
    digitalWrite(m_csPin, LOW);
}

/**
 * Unselect the SPI slave (SS/CS pin)  and end the SPI transaction
 *
 * CS PIN is set to HIGH to unselect the flash memory chip.
 */
 void IUSPIFlash::endTransaction(bool waitForCompletion)
{
    digitalWrite(m_csPin, HIGH);
    m_SPI->endTransaction();
    if (waitForCompletion)
    {
        waitForAvailability();
    }
}

/**
 * Read and return the flash status (0 if available, else busy)
 */
uint8_t IUSPIFlash::readStatus()
{
    uint8_t c;
    beginTransaction(false);  // !!! Don't wait if busy !!!
    m_SPI->transfer(CMD_READ_STATUS_REG);
    c = m_SPI->transfer(0x00);
    endTransaction();
    return(c);
}

/**
 * Function returns only when the SPI Flash is available (holds the process)
 */
void IUSPIFlash::waitForAvailability()
{
    if(m_busy) {
        while(readStatus() & STATUS_WIP) {
            // wait
        }
        m_busy = false;
    }
}


/***** Address Utilities *****/

/**
 * Return the index of the sector / block containing the given page
 */
uint16_t IUSPIFlash::getBlockIndex(pageBlockTypes blockType,
                                   uint16_t pageIndex)
{
    switch (blockType)
    {
        case SECTOR:
            return pageIndex / SECTOR_PAGE_COUNT;
        case BLOCK_32KB:
            return pageIndex / BLOCK_32KB_PAGE_COUNT;
        case BLOCK_64KB:
            return pageIndex / BLOCK_64KB_PAGE_COUNT;
        default:
            if (debugMode) {
                debugPrint(F("Unknown sector / block type"));
            }
            return -1;
    }
}

uint16_t IUSPIFlash::getBlockFirstPage(pageBlockTypes blockType,
                                       uint16_t blockIndex)
{
    switch (blockType)
    {
        case SECTOR:
            return blockIndex * SECTOR_PAGE_COUNT;
        case BLOCK_32KB:
            return blockIndex * BLOCK_32KB_PAGE_COUNT;
        case BLOCK_64KB:
            return blockIndex * BLOCK_64KB_PAGE_COUNT;
        default:
            if (debugMode) {
                debugPrint(F("Unknown sector / block type"));
            }
            return -1;
    }
}

/**
 * Convert a page number to a 24-bit address
 *
 * @param pageIndex The page number
 */
uint32_t IUSPIFlash::getAddressFromPage(uint16_t pageIndex)
{
  return ((uint32_t) pageIndex) << 8;
}

/**
 * Convert a 24-bit address to a page number
 *
 * @param addr The address
 */
uint16_t IUSPIFlash::getPageFromAddress(uint32_t addr)
{
  return (uint16_t) (addr >> 8);
}


/***** Read / write functions *****/

/**
 * Read the flash memory chip ID
 *
 * @param destination A destination unsigned char array, to receive the read ID
 * @param destinationCount The length of destination (= the expected length of
 *  the ID), usually 3.
 */
void IUSPIFlash::readId(uint8_t *destination, uint8_t destinationCount)
{
    beginTransaction();
    m_SPI->transfer(CMD_READ_ID);
    for(uint8_t i = 0; i < destinationCount; ++i)
    {
        destination[i] = m_SPI->transfer(0x00);
    }
    endTransaction();
}

/**
 * Erase the whole flash memory chip
 */
void IUSPIFlash::eraseChip(bool wait)
{
    beginTransaction();
    m_SPI->transfer(CMD_WRITE_ENABLE);
    digitalWrite(CSPIN, HIGH);
    digitalWrite(CSPIN, LOW);
    m_SPI->transfer(CMD_CHIP_ERASE);
    endTransaction();
    m_busy = true;
    if (wait)
    {
        waitForAvailability();
    }
}

/**
 * Erase all the pages in the sector / block containing given page number
 *
 * Tse Typ=0.6sec Max=3sec, measured at measured 549.024ms for SECTOR.
 * The smallest unit of memory which can be erased is the 4kB sector
 * (which is 16 pages).
 *
 * @param blockType The type of page block to erase (pages can only be erased by
 *  sector or block)
 * @param pageIndex The page number
 */
void IUSPIFlash::erasePages(IUSPIFlash::pageBlockTypes blockType,
                            uint16_t pageIndex)
{
    uint8_t eraseCommand;
    switch (blockType) {
        case SECTOR:
            eraseCommand = CMD_SECTOR_ERASE;
            break;
        case BLOCK_32KB:
            eraseCommand = CMD_BLOCK32K_ERASE;
            break;
        case BLOCK_64KB:
            eraseCommand = CMD_BLOCK64K_ERASE;
            break;
        default:
            if (debugMode) {
                debugPrint(F("Unknown sector / block type"));
            }
            return;
    }
    uint32_t address = getAddressFromPage(pageIndex);
    beginTransaction();
    m_SPI->transfer(CMD_WRITE_ENABLE);
    digitalWrite(CSPIN, HIGH);
    digitalWrite(CSPIN, LOW);
    m_SPI->transfer(eraseCommand);
    // Send the 3 byte address
    m_SPI->transfer((address >> 16) & 0xff);
    m_SPI->transfer((address >> 8) & 0xff);
    m_SPI->transfer(address & 0xff);
    endTransaction();
    m_busy = true;
}

/**
 * Program (write) the given content in a page
 *
 * NB: To program a page, it must have be erased beforehand.
 * A page contains 256 bytes, ie 256 chars or uint8_t.
 *
 * @param content A pointer to a byte array of length 256
 * @param pageIndex The page number
 */
void IUSPIFlash::programPage(uint8_t *content, uint16_t pageIndex)
{
    uint32_t address = getAddressFromPage(pageIndex);
    // Enable write
    beginTransaction();
    m_SPI->transfer(CMD_WRITE_ENABLE);
    endTransaction();
    // Program the page
    beginTransaction(false);
    m_SPI->transfer(CMD_PAGE_PROGRAM);
    m_SPI->transfer((address >> 16) & 0xFF);
    m_SPI->transfer((address >> 8) & 0xFF);
    m_SPI->transfer(address & 0xFF);
    for(uint16_t i = 0; i < 256; i++)
    {
        m_SPI->transfer(content[i]);
    }
    endTransaction();
    m_busy = true;
}

/**
 * Read one or several successive page(s) into given content byte array
 *
 * A page contains 256 bytes, ie 256 chars or uint8_t.
 *
 * @param content A pointer to a byte array, to receive the
 *  content of the read pages. Its length should be pageCount * 256.
 * @param pageIndex The page number
 * @param pageCount The number of successive pages to read
 * @param highSpeed If true, function will use the high speed read flash
 *  command, else will use the default speed read flash command.
 */
void IUSPIFlash::readPages(uint8_t *content, uint16_t pageIndex,
                           const uint16_t pageCount, bool highSpeed)
{
    uint32_t address = getAddressFromPage(pageIndex);
    beginTransaction();
    if (highSpeed)
    {
        m_SPI->transfer(CMD_READ_HIGH_SPEED);
    }
    else
    {
        m_SPI->transfer(CMD_READ_DATA);
    }
    m_SPI->transfer((address >> 16) & 0xFF);
    m_SPI->transfer((address >> 8) & 0xFF);
    m_SPI->transfer(address & 0xFF);
    if (highSpeed)
    {
        m_SPI->transfer(0);  // send dummy byte
    }
    uint32_t byteCount = (uint32_t) pageCount * 256;
    for(uint32_t i = 0; i < byteCount; ++i) {
        content[i] = m_SPI->transfer(0);
    }
    endTransaction();
}

