void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
}

int val = 0;
void loop() {
  // put your main code here, to run repeatedly:
  analogWrite(3, 10);
  delay(1000);
  analogWrite(3, 29);
  delay(1000);
  analogWrite(3, 49);
  delay(1000);
  analogWrite(3, 69);
  delay(1000);
  analogWrite(3, 88);
  delay(1000);
}
