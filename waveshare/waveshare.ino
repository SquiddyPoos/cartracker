#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Waveshare_ILI9486.h>


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

#define SLEN    105

Waveshare_ILI9486 Waveshield;

int get_parking_no() {
  return -1;
}

bool is_alarm_on() {
  return false;
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
  if (onOff) {
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

bool sound = true;
int volume = 100;
bool light = true;
bool lblink = true;

//number - x1 - y1 - x2 - y2
//x -> 0 - 320
//y -> 200 - 480
//-1 -> bcsp, -2 is a back button, -3 is filler.
int keypad_hitbox[13][5] = {
  {1, 0, 200, 106, 270},
  {2, 106, 200, 213, 270},
  {3, 213, 200, 320, 270},
  {4, 0, 270, 106, 340},
  {5, 106, 270, 213, 340},
  {6, 213, 270, 320, 340},
  {7, 0, 340, 106, 410},
  {8, 106, 340, 213, 410},
  {9, 213, 340, 320, 410},
  {0, 106, 410, 213, 480},
  {-1, 213, 410, 320, 480},
  {-2, 0, 0, 140, 50},
  {-3, 0, 410, 106, 480}
};

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

void renderKeypad() {
  //set colour
  Waveshield.fillScreen(LBLUE);
  Waveshield.setTextColor(BLACK);

  Waveshield.fillRect(0, 200, 320, 280, YELLOW);
  //based on the hitboxes in keypad_hitbox.
  for (int i = 0; i < 13; i++) {
    if (keypad_hitbox[i][0] >= 0) {
      Waveshield.fillRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], YELLOW);
      Waveshield.drawRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], BLACK);
      Waveshield.setCursor(keypad_hitbox[i][1] + 42, keypad_hitbox[i][2] + 20);
      Waveshield.setTextSize(5);
      Waveshield.print(String(keypad_hitbox[i][0]));
    } else if (keypad_hitbox[i][0] == -1) {
      Waveshield.fillRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], YELLOW);
      Waveshield.drawRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], BLACK);
      drawBKSP(keypad_hitbox[i][1], keypad_hitbox[i][2]);
    } else if (keypad_hitbox[i][0] == -2) {
      drawBKbutton(keypad_hitbox[i][1], keypad_hitbox[i][2]);
    } else {
      Waveshield.fillRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], YELLOW);
      Waveshield.drawRect(keypad_hitbox[i][1], keypad_hitbox[i][2], keypad_hitbox[i][3] - keypad_hitbox[i][1], keypad_hitbox[i][4] - keypad_hitbox[i][2], BLACK);
    }
  }
}

void render() {
  //set colour
  Waveshield.fillScreen(LBLUE);
  Waveshield.setTextColor(BLACK);

  //Draw top text
  Waveshield.setRotation(0);
  Waveshield.setTextSize(4);
  Waveshield.setCursor(30, 30);
  Waveshield.print("Car Tracker");

  //Draw car parking location
  Waveshield.setTextSize(2);
  String parking_no = String(get_parking_no());
  if (parking_no != "-1") {
    Waveshield.setCursor(27, 80);
    Waveshield.print("Your car is at lot "+parking_no+".");
  } else {
    Waveshield.setCursor(20, 80);
    Waveshield.print("You have not set a lot.");
  }
  
  //Draw button -> Enter/Change parking lot no.
  Waveshield.fillRoundRect(15, 115, 140, 50, 4, YELLOW);
  Waveshield.drawRoundRect(15, 115, 140, 50, 4, BLACK);
  Waveshield.setCursor(25, 122);
  Waveshield.setTextSize(2);
  Waveshield.print("Change lot");
  Waveshield.setCursor(25, 144);
  Waveshield.print("  number");

  //Draw button -> Sound alarm/on light
  Waveshield.fillRoundRect(165, 115, 140, 50, 4, YELLOW);
  Waveshield.drawRoundRect(165, 115, 140, 50, 4, BLACK);
  if (is_alarm_on()) {
    Waveshield.setCursor(175, 122);
    Waveshield.setTextSize(2);
    Waveshield.print("Stop alarm");
    Waveshield.setCursor(180, 144);
    Waveshield.print("and light");
  } else {
    Waveshield.setCursor(170, 122);
    Waveshield.setTextSize(2);
    Waveshield.print("Start alarm");
    Waveshield.setCursor(170, 144);
    Waveshield.print(" and light");
  }

  //Draw box around settings
  Waveshield.fillRoundRect(15, 185, 290, 270, 5, WHITE);
  Waveshield.drawRoundRect(15, 185, 290, 270, 5, BLACK);
   
  //Settings
  Waveshield.setCursor(87, 200);
  Waveshield.setTextSize(3);
  Waveshield.print("Settings");

  //Disclaimer (will not save)
  Waveshield.setCursor(28, 235);
  Waveshield.setTextSize(1);
  Waveshield.print("(Note: These will not be saved over reboots.)");

  //Settings -> Sound
  Waveshield.setCursor(100, 275);
  Waveshield.setTextSize(2);
  Waveshield.print("Sound");
  drawOnOff(185, 275, sound);

  //Settings -> Volume
  Waveshield.setCursor(125, 312);
  Waveshield.setTextSize(2);
  Waveshield.print("Volume");
  volSlider(105, 342, volume);

  //Settings -> Light
  Waveshield.setCursor(100, 375);
  Waveshield.setTextSize(2);
  Waveshield.print("Light");
  drawOnOff(185, 375, light);

  //Settings -> Blink
  Waveshield.setCursor(100, 415);
  Waveshield.setTextSize(2);
  Waveshield.print("Blink");
  drawOnOff(185, 415, lblink);
}

void setup() {
  // put your setup code here, to run once:
  SPI.begin();
  Waveshield.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  render();
  delay(5000);
  renderKeypad();
  delay(5000);
}
