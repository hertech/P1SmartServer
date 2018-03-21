/* 
  P1 Server - Based on UART to Telnet Server for esp8266
  Can be used as remote P1 Smart Metering in Domoticz.
  
  Copyright (c) 2018 Alain Hertog. All rights reserved.
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <ESP8266WiFi.h>

//how many clients should be able to connect to this ESP8266
#define MAX_SRV_CLIENTS 2

const char* ssid = "<SSID>";
const char* password = "<PASSWORD>";

#define MAXLINELENGTH 255 // longest normal line is 47 char but it can grow to 255 see DSMR 5.0 specs (+3 for \r\n\0)
char telegram[MAXLINELENGTH];
#define LED D4 // when client connects builtin LED will be on

WiFiServer server(23);
WiFiClient serverClients[MAX_SRV_CLIENTS]; // set to 1 if only used for Domoticz

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(D8, OUTPUT); // Data request
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to "); Serial.println(ssid);
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
    Serial.print("Could not connect to"); Serial.println(ssid);
    while(1) delay(500);
  }
  //start UART and the server
  Serial.begin(115200);
  server.begin();
  server.setNoDelay(true);
  
  Serial.print("nc  ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");
  digitalWrite(LED, HIGH);
}

void loop() {
  uint8_t i;
  //check if there are any new clients
  if (server.hasClient()){
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()){
        if(serverClients[i])
        {
          serverClients[i].stop();
          digitalWrite(LED, HIGH);
          digitalWrite(D8, LOW);
        }
        serverClients[i] = server.available();
        Serial.print("New client: ");
        Serial.println(i);
        digitalWrite(LED, LOW);
        digitalWrite(D8, HIGH); 
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }
  else
  {
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
      //find free/disconnected spot
      if (serverClients[i] && !serverClients[i].connected()){
        serverClients[i].stop();
        Serial.print("Client diconnected: ");
        Serial.println(i);
        digitalWrite(LED, HIGH);
        digitalWrite(D8, LOW); 
      }
    }
  }

  //check UART for data
  if(Serial.available()){
    memset(telegram, 0, sizeof(telegram));
    while (Serial.available()) {
      int len = Serial.readBytesUntil('\n', telegram, MAXLINELENGTH);
      telegram[len] = '\n';
      telegram[len+1] = 0;
      yield();
    //push UART data to all connected telnet clients
      for(i = 0; i < MAX_SRV_CLIENTS; i++){
        if (serverClients[i] && serverClients[i].connected()){
          serverClients[i].write((const uint8_t*)telegram, len+1);
          yield();
        }
      }
    }
  }
}
