#include <Arduino.h>
#include "ServoShield.h"
#include "BournsEncoder.h"
#include <Wire.h>

#include <WiFi.h>
#include <MQTT.h>

const char ssid[] = "smc3";
const char pass[] = "nejaketazkeheslo";
//const char mqt_ip[] = "192.168.15.127";
const char mqt_ip[] = "192.168.86.202";

WiFiClient net;
MQTTClient client;

String showText;
int position_update = 0;
unsigned long lastMillis = 0;
int lastPosition = 0;

unsigned long dataSentTS = millis();
int encoder_offset = 31; // with the ABS coupler (with the line)

bool sendData = false;
bool autoSendData = false;

int streaming_delay = 25;

void connect();

void setup() {
  Wire.begin();
  //Serial.begin(57600);
  Serial2.begin(57600);

  ServoShield::init();
  BournsEncoder::init(10, 11, 12);
}

void getBTData() {
  if (Serial2.available()) {
    String cmd = Serial2.readStringUntil('\n');
    char dir = cmd.charAt(0);

    switch(dir) {
      case 'a':
        position_update = -ServoShield::FAST;
        break;
      case 's':
        position_update = -ServoShield::SLOW;
        break;
      case 'd':
        position_update = ServoShield::STOP;
        break;
      case 'f':
        position_update = ServoShield::SLOW;
        break;
      case 'g':
        position_update = ServoShield::FAST;
        break;
      case 'm':
        position_update = cmd.substring(1).toInt();
        break;
      case 'o':
        encoder_offset = atoi(cmd.substring(1).c_str());
        break;
      case 'u':
        streaming_delay = atoi(cmd.substring(1).c_str());
        break;
      case 'z':
        sendData = true;
        break;
      case 'x':
        autoSendData = true;
        break;
      case 'y':
        autoSendData = false;
        break;

//      case 'p':
//        showText = cmd.substring(1);
//        Lcd::print_short(showText);
//        break;
      default: break; // keep the current course
    }
    Serial2.flush();
  }
}

void loop() {
  unsigned long newMillis = millis();

  // because the bluetooth module barfs on faster speeds
  if (Serial2.availableForWrite() && (sendData || (autoSendData && (newMillis - dataSentTS > streaming_delay)))) {
    BournsEncoder::read();

    int time_delta = (int) (newMillis - lastMillis);
    lastMillis = newMillis;

    int position = (BournsEncoder::position + encoder_offset) % 1024;
    float velocity = (position - lastPosition) / time_delta;
    lastPosition = position;

    char state[100] = ""; // without the initialization it contains new lines and unprintable chars
    sprintf(state, "S%ld,%d,%d,%f,%f,%f\n",
            newMillis,
            position,
            ServoShield::pulselen,
            BournsEncoder::omegas.average(),
            BournsEncoder::acceleration,
            velocity
    );

    Serial2.print(state);
    dataSentTS = newMillis;

    if (sendData) {
      sendData = false;
    }
  }

  getBTData();
//  Serial.print("Received position update: ");
//  Serial.println(position_update);

//  Lcd::printTelemetry(distance, speed, EncoderE6B2::counter, VoltMeter::voltage);

  delay(10);

  // throttle when omega is over 2 rotations per second
  if (fabs(BournsEncoder::omegas.average()) > 2) {
    position_update = ServoShield::STOP;
  }
  ServoShield::move(position_update);
}

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("arduino", "public", "public")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe("hello");
  // client.unsubscribe("hello");
}

void messageReceived(String &topic, String &payload) {
  Serial.print(millis());
  Serial.println(" incoming: " + topic + " - " + payload);

  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}
