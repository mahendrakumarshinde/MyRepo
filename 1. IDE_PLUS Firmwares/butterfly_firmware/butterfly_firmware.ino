/*
Infinite Uptime BLE Module Firmware

Update 2017-10-10
*/

/* =============================================================================
    Library imports
============================================================================= */

#include "BoardDefinition.h"
#include "Conductor.h"

#include <MemoryFree.h>
#include <Timer.h>
#include <FS.h>

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

const char MAC_ADDRESS[18] = "94:54:93:10:63:DE";


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


/***** Accelerometer Feature computation parameters *****/

uint16_t DEFAULT_LOW_CUT_FREQUENCY = 5;  // Hz
uint16_t DEFAULT_HIGH_CUT_FREQUENCY = 500;  // Hz
float DEFAULT_MIN_AGITATION = 0.03;


/***** Audio DB calibration parameters *****/

// The output Audio level in dB  can be adjusted to account for sound dampening
// when the device enclosure is sealed.
// The raw audio level in dB is multiplied by AUDIO_DB_SCALING, and then
// is added AUDIO_DB_OFFSET, to produce the final output Audio dB.
float AUDIO_DB_SCALING = 1.0;
float AUDIO_DB_OFFSET = 0.0;


/* =============================================================================
    Main global variables
============================================================================= */

/***** Debbugging variables *****/

bool doOnce = true;
uint32_t interval = 30000;
uint32_t lastDone = 0;


/***** Main operator *****/

Conductor conductor(MAC_ADDRESS);


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
        STM32.reset();
    }
    if (iuWiFi.arePublicationsFailing()) {
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


/* =============================================================================
    Main execution
============================================================================= */

void setup()
{
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
        iuBluetooth.setupHardware();
        iuBluetooth.setOnNewMessageCallback(onNewBLEMessage);
        armv7m_timer_create(&bleTransmitTimer, (armv7m_timer_callback_t)bleTransmitCallback);
        armv7m_timer_start(&bleTransmitTimer, 5);
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
        conductor.setCallback(dataAcquisitionCallback);
        setUpComputerSources();
        populateFeatureGroups();
        if (debugMode) {
            debugPrint(F("=> Succesfully configured default features - Mem: "),
                       false);
            debugPrint(String(freeMemory(), DEC));
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
        if (!USBDevice.configured())
        {
            iuFlash.begin();
            // WiFi configuration
            conductor.configureFromFlash(IUFlash::CFG_WIFI0);
            // Feature, FeatureGroup and sensors coonfigurations
            for (uint8_t i = 0; i < conductor.CONFIG_TYPE_COUNT; ++i) {
                conductor.configureFromFlash(conductor.CONFIG_TYPES[i]);
            }
            if (setupDebugMode) {
                ledManager.overrideColor(RGB_PURPLE);
                delay(5000);
                ledManager.stopColorOverride();
            }
        } else if (setupDebugMode) {
            ledManager.overrideColor(RGB_ORANGE);
            delay(5000);
            ledManager.stopColorOverride();
        }
        opStateFeature.setOnNewValueCallback(operationStateCallback);
        ledManager.resetStatus();
        conductor.changeUsageMode(UsageMode::OPERATION);
    #endif
}

/**
 * 
 */
void loop()
{
    lastActive = millis();
    #if defined(UNITTEST) || defined(COMPONENTTEST) || defined(INTEGRATEDTEST)
        Test::run();
        if (Test::getCurrentFailed() > 0) {
            m_ledStrip.overrideColor(RGB_RED);
        } else {
            m_ledStrip.overrideColor(RGB_GREEN);
        }
    #else
        if (loopDebugMode) {
            if (doOnce) {
                doOnce = false;
                /* === Place your code to excute once here ===*/
                
                /*======*/
            }
        }
        // Manage power saving
        conductor.manageSleepCycles();
        // Receive messages & configurations
        iuUSB.readMessages();
        iuBluetooth.readMessages();
        iuWiFi.readMessages();
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
        yield();
    #endif
}

