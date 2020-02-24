#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "FastLED.h"
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "fauxmoESP.h"
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

#define SERIAL_BAUDRATE 115200

#define NUM_LEDS 60
CRGB leds[NUM_LEDS];

//
#define LED_1 25
#define LED_2 26
#define LED_3 27
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

//Initialize variables
int pixelIndex = 0;

int valveOneOpenTime = 0;
int valveTwoOpenTime = 0;
long oneShotTime = 2000;

unsigned long lastPour = 0;
const long utcOffsetInSeconds = -18000; //EST
bool isPouring = false;
// bool partyMode = false;

bool drainValveOne = false;
bool drainValveTwo = false;

bool specialDrink = false; 

fauxmoESP fauxmo;
WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// -----------------------------------------------------------------------------
const char* loginIndex = 
    "<form name='loginForm'>"
        "<table width='20%' bgcolor='A09F9F' align='center'>"
            "<tr>"
                "<td colspan=2>"
                    "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                    "<br>"
                "</td>"
                "<br>"
                "<br>"
            "</tr>"
            "<td>Username:</td>"
            "<td><input type='text' size=25 name='userid'><br></td>"
            "</tr>"
            "<br>"
            "<br>"
            "<tr>"
                "<td>Password:</td>"
                "<td><input type='Password' size=25 name='pwd'><br></td>"
                "<br>"
                "<br>"
            "</tr>"
            "<tr>"
                "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
            "</tr>"
        "</table>"
    "</form>"
    "<script>"
        "function check(form)"
        "{"
        "if(form.userid.value=='admin' && form.pwd.value=='admin')"
        "{"
        "window.open('/serverIndex')"
        "}"
        "else"
        "{"
        " alert('Error Password or Username')/*displays error message*/"
        "}"
        "}"
    "</script>";

const char* serverIndex = 
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
      "<input type='file' name='update'>"
            "<input type='submit' value='Update'>"
        "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
      "$('form').submit(function(e){"
      "e.preventDefault();"
      "var form = $('#upload_form')[0];"
      "var data = new FormData(form);"
      " $.ajax({"
      "url: '/update',"
      "type: 'POST',"
      "data: data,"
      "contentType: false,"
      "processData:false,"
      "xhr: function() {"
      "var xhr = new window.XMLHttpRequest();"
      "xhr.upload.addEventListener('progress', function(evt) {"
      "if (evt.lengthComputable) {"
      "var per = evt.loaded / evt.total;"
      "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
      "}"
      "}, false);"
      "return xhr;"
      "},"
      "success:function(d, s) {"
      "console.log('success!')" 
  "console.log('success!')" 
      "console.log('success!')" 
  "console.log('success!')" 
      "console.log('success!')" 
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";
//------------------------------------------------------------------------------
void OTAServer()
{
  if (!MDNS.begin("esp32"))
  { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } });
  server.begin();
}

//------------------------------------------------------------------------------
void weakDrink()
{
  Serial.println("WEAK");
  //Set LEDs, change times
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  valveOneOpenTime = 1*oneShotTime;
  valveTwoOpenTime = 6000;
}

void normalDrink()
{
  Serial.println("NORMAL");
  //Set LEDs, change times
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, LOW);
  valveOneOpenTime = 2*oneShotTime;
  valveTwoOpenTime = 6000;
}

void strongDrink()
{
  Serial.println("STRONG");
  //Set LEDs, change times
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, HIGH);
  valveOneOpenTime = 3*oneShotTime;
  valveTwoOpenTime = 6000;
}

void pop()
{
  Serial.println("POP");
  //Set LEDs, change times
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  valveOneOpenTime = 0;
  valveTwoOpenTime = 4000;  
  //specify as a special drink 
  specialDrink = true;
}

void oneShot()
{
  Serial.println("ONE SHOT");
  //Set LEDs, change times
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, LOW);
  valveOneOpenTime = oneShotTime;
  valveTwoOpenTime = 0;
  //specify as a special drink
  specialDrink = true;

}

void twoShots()
{
  Serial.println("TWO SHOTS");
  //Set LEDs, change times
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, HIGH);
  digitalWrite(LED_3, HIGH);
  valveOneOpenTime = 2 * oneShotTime;
  valveTwoOpenTime = 0;
  //specify as a special drink 
  specialDrink = true;
}

void pour()
{
  Serial.println("POUR");
  //Open both valves, update last pour variable, offically start pour, and set the pixel index to 0 for the progress bar
  digitalWrite(VALVE_ONE, HIGH);
  digitalWrite(VALVE_TWO, HIGH);
  lastPour = millis();
  isPouring = true;
  pixelIndex = 0;
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
  //  for(int k = 0; k < 120; k=k+1) {
  //    r = (k/120.0)*red;
  //    g = (k/120.0)*green;
  //    b = (k/120.0)*blue;
  //    setAll(r,g,b);
  //    showStrip();
  //  }
  //  delay(20);
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
  //Set up led strip
  FastLED.addLeds<WS2812B, LED_STRIP, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

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

  //Set up WiFi
  wifiSetup();

  //Set up OTA Server
  // OTAServer();

  //Connect to time server
  timeClient.begin();

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
        if (value == 62)
        {
          weakDrink();
        }
        if (value == 128)
        {
          normalDrink();
        }
        if (value == 231)
        {
          strongDrink();
        }
        if (value == 33)//need to test and get actual values
        {
          pop();
        }
        if (value == 34)//need to test and get actual values
        {
          oneShot();
        }
        if (value == 35)//need to test and get actual values
        {
          twoShots();
        }
        if (value = 36)//need to test and get actual values
        {
          //Set flag to trigger the drain method in the main loop
          drainValveOne = true;
          pixelIndex = 0;
        }
        if (value = 37)//need to test and get actual values
        {
          //Set flag to trigger the drain method in the main loop
          drainValveTwo = true;
          pixelIndex = 0;
        }
        if (value != 36 && value != 37) //Pour if the draining system was not turned on
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

  //Listen for OTA files
  //server.handleClient();

  //Get current time
  timeClient.update();
  unsigned long currentTime = millis();

  //Wait for appropiate amount of time to pass to close valves. Pour must be in process
  if ((currentTime - lastPour) >= valveOneOpenTime && isPouring)
  {
    digitalWrite(VALVE_ONE, LOW);
  }

  if ((currentTime - lastPour) >= valveTwoOpenTime && isPouring)
  {
    digitalWrite(VALVE_TWO, LOW);
  }

  //Run syncronized progress bar with valve that is open the longest
  if (currentTime - lastPour > (((valveTwoOpenTime>valveOneOpenTime) ? valveTwoOpenTime : valveOneOpenTime) / NUM_LEDS) * pixelIndex && isPouring)//not sure if works lolxd
  {
    setPixel(pixelIndex, 0x00, 0xff, 0xdf);
    showStrip();
    pixelIndex = pixelIndex + 1;
  }

  //Check for both valves to be closed to run end animation and officially end the pour. Pour must be in process. 
  if (digitalRead(VALVE_ONE) == LOW && digitalRead(VALVE_TWO) == LOW && isPouring)
  {
    //Run end animation and set isPouring false
    endAnimation(0x00, 0xff, 0x00);
    isPouring = false;
    //If a special drink was poured, set times to a normal drink
    if (specialDrink = true)
    {
      normalDrink();
      specialDrink = false;
    }
  }

  //Must be at least 6 am to listen for Alexa input
  if (timeClient.getHours() > 6)
  {
    fauxmo.handle();
  }

  //Draining method. This only runs if alexa sets drink bot to drain. Which it only does during the drain routine "Alexa, drain system seven one five"
  drain();
}