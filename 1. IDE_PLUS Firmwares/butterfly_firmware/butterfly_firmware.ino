/*
Infinite Uptime IDE+ Firmware
Vr. 1.1.2
Update 22-06-2019
Type - Standard Firmware Release
*/

/* =============================================================================
    Library imports
============================================================================= */
// Uart driver update at /home/vikas/.arduino15/packages/grumpyoldpizza/hardware/stm32l4/0.0.28/cores/stm32l4/Uart.h RX buffer from 64 Bytes to 512 Bytes
#include "BoardDefinition.h"
#include "Conductor.h"
#include "FFTConfiguration.h"

#include <MemoryFree.h>
#include <Timer.h>
#include <FS.h>
//#include"IUTimer.h"

const uint8_t ESP32_IO0  =  7;  // IDE1.5_PORT_CHANGE

#ifdef DRAGONFLY_V03
#else
    // FIXME For some reason, if this is included in conductor,
    // it blocks the I2S callback
    #include "IUFlash.h"
    IUSPIFlash iuFlash(&SPI, A1, SPISettings(50000000, MSBFIRST, SPI_MODE0));
#endif


/* =============================================================================
    MAC Address
============================================================================= */

 //const char MAC_ADDRESS[18] = "9C:A5:25:86:34:6E";

/* Motor Scaling Factor 
 *  
 */
 
float motorScalingFactor = 1.0;

/* =============================================================================
    Unit test includes
============================================================================= */

#ifdef UNITTEST
    #include <UnitTest/Test_IUSerial.h>
    #include "UnitTest/Test_Component.h"
    #include "UnitTest/Test_FeatureClass.h"
    #include "UnitTest/Test_FeatureComputer.h"
    #include "UnitTest/Test_FeatureGroup.h"
    #include "UnitTest/Test_Sensor.h"
    #include "UnitTest/Test_Utilities.h"
#endif

#ifdef INTEGRATEDTEST
    #include "IntegratedTest/IT_All_Conductor.h"
    #if defined(BUTTERFLY_V03) || defined(BUTTERFLY_V04)
        #include "IntegratedTest/IT_Butterfly.h"
    #elif defined(DRAGONFLY_V03)
        #include "IntegratedTest/IT_Dragonfly.h"
    #endif
#endif


/* =============================================================================
    Configuration Variables
============================================================================= */

/***** Accelerometer Calibration parameters *****/

float ACCEL_RMS_SCALING[3] = {
    1.00, // Axis X
    1.00, // Axis Y
    1.00, // Axis Z
};
float VELOCITY_RMS_SCALING[3] = {
    1.00, // Axis X
    1.00, // Axis Y
    1.00, // Axis Z
};
float DISPLACEMENT_RMS_SCALING[3] = {
    1.00, // Axis X
    1.00, // Axis Y
    1.00, // Axis Z
};


/***** Acceleration Energy 512 default thresholds *****/

float DEFAULT_ACCEL_ENERGY_NORMAL_TH = 110;
float DEFAULT_ACCEL_ENERGY_WARNING_TH = 130;
float DEFAULT_ACCEL_ENERGY_HIGH_TH = 150;

/***** Audio DB calibration parameters *****/

// The output Audio level in dB  can be adjusted to account for sound dampening
// when the device enclosure is sealed.
// The raw audio level in dB is multiplied by AUDIO_DB_SCALING, and then
// is added AUDIO_DB_OFFSET, to produce the final output Audio dB.
float AUDIO_DB_SCALING = 1.0;
float AUDIO_DB_OFFSET = 0.0;
float audioHigherCutoff = 160.0;

/* =============================================================================
    Main global variables
============================================================================= */

/***** Debbugging variables *****/

bool doOnceFWValid = true;
int FWValidDelCnt = 0;
char FW_Valid_State = 0;

bool doOnce = true;
uint32_t interval = 30000;
uint32_t lastDone = 0;


/***** Main operator *****/

Conductor conductor;//(MAC_ADDRESS);


/* =============================================================================
    Led timers and callbacks
============================================================================= */

static armv7m_timer_t RGBHighFreqBlinker;

static void blinkRGBAtHighFreq(void) {
    rgbLed.manageHighFreqBlinking();
    armv7m_timer_start(&RGBHighFreqBlinker, 1);
}

void onWiFiConnect() {
    ledManager.setBaselineStatus(&STATUS_WIFI_CONNECTED);
}

void onWiFiDisconnect() {
    ledManager.setBaselineStatus(&STATUS_NO_STATUS);
}
void onBLEConnect() {
    //TODO Show that BLE is connected on the LED
}

void onBLEDisconnect() {
    //TODO Show that BLE is disconnected on the LED
}

void operationStateCallback(Feature *feature) {
    q15_t value = feature->getLastRecordedQ15Values()[0];
    //Serial.print("Ops State Call Back:");Serial.println(value);
    if (value < OperationState::COUNT) {
        ledManager.showOperationState((uint8_t) value);
    }
}


/* =============================================================================
    Watch Dogs
============================================================================= */

static armv7m_timer_t watchdogTimer;
uint32_t lastActive = 0;
uint32_t loopTimeout = 60000;  // 1min timeout
uint32_t oneDayTimeout = 86400000;

static void watchdogCallback(void) {
    uint32_t now = millis();
    if (now > oneDayTimeout ||
        (lastActive > 0 && now - lastActive > loopTimeout))
    {
        Serial.println("STM Watchdog Reset !");
        delay(100);
        STM32.reset();
    }
    if (iuWiFi.arePublicationsFailing()) {
        //Ensure your PubSubClient Arduino library version is 2.7
        debugPrint("Publications are failing: hard resetting now.");
        if(conductor.isBLEConnected()) {
           iuBluetooth.write("WIFI-DISCONNECTED;");
        }
        Serial.println("Pub Failed !");
        iuWiFi.hardReset();
    }
    armv7m_timer_start(&watchdogTimer, 1000);
}




/* =============================================================================
    BLE transmission with throughput control
============================================================================= */

static armv7m_timer_t bleTransmitTimer;

static void bleTransmitCallback(void) {
    iuBluetooth.bleTransmit();
    armv7m_timer_start(&bleTransmitTimer, 5);
}


/* =============================================================================
 *  Read HTTP pending config messages using timer
 * ============================================================================*/

static armv7m_timer_t httpConfigTimer;

static void httpConfigCallback(void) {
    //iuBluetooth.bleTransmit();
    //Serial.println("HIT HTTP CONFIG....................................................");
    iuWiFi.sendMSPCommand(MSPCommand::GET_PENDING_HTTP_CONFIG);
    armv7m_timer_start(&httpConfigTimer, 180000);   // 3 min  180000
}

/* ================================================================================
 * Ethernet Status Timer callback
 * ===============================================================================*/

static armv7m_timer_t ethernetStatusTimer;
static void ethernetStatusCallback(void){

    iuEthernet.isEthernetConnected = iuEthernet.TCPStatus();
    //iuEthernet.ExitAT();
    armv7m_timer_start(&ethernetStatusTimer, 2000);    
}
/* =============================================================================
    Driven sensors acquisition callback
============================================================================= */

/**
 * This function will be called every time the Microphone sends an interrupt.
 *
 * The interrupt is raised every time the raw sound data buffer is full. The
 * interrupt frequency then depends on the Microphone (here I2S) clock rate and
 * on the size of the buffer.
 * NB: Printing is time consuming and may cause issues in callback. Always
 * deactivate the asyncDebugMode in prod.
 */
void dataAcquisitionCallback()
{
    uint32_t startT = 0;
    if (asyncDebugMode) {
        startT = micros();
    }
    
    conductor.acquireData(true);
    
    if (asyncDebugMode) {
        debugPrint(micros() - startT);
    }
}

/* =============================================================================
    Accelerometer Interrupt Service Routine for Data acquisition on callback
============================================================================= */

/**
 * This function will be called every time the Accelerometer(LSM6DSM) sends an Data 
 * Ready interrupt on STM32 GPIO.
 *
 * The interrupt is raised every time the raw sound data is ready. The
 * interrupt frequency then depends on the sensor ODR (Output Data Rate) sampling frequency. 
 * 
 * NB: Printing is time consuming and may cause issues in ISRs/callback. Always
 * deactivate the asyncDebugMode in prod.
 */
void dataAcquisitionISR()
{

    // digitalWrite(A3,HIGH);
    conductor.acquireData(true);
    //   digitalWrite(A3,LOW);
}


/* =============================================================================
    Interface message processing callbacks
============================================================================= */

void onNewUSBMessage(IUSerial *iuSerial) {
    conductor.processUSBMessage(iuSerial);
}

void onNewBLEMessage(IUSerial *iuSerial) {
    conductor.processBLEMessage(iuSerial);
}

void onNewWiFiMessage(IUSerial *iuSerial) {
    conductor.processWiFiMessage(iuSerial);
}

void onNewEthernetMessage(IUSerial *iuSerial){
    conductor.processWiFiMessage(iuSerial);
}
/* =============================================================================
    Microsecond Timer ISR
============================================================================= */


#include "stm32l4xx.h"
#include "stm32l4_timer.h"

/* TIM_Interrupt_definition TIM Interrupt Definition */
#define TIM_IT_UPDATE           (TIM_DIER_UIE)
#define TIM_IT_CC1              (TIM_DIER_CC1IE)
#define TIM_IT_CC2              (TIM_DIER_CC2IE)
#define TIM_IT_CC3              (TIM_DIER_CC3IE)
#define TIM_IT_CC4              (TIM_DIER_CC4IE)
#define TIM_IT_COM              (TIM_DIER_COMIE)
#define TIM_IT_TRIGGER          (TIM_DIER_TIE)
#define TIM_IT_BREAK            (TIM_DIER_BIE)



#define BIT(type, n)                 ( (type)1 << n )
#define BIT_SET(type, reg, mask)     ( reg |= (type)mask )
#define BIT_CLEAR(type, reg, mask)   ( reg &= ~((type)mask) )
#define REG_READ(REG)                ((REG))   
#define REG_WRITE(REG, VAL)          ((REG) = (VAL))
#define REG_MODIFY(REG, CLEARMASK, SETMASK)  REG_WRITE((REG), \
                               (((REG_READ(REG)) & (~(CLEARMASK))) | (SETMASK)))
#define IS_BIT_CLEAR(type, reg, n)   ( (reg & BIT(type, n)) == 0 )
#define IS_BIT_SET(type, reg, n)     ( (reg & BIT(type, n)) != 0 ) 

stm32l4_timer_t timerIns;

static int isrPeriod = 300;  //conductor.timerISRPeriod          // in microseconds (10 us resolution) 450(2.2K), 420(2.38KHz)

void timerISR(void *context, uint32_t events)
{
  static uint8_t temp = 0;
  if(isrPeriod != 300){
    timerIns.state = TIMER_STATE_INIT;  
    timerInit();
    //Serial.print("isrPeriod :");Serial.println(isrPeriod);
  
  }
  //digitalWrite(6,HIGH);
  if(((TIM5->SR & TIM_SR_UIF) == TIM_SR_UIF) != 0)
  {
    if(IS_BIT_SET(uint32_t, TIM5->DIER, TIM_DIER_UIE_Pos))
    {
      TIM5->SR = ~TIM_IT_UPDATE;
    }
  }
  //User Code Here
  //dataAcquisitionCallback();
  //digitalWrite(6,LOW);

// ISR Handler 1 - ON, 0 -OFF  
#if 1
if(temp == 0)
  {
    // digitalWrite(6, HIGH);
    dataAcquisitionCallback();      // data acquisition callback 
    temp = 1;
  }
  else if(temp == 1)
  {
    // digitalWrite(6, LOW);
    temp = 0;
  }      
#endif

}

void timerInit(void)
{
  if (timerIns.state == TIMER_STATE_NONE) {
  stm32l4_timer_create(&timerIns, TIMER_INSTANCE_TIM5, 3, 0);  // 3 - Priority , 0 -  Mode
  }
  if (timerIns.state == TIMER_STATE_INIT) {
    /* clock is running on 10Mhz, you need to change period according to the requirement, i.e. 
     *  10000000 for 1 second, 
     *  10000    for 1 ms 
     *  10       for 1 micro second
     */ 
    stm32l4_timer_enable(&timerIns, (stm32l4_timer_clock(&timerIns) / 10000000) -1, 5*isrPeriod /* change this */, TIMER_OPTION_COUNT_UP, (stm32l4_timer_callback_t)timerISR, NULL, TIMER_EVENT_PERIOD); // xyz ISR function callback
  }
  stm32l4_timer_start(&timerIns, 0);
}



/* =============================================================================
    Main execution
============================================================================= */

void setup()
{   
  
  pinMode(ESP32_IO0,OUTPUT); // IDE1.5_PORT_CHANGE
//   pinMode(6,OUTPUT); 
//   pinMode(A3,OUTPUT);  // ISR (ODR checked from pin 50)
  digitalWrite(ESP32_IO0,HIGH); // IDE1.5_PORT_CHANGE
  DOSFS.begin();
  #if 1
    
    
    iuUSB.begin();
    iuUSB.setOnNewMessageCallback(onNewUSBMessage);
    rgbLed.setup();
    ledManager.setBaselineStatus(&STATUS_NO_STATUS);
    ledManager.overrideColor(RGB_WHITE);
    armv7m_timer_create(&RGBHighFreqBlinker, (armv7m_timer_callback_t)blinkRGBAtHighFreq);
    armv7m_timer_start(&RGBHighFreqBlinker, 20);
    #if defined(UNITTEST) || defined(COMPONENTTEST) || defined(INTEGRATEDTEST)
        delay(2000);
        iuI2C.begin();
        ledManager.resetStatus();
    #else
        armv7m_timer_create(&watchdogTimer, (armv7m_timer_callback_t)watchdogCallback);
        armv7m_timer_start(&watchdogTimer, 1000);
        if (debugMode) {
            delay(5000);
            debugPrint(F("Start - Mem: "), false);
            debugPrint(String(freeMemory(), DEC));
        }
        iuI2C.begin();
        // Interfaces
        if (debugMode) {
            debugPrint(F("\nInitializing interfaces..."));
        }
        // BLE SETUP BEGIN
        iuBluetooth.setupHardware();
        debugPrint(" Is BLE Chip Available?:",false);
        debugPrint(iuBluetooth.isBLEAvailable);
        
        if(iuBluetooth.isBLEAvailable){
            iuBluetooth.setOnNewMessageCallback(onNewBLEMessage);
            
            armv7m_timer_create(&bleTransmitTimer, (armv7m_timer_callback_t)bleTransmitCallback);
            armv7m_timer_start(&bleTransmitTimer, 5);
             // set the BLE address for conductor
            conductor.setConductorMacAddress();          
        }
        if(!iuBluetooth.isBLEAvailable) {   // BLE Hardware is Not available
            // Read the configurations over httpClient
            String availableOnpremConfigs = iuEthernet.getServerConfiguration();
            debugPrint("Available On-Prem Configs:",true);
            debugPrint(availableOnpremConfigs);
            debugPrint("JSON timeout:",false);
            debugPrint(iuEthernet.responseIsNotAvailabel);

            bool isDataWriteComplete = false;
            // SETUP available Ethernet configurations
            if ( (DOSFS.exists("relayAgentConfig.conf") && availableOnpremConfigs[0] == '{'  )  || ( !DOSFS.exists("relayAgentConfig.conf") && availableOnpremConfigs[0] == '{' )
                   || (DOSFS.exists("relayAgentConfig.conf") && iuEthernet.responseIsNotAvailabel && availableOnpremConfigs == NULL )   )
            {   
                debugPrint("__________________Init Ethernet Config__________________________",true);
               if( availableOnpremConfigs[0] == '{'){
                // Write configuration to file
                File storeConfig = DOSFS.open("relayAgentConfig.conf","w");
                if(storeConfig){
                    if(debugMode){
                        debugPrint("Writing configuration into File");
                    }    
                    storeConfig.print(availableOnpremConfigs);
                    storeConfig.close();
                    isDataWriteComplete = true;
                }else
                {
                    if (debugMode)
                    {
                        debugPrint("Failed to Write into File");
                    }
                    
                }
               }

                if(isDataWriteComplete == true || iuEthernet.responseIsNotAvailabel ){
                    debugPrint("Content From File:");
                    conductor.setEthernetConfig("relayAgentConfig.conf");       // Handle file not available condition     
                    
                    debugPrint("Setting up the Ethernet hardware");
                    iuEthernet.setupHardware();
                    
                    iuEthernet.setOnNewMessageCallback(onNewEthernetMessage);
                }
                if (!iuBluetooth.isBLEAvailable)
                {  // set the BLE address for conductor
                    conductor.setConductorMacAddress();
                }
                
                //armv7m_timer_create(&ethernetStatusTimer, (armv7m_timer_callback_t)ethernetStatusCallback);
                //armv7m_timer_start(&ethernetStatusTimer, 30000);   // 30 sec
        
                debugPrint("___________________Done Ethernet Init ______________________________",true);
                 
            }else
            {
                debugPrint("iurelayAgent config file does not exists and configJSON is not available");
            }
            
        }else 
        {
            debugPrint("BLE Chip is Available, BLE init Complete");
        }
 //   IDE1.5_PORT_CHANGE - Disabled for testing. This Req is causing ESP32 Reset.         
        // httpConfig message read timerCallback
 //       armv7m_timer_create(&httpConfigTimer, (armv7m_timer_callback_t)httpConfigCallback);
 //       armv7m_timer_start(&httpConfigTimer, 180000);   // 3 min Timer 180000
        
        // WIFI SETUP BEGIN
        iuWiFi.setupHardware();
        iuWiFi.setOnNewMessageCallback(onNewWiFiMessage);
        iuWiFi.setOnConnect(onWiFiConnect);
        iuWiFi.setOnDisconnect(onWiFiDisconnect);
       
        if (setupDebugMode) {
            iuI2C.scanDevices();
            debugPrint("");
        }
        if (debugMode) {
            debugPrint(F("=> Successfully initialized interfaces - Mem: "),
                       false);
            debugPrint(String(freeMemory(), DEC));
        }
        // Default feature configuration
        if (debugMode) {
            debugPrint(F("\nSetting up default feature configuration..."));
        }
        //conductor.setCallback(dataAcquisitionCallback);
        setUpComputerSources();
        populateFeatureGroups();
        if (debugMode) {
            debugPrint(F("=> Succesfully configured default features - Mem: "),
                       false);
            debugPrint(String(freeMemory(), DEC));
        }

        iuFlash.begin();

        // Update the configuration of FFT computers from fft.conf
        if(conductor.setFFTParams()) {
            if(setupDebugMode) {
                debugPrint("Updated FFT configuration from file");
                //TODO: Expose the current FFT config
            }
        } else {
            if(setupDebugMode) {
                debugPrint("Using defaults for FFT Computers", false);
                debugPrint(": samplingRate = ", false); debugPrint(FFTConfiguration::DEFAULT_SAMPLING_RATE, false);
                debugPrint(": block size = ", false); debugPrint(FFTConfiguration::DEFAULT_BLOCK_SIZE, false);
            }
        }

        // Sensors
        if (debugMode) {
            debugPrint(F("\nInitializing sensors..."));
        }
        uint16_t callbackRate = iuI2S.getCallbackRate();
        for (uint8_t i = 0; i < Sensor::instanceCount; ++i) {
            Sensor::instances[i]->setupHardware();
            if (Sensor::instances[i]->isHighFrequency()) {
                Sensor::instances[i]->setCallbackRate(callbackRate);
            }
        }
        #ifdef BUTTERFLY_V04
            iuGyroscope.setPowerMode(PowerMode::SUSPEND);
        #endif
        if (debugMode) {
            debugPrint(F("=> Succesfully initialized sensors - Mem: "),
                       false);
            debugPrint(String(freeMemory(), DEC));
        }
        if (setupDebugMode) {
            conductor.exposeAllConfigurations();
            if (iuI2C.isError()) {
                debugPrint(F("\nI2C Satus: Error"));
            } else {
                debugPrint(F("\nI2C Satus: OK"));
            }
            debugPrint(F("\n***Finished setup at (ms): "), false);
            debugPrint(millis(), false);
            debugPrint(F("***\n"));
        }
        // Start flash and load configuration files
        // if (!USBDevice.configured())
        // {
        // WiFi configuration
//        conductor.configureFromFlash(IUFlash::CFG_WIFI0);
        // Feature, FeatureGroup and sensors coonfigurations
        for (uint8_t i = 0; i < conductor.CONFIG_TYPE_COUNT; ++i) {
            conductor.configureFromFlash(conductor.CONFIG_TYPES[i]);
            // For "device.conf", value for "DSP" (dataSendPeriod) will be saved from v1.1.3
            // Devices with v1.1.2 will not apply this value while loading configuration from flash.
        }
        if (setupDebugMode) {
            ledManager.overrideColor(RGB_PURPLE);
            delay(5000);
            ledManager.stopColorOverride();
        }
        // } else if (setupDebugMode) {
        ledManager.overrideColor(RGB_ORANGE);
        delay(5000);
        ledManager.stopColorOverride();
        // }
        delay(5000);
        //configure mqttServer
        conductor.configureMQTTServer("MQTT.conf");
        //http configuration
        conductor.configureBoardFromFlash("httpConfig.conf",1);
        // get the previous offset values 
        conductor.setSensorConfig("sensorConfig.conf"); 
        delay(500);
        iuWiFi.hardReset();
        delay(1000);
        conductor.configureFromFlash(IUFlash::CFG_WIFI0);
        delay(100);
        opStateFeature.setOnNewValueCallback(operationStateCallback);
        ledManager.resetStatus();
        conductor.changeUsageMode(UsageMode::OPERATION);
        /* code uncommented */
        // pinMode(IULSM6DSM::INT1_PIN, INPUT);
        // attachInterrupt(IULSM6DSM::INT1_PIN, dataAcquisitionISR, RISING);
        pinMode(IUKX222::INT1_PIN, INPUT);
        attachInterrupt(IUKX222::INT1_PIN, dataAcquisitionISR, RISING);
        // debugPrint(F("ISR PIN:"));debugPrint(IULSM6DSM::INT1_PIN);

        //Resume previous operational state of device
        conductor.setThresholdsFromFile();
                
        
                
        // Timer Init
        //timerInit();
        
    #endif
 #endif   
}

/**
 * 
 */
void loop()
{   
    #if 1
    isrPeriod = conductor.timerISRPeriod;
    lastActive = millis();
    #if defined(UNITTEST) || defined(COMPONENTTEST) || defined(INTEGRATEDTEST)
        Test::run();
        if (Test::getCurrentFailed() > 0) {
            m_ledStrip.overrideColor(RGB_RED);
        } else {
            m_ledStrip.overrideColor(RGB_GREEN);
        }
    #else
        // if (loopDebugMode) {
            if (doOnce) {
                doOnce = false;
                /* === Place your code to excute once here ===*/
                if(iuEthernet.isEthernetConnected == 0) {
                    ledManager.showStatus(&STATUS_WIFI_CONNECTED);
                }
                /*======*/
                debugPrint("CURRENT FFT CONFIGURATION: ");
                debugPrint("Current samplingRate: ", false); debugPrint(FFTConfiguration::currentSamplingRate);
                debugPrint("Current blockSize: ", false); debugPrint(FFTConfiguration::currentBlockSize);
                debugPrint("Current lowCutOffFrequency: ", false); debugPrint(FFTConfiguration::currentLowCutOffFrequency);
                debugPrint("Current highCutOffFrequency: ", false); debugPrint(FFTConfiguration::currentHighCutOffFrequency);
                debugPrint("Current minAgitation: ", false); debugPrint(FFTConfiguration::currentMinAgitation);
            }
        // }
       
        conductor.manageSleepCycles();
        // Receive messages & configurations
        iuUSB.readMessages();
        iuBluetooth.readMessages();
        if (iuBluetooth.isBLEAvailable) //  iuEthernet.isEthernetConnected :0 -> connected, 1-> not connected
        {
            iuWiFi.readMessages();
        }else {
            iuEthernet.readMessages();
        }
        // Manage WiFi autosleep
        iuWiFi.manageAutoSleep();
        // Acquire data from sensors
        conductor.acquireData(false);
        // Compute features depending on operation mode
        conductor.computeFeatures();
        // Stream features
        conductor.streamFeatures();
        // Send accel raw data
        conductor.periodicSendAccelRawData();
        // Send config checksum
        conductor.periodicSendConfigChecksum();
        ledManager.updateColors();
        uint32_t now = millis();
        if (now - lastDone > interval) {
            lastDone = now;
            /* === Place your code to excute at fixed interval here ===*/
            conductor.streamMCUUInfo(iuWiFi.port);
            /*======*/
        }

        if (millis() - conductor.lastTimeSync > conductor.m_connectionTimeout ) {

            if(iuEthernet.isEthernetConnected == 0) {
                iuEthernet.isEthernetConnected = 1;
                ledManager.showStatus(&STATUS_NO_STATUS);
            }
        }

        // Consume ready segmented message
        char configMessageFromBLE[MESSAGE_LENGTH+1];
        if (conductor.consumeReadySegmentedMessage(configMessageFromBLE)) {
            // TODO: if all messages [0->MAX_SEGMENTED_MESSAGES-1] are ready, the later messages
            // might time out which the first few messages are being consumed. Add logic to 
            // extend timeout for later messages if former messages are being consumed.
            #ifdef IU_DEBUG_SEGMENTED_MESSAGES
            debugPrint("DEBUG: LOOP: configMessageFromBLE: ", false); debugPrint(configMessageFromBLE);
            #endif
            conductor.processConfiguration(configMessageFromBLE, true);
        }        

        // Clean consumed segmented messages
        conductor.cleanConsumedSegmentedMessages();

        // Clean timed out segmented messages
        conductor.cleanTimedoutSegmentedMessages();

        // Manage raw data sending depending on RawDataState::startRawDataTransmission and RawDataState::rawDataTransmissionInProgress
        conductor.manageRawDataSending();
#if 0 // FW Validation
        if(doOnceFWValid == true)
        {
            if((FWValidDelCnt % 2000) == 0 && FWValidDelCnt > 0)
            {
                yield();
                uint32_t ret = 0;
                ret = conductor.FW_Validation();
                if(ret != 0)
                {// Waiting for WiFi Disconnect/Connect Cycle.
                    doOnceFWValid = true;
                    FWValidDelCnt = 1;                    
                }
                else if(ret == 0)
                    doOnceFWValid = false;
 //               FWValidDelCnt++;
            }
            else
            {
                FWValidDelCnt++;
            }                  
        }
#endif
        yield();
       
    #endif
  #endif  
}
