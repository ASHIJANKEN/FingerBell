const int BTN_PIN = 12;
//Button state
const int NOT_PRESSED         = 0;
const int PRESSED             = 1;
const int PRESSING            = 2;
const int RELEASED            = 3;
const int LONG_PRESSED        = 4;
const int LONG_PRESSING       = 5;
const int LONG_PRESS_RELEASED = 6;

void setup(){
  Serial.begin(38400);
  pinMode(BTN_PIN, INPUT_PULLUP);
}
bool transit = false;

void loop(){
  switch (detectBtnStat()) {
      case 0:
        Serial.println("NOT_PRESSED");
        break;
      case 1:
        Serial.println("PRESSED");
        break;
      case 2:
        Serial.println("PRESSING");
        break;
      case 3:
        Serial.println("RELEASED");
        break;
      case 4:
        Serial.println("LONG_PRESSED");
        break;
      case 5:
        Serial.println("LONG_PRESSING");
        break;
      case 6:
        Serial.println("LONG_PRESS_RELEASED");
        break;
  }
  delay(100);
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