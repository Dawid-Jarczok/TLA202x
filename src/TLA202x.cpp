#include "TLA202x.h"


TLA202x::TLA202x(TwoWire *dev): wire(dev) {
}

bool TLA202x::begin(uint8_t address) {
    this->addr = address;
    reset();
    delay(10);

    uint16_t init = read(this->confReg_);
    Serial.printf("Done init. 0x%X\n", init);

    // make sure communication with device is working and that it is OK
    return (init == this->initConf_) ? true : false;
}

uint16_t TLA202x::read(uint8_t mem_addr) {
    this->wire->beginTransmission(this->addr);
    this->wire->write(mem_addr);
    this->wire->endTransmission(false);
    //delay(5);
    this->wire->requestFrom(this->addr, (uint8_t)2);
    if (2 <= this->wire->available()) {
        uint16_t ret = (uint16_t)this->wire->read() << 8;
        ret += this->wire->read();
        return ret;
    }

    return 0;
}

void TLA202x::readCurrentConf() {
    this->currentConf = this->read(this->confReg_);
}

int TLA202x::write(uint16_t data) {
    int written = 0;
    this->wire->beginTransmission(this->addr);
    this->wire->write(this->confReg_);
    written += this->wire->write((uint8_t)(data >> 8));
    written += this->wire->write((uint8_t)(data & 0xFF));
    this->wire->endTransmission();
    return written;
}

void TLA202x::reset() {
    this->write(this->initConf_);
}

void TLA202x::restore() {
    uint16_t restore_conf = this->currentConf & ~0x8000;
    this->write(restore_conf);
}

int16_t TLA202x::analogRead() {
    // this only needs to run when in single shot.
    if (this->currentMode_ == OP_SINGLE) {
        // write 1 to OS bit to start conv
        this->currentConf |= 0x8000;
        this->write(this->currentConf);
        // OS bit will be 0 until conv is done.
        do {
            delay(1);
        } while ((this->read(this->confReg_) & 0x8000) == 0);
    }

    // get data from conv_reg
    uint16_t in_data = this->read(this->convReg_);

    // shift out unused bits
    in_data >>= 4;

    // get sign and mask accordingly
    if (in_data & (1 << 11)) {
        // 11th bit is sign bit. if its set, set bits 15-12
        in_data |= 0xF000;
    } else {
        // not set, clear bits 15-12
        in_data &= ~0xF000;
    }

    // now store it as a signed 16 bit int.
    int16_t ret = in_data;

    // default Full Scale Range is -2.048V to 2.047V.
    // our 12bit 2's complement goes from -2048 to 2047
    return ret;
}

int16_t TLA202x::analogRead(FullScaleRange range) {
    //Writes new settings immediately only if in continous mode.
    //In single mode analogRead writes config.
    this->setFullScaleRange(range, this->currentMode_ == OP_CONTINUOUS);
    return this->analogRead();
}

int16_t TLA202x::analogRead(uint8_t channel) {
    //Writes new settings immediately only if in continous mode.
    //In single mode analogRead writes config.
    MultiplexerConfig muxCfg = (MultiplexerConfig)(channel - 4);
    this->setMuxConfig(muxCfg, this->currentMode_ == OP_CONTINUOUS);
    return this->analogRead();
}

int16_t TLA202x::analogRead(uint8_t channel, FullScaleRange range) {
    //Writes new settings immediately only if in continous mode.
    //In single mode analogRead writes config.
    this->setFullScaleRange(range, false);
    MultiplexerConfig muxCfg = (MultiplexerConfig)(channel - 4);
    this->setMuxConfig(muxCfg, this->currentMode_ == OP_CONTINUOUS);
    return this->analogRead();
}

void TLA202x::setFullScaleRange(TLA202x::FullScaleRange range, bool write) {
    if (this->currentFSR_val_ == range) return;
    this->currentFSR_val_ = range;
    // clear the PGA bits:
    this->currentConf &= ~0x0E00;
    this->currentConf |= range << 9;
    if (write) this->write(this->currentConf);
}

void TLA202x::setMuxConfig(MultiplexerConfig option, bool write) {
    if (this->muxConfig == option) return;
    this->muxConfig = option;
    // clear MUX bits
    this->currentConf &= ~0x7000;
    this->currentConf |= this->muxConfig << 12;
    if (write) this->write(this->currentConf);
}

void TLA202x::setOperatingMode(OperatingMode mode, bool write) {
    this->currentMode_ = mode;

    // clear MODE bit (8) (continous conv)
    this->currentConf &= ~(1 << 8);
    if (mode == OP_SINGLE) this->currentConf |= (1 << 8);
    if (write) this->write(this->currentConf);
}

void TLA202x::setDataRate(DataRate rate, bool write) {
    this->currentConf |= rate << 5;
    if (write) this->write(this->currentConf);
}

float TLA202x::getCurrentFullRangeVoltage() {
    uint16_t shifted = 8192 >> this->currentFSR_val_;

    // Special case
    if (this->currentFSR_val_ == 0) {
        shifted = 6144;
    }

    return (float)shifted / 1000.0f;
}

float TLA202x::voltageRead(uint8_t channel) {
    if (channel > 3) return 0.0f;
    int16_t val = this->analogRead(channel);

    float fsrV = this->getCurrentFullRangeVoltage() - this->getVoltageResolution();

    //return (val - 0) * fsrV / (2047l - 0);
    return (float)val * fsrV / 2047.0f; //map

    //long converted = map((long)val, -2048, 2047, 0, fsrV * 100000);
    //long converted = map((long)val, 0l, 2047l, 0l, fsrV * 100000);
    // return (float)converted / 100000.0f;
}

float TLA202x::getVoltageResolution() {
    switch (this->currentFSR_val_) {
        case TLA202x::FullScaleRange::FSR_6_144V: return 0.003f;
        case TLA202x::FullScaleRange::FSR_4_096V: return 0.002f;
        case TLA202x::FullScaleRange::FSR_2_048V: return 0.001f;
        case TLA202x::FullScaleRange::FSR_1_024V: return 0.0005f;
        case TLA202x::FullScaleRange::FSR_0_512V: return 0.00025f;
        case TLA202x::FullScaleRange::FSR_0_256V: return 0.000125f;
    }
    return 0.0f;
}

float TLA202x::voltageReadAutoRange(uint8_t channel) {
    if (channel > 3) return 0.0f;
    int16_t val = 0;
    do {
        val = this->analogRead(channel, (FullScaleRange)this->autoRangeFSR[channel]);

        // Below calculation dont match going in or from fsr = 6.144V ;)
        // 1023 * 0.9 = 921, in lower fsr value should be less than 1842
        if (val <= 921 && this->autoRangeFSR[channel] < TLA202x::FullScaleRange::FSR_0_256V){
             this->autoRangeFSR[channel] += 1;
        }
        // 2047 * 0.9 = 1842, in higher fsr value should be more than 921
        else if (val >= 1842 && this->autoRangeFSR[channel] > TLA202x::FullScaleRange::FSR_6_144V) {
            this->autoRangeFSR[channel] -= 1;
        }
    // Repeat analogRead if we exceeded scale
    } while (val == 2047 && this->autoRangeFSR[channel] != TLA202x::FullScaleRange::FSR_6_144V);
    float fsrV = this->getCurrentFullRangeVoltage() - this->getVoltageResolution();
    return (float)val * fsrV / 2047.0f; //map
}





