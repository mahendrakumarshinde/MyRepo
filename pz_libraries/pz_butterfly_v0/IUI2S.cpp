#include "IUI2S.h"

IUI2S::IUI2S(IUI2C iuI2C, IUBLE iuBLE) : m_iuI2C(iuI2C), m_iuBLE(iuBLE)
{
    //ctor
  resetTargetSample();
}

/**
 * Configure and Initialize the device
 */
void IUI2S::wakeUp()
{
  if (!m_iuI2C.isSilent())
  {
    m_iuI2C.port->print("I2S Pin configuration setting: ");
    m_iuI2C.port->println( I2S_PIN_PATTERN , HEX );
  }
  if (!m_iuI2C.isSilent()) 
  {
    m_iuI2C.port->println( "Initialized I2C Codec" );
  }
}

void IUI2S::begin(void (*callback) (int32_t*))
{
  if (m_iuI2C.getErrorMessage().equals("ALL_OK"))
  {
    I2SRx0.begin( CLOCK_TYPE, callback );  // prepare I2S RX with interrupts
    if (!m_iuI2C.isSilent())
    {
      m_iuBLE.port->println( "Initialized I2S RX without DMA" );
    }
    delay(50);
    I2SRx0.start(); // start the I2S RX
    if (!m_iuI2C.isSilent())
    {
      m_iuI2C.port->println( "Started I2S RX" );
    }
  }
  else {
    m_iuI2C.port->println(m_iuI2C.getErrorMessage());
  }
}

/* ==================== Update and Control Functions =============================== */
/*
 * The following functions check and read the I2C wire buffer to see if configuration updates
 * have been received. If so, the updates are done.
 * Functions return true if an update happened.
 */

/**
 * Check the I2C wire buffer for update of sampling rate
 */
bool IUI2S::updateSamplingRateFromI2C()
{
  String wireBuffer = m_iuI2C.getBuffer();
  if (wireBuffer.indexOf("acosr") > -1)
  {
    int acosrLocation = wireBuffer.indexOf("acosr");
    int target_sample_A = wireBuffer.charAt(acosrLocation + 6) - 48;
    int target_sample_B = wireBuffer.charAt(acosrLocation + 7) - 48;
    m_targetSample = (target_sample_A * 10 + target_sample_B) * 1000;
    return true;
  }
  return false;
}

/* ==================== Data Collection and Feature Calculation functions ======================== */

/**
 * Extract the 24bit INMP441 audio data from 32bit sample
 */
void IUI2S::extractDataInPlace(int32_t  *pBuf)
{
  if (pBuf[0] < 0)  // Check if the data is negative
  {
    pBuf[0] = pBuf[0] >> 8;
    pBuf[0] |= 0xff000000;
  }
  else  // Data is positive
  {
    pBuf[0] = pBuf[0] >> 8;
  }
}

/**
 * Push acceleration in the batch array to later compute the FFT
 * NB: We want to do this in *RUN* mode
 */
void IUI2S::pushDataToBatch(uint8_t buffer_record_index, uint32_t index, int32_t  *pBuf)
{
  byte* ptr = (byte*)&m_batch[buffer_record_index][index];
  ptr[0] = (pBuf[0] >> 8) & 0xFF;
  ptr[1] = (pBuf[0] >> 16) & 0xFF;
}


/**
 * Dump audio data to serial via I2C 
 * NB: We want to do this in *DATA COLLECTION* mode
 */
void IUI2S::dumpDataThroughI2C(int32_t  *pBuf)
{
  m_iuI2C.port->write((pBuf[0] >> 16) & 0xFF);
  m_iuI2C.port->write((pBuf[0] >> 8) & 0xFF);
  m_iuI2C.port->write(pBuf[0] & 0xFF);
}

/** 
 * Calculate and return Audio data in DB
 */
float IUI2S::getAudioDB(uint8_t buffer_compute_index)
{
  float audioDB_val = 0.0;
  float data = 0.0;
  for (int i = 0; i < MAX_INTERVAL; i++)
  {
    data = (float) abs(m_batch[buffer_compute_index][i]);// / 16777215.0;    //16777215 = 0xffffff - 24 bit max value
    if (data > 0)
    {
      audioDB_val += (20 * log10(data));
    }
  }
  audioDB_val /= MAX_INTERVAL;
  return audioDB_val;
}
/**
 * Single instance RFFT calculation on audio data
 * 
 */
void IUI2S::computeAudioRFFT(uint8_t buffer_compute_index, q15_t *hamming_window, int magSize, float HammingK, float inverseWLen)
{
  // Apply Hamming window in-place
  arm_mult_q15(m_batch[buffer_compute_index], hamming_window,
               m_batch[buffer_compute_index], NFFT);

  arm_mult_q15(&m_batch[buffer_compute_index][NFFT], hamming_window,
               &m_batch[buffer_compute_index][NFFT], NFFT);

  // TODO: Computation time verification
  // Since input vector is 2048, perform FFT twice.
  /*====== 2 FFT, from 0 to 2047 and from 2048 to 4095======*/
  for (int i = 0; i < 2; i++)
  {
    arm_rfft_init_q15(&m_rfftInstance, &m_cfftInstance, NFFT, 0, 1);
    if (i == 0)
    {
      arm_rfft_q15(&m_rfftInstance, m_batch[buffer_compute_index], m_rfftBuffer);
    }
    if (i == 1)
    {
      arm_rfft_q15(&m_rfftInstance, &m_batch[buffer_compute_index][NFFT], m_rfftBuffer);
    }
    arm_shift_q15(m_rfftBuffer, RESCALE, m_rfftBuffer, NFFT * 2);
  
    // NOTE: OUTPUT FORMAT IS IN 2.14
    arm_cmplx_mag_q15(m_rfftBuffer, m_batch[buffer_compute_index], NFFT);
    arm_q15_to_float(m_batch[buffer_compute_index], (float*)m_rfftBuffer, magSize);
  
    // TODO: REDUCE VECTOR CALCULATION BY COMING UP WITH SINGLE FACTOR
    // Div by K
    arm_scale_f32((float*)m_rfftBuffer, HammingK, (float*)m_rfftBuffer, magSize);
    // Div by wlen
    arm_scale_f32((float*)m_rfftBuffer, inverseWLen, (float*)m_rfftBuffer, magSize);
    // Fix 2.14 format from cmplx_mag
    arm_scale_f32((float*)m_rfftBuffer, 2.0, (float*)m_rfftBuffer, magSize);
    // Correction of the DC & Nyquist component, including Nyquist point.
    arm_scale_f32(&((float*)m_rfftBuffer)[1], 2.0, &((float*)m_rfftBuffer)[1], magSize - 2);
  }
}

