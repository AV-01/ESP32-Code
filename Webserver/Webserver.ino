// necessary to access wifi
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include <Preferences.h>

// for displaying the HTML files. Note: could remove and just store all data here
#include <FS.h>
#include <LittleFS.h>

// for QR code display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "qrcode.h"

#define SDA_PIN 8
#define SCL_PIN 9
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);

Preferences preferences;
bool hasWifi = true;

// std::vector<String> displayMessages(5);

// void addDisplayMessages(String msg){
//   displayMessages.push_back(msg);
//   if(displayMessages.size() > 5){
//     displayMessages.erase(displayMessages.begin());
//   }

//   display.clearDisplay();
  // display.setTextSize(1);
  // display.setTextColor(WHITE);

//   for(int i = 0; i < displayMessages.size(); i ++){
//     display.setCursor(70, i * 12);
//     display.print(displayMessages[i]);
//   }
//   display.display();
// }

void drawCenterString(const char *buf, int x, int y){
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
    display.setCursor(x - w / 2, y);
    display.print(buf);
}

void displayQRCode(esp_qrcode_handle_t qrcode) {
  int size = esp_qrcode_get_size(qrcode);
  int scale = 2; // Scale up the pixels so it is readable
  int xOffset = (SCREEN_WIDTH - (size * scale)) / 2 - 35; // Center horizontally
  int yOffset = 9 + (SCREEN_HEIGHT - (size * scale)) / 2; // Center vertically

  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      // esp_qrcode_get_module returns true for a black pixel
      if (esp_qrcode_get_module(qrcode, x, y)) {
        display.fillRect(xOffset + (x * scale), yOffset + (y * scale), scale, scale, WHITE);
      }
    }
  }
  display.display();
}
void setup() {
  Serial.begin(115200);

  // Set up Display
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  drawCenterString("Booting up...", 64, 32);
  display.display();

  if(!LittleFS.begin(true)){
    Serial.println("Error occured during LittleFS init");
    return;
  }
  // Try and load WiFi
  preferences.begin("my-app", false); 
  String ssid = preferences.getString("ssid", "");
  String password = preferences.getString("password","");
  Serial.print("SSID: ");
  Serial.println(ssid);
  if(ssid == ""){
    Serial.println("No SSID found. Entering AP mode.");
    hasWifi = false;
  }
  else{
    // LOAD WIFI SEQUENCE
    Serial.println("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid.c_str(), password.c_str());
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
      delay(500);
      Serial.print(".");
      retries++;
    }

    Serial.println("");
    if(WiFi.status() == WL_CONNECTED){
      Serial.println("Connected! IP address: ");
      Serial.println(WiFi.localIP());

      display.clearDisplay();

      esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
      cfg.display_func = displayQRCode;
      String ipStr = "http://" + WiFi.localIP().toString();
      // Display title
      display.setTextSize(1);
      display.setTextColor(WHITE);

      drawCenterString(ipStr.c_str(), 64, 8);

      display.setCursor(55 , 16);
      // one line fits "Hello World"
      // "Go to link"
      // "Hello World"
      display.print("1. Join WiFi");
      display.setCursor(55 , 26);
      display.print("2. Scan QR");
      display.setCursor(55 , 36);
      display.print("3. Do form");
      display.setCursor(55 , 46);
      display.print("4. HW done!");

      esp_qrcode_generate(&cfg, ipStr.c_str());
    }
    else{
      Serial.println("Failed to connect. Converting to Access Point...");
      hasWifi = false;

      WiFi.mode(WIFI_AP);
      WiFi.softAP("ESP32-SETUP");
      Serial.print("AP IP address: ");
      Serial.println(WiFi.softAPIP());

      display.clearDisplay();
      esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
      cfg.display_func = displayQRCode;
      String ipStr = "http://" + WiFi.softAPIP().toString();
      // Display title
      display.setTextSize(1);
      display.setTextColor(WHITE);
      drawCenterString(ipStr.c_str(), 64, 8);

      display.setCursor(55 , 16);
      // one line fits "Hello World"
      // "Go to link"
      // "Hello World"
      display.print("1. Join WiFi");
      display.setCursor(55 , 26);
      display.print("ESP32-SETUP");
      display.setCursor(55 , 36);
      display.print("2. Scan QR");
      display.setCursor(55 , 46);
      display.print("3. Do form");
      display.setCursor(55 , 56);
      display.print("4. Restart!");

      esp_qrcode_generate(&cfg, ipStr.c_str());
    }
  }

  // if(hasWifi == false){
  //   WiFi.mode(WIFI_AP);
  //   WiFi.softAP("ESP32-SETUP");
  //   Serial.print("AP IP address: ");
  //   Serial.println(WiFi.softAPIP());
  // }

  server.on("/", []() {
      server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
      File file = LittleFS.open((hasWifi) ? "/index.html" : "/setup.html", "r");
      if(!file){
        server.send(500, "text/plain", "File not found!");
        return;
      }
      server.streamFile(file, "text/html");
      file.close();

      if(hasWifi){
        Serial.println("Accessed index.html");
      }
      else{
        Serial.println("Accessed setup.html");
      }
  });

  server.on("/setup", []{
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    File file = LittleFS.open("/setup.html", "r");
    server.streamFile(file, "text/html");
    file.close();
    Serial.println("Accessed setup.html");
  });
  server.on("/save-config", []{
      if(server.hasArg("wifi_name") && server.hasArg("api_key") && server.hasArg("wifi_password")){
        preferences.putString("ssid", server.arg("wifi_name"));
        preferences.putString("password", server.arg("wifi_password"));
        preferences.putString("api_key", server.arg("api_key"));

        Serial.println("Success! Recieved SSID, API Key, and Password. Restarting...");
        Serial.println(server.arg("wifi_name"));
        Serial.println(server.arg("wifi_password"));
        server.send(200, "text/plain", "Success! Restarting ESP32");
        delay(2000);
        ESP.restart();
      }
      else{
        Serial.println("Missing fields");
        server.send(400, "text/plain", "Missing fields");
      }
  });

  server.on("/get-api-key", []{
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    String api_key = preferences.getString("api_key", "");
    if(hasWifi && api_key != ""){
      String json_result = "{ \"api_key\": \"" + api_key + "\"}";
      server.send(200, "application/json", json_result);
    }
    else{
      server.send(400, "text/plain", "Failed to fetch API Key, please set up WiFi and key.");
    }
  });

  server.on("/settings", []{
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    File file = LittleFS.open("/settings.html", "r");
    if(!file){
      server.send(400, "text/plain", "file not found");
      return;
    }
    server.streamFile(file, "text/html");
    file.close();
    Serial.println("Accessed settings.html");
  });

  server.on("/save-settings", HTTP_POST, []{
    String setting_objects[13] = {"X_MIN", "X_MAX", "Y_MIN", "Y_MAX", "Z_SAFE", "Z_DRAW", "F_TRAVEL", "F_DRAW", "F_Z", "LINE_SPACING", "TOP_MARGIN", "FLOAT_OFFSET", "LEFT_MARGIN"};
    String json = "{\n";
    for(String i : setting_objects){
      if(!server.hasArg(i)){
        server.send(400, "text/plain", "missing argument: " + i);
        return;
      }
      json += "\"" + i + "\": " + server.arg(i) + ",\n";
    }
    
    // json = json.substr(0, json.size()-1);
    json.remove(json.length() - 1);
    json += "}";
    File file = LittleFS.open("/fonts/settings.json", "w");
    if(!file){
      server.send(400, "text/plain", "failed to open settings file");
      return;
    }

    file.print(json);
    file.close();
    Serial.println("Updated settings.json");
    server.send(200, "text/plain", "updated settings.json");
  });

  server.serveStatic("/fonts/", LittleFS, "/fonts/");

  server.serveStatic("/font_engine.js", LittleFS, "/font_engine.js");
  
  server.serveStatic("/initialize_gcode.txt", LittleFS, "/initialize_gcode.txt");
  server.serveStatic("/end_gcode.txt", LittleFS, "/end_gcode.txt");

  server.enableCORS(true);
  server.begin();
}

void loop() {
  server.handleClient();
}
