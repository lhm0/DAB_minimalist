// =========================================================================================================================================
//                                                 Minimalist DAB+ radio
//                                                   © Ludwin Monz 2024
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#include "webInterface.h"
#include "FileFunctions.h"
#include <LittleFS.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// =========================================================================================================================================
//                                                      Constructor
// =========================================================================================================================================

webInterface::webInterface() {
}

// =========================================================================================================================================
//                                                      begin Method:
// =========================================================================================================================================

void webInterface::begin(String ssid_, String password_) {
    _ssid = ssid_;
    _password = password_;

    _startServer();
}

void webInterface::update() {
  _server.handleClient();
}

// =========================================================================================================================================
//                                                        _startServer() and webSocket handlers
// =========================================================================================================================================

void webInterface::_startServer() {
  
  // ====================================================================================
  // index
  // ====================================================================================

  _server.on("/", [this]() {
    File file = LittleFS.open("/html/index.html", FILE_READ);
    String fileContent = "";
    while (file.available()) {
        fileContent += (char)file.read();
    }
    file.close();
    _server.send(200, "text/html", fileContent);
  });

  _server.on("/index.css", [this]() {
    File file = LittleFS.open("/html/css/index.css", FILE_READ);
    String fileContent = "";
    while (file.available()) {
        fileContent += (char)file.read();
    }
    file.close();
    _server.send(200, "text/css", fileContent);
  });

  _server.on("/index.js", [this]() {
    File file = LittleFS.open("/html/scripts/index.js", FILE_READ);
    String fileContent = "";
    while (file.available()) {
      fileContent += (char)file.read();
    }
    file.close();
    _server.send(200, "text/javascript", fileContent);
  });

  _server.on("/favicon.ico", [this]() {
    _server.send(200, "text/plain", "OK");
  });

  _server.on("/getParam", [this]() {
    JsonDocument doc; 
    doc["preset"] = _preset;
    doc["volume"] = _volume;

    JsonArray stationsArray = doc["stations"].to<JsonArray>();

    for (int i = 0; i < my_DAB.numberOfStations(); i++) {
      JsonObject stationObj = stationsArray.add<JsonObject>();
      stationObj["id"] = my_DAB.stationList[i].number;
      stationObj["name"] = my_DAB.stationList[i].name;
    }

    JsonArray presetsArray = doc["presets"].to<JsonArray>();

    for (int i = 0; i < 6; i++) {
      JsonObject presetsObj = presetsArray.add<JsonObject>();
      presetsObj["pnr"] = i+1;
      presetsObj["id"] = my_DAB.presets[i].number;
      presetsObj["name"] = my_DAB.presets[i].name;
    }

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
  });

  _server.on("/selectStation", [this]() {
    if (_server.hasArg("id")) {
        String stationId = _server.arg("id");  
        Serial.print("Selected station ID: ");
        Serial.println(stationId);  
        activeStation = stationId.toInt();
        my_DAB.tune(activeStation);
        activePreset = 0;
        for (int i=0; i<6; i++){
          if (my_DAB.presets[i].number==activeStation) activePreset = i+1;
        }
        _server.send(200, "text/plain", "Station selected");  
    } else {
        _server.send(400, "text/plain", "Station ID missing");  
    }
  });

  _server.on("/getInfo", [this]() {
    JsonDocument jsonDoc;

    jsonDoc["serviceInfo"] = my_DAB.sInfo; 
    jsonDoc["station"] =  activeStation;  
    jsonDoc["preset"] = activePreset;   
    jsonDoc["volume"] = v_value*100/63;

    String jsonResponse;
    serializeJson(jsonDoc, jsonResponse);

    _server.send(200, "application/json", jsonResponse);
  });

  _server.on("/savePreset", [this](){
    String button = "";
    String stationId = "";
    String stationName = "";

    if (_server.hasArg("button")) {
        button = _server.arg("button");
    }
    if (_server.hasArg("stationId")) {
        stationId = _server.arg("stationId");
    }
    if (_server.hasArg("stationName")) {
        stationName = _server.arg("stationName");
    }
    Serial.println("Button: " + button);
    Serial.println("Station ID: " + stationId);
    Serial.println("Station Name: " + stationName);

    my_DAB.presets[button.toInt()-1].number = stationId.toInt();
    my_DAB.presets[button.toInt()-1].name = stationName;
    my_DAB.savePresets();

    _server.send(200, "text/plain", "preset updated");

    activeStation = stationId.toInt();
    activePreset = button.toInt();
  });

  _server.on("/updateradiobutton", [this](){
    String button = "";

    if (_server.hasArg("button")) {
      button = _server.arg("button");

      int stationId = my_DAB.presets[button.toInt()-1].number;
      _server.send(200, "text/plain", String(stationId));
      my_DAB.tune(stationId);
      activeStation = stationId;
      activePreset = button.toInt();
      Serial.println(my_DAB.stationList[stationId].name + "wurde eingestellt");
    }
    else {
      _server.send(404, "text/plain", "error");
    }
  });

  _server.on("/slider", [this](){
    if (_server.hasArg("value")) { 
      String value = _server.arg("value"); 
      v_value = value.toInt()*63/100;
      _server.send(200, "text/plain", "volume set to " + value);
    }
  });

  _server.on("/scan", [this](){
    deleteFile(LittleFS, "/stationList.json");
    deleteFile(LittleFS, "/presets.json");

    my_DAB.initStationList();
    my_DAB.printStationTable();
    my_DAB.initPresets();
    my_DAB.printPresetsTable();

    _server.send(200, "text/plain", "scan complete");
  });

  // ====================================================================================
  // resetWifi
  // ====================================================================================

  _server.on("/resetWifi", [this]() {
    File file = LittleFS.open("/html/resetWifi.html", FILE_READ);
    String fileContent = "";
    while (file.available()) {
      fileContent += (char)file.read();
    }
    file.close();
    _server.send(200, "text/html", fileContent);
  });

  _server.on("/resetWifi.css", [this]() {
    File file = LittleFS.open("/html/css/resetWifi.css", FILE_READ);
    String fileContent = "";
    while (file.available()) {
      fileContent += (char)file.read();
    }
    file.close();
    _server.send(200, "text/css", fileContent);
  });

  _server.on("/resetWifi.js", [this]() {
    File file = LittleFS.open("/html/scripts/resetWifi.js", FILE_READ);
    String fileContent = "";
    while (file.available()) {
      fileContent += (char)file.read();
    }
    file.close();
    _server.send(200, "text/javascript", fileContent);  
  });

  _server.on("/reset", [this]() {
    _server.send(200, "text/plain", "OK");
    deleteFile(LittleFS, "/ssid.txt");
    deleteFile(LittleFS, "/password.txt");
  });

  _server.on("/getWifiParam", [this]() {
    JsonDocument doc;
    doc["ssid"] = readFile(LittleFS, "/ssid.txt");
    doc["password"] = readFile(LittleFS, "/password.txt");

    String response;
    serializeJson(doc, response);
    _server.send(200, "application/json", response);
  });

  _server.on("/uploadWifiParam", [this]() {
    if ((_server.hasArg("value1"))&&(_server.hasArg("value2"))){ 
        String _ssid = _server.arg("value1");
        String _password = _server.arg("value2");
      
        writeFile(LittleFS, "/ssid.txt", _ssid.c_str());
        writeFile(LittleFS, "/password.txt", _password.c_str());
    }
    _server.send(200, "text/plain", "OK");
  });

 _server.onNotFound( [this]() {
    _server.send(404, "text/html", "-- not found --");
  });


  // ====================================================================================
  // Start server
  // ====================================================================================

  _server.begin();  
  Serial.println("der Server wurde gestartet");

}