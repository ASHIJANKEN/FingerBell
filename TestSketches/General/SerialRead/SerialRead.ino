#include <stdio.h>
#include <SoftwareSerial.h>

SoftwareSerial Serial1(A4, A5); // RX, TX

void setup() {
    Serial.begin(9600);  // Initialize serial for debugging
    Serial1.begin(9600);
}

void loop() {
    int bufnum = Serial1.available();
    if(bufnum != 0){
        Serial.print("bufnum: ");
        Serial.println(bufnum);
        for(int i = 0; i < bufnum; i++){
            Serial.print((char)Serial1.read());
        }
        Serial.println();
        Serial.println("--------------------");
    }

    // Serial.println(millis());
    // Serial1.println(millis());
    // delay(1000);
}