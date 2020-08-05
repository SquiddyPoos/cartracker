#include <SoftwareSerial.h>

#define RX_PIN 10
#define TX_PIN 11
#define ON_OFF 12

void turnOn() {
  digitalWrite(ON_OFF, HIGH);
}

SoftwareSerial Sim7000C(10, 11);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Sim7000C.begin(115200);
  pinMode(ON_OFF, OUTPUT);
  turnOn();
}

void loop() {
  // put your main code here, to run repeatedly:
  while (Sim7000C.available()) {
    Serial.write(Sim7000C.read());
  }

  while (Serial.available()) {
    Sim7000C.write(Serial.read());
  }
}
