#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <stdio.h>
#include <string.h>
#include <Servo.h>
#include <WiFiEsp.h>
#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

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

// Status of Pro Mini
#define SET_ROUTER_CONFIG 0
#define CONNECT_ROUTER    1
#define API_SET_TIMER     2
#define API_RUN_TIMER     3

// Pins for UART communication
#define UART_SIGNAL_IN  A2
#define UART_SIGNAL_OUT A3

// Pins for sending status to esp8266
#define STATUS_1 A6
#define STATUS_0 A7

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

// ssid and password for router
char ssid[40] = {'\0'};
char password[70] = {'\0'};

// Serial port for ESP-WROOM-02
SoftwareSerial Serial1(A4, A5); // RX, TX

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
    Serial1.begin(9600);
    delay(10);

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

    // Initiallize status pins
    pinMode(STATUS_0, OUTPUT);
    pinMode(STATUS_1, OUTPUT);
    pinMode(UART_SIGNAL_IN, INPUT);
    pinMode(UART_SIGNAL_OUT, OUTPUT);

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

    delay(5000);
}

void loop(){
    // Enters setting mode for connecting other AP
    // eiter when you start FingerBell with pressing button or if SSID and password is null
    if(detectBtnStat() == PRESSING || ssid_length == 0){
        setRouterConfig();
    }

    connectRouter();
    delay(1000);

    while(1){
        // Set timer to ring the bell.
        setTimer();
        lcd.clear();

        // Start timer.
        runTimer();
        lcd.clear();
    }
}

// Set SSID and password for connecting other AP
bool setRouterConfig(){
    Serial.println(F("============================="));
    Serial.println(F("Entered setRouterConfig()"));
    Serial.println(F("============================="));
    clearSerialBuffer();
    sendProMiniStatus(SET_ROUTER_CONFIG);

    // Display "setup mode"
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("ENTERED"));
    lcd.setCursor(0, 1);
    lcd.print(F("SETUP MODE..."));

    // Wait for ESP8266 to be ready to send data
    waitUartReceive();
    int bufnum = Serial1.available();
    Serial.print("bufnum:");
    Serial.println(bufnum);
    for(int i = 0; i < bufnum; i++){
        Serial.print((char)Serial1.read());
    }
    Serial.println();
    clearSerialBuffer();

    // Show some info
    Serial.println("SSID: FingerBell_AP");
    Serial.println("PASSWORD: FingerBell_1234");
    Serial.println("URL: http://fingerbell.local");

    unsigned long st = millis();
    int lcd_state = 0;
    while(1){
        //Display info to lcd
        switch(((millis() - st) % 15000) / 5000){
        case 0:
            if(lcd_state != 0){
                lcd.clear();
                lcd_state = 0;

            }
            lcd.setCursor(0, 0);
            lcd.print("SSID:");
            lcd.setCursor(0, 1);
            lcd.print("FingerBell_AP");
            break;
        case 1:
            if(lcd_state != 1){
                lcd.clear();
                lcd_state = 1;
            }
            lcd.setCursor(0, 0);
            lcd.print("PASSWORD:");
            lcd.setCursor(0, 1);
            lcd.print("FingerBell_1234");
            break;
        case 2:
            if(lcd_state != 2){
                lcd.clear();
                lcd_state = 2;
            }
            lcd.setCursor(0, 0);
            lcd.print("URL: http://");
            lcd.setCursor(0, 1);
            lcd.print("fingerbell.local");
            break;
        }

        // Check Serial1
        if(Serial1.available()){
            waitUartReceive();

            char buf_serial[150] = {'\0'};
            for(int i = 0; Serial1.available() > 0; i++){
                buf_serial[i] = Serial1.read();
            }

            String ssid_str = subString(buf_serial, "SSID:", " ");
            ssid_str.toCharArray(ssid, ssid_str.length());
            String password_str = subString(buf_serial, "PASSWORD:", " ");
            password_str.toCharArray(password, password_str.length());

            Serial.print("SSID: ");
            Serial.println(ssid);
            Serial.print("password: ");
            Serial.println(password);

            // Write SSID and password to EEPROM
            EEPROM.write(SSID_OFFSET, ssid_str.length());
            // Write SSID and password to EEPROM
            for (int i = 0; i < ssid_str.length(); i++){
                EEPROM.write(SSID_OFFSET + 1 + i, ssid[i] - 0x30);
            }

            EEPROM.write(PASSWORD_OFFSET, password_str.length());
            for (int i = 0; i < password_str.length(); i++){
                EEPROM.write(PASSWORD_OFFSET + 1 + i, password[i] - 0x30);
            }

            break;
        }
    }

    return true;
}

bool connectRouter(){
    Serial.println(F("============================="));
    Serial.println(F("Entered connectRouter()"));
    Serial.println(F("============================="));
    clearSerialBuffer();
    sendProMiniStatus(CONNECT_ROUTER);

    ssid_length = EEPROM.read(SSID_OFFSET);
    password_length = EEPROM.read(PASSWORD_OFFSET);

    // Get SSID and password from EEPROM.
    for(int i = 0; i < ssid_length; i++){
        ssid[i] = EEPROM.read(SSID_OFFSET + 1 + i) + 0x30;
    }
    for(int i = 0; i < password_length; i++){
        password[i] = EEPROM.read(PASSWORD_OFFSET + 1 + i) + 0x30;
    }

    Serial.print("SSID:");
    Serial.println(ssid);
    Serial.print("PASSWORD:");
    Serial.println(password);

    Serial1.print("SSID:");
    Serial1.print(ssid);
    Serial1.print(" PASSWORD:");
    Serial1.print(password);
    Serial1.print(" ");

    sendUartReady();

    // Wait for esp8266 to send OK
    waitUartReceive();

    // Show URL address to LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ACCESS THIS URL:");
    lcd.setCursor(0, 1);
    lcd.print("fingerbell.local");

    return true;
}

// Set time to ring the bell.
void setTimer(){
    Serial.println(F("============================="));
    Serial.println(F("Entered setTimer()"));
    Serial.println(F("============================="));
    clearSerialBuffer();
    sendProMiniStatus(API_SET_TIMER);

    // Present encoders state
    int enc_state[3][2] = {{0, 0},
                            {0, 0},
                            {0, 0}};

    // Previous encoders state
    int enc_prev_state[3][2] = {{0, 0},
                                {0, 0},
                                {0, 0}};

    Serial.println("Ring times");
    Serial.print("[1]: ");
    Serial.print(ring_seconds[0] / 60);
    Serial.print(":");
    Serial.println(ring_seconds[0] % 60);
    Serial.print("[2]: ");
    Serial.print(ring_seconds[1] / 60);
    Serial.print(":");
    Serial.println(ring_seconds[1] % 60);
    Serial.print("[3]: ");
    Serial.print(ring_seconds[2] / 60);
    Serial.print(":");
    Serial.println(ring_seconds[2] % 60);
    Serial.println();

    lcd.clear();
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

        // Read ring seconds from Serial1
        if(Serial1.available()){
            waitUartReceive();

            char buf[60] = {'\0'};
            for(int i = 0; Serial1.available() > 0; i++){
                buf[i] = Serial1.read();
            }

            String buf_str = buf;

            for(int i = 0; i < 3; i++){
                String tag_str = String("set_ring" + String(i + 1));
                if(buf_str.indexOf(tag_str) != -1){
                    char tag[10] = {'\0'};
                    tag_str.toCharArray(tag, 9);
                    String ringtime_str = subString(buf, tag, " ");

                    ring_seconds[i] = ringtime_str.toInt();
                }
            }

            // Send ring_seconds
            for(int i = 0; i < 3; i++){
                Serial1.print(String("r" + String(i + 1) + ":"));
                Serial1.print(ring_seconds[i]);
                Serial1.print(" ");
            }
            sendUartReady();

            Serial.println("Ring times");
            Serial.print("[1]: ");
            Serial.print(ring_seconds[0] / 60);
            Serial.print(":");
            Serial.println(ring_seconds[0] % 60);
            Serial.print("[2]: ");
            Serial.print(ring_seconds[1] / 60);
            Serial.print(":");
            Serial.println(ring_seconds[1] % 60);
            Serial.print("[3]: ");
            Serial.print(ring_seconds[2] / 60);
            Serial.print(":");
            Serial.println(ring_seconds[2] % 60);
            Serial.println();
        }
    }
}

void runTimer(){
    int is_stopping = 0;                      // Is the timer stops?
    unsigned long start_time = millis();      // Time when this method starts[msec].
    unsigned long suspension_start_time = 0;  // Time when stops timer[msec].
    unsigned long total_suspension_time = 0;  // Total elapsed time while timer stops[msec].
    int rang_bell[3] = {0, 0, 0};           // See whether it already rang the bell or not
    int args[2] = {0, 0};                     // [is_stopping, Elapsed time]

    Serial.println(F("============================="));
    Serial.println(F("Entered runTimer()"));
    Serial.println(F("============================="));
    clearSerialBuffer();
    sendProMiniStatus(API_RUN_TIMER);

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

        if(Serial1.available()){
            waitUartReceive();

            char buf[10] = {'\0'};
            for(int i = 0; Serial1.available() > 0; i++){
                buf[i] = Serial1.read();
            }
            String buf_str = buf;
            clearSerialBuffer();

            // Send elapsed_time and ring_seconds
            if(buf_str.indexOf("res") == -1){
                Serial1.print("e:");
                Serial1.print(args[1]);
                Serial1.print(" ");
                for(int i = 0; i < 3; i++){
                    Serial1.print(String("r" + String(i + 1) + ":"));
                    Serial1.print(ring_seconds[i]);
                    Serial1.print(" ");
                }
            }
            sendUartReady();
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
    static unsigned long pressed_time = millis();           // Time when the button pressed.
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

void sendProMiniStatus(int status){
    analogWrite(A6, ((status >> 1) & 1) * 255);
    analogWrite(A7, (status & 1) * 255);

    // uint8_t portc = PORTC;
    // PORTC = (portc & 0b11000111)
    //         | (status & 1) << 5
    //         | ((status >> 1) & 1) << 4;
}

void clearSerialBuffer(){
    while(Serial1.available()){
        Serial1.read();
    }
}

String subString(char* buf, char* start_tag, char* end_tag){
    String buf_str = buf;
    int start_indexof = buf_str.indexOf(start_tag);
    int end = buf_str.indexOf(end_tag, start_indexof);

    if(start_indexof == -1 || end == -1){
        return "";
    }

    int start = start_indexof + strlen(start_tag);
    int length = end - start;
    // Serial.println(start);
    // Serial.println(end);
    // Serial.println(length);
    char* ret = new char[length];

    strncpy(ret, buf + start, length);
    String ret_str = ret;

    return ret_str;
}

/* Send 'ready' status.
 * 1. Send UART_SIGNAL_OUT to notify that strings are already printed.
 * 2. UART_SIGNAL_IN goes to be high as an ESP's ack after detecting UART_SIGNAL_OUT signal.
 * 3. Disable UART_SIGNAL_OUT signal.
 * 4. Make sure ESP's signal goes to be low
 */
void  sendUartReady(){
    digitalWrite(UART_SIGNAL_OUT, HIGH);

    while(digitalRead(UART_SIGNAL_IN) == LOW){
    }

    digitalWrite(UART_SIGNAL_OUT, LOW);

    while(digitalRead(UART_SIGNAL_IN) == HIGH){
    }
}

/* Wait for ESP8266 to be ready to send data.
 * 1. Wait for ESP's signal to be high.
 * 2. Send UART_SIGNAL_IN status as an ack.
 * 3. Wait for ESP's signal goes to be low.
 * 4. Disable UART_SIGNAL_OUT.
 */
void waitUartReceive(){
    while(digitalRead(UART_SIGNAL_IN) == LOW){
    }
    digitalWrite(UART_SIGNAL_OUT, HIGH);

    while(digitalRead(UART_SIGNAL_IN) == HIGH){
    }
    digitalWrite(UART_SIGNAL_OUT, LOW);
}