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

  // HiTec
//  static constexpr int  SLOW = 2;
//  static constexpr int  FAST = 4;


  static constexpr int  STOP = 0;

// our servo # counter
  uint8_t servonum = 4;
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

}

#endif //PENDULUM_R_ESP32_SERVOSHIELD_H
