#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Roboto_Condensed_14.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String currentApp = "";
String inbuf = "";

// ---- marquee state ----
String marqueeText = "";
int16_t textX = 0;                // current scroll offset
int16_t textW = 0;                // pixel width of text
uint32_t lastScrollMs = 0;
const uint16_t SCROLL_INTERVAL_MS = 35; // スクロール速度（小さいほど速い）
const int16_t GAP_PX = 24;        // ループ時の余白

// ---- helpers ----
static inline bool isUpper(char c){ return c >= 'A' && c <= 'Z'; }
static inline bool isLower(char c){ return c >= 'a' && c <= 'z'; }
static inline bool isDigitC(char c){ return c >= '0' && c <= '9'; }

// 既知のゲーム略称を正式名称に展開（必要に応じて追加）
String expandKnownGames(const String &sIn) {
  String s = sIn;

  // 大文字小文字を気にせず比較したいので、比較用のlowerを作る
  String low = s; low.toLowerCase();

  // TSW 4 / TSW4 → Train Sim World 4
  if (low == "tsw 4" || low == "tsw4") return "Train Sim World 4";
  // TSW 5 / TSW5
  if (low == "tsw 5" || low == "tsw5") return "Train Sim World 5";
  // TSC / Train Simulator Classic
  if (low == "tsc" || low == "train simulator classic") return "Train Simulator Classic";
  // Open Rails variations
  if (low == "open rails" || low == "openrails") return "Open Rails";
  // SimRail
  if (low == "simrail") return "SimRail";

  // RailWorks（TS Classicの実行ファイル由来名など）
  if (low == "railworks") return "Train Simulator Classic";

  // 既知以外はそのまま
  return s;
}

// 文字列整形：拡張子除去、区切り統一、camelCase分割、ALL-CAPS維持、末尾数字分離、" detected."付与
String prettifyExeName(String raw) {
  raw.trim();

  // remove extension (e.g., ".exe")
  int dot = raw.lastIndexOf('.');
  if (dot >= 0) raw = raw.substring(0, dot);

  // replace common separators to space
  for (int i = 0; i < (int)raw.length(); i++) {
    char c = raw[i];
    if (c == '_' || c == '-' || c == '.') raw.setCharAt(i, ' ');
  }

  String out = "";
  int n = raw.length();
  for (int i = 0; i < n; i++) {
    char c = raw[i];
    if (c == ' ') {
      if (out.length() && out[out.length()-1] != ' ') out += ' ';
      continue;
    }

    if (out.length() == 0) {
      // 先頭だけ大文字化（元がALL-CAPSならそのまま維持される）
      out += isLower(c) ? (char)toupper(c) : c;
      continue;
    }

    char prevIn  = (i > 0) ? raw[i-1] : 0;
    char nextIn  = (i+1 < n) ? raw[i+1] : 0;

    bool cU = isUpper(c), cL = isLower(c), cD = isDigitC(c);
    bool pU = isUpper(prevIn), pL = isLower(prevIn), pD = isDigitC(prevIn);
    bool nL = isLower(nextIn);

    bool camelStart     = (cU && pL);                     // aA
    bool acronymToWord  = (cU && pU && nL);               // AAa のAで区切る
    bool digitBoundary  = (cD && !pD) || (!cD && pD);     // 文字↔数字
    bool upToUp         = (cU && pU);                     // ALL-CAPSは連結維持

    if (!upToUp && (camelStart || acronymToWord || digitBoundary)) {
      if (out.length() && out[out.length()-1] != ' ') out += ' ';
    }

    out += c;
  }

  out.trim();

  // 末尾が「大文字＋数字」なら数字を分離（TSW4 → TSW 4）
  if (out.length() >= 2) {
    int last = out.length() - 1;
    if (isDigitC(out[last]) && isUpper(out[last - 1]) && out[last - 1] != ' ') {
      out = out.substring(0, last) + " " + out.substring(last);
    }
  }

  // 既知の略称を正式名に展開（展開後に再トリム）
  out = expandKnownGames(out);
  out.trim();

  if (out.length()) out += " detected.";
  return out;
}

// 描画更新：短文=中央表示、長文=マルキー
void renderDisplay() {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(marqueeText, 0, 0, &x1, &y1, &w, &h);
  textW = (int16_t)w;

  if (textW <= SCREEN_WIDTH) {
    // 中央表示
    int16_t x = (SCREEN_WIDTH - textW) / 2;
    int16_t y = (SCREEN_HEIGHT - (int16_t)h) / 2 + 8; // だいたい縦センター
    display.setCursor(x, y);
    display.print(marqueeText);
  } else {
    // マルキー：2回描画してループ感を出す
    int16_t y = 10;
    int16_t loopW = textW + GAP_PX;
    int16_t xA = textX;
    int16_t xB = textX + loopW;

    display.setCursor(xA, y);
    display.print(marqueeText);
    display.setCursor(xB, y);
    display.print(marqueeText);

    // スクロール進行
    uint32_t now = millis();
    if (now - lastScrollMs >= SCROLL_INTERVAL_MS) {
      lastScrollMs = now;
      textX -= 1; // 1pxずつ左へ
      if (textX <= -loopW) {
        textX = 0; // ループ
      }
    }
  }

  display.display();
}

void applyNewText(const String& pretty) {
  marqueeText = pretty;
  textX = 0;
  lastScrollMs = millis();
  renderDisplay();
}

void drawAppImmediate(const String& s) {
  applyNewText(s);
}

void setup() {
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setFont(&Roboto_Condensed_14);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.print("TN-01 ready");
  display.display();

  Serial.begin(115200);
}

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
      if (inbuf.length() > 96) inbuf.remove(0); // guard
    }
  }

  // スクロールが必要なときは継続的に再描画
  if (marqueeText.length()) {
    renderDisplay();
  }
}
