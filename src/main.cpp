#include <Arduino.h>
#include "ServoShield.h"
#include "BournsEncoder.h"
#include <Wire.h>

#include <WiFi.h>
#include <MQTT.h>

const char ssid[] = "smc6";
const char pass[] = "nejaketazkeheslo";
const char mqt_ip[] = "192.168.86.202";

WiFiClient net;
MQTTClient client;

String showText;
int position_update = 0;
unsigned long lastMillis = 0;
int lastPosition = 0;

unsigned long dataSentTS, lastActionTS = millis();
int encoder_offset = 31; // with the ABS coupler (with the line)

bool sendData = false;
bool autoSendData = false;
bool continuousActionSpace = false;

constexpr float throttleSpeed = 2.0;

int streaming_delay = 25;
int acting_delay = 10;

constexpr int setupNo = 1;

String observationsTopic = String("pendulum-r/obs/r0") + setupNo;
String actionsTopic = String("pendulum-r/act/r0") + setupNo;

void connect();
void getBTData(String &topic, String &payload);

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Starting...");

  Wire.begin(1, 2); // ESP32S2
  Wire.setClock(400000);

  WiFi.begin(ssid, pass);

  ServoShield::init();
//  BournsEncoder::init(32, 15, 33); // ESP32
//   BournsEncoder::init(8, 4, 6); //ESP32S2 - P2
  BournsEncoder::init(6, 8, 10); //ESP32S2 - P1

  // Note: Local domain names (e.g. "Computer.local" on OSX) are not supported
  // by Arduino. You need to set the IP address directly.
  client.begin(mqt_ip, net);
  client.onMessage(getBTData);

  connect();

  Serial.print("Listening on ");
  Serial.println(actionsTopic);
  Serial.print("Observations on ");
  Serial.println(observationsTopic);
}

void loop() {
  client.loop();
  // delay(1);  // <- fixes some issues with WiFi stability

  unsigned long newMillis = millis();

  // this block takes about 5ms
  if ((sendData || (autoSendData && (newMillis - dataSentTS > streaming_delay)))) {
    // this call takes 3ms
    BournsEncoder::read();
    // Serial.print("Bourns reading took: ");
    // Serial.println(millis() - newMillis);

    int time_delta = (int) (newMillis - lastMillis);
    lastMillis = newMillis;

    int position = (BournsEncoder::position + encoder_offset) % 1024;
    float velocity = (position - lastPosition) / time_delta;
    lastPosition = position;

    char state[100] = ""; // without the initialization it contains new lines and unprintable chars
    sprintf(state, "S%ld,%d,%d,%f,%f,%f",
            newMillis,
            position,
            ServoShield::pulselen,
            BournsEncoder::omegas.average(),
            BournsEncoder::acceleration,
            velocity
    );

    Serial.println(state);
    client.publish(observationsTopic, state);
    dataSentTS = newMillis;

    if (sendData) {
      sendData = false;
    }

     // Serial.print("Sending data took: ");
     // Serial.println(millis() - newMillis);
  }

  newMillis = millis();
  // throttle when omega is over 2 rotations per second
  if (!continuousActionSpace && newMillis - lastActionTS > acting_delay) {
    if (fabs(BournsEncoder::omegas.average()) > throttleSpeed) {
      position_update = ServoShield::STOP;
    }
    ServoShield::move(position_update);
    lastActionTS = newMillis;
  }
}

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  String clientId = String("pendulum_r_prod_s") + setupNo;

  Serial.print("\nconnecting...");
  while (!client.connect(clientId.c_str(), "public", "public")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe(actionsTopic);
}


void getBTData(String &topic, String &payload) {
  Serial.print("Received command: ");
  Serial.println(payload);

  char dir = payload.charAt(0);

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
      position_update = payload.substring(1).toInt();
      break;
    case 'o':
      encoder_offset = atoi(payload.substring(1).c_str());
      break;
    case 'u':
      streaming_delay = atoi(payload.substring(1).c_str());
      break;
    case 'i':
      acting_delay = atoi(payload.substring(1).c_str());
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
    case 'c':
      continuousActionSpace = true;
      break;
    case 'v':
      continuousActionSpace = false;
      break;
    case 'e':
      ServoShield::EXP_FILTER_C = atof(payload.substring(1).c_str());
      break;
    case 'b':
      if (fabs(BournsEncoder::omegas.average()) <= throttleSpeed) {
        ServoShield::set_action(atof(payload.substring(1).c_str()));
      }
      break;

//      case 'p':
//        showText = cmd.substring(1);
//        Lcd::print_short(showText);
//        break;
    default: break; // keep the current course
  }
}
