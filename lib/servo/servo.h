#ifndef SERVO_H
#define SERVO_H

#include<Arduino.h>
#include<XYZrobotServo.h>

enum class Type : uint8_t { Angle, Speed };

class ServoBase{
public:
    ServoBase(Stream& s, uint8_t id) : servo(s, id){}

    virtual Type type() const = 0;
    virtual void update(int16_t val, uint32_t time = 0) = 0; 

    virtual ~ServoBase() = default;
protected:
    XYZrobotServo servo;
};

class ServoAngle : public ServoBase{
public:
    ServoAngle(Stream& s, uint8_t id, uint16_t base) : ServoBase(s, id), base(base){}

    Type type() const override { return Type::Angle; }
    void update(int16_t val, uint32_t time = 0) override { servo.setPosition(base + val, time / 10); }

private:
    uint16_t base;
};

class ServoSpeed : public ServoBase{
public:
    ServoSpeed(Stream& s, uint8_t id, uint16_t base) : ServoBase(s, id), base(base){}

    Type type() const override { return Type::Speed; }
    void update(int16_t val, uint32_t time = 0) override { servo.setSpeed(base + val, time / 10); }
private:
    uint16_t base;
};

#endif