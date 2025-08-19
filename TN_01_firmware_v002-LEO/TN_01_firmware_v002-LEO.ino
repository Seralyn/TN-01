#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Roboto_Condensed_14.h"
#include <AS5600.h>

// -------- Bitmap (train icon) --------
const unsigned char epd_bitmap_train_icon_old_locomotive_silhouette_symbol_sign_illustration_vector [] PROGMEM = {
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x87,0xff,0xff,0xff,0x03,0xff,0xff,
  0xff,0x87,0xe0,0x0f,0xff,0x87,0xe0,0x0f,0xff,0x87,0xef,0x6f,0xff,0x86,0x6f,0x6f,
  0xff,0x86,0x6f,0x6f,0xfe,0x00,0x0f,0x6f,0xfc,0x00,0x0f,0x0f,0xfc,0x00,0x0f,0x0f,
  0xfc,0x00,0x00,0x0f,0xfc,0x00,0x00,0x0f,0xfe,0x00,0x00,0x0f,0xfc,0x00,0x00,0x0f,
  0xfc,0x00,0x00,0x07,0xf8,0x00,0x00,0x07,0xf8,0x00,0x00,0x07,0xf0,0x00,0x00,0x0f,
  0xf1,0x04,0x10,0x3f,0xff,0x0e,0x38,0x3f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

// -------- OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -------- Font & default text --------
extern const GFXfont Roboto_Condensed_14;
const char* DEFAULT_LINE1 = "Train Sim World";
const char* DEFAULT_LINE2 = "5 detected.";

// -------- AS5600 --------
AS5600 encoder;
int lastHEValScaled = -1;
unsigned long lastHEPrintTime = 0;

// -------- Pins --------
int greenLED = 10;
int redLED   = 11;
const int bIPB_led   = 4;
const int bIPB       = 5;
const int illumRocker= 6;
const int mtlThrw    = 7;
const int ignition   = 8;
const int pbMomSw    = 9;

// Analogs
const int linPot = A0;
const int rotPot = A1;

// Print throttling
int lastLinPotValScaled = -1;
unsigned long lastLinPotPrintTime = 0;
int lastRotPotValScaled = -1;
unsigned long lastRotPotPrintTime = 0;

// -------- Marquee / app title --------
String currentApp = "";
String inbuf = "";

String marqueeText = "";
int16_t textX = 0;
int16_t textW = 0;
uint32_t lastScrollMs = 0;
const uint16_t SCROLL_INTERVAL_MS = 35;
const int16_t GAP_PX = 24;

// Icon layout
const int16_t ICON_W = 32;
const int16_t ICON_X = SCREEN_WIDTH - ICON_W;
const int16_t ICON_Y = 0;
const int16_t TEXT_Y = 10;

// ---- helpers ----
static inline bool isUpper(char c){ return c >= 'A' && c <= 'Z'; }
static inline bool isLower(char c){ return c >= 'a' && c <= 'z'; }
static inline bool isDigitC(char c){ return c >= '0' && c <= '9'; }

String expandKnownGames(const String &sIn) {
  String s = sIn;
  String low = s; low.toLowerCase();
  if (low == "tsw 4" || low == "tsw4") return "Train Sim World 4";
  if (low == "tsw 5" || low == "tsw5") return "Train Sim World 5";
  if (low == "tsc" || low == "train simulator classic") return "Train Simulator Classic";
  if (low == "open rails" || low == "openrails") return "Open Rails";
  if (low == "simrail") return "SimRail";
  if (low == "railworks") return "Train Simulator Classic";
  return s;
}

bool isTrainTitle(const String& title) {
  String low = title; low.toLowerCase();
  if (low.indexOf("train") >= 0) return true;                  // catches “Train …”
  // explicit list as backup
  return (low.indexOf("simrail")>=0 || low.indexOf("open rails")>=0 ||
          low.indexOf("railworks")>=0 || low.indexOf("tsw")>=0 ||
          low.indexOf("train simulator classic")>=0);
}

// Prettify EXE name -> "X detected."
String prettifyExeName(String raw) {
  raw.trim();
  int dot = raw.lastIndexOf('.');
  if (dot >= 0) raw = raw.substring(0, dot);
  for (int i = 0; i < (int)raw.length(); i++) {
    char c = raw[i];
    if (c == '_' || c == '-' || c == '.') raw.setCharAt(i, ' ');
  }
  String out = "";
  int n = raw.length();
  for (int i = 0; i < n; i++) {
    char c = raw[i];
    if (c == ' ') { if (out.length() && out[out.length()-1] != ' ') out += ' '; continue; }
    if (out.length() == 0) { out += isLower(c) ? (char)toupper(c) : c; continue; }
    char prevIn  = (i > 0) ? raw[i-1] : 0;
    char nextIn  = (i+1 < n) ? raw[i+1] : 0;
    bool cU = isUpper(c), cL = isLower(c), cD = isDigitC(c);
    bool pU = isUpper(prevIn), pL = isLower(prevIn), pD = isDigitC(prevIn);
    bool nL = isLower(nextIn);
    bool camelStart    = (cU && pL);
    bool acronymToWord = (cU && pU && nL);
    bool digitBoundary = (cD && !pD) || (!cD && pD);
    bool upToUp        = (cU && pU);
    if (!upToUp && (camelStart || acronymToWord || digitBoundary)) {
      if (out.length() && out[out.length()-1] != ' ') out += ' ';
    }
    out += c;
  }
  out.trim();
  if (out.length() >= 2) {
    int last = out.length() - 1;
    if (isDigitC(out[last]) && isUpper(out[last - 1]) && out[last - 1] != ' ')
      out = out.substring(0, last) + " " + out.substring(last);
  }
  out = expandKnownGames(out);
  out.trim();
  if (out.length()) out += " detected.";
  return out;
}

// -------- Display --------
void renderDisplay(bool showIcon) {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setFont(&Roboto_Condensed_14);

  // text area depends on icon
  int16_t usableW = showIcon ? (SCREEN_WIDTH - ICON_W) : SCREEN_WIDTH;

  if (marqueeText.length()) {
    // measure text width
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(marqueeText, 0, 0, &x1, &y1, &w, &h);
    textW = (int16_t)w;

    if (textW <= usableW) {
      int16_t x = (usableW - textW) / 2;
      if (x < 0) x = 0;
      int16_t y = (SCREEN_HEIGHT - (int16_t)h) / 2 + 8;
      display.setCursor(x, y);
      display.print(marqueeText);
    } else {
      int16_t y = TEXT_Y;
      int16_t loopW = textW + GAP_PX;
      int16_t xA = textX;
      int16_t xB = textX + loopW;
      display.setCursor(xA, y); display.print(marqueeText);
      display.setCursor(xB, y); display.print(marqueeText);
      uint32_t now = millis();
      if (now - lastScrollMs >= SCROLL_INTERVAL_MS) {
        lastScrollMs = now;
        textX -= 1;
        if (textX <= -loopW) textX = 0;
      }
    }
  } else {
    display.setCursor(0, 10); display.println(DEFAULT_LINE1);
    display.setCursor(0, 20); display.println(DEFAULT_LINE2);
    showIcon = true; // default text is a train game
  }

  // draw icon LAST when needed (masks any text that would intrude)
  if (showIcon) {
    display.drawBitmap(ICON_X, ICON_Y,
      epd_bitmap_train_icon_old_locomotive_silhouette_symbol_sign_illustration_vector,
      ICON_W, 32, SSD1306_BLACK, SSD1306_WHITE);
  }
  display.display();
}

void applyNewText(const String& pretty) {
  marqueeText = pretty;
  textX = 0;
  lastScrollMs = millis();
  renderDisplay(isTrainTitle(currentApp));
}

void drawAppImmediate(const String& s) { applyNewText(s); }

// -------- Setup --------
void setup() {
  pinMode(pbMomSw, INPUT_PULLUP);
  pinMode(ignition, INPUT_PULLUP);
  pinMode(mtlThrw, INPUT_PULLUP);
  pinMode(bIPB, INPUT_PULLUP);
  pinMode(illumRocker, INPUT);

  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(bIPB_led, OUTPUT);

  Serial.begin(115200);
  while (!Serial) {}

  Wire.begin();

  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, HIGH);
  digitalWrite(bIPB_led, LOW);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for(;;);
  }
  display.clearDisplay();
  display.setFont(&Roboto_Condensed_14);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.print("TN-01 ready");
  display.display();

  if (!encoder.begin()) {
    Serial.println("AS5600 not detected. Check wiring.");
    while (1);
  }
  encoder.setDirection(AS5600_CLOCK_WISE);

  marqueeText = ""; // default screen shows (train) lines
  renderDisplay(true);
}

// -------- Loop --------
void loop() {
  // --- Serial line-based: APP:<name>\n ---
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      if (inbuf.startsWith("APP:")) {
        String app = inbuf.substring(4);
        app.trim();
        if (app.length() > 0) {
          String pretty = prettifyExeName(app);
          if (pretty != currentApp) {
            currentApp = pretty;
            drawAppImmediate(currentApp);
          }
        }
      }
      inbuf = "";
    } else if (c != '\r') {
      inbuf += c;
      if (inbuf.length() > 96) inbuf.remove(0);
    }
  }

  // ---- Inputs / sensors ----
  int msButtonState   = digitalRead(pbMomSw);
  int ignitionState   = digitalRead(ignition);
  int mtlThrwState    = digitalRead(mtlThrw);
  int bIPBState       = digitalRead(bIPB);
  int illumRockerState= digitalRead(illumRocker);
  int linPotVal       = analogRead(linPot);
  int rotPotVal       = analogRead(rotPot);

  int linPotValScaled = map(linPotVal, 0, 1023, 0, 100);
  int rotPotValScaled = map(rotPotVal, 159, 597, 0, 100);

  int rawHEVal = encoder.readAngle();
  rawHEVal = constrain(rawHEVal, 1404, 2280);
  int heValScaled = map(rawHEVal, 1404, 2280, 0, 100);

  unsigned long now = millis();

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
  } else if (ignitionState == LOW) {
    Serial.println("ignition is ON");
  } else if (illumRockerState == HIGH) {
    Serial.println("rocker activated");
  }

  if (bIPBState == LOW) {
    digitalWrite(bIPB_led, HIGH);
    Serial.println("Big Switch Pressed!");
  } else {
    digitalWrite(bIPB_led, LOW);
  }

  if (mtlThrwState == LOW) {
    Serial.println("Metal Switch ON");
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
  } else {
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);
  }

  // keep display updated (icon only when train app)
  renderDisplay(isTrainTitle(currentApp));

  delay(10);
}
