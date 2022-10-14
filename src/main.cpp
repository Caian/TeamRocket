/*
 * Copyright (C) 2020 Caian Benedicto <caianbene@gmail.com>
 *
 * This file is part of TeamRocket.
 *
 * TeamRocket is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * TeamRocket is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TeamRocket.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <TeamRocket.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHTesp.h>
#include <FS.h>

// The WHOAMI string with the DNS name must be defined by the including file

#define BOOT_WAIT_SECS 10
#define WIFI_WAIT_SECS 20
#define SENSOR_UPDATE_FREQ_MILLIS 10000
#define SERIAL_TIMEOUT_MILLIS 1000

#define MAX_SSID_SIZE 32
#define MAX_PASS_SIZE 63
#define SSID_LENGTH MAX_SSID_SIZE + 2
#define PASS_LENGTH MAX_PASS_SIZE + 2

#define AP_FILE "ap"

char ssid[SSID_LENGTH];
char password[PASS_LENGTH];

ESP8266WebServer server(80);

DHTesp dht;
float dhtTemperature;
float dhtHumidity;

unsigned long dhtLastUpdate;

int ledOut; // Just to flip led

void ledOn() {
  ledOut = 1;
  digitalWrite(led, LOW);
}

void ledOff() {
  ledOut = 0;
  digitalWrite(led, HIGH);
}

void ledFlip() {
  if (ledOut == 0) {
    ledOn();
  }
  else {
    ledOff();
  }
}

void beginRegion() {
  ledOn();
}

void endRegion() {
  delay(200);
  ledOff();
  delay(200);
}

void halt() {
  while (true) {
    ledOff();
    delay(100);
  }
}

void sendDHT11Data() {
  // Format the temperature with one decimal place
  String res = String(dhtTemperature, 1);
  res += ",oC,";
  res += String(dhtHumidity, 1);
  res += ",%";
  server.send(200, "text/plain", res);
}

void updateDHT11Data() {
  unsigned long nowMs = millis();
  // Update the sensor every SENSOR_UPDATE_FREQ_MILLIS
  if (nowMs - dhtLastUpdate < SENSOR_UPDATE_FREQ_MILLIS)
    return;
  ledOn();
  Serial.print("Updating sensor data... ");
  TempAndHumidity dhtData = dht.getTempAndHumidity();
  if (!isnan(dhtData.temperature)) {
    dhtTemperature = dhtData.temperature;
  }
  if (!isnan(dhtData.humidity)) {
    dhtHumidity = dhtData.humidity;
  }
  Serial.print(dhtTemperature);
  Serial.print(",");
  Serial.println(dhtHumidity);
  ledOff();
  // Call millis again to compensate for the
  // delay when reading from the sensor
  dhtLastUpdate = millis();
}

void handleRoot() {
  ledOn();
  sendDHT11Data();
  ledOff();
}

void handleNotFound() {
  // Send the data anyway...
  ledOn();
  sendDHT11Data();
  ledOff();
}

const char* formatWiFiStatus(int s) {
  switch (s) {
    case WL_CONNECTED: return "WL_CONNECTED";
    case WL_NO_SHIELD: return "WL_NO_SHIELD";
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    default: return "UNKNOWN";
  }
}

int connectWiFi() {
  int failed = 1;
  // Begin connection
  beginRegion();
  Serial.print("Connecting to SSID ");
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  Serial.print(ssid);
  Serial.println("...");
  WiFi.begin(ssid, password);
  endRegion();
  // Wait for connection
  beginRegion();
  for (int i = 0; i < 2*WIFI_WAIT_SECS; i++) {
    int wifiStatus = WiFi.status();
    Serial.print("Connecting (");
    Serial.print(formatWiFiStatus(wifiStatus));
    Serial.println(")...");
    if (wifiStatus == WL_CONNECT_FAILED) {
      break;
    }
    if (wifiStatus == WL_CONNECTED) {
      failed = 0;
      Serial.print("Connected with IP ");
      Serial.println(WiFi.localIP());
      break;
    }
    delay(500);
    ledFlip();
  }
  endRegion();
  return failed;
}

void checkWiFi() {
  int wifiStatus = WiFi.status();
  if (wifiStatus != WL_CONNECTED) {
    Serial.print("Network error ");
    Serial.print(formatWiFiStatus(wifiStatus));
    Serial.println("!");
    connectWiFi();
  }
}

bool tryReadWiFiAuth() {
  Serial.println("Network auth format: SSID/passw\\n");
  if (Serial.available() == 0) {
    return false;
  }
  int ssidRead = Serial.readBytesUntil('/', ssid, SSID_LENGTH-1);
  if (ssidRead == 0) {
    Serial.println("Empty SSID!");
    return false;
  }
  else if (ssidRead > MAX_SSID_SIZE) {
    Serial.println("SSID is too long!");
    return false;
  }
  ssid[ssidRead] = '\x00';
  int passRead = Serial.readBytesUntil('\n', password, PASS_LENGTH-1);
  if (passRead == 0) {
    Serial.println("Empty password!");
    return false;
  }
  else if (passRead > MAX_PASS_SIZE) {
    Serial.println("password is too long!");
    return false;
  }
  password[passRead] = '\x00';
  return true;
}

void setup(void) {
  dhtTemperature = 0;
  dhtLastUpdate = 0;
  ledOut = 0;

  pinMode(led, OUTPUT);
  ledOff();

  for (int i = 0; i < 8; i++) {
    delay(250);
    ledFlip();
  }

  beginRegion();
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
    ledFlip();
  }
  endRegion();

  for (int i = 0; i < BOOT_WAIT_SECS; i++) {
    ledFlip();
    Serial.print("Starting in ");
    Serial.print(BOOT_WAIT_SECS-i);
    Serial.println(" seconds...");
    delay(1000);
  }
  Serial.print("Hello from ");
  Serial.print(WHOAMI);
  Serial.println("!");

  // Initialize the internal flash storage and read the WiFi credentials
  beginRegion();
  Serial.setTimeout(SERIAL_TIMEOUT_MILLIS);
  bool hasWiFiAuth = tryReadWiFiAuth();
  if (hasWiFiAuth) {
    Serial.println("Using WiFi credentials:");
    Serial.print("  ");
    Serial.println(ssid);
    Serial.print("  ");
    Serial.println(password);
  }
  ledFlip();
  if (!SPIFFS.begin()) {
    Serial.println("Failed to open flash memory!");
  }
  else {
    if (hasWiFiAuth) {
      Serial.println("Saving WiFi credentials...");
      File apFile = SPIFFS.open(AP_FILE, "w");
      if (!apFile) {
        Serial.println("IO error!");
      }
      else {
        if (apFile.write(ssid, SSID_LENGTH) != SSID_LENGTH) {
          Serial.println("IO error!");
        }
        if (apFile.write(password, PASS_LENGTH) != PASS_LENGTH) {
          Serial.println("IO error!");
        }
        apFile.close();
      }
    }
    else if (!SPIFFS.exists(AP_FILE)) {
      Serial.println("Credentials file does not exist!");
    }
    else {
      Serial.println("Loading WiFi credentials...");
      File apFile = SPIFFS.open(AP_FILE, "r");
      if (!apFile) {
        Serial.println("IO error!");
      }
      else {
        if (apFile.read((uint8_t*)ssid, SSID_LENGTH) != SSID_LENGTH) {
          Serial.println("IO error!");
        }
        if (apFile.read((uint8_t*)password, PASS_LENGTH) != PASS_LENGTH) {
          Serial.println("IO error!");
        }
        apFile.close();
        hasWiFiAuth = true;
        Serial.println("Using WiFi credentials:");
        Serial.print("  ");
        Serial.println(ssid);
        Serial.print("  ");
        Serial.println(password);
      }
    }
    SPIFFS.end();
  }
  if (!hasWiFiAuth) {
    Serial.println("No WiFi credentials available!");
    halt();
  }
  endRegion();

  // Initialize the temperature sensor
  beginRegion();
  Serial.println("Starting temperature sensor...");
  dht.setup(gpioDht, DHTesp::DHT11);
  endRegion();

  // Initialize the WiFi module
  beginRegion();
  Serial.println("Starting WiFi...");
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi hardware not found!");
    halt();
  }
  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("WiFi MAC is ");
  Serial.print(mac[0], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.println(mac[5], HEX);
  endRegion();
  // List networks in range
  beginRegion();
  Serial.println("Scanning networks in range...");
  int numSsids = WiFi.scanNetworks();
  if (numSsids == -1) {
    Serial.println("Network scan failed!");
  }
  else {
    Serial.print("Found ");
    Serial.print(numSsids);
    Serial.println(" SSIDs.");
    for (int i = 0; i < numSsids; i++) {
      Serial.print(WiFi.SSID(i));
      Serial.print(", ");
      Serial.print(WiFi.RSSI(i));
      Serial.print(" dBm, ");
      switch (WiFi.encryptionType(i)) {
        case ENC_TYPE_WEP:
          Serial.println("WEP.");
          break;
        case ENC_TYPE_TKIP:
          Serial.println("WPA.");
          break;
        case ENC_TYPE_CCMP:
          Serial.println("WPA2.");
          break;
        case ENC_TYPE_NONE:
          Serial.println("Unsecured.");
          break;
        case ENC_TYPE_AUTO:
          Serial.println("Unknown security.");
          break;
      }
    }
  }
  endRegion();
  // Connect to the network
  while (connectWiFi() != 0) ;

  beginRegion();
  Serial.print("Starting DNS responder as ");
  Serial.print(WHOAMI);
  Serial.println("...");
  if (MDNS.begin(WHOAMI)) {
    Serial.println("MDNS responder started");
  }
  endRegion();

  beginRegion();
  Serial.println("Starting HTTP server...");
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started.");
  endRegion();
}

void loop(void) {
  updateDHT11Data();
  checkWiFi();
  server.handleClient();
  MDNS.update();
}

