// =========================================================================================================================================
//                                                 Minimalist DAB+ radio
//                                                   © Ludwin Monz 2024
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#ifndef webInterface_H
#define webInterface_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include "myDAB.h"

extern myDAB my_DAB;
extern int activeStation;
extern int activePreset;
extern int v_value;

class webInterface {
  private:
    WebServer _server{80};                 // server object

    String _ssid; 
    String _password;

    void _startServer();
    bool _captivePortal();

    int _volume = 17;
    int _preset = 2;

  public:
    webInterface();                                             // constructor

    void begin(String ssid_, String password_);                 // starts the web server
    void update();

};

#endif    
