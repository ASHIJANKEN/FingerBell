void setup(){
	Serial.begin(9600);
}

unsigned long ms = millis();
void loop(){
	if(millis() - ms > 1000){
		Serial.println(millis() / 1000);
		ms = millis();
	}

	while(Serial.available()){
		char c = Serial.read();
		Serial.print(c);
	}
}