const byte startBit = 0;
const byte stopBit = 1;
const byte evenPar = 0; //Parity bit when there are even 1's

uint16_t serialize(byte b) {
  uint16_t msg = 0;
  msg += stopBit;
  msg = (msg << 1) + getPar(b);

  for (int i = 7; i >= 0; i--) { //LSB is entered first
    msg = (msg << 1) + ((b >> i) & 1);
  }

  msg = (msg << 1) + startBit;
  return msg;
}

bool deserialize(uint16_t msg, byte* b) {
  if (msg & 1 != startBit) return false;
  if ((msg & _BV(10)) >> 10 != stopBit) return false;
  bool par = (msg & _BV(9)) >> 9;

  msg >>= 1;
  byte t = 0;
  for (int i = 7; i >= 0; i--) {
    t = (t << 1) + ((msg >> i) & 1);
  }

  if (par != getPar(t)) return false;

  *b = t;
  return true;
}

bool getPar(byte b) {
  int ones = 0;
  for (int i = 0; i < 8; i++) if ((b >> i) & 1 == 1) ones++;
  if (ones % 2 == 0) return evenPar;
  else return !evenPar;
}

void printMsg(uint16_t msg) {
  Serial.print("Msg: ");
  for (int i = 10; i >= 0; i--) {
    Serial.print((msg & _BV(i)) >> i);
  }
  Serial.println();
}

void printByte(byte b) {
  Serial.print("Byte: ");
  for (int i = 7; i >= 0; i--) {
    Serial.print((b & _BV(i)) >> i);
  }
  Serial.println();
}

void selfTest() {
  byte b = 124;
  uint16_t msg = serialize(b);
  byte c = 0;
  bool ret = deserialize(msg, &c);
  if (!ret || c != b) Serial.println("TEST_FAILED: Serialize and deserialize 124");

  b = 0;
  msg = serialize(b);
  c = 255;
  ret = deserialize(msg, &c);
  if (!ret || c != b) Serial.println("TEST_FAILED: Serialize and deserialize 0");

  b = 255;
  msg = serialize(b);
  c = 0;
  ret = deserialize(msg, &c);
  if (!ret || c != b) Serial.println("TEST_FAILED: Serialize and deserialize 255");

  b = 0b00000001;
  msg = serialize(b);
  msg ^= (1 << 9); //Flip parity bit
  c = 0;
  ret = deserialize(msg, &c);
  if (ret) Serial.println("TEST_FAILED: Serialize and deserialize bad par");

  b = 0b10000000;
  msg = serialize(b);
  msg ^= (1 << 3); //Flip the fourth bit in the message
  c = 0;
  ret = deserialize(msg, &c); //Should be caught by parity
  if (ret) Serial.println("TEST_FAILED: Serialize and deserialize bad content");

  b = 0b10001100;
  msg = serialize(b);
  msg ^= 1; //Flip the start bit in the message
  c = 0;
  ret = deserialize(msg, &c); //Should be caught by parity
  if (ret) Serial.println("TEST_FAILED: Serialize and deserialize bad start");

  b = 0b10000100;
  msg = serialize(b);
  msg ^= (1 << 10); //Flip the stop bit in the message
  c = 0;
  ret = deserialize(msg, &c); //Should be caught by parity
  if (ret) Serial.println("TEST_FAILED: Serialize and deserialize bad stop");
}
