// necessary to access wifi
#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

// for getting wifi and api key
// #include "secrets.h"
// to replace hardcoded stuff
#include <Preferences.h>


// for displaying the HTML files. Note: could remove and just store all data here
#include <FS.h>
#include <LittleFS.h>
// #include <FFat.h>

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

void drawCentreString(const char *buf, int x, int y){
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
    display.setCursor(x - w / 2, y);
    display.print(buf);
}

void displayQRCode(esp_qrcode_handle_t qrcode) {
  int size = esp_qrcode_get_size(qrcode);
  int scale = 2; // Scale up the pixels so it is readable
  int xOffset = (SCREEN_WIDTH - (size * scale)) / 2; // Center horizontally
  int yOffset = 9 + (SCREEN_HEIGHT - (size * scale)) / 2; // Center vertically

  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      // esp_qrcode_get_module returns true for a black pixel
      if (esp_qrcode_get_module(qrcode, x, y)) {
        display.fillRect(xOffset + (x * scale), yOffset + (y * scale), scale, scale, WHITE);
      }
    }
  }

  // Display title
  display.setTextSize(1);
  display.setTextColor(WHITE);
  drawCentreString("Homework Machine", 64, 8);

  display.display();
}
void setup() {
  Serial.begin(115200);

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
  Serial.print("Password: ");
  Serial.println(password);
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
    }
    else{
      Serial.println("Failed to connect. Converting to Access Point...");
      hasWifi = false;
    }
  }


  // QR Code generation begins here
  // Wire.begin(SDA_PIN, SCL_PIN);
  // display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // display.clearDisplay();
  // esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  // cfg.display_func = displayQRCode;
  // String ipStr = "http://" + WiFi.localIP().toString();
  // esp_qrcode_generate(&cfg, ipStr.c_str());

  if(hasWifi == false){
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-SETUP");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  }

  server.on("/", []() {
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

  server.begin();
}

void loop() {
  server.handleClient();
}
