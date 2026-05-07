#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#include "secrets.h"

#include <FS.h>
#include <LittleFS.h>
#include <FFat.h>

WebServer server(80);
void setup() {
  Serial.begin(115200);

  if(!LittleFS.begin(true)){
    Serial.println("Error occured during LittleFS init");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    File file = LittleFS.open("/index.html", "r");
    if(!file){
      server.send(500, "text/plain", "File not found!");
      return;
    }
    
    server.streamFile(file, "text/html");
    file.close();
  });

  server.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
}
