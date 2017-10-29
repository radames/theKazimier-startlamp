#include "parameters.h"
#include "WifiPass.h"
#include <ESP8266WiFi.h>
#include <NTPClient.h> //NTPClient by Arduino
#include <WiFiUdp.h>
#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h> //RTClib by Adafruit
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#include "Scheduler.h"
#include "AudioAnalysis.h"
#include "MP3Player.h"

SoftwareSerial mySoftwareSerial(14, 12, false, 256); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

Scheduler mScheduler;
AudioAnalysis mAudio;
MP3Player tracks[AUDIO_TRACKS];


char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

RTC_DS3231 rtc;
WiFiUDP ntpUDP;

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
NTPClient timeClient(ntpUDP, NTP_SERVER, GMT_TIME_ZONE * 3600 , 60000);

int timeUpdated = 0;
long lastPrintTime = 0;

enum EventState { AMBIENT, EVENT1, EVENT2 };
EventState nState = AMBIENT;

void setup() {
  Serial.begin(9600);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  //DFPlayer Startup
  mySoftwareSerial.begin(9600);

  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini Demo"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  Serial.print("\n\n");
  if (!myDFPlayer.begin(mySoftwareSerial)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while (true);
  }
  Serial.println(F("DFPlayer Mini online."));
  Serial.print("\n\n");

  Serial.println("Waiting to Start....");
  delay(5000);

  syncTime();

  mAudio.initAudioAnalisys();
  myDFPlayer.volume(MP3_VOLUME);  //Set volume value. From 0 to 30
  for (int i = 0; i < AUDIO_TRACKS; i++) {
    tracks[i].start(myDFPlayer);
  }
  mScheduler.setStart("16:00:00");
  mScheduler.setEnd("22:00:00");
  // mScheduler.setEvent(1,"16:30:00", "21:30:00", "00:30:00");
  //mScheduler.setEvent(2,"16:00:00", "22:00:00", "01:00:00");

}


void syncTime(void) {

  //Connect to Wifi
  //SSID and Password on WifiPass.h file
  Serial.println("Checking WIFI...");
  Serial.print("\n\n");

  WiFi.begin(ssid, password);
  delay(10000); //wait 10 secods for connection

  switch (WiFi.status()) {
    case WL_CONNECTED:
      {
        Serial.println("Wifi Connected...");
        Serial.println("Syncing NTP clock");
        timeClient.begin();
        timeClient.update();
        timeClient.update();
        delay(1000);
        Serial.print("Time before sync: ");
        logDateTime();

        long actualTime = timeClient.getEpochTime();
        Serial.print("Internet Epoch Time: ");
        Serial.println(actualTime);
        rtc.adjust(DateTime(actualTime));
        Serial.println("RTC Synced with NTP time");

        Serial.print("Time after sync: ");
        logDateTime();
      }
      break;

    case WL_NO_SSID_AVAIL:
      {
        Serial.println("No Wifi SSID detected...");
        Serial.println("Time not synced..");
      }
      break;
  }
  Serial.print("\n\n");

  //Turn Off WIFI
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();

}

void loop () {

  if (millis() - lastPrintTime > 500) { //po
    lastPrintTime = millis();

    DateTime now = rtc.now();
    logDateTime();
    Serial.println();
  }

  switch (nState) {
    case AMBIENT:
      ambientIdle();
      break;
    case EVENT1:
      break;
    case EVENT2:
      break;
  }

  //audio();
}


void ambientIdle(void) {
  if (!tracks[0].isPlaying()) {
    tracks[0].loop();
  }
  ledOSC();
}


void ledOSC() {
    //sine wave led pattern 
    float t = (float)millis()/LED_OSC_PERIOD;
    int pValue = 0.5*OUT_MAX*(1 + sin(2.0*PI*t));
    analogWrite(LED_PIN, pValue);
}

void event1(void) {
  //1 min
  tracks[1].play();  //track 2 (Bell Sound 1)
  int input = analogRead(MIC_PIN);
  unsigned int out = mAudio.analysis(input);
  analogWrite(LED_PIN, out);
}

void event2(void) {
  //2min
  tracks[2].play();  //track 2 (Bell Sound 2)
  int input = analogRead(MIC_PIN);
  unsigned int out = mAudio.analysis(input);
  analogWrite(LED_PIN, out);
}

void logDateTime(void) {
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC); Serial.print('/');
  Serial.print(now.month(), DEC); Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print("  ");
  Serial.print(now.hour(), DEC); Serial.print(':');
  Serial.print(now.minute(), DEC); Serial.print(':');
  Serial.print(now.second(), DEC); Serial.println();
}

