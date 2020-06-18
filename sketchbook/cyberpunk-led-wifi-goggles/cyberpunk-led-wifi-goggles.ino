
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Hash.h>
#include <FastLED.h>

// Data-out PIN on the Arduino
#define DATA_PIN 14
// Default brightness
#define BRIGHTNESS 150
// Amount of LEDs
#define NUM_LEDS 32
// Time to wait in ms for the next animation update
unsigned long interval = 40;
unsigned long previousMillis = 0;
unsigned long toBlack = 60;
CRGB leds[NUM_LEDS];
uint8_t activeLed = 0;
uint8_t halfNumLeds = NUM_LEDS / 2;


const char* ssid = "";
const char* password = "";
const char* server = "";
const int   port     = 3006;
const char* path     = "/cyberpunk";

ESP8266WiFiMulti WiFiMulti;
// Per Message deflate -> Off? https://github.com/websockets/ws#opt-in-for-performance-and-spec-compliance
// Async TCP? -> https://github.com/Links2004/arduinoWebSockets#esp-async-tcp
WebSocketsClient webSocket;

// See https://arduinojson.org/v6/assistant/
const size_t capacity = JSON_ARRAY_SIZE(3);
DynamicJsonDocument doc(capacity);
DeserializationError deserializeError;

#define USE_SERIAL Serial



void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {

  switch(type) {
    case WStype_DISCONNECTED:
      USE_SERIAL.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED: {
      USE_SERIAL.printf("[WSc] Connected to url: %s\n", payload);

      // send message to server when Connected
      // BUG: This results in a 1006 and disconnect
      //webSocket.sendTXT("{\"message\":\"hello\"}");
    }
      break;
      
    case WStype_TEXT: {
      parseMessage(payload);
    }
      break;
      
    case WStype_BIN:
      USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);
      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
      
    case WStype_PING:
      // pong will be send automatically
      USE_SERIAL.printf("[WSc] get ping\n");
      break;
      
    case WStype_PONG:
      // answer to a ping we send
      USE_SERIAL.printf("[WSc] get pong\n");
      break;
    }

}

void setup() {
  USE_SERIAL.begin(115200); 

  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for(uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  WiFiMulti.addAP(ssid, password);

  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  // server address, port and URL
  webSocket.begin(server, port, path);

  // event handler
  webSocket.onEvent(webSocketEvent);

  // use HTTP Basic Authorization this is optional remove if not needed
  // webSocket.setAuthorization("user", "Password");

  // try ever 5000 again if connection has failed
  webSocket.setReconnectInterval(5000);

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  // start heartbeat (optional)
  // ping server every 15000 ms
  // expect pong from server within 3000 ms
  // consider connection disconnected if pong is not received 2 times
  // webSocket.enableHeartbeat(15000, 3000, 2);
}

void parseMessage(uint8_t * payload) {
  char* json = (char*) payload;
  // USE_SERIAL.printf("[WSc] get text: %s\n", (char*) payload);
  // Deserialize json and save it into doc
  deserializeError = deserializeJson(doc, json);

  // Was there an error?
  if (deserializeError) {
    USE_SERIAL.printf("[WSc] deserializeJson failed: %s\n", deserializeError.c_str());
    return;
  }
}

void loop() {
  webSocket.loop();
  // Hard delay in order to smooth the websocket connection and to not have
  // multiple messages at the same time
  delay(24); 

  // Only update the LEDs after the interval has passed
  if ((unsigned long)(millis() - previousMillis) >= interval) {
    previousMillis = millis();

    // Set the color of thee active LED
    leds[activeLed].r = doc[0];
    leds[activeLed].g = doc[1];
    leds[activeLed].b = doc[2];

    leds[activeLed + halfNumLeds].r = doc[0];
    leds[activeLed + halfNumLeds].g = doc[1];
    leds[activeLed + halfNumLeds].b = doc[2];

    // Fade every LED to black
    for(int dot = 0; dot < NUM_LEDS; dot++) { 
      leds[dot].fadeToBlackBy(toBlack);
    }

    activeLed = activeLed + 1;

    // Reset the active LED when the end of the strip is reached
    if (activeLed >= halfNumLeds) {
      activeLed = 0;
    }
   
    FastLED.show();
 }

}
