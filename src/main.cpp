#include <sstream>
#include <Arduino.h>
#include <QTRSensors.h>
#include <PID_v1.h>
#include "hall_sensor.h"
#include "servo.h"

#define LED_BUILTIN 2

#define FL_ANGLE_SERVO_ID 1
#define FR_ANGLE_SERVO_ID 5
#define BL_ANGLE_SERVO_ID 3
#define BR_ANGLE_SERVO_ID 7
#define FL_SPEED_SERVO_ID 2
#define FR_SPEED_SERVO_ID 6
#define BL_SPEED_SERVO_ID 4
#define BR_SPEED_SERVO_ID 8

#define BASE_SPEED 600
#define MOTOR_ANGLE_RES 0.323 

enum State{DOCKED, RUNNING, WAITING};

const std::string ToString(State s){
  switch(s){
    case DOCKED: return "DOCKED";
    case RUNNING: return "RUNNING";
    case WAITING: return "WAITING";
    default: return "NONE";
  }
}

State current_state = DOCKED;

//inicjalizacja czujników
QTRSensors qtr_rf, qtr_rb, qtr_lb, qtr_lf;

const uint8_t SensorCount = 3;
uint16_t SensorValues[SensorCount];

const uint8_t qtr_pins_left[SensorCount] = {27,32,33};
const uint8_t qtr_pins_right[SensorCount] = {12,13,14};
const int qtr_ctrl_pin = 4;

HallSensor front_hall_sensor;
HallSensor back_hall_sensor;

const int front[8] = {-1, 1, 1, 1, -1, -1, 1, -1};
const int back[8] = {1, -1, -1, -1, 1, 1, -1, 1};

struct Side{
  std::string id;
  const int (&motor_direction)[8];
  QTRSensors* left_qtr;
  QTRSensors* right_qtr;
  HallSensor* hall;
  uint8_t qtr_ctrl;
};

Side front_side = {"front", front, &qtr_lf, &qtr_rf, &front_hall_sensor, LOW};
Side back_side = {"back", back, &qtr_lb, &qtr_rb, &back_hall_sensor, HIGH};
Side* current_side = &front_side;

//inicjalizacja silników
ServoAngle fl_angle_servo(Serial2, FL_ANGLE_SERVO_ID, 355);
ServoAngle fr_angle_servo(Serial2, FR_ANGLE_SERVO_ID, 385);
ServoAngle bl_angle_servo(Serial2, BL_ANGLE_SERVO_ID, 355);
ServoAngle br_angle_servo(Serial2, BR_ANGLE_SERVO_ID, 385);

ServoSpeed fl_speed_servo(Serial2, FL_SPEED_SERVO_ID, 0);
ServoSpeed fr_speed_servo(Serial2, FR_SPEED_SERVO_ID, 0);
ServoSpeed bl_speed_servo(Serial2, BL_SPEED_SERVO_ID, 0);
ServoSpeed br_speed_servo(Serial2, BR_SPEED_SERVO_ID, 0);

ServoBase* all_servo[8] = {
  &fl_angle_servo, &fl_speed_servo,
  &bl_angle_servo, &bl_speed_servo,
  &fr_angle_servo, &fr_speed_servo,
  &br_angle_servo, &br_speed_servo
};

const uint32_t motor_update_period = 50;
uint32_t last_motor_control = 0;

void update_servo(Type type, int16_t val){
  int i = 0;
  for(ServoBase* servo : all_servo){
    if(servo->type() == type){
      servo->update(val * current_side->motor_direction[i], 3 * motor_update_period);
    }
    i++;
  }
}

//inicjalicacja PID
int16_t control_output;
double pid_input, pid_output;
double pid_setpoint = 0;
double Kp = 0.0225, Ki = 0.002, Kd = 0.005;

PID mainPID(&pid_input, &pid_output, &pid_setpoint, Kp, Ki, Kd, DIRECT);


void setup() {
  Serial.begin(115200);   //serial do testów
  Serial2.begin(115200, SERIAL_8N1, 16, 17);    //serial do silników
  pinMode(LED_BUILTIN, OUTPUT);

  //setup czujników halla
  front_hall_sensor.set_pin(22);
  back_hall_sensor.set_pin(23);

  //setup i kalibracja czujników QTR
  pinMode(qtr_ctrl_pin, OUTPUT);

  qtr_rf.setTypeRC();
  qtr_rb.setTypeRC();
  qtr_lf.setTypeRC();
  qtr_lb.setTypeRC();

  qtr_rf.setSensorPins(qtr_pins_right, SensorCount);
  qtr_rb.setSensorPins(qtr_pins_right, SensorCount);
  qtr_lf.setSensorPins(qtr_pins_left, SensorCount);
  qtr_lb.setSensorPins(qtr_pins_left, SensorCount);

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("calibrating front qtr");
  digitalWrite(qtr_ctrl_pin, LOW);
  for(int i = 0; i < 200; i++){
    qtr_lf.calibrate();
    qtr_rf.calibrate();
  }
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(qtr_ctrl_pin, HIGH);
  Serial.println("calibrating back qtr");
  for(int i = 0; i < 200; i++){
    qtr_lb.calibrate();
    qtr_rb.calibrate();
  }
  digitalWrite(LED_BUILTIN, LOW);

  //setup PID
  mainPID.SetMode(AUTOMATIC);
  mainPID.SetOutputLimits(-45, 45);
}

std::ostringstream serial_message;

void loop() {
uint32_t now = millis();
front_hall_sensor.update();
back_hall_sensor.update();


switch(current_state){
case DOCKED:
  update_servo(Type::Speed, 0);
  if(front_hall_sensor.detect_pulse()){
    current_side = &back_side;
    current_state = RUNNING;
  }
  if(back_hall_sensor.detect_pulse()){
    current_side = &front_side;
    current_state = RUNNING;
  }
  break;

case RUNNING:
  if(current_side->hall->get_magnet_active()){
    current_state = WAITING;
    break;
  }
  digitalWrite(qtr_ctrl_pin, current_side->qtr_ctrl);

  pid_input = current_side->right_qtr->readLineBlack(SensorValues) - current_side->left_qtr->readLineBlack(SensorValues);
  mainPID.Compute();

  if(now - last_motor_control >= motor_update_period){
    control_output = (int16_t)lroundf(pid_output / MOTOR_ANGLE_RES);
    update_servo(Type::Angle, control_output);
    update_servo(Type::Speed, BASE_SPEED);
    last_motor_control = now;
  }
  break;

case WAITING:
  update_servo(Type::Speed, 0);
  if(current_side->hall->detect_pulse()){
    current_state = DOCKED;
  }
  break;
}

// serial_message << 
//   ToString(current_state) << " : " <<
//   current_side->id << " : " <<
//   current_side->hall->get_magnet_active() << " : " <<
//   pid_input << " : " <<
//   pid_output << " : " <<
//   control_output << " : " <<
//   current_side->right_qtr->readLineBlack(SensorValues) << " : " <<
//   current_side->left_qtr->readLineBlack(SensorValues);

// Serial.println(serial_message.str().c_str());
// serial_message.str("");
// serial_message.clear();
  
}