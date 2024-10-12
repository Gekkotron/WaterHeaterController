
volatile unsigned long lastTime = 0;  // Last time a zero-crossing was detected
volatile unsigned long cycleTime = 0; // Time between two zero-crossings (half cycle)

const int zeroCrossingPin = 2;  // Pin connected to MOC3041 output (INT0 on Arduino Uno)
float frequency = 0;

void setup() {
  pinMode(zeroCrossingPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(zeroCrossingPin), zeroCrossingISR, FALLING);  // Trigger on falling edge
  
  Serial.begin(115200);
  Serial.println("AC Init: ");
}

void loop() {
  // Calculate frequency in the main loop
  if (cycleTime != 0) {
    // Frequency = 1 / (full cycle time in seconds)
    frequency = 1000000.0 / cycleTime;  // Full cycle frequency
    
    Serial.print("AC Frequency: ");
    Serial.print(frequency);
    Serial.println(" Hz");
    
    delay(500);  // Update frequency every 0.5 second*/
  }
}

// Interrupt Service Routine (ISR) for zero-crossing detection
void zeroCrossingISR() {
  unsigned long currentTime = micros();  // Get current time in microseconds
  
  // Calculate time difference between this and the last zero-crossing
  cycleTime = currentTime - lastTime;
  lastTime = currentTime;
}