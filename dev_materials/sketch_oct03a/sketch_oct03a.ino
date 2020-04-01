#include <WiFiEsp.h>
// #include <WiFiEspServer.h>
// #include <WiFiEspClient.h>
// #include "RingBuffer.h"

// Emulate Serial on pins 6/7 if not present
#ifndef HAVE_HWSerial
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#endif

String ssid_ap = "TwimEsp";         // your network SSID (name)
String password_ap = "12345678";        // your network password
// int status = WL_IDLE_STATUS;     // the Wifi radio's status
int reqCount = 0;                // number of requests received

// WiFiEspServer server(80);

// use a ring buffer to increase speed and reduce memory allocation
//RingBuffer buf(8);

#define MAX_SOCK_NUM 10
#define NUMESPTAGS2 5
static int16_t state[MAX_SOCK_NUM];
static uint16_t server_port[MAX_SOCK_NUM];



void setup() {
  // put your setup code here, to run once:
  SoftwareSerial Serial1(6, 7);
  Serial1.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

}
