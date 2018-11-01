#include <EEPROM.h>
#include <stdio.h>
#include <string.h>

char s[] = "1234567890-_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ~!@#$%^&*()_+`-=[]\\{}|;':\",./<>?";
const int SSID_OFFSET = 0;
const int PASSWORD_OFFSET = 40;

int ssid_length;
int pass_length;
char ssid[40];
char pass[70];

void setup() {
  Serial.begin(9600);
  if(ssid[0] == '\0'){
    Serial.println("aaaa");
  }

  // Read SSID and password from EEPROM
  int ssid_len = EEPROM.read(SSID_OFFSET);
  for(int i = 0; i < ssid_len; i++){
    ssid[i] = EEPROM.read(SSID_OFFSET + 1 + i) + 0x30;
  }
  int pass_len = EEPROM.read(PASSWORD_OFFSET);
  for(int i = 0; i < pass_len; i++){
    pass[i] = EEPROM.read(PASSWORD_OFFSET + 1 + i) + 0x30;
  }
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("PASSWORD: ");
  Serial.println(pass);

  strcpy(ssid, "dokadokadokad");
  strcpy(pass, "n1L1>nnr4A");
}

void loop() {

  // Check output for char to uint8_t
  for (int i = 0; i < 96; i++){
    int8_t a = s[i] - 0x30;
    Serial.print(a);
  }

  Serial.println("");
  Serial.println("---------");

  // Check output for uint8_t to char
  for (int i = 0; i < 96; i++){
    char c = (uint8_t)(s[i] - 0x30) + 0x30;
    Serial.print(c);
  }

  Serial.println("");
  Serial.println("---------");

  // Write SSID and password to EEPROM
  EEPROM.write(SSID_OFFSET, strlen(ssid));
  for (int i = 0; i < strlen(ssid); i++){
    EEPROM.write(SSID_OFFSET + 1 + i, ssid[i] - 0x30);
  }
  EEPROM.write(PASSWORD_OFFSET, strlen(pass));
  for (int i = 0; i < strlen(pass); i++){
    EEPROM.write(PASSWORD_OFFSET + 1 + i, pass[i] - 0x30);
  }

  // end
  while(1){}

}
