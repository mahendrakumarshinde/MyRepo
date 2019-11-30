#include "IUKX222.h"
#include "IUkx224reg.h"

IUKX222::IUKX222(SPIClass *spiPtr, uint8_t csPin, SPISettings settings,
					const char* name,void (*SPIReadCallback)(),
					FeatureTemplate<q15_t> *accelerationX,
					FeatureTemplate<q15_t> *accelerationY,
					FeatureTemplate<q15_t> *accelerationZ): 
HighFreqSensor(name,3, accelerationX, accelerationY, accelerationZ),
m_spi(spiPtr),
m_cspin(csPin),
m_spiSettings(settings),
m_scale(defaultScale),
m_odr(defaultODR),
m_readCallback(SPIReadCallback)
{
	for (uint8_t i = 0; i < 3; ++i)
		{
			m_bias[i] = 0;
		}
}

void IUKX222::SetupSPI()
{
	pinMode(m_cspin, OUTPUT);
	digitalWrite(m_cspin, HIGH);
	m_spi->begin();
	m_spi->beginTransaction(m_spiSettings);
}

bool IUKX222::checkWHO_AM_I()
{
	uint8_t c = readConfig(IUKX222_WHO_AM_I);
	if (c != IUKX222_WHO_AM_I_WIA_ID){
		kionixPresence = false;
		return false;
	}
	kionixPresence = true;
	return true;
}

void IUKX222::softReset()
{
	uint8_t reg = 0;
	reg |= KX224_CNTL2_SRST;
	writeConfig(KX224_CNTL2,reg);
	// Wait for reset
	delay(100);
}

bool IUKX222::sanityCheck()
{
	// read COTR register - should be 0x55
	uint8_t x = readConfig(KX224_COTR);
	if (x != KX224_COTR_DCSTR_BEFORE)
		return false;

	writeConfig(KX224_CNTL2, KX224_CNTL2_COTC);

	x = readConfig(KX224_COTR);
	if (x != KX224_COTR_DCSTR_AFTER)
		return false;

	return true;
}

void IUKX222::setScale(IUKX222::ScaleOption scale)
{
	m_scale = scale;
	uint8_t reg = 0;
	operate(false);
	// CNTL1
	reg |= KX224_CNTL1_RES;		// high resolution mode ON
	reg |= KX224_CNTL1_DRDYE;	// DRDY interrupt ON
	reg |= uint8_t(scale);      // range
	// all other detection options OFF (0)
	writeConfig(KX224_CNTL1, reg);
	switch (scale)
	{
		case FSR_8G:
			setResolution(8.0 * 9.80665 / 32768.0);
			break;
		case FSR_16G:
			setResolution(16.0 * 9.80665 / 32768.0);
			break;
		case FSR_32G:
			setResolution(32.0 * 9.80665 / 32768.0);
			break;
	}
		operate(true);
}

void IUKX222::setSamplingRate(uint16_t samplingRate)
{
	m_samplingRate = samplingRate;
	HighFreqSensor::setSamplingRate(samplingRate);

	if (samplingRate > 12800) { m_odr = ODR_25600Hz; }
	else if (samplingRate > 6400) { m_odr = ODR_12800Hz; }
	else if (samplingRate > 3200) { m_odr = ODR_6400Hz; }
	else if (samplingRate > 1600) { m_odr = ODR_3200Hz; }
	else if (samplingRate > 800) { m_odr = ODR_1600Hz; }
	else if (samplingRate > 400) { m_odr = ODR_800Hz; }
	else if (samplingRate > 200) { m_odr = ODR_400Hz; }
	else if (samplingRate > 100) { m_odr = ODR_200Hz; }
	else if (samplingRate > 50) { m_odr = ODR_100Hz; }
	else if (samplingRate > 25) { m_odr = ODR_50Hz; }
	else if (samplingRate > 12) { m_odr = ODR_25Hz; }
	else { m_odr = ODR_12_5Hz; }
	operate(false);
	// ODCNTL settings configured by user
	writeConfig(KX224_ODCNTL, uint8_t(m_odr));
	operate(true);
}

void IUKX222::configure(JsonVariant &config)
{
    HighFreqSensor::configure(config);  // General sensor config
    JsonVariant value = config["FSR"];  // Full Scale Range
    if (value.success())
    {
        setScale((ScaleOption) (value.as<int>()));
    }
}

void IUKX222::avgFilterCntl(LpfSetting lpf)
{
	operate(false);
	// LP_CNTL : averaging filter
	uint8_t reg = uint8_t(lpf) | (11);
	writeConfig(KX224_LP_CNTL, reg);
	operate(true);
}

void IUKX222::setupHardware()
{
	SetupSPI();
	// put device into standby mode first
	operate(false);
	if(!checkWHO_AM_I() || !sanityCheck()){
		debugPrint("Kionix KX222 Error");
	} else {
		debugPrint("Kionix KX222 Found");
	}
	softReset();

	// set resolution
	setScale(defaultScale);

	// tilt detection off
	writeConfig(KX224_CNTL2, 0);

	// refresh rates for motion detection lowest (off anyway)
	writeConfig(KX224_CNTL3, 0);

	// ODCNTL settings configured by user
	setSamplingRate(m_samplingRate);

	/// INC2 disable motion detection
	writeConfig(KX224_INC2, 0);

	/// INC3 disable motion detection
	writeConfig(KX224_INC3, 0);

	/// INC5 disable motion detection
	writeConfig(KX224_INC5, 0);

	/// INC6 disable motion detection
	writeConfig(KX224_INC6, 0);

	// TDTRC disable tap events reporting
	writeConfig(KX224_TDTRC, 0);

	// FFCTNL disable freefall detection
	writeConfig(KX224_FFCNTL, 0);

	// // LP_CNTL : averaging filter
	avgFilterCntl(defaultLPF);

	// buffer full interrupt
	writeConfig(KX224_BUF_CNTL1, 0);
	uint8_t reg = 0;
	// BUF_CNTL2 : buffer configuration
	reg = 0;
	reg |= KX224_BUF_CNTL2_BUFE;         // buffer active
	reg |= KX224_BUF_CNTL2_BRES;         // 16bit samples in buffer
	reg &= KX224_BUF_CNTL2_BFIE;         // buffer interrupt disabled
	reg |= KX224_BUF_CNTL2_BUF_M_STREAM; // stream buffer mode
	writeConfig(KX224_BUF_CNTL2, reg);

	// clear buffer
	writeConfig(KX224_BUF_CLEAR, 0);

	// switch off self-test
	writeConfig(KX224_SELF_TEST, 0);

	// Enable intrrupt on INT1
	configureInterrupts();
	
	// switch operation  back on
	operate(true);
	setPowerMode(PowerMode::REGULAR);
}

void IUKX222::configureInterrupts()
{
	// INC1 interrupts off, SPI 4-wire
	writeConfig(KX224_INC1, 0x38);
	/// INC4 disable motion detection
	writeConfig(KX224_INC4, 0x10);
}

void IUKX222::setPowerMode(PowerMode::option pMode)
{
    m_powerMode = pMode;
    // TODO Implement
    switch (m_powerMode)
    {
        case PowerMode::PERFORMANCE:
        case PowerMode::ENHANCED:
        case PowerMode::REGULAR:
        case PowerMode::LOW_1:
        case PowerMode::LOW_2:
        case PowerMode::SLEEP:
        case PowerMode::DEEP_SLEEP:
        case PowerMode::SUSPEND:
            // TODO: Implement
            break;
        default:
            if (debugMode)
            {
                debugPrint(F("Unhandled power Mode "), false);
                debugPrint(m_powerMode);
            }
    }
}

void IUKX222::setResolution(float resolution)
{
    m_resolution = resolution;
    for (uint8_t i = 0; i < 3; ++i)
    {
        m_destinations[i]->setResolution(m_resolution);
    }
}

// Refer http://kionixfs.kionix.com/en/datasheet/KX222-1054-Specifications-Rev-2.0.pdf Ref Pg No. 42
// It applay for all write registers
void IUKX222::operate(bool enable)
{
	uint8_t cntl = readConfig(KX224_CNTL1);
	if (enable)
		cntl |= KX224_CNTL1_PC1;
	else
		cntl &= ~KX224_CNTL1_PC1;
	writeConfig(KX224_CNTL1, cntl);

	// wait for transition to take effect
	delay(20);
}

void IUKX222::readData()
{
	uint16_t bitLow, bitHigh;
	int16_t acc;
	// m_spi->beginTransaction(m_spiSettings);
	digitalWrite(m_cspin, LOW);
	//Since we're performing a read operation, the most significant bit (bit 7(counting starts from bit 0)) of the register address should be set high.
	int x = m_spi->transfer(KX224_XOUT_L | 0x80);
	//Continue to read registers until we've read the number specified, storing the results to the input buffer.
	//SPI is full duplex transsmission. Every signle time SPI.transfer can and can only transmit 8 bits. 
	//the input parameter will be delived to slave, and it's return will be the data that master device and get from the slave device. 
	//The data returned during master device's write operation makes no sense, hence always been ignored by not doing any assignment operation.
	for (int i=0;i<3;++i)
	{
		bitLow = m_spi->transfer(0);
		bitHigh = m_spi->transfer(0);
		acc = (bitHigh << 8) | bitLow;
		m_rawData[i] = acc;
	}
	digitalWrite(m_cspin, HIGH);
	// m_spi->endTransaction();
	m_readCallback();
}

void IUKX222::processData()
{
	for (uint8_t i = 0; i < 3; ++i) {
		// Accelaration data
		m_data[i] = m_rawData[i] + m_bias[i];
		m_destinations[i]->addValue(m_data[i]);
	}
}

void IUKX222::sendData(HardwareSerial *port)
{
    if (loopDebugMode) {
        // Human readable in the console
        port->println(millis());
        port->print("AX: ");
        port->println((float) m_data[0] * m_resolution / 9.80665, 4);
        port->print("AY: ");
        port->println((float) m_data[1] * m_resolution / 9.80665, 4);
        port->print("AZ: ");
        port->println((float) m_data[2] * m_resolution / 9.80665, 4);
    } else {
        // Send bytes
        float accel;
        byte* data;
        for (uint8_t i = 0; i < 3; i++) {
            // Stream float value as 4 bytes
            accel = (float) m_data[i] * m_resolution / 9.80665;
            data = (byte*) &accel;
            port->write(data, 4);
        }
    }
}

/**
 * Dump acceleration data to serial - unit is G, in float format
 *
 * NB: We want to do this in * CUSTOM DATA COLLECTION* mode
 * return the x,y,z acceleration
 */
float* IUKX222::getData(HardwareSerial *port)
{
    static float destinations[3];
    if (loopDebugMode) {
        // Human readable in the console
        port->println(millis());
        port->print("AX: ");
        port->println((float) m_data[0] * m_resolution / 9.80665, 4);
        port->print("AY: ");
        port->println((float) m_data[1] * m_resolution / 9.80665, 4);
        port->print("AZ: ");
        port->println((float) m_data[2] * m_resolution / 9.80665, 4);
    } else {
        // Send bytes
        //float rawAccelbuff[3];
        float accel;
        byte* data;
        for (uint8_t i = 0; i < 3; ++i) {
            // Stream float value as 4 bytes
            accel = (float) m_data[i] * m_resolution / 9.80665;
            
            data = (byte*) &accel;
            
            destinations[i] = accel;                
        }       
    }
  return destinations;
}

// uint32_t IUKX222::readSampleBuffer(int16_t buf[])
// {
//   // retrieve the number of bytes currently in the buffer
//   uint8_t stat1 = readConfig(KX224_BUF_STATUS_1);
//   uint8_t stat2 = readConfig(KX224_BUF_STATUS_2);

//   // mask of the flag bits in BUF_STATUS_2
//   stat2 &= (7);
//   uint32_t nbytes = (uint8_t(stat2) << 8) | stat1;
//   uint32_t nval = nbytes / 2;

//   // uint8_t *bytes = reinterpret_cast<uint8_t*>(buf);

//   if (nbytes > 0)
//   {

//     uint16_t tf = KX224_BUF_READ;

//     m_spi->beginTransaction(SPISettings(m_spiclock, MSBFIRST, SPI_MODE0));
//     digitalWrite(m_cspin, LOW);

//     // could use a single transferBytes for the part that is
//     // divisible by four, or just pickup th entire buffer in any case

//     for (uint32_t i = 0; i < nval; ++i)
//       buf[i] = m_spi->transfer16(tf);

//     digitalWrite(m_cspin, HIGH);
//     m_spi->endTransaction();

//     // byteswap
//     for (uint32_t i = 0; i < nval; ++i)
//     {
//       uint16_t blo = (buf[i] & 0xff);
//       uint16_t bhi = (buf[i] >> 8);
//       buf[i] = (blo << 8) | bhi;
//     }
//   }

//   return nbytes;
// }

uint8_t IUKX222::readConfig(uint8_t addr)
{
	// m_spi->beginTransaction(m_spiSettings);
	digitalWrite(m_cspin, LOW);

	uint8_t reg;
	reg = m_spi->transfer(addr | 0x80);
	reg = m_spi->transfer(0);

	digitalWrite(m_cspin, HIGH);
	// m_spi->endTransaction();

	return reg;
}

void IUKX222::writeConfig(uint8_t addr, uint8_t value)
{
	// m_spi->beginTransaction(m_spiSettings);
	digitalWrite(m_cspin, LOW);
	m_spi->transfer(addr & ~0x80);
	m_spi->transfer(value);
	digitalWrite(m_cspin, HIGH);
	// m_spi->endTransaction();
}

void IUKX222::exposeCalibration()
{
#ifdef IUDEBUG_ANY
    debugPrint(F("Accelerometer calibration data: "));
    debugPrint(F("Resolution (m.s-2): "));
    debugPrint(m_resolution, 7);
    debugPrint(F("Bias (q15_t): "));
    for (uint8_t i = 0; i < 3; ++i) {
        debugPrint(m_bias[i], false);
        debugPrint(", ", false);
    }
    debugPrint(' ');
#endif
}
