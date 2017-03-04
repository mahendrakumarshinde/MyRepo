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
*/

#ifndef IUI2S_H
#define IUI2S_H

#include <Arduino.h>
#include <i2s.h>  /* I2S digital audio */
#include <arm_math.h> /* CMSIS-DSP library for RFFT */

#include "IUUtilities.h"
#include "IUI2C.h"
#include "IUBLE.h"


#define CLOCK_TYPE  (I2S_CLOCK_48K_INTERNAL)     // I2S clock

class IUI2S
{
  public:
  
    // Frequency and sampling setting
    static const uint16_t FREQ = 8000; // ex AUDIO_FREQ // Audio frequency set to 8000 Hz
    static const uint16_t MAX_INTERVAL = 4096; // ex MAX_INTERVAL_AUDIO //Number of Audio readings per cycle
    static const uint16_t NFFT = 2048; // ex AUDIO_NFFT // Audio FFT Index
    static const uint8_t RESCALE = 10; //8; // ex AUDIO_RESCALE // Scaling factor in Audio fft function
    // Getters, Setters, constructor and Destructor
    IUI2S(IUI2C iuI2C, IUBLE iuBLE);
    void setTargetSample(uint16_t target) { m_targetSample = target; }
    void resetTargetSample() { m_targetSample = FREQ; }
    uint16_t getTargetSample() { return m_targetSample; }

    //Methods:
    uint32_t getDownsampleMax() { return 48000 / m_targetSample; } // ex DOWNSAMPLE_MAX
    void wakeUp();
    void begin(void (*callback) (int32_t*));
    bool updateSamplingRateFromI2C();
    void extractDataInPlace(int32_t  *pBuf);
    void pushDataToBatch(uint8_t buffer_compute_index, uint32_t index, int32_t  *pBuf);
    void dumpDataThroughI2C(int32_t  *pBuf);
    float getAudioDB(uint8_t buffer_compute_index);
    void computeAudioRFFT(uint8_t buffer_compute_index, q15_t *hamming_window, int magSize, float HammingK, float inverseWLen);

  private:
    IUI2C m_iuI2C;
    IUBLE m_iuBLE;
    uint16_t m_targetSample; // ex TARGET_AUDIO_SAMPLE // Target Accel frequency may change in data collection mode

    // Acceleration Buffers
    q15_t m_batch[2][MAX_INTERVAL]; // ex audio_batch // Audio buffer - 2 buffers namely for recording and computation simultaneously
    q15_t m_buffer[MAX_INTERVAL];
    q15_t m_rfftBuffer[NFFT * 2]; // ex rfft_audio_buffer // RFFT Output and Magnitude Buffer
    //TODO: May be able to use just one instance. Check later.
    arm_rfft_instance_q15 m_rfftInstance; // ex audio_rfft_instance
    arm_cfft_radix4_instance_q15 m_cfftInstance; //ex audio_cfft_instance
};

#endif // IUI2S_H
