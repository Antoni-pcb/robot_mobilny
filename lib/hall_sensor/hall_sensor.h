#ifndef HALL_SENSOR_H
#define HALL_SENSOR_H

#include<Arduino.h>

class HallSensor {
private:
    int sensor_pin;
    bool prev_state;
    bool magnet_active;
public:
    HallSensor() = default;
    void set_pin(int pin);
    bool get_magnet_active(){ return magnet_active; }
    void update();
    bool detect_pulse();
};

#endif