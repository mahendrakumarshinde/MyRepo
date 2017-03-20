/*
  Infinite Uptime Firmware

  Update:
    03/03/2017

  Component:
    Name:
      ICS43432 (Audio) + I2S bus
    Description:
      Audio sensor (ICS43432) + I2S bus for Butterfly

   Comments:
     A specific Arduino core have been develop for the Butterfly. For more info on the I2S library currently
     in use, see Thomas Roell (aka GumpyOldPizza) github: https://github.com/GrumpyOldPizza/arduino-STM32L4
     I2S use a buffer to collect data and when it's full it calls an onReceive callback function.

*/

#ifndef IUI2S_H
#define IUI2S_H

#include <Arduino.h>
#include <I2S.h>  /* I2S digital audio */
#include <arm_math.h> /* CMSIS-DSP library for RFFT */

#include "IUUtilities.h"
#include "IUABCSensor.h"
#include "IUI2C.h"

/**
 *
 * IUI2S is sensibility more complicated than other sensor classes because it handles the
 */
class IUI2S : public IUABCSensor
{
  public:
    static const uint8_t sensorTypeCount = 2;
    static char sensorTypes[sensorTypeCount];
    // Frequency and sampling setting
    static const uint32_t defaultClockRate = 8000; // Hz clock rate of audio device
    static const uint32_t defaultSamplingRate = 8000; // Hz sampling rate (can be downclocked from clockRate)
    enum dataSendOption : uint8_t {sound = 0,
                                   optionCount = 1};
    static const uint8_t bitsPerSample = 32;
    static const uint16_t audioSampleSize = (I2S_BUFFER_SIZE * 8) / bitsPerSample;
    // Getters, Setters, constructor and Destructor
    IUI2S(IUI2C *iuI2C);
    virtual ~IUI2S() {}
    void setClockRate(uint32_t clockRate);
    uint32_t getClockRate() { return m_clockRate; }
    virtual void setCallbackRate() {}  // Non settable
    uint32_t getCallbackRate() { return m_clockRate / (audioSampleSize / 2); }
    void setSamplingRate(uint32_t samplingRate);
    void prepareDataAcquisition();

    //Methods:
    virtual void wakeUp();
    void processAudioData(q31_t *data);
    virtual void readData();
    virtual bool acquireData();
    virtual void sendToReceivers();
    void dumpDataThroughI2C();
    bool updateSamplingRateFromI2C();

  protected:
    IUI2C *m_iuI2C;
    uint32_t m_clockRate;
    bool m_newData;
    uint8_t m_rawAudioData[I2S_BUFFER_SIZE];
    q15_t m_audioData[audioSampleSize];
    uint16_t m_targetSample;  // ex TARGET_AUDIO_SAMPLE // Target Accel frequency may change in data collection mode
};

#endif // IUI2S_H
