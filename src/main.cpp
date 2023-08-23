#include <Arduino.h>
#include <WIFI.h>
#include <painlessMesh.h>
#include <UMS3.h>
#include <RH_ASK.h>


////////////////////////////////////////////////////////////////////////////////
// WIFI
////////////////////////////////////////////////////////////////////////////////
#define   MESH_PREFIX     "wizardMesh"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

void newConnectionCallback(uint32_t nodeId);
bool connected = false;

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

// message types
const byte typeCycle = 1;
const byte typeBrightness = 2;
const byte typeDensity = 3;
const byte typeSparkles = 4;
const byte typeHue = 5;
const byte typeStreaks = 7;
const byte typeSolid = 8;
const byte typeSteal = 9;
const byte typeSync = 10;
const byte typeHue2 = 11;
const byte typeCycle2 = 12;

StaticJsonDocument<128> config;

////////////////////////////////////////////////////////////////////////////////
// Setup
////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("setup");

// WIFI SETUP
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onNewConnection(&newConnectionCallback);
  Serial.println("wifi setup complete");

  // Initialise radiohead
  if (!driver.init()) {
    Serial.println("init failed");
  } else {
    Serial.println("init success");
  }

  Serial.println("setup complete");
  Serial.println("setup complete");
  Serial.println("setup complete");
}

uint_fast8_t lastMessageID = 255;
uint_fast32_t loggingTimestamp = 0;

void loop() {
  mesh.update();
  uint8_t buf[12];
  uint8_t buflen = sizeof(buf);

  unsigned long localTime = millis();

  if (localTime > loggingTimestamp + 2000) {
    // Serial.println("logging");
    loggingTimestamp = localTime;

    if (mesh.getNodeList().size() == 0) {
      if (connected) {
        Serial.println("disconnected");
      }
      connected = false;
    } else {
      if (!connected) {
        Serial.println("connected");
      }
      connected = true;
    }
  }

  if (driver.recv(buf, &buflen)) {
    if ((buf[0] != authByteStart) || (buf[buflen - 1] != authByteEnd)) {
      Serial.println("bad message");
      Serial.print("Got: ");
      for (uint_fast8_t i = 0; i < buflen; i++) {
        Serial.print(buf[i]);
        Serial.print(' ');
      }
      Serial.println();
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

    String json;

    if (messageType == typeSync && buflen == 8) {
      // sync
      messageData = buf[3] << 24 | buf[4] << 16 | buf[5] << 8 | buf[6];
      // Serial.print("sync: ");
      // Serial.println(timeSync);

      // immediately send sync message
      StaticJsonDocument<32> sync;
      sync["sync"] = messageData;
      serializeJson(sync, json);
      mesh.sendBroadcast(json);
      Serial.println(json);
      return;
    }

    messageData = buf[3];

    // log message type and data
    Serial.print("messageType: ");
    Serial.println(messageType);
    Serial.print("messageData: ");
    Serial.println(messageData);

    if (messageType == typeSteal) {
      // immediately send steal message
      StaticJsonDocument<64> steal;
      steal["steal"] = messageData;
      steal["hue"] = messageData;
      steal["cycle"] = 0;
      serializeJson(steal, json);
      mesh.sendBroadcast(json);
      Serial.println(json);
      return;
    }

    // generate json config from message
    switch (messageType)
    {
      case typeCycle:
        config["cycle"] = messageData;
        break;
      case typeBrightness:
        config["brightness"] = messageData;
        break;
      case typeDensity:
        config["density"] = messageData;
        break;
      case typeSparkles:
        config["sparkles"] = messageData;
        break;
      case typeHue:
        config["hue"] = messageData;
        config["cycle"] = 0;
        break;
      case typeStreaks:
        config["streaks"] = messageData;
        break;
      case typeSolid:
        config["solid"] = messageData;
        break;
      case typeHue2:
        config["hue"] = messageData;
        break;
      case typeCycle2:
        if (config["cycle"] > 0) {
          config["cycle"] = messageData;
        }
        break;

      default:
        break;
    }

    serializeJson(config, json);
    mesh.sendBroadcast(json);
    Serial.println(json);
  }
}

////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////
void newConnectionCallback(uint32_t nodeId) {
}

