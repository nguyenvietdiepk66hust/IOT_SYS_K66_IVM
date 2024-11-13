#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include <EEPROM.h>
#include "DHTesp.h"
#define eeprom_size 200
#define eeprom_ssid_length_address  1
#define eeprom_ssid_address  2
#define eeprom_password_length_address  50
#define eeprom_password_address  51
#define eeprom_token_address  101
#define LED_1 18
#define LED_2 5
#define DHT_PIN 33
DHTesp dht;
char ssid[] = "VIETTEL";     // your network SSID (name)
char password[] = "3ngay0tam";  // your network password
String serverToken = "123123123";
int status = WL_IDLE_STATUS;     // the Wifi radio's status

String str_ssid, str_pass;
String eeprom_ssid;
String eeprom_password;

//Prepheral setting
int hum;
int temp;
AsyncWebServer server(80);
unsigned long measureDelay = 2500;       //    NOT LESS THAN 2000!!!!!   
unsigned long lastTimeRan;

StaticJsonDocument<1024> jsonDocument;

char buffer[1024];
void create_json(char *tag, int value, char *unit) {  
  jsonDocument.clear();
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["value"] = unit;
  serializeJson(jsonDocument, buffer);  
}
 
void Read_sensor()
{
    hum = dht.getHumidity();
    temp = dht.getTemperature();
   delay(40);
}
void addJsonObject(char *name, int value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["name"] = name;
  obj["value"] = value;
  obj["unit"] = unit; 
}

void setLEDState(AsyncWebServerRequest *request){
  Serial.println("APT POST LED State is called");
  if (request->hasHeader("token")){
    String token = request->getHeader("token")->value();
    if(serverToken == token){
      Serial.printf("getting token from user is %s\n",token);
      
    
      if (request->hasParam("LEDstate")){
        String requestedLEDState = request->getParam("LEDstate")->value();
        Serial.printf("Request set LED state to %s\n",requestedLEDState);
        if(requestedLEDState == "1"){
          digitalWrite(18,HIGH);
        }
        else{
          if(requestedLEDState == "2"){
            digitalWrite(18,LOW);
          }
        }
      }
if (request->hasParam("LED1state")){
        String requestedLED1State = request->getParam("LED1state")->value();
        Serial.printf("Request set LED state to %s\n",requestedLED1State);
        if(requestedLED1State == "1"){
          digitalWrite(5,HIGH);
        }
        else{
          if(requestedLED1State == "2"){
            digitalWrite(5,LOW);
          }
        }
      }
      int LED_pin_value = digitalRead(LED_1);
      Serial.println(LED_pin_value);
      if (LED_pin_value == 0)
        addJsonObject("LED_1", LED_pin_value, "Off");
      if (LED_pin_value == 1)
        addJsonObject("LED_1", LED_pin_value, "On");
      
      request->send(200, "application/json", buffer);

    }
    else{
      Serial.println("Token is wrong");
    }
  }
  else{
    Serial.println("There is no token");
  }
}




void getData(AsyncWebServerRequest *request) {
  Serial.println("APT Get LED State is called");
  
  if (request->hasHeader("token")){
    String token = request->getHeader("token")->value();
    if(serverToken == token){
      Serial.printf("getting token from user is %s\n",token);
      Read_sensor();
      int LED_1_value = digitalRead(LED_1);
      Serial.println(LED_1_value);
      Serial.println(hum);
      Serial.println(temp);
      addJsonObject("temp", temp ,"°C");
      addJsonObject("hum", hum, "%");
      if (LED_1_value == 0) addJsonObject("LED_1",LED_1_value,"Off");
      if (LED_1_value == 1) addJsonObject("LED_1",LED_1_value, "On");
       serializeJson(jsonDocument, buffer);
      request->send(200, "application/json", buffer);
    }
    else{
      Serial.println("Token is wrong");
    }
  }
  else{
    Serial.println("There is no token");
  }
}


void setupApi() {
  server.on("/getData",HTTP_GET, getData);
  server.on("/setData", HTTP_POST, setLEDState);
  // start server
  server.begin();
}
void try_to_connect_wifi(String eeprom_ssid, String eeprom_password, int timeout = 20){
  Serial.printf("Try to login with recent ssid:%s and password:%S \n",eeprom_ssid,eeprom_password);
  status = WiFi.begin(eeprom_ssid, eeprom_password);
  // wait 20 seconds for connection:
  for(int wait_count = 0; wait_count <= timeout;wait_count++){
    delay(1000);
    Serial.print(".");
    status = WiFi.status();
    Serial.print(status);
    if(status == WL_CONNECTED) {
      Serial.println(" Connected successfully");
      Serial.println(WiFi.SSID());
      Serial.printf("Token: %s\n",serverToken);
      Serial.println("RSSI:");
      Serial.println(WiFi.RSSI());
      Serial.println(WiFi.localIP());
      Serial.println(WiFi.localIPv6());
      break;
    }
    if (wait_count == timeout){
      Serial.println(" Connection failed");
    }
  }
}




void setup() {
 delay(3);
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
  }

#ifdef MANUAL_INPUT
  // read inform from eeprom
  EEPROM.begin(eeprom_size);

  // EEPROM.writeString(eeprom_ssid_address,"AAAA");
  // EEPROM.writeString(eeprom_password_address,"AAAA");
  // EEPROM.commit();

  int eeprom_ssid_length = EEPROM.readByte(eeprom_ssid_length_address);
  eeprom_ssid = EEPROM.readString(eeprom_ssid_address);
  eeprom_password = EEPROM.readString(eeprom_password_address);
  serverToken = EEPROM.readString(eeprom_token_address);

  
  try_to_connect_wifi(eeprom_ssid, eeprom_password);
  
#endif

  if(status != WL_CONNECTED){
    while (status != WL_CONNECTED) {
#ifdef MANUAL_INPUT
      Serial.println("Enter your ssid");
      while (Serial.available() == 0) {}
          str_ssid = Serial.readString();
          str_ssid.trim();
          Serial.print("SSID entered: ");
          Serial.println(str_ssid);
      
      Serial.println("Enter your password");
      while (Serial.available() == 0) {}
      str_pass = Serial.readString();
      str_pass.trim();
          if (str_pass.length() != 0) { // user has entered data
              while (str_pass.length() <8 ) { // to catch pwd<8 exception
                  Serial.println("Password cannot be less than 8 characters! Try again");
                  while (Serial.available() == 0) {}
                  str_pass = Serial.readString();
                  str_pass.trim();
              }
                  Serial.print("Password entered: ");
                  Serial.println(str_pass);
          }
      Serial.println("Enter your token for Get and POST API");
      while (Serial.available() == 0) {}
          serverToken = Serial.readString();
          serverToken.trim();
          Serial.print("Token entered: ");
          Serial.println(serverToken);
      
      Serial.println("Enter your password");
#endif
      Serial.print("Attempting to connect to WPA SSID: ");


#ifndef MANUAL_INPUT
      Serial.println(ssid);
      // Connect to WPA/WPA2 network:
      status = WiFi.begin(ssid, password);
#else
      char ssid_cust[str_ssid.length() + 1];
      char pass_cust[str_pass.length() + 1];
      strcpy(ssid_cust, str_ssid.c_str());
      strcpy(pass_cust, str_pass.c_str());
      Serial.println(str_ssid.c_str());
      status = WiFi.begin(ssid_cust, pass_cust);
      str_ssid = str_pass = "";
#endif
      // wait 10 seconds for connection:
      for(int wait_count = 0;wait_count <=20; wait_count++){
        delay(1000);
        Serial.print(".");
        status = WiFi.status();
        if(status == WL_CONNECTED) {
#ifdef MANUAL_INPUT
          Serial.printf("Save ssid:%s and password:%s",ssid_cust,pass_cust);
          EEPROM.writeString(eeprom_ssid_address,ssid_cust);
          EEPROM.writeString(eeprom_password_address,pass_cust);
          EEPROM.writeString(eeprom_token_address,serverToken);
          EEPROM.commit();
#endif
          break;
        }
      }
    }
  }
  
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.SSID());
  Serial.printf("Token: %s\n",serverToken);
  Serial.println("RSSI:");
  Serial.println(WiFi.RSSI());
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.localIPv6());
  // Handle API requests

  dht.setup(DHT_PIN, DHTesp::DHT11);
  pinMode(DHT_PIN,INPUT);
  pinMode(LED_1,OUTPUT);
  pinMode(LED_2,OUTPUT);
  digitalWrite(LED_1, HIGH); 
   digitalWrite(LED_2, HIGH); 
  // Start server
  setupApi();
}
uint32_t time_counter = 0;
void loop() {
  // put your main code here, to run repeatedly:
  Read_sensor();
delay(1000);
  int wifi_status = WiFi.status();
  switch (wifi_status)
  {
  case WL_IDLE_STATUS:
    printf("Wifi is in idle status\n");
    break;

  case WL_NO_SSID_AVAIL:
    printf("The connected wifi is not available\n");
    try_to_connect_wifi(
      eeprom_ssid,
      eeprom_password,
      20
      );
    break;

  case WL_SCAN_COMPLETED:
    printf("Wifi scan is completed \n");
    break;

  case WL_CONNECTED:
    // printf("Wifi is connected\n");
    break;

  case WL_CONNECT_FAILED:
    printf("Wifi connection is failed\n");
    try_to_connect_wifi(
      eeprom_ssid,
      eeprom_password,
      20
      );
    break;
  case WL_CONNECTION_LOST:
    printf("Wifi connection is lost\n");
    try_to_connect_wifi(
      eeprom_ssid,
      eeprom_password,
      20
      );
    break;

  case WL_DISCONNECTED:
    printf("Wifi is disconnected \n");
    try_to_connect_wifi(
      eeprom_ssid,
      eeprom_password,
      20
      );
    break;
  
  default:
    break;
  }
}