/*
  Infinite Uptime Firmware

  Update:
    03/03/2017

  Component:
    Name:

    Description:


   Comments:
     A specific Arduino core have been develop for the Butterfly. For more info on the I2S library currently
     in use, see Thomas Roell (aka GrumpyOldPizza) github: https://github.com/GrumpyOldPizza/arduino-STM32L4
     I2S use a buffer to collect data and when it's full it calls an onReceive callback function.

*/

#ifndef IUI2S_H
#define IUI2S_H

#include <Arduino.h>
#include <IUButterflyI2S.h>  /* I2S digital audio */
#include <arm_math.h> /* CMSIS-DSP library for RFFT */

#include "Sensor.h"
#include "IUI2C.h"


namespace IUComponent
{
    /**
     * Class to handle both the I2S and the related ICS43432 microphone
     *
     * Also implement a callback mechanism (with callback rate vs sampling rate)
     * to drive the sampling rate of other asynchronous sensors.
     *
     * Component:
     *  Name:
     *    ICS43432 (Audio) + I2S bus
     *  Description:
     *    Audio sensor (ICS43432) + I2S bus for Butterfly
     * Comments:
     *    A specific Arduino core have been develop for the Butterfly. For more
     *    info on the I2S library currently in use, see Thomas Roell (aka
     *    GrumpyOldPizza) github:
     *    https://github.com/GrumpyOldPizza/arduino-STM32L4
     *    I2S use a buffer to collect data and when it's full it calls an
     *    onReceive callback function.
     * Destinations:
     *      - audio: a Q15Feature with section size = 2048
     */
    class IUI2S : public Sensor
    {
        public:
            /***** Preset values and default settings *****/
            static const sensorTypeOptions sensorType = IUABCSensor::SOUND;
            static const uint8_t availableClockRateCount = 9;
            static uint16_t availableClockRate[availableClockRateCount];
            // Clock rate of audio device in Hz
            static const uint16_t defaultClockRate = 48000;
            // sampling rate in Hz (can be downclocked from clockRate)
            static const uint16_t defaultSamplingRate = 8000;
            static const uint8_t bitsPerAudioSample = 32;
            static const uint16_t audioSampleSize = (I2S_BUFFER_SIZE * 8) \
                / (bitsPerAudioSample); // stereo recording

            /***** Constructors and destructors *****/
            IUI2S(IUI2C *iuI2C, uint8_t id=100, FeatureBuffer *audio=NULL);
            virtual ~IUI2S() {}
            /***** Hardware and power management *****/

            /***** Configuration and calibration *****/
            bool setClockRate(uint16_t clockRate);
            uint16_t getClockRate() { return m_clockRate; }
            virtual void setCallbackRate() {}  // Non settable
            uint16_t getCallbackRate()
                { return m_clockRate / (audioSampleSize / 2); }
            void setSamplingRate(uint16_t samplingRate);
            bool updateSamplingRateFromI2C();
            /***** Data acquisition *****/
            void prepareDataAcquisition();
            bool triggerDataAcquisition(void (*callback)());
            bool endDataAcquisition();
            void processAudioData(q31_t *data);
            virtual void readData();
            virtual bool acquireData();
            /***** Communication *****/
            void dumpDataThroughI2C();
            void dumpDataForDebugging();

        protected:
            /***** Core *****/
            IUI2C *m_iuI2C;
            /***** Configuration and calibration *****/
            uint16_t m_clockRate;
            bool m_firstI2STrigger;
            /***** Data acquisition *****/
            uint8_t m_rawAudioData[I2S_BUFFER_SIZE];
            // 32bit samples, we use mono data so array half the size
            q31_t m_audioData[audioSampleSize / 2];
            uint16_t m_targetSample;
    };

#endif // IUI2S_H
