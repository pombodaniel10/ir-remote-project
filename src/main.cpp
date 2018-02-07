#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

IRsend irsend(4);
const int pinButton = 2;

const char *ssid = "UNE_HFC_AB83";
const char *password = "D2d1n2s21628";
const char *mqtt_server = "192.168.1.53";
const char *dns = "irremote-01";

boolean debug = false;
unsigned long startTime = millis();
const char *str_status[] = {"WL_IDLE_STATUS",    "WL_NO_SSID_AVAIL",
                            "WL_SCAN_COMPLETED", "WL_CONNECTED",
                            "WL_CONNECT_FAILED", "WL_CONNECTION_LOST",
                            "WL_DISCONNECTED"};

// provide text for the WiFi mode
const char *str_mode[] = {"WIFI_OFF", "WIFI_STA", "WIFI_AP", "WIFI_AP_STA"};

WiFiClient espClient;
PubSubClient client(espClient);
MDNSResponder mdns;
bool dnsConnection = false;

void setup_wifi();
void callback(char *topic, byte *payload, unsigned int length);
void telnetHandle();

WiFiServer telnetServer(23);
WiFiClient serverClient;

void setup() {
  irsend.begin();
  Serial.begin(115200);
  setup_wifi();
  delay(15);
  if (mdns.begin(dns, WiFi.localIP())) {
    dnsConnection = true;
  }
  delay(15);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi mode: ");
    Serial.println(str_mode[WiFi.getMode()]);
    Serial.print("Status: ");
    Serial.println(str_status[WiFi.status()]);
    // signal WiFi connect
    delay(300); // ms
  } else {
    Serial.println("");
    Serial.println("WiFi connect failed, push RESET button.");
  }

  telnetServer.begin();
  telnetServer.setNoDelay(true);
  Serial.println("Please connect Telnet Client, exit with ^] and 'quit'");

  Serial.print("Free Heap[B]: ");
  Serial.println(ESP.getFreeHeap());
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println("Booting");
  // WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // WiFi.begin(ssid, password);
  WiFiManager wifiManager;
  wifiManager.resetSettings();

  if (!wifiManager.autoConnect("IR Remote")) {
    Serial.println("failed to connect and hit timeout");
    // reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  // while (WiFi.status() != WL_CONNECTED) {
  //  delay(500);
  //  Serial.print(".");
  //}

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length) {
  StaticJsonBuffer<200> jsonBuffer;
  String boton = "";
  JsonObject& root = jsonBuffer.parseObject(payload);

  boton = root["boton"].as<String>();

  if(boton=="POWER"){
    irsend.sendNEC(0x20DF10EF,32);
    Serial.println("Power");
    delay(100);
  }else if(boton=="VOLUP"){
    irsend.sendNEC(0x20DF40BF,32);
    Serial.println("Vol UP");
    delay(100);
  }else if(boton=="VOLDOWN"){
    irsend.sendNEC(0x20DFC03F,32);
    Serial.println("Vol DOWN");
    delay(100);
  }else if(boton=="CHUP"){
    irsend.sendNEC(0x20DF00FF,32);
    Serial.println("Channel UP");
    delay(100);
  }else if(boton=="CHDOWN"){
    irsend.sendNEC(0x20DF807F,32);
    Serial.println("Channel DOWN");
    delay(100);
  }


}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ir_remote")) {
      Serial.println("connected");
      client.subscribe("ir_test");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  telnetHandle();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
bool mensaje = false;
void telnetHandle() {
  if (telnetServer.hasClient()) {
    if (!serverClient || !serverClient.connected()) {
      if (serverClient) {
        serverClient.stop();
        Serial.println("Telnet Client Stop");
      }
      serverClient = telnetServer.available();
      Serial.println("New Telnet client");
      serverClient
          .flush(); // clear input buffer, else you get strange characters
    }
  }

  while (serverClient.available()) { // get data from Client
    Serial.write(serverClient.read());
  }

  if (!mensaje) { // run every 2000 ms
    startTime = millis();

    if (serverClient && serverClient.connected()) { // send data to Client
      serverClient.println("Conectado por telnet. Sos re-groso che.");
      serverClient.println("Un saludo para los mortales.");
      if (WiFi.status() == WL_CONNECTED) {
        serverClient.println("Conectado a: ");
        serverClient.println(WiFi.localIP());
      }
      if (client.connected()) {
        serverClient.println("Conectado a MQTT");
      }
      if (dnsConnection) {
        serverClient.println("Se logró establecer el dns");
      }
      mensaje = true;
    }
  }
  delay(10); // to avoid strange characters left in buffer
}
