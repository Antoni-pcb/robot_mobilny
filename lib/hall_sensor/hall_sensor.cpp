#include "hall_sensor.h"

void HallSensor::set_pin(int pin){
    sensor_pin = pin;
    pinMode(pin, INPUT);
}

void HallSensor::update(){
    prev_state = magnet_active;
    magnet_active = !digitalRead(sensor_pin);
}

bool HallSensor::detect_pulse(){
    if(prev_state == true and !magnet_active){
        return true;
    }
    return false;
}