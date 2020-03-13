#include <Arduino.h>
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "FastLED.h"

//#include <NTPClient.h>
#include <WiFi.h>
#include "fauxmoESP.h"
#include <WebServer.h>
//#include <ESPmDNS.h>
#include <Update.h>
#include "ota.h"
#include <WiFiClient.h>

#define SERIAL_BAUDRATE 115200

#define NUM_LEDS 60
CRGB leds[NUM_LEDS];

//#define SEND_SIGNAL_PIN 14 // find actual pin
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

bool pourValveOne = false;
bool pourValveTwo = false;
 
//
#define LED_STRIP 15
//
#define B_1 5
#define B_2 17
#define B_3 16
#define BUTTON 4
//
#define VALVE_ONE 18
#define VALVE_TWO 19

#define WIFI_SSID "Prancing Pony"
#define WIFI_PASS "sexykeaton"

//fill in values
#define WEAK_VALUE 234
#define NORMAL_VALUE 128
#define STRONG_VALUE 77
#define ONE_SHOT_VALUE 163
#define TWO_SHOT_VALUE 54
#define POP_VALUE 216
#define DRAIN_ONE_VALUE 15
#define DRAIN_TWO_VALUE 16
#define OTA_VALUE 62
#define POUR_VALUE 255


//Initialize variables
int pixelIndex = 0;

int valveOneOpenTime = 0;
int valveTwoOpenTime = 0;
long oneShotTime = 2000;

unsigned long lastPour = 0;
unsigned long startPour = 0;
//const long utcOffsetInSeconds = -18000; //EST
bool isPouring = false;

bool drainValveOne = false;
bool drainValveTwo = false;

bool specialDrink = false; 

fauxmoESP fauxmo;
WiFiClient wifi_client;

//WebServer server(80);
//WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// -----------------------------------------------------------------------------

//------------------------------------------------------------------------------
void weakDrink()
{
  Serial.println("WEAK");
  valveOneOpenTime = 1*oneShotTime;
  valveTwoOpenTime = 6000;
//  ledcWrite(ledChannel, WEAK_VALUE);

}

void normalDrink()
{
  Serial.println("NORMAL");
  valveOneOpenTime = 2*oneShotTime;
  valveTwoOpenTime = 6000;
//  ledcWrite(ledChannel, NORMAL_VALUE);
}

void strongDrink()
{
  Serial.println("STRONG");
  valveOneOpenTime = 3*oneShotTime;
  valveTwoOpenTime = 6000;
//  ledcWrite(ledChannel, STRONG_VALUE);
}

void pop()
{
  Serial.println("POP");
  valveOneOpenTime = 0;
  valveTwoOpenTime = 4000;  
  //specify as a special drink 
  specialDrink = true;
//  ledcWrite(ledChannel, POP_VALUE);
}

void oneShot()
{
  Serial.println("ONE SHOT");
  valveOneOpenTime = oneShotTime;
  valveTwoOpenTime = 0;
  //specify as a special drink
  specialDrink = true;
//  ledcWrite(ledChannel, ONE_SHOT_VALUE);
}

void twoShots()
{
  Serial.println("TWO SHOTS");
  valveOneOpenTime = 2 * oneShotTime;
  valveTwoOpenTime = 0;
  //specify as a special drink 
  specialDrink = true;
//  ledcWrite(ledChannel, TWO_SHOT_VALUE);
}

void pour()
{
  Serial.println("POUR");

  //Open both valves, update last pour variable, offically start pour, and set the pixel index to 0 for the progress bar
  pourValveOne = true;
  startPour = millis();
  isPouring = true;
  pixelIndex = 0;
//  ledcWrite(ledChannel, POUR_VALUE);
}

void setStrength()
{
  if (digitalRead(B_1) == LOW)
  {
    weakDrink();
  }
  if (digitalRead(B_2) == LOW)
  {
    normalDrink();
  }
  if (digitalRead(B_3) == LOW)
  {
    strongDrink();
  }
}

void drain() { //Wrote to be blocking on purpose. Do not want any button inputs or Alexa inputs once this method has begun
  if (drainValveOne == true)
  {
    ledcWrite(ledChannel, DRAIN_ONE_VALUE);
    digitalWrite(VALVE_ONE, HIGH);
    for (int i = 0; i < NUM_LEDS; i++) //using progress bar as block
    {
      setPixel(i, 0x00, 0xff, 0xdf);
      showStrip();
      delay(5*60*1000/NUM_LEDS); //5 minutes total
    }
    digitalWrite(VALVE_ONE, LOW);
    drainValveOne = false;
    endAnimation(0x00, 0xff, 0x00);
    }

  if (drainValveTwo == true)
  {    
    ledcWrite(ledChannel, DRAIN_TWO_VALUE);
    digitalWrite(VALVE_TWO, HIGH);
    for (int i = 0; i < NUM_LEDS; i++) //using progress bar as block
    {
      setPixel(i, 0x00, 0xff, 0xdf);
      showStrip();
      delay(5*60*1000/NUM_LEDS); //5 minutes total
    }
    digitalWrite(VALVE_TWO, LOW);
    drainValveTwo = false;
    endAnimation(0x00, 0xff, 0x00);
  }
}
//-----------------------------------------------------------------------------------------------
void wifiSetup()
{
  WiFi.mode(WIFI_STA);
  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}
//-----------------------------------------------------------------------------------------------
void setPixel(int Pixel, byte red, byte green, byte blue)
{
  leds[Pixel].r = red;
  leds[Pixel].g = green;
  leds[Pixel].b = blue;
}

void setAll(byte red, byte green, byte blue)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    setPixel(i, red, green, blue);
  }
  showStrip();
}

void showStrip()
{
  FastLED.show();
}

void meteorRain(byte red, byte green, byte blue, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay)
{
  setAll(0, 0, 0);
  for (int i = 0; i < NUM_LEDS + NUM_LEDS; i++)
  {
    // fade brightness all LEDs one step
    for (int j = 0; j < NUM_LEDS; j++)
    {
      if ((!meteorRandomDecay) || (random(10) > 5))
      {
        leds[j].fadeToBlackBy(meteorTrailDecay);
      }
    }
    // draw meteor
    for (int j = 0; j < meteorSize; j++)
    {
      if ((i - j < NUM_LEDS) && (i - j >= 0))
      {
        setPixel(i - j, red, green, blue);
        delay(SpeedDelay);
      }
    }
    showStrip();
  }
}

void endAnimation(byte red, byte green, byte blue)
{
  float r, g, b;
  for (int k = 215; k >= 0; k = k - 2)
  {
    r = (k / 215.0) * red;
    g = (k / 215.0) * green;
    b = (k / 215.0) * blue;
    setAll(r, g, b);
    showStrip();
  }
  for (int k = 0; k < 115; k = k + 1)
  {
    r = (k / 115.0) * red;
    g = (k / 115.0) * green;
    b = (k / 115.0) * blue;
    setAll(r, g, b);
    showStrip();
  }
  delay(55);
  for (uint16_t i = NUM_LEDS; i >= 0; i--)
    {
      setPixel(i, 0x00, 0x00, 0x00);
      showStrip();
      delay(5.0);
    }
}

//-----------------------------------------------------------------------------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  //Set up led strip
//  FastLED.addLeds<WS2812B, LED_STRIP, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  //Start serial
  

  //Set up pins
  pinMode(B_1, INPUT_PULLUP);
  pinMode(B_2, INPUT_PULLUP);
  pinMode(B_3, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(VALVE_ONE, OUTPUT);
  pinMode(VALVE_TWO, OUTPUT);

//  // configure LED PWM functionalitites
//  ledcSetup(ledChannel, freq, resolution);
//  // attach the channel to the GPIO to be controlled
//  ledcAttachPin(SEND_SIGNAL_PIN, ledChannel);
  //Set drink strength to normal at boot
//  normalDrink();

  //Set up WiFi

  wifiSetup();

  //Connect to time server
//  timeClient.begin();

  //Start fauxmo and set up smart device
  fauxmo.createServer(true);
  fauxmo.setPort(80);
  fauxmo.enable(false);
  fauxmo.enable(true);
  fauxmo.addDevice("Drink Bot");
  fauxmo.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value) {
    //When Alexa turns ON smart device, adjust settings accordingly

    //After every call, Alexa will set the "brightness" to 2%, or value = 6. This is because if value stays on 128 when turned off, drink
    //bot will pour a normal drink, then pour the drink you actually want. By setting value to 6 (or any number) and adding the if (value != 6)
    //line, drink bot will not process anything until we are at the desired value.
    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
    if (value != 6)
    {
      Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
      if (state)
      {
        if (value == WEAK_VALUE)
        {
          weakDrink();
        }
        else if (value == NORMAL_VALUE)
        {
          normalDrink();
        }
        else if (value == STRONG_VALUE)
        {
          strongDrink();
        }
        else if (value == POP_VALUE)//need to test and get actual values
        {
          pop();
        }
        else if (value == ONE_SHOT_VALUE)//need to test and get actual values
        {
          oneShot();
        }
        else if (value == TWO_SHOT_VALUE)//need to test and get actual values
        {
          twoShots();
        }
        else if (value == DRAIN_ONE_VALUE)//need to test and get actual values
        {
          //Set flag to trigger the drain method in the main loop
          drainValveOne = true;
          pixelIndex = 0;
        }
        else if (value == DRAIN_TWO_VALUE)//need to test and get actual values
        {
          //Set flag to trigger the drain method in the main loop
          drainValveTwo = true;
          pixelIndex = 0;
        }
        if(value == OTA_VALUE){
          disableCore0WDT();
          disableCore1WDT();
          perform_ota(&wifi_client);
        }
        if (value != OTA_VALUE && value != DRAIN_TWO_VALUE && value != DRAIN_ONE_VALUE) //Pour if the draining system was not turned on
        {
          pour();
        }
      }
    }
  });

  //End set up animation 
  for (int i = 0; i < NUM_LEDS; i++)
  {
    setPixel(i, 0x00, 0xff, 0x00);
    showStrip();
  }
  delay(1000);
  setAll(0x00, 0x00, 0x00);

  Serial.println("OTA WORKED");
}


void loop()
{
  //Listen for button events to set the drink strength if a pour is not in process
  if (!isPouring)
  {
    setStrength();
  }

  //If button is pressed and not currently pouring then pour
  if (digitalRead(BUTTON) == LOW && !isPouring)
  {
    
    pour();
  }

  //Get current time
//  timeClient.update();
  unsigned long currentTime = millis();

  if (pourValveOne)
  {
    digitalWrite(VALVE_ONE, HIGH);
  }


  //Wait for appropiate amount of time to pass to close valves. Pour must be in process
  if ((currentTime - startPour) >= valveOneOpenTime && pourValveOne)
  {
    digitalWrite(VALVE_ONE, LOW);
    pourValveOne = false;
    pourValveTwo = true;
    lastPour = millis();
    digitalWrite(VALVE_TWO, HIGH);
  }


  if ((currentTime - lastPour) >= valveTwoOpenTime && pourValveTwo)
  {
    digitalWrite(VALVE_TWO, LOW);
    pourValveTwo = false;
//    endAnimation(0x00, 0xff, 0x00);
    isPouring = false;
    //If a special drink was poured, set times to a normal drink
    for (int i = 0; i < NUM_LEDS; i++){
    setPixel(i, 0x00, 0x00, 0x00);
    }
    showStrip();
    if (specialDrink)
    {
      normalDrink();
      specialDrink = false;
    }
    Serial.println("Pour ended");
  }

  //Run syncronized progress bar with valve that is open the longest
  if (currentTime - startPour > (valveTwoOpenTime+valveOneOpenTime / NUM_LEDS) * pixelIndex && isPouring)
  {
    setPixel(pixelIndex, 0x00, 0xff, 0xdf);
    showStrip();
    pixelIndex = pixelIndex + 1;
  }

  //Check for both valves to be closed to run end animation and officially end the pour. Pour must be in process. 
//  if (!pourValveOne && !pourValveTwo && isPouring)
//  {
    //Run end animation and set isPouring false
//    endAnimation(0x00, 0xff, 0x00);
//    isPouring = false;
//    //If a special drink was poured, set times to a normal drink
//    if (specialDrink == true)
//    {
//      normalDrink();
//      specialDrink = false;
//    }
//  }


  //Must be at least 6 am to listen for Alexa input
  fauxmo.handle();
  

  //Draining method. This only runs if alexa sets drink bot to drain. Which it only does during the drain routine "Alexa, drain system seven one five"
//  drain();
}
