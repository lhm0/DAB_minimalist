// =========================================================================================================================================
//                                                 Minimalist DAB+ radio
//                                                   © Ludwin Monz 2024
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include "myDAB.h"
#include "FileFunctions.h"
#include <LittleFS.h>

bool myDAB::_dabmode = true;
DAB myDAB::_Dab; 
int myDAB::_numberOfStations = 0;
String myDAB::_lastServiceData = "";
String myDAB::sInfo = "";

// =========================================================================================================================================
//                                                      Constructor
// =========================================================================================================================================

myDAB::myDAB() {
}

// =========================================================================================================================================
//                                                      begin Method:
// =========================================================================================================================================

void myDAB::begin() {
 
 //Enable SPI
  pinMode(slaveSelectPin, OUTPUT);
  digitalWrite(slaveSelectPin, HIGH);
  SPI.begin();

  Serial.print(F("Initialising DAB ==> ")); 

  //DAB Setup
  _Dab.setCallback(_ServiceData);
  _Dab.begin();

  if(_Dab.error != 0)
  {
    Serial.print(F("ERROR: "));
    Serial.print(_Dab.error);
    Serial.print(F("\nCheck DABShield is Connected and SPI Communications\n"));
  }
  else  
  {
    Serial.println("DABShield found"); 
  }
}

int myDAB::initStationList(){
  // read stationList
  if (LittleFS.exists("/stationList.json")) {
    _numberOfStations = readStationList();
    Serial.println("stationList read from file. Number of entries: "+String(_numberOfStations));
  }
  if ((!LittleFS.exists("/stationList.json"))||(_numberOfStations==0)) {
    Serial.println("No stationList found. Start scan.");
    _numberOfStations = scan();
    Serial.println("scan compete. Number of stations: " + String(_numberOfStations));
    if (saveStationList()) Serial.println("stationList saved successfully");
    else Serial.println("failed to save stationList");
  }
  return _numberOfStations;
}

void myDAB::task() {
  _Dab.task();
}

int myDAB::scan(){
  uint8_t freq_index;
  char freqstring[32];
  int stationIndex = 0;

  Serial.println("Starting scan....");
  Serial.println("===> number of DABfrequs = " + String(DAB_FREQS) + " <====");

  for (freq_index = 0; freq_index < DAB_FREQS; freq_index++)
  {
    Serial.print(".");
    _Dab.tune(freq_index);
    if(_Dab.servicevalid() == true)
    {

      for (int i = 0; i < _Dab.numberofservices; i++)
      {
        char channelInfo[128];

        _Dab.status(_Dab.service[i].ServiceID, _Dab.service[i].CompID);
        if(_Dab.type == SERVICE_AUDIO)
        {
          stationList[stationIndex].number = stationIndex;
          stationList[stationIndex].name = _Dab.service[i].Label;
          stationList[stationIndex].ensemble = (int)freq_index;
          stationList[stationIndex].serviceId = _Dab.service[i].ServiceID;
          stationList[stationIndex].compId = _Dab.service[i].CompID;
          stationIndex++;
        }
      }
    }
  }
  Serial.println("end of scan.");
  return stationIndex;
}

void myDAB::tune(int id) {
  uint8_t freq = (uint8_t)stationList[id].ensemble;
  uint32_t ServiceId = stationList[id].serviceId;
  uint32_t CompId = stationList[id].compId;

  _Dab.tuneservice(freq, ServiceId, CompId);
}

void myDAB::vol(uint8_t vol) {
  _Dab.vol(vol);
}

String myDAB::stationListJson() {
    // Create a static JSON document object.
    JsonDocument doc;
    JsonArray stationArray = doc.to<JsonArray>();

    // Loop through the stationList array and add each station to the JSON array
    for (int i = 0; i < _numberOfStations; i++) {
        JsonObject stationObj = stationArray.add<JsonObject>();
        stationObj["number"] = stationList[i].number;
        stationObj["name"] = stationList[i].name;
        stationObj["ensemble"] = stationList[i].ensemble;
        stationObj["serviceId"] = stationList[i].serviceId;
        stationObj["compId"] = stationList[i].compId;
    }

    // Serialize the JSON document into a string
    String output;
    serializeJson(doc, output);

    return output;
}

int myDAB::numberOfStations() {
  return _numberOfStations;
}   

bool myDAB::saveStationList() {
    String jsonString = stationListJson();
    return writeFile(LittleFS, "/stationList.json", jsonString.c_str());
}

int myDAB::readStationList() {
    File file = LittleFS.open("/stationList.json", FILE_READ);

    if (!file) {
        Serial.println("Failed to open /stationList.json");
        return 0; // return 0 on error
    }

    String jsonString;
    while (file.available()) {
        jsonString += file.readString();
    }
    file.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    // check if deserialization successful
    if (error) {
        Serial.println("Failed to deserialize JSON");
        return 0; // return 0 on error
    }

    JsonArray stationArray = doc.as<JsonArray>();

    int i = 0;
    for (JsonObject stationObj : stationArray) {
        stationList[i].number = stationObj["number"];
        stationList[i].name = stationObj["name"].as<String>();
        stationList[i].ensemble = stationObj["ensemble"];
        stationList[i].serviceId = stationObj["serviceId"];
        stationList[i].compId = stationObj["compId"];
        i++;
    }

    _numberOfStations = i;
    return _numberOfStations;
}

void myDAB::printStationTable() {

  Serial.println(F("----------------------------------------------------------"));
  Serial.println(F("| Number |    Station Name    | Ensemble |   ServiceId  |   CompId  |"));
  Serial.println(F("----------------------------------------------------------"));

  for (int i=0; i<_numberOfStations; i++) {
    // Formatierte Ausgabe
    Serial.printf("|   %-5d| %-18s|   %-7d|   %-10d|   %-10d|\n", stationList[i].number, stationList[i].name.c_str(), stationList[i].ensemble, stationList[i].serviceId, stationList[i].compId);
  }

  Serial.println(F("----------------------------------------------------------"));
}

void myDAB::initPresets(){
  // read presets
  bool success;
  if (LittleFS.exists("/presets.json")) {
    success = readPresets();
    Serial.println("presets read from file.");
  }
  if ((!LittleFS.exists("/presets.json"))||(!success)) {
    Serial.println("No presets found. Use first 6 entries of stationList")  ;
    if (_numberOfStations>5) {
      for (int i = 0; i<6; i++) {
        presets[i].number=i;
        presets[i].name = stationList[i].name;
      }
    }
    else 
    {
      Serial.println("No stationList");
    }
  }
}

String myDAB::presetsJson() {
    // Create a static JSON document object.
    JsonDocument doc;
    JsonArray presetsArray = doc.to<JsonArray>();

    // Loop through the stationList array and add each station to the JSON array
    for (int i = 0; i < 6; i++) {
        JsonObject stationObj = presetsArray.add<JsonObject>();
        stationObj["number"] = presets[i].number;
        stationObj["name"] = presets[i].name;
    }

    // Serialize the JSON document into a string
    String output;
    serializeJson(doc, output);

    return output;
}

bool myDAB::savePresets() {
    String jsonString = presetsJson();
    return writeFile(LittleFS, "/presets.json", jsonString.c_str());
}

bool myDAB::readPresets() {
    File file = LittleFS.open("/presets.json", FILE_READ);

    if (!file) {
        Serial.println("Failed to open /stationList.json");
        return false; // return false on error
    }

    String jsonString;
    while (file.available()) {
        jsonString += file.readString();
    }
    file.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    // check if deserialization successful
    if (error) {
        Serial.println("Failed to deserialize JSON");
        return false; // return false on error
    }

    JsonArray presetsArray = doc.as<JsonArray>();

    int i = 0;
    for (JsonObject presetsObj : presetsArray) {
        presets[i].number = presetsObj["number"];
        presets[i].name = presetsObj["name"].as<String>();
        i++;
    }
    return true;
}

void myDAB::printPresetsTable() {

  Serial.println(F("---------------------------------------"));
  Serial.println(F("| Preset | Number |    Station Name    |"));
  Serial.println(F("----------------------------------------"));

  for (int i=0; i<6; i++) {
    // Formatierte Ausgabe
    Serial.printf("|   %-5d| %-5d|   %-18s|\n", i+1, presets[i].number, presets[i].name.c_str());
  }

  Serial.println(F("----------------------------------------------------------"));
}


void myDAB::_ServiceData()
{
  if ((_dabmode == true)&&(String(_Dab.ServiceData) != _lastServiceData)) {
    _lastServiceData = _Dab.ServiceData;
    sInfo = _lastServiceData;
    Serial.print(sInfo);
    Serial.print(F("\n"));
  }
}

