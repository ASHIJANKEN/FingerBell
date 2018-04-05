const int BTN_PIN = 4;
void setup(){
  Serial.begin(38400);
  pinMode(BTN_PIN, INPUT_PULLUP);
}
bool transit = false;

void loop(){
  a();
}

void a(){
  int old_btn_state = digitalRead(BTN_PIN); // The button's previous state.
  unsigned long pressed_time = 0;

  while(true){
    int btn_state = digitalRead(BTN_PIN);
    // Serial.print(old_btn_state);
    // Serial.print(" : ");
    // Serial.println(btn_state);

    // Check the button state.
    if(!btn_state & (btn_state ^ old_btn_state) & !transit){
      // When the button is pressed.
      pressed_time = millis();
      Serial.println("Pressed!");

    }else if(!btn_state & !old_btn_state & !transit){
      // When the button is kept pressing over 2 seconds.
      // if(millis() - pressed_time > 2000){
      //   Serial.println("Kept pressing!");
      //   transit = true;

      //   break;
      // }

    }else if(btn_state & (btn_state ^ old_btn_state)){
      // When the button is released.
      if(millis() - pressed_time > 100){
        Serial.println("Released!");
        transit = false;
        delay(80);
        break;
      }
    }

    old_btn_state = btn_state;
  }
  Serial.println("-----------------");
}