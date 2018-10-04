#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <stdio.h>
#include <string.h>
#include <Servo.h>
#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

// // Status number for sendHttpResponse
#define SET_ROUTER_INFO 0
#define API_RESPONSE    1
#define SET_RING_TIME   2
#define API_ERROR       3
#define NOT_FOUND       4

// Pin assign of a button.
#define BTN_PIN 12

// Pin assign of a servo.
#define SERVO_PIN 11

// Pin assign of a lcd.
#define RS     14
#define ENABLE 15
#define DB4    8
#define DB5    9
#define DB6    10
#define DB7    13

// For servo
#define MIN_ANGLE 90
#define MAX_ANGLE 180

// Modes
#define SET_TIMER   0
#define START_TIMER 1

//Button state
#define NOT_PRESSED         0
#define PRESSED             1
#define PRESSING            2
#define RELEASED            3
#define LONG_PRESSED        4
#define LONG_PRESSING       5
#define LONG_PRESS_RELEASED 6

// Waiting time to avoid chattering.
#define WAIT_FOR_BOUNCE_MSEC 100

// Address of EEPROM for keeping ringing time.
#define RING_ADDR_OFFSET 0

// Address of router's SSID and PASSWORD
#define SSID_OFFSET 6 // 32 characters at maximum
#define PASSWORD_OFFSET 40 // 64 characters at maximum

// Time when the bell rings.
int ring_seconds[3] = {0, 0, 0};

// SSID and password for connecting to Wi-Fi router
int ssid_length = 0;
int password_length = 0;

// TCP server at port 80 will respond to HTTP requests
WiFiEspServer server(80);

// Serial port for ESP-WROOM-02
SoftwareSerial Serial1(A2, A3); // RX, TX

// Create objects.
LiquidCrystal lcd(RS, ENABLE, DB4, DB5, DB6, DB7);
Servo myservo;

void setup(){
    // Pin assign of rotary encoders(A/B phases)
    int ENCODERS_PINS[3][2] = {{2, 3},
                                {4, 5},
                                {6, 7}};

    // Display initializing message.
    lcd.begin(16, 2); //16 chars × 2 rows
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("STARTING BELL");
    lcd.setCursor(0, 1);
    lcd.print("RINGING MACHINE");

    // Initialize serial ports & ESP module
    Serial.begin(9600);  // Initialize serial for debugging
    Serial1.begin(115200);
    delay(10);
    Serial1.println("AT+UART_CUR=9600,8,1,0,0");  // Change ESP's baudrate
    delay(20);
    Serial1.begin(9600); // Initialize serial for ESP module
    WiFi.init(&Serial1); // Initialize ESP module
    // check for the presence of the shield
    if (WiFi.status() == WL_NO_SHIELD) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ESP-WROOM-02");
        lcd.setCursor(0, 1);
        lcd.print("is not present");
        while (true); // don't continue
    }

    //Initialize the servo.
    myservo.attach(SERVO_PIN);

    // Get ringing time from EEPROM.
    Serial.println("-----------------");
    for(int i = 0; i < 3; i++){
        Serial.println(RING_ADDR_OFFSET + i*2);
        int upper = EEPROM.read(RING_ADDR_OFFSET + i*2);
        Serial.println(EEPROM.read(RING_ADDR_OFFSET + i*2));
        int lower = EEPROM.read(RING_ADDR_OFFSET + i*2 + 1);
        Serial.println(EEPROM.read(RING_ADDR_OFFSET + i*2 + 1));
        ring_seconds[i] = (int)(upper << 8 | lower);
    }

    // Get SSID and password from EEPROM.
    ssid_length = EEPROM.read(SSID_OFFSET);

    // Disable an internal LED.
    pinMode(13,OUTPUT);
    digitalWrite(13,LOW);

    // Initialize rotary encoders.
    pinMode(ENCODERS_PINS[0][0],INPUT_PULLUP);
    pinMode(ENCODERS_PINS[0][1],INPUT_PULLUP);
    pinMode(ENCODERS_PINS[1][0],INPUT_PULLUP);
    pinMode(ENCODERS_PINS[1][1],INPUT_PULLUP);
    pinMode(ENCODERS_PINS[2][0],INPUT_PULLUP);
    pinMode(ENCODERS_PINS[2][1],INPUT_PULLUP);

    // Initialize a button.
    pinMode(BTN_PIN,INPUT_PULLUP);

    // Initialize a servo.
    pinMode(SERVO_PIN,OUTPUT);

    lcd.clear();
}

void loop(){
    // Serial.println("Entered setRouterConfig()");

    // // Display "setup mode"
    // lcd.clear();
    // lcd.setCursor(0, 0);
    // lcd.print("ENTERED");
    // lcd.setCursor(0, 1);
    // lcd.print("SETUP MODE...");

    // SSID and password for an its own access point
    char ssid[] = "SSID";
    char pass[] = "PASS";

    // attempt to connect to WiFi network
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    WiFi.begin(ssid, pass);

    // Start server.
    server.begin();

    delay(1000);

    // Get IP address
    IPAddress ap_ip = WiFi.localIP();

    // Show some info
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("IP Address: ");
    Serial.println(ap_ip);
    Serial.println();

    // Show IP address to LCD
    String ip_str = String(ap_ip[0]) + "." + String(ap_ip[1]) + "." + String(ap_ip[2]) + "." + String(ap_ip[3]);
    int ip_len = ip_str.length() + 1;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ACCESS THIS IP:");
    lcd.setCursor(0, 1);
    lcd.print(ip_str);

    // process for receive data from clients
    int ip_chars_len = ip_len + 3;
    char ip_chars[ip_chars_len];
    ip_str.toCharArray(ip_chars, ip_chars_len);
    unsigned long last_time = micros();
    int rotate_pos = 0;
    // RingBuffer buf(150);
    while(1){
        // Show IP address with rotation on lcd
        unsigned long cur_time = micros();
        if(cur_time - last_time > 500){
            char print_chars[ip_chars_len];
            memcpy(print_chars, ip_chars + (ip_chars_len - rotate_pos + 1), rotate_pos);
            memcpy(print_chars + rotate_pos, ip_chars, ip_chars_len - rotate_pos);

            // lcd.clear();
            // lcd.setCursor(0, 0);
            // lcd.print("ACCESS THIS IP:");
            lcd.setCursor(0, 1);
            lcd.print(print_chars);

            last_time = cur_time;
            rotate_pos = (rotate_pos + 1) % ip_chars_len;
        }

        // Check if a client has connected
        WiFiEspClient client = server.available();
        if(!client){
            continue;
        }

        Serial.println("New client");
        char buf[60] = {'\0'};
        int buf_ptr = 0;

        // Check client is connected
        bool set_ssid_password = false;
        while(client.connected()){
            // Client send request?
            if(client.available()){
                buf[buf_ptr] = client.read();

                if(strncmp(buf + buf_ptr - 6, " HTTP/", 6) == 0){
                    // Read rest of request
                    while(client.available()){
                        client.read();
                    }

                    String query = buf;
                    Serial.print("query: ");
                    Serial.println(query);

                    // Respond to request
                    if(query.indexOf("/?get_") != -1) {
                        sendHttpResponse(client, API_RESPONSE, query);
                    }else{
                        sendHttpResponse(client, API_ERROR, query);
                    }
                    break;
                }

                buf_ptr++;
            }
        }
        delay(10);
        client.stop();
        Serial.println("Done with client");
        // if(set_ssid_password == true) return true;
    }
}

void sendHttpResponse(WiFiEspClient client, int status, String query){
    switch(status){
        case SET_ROUTER_INFO:
            Serial.println("Sending SET_ROUTER_INFO");

            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-type:text/html\r\nConnection: close\r\n\r\n"));
            client.print(F("<!DOCTYPE html><html>"));
            client.print(F("<head><meta name='viewport' content='initial-scale=1.5'></head>"));
            client.print(F("<body><h1>Set SSID and password of a router.</h1><br>"));

            // Change output depends on a request
            if(query.indexOf("Set") != -1){
                int ssid_addr_start = query.indexOf("ssid") + 5;
                int ssid_addr_end = query.indexOf("&", ssid_addr_start);
                int pass_addr_start = query.indexOf("password") + 9;
                int pass_addr_end = query.indexOf("&", pass_addr_start);
                String ssid = query.substring(ssid_addr_start, ssid_addr_end);
                String password = query.substring(pass_addr_start, pass_addr_end);
                Serial.print("SSID : ");
                Serial.println(ssid);
                Serial.print("password : ");
                Serial.println(password);

                char ssid_router[40];
                char password_router[70];
                ssid.toCharArray(ssid_router, ssid.length());
                password.toCharArray(password_router, password.length());

                // Write SSID and password to EEPROM
                EEPROM.write(SSID_OFFSET, ssid.length());
                // Write SSID and password to EEPROM
                for (int i = 0; i < ssid.length(); i++){
                    EEPROM.write(SSID_OFFSET + 1 + i, ssid_router[i] - 0x30);
                }

                EEPROM.write(PASSWORD_OFFSET, password.length());
                for (int i = 0; i < password.length(); i++){
                    EEPROM.write(PASSWORD_OFFSET + 1 + i, password_router[i] - 0x30);
                }

                client.println(F("Set SSID and password correctly.<br>"));
            }

            client.print(F("<form>SSID : <input type=\"text\" maxlength=\"32\" name='ssid' required><br>"));
            client.print(F("password : <input type=\"text\" maxlength=\"64\" name='password' required><br>"));
            client.print(F("<input type='submit' name='Set' value='Set'>"));
            client.print(F("</form></body></html>\r\n"));
            break;

        case API_RESPONSE:{
            Serial.println(F("Sending API_RESPONSE"));
            bool send_elapsed_t = false;

            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-type:application/json; charset=utf-8\r\nConnection: close\r\n\r\n"));

            client.print(F("{\r\n"));
            if(query.indexOf("get_elapsed_t") != -1){
                send_elapsed_t = true;
                client.print(F("    \"elapsed_t\": \""));
                // client.print(args[1]);
                client.print(1253);
                client.print(F("\""));
            }

            if(query.indexOf("get_ring_t") != -1){
                if(send_elapsed_t == true){
                    client.print(F(",\r\n"));
                }

                client.print(F("    \"ring_t\": {\r\n       \"1\": \""));
                client.print(ring_seconds[0]);
                client.print(F("\",\r\n       \"2\": \""));
                client.print(ring_seconds[1]);
                client.print(F("\",\r\n       \"3\": \""));
                client.print(ring_seconds[2]);
                client.print(F("\"\r\n    }"));
            }

            client.print(F("\r\n}\r\n"));
            break;
        }
        case SET_RING_TIME:

            if(query.indexOf("set_ring") != -1){
                if(query.indexOf("set_ring1") != 1){
                    int addr_start = query.indexOf("set_ring1") + 9;
                    int addr_end = query.indexOf("&", addr_start);
                    if(addr_end == -1){
                        query.indexOf(" ", addr_start);
                    }
                    ring_seconds[0] = query.substring(addr_start, addr_end).toInt();
                }
                if(query.indexOf("set_ring2") != 1){
                    int addr_start = query.indexOf("set_ring2") + 9;
                    int addr_end = query.indexOf("&", addr_start);
                    if(addr_end == -1){
                        query.indexOf(" ", addr_start);
                    }
                    ring_seconds[1] = query.substring(addr_start, addr_end).toInt();
                }
                if(query.indexOf("set_ring3") != 1){
                    int addr_start = query.indexOf("set_ring3") + 9;
                    int addr_end = query.indexOf("&", addr_start);
                    if(addr_end == -1){
                        query.indexOf(" ", addr_start);
                    }
                    ring_seconds[2] = query.substring(addr_start, addr_end).toInt();
                }
            }

            Serial.println(F("Sending SET_RING_TIME"));

            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-type:application/json; charset=utf-8\r\nConnection: close\r\n\r\n"));

            client.print(F("{\r\n    \"ring_t\": {\r\n       \"1\": \""));
            client.print(ring_seconds[0]);
            client.print(F(",\r\n       \"2\": \""));
            client.print(ring_seconds[1]);
            client.print(F(",\r\n       \"3\": \""));
            client.print(ring_seconds[2]);
            client.print(F("\r\n    }\r\n}\r\n"));

            break;

        case API_ERROR:
            Serial.println(F("Sending API_ERROR"));

            client.print(F("HTTP/1.1 200 OK\r\n"));
            client.print(F("Content-type:application/json; charset=utf-8\r\nConnection: close\r\n\r\n"));

            client.print(F("{\r\n  \"error\" : {\r\n    \"msg\" : \"Invalid requests\"\r\n  }\r\n}\r\n"));
            break;

        case NOT_FOUND:
            Serial.println(F("Sending 404"));

            client.print(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));
            client.print(F("404 Not Found\r\n"));
            break;
    }
}

bool subString(char* dest, char* str_origin, char* start_tag, char* end_tag){
    int addr_start = -1;
    int addr_end = -1;
    int start_tag_len = strlen(start_tag);
    int end_tag_len = strlen(end_tag);

    int i = 0;
    for(i = 0; str_origin[i] != '\0'; i++){
        if(strncmp(str_origin + i, start_tag, start_tag_len) == 0){
            addr_start = i + start_tag_len;
            i += start_tag_len;
            break;
        }
    }

    i++;

    for(; str_origin[i] != '\0'; i++){
        if(strncmp(str_origin + i, end_tag, end_tag_len) == 0){
            addr_end = i;
            break;
        }
    }

    if(addr_start == -1 || addr_end == -1){
        return false;
    }

    memcpy(dest, str_origin + addr_start, addr_end - addr_start);
    return true;
}
