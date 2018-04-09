#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Servo.h>

// Pin assign of rotary encoders(A/B phases)
const int ENCODERS_PINS[3][2] = {{2, 3},
                                {4, 5},
                                {6, 7}
                              };

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

// Addresses of EEPROM for keeping ringing time.
const int RING_ADDR_OFFSET = 500;

// Time when the bell rings.
int ring_seconds[3] = {0, 0, 0};

// Present encoders state
int enc_state[3][2] = {{0, 0},
                      {0, 0},
                      {0, 0}
                    };

// Previous encoders state
int enc_prev_state[3][2] = {{0, 0},
                            {0, 0},
                            {0, 0}
                          };

// Create objects.
LiquidCrystal lcd(RS, ENABLE, DB4, DB5, DB6, DB7);
Servo myservo;

void setup(){
  Serial.begin(9600);
  // Display initializing message.
  lcd.begin(16, 2); //16 chars × 2 rows
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("STARTING BELL");
  lcd.setCursor(0, 1);
  lcd.print("RINGING MACHINE");

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

  // set timer to ring the bell.
  setTimer();
  lcd.clear();

  // start timer.
  startTimer();
  lcd.clear();

}

// set time to ring the bell.
void setTimer(){
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
        // case LONG_PRESSED:
          // TODO: Implement preset mode.
          // break;
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
      // now
      printTime(*(args+1));
      break;

    default:
      ;
  }
}

// Returns encoder's rotate dirction.
int readEnc(uint8_t prev_a, uint8_t prev_b, uint8_t a, uint8_t b){
  uint8_t encoded  = (prev_a << 4) | (prev_b << 3) | (a << 2) | b;

  // if(encoded == 0b1101 || encoded == 0b0100 || encoded == 0b0010 || encoded == 0b1011){
  if(encoded == 0b0100|| encoded == 0b0010){
    // delay(WAIT_FOR_BOUNCE_MSEC);
    return 1;
  // } else if(encoded == 0b0010 || encoded == 0b0111 || encoded == 0b0001 || encoded == 0b1000) {
  }else if(encoded == 0b0001) {
    return -1;
    // delay(WAIT_FOR_BOUNCE_MSEC);
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