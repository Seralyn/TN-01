const int greenLED = 2;
const int redLED = 3;

void setup() {
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  Serial.begin(9600);
  while (!Serial);

  Serial.println("Turning LEDs OFF");
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);
  delay(2000);

  Serial.println("Turning LEDs ON");
  digitalWrite(greenLED, HIGH);
  digitalWrite(redLED, HIGH);
}

void loop() {}