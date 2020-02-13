#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"


fauxmoESP fauxmo;

// -----------------------------------------------------------------------------

#define SERIAL_BAUDRATE     115200

#define LED_1                    4
#define LED_2                    5
#define LED_3                    0
#define B_1                      2
#define B_2                      2
#define B_3                      2
#define BUTTON                   2
#define VALVE_ONE                10
#define VALVE_TWO                10
#define ID_DRINK_BOT            "Dring Bot"
#define WIFI_SSID               "Prancing Pony"
#define WIFI_PASS               "sexykeaton"

int VALVE_ONE_OPEN_TIME = 0;
int VALVE_TWO_OPEN_TIME = 0;
unsigned long lastPour = 0;
int count = 0;

void weakDrink(){
    digitalWrite(LED_1, HIGH);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW);

    VALVE_ONE_OPEN_TIME = 0*1000;
    VALVE_TWO_OPEN_TIME = 0*1000;
}

void normalDrink(){
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, HIGH);
    digitalWrite(LED_3, LOW);

    VALVE_ONE_OPEN_TIME = 0*1000;
    VALVE_TWO_OPEN_TIME = 0*1000;
}

void strongDrink(){
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, HIGH);

    VALVE_ONE_OPEN_TIME = 0*1000;
    VALVE_TWO_OPEN_TIME = 0*1000;
}

void setStrength(){
    if (digitalRead(B_1) == LOW){
        weakDrink();
    }
    else if (digitalRead(B_2) == LOW){
        normalDrink();
    }
    else if (digitalRead(B_3) == LOW){
        strongDrink();
    }
}
 void pour(){
    digitalWrite(VALVE_ONE, HIGH);
    digitalWrite(VALVE_ONE, HIGH);
    lastPour = millis();
 }


void wifiSetup() {
    WiFi.mode(WIFI_STA);
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(100);
    }
    Serial.println();
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void setup() {
    Serial.begin(SERIAL_BAUDRATE);
    Serial.println();
    Serial.println();

    
    pinMode(LED_1, OUTPUT);
    pinMode(LED_2, OUTPUT);
    pinMode(LED_3, OUTPUT);
    pinMode(B_1, INPUT_PULLUP);
    pinMode(B_2, INPUT_PULLUP);
    pinMode(B_3, INPUT_PULLUP);
    pinMode(BUTTON, INPUT_PULLUP);
    pinMode(VALVE_ONE, OUTPUT);
    pinMode(VALVE_TWO, OUTPUT);
    


    wifiSetup();
    fauxmo.createServer(true); // not needed, this is the default value
    fauxmo.setPort(80); // This is required for gen3 devices
    fauxmo.enable(true);
    fauxmo.addDevice(ID_DRINK_BOT);
    fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
        Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
        if (state){
          if (value == 0){
            weakDrink();
          }
          if (value == 128){
            normalDrink();
          }
          if (value == 255){
            strongDrink();
          }
          pour();
          //fauxmo.setState(ID_DRINK_BOT, false, 1);
        }
    });
}

void loop() {
  Serial.print("Time at beggining of loop: "+ millis());
  setStrength();
  unsigned long timeSinceLastPour = millis() - lastPour;
  if (timeSinceLastPour >= VALVE_ONE_OPEN_TIME){
      digitalWrite(VALVE_ONE, LOW);
  }
  if (timeSinceLastPour >= VALVE_TWO_OPEN_TIME){
      digitalWrite(VALVE_TWO, LOW);
      fauxmo.setState(ID_DRINK_BOT, false, 1);
  }
  if (digitalRead(BUTTON) == LOW){
      pour();
  }
  fauxmo.handle();
  Serial.print("Time at end of loop: "+ millis());
}
