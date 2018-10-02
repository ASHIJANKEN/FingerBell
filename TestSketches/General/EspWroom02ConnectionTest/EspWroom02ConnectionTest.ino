#include <SoftwareSerial.h>

SoftwareSerial Serial1(6, 7); // RX, TX

void setup() {
  Serial.begin(9600);

  Serial.print("\r\nStart\r\n");
  Serial1.begin(115200);
  delay(10);
  Serial1.println("AT+UART_CUR=9600,8,1,0,0");
  Serial1.begin(9600);
  delay(10);
}

void loop() {
  // Serial.println(Serial1.available());
  if (Serial1.available() > 0) {
    byte c = Serial1.read();
    Serial.write(c);
  }
  if (Serial.available() > 0) {
    byte c = Serial.read();
    Serial1.write(c);
  }
}
