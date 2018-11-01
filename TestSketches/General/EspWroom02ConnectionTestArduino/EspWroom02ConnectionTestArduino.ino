#include <SoftwareSerial.h>

SoftwareSerial Serial1(6, 7); // RX, TX

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  delay(10);
  Serial1.println("testesp>?!");
}

void loop() {
  while(Serial1.available()){
    char c = Serial1.read();
    Serial.print(c);
  }
}
