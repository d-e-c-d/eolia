#pragma once
class Adafruit_INA219{public: Adafruit_INA219(uint8_t){} bool begin(){return false;}
 float getBusVoltage_V(){return 0;} float getShuntVoltage_mV(){return 0;} float getCurrent_mA(){return 0;}};
