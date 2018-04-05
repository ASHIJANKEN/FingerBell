#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Servo.h>

// Pin assign of rotary encoders(A/B phases)
const int ENCODERS_PINS[][] = {{0, 1},
                                {2, 3},
                                {4, 5}
                              };

// Pin assign of a button.
const int BTN_PIN = 12;

// Pin assign of a servo.
const int SERVO_PIN = 11;

// Pin assign of a lcd.
const int RS       = 6;
const int ENABLE   = 7;
const int DB4      = 8;
const int DB5      = 9;
const int DB6      = 10;
const int DB7      = 13;

// For servo
const int MIN_ANGLE = 70;
const int MAX_ANGLE = 180;

// Modes
const int SET_TIMER   = 0;
const int START_TIMER = 1;

// Waiting time to avoid chattering.
const int WAIT_FOR_BOUNCE_MSEC = 100;

// Addresses of EEPROM for keeping ringing time.
const int RING_SECONDS_ADDR0 = 0;
const int RING_SECONDS_ADDR1 = 1;
const int RING_SECONDS_ADDR2 = 2;

// Time when the bell rings.
int ring_seconds[3] = {0, 0, 0};

// Manage the button long pressing
bool transit = false;

// Present encoders state
int enc_state[][] = {{0, 0},
                      {0, 0},
                      {0, 0}
                    };

// Previous encoders state
int enc_prev_state[][] = {{0, 0},
                            {0, 0},
                            {0, 0}
                          };

// LCD
LiquidCrystal lcd(RS, ENABLE, DB4, DB5, DB6, DB7);

void setup(){
  // Initialize the LCD.
  lcd.begin(16, 2); //16 chars × 2 rows
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("STARTING BELL");
  lcd.setCursor(0, 1);
  lcd.print("RINGING MACHINE");

  // Initialize a serial.
  //Serial.begin(4800);

  // Get ringing time from EEPROM.
  // ring_seconds[0] = (int)EEPROM.read(RING_SECONDS_ADDR0);
  // ring_seconds[1] = (int)EEPROM.read(RING_SECONDS_ADDR1);
  // ring_seconds[2] = (int)EEPROM.read(RING_SECONDS_ADDR2);

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
}

void loop(){

  // set timer to ring the bell.
  setTimer();

  // start timer.
  startTimer();

}

// set time to ring the bell.
void setTimer(){
  int old_btn_state = digitalRead(BTN_PIN); // The button's previous state.
  unsigned long pressed_time = 0;           // Time when the button pressed.

  while(true){
    unsigned long pressed_time = 0;
    int btn_state = digitalRead(BTN_PIN);

    // Display setting information on LCD.
    displayInfo(SET_TIMER, ring_seconds);

    // Adjust ringing time with rotary encoder and wait a short period to avoid chattering.
    for(int i=0; i<3; i++){
      int a = 2 * i; // Port number for A phase
      int b = a + 1; // Port number for B phase
      enc_state[i][0] = (PIND & _BV(a)) >> a;
      enc_state[i][1] = (PIND & _BV(b)) >> b;

      if(enc_state[i][0] != enc_prev_state[i][0] || enc_state[i][1] != enc_prev_state[i][1] ){
        ring_seconds[i] += readEnc(enc_prev_state[i][0], enc_prev_tate[i][1], enc_state[i][0], enc_state[i][1]) * 10;
      }

      enc_prev_state[i][0] = enc_state[i][0];
      enc_prev_state[i][1] = enc_state[i][1];
    }
    // delay(WAIT_FOR_BOUNCE_MSEC);

    // Check the button state.
    if(!btn_state & (btn_state ^ old_btn_state) & !transit){
      // When the button is pressed.
      pressed_time = millis();

    }else if(!btn_state & !old_btn_state & !transit){
      // When the button is kept pressing over 2 seconds.
      // Goto preset mode.
      // TODO: Implement preset mode.
      // break;

    }else if(btn_state & (btn_state ^ old_btn_state)){
      // When the button is released.
      // Finish setting.
      if(millis() - pressed_time > WAIT_FOR_BOUNCE_MSEC){
        transit = false;
        break;
      }
    }

    old_btn_state = btn_state;
  }
}

void startTimer(){
  int old_btn_state = digitalRead(BTN_PIN); // The button's previous state.
  unsigned long pressed_time = 0;           // Time when the button pressed.
  int is_stopping = 0;                      // Is the timer stops?
  unsigned long start_time = millis();      // Time when this method starts[msec].
  unsigned long suspension_start_time = 0;  // Time when stops timer[msec].
  unsigned long total_suspension_time = 0;  // Total elapsed time when timer stops[msec].
                                            // int elapsed_time = 0;                       // Elpsed time[sec].
  int ranged_bell[3] = [0, 0, 0];           // See whether it already rang the bell or not
  int args[2] = [0, 0];                     // [is_stopping, Elapsed time]

  // Start timer.
  // Control the servo when it's ringing time.
  while(true){
    int btn_state = digitalRead(BTN_PIN);

    // Display information on LCD.
    displayInfo(START_TIMER, args);

    // If it is ringing time, ring the bell.
    if(args[1] == ring_seconds[0] && ranged_bell[0] == 0){
      ringBell(1);
      ranged_bell[0] = !ranged_bell[0];
    }else if(args[1] == ring_seconds[1] && ranged_bell[1] == 0){
      ringBell(2);
      ranged_bell[1] = !ranged_bell[1];
    }else if(args[1] == ring_seconds[2] && ranged_bell[2] == 0){
      ringBell(3);
      ranged_bell[2] = !ranged_bell[2];
    }

    // Update the elapsed time.
    args[1] = (int)((millis() - start_time - total_suspension_time) / 1000L);

    // Check the button state.
    if(!btn_state & (btn_state ^ old_btn_state) & !transit){
      // When the button is pressed.
      pressed_time = millis();

    }else if(!btn_state & !old_btn_state & !transit){
      // When the button is kept pressing over 2 seconds.
      // Return to setTimer.
      if(millis() - pressed_time > 2000){
        transit = true;
        break;
      }

    }else if(btn_state & (btn_state ^ old_btn_state)){
      // When the button is released.
      // Stop/restart timer.
      if(millis() - pressed_time > WAIT_FOR_BOUNCE_MSEC){
        is_stopping = !is_stopping;
        transit = false;

        if(is_stopping != 0){
          // When the timer stops.
          suspension_start_time = millis();
        }else{
          //When the timer restarts.
          total_suspension_time += millis() - suspension_start_time;
        }
      }
    }

    old_btn_state = btn_state;
  }
}

// Display information on LCD.
void displayInfo(int mode, int* args){
  switch (mode) {
    case SET_TIMER:
      //上の段
      lcd.setCursor(0, 0);
      lcd.print("   1     2     3");

      //下の段
      lcd.setCursor(0,1);
      // 1st ringing time
      printTime(*args);
      lcd.print(" ");
      // 2nd ringing time
      printTime(*(args+1));
      lcd.print(" ");
      // 3rd ringing time
      printTime(*(args+2));
      lcd.print(" ");
      break;

    case START_TIMER:
      //上の段
      lcd.setCursor(0, 0);
      if(*args == 0){
        // When measuring
        lcd.print("Measuring...")
      }else{
        // When measuring is suspended
        lcd.print("Stop!")
      }

      //下の段
      lcd.setCursor(0,1);
      lcd.print("     ");
      // now
      printTime(*(args+1));
      lcd.print("      ");
      break;

    default:
      ;
  }
}

// Returns encoder's rotate dirction.
int readEnc(uint8_t prev_a, uint8_t prev_b, uint8_t a, uint8_t b){
  uint8_t encoded  = (prev_a << 4) | (prev_b << 3) | (a << 2) | b;

  // if(encoded == 0b1101 || encoded == 0b0100 || encoded == 0b0010 || encoded == 0b1011){
  if(encoded == 0b1101){
    return 1;
  // } else if(encoded == 0b1110 || encoded == 0b0111 || encoded == 0b0001 || encoded == 0b1000) {
  }else if(encoded == 0b1110) {
    return -1;
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
// This method is supposed to be completed in one second.
void ringBell(int times){
  int stroke_time = 1000 / times;                                   // Time for one stroke[msec].
  int delay_msec = stroke_time / ((MAX_ANGLE - MIN_ANGLE) * 2 + 1); // Calculate delay to finish all strokes in one second.

  // Push an arm.
  for(int i=0; i<3; i++){
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
