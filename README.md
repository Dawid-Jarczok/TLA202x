# TLA202x
Library for analog-digital converter with i2c comunication.\
I modified existing [version](https://github.com/andriyadi/ESP32-TLA2024) because wanted some additional funcitons and maybe it will be usefull for you ;)

![](https://img.shields.io/badge/License-MIT-blue.svg)
___
For now only used with [TLA2024](https://www.ti.com/product/TLA2024), some function don't work with [TLA2022](https://www.ti.com/product/TLA2022) or [TLA2021](https://www.ti.com/product/TLA2021).\
Only TLA2024 have MUX.\
TLA2021 don't have PGA.\
Tested on ESP32, but should work on any arduino framework chips.

## Functions
- voltageReadAutoRange is not tested in 100% so have that in mind\
- In continous mode remember that if we use analogRead functions with parameters (channel, mux, full scale range) even if we writes new settings before reading from adc we propably receive value readed with old settings

## Credit
The library is modyfied version made by [this guy, thanks](https://github.com/andriyadi/ESP32-TLA2024).
