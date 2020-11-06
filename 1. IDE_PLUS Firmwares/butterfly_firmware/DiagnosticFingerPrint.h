#ifndef DIAGNOSTICFINGERPRINT_H
#define DIAGNOSTICFINGERPRINT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "FeatureUtilities.h"
#include "FeatureClass.h"
#include "IUFlash.h"
#include <IUSerial.h>
#include "FFTConfiguration.h"

/*
#include "BoardDefinition.h"
#ifdef DRAGONFLY_V03
    #include "InstancesDragonfly.h"
#else
    #include "InstancesButterfly.h"
#endif
*/

/*
 * Configuratble Diagnostic Finger print (Diagnostic Engine) for IDE+ 
 * 
 * class DiagnosticFingerPrint 
 * 
 * Description : Configure the Diagnostic feature in the Runtime remotely on Velociy (m/s) along with Acceleration data (m/s2)
 * 
 * computes the required feature values like speed x constant , band and apply on the FFT spectrum. 
 * 
 * Helps to send the parameters dynamically over MQTT broker.
 * 
 * 
 */

 class DiagnosticEngine //public IUESP8285
 {

  public:

    // void setDiagnosticFingerPrint( char* conditionCode,uint8_t parameterId, float speed, uint8_t band, size_t sampleCount,q15_t *amplitudes );
    // void setDiagnosticFingerPrint( char* conditionCode,uint8_t parameterId, float speed, uint8_t band  );
    //void setDiagnosticFingerPrint( char* conditionCode,uint8_t parameterId, float speed, uint8_t band  );
    
    //char* getDiagnosticResult(char* conditionCode);

    // float* readCurrentDiagnosticFingerPrintData(bool m_mode,float idx, float idy, float idz);
    // char* getDiagnosticFingerPrint();           // return the configured diagnostic parameters


    // bool isDiagnosticConfigAvailable();         // true /false
    // char*  readDiagnosticConfiguration();      // string
    // void configureAllfingerprints(JsonVariant &config);       // apply values on real time data.
    
    
    String sendDiagnosticFingerprint(); // send the json payload to broker
    JsonObject& configureFingerPrintsFromFlash(String filename,bool isSet);
    
    
    q15_t m_specializedCompute(int m_parameterId, float m_speedMultiplier,uint8_t m_bandValue,q15_t *m_amplitudes);        // compute the diagnostic fingerprints
    char* m_specializedCompute(int m_direction, const q15_t *m_amplitudes,int amplitudeCount, float m_resolution, float scaling,float speed);
    
    float lowerCutoff(float m_speedMultiplier,int m_bandValue,float m_freqResolution );//{ return  m_speedMultiplier - m_speedMultiplier*band/100);  // get lower cutoff in frequency domain
    float higherCutoff(float  m_speedMultiplier,int m_bandValue, float m_freqResolution );//{ return m_speedMultiplier + m_speedMultiplier*band/100}; // get higher cutoff in frequency domain

    const char* getFingerprintKeys(JsonObject &config);
    // void publishFingerprintComputation(float* X_fingerprintResult,float* Y_fingerprintResult,float* Z_fingerprintResult);
    const char* generateJSONPayload(JsonObject &config, float* values);

    const char* mergeJOSN(JsonObject& dest, JsonObject& src);
    float getRMS( const q15_t *amplitudes, uint16_t lower_index,uint16_t upper_index);
    
    static int m_SampleingFrequency;
    static int m_smapleSize;
    static int m_fftLength; 
    
  protected:
    //float m_resolution  =  4.0 * 9.80665; // 0.244/32768.0f;
    float m_resolution;
    float m_scalingFactor = 1.414;
    
    int m_direction;
    
    bool m_configAvailable = false;
    char* conditionCode;
    bool m_mode;                  // 0 - Velocity (m/s) & 1 - Acceleration (m/s2)
    uint32_t amplitudeCount = 12; // sampleCount / 2 + 1;
    q15_t m_amplitudes;
    int m_parameterId = 0;   
    float m_speedMultiplier=0;;
    int m_bandValue = 0;;
    
    float m_freqResolution = (float)m_SampleingFrequency/m_smapleSize;
    
    // update fingerprint
    bool isSet = false;
                
 };


























#endif // DIAGNOSTICFINGERPRINT_H
