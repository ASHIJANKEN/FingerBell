#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "SSID";
const char* password = "PASS";

WiFiServer server(80);

void setup(void) {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Set up mDNS responder:
  if (!MDNS.begin("fingerbell")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  server.begin();
  Serial.println("TCP server started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

}

void loop(void) {
    WiFiClient client = server.available();

    // Client send request?
    if(!client){
        return;
    }

    while(client.connected()){
        if(!client.available()){
            delay(1);
        }

        char q_char[150] = {'\0'};
        char http[] = "HTTP/";
        for(int i = 0; client.available(); i++){
            q_char[i] = client.read();
            if(strncmp(q_char + i - 6, " HTTP/", 6) == 0){
                break;
            }
        }
        Serial.println(q_char);

        // Read rest of request
        while(client.available()){
            client.read();
        }

        client.print(F("HTTP/1.1 200 OK\r\n"));
        client.print(F("Content-type:text/html\r\nConnection: close\r\n\r\n"));
        client.print(F("hello from esp8266!\r\n"));

        break;
    }

    // delay(10);
    client.stop();
    Serial.println(F("Done with client"));
}