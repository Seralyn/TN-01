#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Roboto_Condensed_14.h"
#include <AS5600.h>

// 'train-icon-old-locomotive-silhouette-symbol-sign-illustration-vector', 32x32px
const unsigned char epd_bitmap_train_icon_old_locomotive_silhouette_symbol_sign_illustration_vector [] PROGMEM = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x87, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 
	0xff, 0x87, 0xe0, 0x0f, 0xff, 0x87, 0xe0, 0x0f, 0xff, 0x87, 0xef, 0x6f, 0xff, 0x86, 0x6f, 0x6f, 
	0xff, 0x86, 0x6f, 0x6f, 0xfe, 0x00, 0x0f, 0x6f, 0xfc, 0x00, 0x0f, 0x0f, 0xfc, 0x00, 0x0f, 0x0f, 
	0xfc, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x00, 0x0f, 0xfe, 0x00, 0x00, 0x0f, 0xfc, 0x00, 0x00, 0x0f, 
	0xfc, 0x00, 0x00, 0x07, 0xf8, 0x00, 0x00, 0x07, 0xf8, 0x00, 0x00, 0x07, 0xf0, 0x00, 0x00, 0x0f, 
	0xf1, 0x04, 0x10, 0x3f, 0xff, 0x0e, 0x38, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 144)
const int epd_bitmap_allArray_LEN = 1;
const unsigned char* epd_bitmap_allArray[1] = {
	epd_bitmap_train_icon_old_locomotive_silhouette_symbol_sign_illustration_vector
};

// declaring font and empty variable bucket for game names
extern const GFXfont Roboto_Condensed_14;
String gameName = "No game";

//setting constraints for OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET    -1  // No reset pin used on your display

// Provide the I2C address directly, typically 0x3C for most displays
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// AS5600 setup
AS5600 encoder;
int lastHEValScaled = -1;
unsigned long lastHEPrintTime = 0;

// digital pin definitions
int greenLED = 10;
int redLED = 11;
const int bIPB_led = 4;
const int bIPB = 5;
const int illumRocker = 6;
const int mtlThrw = 7;
const int ignition = 8;
const int pbMomSw = 9;

// analog pin definitions
const int linPot = A0;
const int rotPot = A1;

// variables to track last pot values and print times
int lastLinPotValScaled = -1;
unsigned long lastLinPotPrintTime = 0;

int lastRotPotValScaled = -1;
unsigned long lastRotPotPrintTime = 0;

void setup() {
  pinMode(pbMomSw, INPUT_PULLUP);
  pinMode(ignition, INPUT_PULLUP);
  pinMode(mtlThrw, INPUT_PULLUP);
  pinMode(bIPB, INPUT_PULLUP);
  pinMode(illumRocker, INPUT);

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(bIPB_led, OUTPUT);
  
  Serial.begin(9600);
  while (!Serial);
  Wire.begin();


  digitalWrite(greenLED, LOW);  // Ensure green LED is off at boot
  digitalWrite(redLED, HIGH);   // Ensure red LED is on at boot
  digitalWrite(bIPB_led, LOW);  // Ensure big button LED is off

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) {  // Use 0x3C or 0x3D based on your OLED
    Serial.println("SSD1306 allocation failed");
    for(;;);  // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();  // Initializes the display.
  
  if (!encoder.begin()) {
    Serial.println("AS5600 not detected. Check wiring.");
    while (1);
  } else {
    Serial.println("AS5600 detected!");
  }

  encoder.setDirection(AS5600_CLOCK_WISE); // Or COUNTERCLOCK_WISE if needed
  
}

void loop() {
  int msButtonState = digitalRead(pbMomSw);
  int ignitionState = digitalRead(ignition);
  int mtlThrwState = digitalRead(mtlThrw);
  int bIPBState = digitalRead(bIPB);
  int illumRockerState = digitalRead(illumRocker);
  int linPotVal = analogRead(linPot);
  int rotPotVal = analogRead(rotPot);


  int linPotValScaled = map(linPotVal, 0, 1023, 0, 100);
  int rotPotValScaled = map(rotPotVal, 159, 597, 0, 100);

  // Read and scale AS5600 angle
  int rawHEVal = encoder.readAngle();
  rawHEVal = constrain(rawHEVal, 1404, 2280); // safety net
  int heValScaled = map(rawHEVal, 1404, 2280, 0, 100);
  

  unsigned long now = millis();

  // display.setFont(&Roboto_Condensed_14);
  display.setFont();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0,10);
  // display.print(gameName + "\ndetected.");
  display.println("Train Sim World");*
  display.setCursor(0, 20);
  display.println("5 detected.");

// Draw the new train icon
  display.drawBitmap(96, 0, epd_bitmap_train_icon_old_locomotive_silhouette_symbol_sign_illustration_vector, 32, 32, SSD1306_BLACK, SSD1306_WHITE);
  display.display();
  // delay(800);

  if (linPotValScaled != lastLinPotValScaled && (now - lastLinPotPrintTime >= 500)) {
    Serial.print("Linear Pot Value: ");
    Serial.println(linPotValScaled);
    lastLinPotValScaled = linPotValScaled;
    lastLinPotPrintTime = now;
  }

  if (rotPotValScaled != lastRotPotValScaled && (now - lastRotPotPrintTime >= 500)) {
    Serial.print("Adjusted Rotary Pot Value: ");
    Serial.println(rotPotValScaled);
    lastRotPotValScaled = rotPotValScaled;
    lastRotPotPrintTime = now;
  }

  
  if (heValScaled != lastHEValScaled && (now - lastHEPrintTime >= 500)) {
    Serial.print("Hall Sensor (AS5600) Value: ");
    Serial.println(heValScaled);
    lastHEValScaled = heValScaled;
    lastHEPrintTime = now;
  }
  

  if (msButtonState == LOW) {
    Serial.println("Momentary Switch (small) pressed!");
  } 
  else if (ignitionState == LOW) {
    Serial.println("ignition is ON");
    delay(800);
  } 

  else if (illumRockerState == HIGH) {
    Serial.println("rocker activated");
    delay(800);
  } 
  else if (bIPBState == LOW) {
    digitalWrite(bIPB_led, HIGH);
    Serial.println("Big Switch Pressed!");
  } 
  else {
    digitalWrite(bIPB_led, LOW);
  }

  // Serial.print("mtlThrwState: ");
  // Serial.println(mtlThrwState);

  // Serial.print("Setting redLED to: ");
  // Serial.println(mtlThrwState == LOW ? "LOW (OFF)" : "HIGH (ON)");

  // Serial.print("Setting greenLED to: ");
  // Serial.println(mtlThrwState == LOW ? "HIGH (ON)" : "LOW (OFF)");

  if (mtlThrwState == LOW) {
  Serial.println("Metal Switch ON");
  digitalWrite(redLED, LOW);
  digitalWrite(greenLED, HIGH);
} else {
  digitalWrite(redLED, HIGH);
  digitalWrite(greenLED, LOW);
}

  delay(100);
}