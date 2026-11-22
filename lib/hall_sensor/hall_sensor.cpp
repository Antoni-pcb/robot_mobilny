#include "hall_sensor.h"

void HallSensor::set_pin(int pin){
    sensor_pin_ = pin;
    pinMode(pin, INPUT_PULLUP);
}

void HallSensor::update(){
    if(millis() - last_update_ >= treshold_){
        prev_state_ = magnet_active_;
        magnet_active_ = !digitalRead(sensor_pin_);
        last_update_ = millis();
    }
    
}

bool HallSensor::detect_pulse(){
    this->update();
    if(prev_state_ == true and !magnet_active_){
        return true;
    }
    return false;
}