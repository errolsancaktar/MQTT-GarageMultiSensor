//NodeMCU v1 Board
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <WString.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "DHTesp.h"
#include <WebOTA.h>
#include "config.h"


// Setup WifiClient

ESP8266WebServer server(80);
WiFiClient espClient;

// Setup MQTT
PubSubClient client(MQTT_SERVER, 1883, callback, espClient);
PubSubClientTools mqtt(client);


void handleRoot() {
  String htmlmsg = "";
  htmlmsg += "<HTML><BODY><H1>Distance: </H1>";
  htmlmsg += distance;
  htmlmsg += "<HTML><BODY><H1>Temperature: </H1>";
  htmlmsg += ftemp;
  htmlmsg += "<HTML><BODY><H1>Humidity: </H1>";
  htmlmsg += humidity;
  htmlmsg += "<HTML><BODY><H1>Motion: </H1>";
  htmlmsg += motionStatus;
  htmlmsg += "</BODY></HTML>";
  //htmlmsg += "<BR>";
  server.send(200, "text/html", htmlmsg);
  Serial.println(htmlmsg);

}


void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}



boolean reconnect() {
  if (client.connect(clientName, MQTT_USER, MQTT_PASS, statusTopic, 1, 1, "offline")) {

    //mqtt.publish(baseTopic + "status", "online", true);
    mqtt.publish(baseTopic + "ip", ipAdd, true);
    mqtt.publish(baseTopic + "mac", macAdd, true);
    lastReconnectAttempt = 0;
    return client.connected();
  }
  return client.connected();
}

void getPIR(){
  ////////////// PIR Sensor //////////////

 //PIR CODE
    pirValue = digitalRead(PIRPIN); //read state of the
    topic = baseTopic + "motion";
    topic.toCharArray(charBuf,50);
    if (pirValue == LOW && pirStatus != 1) {
      motionStatus = "OFF";
      pirStatus = 1;
      mqtt.publish(baseTopic + "motion","OFF",true);

    }

    else if (pirValue == HIGH && pirStatus != 2) {
     motionStatus = "ON";
     pirStatus = 2;
     mqtt.publish(baseTopic + "motion","ON",true);
    }
    Serial.println(motionStatus);

}

void getTH() {

////////////// Temperature Sensor //////////////
    webota.delay(dht.getMinimumSamplingPeriod());

    humidity = dht.getHumidity();
    temperature = dht.getTemperature();
    ftemp = dht.toFahrenheit(temperature);
    Serial.print("Temperature: ");
    dtostrf(ftemp,5,2, charBuf);

    mqtt.publish(baseTopic + "temperature", charBuf, true);
    Serial.print(ftemp, 1);
    Serial.print("\n");
    Serial.print("Humidity: ");
    dtostrf(dht.getHumidity(),5,2, charBuf);
    mqtt.publish(baseTopic + "humidity", charBuf, true);
    Serial.print(humidity, 1);
    Serial.print("\n");
}

void getDistance() {

////////////// Distance Sensor //////////////
// Clears the trigPin
    digitalWrite(TRIGPIN, LOW);
    delayMicroseconds(2);

//// Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(TRIGPIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGPIN, LOW);

//// Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(ECHOPIN, HIGH);

//// Calculating the distance
    distance = duration*0.034/2;
//// Prints the distance on the Serial Monitor
    Serial.print("Distance: ");
    dtostrf(distance,3,0, charBuf);
    mqtt.publish(baseTopic + "distance", charBuf, true);
    Serial.println(distance);
    if(distance < 250){
      mqtt.publish(baseTopic + "occupied", "true", true);
    }else{
      mqtt.publish(baseTopic + "occupied", "false", true);

    }
}



//##//##//##//##  Setup

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.println("starting Setup");

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();

    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("AutoConnectAP");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();


    //if you get here you have connected to the WiFi
    Serial.println("connected...winning :)");
    ip = WiFi.localIP().toString();
    ip.toCharArray(ipAdd, 20);
    mac = WiFi.macAddress();
    mac.toCharArray(macAdd, 30);

////  Setup Rangefinder
    pinMode(TRIGPIN, OUTPUT); // Sets the trigPin as an Output
    pinMode(ECHOPIN, INPUT); // Sets the echoPin as an Input

//  Setup PIR and TEMP Pins
    pinMode(PIRPIN, INPUT_PULLUP);
    pinMode(DHTPIN, INPUT);

    dht.setup(DHTPIN, DHTesp::DHT22);

    //init webota
    webota.init(8080, "/update");
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    if (MDNS.begin("esp8266")) {
      Serial.println("MDNS responder started");
      // server.on("/cmd", handleLight);
      server.on("/", handleRoot);
      server.onNotFound(handleNotFound);

      server.begin();
      Serial.println("HTTP server started");
    }

    // set LWT
    baseTopic.toCharArray(statusTopic,30);
    strcat(statusTopic, "info");

    // Initial MQTT Setup
    client.connect(clientName, MQTT_USER, MQTT_PASS, statusTopic, 1, 1, "offline");
  //  mqtt.publish(baseTopic + "status", "online", true);
    mqtt.publish(baseTopic + "ip", ipAdd, true);
    mqtt.publish(baseTopic + "mac", macAdd, true);

    // Define Callback func
    client.setCallback(callback);

    // Subscribe to MQTT Set Topic
    // client.subscribe("home/Cabinetlights/set");
    // state = "OFF";
    uptime = millis();
    mqtt.publish(baseTopic + "uptime", (String)uptime);
    getDistance();
    getTH();
    getPIR();

  }

void loop() {
  long now = millis();
  if (!client.connected()) {
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected
    client.loop();
  }
  if(now - lastUpdate > 30000){
    uptime = millis();
    mqtt.publish(baseTopic + "uptime", (String)uptime);
    getDistance();
    getTH();
    getPIR();
    lastUpdate = now;
  }

  webota.handle();
  server.handleClient();
  MDNS.update();

  if (uptime > 4000000000) {
    mqtt.publish(baseTopic + "info", "restart", true);
    ESP.restart();
  }
}
