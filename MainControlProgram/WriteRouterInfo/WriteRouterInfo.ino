#include <EEPROM.h>
#include <stdio.h>
#include <string.h>

// Address of router's SSID and PASSWORD
#define SSID_OFFSET 6 // 32 characters at maximum
#define PASSWORD_OFFSET 40 // 64 characters at maximum

int ssid_length;
int pass_length;
char ssid[] = "SSID";
char pass[] = "PASS";

void setup() {
    Serial.begin(9600);

    // Write SSID and password to EEPROM
    EEPROM.write(SSID_OFFSET, strlen(ssid));
    for (int i = 0; i < strlen(ssid); i++){
        EEPROM.write(SSID_OFFSET + 1 + i, ssid[i] - 0x30);
    }
    EEPROM.write(PASSWORD_OFFSET, strlen(pass));
    for (int i = 0; i < strlen(pass); i++){
        EEPROM.write(PASSWORD_OFFSET + 1 + i, pass[i] - 0x30);
    }

    Serial.println("Flashed to EEPROM.");
}

void loop() {
}
