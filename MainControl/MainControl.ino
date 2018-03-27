#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Servo.h>

#define WAIT_FOR_BOUNCE_MSEC 20

// time when the bell rings.
int ring_seconds[3] = {0, 0, 0};

// LCD
LiquidCrystal lcd(RS, ENABLE, DB4, DB5, DB6, DB7);

/**************************************
 * Pin assign
 **************************************/
//rotary encoders(A,B)
const int encoders_pins[][] = {{1, 2},
                                {3, 4},
                                {5, 6}
                              };

// button
const int btn_pin = 7;

void setup(){
  //シリアルのセットアップ
  //Serial.begin(4800);

  //EEPROMからの起床時刻とスヌーズの長さ、指定メロディパターンの読出し
  time_wakeup[0] = (int)EEPROM.read(HOUR1_WAKEUP_ADRS);
  time_wakeup[1] = (int)EEPROM.read(HOUR2_WAKEUP_ADRS);
  time_wakeup[2] = (int)EEPROM.read(MINUTE1_WAKEUP_ADRS);
  time_wakeup[3] = (int)EEPROM.read(MINUTE2_WAKEUP_ADRS);
  snooze_length_min = (int)EEPROM.read(SNOOZE_LENGTH_ADRS);
  melody_pattern = (int)EEPROM.read(MELODY_PATTERN_ADRS);

  //振動モータのセットアップ
  pinMode(VIBRATOR,OUTPUT);

  //内蔵LEDを消灯
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);

  //初期モードのセットアップ
  mode = MODE_SELECT;
  mode_transit_to = SET_ALARM;

  //スイッチのセットアップ
  pinMode(SWITCH0,INPUT_PULLUP);
  pinMode(SWITCH1,INPUT_PULLUP);
  pinMode(SWITCH2,INPUT_PULLUP);
  pinMode(SWITCH3,INPUT_PULLUP);

  //LCDのセットアップ
  pinMode(V_IN_LCD, OUTPUT);
  digitalWrite(V_IN_LCD,HIGH);
  //delay(100);
  lcd.begin(16, 2); //16文字×2行
  lcd.clear(); //LCD画面消去
  lcd.createChar(0, chonmage_seijin);
  lcd.createChar(1, right_arrow);
  lcd.createChar(2, left_arrow);
  lcd.createChar(3, battery_3bar);
  lcd.createChar(4, battery_2bar);
  lcd.createChar(5, battery_1bar);
  lcd.createChar(6, battery_0bar);
  lcd.setCursor(0, 0);
  lcd.print("STARTING");
  lcd.setCursor(2, 1);
  lcd.print("ALARM CLOCK...");

  //RTCのセットアップ
  Wire.begin();
  Wire.beginTransmission(RTC_ADRS);
  Wire.write(0xE0);
  Wire.write(0x20);  //24hourMode
  Wire.write(0x00);  // PON Clear
  Wire.endTransmission();
  delay(100);

  //RTCから現在時刻を読み取る
  byte seconds, minutes, hours;
  Wire.requestFrom(RTC_ADRS,8);
  Wire.read();//control2
  seconds = Wire.read();//seconds
  minutes = Wire.read();//minutes
  hours = Wire.read();//hours
  time_now[0] = (int)hours>>4;
  time_now[1] = (int)(hours&&0xF0)>>4;
  time_now[2] = (int)minutes>>4;
  time_now[3] = (int)(minutes&&0xF0)>>4;
  time_now[4] = (int)seconds>>4;
  time_now[5] = (int)(seconds&&0xF0)>>4;

  //動作周波数のセットアップ
  //CLKPR = (1<<CLKPCE);
  //CLKPR = DEF_CLK_DIV;
}

void loop(){

  // set timer to ring the bell.
  setTimer();

  // start timer.
  startTimer();

}

// set time to ring the bell.
void setTimer(){
  int duration = 0;
  int old_btn_state = HIGH;

  while(true){
    unsigned long pressed_time = 0;
    int btn_state = digitalRead(btn_pin);

    // Display setting information on LCD.
    displayInfo(SET_TIMER);

    // Adjust ringing time with rotary encoder and wait a short period to avoid chattering.
    for(int i=0; i<3; i++){
      ring_seconds[i] += readEnc(i) * 10;
    }
    delay(WAIT_FOR_BOUNCE_MSEC);

    // Check the button state.
    if(!btn_state & (btn_state ^ old_btn_state)){
      // When the button is pressed.
      pressed_time = millis();

    }else if(btn_state & old_btn_state){
      // When the button is kept pressing over 2 seconds.
      // Goto preset mode.
      // TODO: Implement preset mode.
      // break;

    }else if(btn_state & (btn_state ^ old_btn_state)){
      // When the button is released.
      // Finish setting.
      if(millis() - pressed_time > 100) break;
    }

    old_btn_state = btn_state;
  }
}

void startTimer(){
  int duration = 0;
  int old_btn_state = HIGH;
  int is_stopping = false;
  unsigned long start_time = millis();

  //カウントスタート
  //時間になったらサーボを動かす
  while(true){
    unsigned long pressed_time = 0;
    int btn_state = digitalRead(btn_pin);

    // Display information on LCD.
    displayInfo(START_TIMER);

    // If it is ringing time, ring the bell.


    // Check the button state.
    if(!btn_state & (btn_state ^ old_btn_state)){
      // When the button is pressed.
      pressed_time = millis();

    }else if(btn_state & old_btn_state){
      // When the button is kept pressing over 2 seconds.

      // Return to setTimer.
      if(millis() - pressed_time > 2000) break;

    }else if(btn_state & (btn_state ^ old_btn_state)){
      // When the button is released.
      // Stop timer.
      if(millis() - pressed_time > 100){
        is_stopping = !is_stopping;
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
      printTime(sec);
      lcd.print(time_now[0]);
      lcd.print(time_now[1]);
      lcd.print(":");
      lcd.print(time_now[2]);
      lcd.print(time_now[3]);
      lcd.print(" ");
      // 2nd ringing time
      lcd.print(time_now[0]);
      lcd.print(time_now[1]);
      lcd.print(":");
      lcd.print(time_now[2]);
      lcd.print(time_now[3]);
      lcd.print(" ");
      // 3rd ringing time
      lcd.print(time_now[0]);
      lcd.print(time_now[1]);
      lcd.print(":");
      lcd.print(time_now[2]);
      lcd.print(time_now[3]);
      lcd.print(" ");
      break;

    case START_TIMER:
      //上の段
      lcd.setCursor(0, 0);
      if(args[0] == 1){
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
      lcd.print(time_now[0]);
      lcd.print(time_now[1]);
      lcd.print(":");
      lcd.print(time_now[2]);
      lcd.print(time_now[3]);
      lcd.print("      ");
      break;

    default:
      ;
  }
}

// Returns encoder's rotate dirction.
int readEnc(int num){
  // Memoy of rotary encoders' A/B phases history
  static int8_t encoders_ab[3] = {0, 0, 0};
  // Encoders rotate table
  static const int direction[]={0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0};

  // Update phases value.
  encoders_ab[num] = (encoders_ab[num] << 2 | PIND >>6) & 0x0f;

  // Get rotate direction.
  return d[encoders_ab[num]];
}

void printTime(int total_seconds){
  int minute = total_seconds / 60;
  int seconds = total_seconds % 10;
  int minute1 = total_seconds / 600;
  int minute2 = total_seconds / 60 % 10;
  int seconds1 = total_seconds % 60 / 10;
  int seconds2 = total_seconds % 60 / 10;



  lcd.print(time_now[0]);
  lcd.print(time_now[1]);
  lcd.print(":");
  lcd.print(time_now[2]);
  lcd.print(time_now[3]);
}




      //snooze_lengthの変更
      if(current_btn[0]==LOW && former_btn[0]!=current_btn[0]){//数値の増加
        snooze_length_min++;
        if(snooze_length_min>30) snooze_length_min = 0;
      }else if(current_btn[3]==LOW && former_btn[3]!=current_btn[3]){//数値の減少
        snooze_length_min--;
        if(snooze_length_min<0) snooze_length_min = 30;
      }else if(current_btn[1]==LOW && former_btn[1]!=current_btn[1]){
        EEPROM.write(SNOOZE_LENGTH_ADRS,intToByte(snooze_length_min));
        delay(10);
        mode = MODE_SELECT;
      }
      break;

    case SET_MELODY:
      displaySetMelody(melody_pattern);

      //melody_patternの変更
      if(current_btn[1]==LOW && former_btn[1]!=current_btn[1]){//次のパターンへ
        melody_pattern++;
        if(melody_pattern>MELODY_PATTERNS) melody_pattern = 1;
      }else if(current_btn[2]==LOW && former_btn[2]!=current_btn[2]){//前のパターンへ
        melody_pattern--;
        if(melody_pattern<1) melody_pattern = MELODY_PATTERNS;
      }else if(current_btn[0]==LOW && former_btn[0]!=current_btn[0]){//モードセレクトへ
        EEPROM.write(MELODY_PATTERN_ADRS,intToByte(melody_pattern));
        delay(10);
        mode = MODE_SELECT;
      }
      break;

    case ADJUST_TIME:
      adjustTime();
      break;

    case SLEEP_MODE:
      digitalWrite(V_IN_LCD,LOW);
      delay(10);

      attachInterrupt(0,toSoundAlarm,FALLING);
      attachInterrupt(1,toDisplayCurrentTime_SLEEP_MODE,LOW);
      noInterrupts();
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      interrupts();
      sleep_mode();
      attachInterrupt(0,doNothing,FALLING);
      attachInterrupt(1,doNothing,FALLING);
      break;

    case SOUND_ALARM:
      soundAlarm();
      break;

    case SNOOZE_MODE:
      displayLCD(SNOOZE_MODE);

      //振動モーターの停止
      digitalWrite(VIBRATOR, LOW);

      displaycount++;

      //画面表示している間に起床時刻になったら強制的にtoSoundAlarmへ
      if(time_now[0]*1000+time_now[1]*100+time_now[2]*10+time_now[3] == time_wakeup[0]*1000+time_wakeup[1]*100+time_wakeup[2]*10+time_wakeup[3] && time_now[4]*10+time_now[5] == 0){
        toSoundAlarm();
      }else if(displaycount >= DISPLAY_TIME){
        digitalWrite(V_IN_LCD, LOW);

        displaycount = 0;

        attachInterrupt(0,toSoundAlarm,FALLING);
        attachInterrupt(1,toDisplayCurrentTime_SNOOZE_MODE,LOW);
        noInterrupts();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        interrupts();
        sleep_mode();
        attachInterrupt(0,doNothing,FALLING);
        attachInterrupt(1,doNothing,FALLING);
      }
      break;

    case WARN_LOW_BATTERY:
      displayLCD(WARN_LOW_BATTERY);

      delay(5000);

      //スリープ
      noInterrupts();
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      interrupts();
      sleep_mode();
      break;

    case DISPLAY_CURRENT_TIME:
      displayLCD(DISPLAY_CURRENT_TIME);
      displaycount++;

      //画面表示している間に起床時刻になったら強制的にtoSoundAlarmへ
      if(time_now[0]*1000+time_now[1]*100+time_now[2]*10+time_now[3] == time_wakeup[0]*1000+time_wakeup[1]*100+time_wakeup[2]*10+time_wakeup[3] && time_now[4]*10+time_now[5] == 0){
        toSoundAlarm();
      }else if(displaycount>=DISPLAY_TIME){
        mode=SLEEP_MODE;
        displaycount = 0;
        digitalWrite(V_IN_LCD, LOW);
      }
      break;
  }

  //ボタンの前状態の保持
  former_btn[0] = current_btn[0];
  former_btn[1] = current_btn[1];
  former_btn[2] = current_btn[2];
  former_btn[3] = current_btn[3];
}


//sountAlarm(int i):引数にアラームの遷移状態(alarm_repeat)を引数にとってドミソを繰り返し鳴らす。ブザーの直接的な制御メソッド
void soundBuzzer(int pattern){
  //遷移状態の更新
  alarm_repeat++;

  //ブザーの制御
  switch(pattern){
    case MELODY_CEG:
      if(alarm_repeat >= 10) alarm_repeat=0;

      switch(alarm_repeat / (10/3)){//(alarm_repeat / (メロディ1リピートにかかるクロック/音階数))
        case 0:
          tone(BUZZER, 523*pow(2,(int)BUZZER_CLK_DIV), BEATTIME); // ド
          break;
        case 1:
          tone(BUZZER, 659*pow(2,(int)BUZZER_CLK_DIV), BEATTIME); // ミ
          break;
        case 2:
          tone(BUZZER, 784*pow(2,(int)BUZZER_CLK_DIV), BEATTIME); // ソ
          break;
      }
      break;

    case MELODY_C:
      if(alarm_repeat >= 4) alarm_repeat = 0;
      switch(alarm_repeat / (4/2)){
        case 0:
          tone(BUZZER, 1046*pow(2,(int)BUZZER_CLK_DIV), BEATTIME); //ド
          break;
        case 1:
          noTone(BUZZER);
          break;
      }
      break;

    case MELODY_CEGCGE:
      if(alarm_repeat >= 25) alarm_repeat = 0;
      switch(alarm_repeat / (25/6)){
        case 0:
          tone(BUZZER, 523*pow(2,(int)BUZZER_CLK_DIV), BEATTIME); // ド
          break;
        case 1:
          tone(BUZZER, 659*pow(2,(int)BUZZER_CLK_DIV), BEATTIME); // ミ
          break;
        case 2:
          tone(BUZZER, 784*pow(2,(int)BUZZER_CLK_DIV), BEATTIME); // ソ
          break;
        case 3:
          tone(BUZZER, 1046*pow(2,(int)BUZZER_CLK_DIV), BEATTIME); // ド
          break;
        case 4:
          tone(BUZZER, 784*pow(2,(int)BUZZER_CLK_DIV), BEATTIME); // ソ
          break;
        case 5:
          tone(BUZZER, 659*pow(2,(int)BUZZER_CLK_DIV), BEATTIME); // ミ
          break;
      }
      break;
  }
}

//soundAlarm():目覚ましアラームの鳴動メソッド
void soundAlarm(){
  displayLCD(SOUND_ALARM);

  //ブザーを鳴らす
  soundBuzzer(melody_pattern);

  //バイブの振動開始
  digitalWrite(VIBRATOR,HIGH);

  //ストップボタンが押されたらSNOOZE_MODEモードへ
  if(current_btn[stop_btn]==LOW && former_btn[stop_btn]!=current_btn[stop_btn]){
    //time_wakeup(起床時刻)の更新
    //まずは起床時刻を現在時刻と一致させる
    time_wakeup[0] = time_now[0];
    time_wakeup[1] = time_now[1];
    time_wakeup[2] = time_now[2];
    time_wakeup[3] = (time_now[4]<3)? time_now[3] : time_now[3]+1;//「秒」の四捨五入ならぬ29捨30入
    //スヌーズ時間を加算する
    time_wakeup[3] += snooze_length_min;
    //桁の繰り上げ処理
    if(time_wakeup[3] >= 10){
      time_wakeup[2] += time_wakeup[3]/10;
      time_wakeup[3] = time_wakeup[3]%10;
    }
    if(time_wakeup[2] >= 6){
      time_wakeup[1] += time_wakeup[2]/6;
      time_wakeup[2] = time_wakeup[2]%6;
    }
    if(time_wakeup[1] >= 10){
      time_wakeup[0] += time_wakeup[1]/10;
      time_wakeup[1] = time_wakeup[1]%10;
    }
    int updated_hours = time_wakeup[0]*10 + time_wakeup[1];
    if(updated_hours >= 24){
      updated_hours -= 24;
      time_wakeup[0] = updated_hours/10;
      time_wakeup[1] = updated_hours%10;
    }

    sendWakeUpTime();

    //ブザー鳴動前の動作周波数へ戻す
    //CLKPR = (1<<CLKPCE);
    //CLKPR = DEF_CLK_DIV;

    mode = SNOOZE_MODE;
  }
}

//displayModeSelect(int i):モードセレクト画面の画面表示
void displayModeSelect(int i){
  switch(i){
    case SET_ALARM:
      lcd.setCursor(0, 0);
      lcd.write(LEFT_ARROW);
      lcd.print(" SETALARM ");
      lcd.write(RIGHT_ARROW);
      displayBatteryCharge();
      break;

    case SET_SNOOZE:
      lcd.setCursor(0, 0);
      lcd.write(LEFT_ARROW);
      lcd.print("SET SNOOZE");
      lcd.write(RIGHT_ARROW);
      displayBatteryCharge();
      break;

    case SET_MELODY:
      lcd.setCursor(0, 0);
      lcd.write(LEFT_ARROW);
      lcd.print("SET MELODY");
      lcd.write(RIGHT_ARROW);
      displayBatteryCharge();
      break;

    case ADJUST_TIME:
      lcd.setCursor(0, 0);
      lcd.write(LEFT_ARROW);
      lcd.print("ADJUSTTIME");
      lcd.write(RIGHT_ARROW);
      displayBatteryCharge();
      break;
  }
  displayLCD(MODE_SELECT);
}

void displaySetMelody(int i){
  displayLCD(SET_MELODY);

  switch(i){
    case MELODY_CEG:
      lcd.setCursor(0, 1);
      lcd.write(LEFT_ARROW);
      lcd.print("    CEG   ");
      lcd.write(RIGHT_ARROW);
      break;

    case MELODY_C:
      lcd.setCursor(0, 1);
      lcd.write(LEFT_ARROW);
      lcd.print("     C    ");
      lcd.write(RIGHT_ARROW);
      break;

    case MELODY_CEGCGE:
      lcd.setCursor(0, 1);
      lcd.write(LEFT_ARROW);
      lcd.print("  CEGCGE  ");
      lcd.write(RIGHT_ARROW);
      break;
  }
}


//transitMode(int i):モードセレクト画面からのモードの切り替え
void transitMode(int i){
  switch(i){
    case SET_ALARM:
      mode = SET_ALARM;
      break;

    case SET_SNOOZE:
      mode = SET_SNOOZE;
      break;

    case SET_MELODY:
      mode = SET_MELODY;
      break;

    case ADJUST_TIME:
      mode = ADJUST_TIME;
      break;
  }
}

//setWakeUpTime():起床時刻の設定。最右桁で桁の右移動をするとdigits=4となり、confirmWakeUpTimeへ遷移
void setWakeUpTime(){
  displayLCD(SET_ALARM);

  //起床時刻のセット
  if(current_btn[0]==LOW && former_btn[0]!=current_btn[0]){//数値の増加
    time_wakeup[digits]++;
    if(time_wakeup[digits]>9) time_wakeup[digits] = 0;
  }else if(current_btn[3]==LOW && former_btn[3]!=current_btn[3]){//数値の減少
    time_wakeup[digits]--;
    if(time_wakeup[digits]<0) time_wakeup[digits] = 9;
  }else if(current_btn[1]==LOW && former_btn[1]!=current_btn[1]){//桁の右移動
    digits++;
  }else if(current_btn[2]==LOW && former_btn[2]!=current_btn[2]){//桁の左移動
    digits--;
    if(digits<0) digits = 3;
  }
}

void adjustTime(){
  displayLCD(ADJUST_TIME);

  //現在時刻のセット
  if(current_btn[0]==LOW && former_btn[0]!=current_btn[0]){//数値の増加
    time_now[digits]++;
    if(time_now[digits]>9) time_now[digits] = 0;
  }else if(current_btn[3]==LOW && former_btn[3]!=current_btn[3]){//数値の減少
    time_now[digits]--;
    if(time_now[digits]<0) time_now[digits] = 9;
  }else if(current_btn[1]==LOW && former_btn[1]!=current_btn[1]){//桁の右移動
    digits++;
    if(digits>3){
      //RTCに現在時刻をセット
      sendCurrentTime();
      mode = MODE_SELECT;

      //桁のリセット
      digits = 0;
    }
  }else if(current_btn[2]==LOW && former_btn[2]!=current_btn[2]){//桁の左移動
    digits--;
    if(digits<0) digits = 3;
  }
}

// displayLCD(int mode):LCDの画面表示
void displayLCD(int mode){
  //case内で変数宣言すると「crosses initialization of 'int millis_3digits'」というエラーが起きる
  int millis_3digits = millis() - millis()/1000*1000; //0～999ミリ秒の値

  switch(mode){
    case MODE_SELECT:
      lcd.setCursor(0,1);
      lcd.print("   ");
      lcd.write(CHONMAGE_SEIJIN);
      lcd.print(time_now[0]);
      lcd.print(time_now[1]);
      lcd.print(":");
      lcd.print(time_now[2]);
      lcd.print(time_now[3]);
      lcd.print(":");
      lcd.print(time_now[4]);
      lcd.print(time_now[5]);
      lcd.write(CHONMAGE_SEIJIN);
      lcd.print("   ");
      break;

    case SET_SNOOZE:
      lcd.setCursor(0,0);
      lcd.print("SET SNOOZE  ");
      displayBatteryCharge();
      lcd.setCursor(0,1);
      lcd.print("length: ");
      //snooze_length_minの点滅
      if(millis_3digits/300==0){
        for(int i=1;i<=getDigitsNum(snooze_length_min);i++) lcd.print(" ");
      }else{
        lcd.print(snooze_length_min);
      }
      lcd.print("min     ");
      break;

    case SNOOZE_MODE:
      lcd.setCursor(0,0);
      lcd.print("SNOOZE...   ");
      displayBatteryCharge();
      lcd.setCursor(0,1);
      lcd.print("   ");
      lcd.write(CHONMAGE_SEIJIN);
      lcd.print(time_now[0]);
      lcd.print(time_now[1]);
      lcd.print(":");
      lcd.print(time_now[2]);
      lcd.print(time_now[3]);
      lcd.print(":");
      lcd.print(time_now[4]);
      lcd.print(time_now[5]);
      lcd.write(CHONMAGE_SEIJIN);
      lcd.print("   ");
      break;

    case SOUND_ALARM:
      lcd.setCursor(0,0);
      lcd.print("WAKE UP!!   ");
      displayBatteryCharge();
      lcd.setCursor(0,1);
      lcd.print("   ");
      lcd.write(CHONMAGE_SEIJIN);
      lcd.print(time_now[0]);
      lcd.print(time_now[1]);
      lcd.print(":");
      lcd.print(time_now[2]);
      lcd.print(time_now[3]);
      lcd.print(":");
      lcd.print(time_now[4]);
      lcd.print(time_now[5]);
      lcd.write(CHONMAGE_SEIJIN);
      lcd.print("   ");
      break;

    case SET_ALARM:
      lcd.setCursor(0, 0);
      lcd.write("WAKEUP TIME?");
      displayBatteryCharge();
      lcd.setCursor(0,1);
      lcd.print("   ");
      lcd.write(CHONMAGE_SEIJIN);
      //time_wakeup[digits]を点滅させる
      for(int i=0;i<2;i++){
        if(millis_3digits/300==0 && i==digits){
          lcd.print(" ");
        }else{
          lcd.print(time_wakeup[i]);
        }
      }
      lcd.print(":");
      for(int i=2;i<4;i++){
        if(millis_3digits/300==0 && i==digits){
          lcd.print(" ");
        }else{
          lcd.print(time_wakeup[i]);
        }
      }
      lcd.print("   ");
      break;

    case CONFIRM_WAKEUP_TIME:
      lcd.setCursor(0, 0);
      lcd.print("WAKEUP AT   ");
      displayBatteryCharge();
      lcd.setCursor(0,1);
      lcd.write(LEFT_ARROW);
      lcd.print("NO ?");
      lcd.print(time_wakeup[0]);
      lcd.print(time_wakeup[1]);
      lcd.print(":");
      lcd.print(time_wakeup[2]);
      lcd.print(time_wakeup[3]);
      lcd.print("? YES");
      lcd.write(RIGHT_ARROW);
      break;

    case COMPLETE_SET_ALARM:
      lcd.setCursor(0,0);
      lcd.print("OK!     ");
      displayBatteryCharge();
      lcd.setCursor(0,1);
      lcd.print("  !");
      lcd.print(time_wakeup[0]);
      lcd.print(time_wakeup[1]);
      lcd.print(":");
      lcd.print(time_wakeup[2]);
      lcd.print(time_wakeup[3]);
      lcd.print("!   ");
      break;

    case ADJUST_TIME:
      lcd.setCursor(0, 0);
      lcd.print("CURRENTTIME?");
      displayBatteryCharge();
      lcd.setCursor(0,1);
      lcd.print("   ");
      lcd.write(CHONMAGE_SEIJIN);
      //time_now[digits]を点滅させる
      for(int i=0;i<2;i++){
        if(millis_3digits/300==0 && i==digits){
          lcd.print(" ");
        }else{
          lcd.print(time_now[i]);
        }
      }
      lcd.print(":");
      for(int i=2;i<4;i++){
        if(millis_3digits/300==0 && i==digits){
          lcd.print(" ");
        }else{
          lcd.print(time_now[i]);
        }
      }
      lcd.print("   ");
      break;

    case SET_MELODY:
      lcd.setCursor(0, 0);
      lcd.print("ALARMMELODY?");
      displayBatteryCharge();
      break;

    case WARN_LOW_BATTERY:
      lcd.setCursor(0, 0);
      lcd.print("CHANGE BATTERY!!");
      lcd.setCursor(0, 1);
      lcd.print("        ");
      break;

    case DISPLAY_CURRENT_TIME:
      lcd.setCursor(0,0);
      lcd.print("CURRENT TIME");
      displayBatteryCharge();
      lcd.setCursor(0,1);
      lcd.print("   ");
      lcd.write(CHONMAGE_SEIJIN);
      lcd.print(time_now[0]);
      lcd.print(time_now[1]);
      lcd.print(":");
      lcd.print(time_now[2]);
      lcd.print(time_now[3]);
      lcd.print(":");
      lcd.print(time_now[4]);
      lcd.print(time_now[5]);
      lcd.write(CHONMAGE_SEIJIN);
      lcd.print("   ");
      break;
  }
}

void confirmWakeUpTime(){
  displayLCD(CONFIRM_WAKEUP_TIME);

  //ボタンでの遷移動作
  if(current_btn[1]==LOW && former_btn[1]!=current_btn[1]){//YES
    //時刻指定完了の画面出力
    displayLCD(COMPLETE_SET_ALARM);

    //桁のリセット
    digits = 0;

    //指定時刻をRTCに送信
    sendWakeUpTime();

    //内蔵EEPROMに起床時刻を保存
    EEPROM.write(HOUR1_WAKEUP_ADRS, intToByte(time_wakeup[0]));
    EEPROM.write(HOUR2_WAKEUP_ADRS, intToByte(time_wakeup[1]));
    EEPROM.write(MINUTE1_WAKEUP_ADRS, intToByte(time_wakeup[2]));
    EEPROM.write(MINUTE2_WAKEUP_ADRS, intToByte(time_wakeup[3]));
    delay(10);

    //5秒間表示
    delay(5000);

    //LCDをOFF
    digitalWrite(V_IN_LCD, LOW);
    delay(10);

    //スリープモードへ遷移
    mode = SLEEP_MODE;

  }else if(current_btn[2]==LOW && former_btn[2]!=current_btn[2]){//NO
    digits = 0;
  }
}


//toSoundAlarm():SOUND_ALARMモードへ遷移(外部割込み発生時に呼び出される)
//soundAlarmの為の初期化処理も含む
void toSoundAlarm(){
  mode = SOUND_ALARM;

  alarm_repeat = 0;

  //LCDをON
  digitalWrite(V_IN_LCD, HIGH);
  lcd.begin(16, 2); //16文字×2行
  //lcd.clear();

  //ブザー鳴動時用動作周波数へ切り替え
  //CLKPR = (1<<CLKPCE);
  //CLKPR = BUZZER_CLK_DIV;

  //スヌーズボタンはランダムで決定する。
  stop_btn = millis()%4;
  //snooze_length_min=0ならstop_btnは無効化
  if(snooze_length_min==0) stop_btn = 100;
}

/*doNothing():何もしない。detachInterruptが期待通りに動かなかったので、
 *代わりにこのメソッドをattachすることで問題を解消。
 */
void doNothing(){
}

void getCurrentTime(){
  byte seconds,minutes,hours;
  Wire.requestFrom(RTC_ADRS,8);

  Wire.read();//control2
  seconds = Wire.read();
  minutes = Wire.read();
  hours = Wire.read();
  Wire.read();//weekdays
  Wire.read();//days
  Wire.read();//months
  Wire.read();//years

  time_now[0] = hours>>4;
  time_now[1] = hours&0x0F;
  time_now[2] = minutes>>4;
  time_now[3] = minutes & 0x0F;
  time_now[4] = seconds>>4;
  time_now[5] = seconds&0x0F;
}

void sendCurrentTime(){
  Wire.beginTransmission(RTC_ADRS);
  Wire.write(0x00);  //最初に書き込むアドレスを指定(0x00はsecレジスタのアドレス)
  Wire.write(0x00);  //sec
  Wire.write(intToByte(time_now[2])<<4 | intToByte(time_now[3]));  //min
  Wire.write(intToByte(time_now[0])<<4 | intToByte(time_now[1]));  //hour
  Wire.endTransmission();
  delay(1);
}

void sendWakeUpTime(){
  //詳しい資料は「http://akizukidenshi.com/download/ds/seikoepson/ETM10J_04_R8025SANB_ja.pdf」19Pに掲載
  byte reg_0xE0, reg_0xF0;
  Wire.requestFrom(RTC_ADRS,8);
  //HACK:Control1のアドレスを指定して一回で読み取りを済ませたいが、方法が分からない
  reg_0xF0 = Wire.read();//control2
  Wire.read();//seconds
  Wire.read();//minutes
  Wire.read();//hours
  Wire.read();//weekdays
  Wire.read();//days
  Wire.read();//months
  Wire.read();//years
  Wire.read();//Digital Offset
  Wire.read();//Alarm_W:Minute
  Wire.read();//Alarm_W:Hour
  Wire.read();//Alarm_W:Weekday
  Wire.read();//Alarm_D:Minute
  Wire.read();//Alarm_D:Hour
  Wire.read();//Reserved
  reg_0xE0 = Wire.read();//Control1

  //DALEビットを0に、DAFGビットを0にする(アラームD機能を使うための事前準備)
  bitClear(reg_0xE0,6);
  bitClear(reg_0xF0,0);
  Wire.beginTransmission(RTC_ADRS);
  Wire.write(0xE0);
  Wire.write(reg_0xE0);
  Wire.write(reg_0xF0);
  Wire.endTransmission();
  delay(1);

  //RTCに起床時刻を送信
  Wire.beginTransmission(RTC_ADRS);
  Wire.write(0xB0);
  Wire.write(intToByte(time_wakeup[2])<<4 | intToByte(time_wakeup[3]));  //Alarm_D;Minute
  Wire.write(intToByte(time_wakeup[0])<<4 | intToByte(time_wakeup[1]));  //Alarm_D;Hour
  Wire.endTransmission();
  delay(1);

  //DALEビットを1に戻す(アラームD機能を起動)
  bitSet(reg_0xE0,6);
  Wire.beginTransmission(RTC_ADRS);
  Wire.write(0xE0);
  Wire.write(reg_0xE0);
  Wire.endTransmission();
  delay(1);
}

//intToByte(int num):整数値を2進数に変換して、Byte型の値を返す。アルゴリズム的に負の値は引数に取れない。
byte intToByte(int num){
  int reverse_return_byte[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  byte return_byte=0x00;

  //このforループで10進数の数値を2進数に変換した逆順の値が得られる。
  //11(2進数だと1011)を引数に取ったなら、reverse_return_byteは{1, 1, 0, 1, 0, 0, 0, 0}となる。
  for(int i=0;num!=0;i++){
    reverse_return_byte[i] = num%2;
    num = num/2;
  }

  //得られた逆順の値を反転して正しい2進数の値を得る
  //reverse_return_byteが{1, 1, 0, 1, 0, 0, 0, 0}なら、return_byteは00001011となる。
  for(int i=7;i>=0;i--){
    return_byte = return_byte<<1;
    return_byte += reverse_return_byte[i];
  }
  return return_byte;
}

void displayBatteryCharge(){
  int battery_charge;
  battery_charge = analogRead(BATTERY_CHECKER);

  int battery_charge_rate = (battery_charge-V_MIN)*100 / (V_FULL-V_MIN);
  if(battery_charge_rate == 100){
    lcd.write(BATTERY_3BAR);
    lcd.print("FULL");
  }else if(battery_charge_rate >= 67){
    lcd.write(BATTERY_3BAR);
    lcd.print(battery_charge_rate);
    lcd.print("%");
  }else if(battery_charge_rate >= 34){
    lcd.write(BATTERY_2BAR);
    lcd.print(battery_charge_rate);
    lcd.print("%");
   }else if(battery_charge_rate >= 10){
    lcd.write(BATTERY_1BAR);
    lcd.print(battery_charge_rate);
    lcd.print("%");
   }else if(battery_charge_rate >= 0){
    lcd.write(BATTERY_0BAR);
    lcd.print(" ");
    lcd.print(battery_charge_rate);
    lcd.print("%");
   }else{
    //電池交換警告画面へ
    mode = WARN_LOW_BATTERY;
   }
}

void toDisplayCurrentTime_SLEEP_MODE(){
  mode = DISPLAY_CURRENT_TIME;

  //LCDをON
  digitalWrite(V_IN_LCD, HIGH);
  lcd.begin(16, 2); //16文字×2行
  //lcd.clear();
}

void toDisplayCurrentTime_SNOOZE_MODE(){
  mode = SNOOZE_MODE;
  //LCDをON
  digitalWrite(V_IN_LCD, HIGH);
  lcd.begin(16, 2); //16文字×2行
  //lcd.clear();
}

int getDigitsNum(int num){
  if(num == 0) return 1;

  int digitsnum = 0;
  while(num != 0){
    num /= 10;
    digitsnum++;
  }

  return digitsnum;
}
