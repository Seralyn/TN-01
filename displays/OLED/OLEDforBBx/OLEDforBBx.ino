#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Roboto_Condensed_14.h"

extern const GFXfont Roboto_Condensed_12;
String gameName = "No game";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1  // No reset pin used on your display

// Provide the I2C address directly, typically 0x3C for most displays
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Use 0x3C or 0x3D based on your OLED
    Serial.println("SSD1306 allocation failed");
    for(;;);  // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();  // Initializes the display.
}

void loop() {
  display.setFont(&Roboto_Condensed_14);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0,10);
  // display.print(gameName + "\ndetected.");
  display.print("Train Sim World 5 \ndetected.");

  display.display();
  delay(2000);
}
