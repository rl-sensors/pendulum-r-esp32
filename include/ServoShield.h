//
// Created by bohm on 11/8/21.
//

#ifndef PENDULUM_R_ESP32_SERVOSHIELD_H
#define PENDULUM_R_ESP32_SERVOSHIELD_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

namespace ServoShield {
  Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

  #define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

  // for turnigy 200 and 400
  #define SERVOMIN  200 // This is the 'minimum' pulse length count (out of 4096)
  #define SERVOMAX  400 // This is the 'maximum' pulse length count (out of 4096)

  // for HiTec 9380TH
//  #define SERVOMIN  150 // This is the 'minimum' pulse length count (out of 4096)
//  #define SERVOMAX  450 // This is the 'maximum' pulse length count (out of 4096)
//  #define SERVO_FREQ 50 // Analog servos run at ~50 Hz updates

  // turningy
  static constexpr int  SLOW = 1;
  static constexpr int  FAST = 2;

  float EXP_FILTER_C = 0.9;

    // HiTec
//  static constexpr int  SLOW = 2;
//  static constexpr int  FAST = 4;


  static constexpr int  STOP = 0;

  constexpr uint8_t servonum = 4;

  volatile int pulselen = (SERVOMAX - SERVOMIN) / 2 + SERVOMIN;

  void init() {
    pwm.begin();

    pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates

    delay(10);

    pwm.setPWM(servonum, 0, pulselen);
  }

  void move(int &position_update) {
    if (pulselen + position_update <= SERVOMAX && pulselen + position_update >= SERVOMIN)  {
      pulselen += position_update;
      //position_update = 0;
    } else {
      //position_update = 0;
    }

    pwm.setPWM(servonum, 0, pulselen);
  }

    void set_position_with_filter(int const &new_position_cmd) {
      int old_position = pulselen;
      int new_position = old_position * EXP_FILTER_C + new_position_cmd * (1.0 - EXP_FILTER_C);
      if (new_position <= SERVOMIN) {
        pulselen = SERVOMIN;
      } else if (new_position >= SERVOMAX) {
        pulselen = SERVOMAX;
      } else {
        pulselen = new_position;
      }

      pwm.setPWM(servonum, 0, pulselen);
    }

    void set_action(float const &new_position_cmd) {
      int position = map(
              new_position_cmd * 1000, -1000.0, 1000.0,
              SERVOMIN, SERVOMAX
      );
      set_position_with_filter(position);

//    char msg[100] = "";
//    sprintf(msg, "Servo %i: action %f translated to position %i", servonum, new_position_cmd, position);
//    Mqtt::client.publish("quaid01/actions", msg);
    }


}

#endif //PENDULUM_R_ESP32_SERVOSHIELD_H
