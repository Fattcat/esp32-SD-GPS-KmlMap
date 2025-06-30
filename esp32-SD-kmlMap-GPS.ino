/*
 * GPS + WiFi Logger with SD card to KML file on ESP32
 * Features:
 * - Logs GPS when fix is valid
 * - Logs WiFi SSIDs even without GPS fix
 * - Automatically names files: track001.kml ... track999.kml
 */

#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <SPI.h>
#include <SD.h>

#define GPS_RX 16
#define GPS_TX 17
#define SD_CS 5
#define SCAN_INTERVAL 10000 // milliseconds
#define GPS_TIMEOUT 10000 // milliseconds no signal

TinyGPSPlus gps;
HardwareSerial ss(1);
File logFile;
String currentTrackFile;
bool gpsInitialized = false;
bool gpsFixAvailable = false;
unsigned long lastGPSTime = 0;
unsigned long lastScanTime = 0;

void setup() {
  Serial.begin(115200);
  ss.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  
  if (!SD.begin(SD_CS)) {
    Serial.println("[SD] Initialization failed!");
    while (true);
  }
  Serial.println("[SD] OK");

  // Generate next available filename trackXXX.kml
  char trackFilename[20];
  int fileIndex = 1;
  do {
    sprintf(trackFilename, "/track%03d.kml", fileIndex);
    fileIndex++;
  } while (SD.exists(trackFilename) && fileIndex <= 999);

  if (fileIndex > 999) {
    Serial.println("[ERROR] Too many files on SD card");
    while (true);
  }

  currentTrackFile = String(trackFilename);
  logFile = SD.open(currentTrackFile.c_str(), FILE_WRITE);
  if (logFile) {
    logFile.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    logFile.println("<kml xmlns=\"http://www.opengis.net/kml/2.2\">");
    logFile.println("  <Document><Placemark><LineString><coordinates>");
    logFile.close();
    Serial.println("[KML] File created: " + currentTrackFile);
  } else {
    Serial.println("[ERROR] Cannot open file for writing");
    while (true);
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  gpsInitialized = true;
}

void loop() {
  // Process GPS input
  while (ss.available()) {
    char c = ss.read();
    gps.encode(c);
    if (gps.location.isUpdated()) {
      gpsFixAvailable = gps.location.isValid();
      lastGPSTime = millis();
      if (gpsFixAvailable) {
        float lat = gps.location.lat();
        float lon = gps.location.lng();
        logFile = SD.open(currentTrackFile.c_str(), FILE_APPEND);
        if (logFile) {
          logFile.printf("%.6f,%.6f,0\n", lon, lat);
          logFile.close();
          Serial.printf("[GPS] Lat: %.6f Lon: %.6f\n", lat, lon);
        }
      }
    }
  }

  // Check for GPS timeout
  if (gpsInitialized && millis() - lastGPSTime > GPS_TIMEOUT) {
    gpsFixAvailable = false;
  }

  // Perform WiFi scan every SCAN_INTERVAL ms
  if (millis() - lastScanTime > SCAN_INTERVAL) {
    lastScanTime = millis();

    int n = WiFi.scanNetworks();
    Serial.println("[WiFi] Scan complete");
    logFile = SD.open(currentTrackFile.c_str(), FILE_APPEND);
    if (logFile) {
      for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        int32_t rssi = WiFi.RSSI(i);
        logFile.print("<!-- WiFi: ");
        logFile.print(ssid);
        logFile.print(" RSSI: ");
        logFile.print(rssi);
        logFile.println(" -->");
      }
      logFile.close();
    }
    WiFi.scanDelete();
  }
}

void endTrack() {
  logFile = SD.open(currentTrackFile.c_str(), FILE_APPEND);
  if (logFile) {
    logFile.println("</coordinates></LineString></Placemark></Document></kml>");
    logFile.close();
    Serial.println("[KML] Track ended properly");
  }
}
