#include <Wire.h>
#include <AS5600.h>

// AS5600 setup
AS5600 encoder;
int lastHEValScaled = -1;
unsigned long lastHEPrintTime = 0;

// digital pin definitions
int greenLED = 2;
int redLED = 3;
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

  digitalWrite(redLED, HIGH);
  digitalWrite(bIPB_led, LOW);

  Serial.begin(9600);
  Wire.begin();

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
  } else if (mtlThrwState == LOW) {
    Serial.println("Metal Switch ON");
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
  } else if (illumRockerState == HIGH) {
    Serial.println("rocker activated");
    delay(800);
  } else if (bIPBState == LOW) {
    digitalWrite(bIPB_led, HIGH);
    Serial.println("Big Switch Pressed!");
  } else {
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, HIGH);
    digitalWrite(bIPB_led, LOW);
  }

  delay(100);
}