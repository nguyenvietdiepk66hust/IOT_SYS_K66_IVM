#include "esp_modbus_common.h"
#include "esp_modbus_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <WiFiAP.h>
#include "../apps/Wifi.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "LittleFS.h"

void setup() {
 Serial.begin(115200);
 // || Creation           ||     Task function         ||     PC name             || heap size  || Parameter || Priority  || Task handle     || CoreID   ||
xTaskCreatePinnedToCore(  Wifi_Task_Func      , "atApp_Wifi_Application"        ,  10000      ,     NULL    ,   1       , &Task_atApp_Wifi      ,    1     );

}

void loop() {

}