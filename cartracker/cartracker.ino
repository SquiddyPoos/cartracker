#include <SoftwareSerial.h>

#define RX_PIN 10
#define TX_PIN 11
#define SLEEP_PIN 13
#define ON_OFF 12

SoftwareSerial IOT(RX_PIN, TX_PIN);

void turnOn() {
  //pulse off then on
  digitalWrite(ON_OFF, LOW);
  delay(1050); //At least 1s (datasheet)
  digitalWrite(ON_OFF, HIGH);
  //wait for UART port -> from datasheet, minimum 4.5s. Spam "AT" and wait for "OK"
  while (!IOT.available()) {
    IOT.write("AT\r\n");
    delay(250);
  }
  //read all
  while (IOT.available()) {
    IOT.read();
  }
}

String getGPSData() {
  IOT.write("AT+CGNSPWR=1\r\n");
  IOT.write("AT+CGNSINF\r\n");
  String data = "";
  while (IOT.available()) {
    data += IOT.read();
  }
  return data;
}

void sleepOn() {
  digitalWrite(SLEEP_PIN, HIGH);
}

void sleepOff() {
  digitalWrite(SLEEP_PIN, LOW);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  IOT.begin(9600);
  pinMode(ON_OFF, OUTPUT);
  //pinMode(SLEEP_PIN, OUTPUT);
  digitalWrite(ON_OFF, HIGH);
  delay(100);
  turnOn();
  Serial.println("Chip OK");
}

void loop() {
  // put your main code here, to run repeatedly:
  while (IOT.available()) {
    Serial.write(IOT.read());
  }

  while (Serial.available()) {
    IOT.write(Serial.read());
  }
}
