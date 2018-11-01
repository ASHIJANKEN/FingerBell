void setup(){
	Serial.begin(9600);
}

void loop(){
	char a[] = "abcdefghijklmnopqrstuvwxyz";

	// Serial.print("Result of subString():");
	String b = subString(a, "bc", "jk");
	char* c = "asd";
	char* d = itoa(5, d, 10);
	char* e = c + d;
	Serial.println(e);
	Serial.println(b);
	 b = subString(a, "jkl", "z");
	Serial.println(b);
	 b = subString(a, "m", "uvwx");
	Serial.println(b);

	while(1){}
}

String subString(char* buf, char* start_tag, char* end_tag){
	String buf_str = buf;
    int start_indexof = buf_str.indexOf(start_tag);
    int end = buf_str.indexOf(end_tag, start_indexof);

    if(start_indexof == -1 || end == -1){
    	return "";
    }

    int start = start_indexof + strlen(start_tag);
    int length = end - start;
	Serial.println(start);
	Serial.println(end);
	Serial.println(length);
    char* ret = new char[length];

    strncpy(ret, buf + start, length);
    String ret_str = ret;

    return ret_str;
}