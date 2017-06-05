#include "IUFeature.h"


/*====================== Scalar Feature Computation functions ============================ */

/**
 * Compute and return single axis velocity using acceleration RMS and pitch
 * Since a single index is returned, only work for a single source (sourceCount = 1)
 * Check out this example: https://www.keil.com/pack/doc/CMSIS/DSP/html/arm_fft_bin_example_f32_8c-example.html
 * What we try to do is basically pitch detection: https://en.wikipedia.org/wiki/Pitch_detection_algorithm
 * However, have a look here: http://dsp.stackexchange.com/questions/11312/why-should-one-use-windowing-functions-for-fft
 * @param sourceCount the number of sources - should be 2
 * @param sourceSize  pointer to an array of size sourceCount - eg: {512, 1}
 * @param source      should be {acceleration values,
 *                               Acceleration energy operationState}
 */
float computeVelocity(q15_t *accelFFT, float accelRMS, uint16_t sampleCount, uint16_t samplingRate, uint16_t FreqLowerBound, uint16_t FreqHigherBound)
{
  float df = (float) samplingRate / (float) sampleCount;
  uint16_t minIdx = (uint16_t) ((float) FreqLowerBound / df);
  uint16_t maxIdx = (uint16_t) min((float) FreqHigherBound / df, sampleCount);
  /* Pitch detection */
  //TODO: Check what windowing function work best + maybe test some other pitch detection method
  float flatness = 0;
  q15_t norm = 0;
  uint16_t maxIndex = 0;
  for (uint16_t i = minIdx; i < maxIdx; i++)
  {
    norm = sqrt(sq(accelFFT[2 * i]) + sq(accelFFT[2 * i + 1]));
    if (norm > flatness)
    {
      flatness = norm;
      maxIndex = i;
    }
  }
  return accelRMS / (2 * PI * df * (float) maxIndex);
}

/**
 * Compute velocity RMS from an accel FFT
 * Note that the input accelFFT will be modified in the process
 * @param accelFFT          the FFT (as an array of size 2 * sampleCount as outputed by arm_rfft)
 * @param sampleCount       the sample count
 * @param samplingRate      the sampling rate / frequency
 * @param FreqLowerBound    freq lower bound for bandpass filtering
 * @param FreqHigherBound   freq higher bound for bandpass filtering
 */
float computeFullVelocity(q15_t *accelFFT, uint16_t sampleCount, uint16_t samplingRate, uint16_t FreqLowerBound, uint16_t FreqHigherBound, float scalingFactor)
{
  // Get the max internalRescale we can use (loss of precision has a big impact in velocity computation, so we need to rescale it internally)
  float val, maxVal = 0;
  for (uint16_t i = 2; i < sampleCount; ++i) // skip 0 and 1 idx, as DC component will be later removed
  {
    val = (float) abs(accelFFT[i]) / floor(0.5 * i);
    if (maxVal < val)
    {
      maxVal = val;
    }
  }
  uint8_t rescaleBit = 15 - ceil(log(maxVal) / log(2));
  uint16_t internalRescale = (uint16_t) pow(2, rescaleBit);
  // Filter and integrate in frequency domain
  filterAndIntegrateFFT(accelFFT, sampleCount, samplingRate, FreqLowerBound, FreqHigherBound, internalRescale, false);
  //Serial.print(" max: "); Serial.println(getMax(accelFFT, 512));
  // Inverse FFT
  q15_t velocities[sampleCount];
  computeRFFT(accelFFT, velocities, sampleCount, true); 
  // Velocities RMS
  //Need to multiply by 512 as arm_math FFT downscale by 9 bits
  float velRMS = computeSignalRMS(velocities, sampleCount, samplingRate, scalingFactor * 512. / (float) internalRescale, false);
  return velRMS;
}

float computeFullVelocity(q15_t *accelFFT, uint16_t sampleCount, uint16_t samplingRate, uint16_t FreqLowerBound, uint16_t FreqHigherBound, float scalingFactor, q15_t *window)
{
  // Get the max internalRescale we can use (loss of precision has a big impact in velocity computation, so we need to rescale it internally)
  float val, maxVal = 0;
  for (uint16_t i = 2; i < sampleCount; ++i) // skip 0 and 1 idx, as DC component will be later removed
  {
    val = (float) abs(accelFFT[i]) / floor(0.5 * i);
    if (maxVal < val)
    {
      maxVal = val;
    }
  }
  uint8_t rescaleBit = 15 - ceil(log(maxVal) / log(2));
  uint16_t internalRescale = pow(2, rescaleBit);
  // Filter and integrate in frequency domain
  filterAndIntegrateFFT(accelFFT, sampleCount, samplingRate, FreqLowerBound, FreqHigherBound, internalRescale, false);
  //Serial.print(" max: "); Serial.println(getMax(accelFFT, 512));
  // Inverse FFT
  q15_t velocities[sampleCount];
  computeRFFT(accelFFT, velocities, sampleCount, true, window); // FFT / iFFT dowscale values by 9bit
  // Velocities RMS
  float velRMS = computeSignalRMS(velocities, sampleCount, samplingRate, scalingFactor / (float) internalRescale, false);
  return velRMS;
}

/**
 * Return the average sound level in DB over an array of sound pressure data
 * Acoustic decibel formula: 20 * log10(p / p_ref) where
 * p is the sound pressure
 * p_ref is the reference pressure for sound in air = 20 mPa (or 1 mPa in water)
 * @param sourceCount the number of sources
 * @param sourceSize  pointer to an array of size sourceCount
 * @param source      pointer to an array q15_t[sourceCount][sourceSize[i for i in sourceCount]]
 */
float computeAcousticDB(uint8_t sourceCount, const uint16_t* sourceSize, q15_t **source)
{
  // Strangely, log10 is very slow. It's better to use it the least possible, so we
  // multiply data points as much as possible and then log10 the results when we have to.
  // We can multiply data points until we reach uint64_t limit (so 2^64)
  // First, compute the limit for our accumulation
  q15_t maxVal = 0;
  uint32_t maxIdx = 0;
  uint64_t limit = 0;
  int data = 0;
  uint64_t accu = 1;
  float audioDB = 0.;
  float meanAudioDB = 0.;
  for (uint8_t i = 0; i < sourceCount; i++)
  {
    arm_max_q15(source[i], sourceSize[i], &maxVal, &maxIdx); // Get max data value
    limit = pow(2, 63 - round(log(maxVal) / log(2)));
    // Then, use it to compute our data
    for (uint16_t j = 0; j < sourceSize[i]; j++)
    {
      data = abs(source[i][j]);
      if (data > 0)
      {
        accu *= data;
      }
      if (accu >= limit)
      {
        audioDB += log10(accu);
        accu = 1;
      }
    }
    audioDB += log10(accu);
    accu = 1;
    meanAudioDB += 20.0 * audioDB / (float) sourceSize[i];
    audioDB = 0;
  }
  meanAudioDB /= (float) sourceCount;
  return meanAudioDB;
}


/*====================== Array Feature Computation functions ============================ */


/**
 * Single instance RFFT calculation on audio data - expects only 1 source
 * NB - this function is destructive - To save space, it will modify the source itself, so the source won't be usable afterward.
 * The function use 2 successive RFFT calculation of half the size of the source
 * @param sourceSize       must be {1024} or {4096} in current implementation - RFFT will then be of size 512 or 2048
 * @param source           pointer to an array q15_t[sourceSize]
 * @param destinationSize  the size of the destination array
 * @param destination      pointer to an array to collect the results
 * @return                 true if the computation succeeded, else false.
 */
bool computeAudioRFFT(const uint16_t sourceSize, q15_t *source, const uint16_t destinationSize, q15_t *destination)
{
  //TODO: rebuild this function, it is not working
  if (debugMode) { debugPrint(F("Need to refactor the function computeAudioRFFT")); }
  return false;
  if ((2 * sourceSize) != destinationSize)
  {
    if (loopDebugMode) { debugPrint(F("RFFT output size must be twice the input size")); }
    return false;
  }
  //TODO allow to pass this as argument (maybe in the source)
  uint8_t AUDIO_RESCALE = 10;
  
  const uint16_t rfftSize = destinationSize / 2;
  float magSize(0), hammingK(0), inverseWLen(0);
  switch(rfftSize)
  {
    case 512:
      arm_mult_q15(source, hamming_window_512, source, rfftSize);
      arm_mult_q15(&source[rfftSize], hamming_window_512, &source[rfftSize], rfftSize);
      magSize = magsize_512;
      hammingK = hamming_K_512;
      inverseWLen = inverse_wlen_512;
      break;
    case 2048:
      arm_mult_q15(source, hamming_window_2048, source, rfftSize);
      arm_mult_q15(&source[rfftSize], hamming_window_2048, &source[rfftSize], rfftSize);
      magSize = magsize_2048;
      hammingK = hamming_K_2048;
      inverseWLen = inverse_wlen_2048;
      break;
    default:
      return false; //Fail if sourceSize is not supported
  }
  // RFFT
  arm_rfft_instance_q15 rfftInstance;
  for (int i = 0; i < 2; i++)
  {
    arm_status armStatus = arm_rfft_init_q15(&rfftInstance,   // RFFT instance
                                             sourceSize,      // FFT length
                                             0,               // 0: forward FFT, 1: inverse FFT
                                             1);              // 0: disable bit reversal output, 1: enable it
    if (!armStatus)
    {
      if (loopDebugMode) { debugPrint("RFFT computation failed"); }
      return false;
    }
    arm_rfft_q15(&rfftInstance, &source[i * rfftSize], destination);
    arm_shift_q15(&destination[i * rfftSize], AUDIO_RESCALE, &destination[i * rfftSize], sourceSize);
  }

  // NOTE: OUTPUT FORMAT IS IN 2.14
  arm_cmplx_mag_q15(destination, source, sourceSize);
  arm_q15_to_float(source, (float*)destination, magSize);

  // TODO: REDUCE VECTOR CALCULATION BY COMING UP WITH SINGLE FACTOR
  // Div by K
  arm_scale_f32((float*)destination, hammingK, (float*)destination, magSize);
  // Div by wlen
  arm_scale_f32((float*)destination, inverseWLen, (float*)destination, magSize);
  // Fix 2.14 format from cmplx_mag
  arm_scale_f32((float*)destination, 2.0, (float*)destination, magSize);
  // Correction of the DC & Nyquist component, including Nyquist point.
  arm_scale_f32(&((float*)destination)[1], 2.0, &((float*)destination)[1], magSize - 2);
  return true;
}


/* ======================== IUFeatureProducer Definitions ========================== */

IUFeatureProducer::IUFeatureProducer() :
  IUABCProducer(),
  m_latestValue(0),
  m_rms(0),
  m_mainFreq(0),
  m_samplingRate(0),
  m_sampleCount(0),
  m_resolution(0),
  m_state(operationState::idle),
  m_highestDangerLevel(operationState::idle)
{
  m_destination = NULL;
}

bool IUFeatureProducer::prepareDestination()
{
  uint16_t ds = getDestinationSize();
  m_destination = new q15_t[ds];
  if (!m_destination)
  {
    if (setupDebugMode) { debugPrint("Failed to prepare producer destination array"); }
    return false;  
  }
  for (uint16_t i = 0; i < ds; i++)
  {
    m_destination[i] = 0;
  }
  return true;
}

void IUFeatureProducer::sendToReceivers()
{
  for (uint8_t i = 0; i < m_receiverCount; i++)
  {
    if (m_receivers[i])
    {
      if (loopDebugMode && highVerbosity)
      {
        debugPrint("send to ", false);
        debugPrint(m_receivers[i]->getName(), false);
        debugPrint(", opt ", false);
        debugPrint((int) m_toSend[i], false);
      }
      switch(m_toSend[i])
      {
        case (uint8_t) dataSendOption::value:
          if (loopDebugMode && highVerbosity)
          {
            debugPrint("=> value ", false);
            debugPrint((float) m_latestValue);
          }
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_latestValue);
          break;
        case (uint8_t) dataSendOption::state:
          if (loopDebugMode && highVerbosity)
          {
            debugPrint("=> state ", false);
            debugPrint((int) m_state);
          }
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], (q15_t) m_state);
          break;
        case (uint8_t) dataSendOption::samplingRate:
          if (loopDebugMode && highVerbosity)
          {
            debugPrint("=> samplingRate ", false);
            debugPrint(m_samplingRate);
          }
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], (q15_t) m_samplingRate);
          break;
        case (uint8_t) dataSendOption::sampleCount:
          if (loopDebugMode && highVerbosity)
          {
            debugPrint("=> sampleCount ", false);
            debugPrint(m_sampleCount);
          }
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], (q15_t) m_sampleCount);
          break;
        case (uint8_t) dataSendOption::RMS:
          if (loopDebugMode && highVerbosity)
          {
            debugPrint("=> RMS ", false);
            debugPrint(m_rms);
          }
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_rms);
          break;
        case (uint8_t) dataSendOption::mainFreq:
          if (loopDebugMode && highVerbosity)
          {
            debugPrint("=> Main Freq ", false);
            debugPrint(m_mainFreq);
          }
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_mainFreq);
          break;
        case (uint8_t) dataSendOption::resolution:
          if (loopDebugMode && highVerbosity)
          {
            debugPrint("=> Resolution ", false);
            debugPrint(m_resolution);
          }
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_resolution);
          break;
        case (uint8_t) dataSendOption::valueArray:
          if (loopDebugMode && highVerbosity) { debugPrint("=> array"); }
          m_receivers[i]->receiveArray(m_receiverSourceIndex[i]);
          break;
        default:
          if (loopDebugMode)
          {
            debugPrint(F("option "), false);
            debugPrint((uint8_t) m_toSend[i], false);
            debugPrint(F(" for receiver "), false);
            debugPrint(m_receivers[i]->getName(), false);
            debugPrint(F(" is unknown"));
          }
          break;
          
      }
    }
  }
}


bool IUFeatureProducer::addArrayReceiver(uint8_t sendOption, uint8_t receiverSourceIndex, IUABCFeature *receiver)
{
  if (m_receiverCount >= maxReceiverCount)
  {
    if(setupDebugMode) { debugPrint("Producer's receiver list is full"); }
    return false;
  }
  m_receivers[m_receiverCount] = receiver;
  if (!m_receivers[m_receiverCount])
  {
    if(setupDebugMode) { debugPrint("Pointer assignment for array source failed"); }
    m_receivers[m_receiverCount] = NULL;
    return false;
  }
  m_receiverSourceIndex[m_receiverCount] = receiverSourceIndex;
  m_toSend[m_receiverCount] = sendOption;
  m_receiverCount++;
  bool success = receiver->setSource(receiverSourceIndex, getDestinationSize(), m_destination);
  if (!success)
  {
    if(setupDebugMode) { debugPrint("Array source setting failed"); }
  }
  return success;
}

bool IUFeatureProducer::addReceiver(uint8_t sendOption, uint8_t receiverSourceIndex, IUABCFeature *receiver)
{
  if (sendOption == 99)
  {
    m_toSend[m_receiverCount] = sendOption;
    return addArrayReceiver(sendOption, receiverSourceIndex, receiver);
  }
  else
  {
    return addScalarReceiver(sendOption, receiverSourceIndex, receiver);
  }
}


IUFeatureProducer256::IUFeatureProducer256() :
  IUFeatureProducer()
{
  // ctor
}


IUFeatureProducer1024::IUFeatureProducer1024() :
  IUFeatureProducer()
{
  // ctor
}


/* ================== IUFeature Definitions ==================== */

IUFeature::IUFeature(uint8_t id, char *name) :
  IUABCFeature(id, name)
{
  m_producer = NULL;
}

IUFeature::~IUFeature()
{
  Serial.println("delete - here 20");
  if(m_producer)
  {
    delete m_producer;
  }
  Serial.println("here 21");
  m_producer = NULL;
  Serial.println("here 22");
}

bool IUFeature::prepareProducer()
{
  m_producer = new IUFeatureProducer();
  if (!m_producer || !m_producer->prepareDestination())
  {
    if (setupDebugMode) { debugPrint("Producer preparation failed."); }
  }
  m_producerReady = true;
  return m_producerReady;
}

/* ================== IUQ15Feature Definitions ==================== */

IUQ15Feature::IUQ15Feature(uint8_t id, char *name) :
  IUFeature(id, name)
{
  // ctor
}

/**
 * Reset all source pointers to NULL
 */
void IUQ15Feature::resetSource(bool deletePtr)
{
  uint8_t count = getSourceCount();
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < count; j++)
    {
      if (deletePtr)
      {
        delete m_source[i][j];
      }
      m_source[i][j] = NULL;
    }
  }
  m_sourceReady = false;
}

/**
 * Creates the source pointers
 * @return true if pointer assignments succeeded, else false
 */
bool IUQ15Feature::newSource()
{
  uint8_t count = getSourceCount();
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < count; j++)
    {
      m_source[i][j] = new q15_t[getSourceSize(j)];
      if (m_source[i][j] == NULL) // Pointer assignment failed
      {
        return false;
      }
    }
  }
  return true;
}

/*
 * Set the source at sourceIndex as a pointer to given values
 * NB: prepareSource must have run beforehand or pointer manipulation will cause undefined behavior
 * (this generally result in the board hanging indefinitely).
 */
bool IUQ15Feature::setSource(uint8_t sourceIndex, uint16_t valueCount, q15_t *values)
{
  if(getSourceSize(sourceIndex) != valueCount)
  {
    if(setupDebugMode) { debugPrint("Receiver source size doesn't match producer value count."); }
    return false;
  }
  // TODO delete is causing the firmware to hang, while m_source have been previously assigned... Need to find out why
  /*
  // Delete previous source assignment
  delete m_source[0][sourceIndex]; m_source[0][sourceIndex] = NULL;
  delete m_source[1][sourceIndex]; m_source[1][sourceIndex] = NULL;
  */
  m_source[0][sourceIndex] = values;
  m_source[1][sourceIndex] = values;
  return true;
}

/**
 * Receive a new source value - it will be stored only if the source buffers are ready
 * @param sourceIndex   the index of the source to hydrate
 * @param value         the value to add to the source
 * @return              true if the value was stored in source buffer, else false
 */
bool IUQ15Feature::receiveScalar(uint8_t sourceIndex, q15_t value)
{
  if (!m_recordNow[m_recordIndex][sourceIndex])                         // Recording buffer is not ready
  {
    // Printing during callbacks can cause issues (do it only for debugging)
    if (callbackDebugMode)
    {
      debugPrint(m_name, false);
      debugPrint(F(" can't record at "), false);
      debugPrint(m_recordIndex);
    }
    return false;
  }
  if (m_sourceCounter[sourceIndex] < getSourceSize(sourceIndex))        // Recording buffer is not full
  {
    m_source[m_recordIndex][sourceIndex][m_sourceCounter[sourceIndex]] = value;
    m_sourceCounter[sourceIndex]++;
  }
  if (m_sourceCounter[sourceIndex] >= getSourceSize(sourceIndex))       // Recording buffer is now full
  {
    m_recordNow[m_recordIndex][sourceIndex] = false;         //Do not record anymore in this buffer
    m_computeNow[m_recordIndex][sourceIndex] = true;         //Buffer ready for computation
    m_sourceCounter[sourceIndex] = 0;                        //Reset counter
    // Printing during callbacks can cause issues (do it only for debugging)
    if (callbackDebugMode)
    {
      debugPrint(m_name, false);
      debugPrint(" source #", false);
      debugPrint(sourceIndex, false);
      debugPrint(" is full");
    }
  }
  if (isTimeToEndRecord())
  {
    m_recordIndex = m_recordIndex == 0 ? 1 : 0; //switch buffer index
  }
  return true;
}

bool IUQ15Feature::receiveScalar(uint8_t sourceIndex, float value)
{
  q15_t val = (q15_t) value *rescaleFloatScalar;
  return receiveScalar(sourceIndex, val);
}


/* ================== IUFloatFeature Definitions ==================== */

IUFloatFeature::IUFloatFeature(uint8_t id, char *name) :
  IUFeature(id, name)
{
  // ctor
}

/**
 * Reset all source pointers to NULL
 */
void IUFloatFeature::resetSource(bool deletePtr)
{
  uint8_t count = getSourceCount();
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < count; j++)
    {
      if (deletePtr)
      {
        delete m_source[i][j];
      }
      m_source[i][j] = NULL;
    }
  }
  m_sourceReady = false;
}

/**
 * Creates the source pointers
 * @return true is pointer assignments succeeded, else false
 */
bool IUFloatFeature::newSource()
{
  uint8_t count = getSourceCount();
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < count; j++)
    {
      m_source[i][j] = new float[getSourceSize(j)];
      if (m_source[i][j] == NULL) // Pointer assignment failed
      {
        return false;
      }
    }
  }
  return true;
}

/*
 * Set the source at sourceIndex as a pointer to given values
 * NB: prepareSource must have run beforehand or pointer manipulation will cause undefined behavior
 * (this generally result in the board hanging indefinitely).
 */
bool IUFloatFeature::setSource(uint8_t sourceIndex, uint16_t valueCount, float *values)
{
  if(getSourceSize(sourceIndex) != valueCount)
  {
    if(setupDebugMode) { debugPrint("Receiver source size doesn't match producer value count."); }
    return false;
  }
  // TODO delete is causing the firmware to hang, while m_source have been previously assigned... Need to find out why
  /*
  delete m_source[0][sourceIndex]; m_source[0][sourceIndex] = NULL;
  delete m_source[1][sourceIndex]; m_source[1][sourceIndex] = NULL;
  */
  m_source[0][sourceIndex] = values;
  m_source[1][sourceIndex] = values;
  return true;
}

/**
 * Receive a new source value - it will be stored only if the source buffers are ready
 * @param sourceIndex   the index of the source to hydrate
 * @param value         the value to add to the source
 * @return              true if the value was stored in source buffer, else false
 */
bool IUFloatFeature::receiveScalar(uint8_t sourceIndex, float value)
{
  if (!m_recordNow[m_recordIndex][sourceIndex])                         // Recording buffer is not ready
  {
    // Printing during callbacks can cause issues (do it only for debugging)
    if (callbackDebugMode)
    {
      debugPrint(m_name, false);
      debugPrint(F(" can't record at "), false);
      debugPrint(m_recordIndex);
    }
    return false;
  }
  if (m_sourceCounter[sourceIndex] < getSourceSize(sourceIndex))        // Recording buffer is not full
  {
    m_source[m_recordIndex][sourceIndex][m_sourceCounter[sourceIndex]] = value;
    m_sourceCounter[sourceIndex]++;
  }
  if (m_sourceCounter[sourceIndex] >= getSourceSize(sourceIndex))       // Recording buffer is now full
  {
    m_recordNow[m_recordIndex][sourceIndex] = false;         //Do not record anymore in this buffer
    m_computeNow[m_recordIndex][sourceIndex] = true;         //Buffer ready for computation
    m_sourceCounter[sourceIndex] = 0;                        //Reset counter
    // Printing during callbacks can cause issues (do it only for debugging)
    if (callbackDebugMode)
    {
      debugPrint(m_name, false);
      debugPrint(" source #", false);
      debugPrint(sourceIndex, false);
      debugPrint(" is full");
    }
  }
  if (isTimeToEndRecord())
  {
    m_recordIndex = m_recordIndex == 0 ? 1 : 0; //switch buffer index
  }
  return true;
}

bool IUFloatFeature::receiveScalar(uint8_t sourceIndex, q15_t value)
{
  float val = q15ToFloat(value);
  return receiveScalar(sourceIndex, val);
}

/* ========================== Speicalized Feature Classes ============================= */

const uint16_t IUAccelPreComputationFeature128::sourceSize[IUAccelPreComputationFeature128::sourceCount] = {128, 1, 1};

IUAccelPreComputationFeature128::IUAccelPreComputationFeature128(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  //m_producer = NULL;
}

bool IUAccelPreComputationFeature128::prepareProducer()
{
  m_producer = new IUFeatureProducer256();
  if (!m_producer || !m_producer->prepareDestination())
  {
    if (setupDebugMode) { debugPrint("Producer preparation failed."); }
  }
  m_producerReady = true;
  getProducer()->setSampleCount(getSourceSize(0));
  return m_producerReady;
}

void IUAccelPreComputationFeature128::m_computeScalar (uint8_t computeIndex)
{
  float val = computeSignalEnergy(m_source[m_computeIndex][0],                  // samples
                                  getSourceSize(0),                             // sample count
                                  m_source[computeIndex][1][0],                 // sampling frequency
                                  getFactorToMS2(m_source[computeIndex][2][0]), // Scaling factor
                                  false);                                       // center signal around 0 ?
  getProducer()->setLatestValue(val);
  float rms = computeSignalRMS(m_source[m_computeIndex][0],                  // values
                               getSourceSize(0),                             // sample count
                               m_source[computeIndex][1][0],                 // sampling freq
                               getFactorToMS2(m_source[computeIndex][2][0]), // Convert to m.s-2
                               true);                                        // remove mean
  getProducer()->setRMS(1.03 * rms);  // Upscale by 3%
  updateState();
  getProducer()->setSamplingRate(m_source[computeIndex][1][0]);
  getProducer()->setResolution(m_source[computeIndex][2][0]);
  getProducer()->setSampleCount(getSourceSize(0));
}

void IUAccelPreComputationFeature128::m_computeArray (uint8_t computeIndex)
{
  computeRFFT(m_source[m_computeIndex][0],  // pointer to source
              getDestination(),             // pointer to destination
              getSourceSize()[0],           // FFT length
              false);                       // Forward FFT
  if (getProducer()->getRMS() < accelRMSThreshold)
  {
    // When the board is still, frequency computation is unreliable => return 0 instead
    getProducer()->setMainFreq(0);
  }
  else
  {
    float mainFreq = getMainFrequency(getDestination(), getSourceSize(0), m_source[computeIndex][1][0]);
    getProducer()->setMainFreq(mainFreq);
  }
}

/**
 * Stream raw accel data through given port
 * @param port        the port through which data must be streamed
 * This is useful to get a whole batch of data, and to perform computation 
 * outside of the device itself
 */
void IUAccelPreComputationFeature128::streamSourceData(HardwareSerial *port, String macAddr, String keyword)
{
  while (!m_computeNow[m_computeIndex][0])
  {
    delay(1); // Wait for the recording to finish
  }
  port->print("REC,");
  port->print(macAddr);
  port->print(',');
  port->print(keyword);
  for (int i = 0; i < getSourceSize(0); i++)
  {
    port->print(',');
    port->print(toMS2(m_source[m_computeIndex][0][i], m_source[m_computeIndex][2][0]));
    port->flush();
  }
  port->print(';');
  port->flush();
}


const uint16_t IUAccelPreComputationFeature512::sourceSize[IUAccelPreComputationFeature512::sourceCount] = {512, 1, 1};

IUAccelPreComputationFeature512::IUAccelPreComputationFeature512(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  //m_producer = NULL;
}

bool IUAccelPreComputationFeature512::prepareProducer()
{
  m_producer = new IUFeatureProducer1024();
  if (!m_producer || !m_producer->prepareDestination())
  {
    if (setupDebugMode) { debugPrint("Producer preparation failed."); }
  }
  m_producerReady = true;
  m_producer->setSampleCount(getSourceSize(0));
  return m_producerReady;
}

void IUAccelPreComputationFeature512::m_computeScalar (uint8_t computeIndex)
{
  float val = computeSignalEnergy(m_source[m_computeIndex][0],                  // samples
                                  getSourceSize(0),                             // sample count
                                  m_source[computeIndex][1][0],                 // sampling frequency
                                  getFactorToMS2(m_source[computeIndex][2][0]), // Scaling factor
                                  false);                                       // center signal around 0 ?
  getProducer()->setLatestValue(val);
  float rms = computeSignalRMS(m_source[m_computeIndex][0],                  // values
                               getSourceSize(0),                             // sample count
                               m_source[computeIndex][1][0],                 // sampling freq
                               getFactorToMS2(m_source[computeIndex][2][0]), // Convert to m.s-2
                               true);                                        // remove mean
  getProducer()->setRMS(1.03 * rms); // Upscale by 3%
  updateState();
  getProducer()->setSamplingRate(m_source[computeIndex][1][0]);
  getProducer()->setResolution(m_source[computeIndex][2][0]);
  getProducer()->setSampleCount(getSourceSize(0));
}

void IUAccelPreComputationFeature512::m_computeArray (uint8_t computeIndex)
{
  computeRFFT(m_source[m_computeIndex][0],  // pointer to source
              getDestination(),             // pointer to destination
              getSourceSize(0),             // FFT length
              false);                        // Forward FFT
  if (getProducer()->getRMS() < accelRMSThreshold)
  {
    // When the board is still, frequency computation is unreliable => return 0 instead
    getProducer()->setMainFreq(0);
  }
  else
  {
    float mainFreq = getMainFrequency(getDestination(), getSourceSize(0), m_source[computeIndex][1][0]);
    getProducer()->setMainFreq(mainFreq);
  }
}

/**
 * Stream raw accel data through given port
 * @param port        the port through which data must be streamed
 * This is useful to get a whole batch of data, and to perform computation 
 * outside of the device itself
 */
void IUAccelPreComputationFeature512::streamSourceData(HardwareSerial *port, String macAddr, String keyword)
{
  while (m_computeNow[m_computeIndex][0])
  {
    delay(1); // Wait for the recording to finish
  }
  port->print("REC,");
  port->print(macAddr);
  port->print(',');
  port->print(keyword);
  for (int i = 0; i < getSourceSize(0); i++)
  {
    port->print(',');
    port->print(toMS2(m_source[m_computeIndex][0][i], m_source[m_computeIndex][2][0]));
    port->flush();
  }
  port->print(';');
  port->flush();
}


const uint16_t IUSingleAxisEnergyFeature128::sourceSize[IUSingleAxisEnergyFeature128::sourceCount] = {128, 1, 1};

IUSingleAxisEnergyFeature128::IUSingleAxisEnergyFeature128(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  // ctor
}

void IUSingleAxisEnergyFeature128::m_computeScalar (uint8_t computeIndex)
{
  float val = computeSignalEnergy(m_source[m_computeIndex][0],                  // samples
                                  getSourceSize(0),                             // sample count
                                  m_source[computeIndex][1][0],                 // sampling frequency
                                  getFactorToMS2(m_source[computeIndex][2][0]), // Scaling factor
                                  false);                                       // center signal around 0
  getProducer()->setLatestValue(val);
  getProducer()->setSamplingRate(m_source[computeIndex][1][0]);
  getProducer()->setResolution(m_source[computeIndex][2][0]);
}


const uint16_t IUTriAxisEnergyFeature128::sourceSize[IUTriAxisEnergyFeature128::sourceCount] = {128, 128, 128, 1, 1};

IUTriAxisEnergyFeature128::IUTriAxisEnergyFeature128(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  // ctor
}

void IUTriAxisEnergyFeature128::m_computeScalar (uint8_t computeIndex)
{
  // Compute on each axis and then take the average
  float values[3] = {0, 0, 0};
  for (uint8_t i = 0; i < 3; ++i)
  {
    values[i] = computeSignalEnergy(m_source[m_computeIndex][i],                  // samples
                                    getSourceSize(i),                             // sample count
                                    m_source[computeIndex][4][0],                 // sampling frequency
                                    getFactorToMS2(m_source[computeIndex][5][0]), // Scaling factor
                                    false);                                       // center signal around 0
  }
  float val = (values[0] + values[1] + values[2]) / 3.0;
  getProducer()->setLatestValue(val);
  getProducer()->setSamplingRate(m_source[computeIndex][3][0]);
  getProducer()->setResolution(m_source[computeIndex][4][0]);
}


const uint16_t IUSingleAxisEnergyFeature512::sourceSize[IUSingleAxisEnergyFeature512::sourceCount] = {512, 1, 1};

IUSingleAxisEnergyFeature512::IUSingleAxisEnergyFeature512(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  // ctor
}

void IUSingleAxisEnergyFeature512::m_computeScalar (uint8_t computeIndex)
{
  float val = computeSignalEnergy(m_source[m_computeIndex][0],                  // samples
                                  getSourceSize(0),                             // sample count
                                  m_source[computeIndex][1][0],                 // sampling frequency
                                  getFactorToMS2(m_source[computeIndex][2][0]), // Scaling factor
                                  false);                                       // center signal around 0
  getProducer()->setLatestValue(val);
  getProducer()->setSamplingRate(m_source[computeIndex][1][0]);
  getProducer()->setResolution(m_source[computeIndex][2][0]);
}


const uint16_t IUTriAxisEnergyFeature512::sourceSize[IUTriAxisEnergyFeature512::sourceCount] = {512, 512, 512, 1, 1};

IUTriAxisEnergyFeature512::IUTriAxisEnergyFeature512(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  // ctor
}

void IUTriAxisEnergyFeature512::m_computeScalar(uint8_t computeIndex)
{
  // Compute on each axis and then take the average
  float values[3] = {0, 0, 0};
  for (uint8_t i = 0; i < 3; ++i)
  {
    values[i] = computeSignalEnergy(m_source[m_computeIndex][i],                  // samples
                                    getSourceSize(i),                             // sample count
                                    m_source[computeIndex][4][0],                 // sampling frequency
                                    getFactorToMS2(m_source[computeIndex][5][0]), // Scaling factor
                                    false);                                       // center signal around 0
  }
  float val = (values[0] + values[1] + values[2]) / 3.0;
  getProducer()->setLatestValue(val);
  getProducer()->setSamplingRate(m_source[computeIndex][3][0]);
  getProducer()->setResolution(m_source[computeIndex][4][0]);
}


const uint16_t IUTriSourceSummingFeature::sourceSize[IUTriSourceSummingFeature::sourceCount] = {1, 1, 1};

IUTriSourceSummingFeature::IUTriSourceSummingFeature(uint8_t id, char *name) :
  IUFloatFeature(id, name)
{
  // ctor
}

void IUTriSourceSummingFeature::m_computeScalar(uint8_t computeIndex)
{
  float val = multiArrayMean(getSourceCount(), getSourceSize(), m_source[m_computeIndex]);
  getProducer()->setLatestValue(val);
}


const uint16_t IUVelocityFeature512::sourceSize[IUVelocityFeature512::sourceCount] = {1024, 1, 1, 1, 1};

IUVelocityFeature512::IUVelocityFeature512(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  // ctor
}

void IUVelocityFeature512::m_computeScalar (uint8_t computeIndex)
{
  if (m_source[computeIndex][3][0] < accelRMSThreshold)
  {
    // When the board is still, Velocity computation is unreliable => return 0 instead
    getProducer()->setLatestValue(0);
  }
  else
  {
    float val = computeFullVelocity(m_source[computeIndex][0],                            // accelFFT
                                    m_source[computeIndex][2][0],                         // sampleCount
                                    m_source[computeIndex][1][0],                         // samplingRate
                                    0,                                                    // Bandpass filtering lower bound
                                    1000,                                                 // Bandpass filtering higher bound
                                    1000 * getFactorToMS2(m_source[computeIndex][4][0])); // convert to mm/s2
    getProducer()->setLatestValue(val);
  }
}

/**
 * Creates the source pointers
 * @return true if pointer assignments succeeded, else false
 */
bool IUVelocityFeature512::newSource()
{
  uint8_t count = getSourceCount();
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint8_t j = 0; j < count; ++j)
    {
      m_source[i][j] = new q15_t[1];
      if (m_source[i][j] == NULL) // Pointer assignment failed
      {
        return false;
      }
    }
  }
  return true;
}

const uint16_t IUDefaultFloatFeature::sourceSize[IUDefaultFloatFeature::sourceCount] = {1};

IUDefaultFloatFeature::IUDefaultFloatFeature(uint8_t id, char *name) :
  IUFloatFeature(id, name)
{
  // ctor
}

void IUDefaultFloatFeature::m_computeScalar (uint8_t computeIndex)
{
  getProducer()->setLatestValue(m_source[computeIndex][0][0]);
}


/* Audio Data is very bulky, so we put it in SRAM2 */
__attribute__((section(".noinit2"))) q15_t I2SDataSRAM2_0[4096];
__attribute__((section(".noinit2"))) q15_t I2SDataSRAM2_1[4096];



const uint16_t IUAudioDBFeature2048::sourceSize[IUAudioDBFeature2048::sourceCount] = {2048};

IUAudioDBFeature2048::IUAudioDBFeature2048(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  // ctor
}

void IUAudioDBFeature2048::m_computeScalar (uint8_t computeIndex)
{
  float val = computeAcousticDB(getSourceCount(), getSourceSize(), m_source[m_computeIndex]);
  getProducer()->setLatestValue(60. + (val-23.3) * 0.642); // adjust
}

/**
 * Creates the source pointers
 * @return true if pointer assignments succeeded, else false
 */
bool IUAudioDBFeature2048::newSource()
{
  uint8_t count = getSourceCount();
  m_source[0][0] = &I2SDataSRAM2_0[0];
  m_source[1][0] = &I2SDataSRAM2_1[0];
  if (m_source[0][0] == NULL || m_source[1][0] == NULL) // Pointer assignment failed
  {
    return false;
  }
  return true;
}


const uint16_t IUAudioDBFeature4096::sourceSize[IUAudioDBFeature4096::sourceCount] = {4096};

IUAudioDBFeature4096::IUAudioDBFeature4096(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  // ctor
}

void IUAudioDBFeature4096::m_computeScalar (uint8_t computeIndex)
{
  float val = computeAcousticDB(getSourceCount(), getSourceSize(), m_source[m_computeIndex]);
  getProducer()->setLatestValue(60. + (val-23.3) * 0.642);
}

/**
 * Creates the source pointers
 * @return true if pointer assignments succeeded, else false
 */
bool IUAudioDBFeature4096::newSource()
{
  uint8_t count = getSourceCount();
  m_source[0][0] = &I2SDataSRAM2_0[0];
  m_source[1][0] = &I2SDataSRAM2_1[0];
  if (m_source[0][0] == NULL || m_source[1][0] == NULL) // Pointer assignment failed
  {
    return false;
  }
  return true;
}

