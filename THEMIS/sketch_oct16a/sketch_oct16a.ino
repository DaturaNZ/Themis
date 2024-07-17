
// Import required libraries
// #ifdef ESP32
//   #include <WiFi.h>
//   #include <ESPAsyncWebServer.h>
//   #include <SPIFFS.h>
// #else
//   #include <Arduino.h>
//   #include <ESP8266WiFi.h>
//   #include <Hash.h>
//   #include <ESPAsyncTCP.h>
//   #include <ESPAsyncWebServer.h>
//   #include <FS.h>
// #endif
// #include <Wire.h>
// #include <Adafruit_Sensor.h>
// #include <Adafruit_BME280.h>

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino_JSON.h>

#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

//HX711 Config/init:
const int HX711_dout = 4; //mcu > HX711 dout pin
const int HX711_sck = 5; //mcu > HX711 sck pin
const int extLED = 23;

float load,peak,peakAdj,max_load = 0.0;
const int serialPrintInterval = 0;
unsigned long t = 0;
const int calVal_eepromAdress = 0;

//buffer array
float cell_data[2];

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Replace with your network credentials
const char* ssid = "vodafoneA58E";
const char* password = "9P5XJBYFJG";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 333; //200ms


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  
  //Initialize SPIFFS and WiFi
  initSPIFFS();
  initWiFi();

  //Rewrite to handle new function calls

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });

  server.serveStatic("/", SPIFFS, "/");

  //Define functionality

  // Request for the latest sensor readings
  server.on("/load", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = getLoadCellReadings(cell_data[0], cell_data[1]);
    request->send(200, "application/json", json);
    json = String();
  });
  // server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", readBME280Humidity().c_str());
  // });
  // server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", readBME280Pressure().c_str());
  // });

   events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);

  // Start server
  server.begin();  

}
 
void loop(){
  callHx711();
  //Fires event to script.js to handle client side updates
  if ((millis() - lastTime) > timerDelay) {
    // Send Events to the client with the Sensor Readings Every 10 seconds
    //events.send("ping",NULL,millis());
    events.send(getLoadCellReadings(cell_data[0], cell_data[1]).c_str(),"new_readings" ,millis());
    lastTime = millis();
  }  
}

//========================FUNC DEFINTIIONS===========================

void calibrateLoadCell() {
  Serial.println("***");
  Serial.println("Start calibration:");
  Serial.println("Place the load cell an a level stable surface.");
  Serial.println("Remove any load applied to the load cell.");
  Serial.println("Send 't' from serial monitor to set the tare offset.");

  boolean _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      if (Serial.available() > 0) {
        char inByte = Serial.read();
        if (inByte == 't') LoadCell.tareNoDelay();
      }
    }
    if (LoadCell.getTareStatus() == true) {
      Serial.println("Tare complete");
      _resume = true;
    }
  }

  Serial.println("Now, place your known mass on the loadcell.");
  Serial.println("Then send the weight of this mass (i.e. 100.0) from serial monitor.");

  float known_mass = 0;
  _resume = false;
  while (_resume == false) {
    LoadCell.update();
    if (Serial.available() > 0) {
      known_mass = Serial.parseFloat();
      if (known_mass != 0) {
        Serial.print("Known mass is: ");
        Serial.println(known_mass);
        _resume = true;
      }
    }
  }

  LoadCell.refreshDataSet(); //refresh the dataset to be sure that the known mass is measured correct
  float newCalibrationValue = LoadCell.getNewCalibration(known_mass); //get the new calibration value

  Serial.print("New calibration value has been set to: ");
  Serial.print(newCalibrationValue);
  Serial.println(", use this as calibration value (calFactor) in your project sketch.");
  Serial.print("Save this value to EEPROM adress ");
  Serial.print(calVal_eepromAdress);
  Serial.println("? y/n");

  _resume = false;
  while (_resume == false) {
    if (Serial.available() > 0) {
      char inByte = Serial.read();
      if (inByte == 'y') {
    #if defined(ESP8266)|| defined(ESP32)
        EEPROM.begin(512);
    #endif
      EEPROM.put(calVal_eepromAdress, newCalibrationValue);
    #if defined(ESP8266)|| defined(ESP32)
        EEPROM.commit();
    #endif
        EEPROM.get(calVal_eepromAdress, newCalibrationValue);
        Serial.print("Value ");
        Serial.print(newCalibrationValue);
        Serial.print(" saved to EEPROM address: ");
        Serial.println(calVal_eepromAdress);
        _resume = true;
      }
      else if (inByte == 'n') {
        Serial.println("Value not saved to EEPROM");
        _resume = true;
      }
    }
  }

  Serial.println("End calibration");
}

void startHX711() {
  LoadCell.begin();
  unsigned long stabilizingtime = 6000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  //set this to false if you don't want tare to be performed in the next step
  boolean _tare = false;
  LoadCell.start(stabilizingtime, _tare);
  //LOADCELL CONNECTIVITY CHECK AND SETCALIB
  if (LoadCell.getTareTimeoutFlag() || LoadCell.getSignalTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    //INACTIVE LoadCell.setCalFactor(-14.58); // user set calibration value (float), initial value 1.0 may be used for this sketch
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
}

float peakComp(float inp){
  if (inp > peak){
    peak = inp;
  }
  return peak;
}

void callHx711(){
  static boolean newDataReady = 0;
  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData() / 1000.0;
      cell_data[0] = i;
      cell_data[1] = peakComp(i);
      newDataReady = 0;
      t = millis();
    }
  }

}

String getLoadCellReadings(float load, float max_load) {
  readings["load"] = String(load, 2);
  readings["max"] = String(max_load, 2);
  String jsonObj = JSON.stringify(readings);
  return jsonObj;
}

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}
