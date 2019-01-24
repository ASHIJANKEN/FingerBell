#define READ_PIN A0

void setup(){
  Serial.begin(9600);
	pinMode(READ_PIN, INPUT);
}

void loop(){
	Serial.print("Val: ");
	Serial.println(analogRead(READ_PIN));
    delay(100);
}
