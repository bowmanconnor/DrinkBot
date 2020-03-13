#include <Update.h>
#include <WiFi.h>

#define OTA_HOST "github.com"
#define OTA_PORT 80//443
#define FIRMWARE_BIN "/bowmanconnor/DrinkBot/blob/master/drinkbot/drinkbot.ino.esp32.bin"

// This would get the file from http://cjelm.spdns.org:1234/firmware.bin

void perform_ota(WiFiClient* client);
