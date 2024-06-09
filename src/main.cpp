#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <AsyncWebSocket.h>
#include "config.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

#define NUM_LANES 4
const uint8_t lanePins[NUM_LANES] = {D5, D6, D7, D8};
unsigned long lastCheckTime = 0;
unsigned long countdownTimers[NUM_LANES] = {0};
int countdownTimer = 3;

struct ButtonState {
  String label;
  String pilotName;
  int countdown;
  bool isPitting;
};

ButtonState buttonStates[NUM_LANES] = {
  {"Lane 1", "", 0, false},
  {"Lane 2", "", 0, false},
  {"Lane 3", "", 0, false},
  {"Lane 4", "", 0, false}
};

void notifyClients() {
  String message = "{\"type\":\"update\",\"data\":[";
  for (int i = 0; i < NUM_LANES; i++) {
    // message += "{\"label\":\"" + buttonStates[i].label + "\",\"pilotName\":\"" + buttonStates[i].pilotName + "\",\"countdown\":" + String(buttonStates[i].countdown) + "}";
    message += "{\"label\":\"" + buttonStates[i].label + "\",\"pilotName\":\"" + buttonStates[i].pilotName + "\",\"countdown\":" + String(buttonStates[i].countdown) + ",\"isPitting\":" + (buttonStates[i].isPitting ? "true" : "false") + "}";
    
    if (i < NUM_LANES - 1) message += ",";
  }
  message += "]}";
  ws.textAll(message);
}

void announcePitting(int lane, bool isPitting) {
  String message = "{\"type\":\"announce\",\"lane\":" + String(lane) + ",\"pilotName\":\"" + buttonStates[lane].pilotName + "\",\"isPitting\":" + (isPitting ? "true" : "false") + "}";
  Serial.println("AnnouncePitting message is:");
  Serial.println(message);
  ws.textAll(message);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    if (message.startsWith("start")) {
      int lane = message.substring(5).toInt();
      buttonStates[lane].countdown = countdownTimer;
      countdownTimers[lane] = millis();
      buttonStates[lane].isPitting = !buttonStates[lane].isPitting; // change the pitting flag
      announcePitting(lane, buttonStates[lane].isPitting);
    } else if (message.startsWith("update")) {
      int lane = message.charAt(6) - '0';
      String name = message.substring(8);
      buttonStates[lane].pilotName = name;
      buttonStates[lane].label = "Lane " + String(lane + 1) + ": " + name;
    }
    notifyClients();
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    client->text("Connected");
    notifyClients();
  } else if (type == WS_EVT_DATA) {
    handleWebSocketMessage(arg, data, len);
  }
}

void checkLaneSwitches() {
  for (int i = 0; i < NUM_LANES; i++) {
    if (digitalRead(lanePins[i]) == HIGH) { // Assuming switch opens to high
      if (buttonStates[i].countdown == 0) { // Only trigger if not already in countdown
        buttonStates[i].countdown = countdownTimer;
        countdownTimers[i] = millis();
        notifyClients();
        buttonStates[i].isPitting = !buttonStates[i].isPitting; // change the pitting flag for a button press
        announcePitting(i, buttonStates[i].isPitting);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }
  Serial.println("LittleFS mounted successfully");

  WiFi.hostname(hostname);
  Serial.printf("Hostname: %s\n", WiFi.hostname().c_str());
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/favicon.png", "image/png");
  });

  server.serveStatic("/", LittleFS, "/");

  server.begin();

  for (int i = 0; i < NUM_LANES; i++) {
    buttonStates[i].countdown = 0;
    pinMode(lanePins[i], INPUT_PULLUP); // Assuming switch closes to ground
  }
}

void loop() {
  ws.cleanupClients();
  checkLaneSwitches();

  unsigned long currentTime = millis();
  for (int i = 0; i < NUM_LANES; i++) {
    if (buttonStates[i].countdown > 0 && (currentTime - countdownTimers[i]) >= 1000) {
      buttonStates[i].countdown--;
      countdownTimers[i] = currentTime;
      notifyClients();
    }
  }

  // Check lane switches every 50 ms to debounce
  if (currentTime - lastCheckTime > 50) {
    lastCheckTime = currentTime;
    checkLaneSwitches();
  }
}
