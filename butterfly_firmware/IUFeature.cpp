#include "IUFeature.h"


/*====================== Scalar Feature Computation functions ============================ */

//arm_rfft_instance_q15 rfftInstance;

/**
 * Compute and return the total signal energy along one or several axis
 * @param sourceCount the number of sources (= the number of axis)
 * @param sourceSize  pointer to an array of size sourceCount
 * @param source      pointer to an array q15_t[sourceCount][sourceSize[i for i in sourceCount]]
 */
float computeSignalEnergy(uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source)
{
  float energy = 0;
  float totalEnergy = 0;
  for (uint8_t i = 0; i < sourceCount; i++)
  {
    for (uint16_t j = 0; j < sourceSize[i]; j++)
    {
      energy += sq(q15ToFloat(source[i][j]));
    }
    totalEnergy += energy / (float) sourceSize[i];
    energy = 0;
  }
  totalEnergy /= (float) sourceCount;
  return totalEnergy;
}

/**
 * Compute and return the normalized sum of all sources
 * @param sourceCount the number of sources
 * @param sourceSize  pointer to an array of size sourceCount
 * @param source      pointer to an array q15_t[sourceCount][sourceSize[i for i in sourceCount]]
 */
float computeSumOf(uint8_t sourceCount, const uint16_t* sourceSize, float **source)
{
  float total = 0;
  float grandTotal = 0;
  for (uint8_t i = 0; i < sourceCount; i++)
  {
    for (uint16_t j = 0; j < sourceSize[i]; j++)
    {
      total += q15ToFloat(source[i][j]);
    }
    grandTotal += total / (float) sourceSize[i];
    total = 0;
  }
  grandTotal /= (float) sourceCount;
  return grandTotal;
}


//TODO: Check what windowing function work best + maybe test some other pitch detection method
/**
 * Compute RFFT and return the index of the max factor - WORK FOR A SINGLE SOURCE
 * Since a single index is returned, only work for a single source (sourceCount = 1)
 * Check out this example: https://www.keil.com/pack/doc/CMSIS/DSP/html/arm_fft_bin_example_f32_8c-example.html
 * What we try to do is basically pitch detection: https://en.wikipedia.org/wiki/Pitch_detection_algorithm
 * However, have a look here: http://dsp.stackexchange.com/questions/11312/why-should-one-use-windowing-functions-for-fft
 * @param sourceCount the number of sources - must be 1
 * @param sourceSize  pointer to an array of size 1 - must be {512} or {2048} in current implementation
 * @param source      pointer to an array q15_t[sourceCount][sourceSize[i for i in sourceCount]]
 */
float computeRFFTMaxIndex(uint8_t sourceCount, const uint16_t* sourceSize, q15_t **source)
{
  if (sourceCount != 1)
  {
    return -1; // sourceCount expected to be 1
  }
  uint16_t ss = sourceSize[0];
  q15_t compBuffer[ss];
  switch(ss)
  {
    case 512:
      arm_mult_q15(source[0], hamming_window_512, compBuffer, ss);
      break;
    case 2048:
      arm_mult_q15(source[0], hamming_window_2048, compBuffer, ss);
      break;
    default:
      return -1; //Fail if sourceSize is not supported
  }
  // RFFT
  arm_rfft_instance_q15 rfftInstance;
  q15_t rfftBuffer[ss];
  arm_status status = arm_rfft_init_q15(&rfftInstance,  // RFFT instance
                                        ss        ,     // FFT length
                                        0,              // 0: forward FFT, 1: inverse FFT
                                        1);             // 0: disable bit reversal output, 1: enable it
  if (status != ARM_MATH_SUCCESS)
  {
      return -1; // failure
  }
  arm_rfft_q15(&rfftInstance, compBuffer, rfftBuffer);
  // TODO check what is better between block 1 and block 2
  // block 1 ===========================================
  // Find index of max factor
  float flatness(0), index(0);
  // TODO: check why we start at index 2
  for (int i = 2; i < ss; i++) {
    if (rfftBuffer[i] > flatness) {
      flatness = rfftBuffer[i];
      index = i;
    }
  }
  // ===================================================
  // block 2 ===========================================
  /*
  q15_t flatness = 0;
  uint32_t index = 0;
  // TODO: check why we start at index 2
  arm_max_q15(&rfftBuffer[2],   // array
              ss - 2,   // array size
              flatness,
              index);
  */
  // ===================================================
  return ((float) index);
}

/**
 * Compute and return single axis velocity using acceleration RMS and pitch
 * @param sourceCount the number of sources - should be 2
 * @param sourceSize  pointer to an array of size sourceCount - eg: {512, 1}
 * @param source      should be {acceleration values,
 *                               Acceleration energy operationState}
 */
float computeVelocity(uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source)
{
  if (source[1][0] == operationState::idle)
  {
    return 0;
  }
  else
  {
    float maxIndex = computeRFFTMaxIndex(1, sourceSize, source);
    return computeRMS(sourceSize[0], source[0]) * 1000 / (2 * PI * maxIndex);
  }
}

/**
 * Compute and return single axis velocity using acceleration RMS and pitch
 * @param sourceCount the number of sources - should be 2
 * @param sourceSize  pointer to an array of size sourceCount - eg: {512, 1}
 * @param source      should be {acceleration values,
 *                               Acceleration energy operationState}
 */
float computeFullVelocity(uint8_t sourceCount, const uint16_t *sourceSize, q15_t **source)
{
  if (source[1][0] == operationState::idle)
  {
    return 0;
  }
  else
  {
    float maxIndex = computeRFFTMaxIndex(1, sourceSize, source);
    return computeRMS(sourceSize[0], source[0]) * 1000 / (2 * PI * maxIndex);
  }
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
  return audioDB;
}


/*====================== Scalar Feature Computation functions ============================ */

/**
 * Compute RFFT and put it in destination - expects only 1 source
 * NB - this function is destructive -
 * To save space, it will modify the source itself, so the source won't be usable afterward.
 * @param sourceCount      the number of sources - must be 1
 * @param sourceSize       pointer to an array of size 1 - must be {512} or {2048} in current implementation
 * @param source           pointer to an array q15_t[sourceCount][sourceSize[i for i in sourceCount]]
 * @param destinationSize  the size of the destination array
 * @param destination      pointer to an array to collect the results
 * @return                 true if the computation succeeded, else false.
 */
bool computeRFFT(uint8_t sourceCount, const uint16_t* sourceSize, q15_t **source, const uint16_t destinationSize, q15_t *destination)
{
  if (sourceCount != 1)
  {
    return false; // sourceCount expected to be 1
  }
  uint16_t ss = sourceSize[0];
  // Apply Hamming window in place
  switch(ss)
  {
    case 512:
      arm_mult_q15(source[0], hamming_window_512, source[0], ss);
      break;
    case 2048:
      arm_mult_q15(source[0], hamming_window_2048, source[0], ss);
      break;
    default:
      return false; //Fail if sourceSize is not supported
  }
  // RFFT
  arm_rfft_instance_q15 rfftInstance;
  arm_status status = arm_rfft_init_q15(&rfftInstance,   // RFFT instance
                                        destinationSize, // FFT length
                                        0,               // 0: forward FFT, 1: inverse FFT
                                        1);              // 0: disable bit reversal output, 1: enable it
  if (status != ARM_MATH_SUCCESS)
  {
      return false; // failure
  }
  arm_rfft_q15(&rfftInstance, source[0], destination);
}

/**
 * Single instance RFFT calculation on audio data - expects only 1 source
 * NB - this function is destructive -
 * To save space, it will modify the source itself, so the source won't be usable afterward.
 * The function use 2 successive RFFT calculation of half the size of the source
 * @param sourceCount      the number of sources - must be 1
 * @param sourceSize       pointer to an array of size 1 - must be {1024} or {4096} in current implementation
                           RFFT will then be of size 512 or 2048
 * @param source           pointer to an array q15_t[sourceCount][sourceSize[i for i in sourceCount]]
 * @param destinationSize  the size of the destination array
 * @param destination      pointer to an array to collect the results
 * @return                 true if the computation succeeded, else false.
 */
bool computeAudioRFFT(uint8_t sourceCount, const uint16_t* sourceSize, q15_t **source, const uint16_t destinationSize, q15_t *destination)
{
  //TODO: rebuild this function, it is not working
  if (setupDebugMode) { debugPrint(F("Need to refactor the function computeAudioRFFT")); }
  return false;

  //TODO allow to pass this as argument (maybe in the source)
  uint8_t AUDIO_RESCALE = 10;

  if (sourceCount != 1)
  {
    return false; // sourceCount expected to be 1
  }
  uint16_t ss = sourceSize[0];
  const uint16_t rfftSize = destinationSize / 2;
  float magSize(0), hammingK(0), inverseWLen(0);
  switch(rfftSize)
  {
    case 512:
      arm_mult_q15(source[0], hamming_window_512, source[0], rfftSize);
      arm_mult_q15(&source[0][rfftSize], hamming_window_512, &source[0][rfftSize], rfftSize);
      magSize = magsize_512;
      hammingK = hamming_K_512;
      inverseWLen = inverse_wlen_512;
      break;
    case 2048:
      arm_mult_q15(source[0], hamming_window_2048, source[0], rfftSize);
      arm_mult_q15(&source[0][rfftSize], hamming_window_2048, &source[0][rfftSize], rfftSize);
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
    arm_status status = arm_rfft_init_q15(&rfftInstance,   // RFFT instance
                                          destinationSize, // FFT length
                                          0,               // 0: forward FFT, 1: inverse FFT
                                          1);              // 0: disable bit reversal output, 1: enable it
    arm_rfft_q15(&rfftInstance, &source[0][i * rfftSize], destination);
    arm_shift_q15(&destination[i * rfftSize], AUDIO_RESCALE, &destination[i * rfftSize], ss);
  }

  // NOTE: OUTPUT FORMAT IS IN 2.14
  arm_cmplx_mag_q15(destination, source[0], ss);
  arm_q15_to_float(source[0], (float*)destination, magSize);

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


/* ================== IUFeature Definitions ==================== */

IUFeature::IUFeature(uint8_t id, char *name) :
  IUABCProducer(),
  IUABCFeature(id, name)
{
  // ctor
}

void IUFeature::sendToReceivers()
{
  Serial.print(m_name);
  Serial.print(" check: ");
  Serial.println(m_receiverCount);
  for (uint8_t i = 0; i < m_receiverCount; i++)
  {
    if (m_receivers[i])
    {
      switch(m_toSend[i])
      {
        case (uint8_t) dataSendOption::value:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], m_latestValue);
          break;
        case (uint8_t) dataSendOption::state:
          m_receivers[i]->receiveScalar(m_receiverSourceIndex[i], (q15_t) m_state);
          break;
         case (uint8_t) dataSendOption::valueArray:
          m_receivers[i]->receiveArray(m_receiverSourceIndex[i]);
          break;
      }
    }
  }
}


bool IUFeature::addArrayReceiver(uint8_t receiverSourceIndex, IUABCFeature *receiver)
{
  return IUABCProducer::addArrayReceiver(getDestinationSize(), m_destination, receiverSourceIndex, receiver);
}

bool IUFeature::addReceiver(uint8_t sendOption, uint8_t receiverSourceIndex, IUABCFeature *receiver)
{
  if (sendOption == 99)
  {
    m_toSend[m_receiverCount] = sendOption;
    return addArrayReceiver(receiverSourceIndex, receiver);
  }
  else
  {
    return addScalarReceiver(sendOption, receiverSourceIndex, receiver);
  }
}

/* ================== IUQ15Feature Definitions ==================== */

IUQ15Feature::IUQ15Feature(uint8_t id, char *name) :
  IUFeature(id, name)
  {
    m_computeScalarFunction = computeDefaultQ15;
  }

/**
 * Creates the source pointers
 * @return true if pointer assignments succeeded, else false
 */
bool IUQ15Feature::newSource()
{
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < getSourceCount(); j++)
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
  // Delete previous source assignment
  delete m_source[0][sourceIndex]; m_source[0][sourceIndex] = NULL;
  delete m_source[1][sourceIndex]; m_source[1][sourceIndex] = NULL;
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
  }
  if (isTimeToEndRecord())
  {
    if (loopDebugMode)
    {
      debugPrint(m_name, false);
      debugPrint(F(" end of "), false);
      debugPrint(m_recordIndex, false);
      debugPrint(": ", false);
      debugPrint(millis());
    }
    Serial.print(m_recordIndex);
    Serial.print(", ");
    Serial.print(m_computeNow[m_recordIndex][0]);
    Serial.print(", ");
    Serial.print(m_recordNow[m_recordIndex][0]);
    Serial.print(", ");
    m_recordIndex = m_recordIndex == 0 ? 1 : 0; //switch buffer index
    Serial.println(m_recordIndex);
  }
  return true;
}


/* ================== IUFloatFeature Definitions ==================== */

IUFloatFeature::IUFloatFeature(uint8_t id, char *name) :
  IUFeature(id, name)
{
  m_computeScalarFunction = computeDefaultFloat;
}

/**
 * Creates the source pointers
 * @return true is pointer assignments succeeded, else false
 */
bool IUFloatFeature::newSource()
{
  for (uint8_t i = 0; i < 2; i++)
  {
    for (uint16_t j = 0; j < getSourceCount(); j++)
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
  // Delete previous source assignment
  delete m_source[0][sourceIndex]; m_source[0][sourceIndex] = NULL;
  delete m_source[1][sourceIndex]; m_source[1][sourceIndex] = NULL;
  m_source[0][sourceIndex] = values;
  m_source[1][sourceIndex] = values;
  return true;
}

/**
 * Receive a new source valu, it will be stored if the source buffers are ready
 * @return true if the value was stored in source buffer, else false
 */
bool IUFloatFeature::receiveScalar(uint8_t sourceIndex, float value)
{
  if (!m_recordNow[m_recordIndex][sourceIndex])                         // Recording buffer is not ready
  {
     return false;
  }
  if (m_sourceCounter[sourceIndex] < getSourceSize(sourceIndex))        // Recording buffer is not full
  {
      m_source[m_recordIndex][sourceIndex][m_sourceCounter[sourceIndex]] = value;
      m_sourceCounter[sourceIndex]++;
  }
  if (m_sourceCounter[sourceIndex] == getSourceSize(sourceIndex))       // Recording buffer is now full
  {
    m_recordNow[m_recordIndex][sourceIndex] = false;                    //Do not record anymore in this buffer
    m_computeNow[m_recordIndex][sourceIndex] = true;                    //Buffer ready for computation
    m_sourceCounter[sourceIndex] = 0;                                   //Reset counter
  }
  if (isTimeToEndRecord())
  {
    m_recordIndex = m_recordIndex == 0 ? 1 : 0; //switch buffer index
  }
  return true;
}


/* ========================== Speicalized Feature Classes ============================= */

const uint16_t IUAccelPreComputationFeature128::sourceSize[IUAccelPreComputationFeature128::sourceCount] = {128};

IUAccelPreComputationFeature128::IUAccelPreComputationFeature128(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
   m_computeArrayFunction = computeRFFT;
   m_computeScalarFunction = computeSignalEnergy;
}


const uint16_t IUAccelPreComputationFeature512::sourceSize[IUAccelPreComputationFeature512::sourceCount] = {512};

IUAccelPreComputationFeature512::IUAccelPreComputationFeature512(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
   m_computeArrayFunction = computeRFFT;
   m_computeScalarFunction = computeSignalEnergy;
}


const uint16_t IUSingleAxisEnergyFeature128::sourceSize[IUSingleAxisEnergyFeature128::sourceCount] = {128};

IUSingleAxisEnergyFeature128::IUSingleAxisEnergyFeature128(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
   m_computeScalarFunction = computeSignalEnergy;
}


const uint16_t IUTriAxisEnergyFeature128::sourceSize[IUTriAxisEnergyFeature128::sourceCount] = {128, 128, 128};

IUTriAxisEnergyFeature128::IUTriAxisEnergyFeature128(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
   m_computeScalarFunction = computeSignalEnergy;
}


const uint16_t IUSingleAxisEnergyFeature512::sourceSize[IUSingleAxisEnergyFeature512::sourceCount] = {512};

IUSingleAxisEnergyFeature512::IUSingleAxisEnergyFeature512(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
   m_computeScalarFunction = computeSignalEnergy;
}


const uint16_t IUTriAxisEnergyFeature512::sourceSize[IUTriAxisEnergyFeature512::sourceCount] = {512, 512, 512};

IUTriAxisEnergyFeature512::IUTriAxisEnergyFeature512(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
   m_computeScalarFunction = computeSignalEnergy;
}


const uint16_t IUTriSourceSummingFeature::sourceSize[IUTriSourceSummingFeature::sourceCount] = {1, 1, 1};

IUTriSourceSummingFeature::IUTriSourceSummingFeature(uint8_t id, char *name) :
  IUFloatFeature(id, name)
{
  m_computeScalarFunction = computeSumOf;
}


const uint16_t IUVelocityFeature512::sourceSize[IUVelocityFeature512::sourceCount] = {512, 1};

IUVelocityFeature512::IUVelocityFeature512(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  m_computeScalarFunction = computeVelocity;
}

/**
 * Creates the source pointers
 * @return true if pointer assignments succeeded, else false
 */
bool IUVelocityFeature512::newSource()
{
  for (uint8_t i = 0; i < 2; i++)
  {
    m_source[i][0] = new q15_t[1];
    m_source[i][1] = new q15_t[1];
    if (m_source[i][0] == NULL || m_source[i][1] == NULL) // Pointer assignment failed
    {
      return false;
    }
  }
  return true;
}


const uint16_t IUDefaultFloatFeature::sourceSize[IUDefaultFloatFeature::sourceCount] = {1};

IUDefaultFloatFeature::IUDefaultFloatFeature(uint8_t id, char *name) :
  IUFloatFeature(id, name)
{
  m_computeScalarFunction = computeDefaultFloat;
}


const uint16_t IUAudioDBFeature2048::sourceSize[IUAudioDBFeature2048::sourceCount] = {2048};

IUAudioDBFeature2048::IUAudioDBFeature2048(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
   m_computeScalarFunction = computeAcousticDB;
}


const uint16_t IUAudioDBFeature4096::sourceSize[IUAudioDBFeature4096::sourceCount] = {4096};

IUAudioDBFeature4096::IUAudioDBFeature4096(uint8_t id, char *name) :
  IUQ15Feature(id, name)
{
  m_computeScalarFunction = computeAcousticDB;
}

