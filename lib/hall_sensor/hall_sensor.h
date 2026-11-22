#ifndef HALL_SENSOR_H
#define HALL_SENSOR_H

#include<Arduino.h>

class HallSensor {
private:
    int sensor_pin_;
    bool prev_state_;
    bool magnet_active_;
    uint32_t last_update_;
    uint32_t treshold_;
public:
    HallSensor(): sensor_pin_{}, prev_state_{}, magnet_active_{}, treshold_{} { last_update_ = micros(); }
    void set_treshold(uint32_t treshold){ treshold_ = treshold; }
    void set_pin(int pin);
    bool get_magnet_active(){ return magnet_active_; }
    void update();
    bool detect_pulse();
};

#endif