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
//    delay(5);
    this->wire->requestFrom(this->addr, (uint8_t)2);
    if (2 <= this->wire->available()) {
        // bring in data
        this->data.packet[1] = this->wire->read();
        this->data.packet[0] = this->wire->read();
        uint16_t ret = this->data.value;
        this->data.value = 0;
        return ret;
    }

    return 0;
}

int TLA202x::write(uint16_t out_data) {
    int written = 0;
    // save conf
    this->savedConf_ = out_data;
    // put our out_data into the I2C data union so we can send MSB and LSB
    data.value = out_data;
    this->wire->beginTransmission(this->addr);
    this->wire->write(this->confReg_);
    written += this->wire->write(this->data.packet[1]);
    written += this->wire->write(this->data.packet[0]);
    this->wire->endTransmission();
    this->data.value = 0;
    return written;
}

void TLA202x::reset() {
    this->write(this->initConf_);
}

void TLA202x::restore() {
    uint16_t restore_conf = this->savedConf_ & ~0x8000;
    this->write(restore_conf);
}

int16_t TLA202x::analogRead() {
    // this only needs to run when in single shot.
    if (this->currentMode_ == OP_SINGLE) {
        // write 1 to OS bit to start conv
        uint16_t current_conf = this->read(this->confReg_);
        current_conf |= 0x8000;
        this->write(current_conf);
        // OS bit will be 0 until conv is done.
        do {
            delay(5);
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

int16_t TLA202x::analogRead(uint8_t channel) {
    MultiplexerConfig muxCfg = (MultiplexerConfig)(channel - 4);
    this->setMuxConfig(muxCfg);
    return this->analogRead();
}

void TLA202x::setFullScaleRange(TLA202x::FullScaleRange range) {
    this->currentFSR_val_ = range;

    uint16_t conf = this->read(this->confReg_);

    // clear the PGA bits:
    conf &= ~0x0E00;

    // shift
    conf |= range << 9;
    this->write(conf);
}

void TLA202x::setMuxConfig(MultiplexerConfig option) {
    if (this->muxConfig == option) return;
    this->muxConfig = option;
    uint16_t conf = this->read(this->confReg_);
    // clear MUX bits
    conf &= ~0x7000;

    // shift
    conf |= this->muxConfig << 12;
    this->write(conf);
}

void TLA202x::setOperatingMode(OperatingMode mode) {
    this->currentMode_ = mode;
    uint16_t conf = this->read(this->confReg_);

    // clear MODE bit (8) (continous conv)
    conf &= ~(1 << 8);
    if (mode == OP_SINGLE) {
        // single shot
        conf |= (1 << 8);
    }
    this->write(conf);
}

void TLA202x::setDataRate(DataRate rate) {
    uint16_t conf = this->read(this->confReg_);

    // set bits 7:5
    //conf |= 0b111 << 5;
    conf |= rate << 5;
    write(conf);
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
    int16_t val = this->analogRead(channel);

    float fsrV = this->getCurrentFullRangeVoltage() - this->getVoltageResolution();

    //return (val - 0) * fsrV / (2047l - 0);
    return (float)val * fsrV / 2047.0f;

    //long converted = map((long)val, -2048, 2047, 0, fsrV * 100000);
    //long converted = map((long)val, 0l, 2047l, 0l, fsrV * 100000);
    // return (float)converted / 100000.0f;
}

float TLA202x::getVoltageResolution() {
    switch (this->currentFSR_val_) {
        case TLA202x::FullScaleRange::FSR_6_144V: return 0.003;
        case TLA202x::FullScaleRange::FSR_4_096V: return 0.002;
        case TLA202x::FullScaleRange::FSR_2_048V: return 0.001;
        case TLA202x::FullScaleRange::FSR_1_024V: return 0.0005;
        case TLA202x::FullScaleRange::FSR_0_512V: return 0.00025;
        case TLA202x::FullScaleRange::FSR_0_256V: return 0.000125;
    }
    return 0.0;
}





