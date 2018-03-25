

void setup(){
  pinMode(7, INPUT_PULLUP);
  pinMode(13, OUTPUT);
}

void loop(){
  if(digitalRead(7) == LOW){
    digitalWrite(13, HIGH);
  }else{
    digitalWrite(13, LOW);
  }
}
