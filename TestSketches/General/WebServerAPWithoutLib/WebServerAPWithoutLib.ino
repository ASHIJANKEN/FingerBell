
/*
WiFiEsp example: WebServerAP

A simple web server that shows the value of the analog input
pins via a web page using an ESP8266 module.
This sketch will start an access point and print the IP address of your
ESP8266 module to the Serial monitor. From there, you can open
that address in a web browser to display the web page.
The web page will be automatically refreshed each 20 seconds.

For more details see: http://yaab-arduino.blogspot.com/p/wifiesp.html
*/

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
RingBuffer buf(8);

#define MAX_SOCK_NUM 10
#define NUMESPTAGS2 5
static int16_t state[MAX_SOCK_NUM];
static uint16_t server_port[MAX_SOCK_NUM];

const char* ESPTAGS2[] =
{
    "\r\nOK\r\n",
    "\r\nERROR\r\n",
    "\r\nFAIL\r\n",
    "\r\nSEND OK\r\n",
    " CONNECT\r\n"
};


void clearUartBuffer(Stream* serial_port){
    while(serial_port->available()){
        // serial_port->read();
        char c = serial_port->read();
        Serial.print(c);
    }
}

int printUartBuffer(Stream* serial_port, int timeout_ms){
    RingBuffer buf(15);
    unsigned long start_ms = millis();
    char ok[3] = {'\0'};
    while(millis() - start_ms < timeout_ms){
        if(serial_port->available()){
            char c = (char)serial_port->read();
            buf.push(c);
            Serial.print(c);

            for(int i=0; i<NUMESPTAGS2; i++){
                if (buf.endsWith(ESPTAGS2[i])){
                    return i;
                }
            }
        }
    }

    Serial.println("[timeout]printUartBuffer()");
    return 1;
}

int getStatusUartBuffer(Stream* serial_port, int timeout_ms){
    RingBuffer buf(15);
    unsigned long start_ms = millis();
    int p = 0;
    char ok[3] = {'\0'};

    while(millis() - start_ms < timeout_ms){
        if(serial_port->available()){
            buf.push((char)serial_port->read());

            for(int i=0; i<NUMESPTAGS2; i++){
                if (buf.endsWith(ESPTAGS2[i])){
                    return i;
                }
            }
        }
    }

    Serial.println("[timeout]getStatusUartBuffer()");
    return 1;
}

void getCharsUartBuffer(Stream* serial_port, char* dest, int buf_size, char* start_tag, char* end_tag, int timeout_ms){
    RingBuffer buf(buf_size);
    unsigned long start_ms = millis();
    bool found_stag = false;

    while(millis() - start_ms < timeout_ms){
        if(serial_port->available()){
            buf.push((char)serial_port->read());

            if(buf.endsWith(start_tag)){
                found_stag = true;
                break;
            }
        }
    }

    if(!found_stag){
        Serial.println("[timeout]getCharsUartBuffer()");
        strcpy(dest, "ERROR");
        return;
    }

    buf.init();
    while(millis() - start_ms < timeout_ms){
        if(serial_port->available()){
            buf.push((char)serial_port->read());

            if(buf.endsWith(end_tag)){
                buf.getStr(dest, strlen(end_tag));
                return;
            }
        }
    }

    Serial.println("[timeout]getCharsUartBuffer()");
    strcpy(dest, "ERROR");
    return;
}

void sendEspCmd(String cmd, Stream* serial_port){
    clearUartBuffer(serial_port);
    serial_port->println(cmd);
    delay(100);
}

void initEsp(){
    sendEspCmd("ATE0", &Serial1);  // disable echo of commands
    sendEspCmd("AT+CWMODE=1", &Serial1);  // set station mode
    sendEspCmd("AT+CIPMUX=1", &Serial1);  // set multiple connections mode
    sendEspCmd("AT+CIPDINFO=1", &Serial1);  // Show remote IP and port with "+IPD"
    sendEspCmd("AT+CWAUTOCONN=0", &Serial1);  // Disable autoconnect
    sendEspCmd("AT+CWDHCP=1,1", &Serial1);  // enable DHCP
    delay(200);
}

void setup(){
    Serial.begin(9600);   // initialize serial for debugging
    Serial1.begin(115200);    // initialize serial for ESP module
    delay(10);
    sendEspCmd("AT+UART_CUR=9600,8,1,0,0", &Serial1);
    Serial1.begin(9600);
    initEsp();

    Serial.print("Attempting to start AP ");
    Serial.println(ssid_ap);

    // start access point
    sendEspCmd("AT+CWMODE_CUR=2", &Serial1);  // set AP mode
    sendEspCmd("AT+CWSAP_CUR=\"" + ssid_ap + "\",\"" + password_ap + "\",10,4", &Serial1);  // start access point
    delay(1000);
    if(getStatusUartBuffer(&Serial1, 1000) == 0){
        Serial.println("Started access point.");
    }else{
        Serial.println("Failed to start access point.");
    }

    // Show IP address of access point
    sendEspCmd("AT+CIPAP?", &Serial1);  // getIpAddressAP
    char ap_ip[16] = {'\0'};
    getCharsUartBuffer(&Serial1, ap_ip, 25, "ip:\"", "\"", 1000);
    Serial.println("ap_ip is ");
    Serial.println(ap_ip);
    getStatusUartBuffer(&Serial1, 2000);

    // start the web server on port 80
    Serial.println("Attempting to start server");
    state[1] = 1;
    sendEspCmd("AT+CIPSERVER=1,80", &Serial1);
    delay(1000);
    // printUartBuffer(&Serial1, 1500);
    if(getStatusUartBuffer(&Serial1, 1000) == 0){
        Serial.println("Started server.");
    }else{
        Serial.println("Failed to start server.");
    }
}


void loop()
{

    Serial.println("loop");
    while(1){}
    // WiFiEspClient client = server.available();  // listen for incoming clients

    // if (client) {                               // if you get a client,
    // Serial.println("New client");             // print a message out the serial port
    // buf.init();                               // initialize the circular buffer
    // while (client.connected()) {              // loop while the client's connected
    //   if (client.available()) {               // if there's bytes to read from the client,
    //     char c = client.read();               // read a byte, then
    //     buf.push(c);                          // push it to the ring buffer

    //     // you got two newline characters in a row
    //     // that's the end of the HTTP request, so send a response
    //     if (buf.endsWith("\r\n\r\n")) {
    //       sendHttpResponse(client);
    //       break;
    //     }
    //   }
    // }

    // // give the web browser time to receive the data
    // delay(10);

    // // close the connection
    // client.stop();
    // Serial.println("Client disconnected");
    // }
    // }

    // void sendHttpResponse(WiFiEspClient client)
    // {
    // client.print(
    // "HTTP/1.1 200 OK\r\n"
    // "Content-Type: text/html\r\n"
    // "Connection: close\r\n"  // the connection will be closed after completion of the response
    // "Refresh: 20\r\n"        // refresh the page automatically every 20 sec
    // "\r\n");
    // client.print("<!DOCTYPE HTML>\r\n");
    // client.print("<html>\r\n");
    // client.print("<h1>Hello World!</h1>\r\n");
    // client.print("Requests received: ");
    // client.print(++reqCount);
    // client.print("<br>\r\n");
    // client.print("Analog input A0: ");
    // client.print(analogRead(0));
    // client.print("<br>\r\n");
    // client.print("</html>\r\n");
}
