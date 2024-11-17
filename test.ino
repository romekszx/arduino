#include <ESP8266WiFi.h>

#include <PubSubClient.h>

#include <Wire.h>

#include <ESP8266WebServer.h>

#include <Adafruit_MCP23X17.h>



// Konfiguracja MCP23017

Adafruit_MCP23X17 mcp;



int odswiezanie = 60000;

float overvoltage = 4.15;

float undervoltage = 3.0;

int wyjscie_1 = D3;

int wyjscie_2 = D4;



int button_2 = D6;  

int button_3 = D7;



// Zmienne do obsługi stanów przycisków i czasu odbicia

bool button_2LastState = HIGH;

bool button_3LastState = HIGH;

// Zmienne do obsługi stanów przycisków i czasu odbicia MCP23017

bool button_8LastState;

bool button_9LastState;

bool button_10LastState;



unsigned long lastDebounceTime_8 = 0;

unsigned long lastDebounceTime_9 = 0;

unsigned long lastDebounceTime_10 = 0;

const unsigned long debounceDelay = 500; // 500 ms opóźnienia



char mqtt_payload[5]; 

bool once = false;



const char* ssid = "TP-Link";

const char* password = "14122005a";

const char* mqtt_server = "192.168.1.101";

const int mqtt_port = 1883;



WiFiClient espClient;

PubSubClient client(espClient);

ESP8266WebServer server(80);



int messageValue = 0;

unsigned long lastMsg = 0;

String i2cDevices = "";



void setup_wifi() {

  delay(10);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

  }

  Serial.print(" OK ");

  Serial.print("Module IP: ");

  Serial.println(WiFi.localIP());

}



void reconnect() {

  while (!client.connected()) {

    String clientId = "ESP8266Client-";

    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {

      Serial.println("MQTT connected");



      // Subskrypcje dla tematów cmd

      client.subscribe("testmqtt/cmd/odswiezanie");

      client.subscribe("testmqtt/cmd/overvoltage");

      client.subscribe("testmqtt/cmd/undervoltage");

      client.subscribe("testmqtt/cmd/wyjscie_1");

      client.subscribe("testmqtt/cmd/wyjscie_2");

      client.subscribe("testmqtt/cmd/mcp_0");

      client.subscribe("testmqtt/cmd/mcp_1");

      client.subscribe("testmqtt/cmd/mcp_2");



      // Subskrypcje dla tematów status

      client.subscribe("testmqtt/status/mcp_0");

      client.subscribe("testmqtt/status/mcp_1");

      client.subscribe("testmqtt/status/mcp_2");



      Serial.println("Requesting initial states for MCP pins...");

      

    } else {

      Serial.print("failed, rc=");

      Serial.print(client.state());

      Serial.println(" try again in 5 seconds");

      delay(5000);

    }

  }

}



void callback(char* topic, byte* payload, unsigned int length) {

  String receivedTopic = String(topic);

  String payloadStr = "";

  for (int i = 0; i < length; i++) payloadStr += (char)payload[i];

  int value = payloadStr.toInt();



  struct {

    const char* topic;

    int pin;

    bool isMCP;

  } outputs[] = {

    {"testmqtt/cmd/wyjscie_1", wyjscie_1, false},

    {"testmqtt/cmd/wyjscie_2", wyjscie_2, false},

    {"testmqtt/cmd/mcp_0", 0, true},

    {"testmqtt/cmd/mcp_1", 1, true},

    {"testmqtt/cmd/mcp_2", 2, true},

    {"testmqtt/status/mcp_0", 0, true},

    {"testmqtt/status/mcp_1", 1, true},

    {"testmqtt/status/mcp_2", 2, true},

  };



  for (const auto& output : outputs) {

    if (receivedTopic.equals(output.topic)) {

      if (receivedTopic.startsWith("testmqtt/status/")) {

        if (output.isMCP) {

          mcp.digitalWrite(output.pin, value);

        } else {

          digitalWrite(output.pin, value);

        }

      } else {

        if (output.isMCP) {

          mcp.digitalWrite(output.pin, value);

        } else {

          digitalWrite(output.pin, value);

        }

        String statusTopic = "testmqtt/status/" + String(output.topic).substring(13);

        client.publish(statusTopic.c_str(), String(value).c_str(), true);  // Set retain flag to true

      }

    }

  }

}



void generateButton(String& html, const char* label, const char* onPath, const char* offPath) {

  html += "<p><a href=\"" + String(onPath) + "\"><button>Turn " + label + " ON</button></a> ";

  html += "<a href=\"" + String(offPath) + "\"><button>Turn " + label + " OFF</button></a></p>";

}



void handleRoot() {

  String html = "<html><body><h1>Control GPIO Pins</h1>";

  generateButton(html, "wyjscie_1", "/wyjscie_1/on", "/wyjscie_1/off");

  generateButton(html, "wyjscie_2", "/wyjscie_2/on", "/wyjscie_2/off");

  html += "<h2>MCP 0x20 GPIO Control</h2>";

  generateButton(html, "mcp_0", "/mcp_0/on", "/mcp_0/off");

  generateButton(html, "mcp_1", "/mcp_1/on", "/mcp_1/off");

  generateButton(html, "mcp_2", "/mcp_2/on", "/mcp_2/off");

  html += "<h2>Status Information</h2>";

  html += "<p>Odswiezanie: " + String(odswiezanie) + " ms</p>";

  html += "<p>Overvoltage: " + String(overvoltage, 2) + " V</p>";

  html += "<p>Undervoltage: " + String(undervoltage, 2) + " V</p>";

  html += "<h2>I2C Devices</h2>";

  html += i2cDevices != "" ? "<p>" + i2cDevices + "</p>" : "<p>No I2C devices found.</p>";

  html += "</body></html>";

  server.send(200, "text/html", html);

}



// Dodaj funkcje do obsługi wyjść

void handle_wyjscie_1_On() {

  digitalWrite(wyjscie_1, HIGH);

  client.publish("testmqtt/status/wyjscie_1", "1", true);

  server.sendHeader("Location", "/", true);

  server.send(302, "text/plain", "");

}



void handle_wyjscie_1_Off() {

  digitalWrite(wyjscie_1, LOW);

  client.publish("testmqtt/status/wyjscie_1", "0", true);

  server.sendHeader("Location", "/", true);

  server.send(302, "text/plain", "");

}



void handle_wyjscie_2_On() {

  digitalWrite(wyjscie_2, HIGH);

  client.publish("testmqtt/status/wyjscie_2", "1", true);

  server.sendHeader("Location", "/", true);

  server.send(302, "text/plain", "");

}



void handle_wyjscie_2_Off() {

  digitalWrite(wyjscie_2, LOW);

  client.publish("testmqtt/status/wyjscie_2", "0", true);

  server.sendHeader("Location", "/", true);

  server.send(302, "text/plain", "");

}



void handle_mcp_0_On() {

  mcp.digitalWrite(0, HIGH);

  client.publish("testmqtt/status/mcp_0", "1", true);

  server.sendHeader("Location", "/", true);

  server.send(302, "text/plain", "");

}



void handle_mcp_0_Off() {

  mcp.digitalWrite(0, LOW);

  client.publish("testmqtt/status/mcp_0", "0", true);

  server.sendHeader("Location", "/", true);

  server.send(302, "text/plain", "");

}



void handle_mcp_1_On() {

  mcp.digitalWrite(1, HIGH);

  client.publish("testmqtt/status/mcp_1", "1", true);

  server.sendHeader("Location", "/", true);

  server.send(302, "text/plain", "");

}



void handle_mcp_1_Off() {

  mcp.digitalWrite(1, LOW);

  client.publish("testmqtt/status/mcp_1", "0", true);

  server.sendHeader("Location", "/", true);

  server.send(302, "text/plain", "");

}



void handle_mcp_2_On() {

  mcp.digitalWrite(2, HIGH);

  client.publish("testmqtt/status/mcp_2", "1", true);

  server.sendHeader("Location", "/", true);

  server.send(302, "text/plain", "");

}



void handle_mcp_2_Off() {

  mcp.digitalWrite(2, LOW);

  client.publish("testmqtt/status/mcp_2", "0", true);

  server.sendHeader("Location", "/", true);

  server.send(302, "text/plain", "");

}



void scanI2CDevices() {

  byte error, address;

  int deviceCount = 0;

  i2cDevices = "";



  for(address = 1; address < 127; address++ ) {

    Wire.beginTransmission(address);

    error = Wire.endTransmission();

    

    if (error == 0) {

      if (deviceCount > 0) i2cDevices += ", ";

      i2cDevices += "0x";

      if (address < 16) i2cDevices += "0";

      i2cDevices += String(address, HEX);

      deviceCount++;

    }

  }



  if (deviceCount == 0) {

    i2cDevices = "No I2C devices found";

  }

}



// Funkcja do inicjalizacji MCP23017

void setup_mcp23017() {



  mcp.begin_I2C(0x20);  // Inicjalizacja MCP23017 pod adresem I2C 0x20



  mcp.pinMode(0, OUTPUT);

  mcp.pinMode(1, OUTPUT);

  mcp.pinMode(2, OUTPUT);



  mcp.pinMode(8, INPUT_PULLUP);

  mcp.pinMode(9, INPUT_PULLUP);

  mcp.pinMode(10, INPUT_PULLUP);



  // Inicjalizacja stanu przycisków

  button_8LastState = mcp.digitalRead(8); // Initial state of button 8

  button_9LastState = mcp.digitalRead(9); // Initial state of button 9

  button_10LastState = mcp.digitalRead(10); // Initial state of button 10



}



// Obsługa przycisków MCP23017

void ButtonPress(int pin, bool& lastState, unsigned long& lastDebounceTime, int outputPin) {

  bool currentState = mcp.digitalRead(pin);

  if (currentState == LOW && lastState == HIGH && (millis() - lastDebounceTime > debounceDelay)) {

    mcp.digitalWrite(outputPin, !mcp.digitalRead(outputPin));

    client.publish(("testmqtt/status/mcp_" + String(outputPin)).c_str(), String(mcp.digitalRead(outputPin)).c_str(), true);

    lastDebounceTime = millis();

  }

  lastState = currentState;

}



void MCP_Button_Press() {

  ButtonPress(8, button_8LastState, lastDebounceTime_8, 0);

  ButtonPress(9, button_9LastState, lastDebounceTime_9, 1);

  ButtonPress(10, button_10LastState, lastDebounceTime_10, 2);

}



void setup() {



  Serial.begin(115200);



  setup_mcp23017();  // Inicjalizacja MCP23017



  setup_wifi();



  client.setServer(mqtt_server, mqtt_port);

  client.setCallback(callback);



  scanI2CDevices();  // Scan for I2C devices at startup



  pinMode(wyjscie_1, OUTPUT);

  pinMode(wyjscie_2, OUTPUT);

  pinMode(button_2, INPUT_PULLUP);

  pinMode(button_3, INPUT_PULLUP);



  server.on("/", handleRoot);

  server.on("/wyjscie_1/on", handle_wyjscie_1_On);

  server.on("/wyjscie_1/off", handle_wyjscie_1_Off);

  server.on("/wyjscie_2/on", handle_wyjscie_2_On);

  server.on("/wyjscie_2/off", handle_wyjscie_2_Off);

  server.on("/mcp_0/on", handle_mcp_0_On);

  server.on("/mcp_0/off", handle_mcp_0_Off);

  server.on("/mcp_1/on", handle_mcp_1_On);

  server.on("/mcp_1/off", handle_mcp_1_Off);

  server.on("/mcp_2/on", handle_mcp_2_On);

  server.on("/mcp_2/off", handle_mcp_2_Off);

  server.begin();



}



void publishInitialStatus() {

  if (!once) {

    client.publish("testmqtt/status/ip", WiFi.localIP().toString().c_str());

    client.publish("testmqtt/status/i2c", i2cDevices.c_str());

    client.publish("testmqtt/status/overvoltage", String(overvoltage).c_str());

    client.publish("testmqtt/status/undervoltage", String(undervoltage).c_str());

    client.publish("testmqtt/status/odswiezanie", String(odswiezanie).c_str());

    once = true;

  }

}



void loop() {

  if (!client.connected()) reconnect();

  client.loop();

  server.handleClient();

  MCP_Button_Press();

  publishInitialStatus();



  if (millis() - lastMsg > odswiezanie) {

    lastMsg = millis();

    client.publish("testmqtt/status/heartbeat", String(messageValue).c_str());

    messageValue = 1 - messageValue;

  }

}
