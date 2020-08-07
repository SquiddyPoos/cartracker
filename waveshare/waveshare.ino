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
  return 10;
}

bool is_alarm_on() {
  return true;
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
int volume = 50;
bool light = true;
bool lblink = true;

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
  Waveshield.fillRoundRect(15, 110, 140, 50, 4, YELLOW);
  Waveshield.drawRoundRect(15, 110, 140, 50, 4, BLACK);
  Waveshield.setCursor(25, 120);
  Waveshield.setTextSize(2);
  Waveshield.print("Change lot");
  Waveshield.setCursor(25, 137);
  Waveshield.print("  number");

  //Draw button -> Sound alarm/on light
  Waveshield.fillRoundRect(165, 110, 140, 50, 4, YELLOW);
  Waveshield.drawRoundRect(165, 110, 140, 50, 4, BLACK);
  if (is_alarm_on()) {
    Waveshield.setCursor(175, 120);
    Waveshield.setTextSize(2);
    Waveshield.print("Stop alarm");
    Waveshield.setCursor(180, 137);
    Waveshield.print("and light");
  } else {
    Waveshield.setCursor(175, 120);
    Waveshield.setTextSize(2);
    Waveshield.print("Start alarm");
    Waveshield.setCursor(170, 137);
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
  render();
}

void loop() {
  // put your main code here, to run repeatedly:

}
