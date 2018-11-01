#include <EEPROM.h>
#include <stdio.h>
#include <string.h>

String s = "~!%40%23%24%25%5E%26*()_%2B%60-%3D%5B%5D%5C%5C%7B%7D%7C%3B'%3A%5C%22%2C.%2F%3C%3E%3F";

void setup() {
  Serial.begin(9600);

  Serial.print("ANSWER: ");
  Serial.println("~!@#$%^&*()_+`-=[]\\{}|;':\",./<>?");
  Serial.print("RESULT: ");
  Serial.println(urldecode(s));
}

void loop() {
}

String urldecode(String str)
{

    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{

        encodedString+=c;
      }

      yield();
    }

   return encodedString;
}

unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}