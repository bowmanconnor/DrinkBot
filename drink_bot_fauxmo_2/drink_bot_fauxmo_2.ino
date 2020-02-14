#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"


fauxmoESP fauxmo;

// -----------------------------------------------------------------------------

#define SERIAL_BAUDRATE     115200

#define LED_1                    0
#define LED_2                    0
#define LED_3                    D2
#define B_1                      D5
#define B_2                      D6
#define B_3                      D7
#define BUTTON                   2
#define VALVE_ONE                D0
#define VALVE_TWO                D1

#define WIFI_SSID               "Prancing Pony"
#define WIFI_PASS               "sexykeaton"

int VALVE_ONE_OPEN_TIME = 0;
int VALVE_TWO_OPEN_TIME = 0;
unsigned long lastPour = 0;
int count = 0;

void weakDrink() {
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);

  VALVE_ONE_OPEN_TIME = 2 * 1000;
  VALVE_TWO_OPEN_TIME = 3 * 1000;
}

void normalDrink() {
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, LOW);

  VALVE_ONE_OPEN_TIME = 3 * 1000;
  VALVE_TWO_OPEN_TIME = 3 * 1000;
}

void strongDrink() {
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, HIGH);

  VALVE_ONE_OPEN_TIME = 4 * 1000;
  VALVE_TWO_OPEN_TIME = 3 * 1000;
}

void setStrength() {
  if (digitalRead(B_1) == LOW) {
    weakDrink();
  }
  else if (digitalRead(B_2) == LOW) {
    normalDrink();
  }
  else if (digitalRead(B_3) == LOW) {
    strongDrink();
  }
}
void pour() {
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
  normalDrink();


  wifiSetup();

  fauxmo.enable(true);
  fauxmo.addDevice("Normal Drink");
  fauxmo.addDevice("Weak Drink");
  fauxmo.addDevice("Strong Drink");
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state) {
    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF");
    if (strcmp(device_name, "Normal Drink")==0) {
       normalDrink();
    }
    if (strcmp(device_name, "Weak Drink")==0) {
       weakDrink();
    }
    if (strcmp(device_name, "Strong Drink")==0) {
       strongDrink();
    }
    digitalWrite(VALVE_ONE, HIGH);
    digitalWrite(VALVE_TWO, HIGH);
    lastPour = millis();
  });
}

void loop() {
  setStrength();
 
  if ((millis()-lastPour) >= VALVE_ONE_OPEN_TIME) {
    digitalWrite(VALVE_ONE, LOW);
  }
  if ((millis()-lastPour) >= VALVE_TWO_OPEN_TIME) {
    digitalWrite(VALVE_TWO, LOW); 
  }
  if (digitalRead(BUTTON) == LOW) {
    digitalWrite(VALVE_ONE, HIGH);
    digitalWrite(VALVE_TWO, HIGH);
    lastPour = millis();
    
  }
  fauxmo.handle();
  
}
