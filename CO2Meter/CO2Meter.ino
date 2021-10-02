/**
 * Secure Write Example code for InfluxDBClient library for Arduino
 * Enter WiFi and InfluxDB parameters below
 *
 * Demonstrates connection to any InfluxDB instance accesible via:
 *  - unsecured http://...
 *  - secure https://... (appropriate certificate is required)
 *  - InfluxDB 2 Cloud at https://cloud2.influxdata.com/ (certificate is preconfigured)
 * Measures signal level of the actually connected WiFi network
 * This example demonstrates time handling, secure connection and measurement writing into InfluxDB
 * Data can be immediately seen in a InfluxDB 2 Cloud UI - measurement wifi_status
 **/


#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#include <Arduino.h>
#include "MHZ19.h"
#include <SoftwareSerial.h>

#include "secrets.h"

// Set timezone string according to https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
// Examples:
//  Pacific Time: "PST8PDT"
//  Eastern: "EST5EDT"
//  Japanesse: "JST-9"
//  Central Europe: "CET-1CEST,M3.5.0,M10.5.0/3"
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

#define RX_PIN 14                                          // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 16                                          // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600                                      // Device to MH-Z19 Serial baudrate (should not be changed)

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

// Data point
Point sensor("CO2_Sensor");

MHZ19 myMHZ19;
SoftwareSerial mySerial(RX_PIN, TX_PIN); 

int CO2 = 0;
int8_t Temp = 0;

void setup() {

  // Setup wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  while (wifiMulti.run() != WL_CONNECTED) {
    delay(500);
  }

  // Add tags
  sensor.addTag("device", DEVICE);
  sensor.addTag("location", "home1");

  // Alternatively, set insecure connection to skip server certificate validation 
  //client.setInsecure();

  // Accurate time is necessary for certificate validation and writing in batches
  // For the fastest time sync find NTP servers in your area: https://www.pool.ntp.org/zone/
  // Syncing progress and the time will be printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  mySerial.begin(BAUDRATE);
  myMHZ19.begin(mySerial);
}

void loop() {
  // Store measured value into point
  sensor.clearFields();

  CO2 = myMHZ19.getCO2();                             // Request CO2 (as ppm)
  Temp = myMHZ19.getTemperature();                     // Request Temperature (as Celsius)
  
  // Report RSSI of currently connected network
  sensor.addField("rssi", WiFi.RSSI());
  sensor.addField("CO2", CO2);
  sensor.addField("temperature", Temp);

  // If no Wifi signal, try to reconnect it
  if (wifiMulti.run() != WL_CONNECTED) {
    delay(1);
  }

  client.writePoint(sensor);

  //Wait 10s
  delay(10000);
}
