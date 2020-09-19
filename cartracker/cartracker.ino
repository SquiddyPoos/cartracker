#include <SoftwareSerial.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <EEPROM.h>

#define RX_PIN 4
#define TX_PIN 5
#define SLEEP_PIN 7
#define ON_OFF 6

#define LO_RX_PIN 3
#define LO_TX_PIN 2
#define LO_SET 13

#define BUZ_PIN 9
#define LED_PIN 10

#define BLK_DUR 5000
#define BUZ_DUR 20000

SoftwareSerial IOT(RX_PIN, TX_PIN);
SoftwareSerial lora(LO_RX_PIN, LO_TX_PIN);

const byte IR_ADDR_R[6] = "00001";
const byte IR_ADDR_W[6] = "00002";

bool blinkOn = false;
bool masterBuzOn = false;
bool lightOn = false;
bool buzzerOn = false;
int toNextBlink = BLK_DUR;
int toNextBuzzerOff = BUZ_DUR;

/*void turnOn() {
  //pulse off then on
  digitalWrite(ON_OFF, LOW);
  delay(1050); //At least 1s (datasheet)
  digitalWrite(ON_OFF, HIGH);
  //wait for UART port -> from datasheet, minimum 4.5s. Spam "AT" and wait for "OK"
  while (!IOT.available()) {
    Serial.println("Sent AT signal...");
    IOT.write("AT\r\n");
    delay(1000);
  }
  //read all
  while (IOT.available()) {
    IOT.read();
  }
}*/

/*String getGPSData() {
  IOT.write("AT+CGNSPWR=1\r\n");
  IOT.write("AT+CGNSINF\r\n");
  String data = "";
  while (IOT.available()) {
    data += IOT.read();
  }
  return data;
}*/

void sleepOn() {
  digitalWrite(SLEEP_PIN, HIGH);
}

void sleepOff() {
  digitalWrite(SLEEP_PIN, LOW);
}

void writeAlm(bool onOff) {
  EEPROM.write(0, onOff);
}

bool getAlm() {
  int onoff = EEPROM.read(0);
  if (onoff == 255) {
    EEPROM.update(0, 0);
    onoff = 0;
  }
  return onoff;
}

void checkLO() {
  if (lora.available()) {
    Serial.write(lora.read());
  }
}

void alarmOn(bool buzzer, bool light) {
  if (buzzer) {
    tone(BUZ_PIN, 512);
  } else {
    noTone(BUZ_PIN);
  }
  if (light) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void setup() {
  Serial.begin(9600);
  IOT.begin(9600);
  lora.begin(9600);
  lora.listen();
  pinMode(ON_OFF, OUTPUT);
  //pinMode(SLEEP_PIN, OUTPUT);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LO_SET, OUTPUT);
  digitalWrite(ON_OFF, HIGH);
  digitalWrite(A0, LOW);
  digitalWrite(A1, LOW);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LO_SET, LOW);
  lora.print("AA FA AA\r\n");
  Serial.print(lora.available());
  noTone(BUZ_PIN);
  delay(100);
  //turnOn();
  Serial.println("Chip OK");
}

void loop() {
  checkLO();

  if (blinkOn) {
    toNextBlink--;
  }

  if (masterBuzOn) {
    toNextBuzzerOff--;
  }

  if (toNextBlink <= 0) {
    lightOn = !lightOn;
    alarmOn(buzzerOn, lightOn);
    toNextBlink = BLK_DUR;
  }

  if (toNextBuzzerOff <= 0) {
    buzzerOn = !buzzerOn;
    alarmOn(buzzerOn, lightOn);
    toNextBuzzerOff = BUZ_DUR;
  }
}
