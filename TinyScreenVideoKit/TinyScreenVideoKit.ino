
//-------------------------------------------------------------------------------
//  TinyCircuits Tiny Arcade Video Player
//
//  Changelog:
//  1.0.4 24 Jan 2020 - Added support for three screens playing videos in order
//           Removed everything with buttons. This is meant to just run without
//           without interaction
//  1.0.3 15 Nov 3018 - Add option for TinyScreen+ button input
//  1.0.2 10 April 2017 - Cleanup, add messages about SD card/video detection
//  1.0.1 3 October 2016 - Add compatibility for new Arcade revision, change
//        folder structure for IDE compatibility
//  1.0.0 Initial release
//
//  Written by Ben Rose for TinyCircuits, http://TinyCircuits.com
//
//-------------------------------------------------------------------------------

#include <TinyScreen.h>
#include "SdFat.h"
#include <Wire.h>
#include <SPI.h>

TinyScreen display = TinyScreen(TinyScreenPlus);
TinyScreen display1 = TinyScreen(TinyScreenDefault);
TinyScreen display2 = TinyScreen(TinyScreenAlternate);

#if defined (ARDUINO_ARCH_AVR)
#define SerialMonitorInterface Serial
#elif defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#endif

SdFat sd;
SdFile dir;
SdFile vidFile;
uint8_t buffer[96 * 64 * 2];
uint16_t audioBuffer[1024];
volatile uint32_t sampleIndex = 0;

int currentFileNum = 0;
int currentScreen = -1;
int doVideo = 0;
int foundValidVideo = 0;
char currentVideoName[50] = "No video found";

void setup(void) {
  SerialMonitorInterface.begin(9600);
  Wire.begin();
  display.begin();
  display.setFlip(true);

  display1.begin();
  display1.setFlip(true);
  display2.begin();
  
  display.setBitDepth(1);
  display.setFont(thinPixel7_10ptFontInfo);

  display1.setBitDepth(1);
  display1.setFont(thinPixel7_10ptFontInfo);

  display2.setBitDepth(1);
  display2.setFont(thinPixel7_10ptFontInfo);

  SPI.begin();
  if (!sd.begin(10, SPI_FULL_SPEED)) {
    int xPosition = 48 - ((int)display.getPrintWidth("Card not found!") / 2);
    display.setCursor(xPosition, 26);
    display.print("Card not found!");
    while (1);
  }
  SPI.setClockDivider(0);

  printNextFile();
}
int blockInput = 1;

void loop() {
  if (doVideo) {
    bufferVideoFrame();
    writeToDisplay();
  }
}

void bufferVideoFrame() {
  if (!vidFile.available())
    {
      vidFile.rewind();
      printNextFile();
    }
  vidFile.read(buffer, 96 * 64 * 2);
  vidFile.read((uint8_t*)audioBuffer, 1024 * 2);
}

void writeToDisplay() {
  switch (currentScreen) {
    case 1:
      display1.goTo(0, 0);
      display1.startData();
      display1.writeBuffer(buffer, 96 * 64 * 2);
      display1.endTransfer();
      break;
    case 2:
      display2.goTo(0, 0);
      display2.startData();
      display2.writeBuffer(buffer, 96 * 64 * 2);
      display2.endTransfer();
      break;
    default:
      display.goTo(0, 0);
      display.startData();
      display.writeBuffer(buffer, 96 * 64 * 2);
      display.endTransfer();
      break;
  }
}

void printNextFile() {
  doVideo = 0;
  vidFile.close();
  int foundVideo = 0;
  while (!foundVideo) {
    dir.close();
    if (dir.openNext(sd.vwd(), O_READ)) {
      currentFileNum++;
    } else if (!foundValidVideo) {
      //This should mean there are no valid videos or SD card problem
      int xPosition = 48 - ((int)display.getPrintWidth("No video files!") / 2);
      display.setCursor(xPosition, 26 - 10);
      display.print("No video files!");
      xPosition = 48 - ((int)display.getPrintWidth("Place .TSV files") / 2);
      display.setCursor(xPosition, 26);
      display.print("Place .TSV files");
      xPosition = 48 - ((int)display.getPrintWidth("in main directory.") / 2);
      display.setCursor(xPosition, 26 + 10);
      display.print("in main directory.");
      while (1);
    } else {
      sd.vwd()->rewind();
      dir.openNext(sd.vwd(), O_READ);
      currentFileNum = 1;
    }
    if (dir.isFile()) {
      char fileName[100];
      memset(fileName, 0, 100);
      dir.getName(fileName, 50);
      if (!strcmp(fileName + strlen(fileName) - 4, ".tsv")) {
        dir.close();
        if (vidFile.open(fileName, O_READ)) {
          doVideo = 1;
          foundValidVideo = 1;
          foundVideo = 1;

          String fName = String(fileName);
          int screenIndex = fName.indexOf('_');
          SerialMonitorInterface.print("ScreenIndex: ");
          SerialMonitorInterface.println(screenIndex);
          
          if (screenIndex > 0)
          {
            int screen = fName.substring(screenIndex + 1).toInt();            
            SerialMonitorInterface.print("Screen number:");
            SerialMonitorInterface.println(screen);
        
            if (screen >= 0 && screen < 3)
              currentScreen = screen;
          }
        }
      }
    }
  }
  vidFile.getName(currentVideoName, 50);
}
