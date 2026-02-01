/**
 * SIMPLE MOTOR TEST
 * Moves Motor A forward for 1 second, then stops.
 * Compatible with Arduino UNO R4 Minima
 */

// Motor A pins (Right motor)
#define ENA  9   // PWM speed control
#define IN1  8   // Direction pin 1
#define IN2  7   // Direction pin 2

void setup() {
  // Set pins as outputs
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  
  // Set direction: IN1=HIGH, IN2=LOW = forward
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  
  // Set speed (0-255), 150 is medium
  analogWrite(ENA, 150);
  
  // Wait 1 second
  delay(1000);
  
  // Stop motor
  analogWrite(ENA, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

void loop() {
  // Nothing - motor runs once in setup
}
