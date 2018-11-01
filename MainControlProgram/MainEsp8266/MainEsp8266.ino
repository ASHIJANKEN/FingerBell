#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <string.h>

// Status number for sendHttpResponse
#define SET_ROUTER_INFO 0
#define API_RESPONSE    1
#define SET_RING_TIME   2
#define API_ERROR       3
#define NOT_FOUND       4

// Status of Pro Mini
#define SET_ROUTER_CONFIG 0
#define CONNECT_ROUTER    1
#define API_SET_TIMER     2
#define API_RUN_TIMER     3

// Pins for UART communication
#define UART_SIGNAL_IN 13
#define UART_SIGNAL_OUT 14

// Pins for reading status of Pro Mini
#define STATUS_1 4
#define STATUS_0 5

const char* mdns_local_name = "fingerbell";
int status = 0;

WiFiServer server(80);

void setup(){
    // Initialize pins
	pinMode(STATUS_0, INPUT);
    pinMode(STATUS_1, INPUT);
    pinMode(UART_SIGNAL_OUT, OUTPUT);
    pinMode(UART_SIGNAL_IN, INPUT);

	Serial.begin(9600);

    delay(3000);

    status = readProMiniStatus();
}

void loop(){
	switch(status){
		case SET_ROUTER_CONFIG:
			setRouterConfig();
			break;
		case CONNECT_ROUTER:
			connectRouter();
			break;
		case API_SET_TIMER:
			setTimerApi();
			break;
		case API_RUN_TIMER:
			runTimerApi();
			break;
	}

    // Wait until status changes
    while(readProMiniStatus() == status){
    }
    status = readProMiniStatus();
}

void setRouterConfig(){
    clearSerialBuffer();

    char ssid_ap[] = "FingerBell_AP";
    char password_ap[] = "fingerbell_1234";
    bool set_ssid_password = false;

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid_ap, password_ap);

    MDNS.begin(mdns_local_name);

    server.begin();

    IPAddress myIP = WiFi.softAPIP();

    Serial.println('OK');

    sendUartReady();

    while(true){
        WiFiClient client = server.available();

        // Client send request?
        if(!client){
            continue;
        }

        while(client.connected()){
            delay(10);
            if(client.available() <= 0){
                continue;
            }

            char q_char[150] = {'\0'};
            for(int i = 0; client.available(); i++){
                q_char[i] = client.read();
                if(i > 5 && strncmp(q_char + i - 6, " HTTP/", 6) == 0){
                    break;
                }
            }
            String query = q_char;
            // Serial.print("query:");
            // Serial.println(query);

            // Read rest of request
            while(client.available()){
                client.read();
            }

            if(query.indexOf("/") != -1 || query.indexOf("/?") != -1) {
                sendHttpResponse(client, SET_ROUTER_INFO, q_char);
                if(query.indexOf("Set") != -1){
                    set_ssid_password = true;
                }
            }else{
                sendHttpResponse(client, NOT_FOUND, q_char);
            }
            break;
        }

        // delay(10);
        client.stop();
        // Serial.println(F("Done with client"));
        if(set_ssid_password == true){
            return;
        }
    }
}

void connectRouter(){
    clearSerialBuffer();

    // Wait for ProMini to send router's info
    waitUartReceive();

    char buf[150] = {'\0'};
    for(int i = 0; Serial.available() > 0; i++){
        buf[i] = Serial.read();
    }
    String s = buf;

    String ssid_str = subString(buf, "SSID:", " ");
    String password_str = subString(buf, "PASSWORD:", " ");

    char* ssid = new char[ssid_str.length() + 2];
    char* password = new char[password_str.length() + 2];
    ssid_str.toCharArray(ssid, ssid_str.length() + 1);
    password_str.toCharArray(password, password_str.length() + 1);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    MDNS.begin(mdns_local_name);

    // Start server.
    server.begin();

    // Notify ProMini that esp is ready
    Serial.println("OK");

    sendUartReady();
}

void setTimerApi(){
    clearSerialBuffer();
    delay(100);

    while(1){
        if(readProMiniStatus() != API_SET_TIMER){
            return;
        }

        // Check if a client has connected
        WiFiClient client = server.available();
        if(!client){
            continue;
        }

        // Check client is connected
        while(client.connected()){
            delay(10);
            // Client send request?
            if(client.available() <= 0){
                continue;
            }

            char buf[100] = {'\0'};
            for(int i = 0; client.available(); i++){
                buf[i] = client.read();
                if(i > 5 && strncmp(buf + i - 6, " HTTP/", 6) == 0){
                    break;
                }
            }

            // Read rest of request
            while(client.available()){
                client.read();
            }

            String query = buf;

            // Respond to request
            if(query.indexOf("/?set_ring") != -1) {
                sendHttpResponse(client, SET_RING_TIME, buf);
            }else{
                sendHttpResponse(client, API_ERROR, buf);
            }
            break;
        }
        delay(10);
        client.stop();
    }
}

void runTimerApi(){
    clearSerialBuffer();

    while(1){
        if(readProMiniStatus() != API_RUN_TIMER){
            return;
        }

        // Check if a client has connected
        WiFiClient client = server.available();
        if(!client){
            continue;
        }

        // Check client is connected
        while(client.connected()){
            // Client send request?
            if(client.available() <= 0){
                continue;
            }

            char buf[60] = {'\0'};
            for(int i = 0; client.available(); i++){
                buf[i] = client.read();
                if(i > 5 && strncmp(buf + i - 6, " HTTP/", 6) == 0){
                    break;
                }
            }

            // Read rest of request
            while(client.available()){
                client.read();
            }

            String query = buf;

            // Respond to request
            if(query.indexOf("get_elapsed_t") != -1 || query.indexOf("get_ring_t") != -1) {
                sendHttpResponse(client, API_RESPONSE, buf);
            }else{
                sendHttpResponse(client, API_ERROR, buf);
            }
            break;

        }
        delay(10);
        client.stop();
    }
}

int readProMiniStatus(){
	return (digitalRead(STATUS_1) << 1) | digitalRead(STATUS_0);
}

void sendHttpResponse(WiFiClient client, int status, char* query){
    switch(status){
        case SET_ROUTER_INFO:{
            // Sending SET_ROUTER_INFO

            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-type:text/html\r\nConnection: close\r\n\r\n"));
            client.print(F("<!DOCTYPE html><html>"));
            client.print(F("<head><meta name='viewport' content='initial-scale=1.5'></head>"));
            client.print(F("<body><h1>Set SSID and password of a router.</h1><br>"));

            // Change output depends on a request
            String query_str = query;
            if(query_str.indexOf("Set") != -1){
                String ssid = subString(query, "ssid=", "&");
                String password = subString(query, "password=", "&");

                Serial.print("SSID:");
                Serial.print(ssid);
                Serial.print(" password:");
                Serial.print(password);
                Serial.print(" ");

                sendUartReady();

                client.println(F("Done.<br>"));
            }

            client.print(F("<form>SSID : <input type=\"text\" maxlength=\"32\" name='ssid' required><br>"));
            client.print(F("password : <input type=\"text\" maxlength=\"64\" name='password' required><br>"));
            client.print(F("<input type='submit' name='Set' value='Set'>"));
            client.print(F("</form></body></html>\r\n"));
            break;
        }
        case API_RESPONSE:{
            // Sending API_RESPONSE

            bool send_elapsed_t = false;

            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-type:application/json; charset=utf-8\r\nConnection: close\r\n\r\n"));

            client.print(F("{\r\n"));

            String query_str = query;
            if(query_str.indexOf("get_elapsed_t") != -1 || query_str.indexOf("get_ring_t") != -1){
                Serial.print("res");

                sendUartReady();

                waitUartReceive();

                char buf[70];
                for(int i = 0; Serial.available() > 0; i++){
                    buf[i] = Serial.read();
                }

                // Extract elapsed_time
                String elapsed = subString(buf, "e:", " ");
                // Extract ring1
                String ring1 = subString(buf, "r1:", " ");
                // Extract ring2
                String ring2 = subString(buf, "r2:", " ");
                // Extract ring3
                String ring3 = subString(buf, "r3:", " ");

                if(query_str.indexOf("get_elapsed_t") != -1){
                    send_elapsed_t = true;
                    client.print(F("    \"elapsed_t\": \""));
                    client.print(elapsed);
                    client.print(F("\""));
                }

                if(query_str.indexOf("get_ring_t") != -1){
                    if(send_elapsed_t == true){
                        client.print(F(",\r\n"));
                    }

                    client.print(F("    \"ring_t\": {\r\n       \"1\": \""));
                    client.print(ring1);
                    client.print(F("\",\r\n       \"2\": \""));
                    client.print(ring2);
                    client.print(F("\",\r\n       \"3\": \""));
                    client.print(ring3);
                    client.print(F("\"\r\n    }"));
                }
            }

            client.print(F("\r\n}\r\n"));
            break;

        }
        case SET_RING_TIME:{
            // Sending SET_RING_TIME

            String query_str = query;
            for(int i = 1; i < 4; i++){
                char key[12] = {'\0'};
                String key_str = String("set_ring" + String(i));
                key_str.toCharArray(key, 11);

                if(query_str.indexOf(key) != 1){
                    String tag = String(key_str + "=");
                    String ring_str = subString(query, tag, "&");
                    if(ring_str.indexOf("HTTP") != -1){
                        String ring_str = subString(query, tag, " ");
                    }
                    Serial.print(key_str);
                    Serial.print(":");
                    Serial.print(ring_str);
                    Serial.print(" ");
                }
            }

            sendUartReady();

            char buf[70];
            for(int i = 0; Serial.available() > 0; i++){
                buf[i] = Serial.read();
            }

            // Extract ring1
            String ring1 = subString(buf, "r1:", " ");
            // Extract ring2
            String ring2 = subString(buf, "r2:", " ");
            // Extract ring3
            String ring3 = subString(buf, "r3:", " ");


            // Sending SET_RING_TIME
            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-type:application/json; charset=utf-8\r\nConnection: close\r\n\r\n"));

            client.print(F("{\r\n    \"ring_t\": {\r\n       \"1\": \""));
            client.print(ring1);
            client.print(F(",\r\n       \"2\": \""));
            client.print(ring2);
            client.print(F(",\r\n       \"3\": \""));
            client.print(ring3);
            client.print(F("\r\n    }\r\n}\r\n"));

            break;
        }
        case API_ERROR:
            // Sending API_ERROR

            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-type:application/json; charset=utf-8\r\nConnection: close\r\n\r\n"));

            client.print(F("{\r\n  \"error\" : {\r\n    \"msg\" : \"Invalid requests\"\r\n  }\r\n}\r\n"));
            break;

        case NOT_FOUND:
            // Sending 404

            client.print(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
            client.print(F("404 Not Found\r\n"));
            break;
    }
}

void clearSerialBuffer(){
    while(Serial.available()){
        Serial.read();
    }
}

String subString(char* buf, String start_tag, String end_tag){
    String buf_str = buf;

    // unespace
    buf_str = urldecode(buf_str);

    // substring
    int start = buf_str.indexOf(start_tag) + start_tag.length();
    int end = buf_str.indexOf(end_tag, start);
    return buf_str.substring(start, end);

    // int start_indexof = buf_str.indexOf(start_tag);
    // int end = buf_str.indexOf(end_tag, start_indexof);

    // if(start_indexof == -1 || end == -1){
    //     return "";
    // }

    // int start = start_indexof + strlen(start_tag);
    // int length = end - start;
    // char* ret = new char[length];

    // strncpy(ret, buf + start, length);
    // String ret_str = ret;

    // Serial.print("ret_str: ");
    // Serial.println(ret_str);

    // return ret_str;
}

String urldecode(String str){
    String decodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
        if (c == '+'){
            decodedString+=' ';
        }else if (c == '%') {
            i++;
            code0=str.charAt(i);
            i++;
            code1=str.charAt(i);
            c = (h2int(code0) << 4) | h2int(code1);
            decodedString+=c;
        } else{
            decodedString+=c;
        }

        yield();
    }

   return decodedString;
}

unsigned char h2int(char c){
    if(c >= '0' && c <='9'){
        return ((unsigned char)c - '0');
    }
    if(c >= 'a' && c <='f'){
        return ((unsigned char)c - 'a' + 10);
    }
    if(c >= 'A' && c <='F'){
        return ((unsigned char)c - 'A' + 10);
    }
    return 0;
}

/* Send 'ready' status.
 * 1. Send UART_SIGNAL_OUT to notify that strings are already printed.
 * 2. UART_SIGNAL_IN goes to be high as an ProMini's ack after detecting UART_SIGNAL_OUT signal.
 * 3. Disable UART_SIGNAL_OUT signal.
 * 4. Make sure ProMini's signal goes to be low.
 */
void  sendUartReady(){
    digitalWrite(UART_SIGNAL_OUT, HIGH);

    while(digitalRead(UART_SIGNAL_IN) == LOW){
    }

    digitalWrite(UART_SIGNAL_OUT, LOW);

    while(digitalRead(UART_SIGNAL_IN) == HIGH){
    }
}

/* Wait for ProMini to be ready to send data.
 * 1. Wait for ProMini's signal to be high.
 * 2. Send UART_SIGNAL_OUT as an ack.
 * 3. Wait for ProMini's signal goes to be low.
 * 4. Disable UART_SIGNAL_OUT.
 */
void waitUartReceive(){
    while(digitalRead(UART_SIGNAL_IN) != HIGH){
    }
    digitalWrite(UART_SIGNAL_OUT, HIGH);

    while(digitalRead(UART_SIGNAL_IN) == HIGH){
    }
    digitalWrite(UART_SIGNAL_OUT, LOW);
}