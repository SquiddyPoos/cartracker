#include <toneAC.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <EEPROM.h>

#define RX_PIN 4
#define TX_PIN 5
#define SLEEP_PIN 7
#define ON_OFF 6

#define LO_RX_PIN 3
#define LO_TX_PIN 2
#define LO_SET 7

#define BUZ_PIN 9
#define LED_PIN 8

#define BLK_DUR 5000
#define BUZ_DUR 20000

SoftwareSerial IOT(RX_PIN, TX_PIN);
SoftwareSerial lora(LO_RX_PIN, LO_TX_PIN);

bool blinkOn = false;
bool masterBuzOn = false;
bool lightOn = false;
bool buzzerOn = false;
int volume = 10; // Only from 0 to 10
int toNextBlink = BLK_DUR;
int toNextBuzzerOff = BUZ_DUR;
byte current_cmd[100];
int current_cmd_idx = 0;

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
  EEPROM.update(0, onOff);
}

void writeParkingLot(byte* no) {
  // In its final form, this function will call the website
  // For now, it doesn't
  for (int i = 0; i < 5; i++) {
    EEPROM.update(20 + i, no[i]);
  }
}

byte* getParkingLot() {
  // In its final form, this function will call the website
  // For now, it doesn't
  byte lot[5] = {0, 0, 0, 0, 0};
  for (int i = 0; i < 5; i++) {
    lot[i] = EEPROM.read(20 + i);
  }
  return lot;
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
  while (lora.available()) {
    //Commands end with 197 (a kinda random no.)
    byte next = lora.read();
    if (next == 197) {
      // Read from our command, in current_cmd
      Serial.print("Command recieved: ");
      for (int i = 0; i < current_cmd_idx; i++) {
        Serial.print(current_cmd[i], DEC);
        Serial.print(' ');
      }
      Serial.println();
      // Get command
      byte cmd = current_cmd[0];
      if (cmd == 1) {
        // get alarm command
        byte ret_msg[3] = {1, getAlm(), 197};
        lora.write(ret_msg, 3);
      } else if (cmd == 2) {
        // get parking number
        byte* lot = getParkingLot();
        byte msg[7] = {2, 0, 0, 0, 0, 0, 197};
        for (int i = 1; i < 6; i++) {
          msg[i] = *(lot + i - 1);
        }
        lora.write(msg, 7);
      } else if (cmd == 3) {
        // set alarm command
        writeAlm(current_cmd[1]);
        if (current_cmd[1]) {
          bool masterBuzOn = current_cmd[2];
          int volume = current_cmd[3] / 10;
          bool lightOn = current_cmd[4];
          bool blinkOn = current_cmd[5];
          int toNextBlink = BLK_DUR;
          int toNextBuzzerOff = BUZ_DUR;
        } else {
          bool blinkOn = false;
          bool masterBuzOn = false;
          bool lightOn = false;
          bool buzzerOn = false;
          int volume = 10; // Only from 0 to 10
          int toNextBlink = BLK_DUR;
          int toNextBuzzerOff = BUZ_DUR;
          alarmOn(buzzerOn, lightOn);
        }
        byte ret_msg[2] = {3, 197}; // A simple acknowledgement
        lora.write(ret_msg, 2);
      }
      // Reset current_cmd_idx to 0
      memset(current_cmd, 0, 100);
      current_cmd_idx = 0;
    } else {
      current_cmd[current_cmd_idx] = next;
      current_cmd_idx++;
    }
  }
}

void alarmOn(bool buzzer, bool light) {
  if (buzzer) {
    toneAC(512, volume);
  } else {
    noToneAC();
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
  pinMode(LED_PIN, OUTPUT);
  pinMode(LO_SET, OUTPUT);
  digitalWrite(ON_OFF, HIGH);
  digitalWrite(A0, LOW);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LO_SET, HIGH);
  noToneAC();
  // Read all from the LoRa
  while (lora.available()) {
    lora.read();
  }
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
