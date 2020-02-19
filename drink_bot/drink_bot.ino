#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "FastLED.h"
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "fauxmoESP.h"

#define SERIAL_BAUDRATE     115200

#define NUM_LEDS 60 
CRGB leds[NUM_LEDS];

//
#define LED_1                    34
#define LED_2                    35
#define LED_3                    32
#define LED_STRIP                15 
//
#define B_1                      33
#define B_2                      25
#define B_3                      26
#define BUTTON                   27
//
#define VALVE_ONE                16
#define VALVE_TWO                2

#define WIFI_SSID               "WM-Welcome"
#define WIFI_PASS               ""

//Initialize variables
int VALVE_ONE_OPEN_TIME = 0;
int VALVE_TWO_OPEN_TIME = 0;
long ONE_SHOT_TIME = 0;
unsigned long lastPour = 0;
const long utcOffsetInSeconds = -18000;
bool isPouring = false;

fauxmoESP fauxmo;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// -----------------------------------------------------------------------------

//ICACHE_RAM_ATTR needed before a function to run in with an interruption
ICACHE_RAM_ATTR void weakDrink() {
  Serial.println("WEAK");
  //Set LEDs, change times
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  VALVE_ONE_OPEN_TIME = ONE_SHOT_TIME * 1000;
  VALVE_TWO_OPEN_TIME = 2 * 1000;
}

ICACHE_RAM_ATTR void normalDrink() {
  Serial.println("NORMAL");
  //Set LEDs, change times
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, LOW);
  VALVE_ONE_OPEN_TIME = 2*ONE_SHOT_TIME * 1000;
  VALVE_TWO_OPEN_TIME = 3 * 1000;
}

ICACHE_RAM_ATTR void strongDrink() {
  Serial.println("STRONG");  
  //Set LEDs, change times
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, HIGH);
  VALVE_ONE_OPEN_TIME = 3*ONE_SHOT_TIME * 1000;
  VALVE_TWO_OPEN_TIME = 4 * 1000;
}

void pop() {
  Serial.println("POP");  
  //Set LEDs, change times
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  VALVE_ONE_OPEN_TIME = 0 * 1000;
  VALVE_TWO_OPEN_TIME = 4 * 1000;
}

void oneShot() {
  Serial.println("ONE SHOT");  
  //Set LEDs, change times
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, LOW);
  VALVE_ONE_OPEN_TIME = ONE_SHOT_TIME * 1000;
  VALVE_TWO_OPEN_TIME = 0 * 1000;
}

void twoShots() {
  Serial.println("TWO SHOTS");  
  //Set LEDs, change times
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, HIGH);
  VALVE_ONE_OPEN_TIME = 2*ONE_SHOT_TIME * 1000;
  VALVE_TWO_OPEN_TIME = 0 * 1000;
}
ICACHE_RAM_ATTR void pour() {
  Serial.println("POUR");
  //Open both valves and update last pour variable
  digitalWrite(VALVE_ONE, HIGH);
  digitalWrite(VALVE_TWO, HIGH);
  lastPour = millis();
  isPouring = true;
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

void setPixel(int Pixel, byte red, byte green, byte blue) {
   leds[Pixel].r = red;
   leds[Pixel].g = green;
   leds[Pixel].b = blue;
}

void setAll(byte red, byte green, byte blue) {
  for(int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue); 
  }
  showStrip();
}

void showStrip() {
   FastLED.show();
}

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay) {  
  setAll(0,0,0);
  for(int i = 0; i < NUM_LEDS+NUM_LEDS; i++) {
    // fade brightness all LEDs one step
    for(int j=0; j<NUM_LEDS; j++) {
      if( (!meteorRandomDecay) || (random(10)>5) ) {
        leds[j].fadeToBlackBy( meteorTrailDecay );        
      }
    }
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if( ( i-j <NUM_LEDS) && (i-j>=0) ) {
        setPixel(i-j, red, green, blue);
        delay(SpeedDelay);    
      } 
    }
    showStrip();
  }
}

void FadeInOutInGone(byte red, byte green, byte blue){
  float r, g, b;
  for(int k = 0; k < 180; k=k+1) { 
    r = (k/180.0)*red;
    g = (k/180.0)*green;
    b = (k/180.0)*blue;
    setAll(r,g,b);
    showStrip();
  }
   for(int k = 245; k >= 0; k=k-2) {
    r = (k/246.0)*red;
    g = (k/246.0)*green;
    b = (k/246.0)*blue;
    setAll(r,g,b);
    showStrip();
  }
  for(int k = 0; k < 180; k=k+1) { 
    r = (k/180.0)*red;
    g = (k/180.0)*green;
    b = (k/180.0)*blue;
    setAll(r,g,b);
    showStrip();
  }
    delay(100);
    for(uint16_t i=NUM_LEDS; i>0; i--) {
      setPixel(i, 0x00, 0x00, 0x00);
      showStrip();
      delay(7.5);
  }
  setAll(0x00, 0x00, 0x00);
}


void setup() {
  //Set up led strip
  FastLED.addLeds<WS2812B, LED_STRIP, GRB>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );

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
  
  //Connect to time server
  timeClient.begin();

  //Start fauxmo and set up smart device
  fauxmo.createServer(true); 
  fauxmo.setPort(80); 
  fauxmo.enable(false);
  fauxmo.enable(true);
  fauxmo.addDevice("Drink Bot");
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) { 
    
     //When Alexa turns ON smart device, adjust settings accordingly

     //After every call, Alexa will set the "brightness" to 2%, or value = 6. This is because if value stays on 128 when turned off, drink 
     //bot will pour a normal drink, then pour the drink you actually want. By setting value to 6 (or any number) and adding the if (value != 6) 
     //line, drink bot will not process anything until we are at the desired value.
     if (value!= 6) {
       Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
       if (state){
        if (value==62) {
          weakDrink();
        }
        if (value==128) {
          normalDrink();
        }
        if (value==231) {
          strongDrink();
        }
        if (value==){
          pop();
        }
        if (value==){
          oneShot();
        }
        if (value==){
          twoShots();
        }
        //Always pour drink after adjusting settings
        pour();
       }
     }
  });
}

void loop() {
  
  //Get current time
  timeClient.update();

  //Wait for appropiate amount of time to pass to close valves 
  if ((unsigned long)(millis()-lastPour) >= VALVE_ONE_OPEN_TIME) {
    digitalWrite(VALVE_ONE, LOW);
  }
  
  if ((unsigned long)(millis()-lastPour) >= VALVE_TWO_OPEN_TIME) {
      digitalWrite(VALVE_TWO, LOW); 
  }
  //Check for both valves to be done pouring and isPouring to still be true
  if (digitalRead(VALVE_ONE)==LOW && digitalRead(VALVE_TWO)==LOW && isPouring){
      //Run end animation and set isPouring false
      FadeInOutInGone(0x00,0xff,0x00);
      isPouring = false;
  }

  //Check if isPouring is true to run progress bar
  if (isPouring){
      //Set the time for this animation to sync up with whichever valve is open longer
      if (VALVE_TWO_OPEN_TIME>VALVE_ONE_OPEN_TIME){
          meteorRain(0x00,0x00,0xff, 1, 188, true, VALVE_TWO_OPEN_TIME/60);
      else {
          meteorRain(0x00,0x00,0xff, 1, 188, true, VALVE_ONE_OPEN_TIME/60);
        }
      }
  }      

  //Must be at least 6 am to listen for Alexa input
  if (timeClient.getHours() > 6){
    //Serial.println(timeClient.getHours());
    fauxmo.handle();
  }
}
    
       
