#include <Arduino.h>
#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

////////////////////////////////////////////////////////////////////////////
//
// Source: https://github.com/espressif/arduino-esp32/blob/master/libraries/LittleFS/examples/LITTLEFS_test/LITTLEFS_test.ino
//
////////////////////////////////////////////////////////////////////////////

//#define FORMAT_LITTLEFS_IF_FAILED true

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

String readFile(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return "";  
  }
  String fileContent = "";
  while (file.available()) {
    fileContent += (char)file.read();
  }
  file.close();

  return fileContent;
}

bool writeFile(fs::FS &fs, const char *path, const char *message) {
  bool success = false;

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return false;
  }
  if (file.print(message)) {
    success = true;
  } else {
    Serial.println("- write failed");
    success = false;
  }
  file.close();
  return success;
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("- message appended");
  } else {
    Serial.println("- append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  Serial.printf("Renaming file %s to %s\r\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("- file renamed");
  } else {
    Serial.println("- rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

void saveJsonToFile(fs::FS &fs, const char* filename, JsonDocument& doc) {
  // Öffne die Datei im Schreibmodus
  File file = fs.open(filename, FILE_WRITE);
  
  // Überprüfe, ob die Datei erfolgreich geöffnet wurde
  if (!file) {
    Serial.println(F("Failed to open file for writing"));
    return;
  }

  // Schreibe das JSON-Dokument in die Datei
  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write JSON to file"));
  } else {
    Serial.println(F("JSON written to file successfully"));
  }

  // Schließe die Datei
  file.close();
}

void loadJsonFromFile(fs::FS &fs,const char* filename, JsonDocument& doc) {
  // Existierende Einträge im JSON-Dokument löschen
  doc.clear();

  // Datei im Lesemodus öffnen
  File file = fs.open(filename, FILE_READ);
  
  // Überprüfen, ob die Datei geöffnet werden konnte
  if (!file) {
    Serial.println(F("Failed to open file for reading"));
    return;
  }

  // Den Inhalt der Datei in das JSON-Dokument laden
  DeserializationError error = deserializeJson(doc, file);
  
  // Datei schließen
  file.close();

  // Überprüfen, ob es Deserialisierungsfehler gab
  if (error) {
    Serial.print(F("Failed to read file, DeserializationError: "));
    Serial.println(error.c_str());
  } else {
    Serial.println(F("JSON successfully loaded from file"));
    // Anzahl der Einträge im "table"-Array prüfen
    if (doc.containsKey("table")) {
      JsonArray table = doc["table"];
      size_t entryCount = table.size();  // Größe des Arrays ermitteln
      Serial.print(F("Number of entries loaded: "));
      Serial.println(entryCount);
    } else {
      Serial.println(F("No 'table' array found in the JSON file"));
    }
  }
}