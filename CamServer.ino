#include "FS.h"
#include "SD_MMC.h"


void setup() {
  Serial.begin(115200);

  if(!SD_MMC.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  else{
    Serial.println("Card Mount Succeeded");
  }

  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }
  else{
    Serial.println("Card found!");
  }

  const char* path = "/HOMEWORK";
  const char* fileName = "/HOMEWORK/test.gcode";

  if(!SD_MMC.exists(path)){
    if(SD_MMC.mkdir(path)){
      Serial.println("Directory created");
    }else{
      Serial.println("mkdir failed");
      return;
    }
  }
  else{
    Serial.println("Path found!");
  }
  File file = SD_MMC.open(fileName, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  else{
    Serial.println("File created and opened");
  }

  if(file.print("Hello from ESP32-CAM!")){
    Serial.println("File written succesfully");
  }
  else{
    Serial.println("Write failed");
  }

  file.close();
}

void loop() {

}
