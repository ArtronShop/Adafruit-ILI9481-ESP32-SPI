#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9481_ESP32_SPI.h"

extern const uint16_t image_data_logo[];

Adafruit_ILI9481_ESP32_SPI tft;

void setup() {
  Serial.begin(115200);
  Serial.println("ILI9481 Test!"); 
 
  tft.begin();

  Serial.println("Draw Bitmap"); 
  tft.drawRGBBitmap(0, 0, image_data_logo, 320, 480);
}


void loop(void) {

}
