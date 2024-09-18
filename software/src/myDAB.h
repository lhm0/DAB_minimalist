// =========================================================================================================================================
//                                                 Minimalist DAB+ radio
//                                                   © Ludwin Monz 2024
// License: Creative Commons Attribution - Non-Commercial - Share Alike (CC BY-NC-SA)
// you may use, adapt, share. If you share, "share alike".
// you may not use this software for commercial purposes 
// =========================================================================================================================================

#ifndef myDAB_H
#define myDAB_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DABShield.h>

class myDAB {
  private:
    static DAB _Dab;

    static void _ServiceData();
    static String _lastServiceData;

    const byte _SCKPin = 18;
    const byte _MISOPin = 19;
    const byte _MOSIPin = 23;

    static bool _dabmode;
    DABTime _dabtime;
    uint8_t _vol = 63;
    uint8_t _service = 0;
    uint8_t _freq = 0;

    static int _numberOfStations;

  public:
    struct station {
      int number;        // Nummer der Station
      String name;       // Name der Station
      int ensemble;      // Ensemble-Nummer
      uint32_t serviceId;       // ServiceId
      uint32_t compId;          // CompId
    };

    station stationList[256];

    struct preset {
      int number;         // number in stationList
      String name;
    };

    preset presets[6];    // presets[0] = button1, ...

    static String sInfo;

    myDAB();                       // constructor

    void begin();                  // init interface, init DAB, 
    
    int scan();                    // scan DAB, store station data in stationList[]. returns number of stations
    void tune(int id);             // tune in station with id.
    void vol(uint8_t vol);

    String stationListJson();      // generates a serialized JSON of the stationList[]
    int initStationList();         // load stationList[] from file, if available, 
                                   // otherwise perform scan
    int numberOfStations();        // returns the number of stations in stationList[]. Is "0" if empty.
    bool saveStationList();        // saves stationList[] as JSON in file "/stationList.json"
    int readStationList();         // reads stationList[] from "/stationList.json" and saves data to stationList[];
                                   // returns number of stations in list.
    void printStationTable();      // prints stationList[] as table

    
    String presetsJson();          // generates a serialized JSON of the presets[]
    void initPresets();            // load presets[] from file, if available

    bool savePresets();            // saves presets[] as JSON in file "/presets.json"
    bool readPresets();            // reads presets[] from "/presets.json"vand saves data in presets[].
                                   // returns truem if successful, otherwise false
    void printPresetsTable();      // preints presets[] as table
    
    void task();                   // calls service task of DAB object

    const byte slaveSelectPin = 12;
};

#endif    
