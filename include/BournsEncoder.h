//
// Created by bohm on 11/8/21.
//

#ifndef PENDULUM_R_ESP32_BOURNSENCODER_H
#define PENDULUM_R_ESP32_BOURNSENCODER_H

#include "circular_buffer.h"

namespace BournsEncoder {
  int PIN_CS = 32;
  int PIN_CLOCK = 15;
  int PIN_DATA = 33;

  volatile int position = 0;

  int lastPosition = 0;
  int delta = 0;
  unsigned long lastUpdated, now = millis();

  static constexpr int OMEGAS_COUNT = 4;
  Circular_Buffer<double, OMEGAS_COUNT> omegas;
  double acceleration = 0;

  byte stream[16] = {0};//replace testStream with stream for actual data


  void init(int cs_pin, int clock_pin, int data_pin) {
    PIN_CS = cs_pin;
    PIN_CLOCK = clock_pin;
    PIN_DATA = data_pin;

    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_CLOCK, OUTPUT);
    pinMode(PIN_DATA, INPUT);

    digitalWrite(PIN_CLOCK, HIGH);
    digitalWrite(PIN_CS, LOW);
  }

  void read() {
    digitalWrite(PIN_CS, HIGH);
    delayMicroseconds(500);
    digitalWrite(PIN_CS, LOW);

    stream[16] = {0};
    for (int i = 0; i < 16; i++) {
      digitalWrite(PIN_CLOCK, LOW);
      delayMicroseconds(500);
      digitalWrite(PIN_CLOCK, HIGH);
      delayMicroseconds(500);

      stream[i] = digitalRead(PIN_DATA);
    }

    digitalWrite(PIN_CLOCK, LOW);
    digitalWrite(PIN_CLOCK, HIGH);

    //extract 10 bit position from data stream use testStream
    position = 0; //clear previous data
    for (int i = 0; i < 10; i++) {
      position = position << 1;
      position += stream[i];
    }

    delta = position - lastPosition;
    if (abs(delta) > 512) {
      delta = 1024 - abs(delta);
      if (position > lastPosition) {
        delta *= -1;
      }
    }
    lastPosition = position;
    now = millis();
    double timeDelta = (double)(now - lastUpdated);
    lastUpdated = now;

    omegas.push_back(delta / timeDelta / 1.024); // rotations per sec
    acceleration = (omegas[OMEGAS_COUNT - 1] - omegas[OMEGAS_COUNT - 2]) / timeDelta;
    /*
    Serial.print("Delta ENc: ");
    Serial.print(delta);
    Serial.print(" Delta T: ");
    Serial.print((double) (now - lastUpdated));
    Serial.print(" Omega size: ");
    Serial.print(omegas.size());
    Serial.print(" Omega: ");
    Serial.println(delta / (double) (now - lastUpdated) / 1.024);
    */

  }
}

#endif //PENDULUM_R_ESP32_BOURNSENCODER_H
