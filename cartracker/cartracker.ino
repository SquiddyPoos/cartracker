//#include <SoftwareSerial.h>
#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <EEPROM.h>

#define RX_PIN 4
#define TX_PIN 5
#define SLEEP_PIN 7
#define ON_OFF 6

#define IR_CSN 3
#define IR_CE 2

#define BUZ_PIN 9
#define LED_PIN 10

#define BLK_DUR 5000
#define BUZ_DUR 20000

//SoftwareSerial IOT(RX_PIN, TX_PIN);
RF24 IR(IR_CE, IR_CSN);

const byte IR_ADDR_R[6] = "00001";
const byte IR_ADDR_W[6] = "00002";

bool blinkOn = false;
bool masterBuzOn = false;
bool lightOn = false;
bool buzzerOn = false;
int toNextBlink = BLK_DUR;
int toNextBuzzerOff = BUZ_DUR;

bool sendIRData(char* text, int tsize) {
  bool done = false;
  IR.stopListening();
  done = IR.write(&text, tsize);
  IR.startListening();
  return done;
}

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

void checkIR() {
  if (IR.available()) {
    char text[32];
    IR.read(&text, 32);
    String stext = String(text);
    Serial.println("T: "+stext);
    if (stext == "GET ALM") {
      char text[] = {0};
      String(getAlm()).toCharArray(text, 1);
      Serial.println(sendIRData(text, sizeof(text)));
    } else if (stext.substring(0, 7) == "SET ALM") {
      Serial.print("DETECT SET");
      bool onoff = stext[8] - 'A'; //A -> false, B -> true
      if (!onoff) {
        buzzerOn = false;
        lightOn = false;
        blinkOn = false;
        masterBuzOn = false;
        toNextBlink = BLK_DUR;
        toNextBuzzerOff = BUZ_DUR;
      } else {
        masterBuzOn = stext[9] - 'A'; // buzzer on/off
        lightOn = stext[10] - 'A'; // light
        blinkOn = stext[11] - 'A'; // blink
      }
      writeAlm(onoff);
    }
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
  //IOT.begin(9600);
  IR.begin();
  IR.openReadingPipe(1, IR_ADDR_R);
  IR.openWritingPipe(IR_ADDR_W);
  IR.startListening();
  pinMode(ON_OFF, OUTPUT);
  //pinMode(SLEEP_PIN, OUTPUT);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(ON_OFF, HIGH);
  digitalWrite(A0, LOW);
  digitalWrite(A1, LOW);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZ_PIN);
  delay(100);
  //turnOn();
  Serial.println("Chip OK");
  Serial.println(IR.isChipConnected());
}

void loop() {
  checkIR();

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
