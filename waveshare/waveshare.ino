#include <SPI.h>

#include <Adafruit_GFX.h>
#include <Waveshare_ILI9486.h>


// Assign human-readable names to some common 16-bit color values:
#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

int get_parking_no() {
  return 10;
}

void setup() {
  // put your setup code here, to run once:
  Waveshare_ILI9486 Waveshield;
  SPI.begin();
  Waveshield.begin();

  //Draw top text
  Waveshield.setRotation(0);
  Waveshield.setTextSize(2);
  Waveshield.setCursor(100, 10);
  Waveshield.print("Car Tracker");

  //Draw car parking location
  Wave
  
  //Draw the button -> Enter parking lot no.
}

void loop() {
  // put your main code here, to run repeatedly:

}
