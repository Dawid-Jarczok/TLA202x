#pragma once

#include <Arduino.h>
#include <Wire.h>

class TLA202x {
public:

    enum OperatingMode {
        OP_CONTINUOUS = 0,
        OP_SINGLE = 1
    };

    enum DataRate {
        DR_128SPS = 0x0,
        DR_250SPS = 0x1,
        DR_490SPS = 0x2,
        DR_920SPS = 0x3,
        DR_1600SPS = 0x4,
        DR_2400SPS = 0x5,
        DR_3300SPS = 0x6,
    };

    enum FullScaleRange {
        FSR_6_144V = 0x0,
        FSR_4_096V = 0x1,
        FSR_2_048V = 0x2,
        FSR_1_024V = 0x3,
        FSR_0_512V = 0x4,
        FSR_0_256V = 0x5,
    };

    enum MultiplexerConfig {
        MUX_AIN0_AIN1 = 0x0,
        MUX_AIN0_AIN3 = 0x1,
        MUX_AIN1_AIN3 = 0x2,
        MUX_AIN2_AIN3 = 0x3,
        MUX_AIN0_GND = 0x4,
        MUX_AIN1_GND = 0x5,
        MUX_AIN2_GND = 0x6,
        MUX_AIN3_GND = 0x7,
    };

    TLA202x(TwoWire *wire);

    /*
      Initializes I2C bus
      @param address adc address, default 0x48
      @returns
         true - adc responds with correct default conf,
         false - otherwise
    */
    bool begin(uint8_t address = 0x48);

    // Resets device to default configuration
    void reset();

    // Restores device to last config
    void restore();

    /*
      Get analog reading for current multiplexer setting
      @return analog value
    */
    int16_t analogRead();

    /*
      Set full scale range and get analog reading for current multiplexer setting
      @param range full scale range
      @return analog value
    */
    int16_t analogRead(FullScaleRange range);

    /*
      Convenient method to get analog reading of a channel compared to GND
      @param channel channel to read
      @return analog value
    */
    int16_t analogRead(uint8_t channel);

    /*
      Set full scale range and get analog reading of a channel compared to GND
      @param channel channel to read
      @param range full scale range
      @return analog value
    */
    int16_t analogRead(uint8_t channel, FullScaleRange range);

    /*
      Sets the Full Scale Range of the ADC
      @param range full scale range
      @param write write immediately, default true
    */
    void setFullScaleRange(FullScaleRange range, bool write = true);

    /*
      Set multiplexer configuration
      @param option mux config
      @param write write immediately, default true
    */
    void setMuxConfig(MultiplexerConfig option, bool write = true);

    /*
      Continous conversion or single shot
      @param mode mode
      @param write write immediately, default true
    */
    void setOperatingMode(OperatingMode mode, bool write = true);

    /*
      Set data rate setting
      @param rate data rate
      @param write write immediately, default true
    */
    void setDataRate(DataRate rate, bool write = true);

    /*
      Convenient method to read actual voltage (in volt) respecting the full scale range
      @param channel channel to read
    */
    float voltageRead(uint8_t channel);

    float getCurrentFullRangeVoltage();

    /*
      @return voltage resolution in volts
    */
    float getVoltageResolution();

    /*
      Read actual voltage (in volts)
      Changes fsr (full scale range) for future use of this function to get high read precision.
      If exceeded fsr it takes another read - you never should see max voltage
      @param channel channel to read
      @attention It changes fsr settings
    */
    float voltageReadAutoRange(uint8_t channel);

private:

    TwoWire *wire = NULL;
    uint8_t addr = 0x48;
    uint8_t convReg_ = 0x00;
    uint8_t confReg_ = 0x01;

    uint16_t initConf_ = 0x8583;
    uint16_t savedConf_ = 0x8583;
    uint16_t currentConf = savedConf_;
    OperatingMode currentMode_ = OP_SINGLE;
    FullScaleRange currentFSR_val_ = FSR_2_048V;
    MultiplexerConfig muxConfig;

    union I2C_data {
        uint8_t packet[2];
        uint16_t value;
    } data;

    uint8_t autoRangeFSR[4] = {FSR_2_048V, FSR_2_048V, FSR_2_048V, FSR_2_048V};

    /*
      A generic read from mem_addr.
    */
    uint16_t read(uint8_t mem_addr);

    void readCurrentConf();

    /*
      We only write to the configuration register.
      data is the 16 bits of conf_regs
      Should always return 2
      Saves data to current_conf
    */
    int write(uint16_t data);
};

