#include <LiquidCrystal.h>
#include <EEPROM.h>
// #include <stdio.h>
// #include <string.h>
#include <Servo.h>
#include <WiFiEsp.h>
#include <SoftwareSerial.h>


// Pin assign of rotary encoders(A/B phases)
const int ENCODERS_PINS[3][2] = {{2, 3},
                                {4, 5},
                                {6, 7}};

// Pin assign of a button.
const int BTN_PIN = 12;

// Pin assign of a servo.
const int SERVO_PIN = 11;

// Pin assign of a lcd.
const int RS       = 14;
const int ENABLE   = 15;
const int DB4      = 8;
const int DB5      = 9;
const int DB6      = 10;
const int DB7      = 13;

// For servo
const int MIN_ANGLE = 90;
const int MAX_ANGLE = 180;

// Modes
const int SET_TIMER   = 0;
const int START_TIMER = 1;

//Button state
const int NOT_PRESSED         = 0;
const int PRESSED             = 1;
const int PRESSING            = 2;
const int RELEASED            = 3;
const int LONG_PRESSED        = 4;
const int LONG_PRESSING       = 5;
const int LONG_PRESS_RELEASED = 6;

// Waiting time to avoid chattering.
const int WAIT_FOR_BOUNCE_MSEC = 100;

// Address of EEPROM for keeping ringing time.
const int RING_ADDR_OFFSET = 0;

// Address of router's SSID and PASSWORD
const int SSID_OFFSET = 6; // 32 characters at maximum
const int PASSWORD_OFFSET = 40; // 64 characters at maximum

// Time when the bell rings.
int ring_seconds[3] = {0, 0, 0};

// Present encoders state
int enc_state[3][2] = {{0, 0},
                        {0, 0},
                        {0, 0}};

// Previous encoders state
int enc_prev_state[3][2] = {{0, 0},
                            {0, 0},
                            {0, 0}};

// SSID and password for an its own access point
const char ssid_ap[] = "FingerBell_AP";
const char password_ap[] = "fingerbell_1234";

// SSID and password for connecting to Wi-Fi router
char ssid_router[40];
char password_router[70];
int ssid_length = 0;
int password_length = 0;

// TCP server at port 80 will respond to HTTP requests
WiFiEspServer server(80);
SoftwareSerial Serial1(6, 7); // RX, TX

// Create objects.
LiquidCrystal lcd(RS, ENABLE, DB4, DB5, DB6, DB7);
Servo myservo;





void clearUartBuffer(Stream* serial_port){
    while(serial_port->available()){
        serial_port->read();
    }
}

void printUartBuffer(Stream* serial_port, int timeout_ms){
    unsigned long start_ms = millis();
    char ok[3] = {'\0'};
    while(millis() - start_ms < timeout_ms){
        if(serial_port->available()){
            ok[0] = ok[1];
            ok[1] = (char)serial_port->read();
            Serial.write(ok[1]);

            if(strcmp(ok, "OK") == 0){
                break;
            }
      }
    }
}

void getCharsUartBuffer(Stream* serial_port, char* dest, int timeout_ms){
    unsigned long start_ms = millis();
    int p = 0;
    char ok[3] = {'\0'};

    while(millis() - start_ms < timeout_ms){
        if(serial_port->available()){
            ok[0] = ok[1];
            ok[1] = (char)serial_port->read();
            *(dest + p) = ok[1];
            p++;

            if(strcmp(ok, "OK") == 0){
                return;
            }
        }
    }

    Serial.println("[timeout]getCharsUartBuffer()");
    strcpy(dest, "ERROR");
    return;
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
}

void setup(){
    // Display initializing message.
    lcd.begin(16, 2); //16 chars × 2 rows
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("STARTING BELL");
    lcd.setCursor(0, 1);
    lcd.print("RINGING MACHINE");

    // Initialize serial ports & ESP module
    Serial.begin(9600);   // initialize serial for debugging
    Serial1.begin(115200);    // initialize serial for ESP module
    delay(10);
    sendEspCmd("AT+UART_CUR=9600,8,1,0,0", &Serial1);
    delay(1000);
    Serial1.begin(9600);
    sendEspCmd("ATE0", &Serial1);  // disable echo of commands
    sendEspCmd("AT+CWMODE=1", &Serial1);  // set station mode
    sendEspCmd("AT+CIPMUX=1", &Serial1);  // set multiple connections mode
    sendEspCmd("AT+CIPDINFO=1", &Serial1);  // Show remote IP and port with "+IPD"
    sendEspCmd("AT+CWAUTOCONN=0", &Serial1);  // Disable autoconnect
    sendEspCmd("AT+CWDHCP=1,1", &Serial1);  // enable DHCP
    delay(200);

    // Clear Serial1's buffer
    clearUartBuffer(&Serial1);

    // check for the presence of the shield
    Serial1.println("AT+CIPSTATUS");
    char ok[2];
    while(Serial1.available()){
        ok[0] = ok[1];
        ok[1] = Serial1.read();
        if(strcmp(ok, "OK") == 0){
            return;
        }
    }
    if (strcmp(ok, "OK") != 0) {
        Serial.println("ESP-WROOM-02 is not present");
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
    // Serial.println("-----------------");
    for(int i = 0; i < 3; i++){
        // Serial.println(RING_ADDR_OFFSET + i*2);
        int upper = EEPROM.read(RING_ADDR_OFFSET + i*2);
        // Serial.println(EEPROM.read(RING_ADDR_OFFSET + i*2));
        int lower = EEPROM.read(RING_ADDR_OFFSET + i*2 + 1);
        // Serial.println(EEPROM.read(RING_ADDR_OFFSET + i*2 + 1));
        ring_seconds[i] = (int)(upper << 8 | lower);
    }

    // Get SSID and password from EEPROM.
    ssid_length = EEPROM.read(SSID_OFFSET);
    for(int i = 0; i < ssid_length; i++){
        ssid_router[i] = EEPROM.read(SSID_OFFSET + 1 + i) + 0x30;
    }
    password_length = EEPROM.read(PASSWORD_OFFSET);
    for(int i = 0; i < password_length; i++){
        password_router[i] = EEPROM.read(PASSWORD_OFFSET + 1 + i) + 0x30;
    }

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
    Serial.println();
    Serial.println("===Entered setRouterConfig()===");

    // Display "setup mode"
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ENTERED");
    lcd.setCursor(0, 1);
    lcd.print("SETUP MODE...");

    // Start its own access point
    Serial1.println("AT+CWMODE_CUR=2");  // set AP mode
    Serial1.println("AT+CWSAP_CUR=\"" + ssid_ap + "\",\"" + password_ap + "\",10,4");  // start access point
    // int status = WiFi.beginAP(ssid_ap, 10, password_ap, ENC_TYPE_WPA2_PSK);

    // Start server.
    Serial1.println("AT+CIPSERVER=1,80");
    // server.begin();

    delay(1000);

    // Clear Serial1's buffer
    clearUartBuffer(&Serial1);

    // Get IP address
    Serial1.println("AT+CIPAP?");  // getIpAddressAP
    String ap_ip = Serail1.readStringUntil("OK");
    // IPAddress ap_ip = WiFi.localIP();

    // Show some info
    Serial.print("IP Address: ");
    Serial.println(ap_ip);
    Serial.println();
    Serial.print("Connect to ");
    Serial.print(ssid_ap);
    Serial.print(" and open a browser to http://");
    Serial.println(ap_ip);
    Serial.println();

    // Show IP address to LCD
    String ip_str = (String)ap_ip[0] + "." + (String)ap_ip[1] + "." + (String)ap_ip[2] + "." + (String)ap_ip[3];
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

        if (!client) {
                continue;
        }

        Serial.println("");
        Serial.println("New client");

        // Check client is connected
        bool set_ssid_password = false;
        while (client.connected()) {
            // Client send request?
            if (client.available()) {
                String req = client.readStringUntil('\r\n\r\n');
                Serial.println(req);

                // Parse the request to see which page the client want
                int addr_start = req.indexOf(' ');
                int addr_end = req.indexOf(' ', addr_start + 1);
                if (addr_start == -1 || addr_end == -1) {
                    Serial.print("Invalid request: ");
                    Serial.println(req);
                    break;
                }

                req = req.substring(addr_start + 1, addr_end);
                Serial.print("Request: ");
                Serial.println(req);

                if(req.indexOf("/") != -1) {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println();

                    client.println("<!DOCTYPE html>");
                    client.println("<html>");
                    client.println("<head>");
                    client.println("<meta name='viewport' content='initial-scale=1.5'>");
                    client.println("</head>");
                    client.println("<body>");
                    client.println("<form method='get'>");
                    client.println("<br>");

                    // Change output depends on a request
                    if(req.indexOf("Set") != -1){
                        int ssid_addr_start = req.indexOf("ssid") + 5;
                        int ssid_addr_end = req.indexOf("&", ssid_addr_start);
                        int pass_addr_start = req.indexOf("password") + 9;
                        int pass_addr_end = req.indexOf("&", pass_addr_start);
                        String ssid = req.substring(ssid_addr_start, ssid_addr_end);
                        String password = req.substring(pass_addr_start, pass_addr_end);
                        // TODO ; convert string to char[]
                        // ssid_router =
                        // password_router =

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

                        set_ssid_password = true;
                        client.println("Set SSID and password correctly.<br>");
                    }

                    client.println("</font><br>");
                    client.print("Set SSID : ");
                    client.print("<input type=\"text\" name='ssid' value='0' required>");
                    client.print("Set password : ");
                    client.print("<input type=\"text\" name='password' required>");
                    client.print("<br>");
                    client.print("<input type='submit' name='Set' value='Set' style='background-color:black; color:white;' >");
                    client.print("</form>");
                    client.print("</body>");
                    client.print("</html>");

                    client.println();
                }else{
                    // if we can not find the page that client request then we return 404 File not found
                    client.println("HTTP/1.1 404 Not Found");
                    Serial.println("Sending 404");
                }
            }
            break;
        }
        Serial.println("Done with client");
        if(set_ssid_password == true) return true;
    }

    while(1){}
}

void clearUartBuffer(Stream* serial_port){
    while(serial_port->available()){
        serial_port->read();
    }
}