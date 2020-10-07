#include <SoftwareSerial.h>

// Configuration: change settings here
#define LO_RX_PIN 3     /* Note: the one labeled "TX", not "RX" */
#define LO_TX_PIN 2     /* Note: the one labeled "RX", not "TX" */
#define LO_SET    7     /* Labeled "SET" */   
#define FREQUENCY 28
/* Values below are in hexadecimal. You can use 00 - FF. */
/* Do not remove the 0x part. Only use the last 2 digits. */
#define NET_ID_1  0x12 
#define NET_ID_2  0x12
#define NET_ID_3  0x10
#define NET_ID_4  0x12
#define NODE_ID_1 0x00
#define NODE_ID_2 0x0A
// Do not modify below code.

SoftwareSerial lora(LO_RX_PIN, LO_TX_PIN);

void setup() {
  // put your setup code here, to run once:
  pinMode(LO_RX_PIN, INPUT);
  pinMode(LO_TX_PIN, OUTPUT);
  pinMode(LO_SET, OUTPUT);
  digitalWrite(A2, LOW);
  lora.begin(9600);
  delay(100);
  byte command[33] = {0xAA, 0xFA, 0x03, FREQUENCY, 0x04, 0x03, 0x07, 0x03, 0x02, 0x01, 0x01, NET_ID_1, NET_ID_2, NET_ID_3, NET_ID_4, NODE_ID_1, NODE_ID_2, 0x00, 0x00, 0x00, 0x00, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
  lora.write(command, 33);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  while (lora.available()) {
    /*byte msg[30];
    lora.readBytes(msg, 30);
    for(int i = 0; i < 30; i++) {
      Serial.print(String(msg[i]) + " ");
    }*/
    Serial.write(lora.read());
  }
}
