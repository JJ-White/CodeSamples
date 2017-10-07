void setup() {
  setupTrans();
  delay(1000);
}

void loop() {
  delay(9000);
  stringSend("World");
}

void stringSend(String s){
  for(int i = 0; i < s.length(); i++){
    transmit(s[i]);
  }
}

