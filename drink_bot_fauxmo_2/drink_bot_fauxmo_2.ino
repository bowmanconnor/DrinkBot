#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "fauxmoESP.h"

fauxmoESP fauxmo;

// -----------------------------------------------------------------------------

#define SERIAL_BAUDRATE     115200
//D1-D3
#define LED_1                    5
#define LED_2                    4
#define LED_3                    0
//D5-D8
#define B_1                      14
#define B_2                      12
#define B_3                      13
#define BUTTON                   15
//SD3-SD2
#define VALVE_ONE                10
#define VALVE_TWO                9

#define WIFI_SSID               "Prancing Pony"
#define WIFI_PASS               "sexykeaton"

//Initialize variables
int VALVE_ONE_OPEN_TIME = 0;
int VALVE_TWO_OPEN_TIME = 0;
unsigned long lastPour = 0;

ICACHE_RAM_ATTR void weakDrink() {
  //Set LEDs, change times
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  VALVE_ONE_OPEN_TIME = 2 * 1000;
  VALVE_TWO_OPEN_TIME = 3 * 1000;
}

ICACHE_RAM_ATTR void normalDrink() {
  //Set LEDs, change times
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, LOW);
  VALVE_ONE_OPEN_TIME = 3 * 1000;
  VALVE_TWO_OPEN_TIME = 3 * 1000;
}

ICACHE_RAM_ATTR void strongDrink() {  
  //Set LEDs, change times
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, HIGH);
  VALVE_ONE_OPEN_TIME = 4 * 1000;
  VALVE_TWO_OPEN_TIME = 3 * 1000;
}

ICACHE_RAM_ATTR void pour() {
  //Open both valves and update last pour variable
  digitalWrite(VALVE_ONE, HIGH);
  digitalWrite(VALVE_TWO, HIGH);
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
  //Start serial
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println();
  Serial.println();

  //Set up pins
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(B_1, INPUT_PULLUP);
  pinMode(B_2, INPUT_PULLUP);
  pinMode(B_3, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(VALVE_ONE, OUTPUT);
  pinMode(VALVE_TWO, OUTPUT);

  //Set drink strength to normal at boot
  normalDrink();

  //Set up esp interrupts
  attachInterrupt(digitalPinToInterrupt(B_1), weakDrink, FALLING);
  attachInterrupt(digitalPinToInterrupt(B_2), normalDrink, FALLING);
  attachInterrupt(digitalPinToInterrupt(B_3), strongDrink, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON), pour, FALLING);  

 //Set up WiFi
  wifiSetup();

  //Start fauxmo and set up three smart devices
  fauxmo.enable(true);
  fauxmo.addDevice("Normal Drink");
  fauxmo.addDevice("Weak Drink");
  fauxmo.addDevice("Strong Drink");
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state) {
    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF");
    //When Alexa turns ON smart device, adjust settings accordingly
     if (state){
      if (strcmp(device_name, "Normal Drink")==0) {
        normalDrink();
      }
      if (strcmp(device_name, "Weak Drink")==0) {
        weakDrink();
      }
      if (strcmp(device_name, "Strong Drink")==0) {
        strongDrink();
      }
      //Always pour drink after adjusting settings
      pour();
     }
  });
}

void loop() {
  //Wait for appropiate amount of time to pass to close valves 
  if ((millis()-lastPour) >= VALVE_ONE_OPEN_TIME) {
    digitalWrite(VALVE_ONE, LOW);
  }
  if ((millis()-lastPour) >= VALVE_TWO_OPEN_TIME) {
    digitalWrite(VALVE_TWO, LOW); 
  }
  fauxmo.handle();
}
