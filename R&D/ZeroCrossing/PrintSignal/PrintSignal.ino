const int zeroCrossingPin = 2;  // Pin connected to MOC3041 output (INT0 on Arduino Uno)

void setup() {
  pinMode(zeroCrossingPin, INPUT);

  Serial.begin(115200);
  Serial.println("AC Init: ");
}

void loop() {
  Serial.print(digitalRead(2));
  Serial.println(" ");
}