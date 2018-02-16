#ifndef IUICS43432_H
#define IUICS43432_H

#include <Arduino.h>
#include <IUButterflyI2S.h>  /* I2S digital audio */

#include "Sensor.h"


/**
 * Class to handle both the I2S and the related ICS43432 microphone
 *
 * Also implement a callback mechanism (with callback rate vs sampling rate)
 * to drive the sampling rate of other driven sensors.
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
 *      - audio: a Q15Feature
 */
class IUICS43432 : public HighFreqSensor
{
    public:
        /***** Preset values and default settings *****/
        // Clock rate of audio device in Hz
        static const uint16_t clockRate = 48000;
        // sampling rate in Hz (can be downclocked from clockRate)
        static const uint16_t defaultSamplingRate = 8000;
        static const uint8_t bitsPerAudioSample = 32;
        static const uint16_t audioSampleSize = (I2S_BUFFER_SIZE * 8) \
            / (bitsPerAudioSample); // stereo recording

        /***** Constructors and destructors *****/
        IUICS43432(I2SClass *I2SInstance, const char* name, Feature *audio);
        virtual ~IUICS43432() {}
        /***** Hardware and power management *****/

        /***** Configuration and calibration *****/
        virtual void setCallbackRate() {}  // Non settable
        uint16_t getCallbackRate() { return clockRate / (audioSampleSize / 2); }
        /***** Data acquisition *****/
        void computeDownclockingRate();
        bool triggerDataAcquisition(void (*callback)());
        bool endDataAcquisition();
        void processAudioData(q31_t *data);
        virtual void readData();
        virtual void acquireData(bool inCallback=false,
                                 bool force=false);
        /***** Communication *****/
        void sendData(HardwareSerial *port);
        /***** Debugging *****/
        virtual void expose();

    protected:
        /***** Core *****/
        I2SClass *m_I2S;
        /***** Configuration and calibration *****/
        bool m_firstI2STrigger;
        /***** Data acquisition *****/
        uint8_t m_rawAudioData[I2S_BUFFER_SIZE];
        // 32bit samples, we use mono data so array half the size
        q15_t m_audioData[audioSampleSize / 2];
        uint16_t m_targetSample;
};

#endif // IUICS43432_H
