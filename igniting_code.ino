const int inputPin = 8; // Pin connected to the ESP32
const int relaypin = 6; // Pin connected to the base resistor of the relay
const int indicator_pin = 9; //Pin connected to LED
unsigned long signalStartTime = 0; // Variable to store the time when the signal starts
unsigned long signalDuration = 0; // Variable to store the duration of the signal
bool signalActive = false; // Variable to track if the signal is currently active
bool triggered = false; // Variable to track if the 10-second period is active

void setup() {
  pinMode(inputPin, INPUT); // Configure the input pin as input
  pinMode(relaypin, OUTPUT); // Configure the relay base pin as output
  pinMode(indicator_pin, OUTPUT); // Configure the indicator pin as output
  digitalWrite(relaypin, LOW); // Ensure the relay is off initially
  Serial.begin(9600); 
}

void loop() {
  int inputState = digitalRead(inputPin); // Read the state of the input pin

  if (inputState == HIGH && !signalActive) { 
    signalActive = true;
    signalStartTime = millis();
  } else if (inputState == LOW && signalActive) { 
    signalDuration = millis() - signalStartTime;
    signalActive = false;
    if (signalDuration > 750) { // Check if the signal duration is over 750ms for ignore incorrect signal
      triggered = true;
      digitalWrite(relaypin, HIGH); // Turn on the relay
      digitalWrite(indicator_pin, HIGH); //Turn on the LED
      Serial.println("Triggered for 10 seconds"); // Debug message 
    }
  }

  if (triggered && millis() - signalStartTime >= 10000) { // Check if 10 seconds have passed
    digitalWrite(relaypin, LOW); // Turn off the relay
    digitalWrite(indicator_pin, LOW); //Turn off the LED
    triggered = false;
    Serial.println("Reset"); // Debug message 
  }
}
