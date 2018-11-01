#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

// Status number for sendHttpResponse
#define SET_ROUTER_INFO 0
#define API_RESPONSE    1
#define SET_RING_TIME   2
#define API_ERROR       3
#define NOT_FOUND       4

// Status of Pro Mini
#define DO_NOTHING -1
#define SET_ROUTER_CONFIG 0
#define CONNECT_ROUTER 1
#define API_SET_TIMER 2
#define API_RUN_TIMER 3

// Pins for reading status of Pro Mini
#define STATUS_0 5
#define STATUS_1 4

int status = API_SET_TIMER;

WiFiServer server(80);

void setup(){
    pinMode(STATUS_0, INPUT);
    pinMode(STATUS_1, INPUT);
    Serial.begin(9600);
}

void loop(){
    clearSerialBuffer();

    delay(100);

    // // Wait for ProMini to send router's info
    // while(!Serial.available()){
    // }

    // delay(500);

    // char buf[150] = {'\0'};
    // for(int i = 0; Serial.available() > 0; i++){
    //     buf[i] = Serial.read();
    // }

    // String ssid_str = subString(buf, "SSID:", " ");
    // String password_str = subString(buf, "PASSWORD:", " ");

    // char* ssid = new char[ssid_str.length() + 2];
    // char* password = new char[password_str.length() + 2];
    // ssid_str.toCharArray(ssid, ssid_str.length() + 1);
    // password_str.toCharArray(password, password_str.length() + 1);

    WiFi.mode(WIFI_STA);
    WiFi.begin("SSID", "PASS");

    MDNS.begin("fingerbell");

    // Start server.
    server.begin();

    // Notify ProMini that esp is ready
    Serial.println("OK");

    status = DO_NOTHING;







    status = API_SET_TIMER;



    Serial.println("set timer");




    clearSerialBuffer();
    delay(100);

    while(1){
        readProMiniStatus();
        if(status != API_RUN_TIMER){
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

void readProMiniStatus(){
    status = (digitalRead(STATUS_1) << 1) | digitalRead(STATUS_0);
}

void sendHttpResponse(WiFiClient client, int status, char* query){
    switch(status){
        case SET_ROUTER_INFO:{
            // Sending SET_ROUTER_INFO

            Serial.println("SET_ROUTER_INFO");

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
                Serial.println(" ");

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
                Serial.println("res");

                while(!Serial.available()){
                }

                delay(100);

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
            Serial.println();

            while(!Serial.available()){
            }

            delay(100);

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
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}