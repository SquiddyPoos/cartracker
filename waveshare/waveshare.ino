#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Waveshare_ILI9486.h>
#include <EEPROM.h>
#include <nRF24L01.h>
#include <RF24.h>

//The IR module also uses SPI
//SPI is done by connecting to pins 11 -> IR MISO, 12 -> IR MOSI, 13 -> SCK
//A0 for CSN (Chip select, but active low)
#define IR_CSN A0
#define IR_CE A1

// Assign human-readable names to some common 16-bit color values:
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define LBLUE   0xD7FF

#define SLEN        105
#define KEYPAD_SIZE 16
#define AM_SIZE     19
#define NZ_SIZE     19

Waveshare_ILI9486 Waveshield;
RF24 IR(IR_CE, IR_CSN);

const byte IR_ADDR_W[6] = "00001";
const byte IR_ADDR_R[6] = "00002";

bool sound = true;
int volume = 100;
bool light = true;
bool lblink = true;
String parking_no = "";
bool alarm_on = false;
int screen = 0; //0 -> main, 1 -> keypad, 2 -> calibration, 3 -> loading, 4 -> confirmation, 5 -> A - N, 6 -> N - Z
bool currently_pressed = false; //buttons only work if this is false
String current_number = "";

TSConfigData tscd = {100, 939, 59, 970}; //calibration, determined experimentally (from calibration.ino)

//number - x1 - y1 - x2 - y2
//x -> 0 - 320
//y -> 200 - 480
//-1 -> bcsp, -2 is a back button, -3 is enter, -4 is remove button (revert to not set), -5 -> A - N, -6 -> O - Z
//Order is for aesthetic purposes (drawing is not instant)
int keypad_hitbox[KEYPAD_SIZE][5] = {
  {-2, 0, 0, 140, 50},
  {-4, 190, 0, 320, 50},
  {-5, 0, 150, 80, 200},
  {-6, 240, 150, 320, 200},
  {1, 0, 200, 106, 270},
  {2, 106, 200, 213, 270},
  {3, 213, 200, 320, 270},
  {6, 213, 270, 320, 340},
  {5, 106, 270, 213, 340},
  {4, 0, 270, 106, 340},
  {7, 0, 340, 106, 410},
  {8, 106, 340, 213, 410},
  {9, 213, 340, 320, 410},
  {-1, 213, 410, 320, 480},
  {0, 106, 410, 213, 480},
  {-3, 0, 410, 106, 480}
};

int AM_hitbox[AM_SIZE][5] = {
  {-2, 0, 0, 140, 50},
  {-4, 190, 0, 320, 50},
  {-5, 0, 150, 80, 200},
  {-6, 240, 150, 320, 200},
  {'A', 0, 200, 80, 270},
  {'B', 80, 200, 160, 270},
  {'C', 160, 200, 240, 270},
  {'D', 240, 200, 320, 270},
  {'H', 240, 270, 320, 340},
  {'G', 160, 270, 240, 340},
  {'F', 80, 270, 160, 340},
  {'E', 0, 270, 80, 340},
  {'I', 0, 340, 80, 410},
  {'J', 80, 340, 160, 410},
  {'K', 160, 340, 240, 410},
  {'L', 240, 340, 320, 410},
  {-1, 213, 410, 320, 480},
  {'M', 106, 410, 213, 480},
  {-3, 0, 410, 106, 480}
};

int NZ_hitbox[NZ_SIZE][5] = {
  {-2, 0, 0, 140, 50},
  {-4, 190, 0, 320, 50},
  {-5, 0, 150, 80, 200},
  {-6, 240, 150, 320, 200},
  {'N', 0, 200, 80, 270},
  {'O', 80, 200, 160, 270},
  {'P', 160, 200, 240, 270},
  {'Q', 240, 200, 320, 270},
  {'U', 240, 270, 320, 340},
  {'T', 160, 270, 240, 340},
  {'S', 80, 270, 160, 340},
  {'R', 0, 270, 80, 340},
  {'V', 0, 340, 80, 410},
  {'W', 80, 340, 160, 410},
  {'X', 160, 340, 240, 410},
  {'Y', 240, 340, 320, 410},
  {-1, 213, 410, 320, 480},
  {'Z', 106, 410, 213, 480},
  {-3, 0, 410, 106, 480}
};

int lot_no_hb[4] = {15, 115, 155, 165}; //hitbox for the change lot button
int st_alm_hb[4] = {165, 115, 305, 165}; //hitbox for start/stop alarm
int snd_btn_hb[4] = {170, 242, 235, 270}; //sound button hitbox
int lgt_btn_hb[4] = {170, 337, 235, 365}; //light button hitbox
int blk_btn_hb[4] = {170, 377, 235, 405}; //blink button hitbox
int vlm_btn_hb[5] = {86, 303, SLEN + 124, 331, 105}; //volume button hitbox, along with start pos. quite a lot longer than the act bar, to allow 0% and 100%
int reset_btn_hb[4] = {120, 420, 200, 445}; //reset button hitbox

int ok_btn_hb[4] = {120, 430, 200, 470}; //Used in the calibration screen's OK button

int yes_btn_hb[4] = {50, 250, 150, 300}; //Used in confirmation screen
int no_btn_hb[4] = {170, 250, 270, 300}; //Used in confirmation screen

String get_parking_frm_rcvr() {
  //will actually get
  delay(1000);
  return "-";
}

bool sendIRData(char* text, int tsize) {
  bool done = false;
  IR.stopListening();
  Serial.write(text);
  done = IR.write(&text, tsize);
  IR.startListening();
  return done;
}

String get_parking_no() {
  //grab from EEPROM if cannot connect
  String pkng = get_parking_frm_rcvr();
  if (pkng == "-") {
    //locations are: 20, 21, 22, 23, 24
    pkng = "";
    if (EEPROM.read(20) == 255) {
      return pkng;
    }
    pkng += (char)EEPROM.read(20);
    pkng += (char)EEPROM.read(21);
    pkng += (char)EEPROM.read(22);
    pkng += (char)EEPROM.read(23);
    pkng += (char)EEPROM.read(24);
    pkng.trim(); //remove any trailing whitespace
  }
  return pkng;
}

bool is_alarm_on() {
  char text[] = "GET ALM";
  IR.write(&text, sizeof(text));
  bool result = sendIRData(text, sizeof(text));
  if (!result) {
    renderFailed();
    delay(2000);
    return false;
  }
  int wait = 50;
  while (!IR.available() && wait > 0) {
    //Wait for a max of 5s.
    delay(100);
    wait--;
  }
  if (IR.available()) {
    char text[] = "";
    IR.read(&text, sizeof(text));
    if (text == "0") {
      return false;
    } else {
      return true;
    }
  } else {
    renderFailed();
    delay(2000);
    return false;
  }
}

void send_data_alm() {
  String text = (alarm_on) ? ("ALM Y") : ("ALM N");
  char text_ch[] = "";
  text.toCharArray(text_ch, text.length());
  bool result = sendIRData(text_ch, sizeof(text_ch));
  if (!result) {
    alarm_on = !alarm_on;
    renderFailed();
    delay(2000);
  }
}

void send_data_parking() {
  for (int i = 20; i < 25; i++) {
    if (i < 20 + parking_no.length()) {
      EEPROM.update(i, parking_no[i - 20]);
    } else {
      EEPROM.update(i, ' ');
    }
  }
  delay(3000);
}

void resetEEPROM() {
  //This value will be 255 if this is 1st time init, 0 if not.
  bool firstTime = EEPROM.read(0);
  if (firstTime) {
    EEPROM.update(0, 0);
    for (int i = 1; i < EEPROM.length(); i++) {
      EEPROM.update(i, 255);
    }
  }
}

TSConfigData getCalibrationData() {
  //calibration data is stored in the EEPROM.
  //Each is a 2 byte value (i.e. an int).
  //from location 1 to location 8
  int tscd[4] = {0, 0, 0, 0};
  for (int i = 1; i < 9; i++) {
    if (EEPROM.read(i) == 255 && i % 2 == 1) {
      //unset, trigger calibration
      return {0, 0, 0, 0};
    } else {
      tscd[(i - 1) / 2] += EEPROM.read(i);
      if (i % 2 != 0) {
        tscd[(i - 1) / 2] <<= 8;
      }
    }
  }
  TSConfigData tscd_actual;
  tscd_actual.xMin = tscd[0];
  tscd_actual.xMax = tscd[1];
  tscd_actual.yMin = tscd[2];
  tscd_actual.yMax = tscd[3];
  return tscd_actual;
}

void writeCalibrationData() {
  TSConfigData tscd = Waveshield.getTsConfigData();
  int tscd_int[4] = {tscd.xMin, tscd.xMax, tscd.yMin, tscd.yMax};
  for (int i = 8; i > 0; i--) {
    EEPROM.update(i, tscd_int[(i - 1) / 2] & 255);
    tscd_int[(i - 1) / 2] >>= 8;
  }
}

void getVariables() {
  //variables are stored in the EEPROM.
  //9 -> sound, 10 -> volume, 11 -> light, 12 -> lblink
  int sound_i = EEPROM.read(9);
  if (sound == 255) {
    EEPROM.write(9, 1);
    sound_i = 1;
  }
  volume = EEPROM.read(10);
  if (volume == 255) {
    EEPROM.write(10, 100);
    volume = 100;
  }
  int light_i = EEPROM.read(11);
  if (light_i == 255) {
    EEPROM.write(11, 1);
    light_i = 1;
  }
  int lblink_i = EEPROM.read(12);
  if (lblink_i == 255) {
    EEPROM.write(12, 1);
    lblink_i = 1;
  }
  sound = sound_i;
  light = light_i;
  lblink = lblink_i;
}

void setVariables() {
  EEPROM.update(9, sound);
  EEPROM.update(10, volume);
  EEPROM.update(11, light);
  EEPROM.update(12, lblink);
}

void drawOnOff(int x, int y, bool onOff) {
  //draw base slider
  //-1, as ideally this should use same y as text
  if (onOff) {
    Waveshield.fillRoundRect(x, y - 1, 35, 14, 7, GREEN);
    Waveshield.drawRoundRect(x, y - 1, 35, 14, 7, BLACK);
  } else {
    Waveshield.fillRoundRect(x, y - 1, 35, 14, 7, RED);
    Waveshield.drawRoundRect(x, y - 1, 35, 14, 7, BLACK);
  }
  //draw dot
  if (!onOff) {
    Waveshield.fillCircle(x + 7, y + 6, 11, WHITE);
    Waveshield.drawCircle(x + 7, y + 6, 11, BLACK);
  } else {
    Waveshield.fillCircle(x + 28, y + 6, 11, WHITE);
    Waveshield.drawCircle(x + 28, y + 6, 11, BLACK);
  }
}

void volSlider(int x, int y, int vol) {
  Waveshield.drawLine(x, y, x + SLEN, y, BLACK);
  Waveshield.fillCircle(x + (SLEN * vol) / 100, y, 9, WHITE);
  Waveshield.drawCircle(x + (SLEN * vol) / 100, y, 9, BLACK);
}

void drawBKSP(int x, int y) {
  //outline
  Waveshield.drawLine(x + 20, y + 35, x + 40, y + 15, BLACK);
  Waveshield.drawLine(x + 40, y + 15, x + 85, y + 15, BLACK);
  Waveshield.drawLine(x + 85, y + 15, x + 85, y + 55, BLACK);
  Waveshield.drawLine(x + 85, y + 55, x + 40, y + 55, BLACK);
  Waveshield.drawLine(x + 40, y + 55, x + 20, y + 35, BLACK);

  //X
  Waveshield.drawLine(x + 75, y + 25, x + 50, y + 45, BLACK);
  Waveshield.drawLine(x + 50, y + 25, x + 75, y + 45, BLACK);
}

void drawBKbutton(int x, int y) {
  //<
  Waveshield.drawLine(x + 15, y + 25, x + 35, y + 10, BLACK);
  Waveshield.drawLine(x + 15, y + 25, x + 35, y + 40, BLACK);

  //back
  Waveshield.setCursor(x + 50, y + 15);
  Waveshield.setTextSize(3);
  Waveshield.print("Back");
}

String correct_format(String no, int len) {
  //this function inserts spaces before a number
  String whitespace = "";
  for (int i = 0; i < len - no.length(); i++) {
    whitespace += ' ';
  }
  return whitespace + no;
}

void renderCalibration() {
  //Set screen
  screen = 2;
  
  //Set colour
  Waveshield.fillScreen(LBLUE);
  Waveshield.setTextColor(BLACK);

  //Print text -> first time calibration
  Waveshield.setCursor(30, 20);
  Waveshield.setTextSize(2);
  Waveshield.print("First Time Calibration");

  //add explanation
  Waveshield.setCursor(60, 50);
  Waveshield.setTextSize(1);
  Waveshield.print("Move stylus out of bounds over all");
  Waveshield.setCursor(60, 65);
  Waveshield.setTextSize(1);
  Waveshield.print("      4 edges to calibrate.");
  Waveshield.setCursor(7, 80);
  Waveshield.setTextSize(1);
  Waveshield.print("Press 'OK' when the red dot is close to the stylus.");
  Waveshield.setCursor(7, 95);
  Waveshield.setTextSize(1);
  Waveshield.print("    This screen will refresh every 2.5 seconds.");

  //add ok button
  Waveshield.fillRoundRect(ok_btn_hb[0], ok_btn_hb[1], ok_btn_hb[2] - ok_btn_hb[0], ok_btn_hb[3] - ok_btn_hb[1], 4, YELLOW);
  Waveshield.drawRoundRect(ok_btn_hb[0], ok_btn_hb[1], ok_btn_hb[2] - ok_btn_hb[0], ok_btn_hb[3] - ok_btn_hb[1], 4, BLACK);
  Waveshield.setCursor(ok_btn_hb[0] + 23, ok_btn_hb[1] + 10);
  Waveshield.setTextSize(3);
  Waveshield.print("OK");
}

void renderLoading() {
  //Set screen
  screen = 3;
  
  //Set colour
  Waveshield.fillScreen(LBLUE);
  Waveshield.setTextColor(BLACK);

  //Draw loading
  Waveshield.setTextSize(4);
  Waveshield.setCursor(50, 200);
  Waveshield.print("Working...");
}

void renderFailed() {
  //Set screen
  screen = 3;
  
  //Set colour
  Waveshield.fillScreen(LBLUE);
  Waveshield.setTextColor(BLACK);

  //Draw loading
  Waveshield.setTextSize(2);
  Waveshield.setCursor(45, 200);
  Waveshield.print("Failed transmission.");
}

void renderNumber() {
  //Cover current number
  Waveshield.fillRect(113, 160, 100, 27, YELLOW);

  //draw number
  Waveshield.setCursor(115, 165);
  Waveshield.setTextSize(3);
  Waveshield.print(correct_format(current_number, 5));
}

void renderKeypad() {
  //base render function for general keypad layout  
  //set colour
  Waveshield.fillScreen(LBLUE);
  Waveshield.setTextColor(BLACK);

  //draw prompt box
  Waveshield.setCursor(65, 90);
  Waveshield.setTextSize(3);
  Waveshield.print(" Enter New ");
  Waveshield.setCursor(65, 120);
  Waveshield.print("Parking Lot");

  //draw box for number on top
  Waveshield.fillRect(0, 150, 320, 50, YELLOW);
  Waveshield.drawRect(0, 150, 320, 50, BLACK);

  //Render number -> adds number & updates it
  renderNumber();

  //draw 5 rects below no.
  Waveshield.fillRect(115, 190, 16, 2, BLACK);
  Waveshield.fillRect(133, 190, 16, 2, BLACK);
  Waveshield.fillRect(151, 190, 16, 2, BLACK);
  Waveshield.fillRect(169, 190, 16, 2, BLACK);
  Waveshield.fillRect(187, 190, 16, 2, BLACK);

  //render the 0-9 keypad
  render09();
}

void render09() { 
  //set screen
  screen = 1; 
  
  Waveshield.fillRect(0, 200, 320, 280, YELLOW);
  //based on the hitboxes in keypad_hitbox.
  for (byte i = 0; i < KEYPAD_SIZE; i++) {
    if (keypad_hitbox[i][0] >= 0) {
      Waveshield.fillRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], YELLOW);
      Waveshield.drawRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], BLACK);
      Waveshield.setCursor(keypad_hitbox[i][1] + 42, keypad_hitbox[i][2] + 20);
      Waveshield.setTextSize(5);
      Waveshield.print(String(keypad_hitbox[i][0]));
    } else if (keypad_hitbox[i][0] == -1) {
      //backspace button
      Waveshield.fillRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], YELLOW);
      Waveshield.drawRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], BLACK);
      drawBKSP(keypad_hitbox[i][1], keypad_hitbox[i][2]);
    } else if (keypad_hitbox[i][0] == -2) {
      //back button
      drawBKbutton(keypad_hitbox[i][1], keypad_hitbox[i][2]);
    } else if (keypad_hitbox[i][0] == -3) {
      //enter text
      Waveshield.fillRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], YELLOW);
      Waveshield.drawRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], BLACK);
      Waveshield.setCursor(keypad_hitbox[i][1] + 10, keypad_hitbox[i][2] + 25);
      Waveshield.setTextSize(3);
      Waveshield.print("Enter");
    } else if (keypad_hitbox[i][0] == -4) {
      //delete button
      Waveshield.setCursor(keypad_hitbox[i][1] + 15, keypad_hitbox[i][2] + 15);
      Waveshield.setTextSize(3);
      Waveshield.print("Delete");
    } else if (keypad_hitbox[i][0] == -5) {
      //A - M button
      Waveshield.fillRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], YELLOW);
      Waveshield.drawRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], BLACK);
      Waveshield.setCursor(keypad_hitbox[i][1] + 15, keypad_hitbox[i][2] + 15);
      Waveshield.setTextSize(3);
      Waveshield.print("A-M");
    } else if (keypad_hitbox[i][0] == -6) {
      //N - Z button
      Waveshield.fillRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], YELLOW);
      Waveshield.drawRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], BLACK);
      Waveshield.setCursor(keypad_hitbox[i][1] + 15, keypad_hitbox[i][2] + 15);
      Waveshield.setTextSize(3);
      Waveshield.print("N-Z");
    }
  }
}

void renderAM() {
  //set screen
  screen = 5;
  
  //Render number -> adds number & updates it
  renderNumber();
  
  Waveshield.fillRect(0, 200, 320, 280, YELLOW);
  //based on the hitboxes in AM_hitbox.
  for (byte i = 0; i < AM_SIZE; i++) {
    if (AM_hitbox[i][0] == 'M') {
      Waveshield.fillRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], YELLOW);
      Waveshield.drawRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], BLACK);
      Waveshield.setCursor(AM_hitbox[i][1] + 42, AM_hitbox[i][2] + 20);
      Waveshield.setTextSize(5);
      Waveshield.print(String((char)AM_hitbox[i][0]));
    } else if (AM_hitbox[i][0] >= 0) {
      Waveshield.fillRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], YELLOW);
      Waveshield.drawRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], BLACK);
      Waveshield.setCursor(AM_hitbox[i][1] + 27, AM_hitbox[i][2] + 20);
      Waveshield.setTextSize(5);
      Waveshield.print(String((char)AM_hitbox[i][0]));
    } else if (AM_hitbox[i][0] == -1) {
      //backspace button
      Waveshield.fillRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], YELLOW);
      Waveshield.drawRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], BLACK);
      drawBKSP(AM_hitbox[i][1], AM_hitbox[i][2]);
    } else if (AM_hitbox[i][0] == -2) {
      //back button
      drawBKbutton(AM_hitbox[i][1], AM_hitbox[i][2]);
    } else if (AM_hitbox[i][0] == -3) {
      //enter text
      Waveshield.fillRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], YELLOW);
      Waveshield.drawRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], BLACK);
      Waveshield.setCursor(AM_hitbox[i][1] + 10, AM_hitbox[i][2] + 25);
      Waveshield.setTextSize(3);
      Waveshield.print("Enter");
    } else if (AM_hitbox[i][0] == -4) {
      //delete button
      Waveshield.setCursor(AM_hitbox[i][1] + 15, AM_hitbox[i][2] + 15);
      Waveshield.setTextSize(3);
      Waveshield.print("Delete");
    } else if (AM_hitbox[i][0] == -5) {
      //0 - 9 button
      Waveshield.fillRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], YELLOW);
      Waveshield.drawRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], BLACK);
      Waveshield.setCursor(AM_hitbox[i][1] + 15, AM_hitbox[i][2] + 15);
      Waveshield.setTextSize(3);
      Waveshield.print("0-9");
    } else if (AM_hitbox[i][0] == -6) {
      //N - Z button
      Waveshield.fillRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], YELLOW);
      Waveshield.drawRect(AM_hitbox[i][1], AM_hitbox[i][2], AM_hitbox[i][3] - AM_hitbox[i][1], AM_hitbox[i][4] - AM_hitbox[i][2], BLACK);
      Waveshield.setCursor(AM_hitbox[i][1] + 15, AM_hitbox[i][2] + 15);
      Waveshield.setTextSize(3);
      Waveshield.print("N-Z");
    }
  }
}

void renderNZ() {
  //set screen
  screen = 6;
  
  //Render number -> adds number & updates it
  renderNumber();
  
  Waveshield.fillRect(0, 200, 320, 280, YELLOW);
  //based on the hitboxes in NZ_hitbox.
  for (byte i = 0; i < NZ_SIZE; i++) {
    if (NZ_hitbox[i][0] == 'Z') {
      Waveshield.fillRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], YELLOW);
      Waveshield.drawRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], BLACK);
      Waveshield.setCursor(NZ_hitbox[i][1] + 42, NZ_hitbox[i][2] + 20);
      Waveshield.setTextSize(5);
      Waveshield.print(String((char)NZ_hitbox[i][0]));
    } else if (NZ_hitbox[i][0] >= 0) {
      Waveshield.fillRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], YELLOW);
      Waveshield.drawRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], BLACK);
      Waveshield.setCursor(NZ_hitbox[i][1] + 27, NZ_hitbox[i][2] + 20);
      Waveshield.setTextSize(5);
      Waveshield.print(String((char)NZ_hitbox[i][0]));
    } else if (NZ_hitbox[i][0] == -1) {
      //backspace button
      Waveshield.fillRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], YELLOW);
      Waveshield.drawRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], BLACK);
      drawBKSP(NZ_hitbox[i][1], NZ_hitbox[i][2]);
    } else if (NZ_hitbox[i][0] == -2) {
      //back button
      drawBKbutton(NZ_hitbox[i][1], NZ_hitbox[i][2]);
    } else if (NZ_hitbox[i][0] == -3) {
      //enter text
      Waveshield.fillRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], YELLOW);
      Waveshield.drawRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], BLACK);
      Waveshield.setCursor(NZ_hitbox[i][1] + 10, NZ_hitbox[i][2] + 25);
      Waveshield.setTextSize(3);
      Waveshield.print("Enter");
    } else if (NZ_hitbox[i][0] == -4) {
      //delete button
      Waveshield.setCursor(NZ_hitbox[i][1] + 15, NZ_hitbox[i][2] + 15);
      Waveshield.setTextSize(3);
      Waveshield.print("Delete");
    } else if (NZ_hitbox[i][0] == -5) {
      //0 - 9 button
      Waveshield.fillRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], YELLOW);
      Waveshield.drawRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], BLACK);
      Waveshield.setCursor(NZ_hitbox[i][1] + 15, NZ_hitbox[i][2] + 15);
      Waveshield.setTextSize(3);
      Waveshield.print("0-9");
    } else if (NZ_hitbox[i][0] == -6) {
      //A - M button
      Waveshield.fillRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], YELLOW);
      Waveshield.drawRect(NZ_hitbox[i][1], NZ_hitbox[i][2], NZ_hitbox[i][3] - NZ_hitbox[i][1], NZ_hitbox[i][4] - NZ_hitbox[i][2], BLACK);
      Waveshield.setCursor(NZ_hitbox[i][1] + 15, NZ_hitbox[i][2] + 15);
      Waveshield.setTextSize(3);
      Waveshield.print("A-M");
    }
  }
}

void renderSettings() {
  //Draw box around settings
  Waveshield.fillRoundRect(15, 185, 290, 270, 5, WHITE);
  Waveshield.drawRoundRect(15, 185, 290, 270, 5, BLACK);
   
  //Settings
  Waveshield.setCursor(87, 205);
  Waveshield.setTextSize(3);
  Waveshield.print("Settings");  

  //Settings -> Sound
  Waveshield.setCursor(100, 250);
  Waveshield.setTextSize(2);
  Waveshield.print("Sound");
  renderSound();

  //Settings -> Volume
  Waveshield.setCursor(100, 287);
  Waveshield.setTextSize(2);
  Waveshield.print("Volume");
  Waveshield.setCursor(185, 287);
  Waveshield.print(String(volume) + "%");
  renderVolume();

  //Settings -> Light
  Waveshield.setCursor(100, 345);
  Waveshield.setTextSize(2);
  Waveshield.print("Light");
  renderLight();

  //Settings -> Blink
  Waveshield.setCursor(100, 385);
  Waveshield.setTextSize(2);
  Waveshield.print("Blink");
  renderBlink();

  //Settings -> Reset
  Waveshield.fillRoundRect(reset_btn_hb[0], reset_btn_hb[1], reset_btn_hb[2] - reset_btn_hb[0], reset_btn_hb[3] - reset_btn_hb[1], 4, RED);
  Waveshield.drawRoundRect(reset_btn_hb[0], reset_btn_hb[1], reset_btn_hb[2] - reset_btn_hb[0], reset_btn_hb[3] - reset_btn_hb[1], 4, BLACK); 
  Waveshield.setCursor(reset_btn_hb[0] + 12, reset_btn_hb[1] + 5);
  Waveshield.setTextSize(2);
  Waveshield.setTextColor(WHITE);
  Waveshield.print("Reset");
  Waveshield.setTextColor(BLACK); //set text colour back to black
}

void renderSound() {
  //render over previous button
  Waveshield.fillRect(snd_btn_hb[0], snd_btn_hb[1], snd_btn_hb[2] - snd_btn_hb[0], snd_btn_hb[3] - snd_btn_hb[1], WHITE);

  //draw new
  drawOnOff(185, 250, sound);
}

void renderVolume() {
  //render over previous slider
  Waveshield.fillRect(vlm_btn_hb[0], vlm_btn_hb[1], vlm_btn_hb[2] - vlm_btn_hb[0], vlm_btn_hb[3] - vlm_btn_hb[1], WHITE);

  //draw text
  Waveshield.fillRect(185, 287, 70, 50, WHITE);
  Waveshield.setCursor(185, 287);
  Waveshield.print(String(volume) + "%");
  
  //draw new
  volSlider(105, 317, volume);
}

void renderLight() {
  //render over previous button
  Waveshield.fillRect(lgt_btn_hb[0], lgt_btn_hb[1], lgt_btn_hb[2] - lgt_btn_hb[0], lgt_btn_hb[3] - lgt_btn_hb[1], WHITE);

  //draw new
  drawOnOff(185, 345, light);
}

void renderBlink() {
  //render over previous button
  Waveshield.fillRect(blk_btn_hb[0], blk_btn_hb[1], blk_btn_hb[2] - blk_btn_hb[0], blk_btn_hb[3] - blk_btn_hb[1], WHITE);

  //draw new
  drawOnOff(185, 385, lblink);
}

void render() {
  //set screen
  screen = 0;
  
  //set colour
  Waveshield.fillScreen(LBLUE);
  Waveshield.setTextColor(BLACK);

  //Draw top text
  Waveshield.setTextSize(4);
  Waveshield.setCursor(30, 30);
  Waveshield.print("Car Tracker");

  //Draw car parking location
  Waveshield.setTextSize(2);
  if (parking_no != "") {
    //Get width & height
    int x1, y1; //this is always 0, 0
    unsigned int w, h;
    String printed = "Your car is at lot "+parking_no+".";
    Waveshield.getTextBounds(printed, 0, 0, &x1, &y1, &w, &h);

    //Set cursor to be centered
    Waveshield.setCursor(160 - w/2, 80);
    Waveshield.print("Your car is at lot "+parking_no+".");
  } else {
    Waveshield.setCursor(25, 80);
    Waveshield.print("You have not set a lot.");
  }
  
  //Draw button -> Enter/Change parking lot no.
  Waveshield.fillRoundRect(lot_no_hb[0], lot_no_hb[1], lot_no_hb[2] - lot_no_hb[0], lot_no_hb[3] - lot_no_hb[1], 4, YELLOW);
  Waveshield.drawRoundRect(lot_no_hb[0], lot_no_hb[1], lot_no_hb[2] - lot_no_hb[0], lot_no_hb[3] - lot_no_hb[1], 4, BLACK);
  Waveshield.setCursor(25, 122);
  Waveshield.setTextSize(2);
  Waveshield.print("Change lot");
  Waveshield.setCursor(25, 144);
  Waveshield.print("  number");

  //Draw button -> Sound alarm/on light
  if (alarm_on) {
    Waveshield.fillRoundRect(st_alm_hb[0], st_alm_hb[1], st_alm_hb[2] - st_alm_hb[0], st_alm_hb[3] - st_alm_hb[1], 4, RED);
    Waveshield.drawRoundRect(st_alm_hb[0], st_alm_hb[1], st_alm_hb[2] - st_alm_hb[0], st_alm_hb[3] - st_alm_hb[1], 4, BLACK);
    Waveshield.setCursor(175, 122);
    Waveshield.setTextSize(2);
    Waveshield.setTextColor(WHITE);
    Waveshield.print("Stop alarm");
    Waveshield.setCursor(180, 144);
    Waveshield.print("and light");
    Waveshield.setTextColor(BLACK);
  } else {
    Waveshield.fillRoundRect(st_alm_hb[0], st_alm_hb[1], st_alm_hb[2] - st_alm_hb[0], st_alm_hb[3] - st_alm_hb[1], 4, GREEN);
    Waveshield.drawRoundRect(st_alm_hb[0], st_alm_hb[1], st_alm_hb[2] - st_alm_hb[0], st_alm_hb[3] - st_alm_hb[1], 4, BLACK);
    Waveshield.setCursor(170, 122);
    Waveshield.setTextSize(2);
    Waveshield.print("Start alarm");
    Waveshield.setCursor(170, 144);
    Waveshield.print(" and light");
  }

  //render settings dialog
  renderSettings();
}

void renderConfirmation() {
  screen = 4;

  //we do not clear the screen, for effect
  //draw prompt box
  Waveshield.fillRoundRect(30, 170, 260, 140, 5, WHITE);
  Waveshield.drawRoundRect(30, 170, 260, 140, 5, BLACK);

  //draw text -> Are you sure?
  Waveshield.setCursor(47, 190);
  Waveshield.setTextSize(3);
  Waveshield.print("Are You Sure?");

  //warning abt recalibration
  Waveshield.setCursor(70, 230);
  Waveshield.setTextSize(1);
  Waveshield.print("Recalibration will be required.");

  //Yes button
  Waveshield.fillRoundRect(yes_btn_hb[0], yes_btn_hb[1], yes_btn_hb[2] - yes_btn_hb[0], yes_btn_hb[3] - yes_btn_hb[1], 4, RED);
  Waveshield.drawRoundRect(yes_btn_hb[0], yes_btn_hb[1], yes_btn_hb[2] - yes_btn_hb[0], yes_btn_hb[3] - yes_btn_hb[1], 4, BLACK);
  Waveshield.setCursor(yes_btn_hb[0] + 25, yes_btn_hb[1] + 15);
  Waveshield.setTextSize(3);
  Waveshield.setTextColor(WHITE);
  Waveshield.print("Yes");
  Waveshield.setTextColor(BLACK);

  //No button
  Waveshield.fillRoundRect(no_btn_hb[0], no_btn_hb[1], no_btn_hb[2] - no_btn_hb[0], no_btn_hb[3] - no_btn_hb[1], 4, GREEN);
  Waveshield.drawRoundRect(no_btn_hb[0], no_btn_hb[1], no_btn_hb[2] - no_btn_hb[0], no_btn_hb[3] - no_btn_hb[1], 4, BLACK);
  Waveshield.setCursor(no_btn_hb[0] + 33, no_btn_hb[1] + 15);
  Waveshield.setTextSize(3);
  Waveshield.print("No");
}

void doHitboxes(int scrn) {
  //get touchscreen point
  TSPoint p = Waveshield.getPoint();
  Waveshield.normalizeTsPoint(p);

  //check if press ok
  if (p.x >= 0 && p.y >= 0 && Waveshield.pressure() > 50) {
    if (scrn == 0) {
      //sound btn
      if (p.x > snd_btn_hb[0] && p.x < snd_btn_hb[2] && p.y > snd_btn_hb[1] && p.y < snd_btn_hb[3]) {
        sound = !sound;
        setVariables();
        renderSound();
        delay(200);
      //light btn
      } else if (p.x > lgt_btn_hb[0] && p.x < lgt_btn_hb[2] && p.y > lgt_btn_hb[1] && p.y < lgt_btn_hb[3]) {
        light = !light;
        setVariables();
        //Note: the render function creates a natural delay.
        renderLight();
        delay(200);
      //Blink button
      } else if (p.x > blk_btn_hb[0] && p.x < blk_btn_hb[2] && p.y > blk_btn_hb[1] && p.y < blk_btn_hb[3]) {
        lblink = !lblink;
        setVariables();
        //Note: the render function creates a natural delay.
        renderBlink();
        delay(200);
      //Volume button
      } else if (p.x > vlm_btn_hb[0] && p.x < vlm_btn_hb[2] && p.y > vlm_btn_hb[1] && p.y < vlm_btn_hb[3]) {
        int new_volume = max(0, min((p.x - vlm_btn_hb[4]) * 100 / SLEN, 100));
        if (new_volume != volume) {
          volume = new_volume;
          setVariables();
          renderVolume();
        }
      } else if (p.x > lot_no_hb[0] && p.x < lot_no_hb[2] && p.y > lot_no_hb[1] && p.y < lot_no_hb[3]) {
        current_number = "";
        renderKeypad();
      } else if (p.x > st_alm_hb[0] && p.x < st_alm_hb[2] && p.y > st_alm_hb[1] && p.y < st_alm_hb[3]) {
        //start/stop alarm
        renderLoading();
        send_data_alm();
        alarm_on = !alarm_on;
        render();
      } else if (p.x > reset_btn_hb[0] && p.x < reset_btn_hb[2] && p.y > reset_btn_hb[1] && p.y < reset_btn_hb[3]) {
        renderConfirmation();
      }
    } else if (scrn == 1) {
      //do via keypad_hitbox
      for (byte i = 0; i < KEYPAD_SIZE; i++) {
        if (p.x > keypad_hitbox[i][1] && p.x < keypad_hitbox[i][3] && p.y > keypad_hitbox[i][2] && p.y < keypad_hitbox[i][4]) {
          if (keypad_hitbox[i][0] >= 0) {
            //no. buttons
            current_number = current_number + String(keypad_hitbox[i][0]);

            //remove if length > 5
            if (current_number.length() > 5) {
              current_number = current_number.substring(1, current_number.length());
            }
            
            renderNumber();
            delay(200);
          } else if (keypad_hitbox[i][0] == -2) {
            //back button
            current_number = "";
            render();
          } else if (keypad_hitbox[i][0] == -1) {
            //bksp button
            current_number = current_number.substring(0, current_number.length() - 1);
            renderNumber();
            delay(200);
          } else if (keypad_hitbox[i][0] == -3) {
            //enter button
            parking_no = current_number;
            renderLoading();
            send_data_parking();
            current_number = "";
            render();
          } else if (keypad_hitbox[i][0] == -4) {
            //delete data button
            parking_no = "";
            renderLoading();
            send_data_parking();
            current_number = "";
            render();
          } else if (keypad_hitbox[i][0] == -5) {
            //A - M button
            renderAM();
          } else if (AM_hitbox[i][0] == -6) {
            //N - Z button
            renderNZ();
          }
        }
      }
    } else if (scrn == 2) {
      if (p.x > ok_btn_hb[0] && p.x < ok_btn_hb[2] && p.y > ok_btn_hb[1] && p.y < ok_btn_hb[3]) {
        //ok button on the calibration screen
        writeCalibrationData();
        render();
      } else {
        Waveshield.fillCircle(p.x, p.y, 2, RED);
      }
    } else if (scrn == 4) {
      if (p.x > yes_btn_hb[0] && p.x < yes_btn_hb[2] && p.y > yes_btn_hb[1] && p.y < yes_btn_hb[3]) {
        //yes button on confirmation screen
        EEPROM.update(0, 255);
        resetEEPROM();
        setup();
      } else if (p.x > no_btn_hb[0] && p.x < no_btn_hb[2] && p.y > no_btn_hb[1] && p.y < no_btn_hb[3]) {
        render();
      }
    } else if (scrn == 5) {
      //do via AM_hitbox
      for (byte i = 0; i < AM_SIZE; i++) {
        if (p.x > AM_hitbox[i][1] && p.x < AM_hitbox[i][3] && p.y > AM_hitbox[i][2] && p.y < AM_hitbox[i][4]) {
          if (AM_hitbox[i][0] >= 0) {
            //no. buttons
            current_number = current_number + String((char)AM_hitbox[i][0]);

            //remove if length > 5
            if (current_number.length() > 5) {
              current_number = current_number.substring(1, current_number.length());
            }
            
            renderNumber();
            delay(200);
          } else if (AM_hitbox[i][0] == -2) {
            //back button
            current_number = "";
            render();
          } else if (AM_hitbox[i][0] == -1) {
            //bksp button
            current_number = current_number.substring(0, current_number.length() - 1);
            renderNumber();
            delay(200);
          } else if (AM_hitbox[i][0] == -3) {
            //enter button
            parking_no = current_number;
            renderLoading();
            send_data_parking();
            current_number = "";
            render();
          } else if (AM_hitbox[i][0] == -4) {
            //delete data button
            parking_no = "";
            renderLoading();
            send_data_parking();
            current_number = "";
            render();
          } else if (AM_hitbox[i][0] == -5) {
            //0 - 9 button
            render09();
          } else if (AM_hitbox[i][0] == -6) {
            //N - Z button
            renderNZ();
          }
        }
      }
    } else if (scrn == 6) {
      //do via NZ_hitbox
      for (byte i = 0; i < NZ_SIZE; i++) {
        if (p.x > NZ_hitbox[i][1] && p.x < NZ_hitbox[i][3] && p.y > NZ_hitbox[i][2] && p.y < NZ_hitbox[i][4]) {
          if (NZ_hitbox[i][0] >= 0) {
            //no. buttons
            current_number = current_number + String((char)NZ_hitbox[i][0]);

            //remove if length > 5
            if (current_number.length() > 5) {
              current_number = current_number.substring(1, current_number.length());
            }
            
            renderNumber();
            delay(200);
          } else if (NZ_hitbox[i][0] == -2) {
            //back button
            current_number = "";
            render();
          } else if (NZ_hitbox[i][0] == -1) {
            //bksp button
            current_number = current_number.substring(0, current_number.length() - 1);
            renderNumber();
            delay(200);
          } else if (NZ_hitbox[i][0] == -3) {
            //enter button
            parking_no = current_number;
            renderLoading();
            send_data_parking();
            current_number = "";
            render();
          } else if (NZ_hitbox[i][0] == -4) {
            //delete data button
            parking_no = "";
            renderLoading();
            send_data_parking();
            current_number = "";
            render();
          } else if (NZ_hitbox[i][0] == -5) {
            //0 - 9 button
            render09();
          } else if (NZ_hitbox[i][0] == -6) {
            //A - M button
            renderAM();
          }
        }
      }
    }
  }
}

void setup() {
  //Initialise
  SPI.begin();
  Waveshield.begin();
  Serial.begin(9600);
  IR.begin();
  IR.openWritingPipe(IR_ADDR_W);
  IR.openReadingPipe(1, IR_ADDR_R);
  IR.startListening();

  //Set portrait
  Waveshield.setRotation(0);

  //Set brightness
  Waveshield.setScreenBrightness(255);

  //turn off idle mode
  Waveshield.setIdleMode(false);

  //render loading scrn
  renderLoading();

  //grab the variables
  parking_no = get_parking_no();
  alarm_on = is_alarm_on();

  //reset EEPROM if required
  resetEEPROM();

  //get other variables on EEPROM
  getVariables();

  //Load TS config, if it exists
  TSConfigData tscd = getCalibrationData();
  if (tscd.xMin == 0 && tscd.xMax == 0) {
    renderCalibration();
    return;
  }
  Waveshield.setTsConfigData(tscd);

  render();
}

void loop() {
  doHitboxes(screen);
  if (screen == 2 && millis() % 2500 == 0) {
    renderCalibration();
  }
}
