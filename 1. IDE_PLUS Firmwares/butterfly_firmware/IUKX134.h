#ifndef IUKX134_H
#define IUKX134_H
#include <SPI.h>
#include "FeatureUtilities.h"
#include "Sensor.h"

class IUKX134 : public HighFreqSensor
{
public:

  enum OdrSetting {ODR_12_5Hz = 0x04,
                    ODR_25Hz = 0x05,
                    ODR_50Hz = 0x06,
                    ODR_100Hz = 0x07,
                    ODR_200Hz = 0x08,
                    ODR_400Hz = 0x09,
                    ODR_800Hz = 0x0A,
                    ODR_1600Hz = 0x0B,
                    ODR_3200Hz = 0x0C,
                    ODR_6400Hz = 0x0D,
                    ODR_12800Hz = 0x0E,
                    ODR_25600Hz = 0x0F,
                    IIR_FLTR_ODR2 = (1 << 6),      // otherwise: ODR/9
                    IIR_FLTR_BYPASS = (1 << 7) };  // otherwise: IIR on 

    enum LpfSetting { LPF_OFF = 0,
                    LPF_AVG2 = (1 << 4),
                    LPF_AVG4 = (2 << 4),
                    LPF_AVG8 = (3 << 4),
                    LPF_AVG16 = (4 << 4), // default 
                    LPF_AVG32 = (5 << 4),
                    LPF_AVG64 = (6 << 4),
                    LPF_AVG128 = (7 << 4) };

    enum ScaleOption { FSR_8G = (0x00 << 3),
                        FSR_16G = (0x01 << 3),
                        FSR_32G = (0x02 << 3),
                        FSR_64G = (0x03 << 3) };
    IUKX134(SPIClass *spiPtr, uint8_t csPin, SPISettings settings,const char* name,
                    void (*SPIReadCallback)(),
                    FeatureTemplate<q15_t> *accelerationX,
                    FeatureTemplate<q15_t> *accelerationY,
                    FeatureTemplate<q15_t> *accelerationZ);

    static const uint8_t INT1_PIN         = 42; // STM pin

    static const ScaleOption defaultScale = FSR_8G;

    static const OdrSetting defaultODR = ODR_25600Hz;

    static const LpfSetting defaultLPF = LPF_AVG16;

    static const uint16_t defaultSamplingRate = 25600; // Hz
    uint16_t m_samplingRate = defaultSamplingRate;
    const int DEFAULT_BLOCK_SIZE = 8192;

    bool kionixPresence = false;
    /// Checking WHO_AM_I
    bool checkWHO_AM_I();

    ///software reset, which performs the RAM reboot routine.
    void softReset();

    /// setup sensor to use a given SPI controller and cs pin 
    void SetupSPI();

    /// setup sensor resoluton
    void setScale(ScaleOption scale);
    ScaleOption getScale() { return m_scale; }
    void resetScale() { setScale(defaultScale); }
    virtual void setResolution(float resolution);
    void setGrange(uint8_t g);

    /// setup sensor sampling rate 12Hz to 25600Hz
    virtual void setSamplingRate(uint16_t samplingRate);
    void updateSamplingRate(uint16_t samplingRate){m_samplingRate = samplingRate;};
    

    /// setup average filter
    void avgFilterCntl(LpfSetting lpf);

    /// basic SPI stup and Hardware Setup
    virtual void setupHardware();
    void configureInterrupts();

    /// chip and SPI communication check (COTR register)
    bool sanityCheck();

    /// retrieve all 3 latest accelerometer values (x, y, z)
    float* getData(HardwareSerial *port);
    void sendData(HardwareSerial *port);

    /// data acquisition
    virtual void readData();
    void processData();

    virtual void setPowerMode(PowerMode::option pMode);
    virtual void configure(JsonVariant &config);
    virtual void exposeCalibration();

    /// retrieve the contents of the device buffer (2kB, 339 samples max) returns number of bytes
    // uint32_t readSampleBuffer(int16_t buf[]);
  
protected:
    /// write to configuration register 
    void writeConfig(uint8_t addr, uint8_t value);

    /// read byte from configuration register 
    uint8_t readConfig(uint8_t addr);

    /// put device in standby/operate mode to change configuration 
    void operate(bool enable);

    /// SPI controller to use  
    SPIClass *m_spi;

    /// chip-select pin to use 
    uint8_t m_cspin;

    SPISettings m_spiSettings;

    void (*m_readCallback)();

    float m_res = 0;

    OdrSetting m_odr;

    ScaleOption m_scale;

    q15_t m_rawData[3];     // Q15 accelerometer raw output

    q15_t m_data[3]; 
    q15_t m_bias[3];        // Bias corrections

    uint8_t retryCount = 3;

};
#endif