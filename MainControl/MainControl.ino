#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <stdio.h>
#include <string.h>
#include <Servo.h>
#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

// // Status number for sendHttpResponse
#define REQUEST_ROUTER_INFO 0
#define NOT_FOUND 1

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
    lcd.print(F("STARTING BELL"));
    lcd.setCursor(0, 1);
    lcd.print(F("RINGING MACHINE"));

    // Initialize serial ports & ESP module
    Serial.begin(9600);  // Initialize serial for debugging
    Serial1.begin(115200);
    delay(10);
    Serial1.println(F("AT+UART_CUR=9600,8,1,0,0"));  // Change ESP's baudrate
    delay(20);
    Serial1.begin(9600); // Initialize serial for ESP module
    WiFi.init(&Serial1); // Initialize ESP module
    // check for the presence of the shield
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println(F("ESP-WROOM-02 is not present"));
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("ESP-WROOM-02"));
        lcd.setCursor(0, 1);
        lcd.print(F("is not present"));
        while (true); // Don't continue anymore
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
    for(int i = 0; i < ssid_length; i++){
        ssid_router[i] = EEPROM.read(SSID_OFFSET + 1 + i) + 0x30;
    }
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

    // Enters setting mode for connecting other AP
    // eiter when you start FingerBell with pressing button or if SSID and password is null
    if(detectBtnStat() == PRESSING || ssid_length == 0){
        setRouterConfig();
    }

    // Connecting other AP

    while(1){
        // Set timer to ring the bell.
        setTimer();
        lcd.clear();

        // Start timer.
        startTimer();
        lcd.clear();
    }
}

// Set SSID and password for connecting other AP
bool setRouterConfig(){
    Serial.println(F("============================="));
    Serial.println(F("Entered setRouterConfig()"));
    Serial.println(F("============================="));

    // Display "setup mode"
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("ENTERED"));
    lcd.setCursor(0, 1);
    lcd.print(F("SETUP MODE..."));

    // Start its own access point
    char ssid_ap[] = "FingerBell_AP";
    char password_ap[] = "fingerbell_1234";
    WiFi.beginAP(ssid_ap, 10, password_ap, ENC_TYPE_WPA2_PSK);

    // Start server.
    server.begin();

    delay(1000);

    // Get IP address
    IPAddress ap_ip = WiFi.localIP();

    // Show some info
    Serial.print("SSID: ");
    Serial.println(ssid_ap);
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

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(F("ACCESS THIS IP:"));
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

        Serial.println(F("New client"));
        char buf[150] = {'\0'};
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
                    // Serial.print("query: ");
                    // Serial.println(query);

                    // Respond to request
                    if(query.indexOf("/") != -1 || query.indexOf("/?") != -1) {
                        sendHttpResponse(client, REQUEST_ROUTER_INFO, query);
                        if(query.indexOf("Set") != -1){
                            set_ssid_password = true;
                        }
                    }else{
                        sendHttpResponse(client, NOT_FOUND, query);
                    }
                    break;
                }

                buf_ptr++;
            }
        }
        delay(10);
        client.stop();
        Serial.println(F("Done with client"));
        if(set_ssid_password == true){
            return true;
        }
    }
}

void sendHttpResponse(WiFiEspClient client, int status, String query){
    switch(status){
        case REQUEST_ROUTER_INFO:
            Serial.println("Sending REQUEST_ROUTER_INFO");

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
        case NOT_FOUND:
            Serial.println(F("Sending 404"));
            client.println(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"));

            break;
    }
}

// Set time to ring the bell.
void setTimer(){
    // Present encoders state
    int enc_state[3][2] = {{0, 0},
                            {0, 0},
                            {0, 0}};

    // Previous encoders state
    int enc_prev_state[3][2] = {{0, 0},
                                {0, 0},
                                {0, 0}};


    Serial.print(ring_seconds[0]);
    Serial.print(" : ");
    Serial.print(ring_seconds[1]);
    Serial.print(" : ");
    Serial.print(ring_seconds[2]);
    Serial.println(" : ");
    while(true){
        // Display setting information on LCD.
        displayInfo(SET_TIMER, ring_seconds);

        // Adjust ringing time with rotary encoder and wait a short period to avoid chattering.
        for(int i=0; i<3; i++){
            int a = 2 * (i + 1); // Port number for A phase
            int b = a + 1; // Port number for B phase
            enc_state[i][0] = (PIND & _BV(a)) >> a;
            enc_state[i][1] = (PIND & _BV(b)) >> b;

            if(enc_state[i][0] != enc_prev_state[i][0] || enc_state[i][1] != enc_prev_state[i][1] ){
                ring_seconds[i] += readEnc(enc_prev_state[i][0], enc_prev_state[i][1], enc_state[i][0], enc_state[i][1]) * 10;
                if(ring_seconds[i] < 0) ring_seconds[i] = 0;
            }

            enc_prev_state[i][0] = enc_state[i][0];
            enc_prev_state[i][1] = enc_state[i][1];
        }

        // Check the button state.
        switch (detectBtnStat()){
            case RELEASED:
                //save ring_seconds to EEPROM.
                for(int i = 0; i < 3; i++){
                    EEPROM.write(RING_ADDR_OFFSET + i*2, ring_seconds[i] >> 8);
                    EEPROM.write(RING_ADDR_OFFSET + i*2 + 1, ring_seconds[i] & 0xFF);
                    Serial.println(RING_ADDR_OFFSET + i*2);
                    // EEPROM.write(RING_ADDR_OFFSET + i*2, i*10 >> 8);
                    Serial.println(ring_seconds[i] >> 8);
                    // EEPROM.write(RING_ADDR_OFFSET + i*2 + 1, i*10 & 0xFF);
                    Serial.println(ring_seconds[i] & 0xFF);
                }
                return;
                break;
                // Case LONG_PRESSED:
                    // TODO: Implement preset mode.
                    // Break;
        }
    }
}

void startTimer(){
    int is_stopping = 0;                      // Is the timer stops?
    unsigned long start_time = millis();      // Time when this method starts[msec].
    unsigned long suspension_start_time = 0;  // Time when stops timer[msec].
    unsigned long total_suspension_time = 0;  // Total elapsed time while timer stops[msec].
    int rang_bell[3] = {0, 0, 0};           // See whether it already rang the bell or not
    int args[2] = {0, 0};                     // [is_stopping, Elapsed time]

    // Start timer.
    // Control the servo when it's ringing time.
    while(true){
        // Display information on LCD.
        displayInfo(START_TIMER, args);

        // If it is ringing time, ring the bell.
        if(args[1] == ring_seconds[0] && !rang_bell[0]){
            ringBell(1);
            rang_bell[0] = !rang_bell[0];
        }else if(args[1] == ring_seconds[1] && !rang_bell[1]){
            ringBell(2);
            rang_bell[1] = !rang_bell[1];
        }else if(args[1] == ring_seconds[2] && !rang_bell[2]){
            ringBell(3);
            rang_bell[2] = !rang_bell[2];
        }

        // Update args.
        //Update is_stopping.
        args[0] = is_stopping;
        //Update elapsed time.
        if(!is_stopping){
            args[1] = (int)((millis() - start_time - total_suspension_time) / 1000L);
        }

        // Check the button state.
        switch(detectBtnStat()){
            case LONG_PRESSING:
                return;
                break;
            case PRESSED:
                // Stop/restart timer.
                is_stopping = !is_stopping;

                if(is_stopping != 0){
                    // When the timer stops.
                    suspension_start_time = millis();
                }else{
                    //When the timer restarts.
                    total_suspension_time += millis() - suspension_start_time;
                }
                break;
        }
    }
}

// Display information on LCD.
void displayInfo(int mode, int* args){
    switch (mode) {
        case SET_TIMER:
            lcd.setCursor(0, 0);
            // 1st ringing time
            lcd.print("1:");
            printTime(*args);
            // 2nd ringing time
            lcd.print(" 2:");
            printTime(*(args+1));
            lcd.setCursor(0, 1);
            // 3rd ringing time
            lcd.print("3:");
            printTime(*(args+2));
            break;

        case START_TIMER:
            //上の段
            lcd.setCursor(0, 0);
            if(*args == 0){
                // When measuring
                lcd.print("Running...");
            }else{
                // When measuring is suspended
                lcd.print("Stop!     ");
            }

            //下の段
            lcd.setCursor(5,1);
            // Now
            printTime(*(args+1));
            break;

        default:
            ;
    }
}

// Returns encoder's rotate dirction.
int readEnc(uint8_t prev_a, uint8_t prev_b, uint8_t a, uint8_t b){
    uint8_t encoded  = (prev_a << 4) | (prev_b << 3) | (a << 2) | b;

    // If(encoded == 0b1101 || encoded == 0b0100 || encoded == 0b0010 || encoded == 0b1011){
    if(encoded == 0b0100|| encoded == 0b0010){
        // Delay(WAIT_FOR_BOUNCE_MSEC);
        return 1;
    // } else if(encoded == 0b0010 || encoded == 0b0111 || encoded == 0b0001 || encoded == 0b1000) {
    }else if(encoded == 0b0001) {
        return -1;
        // Delay(WAIT_FOR_BOUNCE_MSEC);
    }else{
        return 0;
    }
}

// Display time like XX:XX.
void printTime(int total_seconds){
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;

    // Print time on LCD.
    if(minutes < 10) lcd.print(0);
    lcd.print(minutes);
    lcd.print(":");
    if(seconds < 10) lcd.print(0);
    lcd.print(seconds);
}

// Control the servo to ring the bell.
// This method is supposed to be completed in less than one second.
void ringBell(int times){
    float stroke_time = 333;             // Time for one stroke[msec].
    int delay_msec = (int)(stroke_time / ((MAX_ANGLE - MIN_ANGLE) * 2 + 1)); // Calculate delay to finish all strokes in one second.

    // Push an arm.
    for(int i=0; i<times; i++){
        for(int pos = MIN_ANGLE; pos <= MAX_ANGLE; pos++){
            myservo.write(pos);
            delay(delay_msec);
        }

        // Pull an arm.
        for(int pos = MAX_ANGLE - 1; pos >= MIN_ANGLE; pos--){
            myservo.write(pos);
            delay(delay_msec);
        }
    }
}

int detectBtnStat(){
    static bool is_long = false; // Manage the button long pressing
    static int old_btn_val = digitalRead(BTN_PIN); // The button's previous state.
    static unsigned long pressed_time = 0;           // Time when the button pressed.
    int button_state = NOT_PRESSED;

    int btn_val = digitalRead(BTN_PIN);

    if(!btn_val & (btn_val ^ old_btn_val) & !is_long){
        // When the button is pressed.
        pressed_time = millis();
        button_state = PRESSED;

    }else if(!btn_val & !old_btn_val){
        // When the button is kept pressing.

        if(is_long){
            // When the button is kept pressing over 2 seconds.
            button_state = LONG_PRESSING;
        }else{
            if(millis() - pressed_time > 2000){
                button_state = LONG_PRESSED;
                is_long = true;
            }else{
                button_state = PRESSING;
            }
        }

    }else if(btn_val & (btn_val ^ old_btn_val)){
        // When the button is released.

        if(is_long){
            //When the button is released after long pressing.
            button_state = LONG_PRESS_RELEASED;
            is_long = false;
            delay(80);

        }else{
            if(millis() - pressed_time > 10){
                button_state = RELEASED;
                delay(80);
            }
        }

    }else{
        button_state = NOT_PRESSED;
    }

    old_btn_val = btn_val;

    return button_state;
}
