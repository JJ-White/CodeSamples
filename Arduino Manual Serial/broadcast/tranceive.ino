#include "TimerOne.h"
//Settings
const int tx = 4;
const int rx = 2;
const int frequency = 22; // bits per second

//Send
uint16_t sendMsg = 0;
int sendMsgIndex = 0;
const int resendDelay = 32;
int resendDelayCounter = 0;

//Receive
uint16_t recMsg = 0;
int recMsgIndex = 0;
byte prevRecBit = 0;
const int sampleCount = 16;
int interruptCount = 0;
const int bitSampleCount = 15;
int bitCount = 0;

void callback() {
  //Sampling at 16x (sampleCount) send rate (frequency)
  if (interruptCount == sampleCount) {
    Send();
    interruptCount = 0;
  } interruptCount++;

  Receive();
  checkForMsg();
}

void Send() {
  if (sendMsgIndex >= 11) digitalWrite(tx, HIGH); //Set output high when there is no transmission
  else if (resendDelayCounter > 0) { //Waiting to resend message
    resendDelayCounter--;
    if (resendDelayCounter == 0) {
      Serial.println("Resend");
    }
    return;
  }
  else {
    if ((sendMsg >> sendMsgIndex) & 1) {
      digitalWrite(tx, HIGH);

      //TODO: Improve detection
      delayMicroseconds(500);
      if (digitalRead(rx) != 1) { //If bit is interrupted
        resendDelayCounter = resendDelay; //Wait x cycles
        sendMsgIndex = -1;
        Serial.println("Interrupted");
      }

    }
    else digitalWrite(tx, LOW);
    sendMsgIndex++;

  }
}

void Receive() {
  /*Serial.print(digitalRead(rx) * 14);
    Serial.print(",");
    Serial.println(bitCount);*/

  if (recMsgIndex > 10) return; //The message buffer is full

  byte b = digitalRead(rx);
  if (recMsgIndex == 0 && b != startBit) return; //A message must start with the startBit
  if (b == prevRecBit) {
    bitCount++;
    if (bitCount == bitSampleCount) {
      recMsg = (recMsg >> 1) + (prevRecBit << 10);
      recMsgIndex++;
      bitCount = 0;
    }
  }
  else {
    prevRecBit = b;
    bitCount = 1;
  }
}

void transmit(byte b) {
  while(sendMsgIndex < 11) delay(5); //Another message is being transmitted
  sendMsg = serialize(b);
  sendMsgIndex = 0;
  Serial.print("Send: ");
  Serial.println((char)b);
}

void checkForMsg() {
  if (recMsgIndex > 10) {
    byte b = 0;
    bool r = deserialize(recMsg, &b);
    if (r) {
      Serial.print("Rcvd: ");
      Serial.println((char)b);
    }
    recMsgIndex = 0;
  }
}

void setupTrans() {
  Serial.begin(250000);
  Serial.println("Start");

  selfTest();

  pinMode(tx, OUTPUT);
  digitalWrite(tx, HIGH);
  pinMode(rx, INPUT);
  Timer1.initialize(1000000 / (frequency * 16)); // in microseconds
  Timer1.attachInterrupt(callback);

  delay(1000);
}
