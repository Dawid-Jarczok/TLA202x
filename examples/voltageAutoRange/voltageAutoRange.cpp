#include <Arduino.h>
#include <TLA202x.h>
#include <Wire.h>

TLA202x adc = TLA202x(&Wire);

void setup() {
    Serial.begin(115200);
    Serial.println("Starting ADC...");

    Wire.begin();

    if (adc.begin()) {
        Serial.println("Device is init-ed");
    }
    else {
        Serial.println("Device is not init-ed. Continue anyway...");
    }

    adc.setFullScaleRange(TLA202x::FSR_2_048V);
    adc.setDataRate(TLA202x::DR_3300SPS);
    adc.setOperatingMode(TLA202x::OP_SINGLE);
}

void loop() {
    Serial.printf("%.5fV\t%.5fV\n", adc.voltageRead(0), adc.voltageRead(3));

    delay(50);
}