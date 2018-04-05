volatile int value = 0;
volatile uint8_t prev = 0;
volatile uint8_t prevA = 0;
volatile uint8_t prevB = 0;

void setup() 
{
  pinMode(2, INPUT_PULLUP); 
  pinMode(3, INPUT_PULLUP);
  
  digitalWrite(2, HIGH);
  digitalWrite(3, HIGH);
  
  Serial.begin(9600);
}

void updateEncoder(uint8_t a, uint8_t b)
{
  uint8_t ab = (a << 1) | b;
  uint8_t encoded  = (prev << 2) | ab;

  // if(encoded == 0b1101 || encoded == 0b0100 || encoded == 0b0010 || encoded == 0b1011){
  if(encoded == 0b1101){
    value ++;
  // } else if(encoded == 0b1110 || encoded == 0b0111 || encoded == 0b0001 || encoded == 0b1000) {
  } else if(encoded == 0b1110) {
    value --;
  }

  prev = ab;
}

void loop()
{
    int a = (PIND & _BV(2))>>2;
    int b = (PIND & _BV(3))>>3;

    if( a != prevA || b != prevB ){
      updateEncoder(a, b);
      // delay(60);
    }

    prevA = a;
    prevB = b;
   Serial.println(value);
}