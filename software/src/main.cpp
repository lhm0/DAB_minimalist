// =========================================================================================================================================
//                                                 Minimalist DAB+ radio
//                                                   © Ludwin Monz 2024
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include <Arduino.h>
#include <SPI.h>
#include <DABShield.h>
#include "LittleFS.h"
#include "FileFunctions.h"
#include "webInterface.h"
#include "myDAB.h"
#include <ArduinoJson.h>
#include <WiFi.h>

#define preset1 2
#define preset2 4
#define preset3 35
#define preset4 34
#define preset5 36
#define preset6 39

#define KY040_CLK 22 // 
#define KY040_DT 21 // 

bool connectWiFi();
void accessPoint();
String ssid, password;
webInterface wi;

myDAB my_DAB;
void DABSpiMsg(unsigned char *data, uint32_t len);
void ServiceData();

void init_preset();
int check_preset();
int activeStation=-1;
int activePreset=-1;

void init_KY040();
void IRAM_ATTR handleInterruptA(); 
void IRAM_ATTR handleInterruptB(); 
long eventA = 0;
int lastA = LOW;
int lastVol = 0;
int v_value=0;                   // Rotary encoder value (will be set in ISR)
long lastPrint;

void setup() {

  // intitialise the terminal
  Serial.begin(112500);
  while(!Serial);

  Serial.println("minimalist DAB+"); 
  Serial.println("===============");
  
  // initialize LittleFS
  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  } else {
    Serial.println("LittleFS mounted...");
  }

  listDir(LittleFS, "/", 15);
  
  // initialize DAB
  my_DAB.begin();
  my_DAB.initStationList();
  my_DAB.printStationTable();
  my_DAB.initPresets();
  my_DAB.printPresetsTable();

  // initialize preset buttons
  init_preset();

  // initialize encoder
  init_KY040(); 

  // initialize Wifi
  if (LittleFS.exists("/ssid.txt")) ssid = readFile(LittleFS, "/ssid.txt");
  if (LittleFS.exists("/password.txt")) password = readFile(LittleFS, "/password.txt");

  bool connected = false;
  if ((ssid!="")&&(password!="")) {
    Serial.println("connecting with WiFi..");
    connected = connectWiFi();
  }
  if (!connected) accessPoint();

  // initialize web server user interface
  wi.begin(ssid, password); 

  // restore volume and station settings
  if (LittleFS.exists("/volume.txt")) v_value = readFile(LittleFS, "/volume.txt").toInt();
  else v_value = 32;
  Serial.println("set volume to "+ (String)v_value);
  my_DAB.vol((uint8_t)v_value);

  if (LittleFS.exists("/station.txt")) activePreset = readFile(LittleFS, "/station.txt").toInt();
  activeStation = my_DAB.presets[activePreset-1].number;
  my_DAB.tune(activeStation);
  Serial.println("tune to preset "+(String)activePreset+" = Station "+(String)activeStation);
}

void loop() {
  // check preset buttons
  int button = check_preset();
  if ((button!=0)&&(button!=activePreset)) {
    activePreset = button;
    Serial.println("preset button " + String(button));
    activeStation = my_DAB.presets[button-1].number;
    my_DAB.tune(activeStation);
    
    char buffer[10];  
    sprintf(buffer, "%d", button);
    writeFile(LittleFS, "/station.txt", buffer);
    Serial.println("save preset: "+(String)button);
  }

  // check volume setting
  if (lastVol!=v_value) {
    lastVol = v_value;
    my_DAB.vol((uint8_t)v_value);
    
    char buffer[10];  
    sprintf(buffer, "%d", v_value);
    writeFile(LittleFS, "/volume.txt", buffer);
  }

  // service web server and DAB radio
  my_DAB.task();
  wi.update();
}

//////////////////////////////////////////////////////////
//
//  preset buttons and volume control
//
//////////////////////////////////////////////////////////

void init_preset(){
  pinMode(preset1, INPUT);
  pinMode(preset2, INPUT);
  pinMode(preset3, INPUT);
  pinMode(preset4, INPUT);
  pinMode(preset5, INPUT);
  pinMode(preset6, INPUT);
}

int check_preset() {
  if (digitalRead(preset1) == HIGH) {
    return 1;
  } else if (digitalRead(preset2) == HIGH) {
    return 2;
  } else if (digitalRead(preset3) == HIGH) {
    return 3;
  } else if (digitalRead(preset4) == HIGH) {
    return 4;
  } else if (digitalRead(preset5) == HIGH) {
    return 5;
  } else if (digitalRead(preset6) == HIGH) {
    return 6; 
  }
  return 0; // no key pressed
}


//////////////////////////////////////////////////////////
//
//  KY040 Functions
//
//////////////////////////////////////////////////////////

void init_KY040() {
  pinMode(KY040_CLK, INPUT_PULLUP);
  pinMode(KY040_DT, INPUT_PULLUP);
  attachInterrupt(KY040_CLK, handleInterruptA, CHANGE);
  attachInterrupt(KY040_DT, handleInterruptB, CHANGE);
}


// ISR to handle pin change interrupt for D0 to D7 here
void IRAM_ATTR handleInterruptA() {
  long sysTime = micros();
    while ((micros()-sysTime)<1000) {}          // delay 1ms
    int currentA = digitalRead(KY040_CLK);
    if (currentA != lastA) {
      int increment = 1;
      if (v_value<50) increment = 2;
      if (v_value<40) increment = 3;
      if (v_value<25) increment = 5;
      if (v_value<15) increment = 10;
      if (currentA==HIGH) {            // A change: 1 => 0
        if (digitalRead(KY040_DT)==LOW) {
          v_value=v_value-increment;                       
          if (v_value<=0) v_value=0;
        }
        else {
          v_value=v_value+increment;                        
          if (v_value>=63) v_value=63;
        }
      } else {                        // A change: 0 => 1
        if (digitalRead(KY040_DT)==LOW) {
          v_value=v_value+increment;                        
          if (v_value>=63) v_value=63;
        } else {
          v_value=v_value-increment;                        
          if (v_value<=0) v_value=0;
        }
      }
      lastA = currentA;   
    }
}

void IRAM_ATTR handleInterruptB() {
}

//////////////////////////////////////////////////////////
//
//  system functions
//
//////////////////////////////////////////////////////////

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Wait 20 seconds 
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 60000; 

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttemptTime >= timeout) {
      Serial.println("Failed to connect to Wi-Fi within timeout period.");
      return false;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected to Wi-Fi.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void accessPoint() {
  Serial.println("Setting up Access Point...");
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP("mini_DAB", NULL)) {
    Serial.println("Failed to start Access Point.");
    return;
  }

  IPAddress apIP(192, 168, 4, 1);
  if (!WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0))) {
    Serial.println("Failed to configure Access Point IP address.");
    return;
  }
  Serial.print("Access Point IP Address: ");
  Serial.println(WiFi.softAPIP());

  Serial.println("Access Point is active. Connect to the Wi-Fi network 'mini_DAB' without a password.");
}


//////////////////////////////////////////////////////////
//
//  SPI service function
//
//////////////////////////////////////////////////////////

void DABSpiMsg(unsigned char *data, uint32_t len)
{
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));    //2MHz for starters...
  digitalWrite (my_DAB.slaveSelectPin, LOW);
  SPI.transfer(data, len);
  digitalWrite (my_DAB.slaveSelectPin, HIGH);
  SPI.endTransaction();
}
