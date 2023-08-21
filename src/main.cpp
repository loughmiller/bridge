#include <Arduino.h>
#include <WIFI.h>
#include <painlessMesh.h>
#include <UMS3.h>
#include <RH_ASK.h>



////////////////////////////////////////////////////////////////////////////////
// RF
////////////////////////////////////////////////////////////////////////////////
const uint_fast8_t receive_pin = 1;
const byte authByteStart = 117;
const byte authByteEnd = 115;

RH_ASK driver(2000, receive_pin);

////////////////////////////////////////////////////////////////////////////////
// Protocol
////////////////////////////////////////////////////////////////////////////////
byte messageType = 0;
uint32_t messageData = 0;
uint32_t timeSync = 0;

////////////////////////////////////////////////////////////////////////////////
// Setup
////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("setup");

  // Initialise radiohead
  if (!driver.init()) {
    Serial.println("init failed");
  } else {
    Serial.println("init success");}
}

uint_fast8_t lastMessageID = 255;

void loop() {
  uint8_t buf[12];
  uint8_t buflen = sizeof(buf);

  if (driver.recv(buf, &buflen)) {
    if ((buf[0] != authByteStart) || (buf[buflen - 1] != authByteEnd)) {
      Serial.println("bad message");
      return;
    }

    uint_fast8_t messageID = buf[1];

    if (messageID == lastMessageID) {
      return;
    }

    lastMessageID = messageID;

    // // MESSAGE DEBUG LOGGING
    // Serial.print("Got: ");
    // for (uint_fast8_t i = 0; i < buflen; i++) {
    //   Serial.print(buf[i]);
    //   Serial.print(' ');
    // }
    // Serial.println();
    // // \\ MESSAGE DEBUG LOGGING

    messageType = buf[2];

    if (messageType == 10 && buflen == 8) {
      // sync
      messageData = buf[3] << 24 | buf[4] << 16 | buf[5] << 8 | buf[6];
      // Serial.print("sync: ");
      // Serial.println(timeSync);

    } else if (messageType > 0) {
      messageData = buf[3];
    }

    StaticJsonDocument<32> doc;
    doc["type"] = messageType;
    doc["msg"] = messageData;

    serializeJson(doc, Serial);
    Serial.println();

  }
}
