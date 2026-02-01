/**
 * @file diagnostic.ino
 * @brief DIAGNOSTIC TOOL - Find out what's working and what's not
 * 
 * This will help identify:
 * - Is Serial communication working?
 * - Is the Arduino receiving commands?
 * - Are the motor pins outputting signals?
 * - Is the motor driver getting power?
 */

#include <Servo.h>

// Built-in LED (Pin 13 on most Arduinos)
#define LED_BUILTIN 13

// Motor pins
#define PIN_MOTOR_ENA     9
#define PIN_MOTOR_IN1     8
#define PIN_MOTOR_IN2     7
#define PIN_MOTOR_ENB     10
#define PIN_MOTOR_IN3     11
#define PIN_MOTOR_IN4     12  // Direction pin 2 for Motor B

// Servo pins
#define PIN_SERVO_BASE    A5
#define PIN_SERVO_CLAMP   A4

// Ultrasonic pins
#define PIN_ULTRA_TRIG    A0
#define PIN_ULTRA_ECHO    A1

// Color sensor pins
#define PIN_COLOR_S0      2
#define PIN_COLOR_S1      3
#define PIN_COLOR_S2      4
#define PIN_COLOR_S3      5
#define PIN_COLOR_OUT     6

// IR pins
#define PIN_IR_LEFT       A2
#define PIN_IR_RIGHT      A3

Servo baseServo, clampServo;
int blinkCount = 0;

void setup() {
  // Start Serial FIRST
  Serial.begin(9600);
  
  // Wait for Serial to be ready (important for some boards)
  while (!Serial) {
    ; // wait
  }
  
  delay(1000);
  
  Serial.println();
  Serial.println(F("===================================="));
  Serial.println(F("   DIAGNOSTIC TOOL STARTING"));
  Serial.println(F("===================================="));
  Serial.println();
  
  // Setup all pins
  Serial.println(F("Setting up pins..."));
  
  // Motor pins
  pinMode(PIN_MOTOR_ENA, OUTPUT);
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_MOTOR_ENB, OUTPUT);
  pinMode(PIN_MOTOR_IN3, OUTPUT);
  pinMode(PIN_MOTOR_IN4, OUTPUT);
  
  // Stop motors
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  analogWrite(PIN_MOTOR_ENA, 0);
  analogWrite(PIN_MOTOR_ENB, 0);
  
  // Ultrasonic
  pinMode(PIN_ULTRA_TRIG, OUTPUT);
  pinMode(PIN_ULTRA_ECHO, INPUT);
  
  // Color sensor
  pinMode(PIN_COLOR_S0, OUTPUT);
  pinMode(PIN_COLOR_S1, OUTPUT);
  pinMode(PIN_COLOR_S2, OUTPUT);
  pinMode(PIN_COLOR_S3, OUTPUT);
  pinMode(PIN_COLOR_OUT, INPUT);
  digitalWrite(PIN_COLOR_S0, HIGH);
  digitalWrite(PIN_COLOR_S1, LOW);
  
  // IR sensors
  pinMode(PIN_IR_LEFT, INPUT);
  pinMode(PIN_IR_RIGHT, INPUT);
  
  // Servos
  baseServo.attach(PIN_SERVO_BASE);
  clampServo.attach(PIN_SERVO_CLAMP);
  baseServo.write(45);
  clampServo.write(90);
  
  Serial.println(F("Pins configured!"));
  Serial.println();
  
  // Print menu
  printMenu();
}

void printMenu() {
  Serial.println(F("╔════════════════════════════════════════╗"));
  Serial.println(F("║         DIAGNOSTIC COMMANDS            ║"));
  Serial.println(F("╠════════════════════════════════════════╣"));
  Serial.println(F("║  0 - Test Serial (echo test)           ║"));
  Serial.println(F("║  1 - Test Motor A only (LEFT)          ║"));
  Serial.println(F("║  2 - Test Motor B only (RIGHT)         ║"));
  Serial.println(F("║  3 - Test BOTH motors forward          ║"));
  Serial.println(F("║  4 - Test servos                       ║"));
  Serial.println(F("║  5 - Test ultrasonic                   ║"));
  Serial.println(F("║  6 - Test color sensor                 ║"));
  Serial.println(F("║  7 - Test IR sensors                   ║"));
  Serial.println(F("║  8 - CONTINUOUS sensor reading         ║"));
  Serial.println(F("║  9 - Stop everything                   ║"));
  Serial.println(F("║  ? - Show this menu                    ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Type a number and press ENTER:"));
}

void loop() {
  // Blink indicator - shows Arduino is running
  blinkCount++;
  if (blinkCount >= 20) {
    blinkCount = 0;
    // Toggle pin 13 would conflict with motor, so just print a dot
    Serial.print(".");
  }
  
  // Check for Serial input
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    // Clear any extra characters (like newline)
    delay(10);
    while (Serial.available()) {
      Serial.read();
    }
    
    Serial.println();
    Serial.print(F(">>> Received command: '"));
    Serial.print(cmd);
    Serial.println(F("'"));
    
    switch (cmd) {
      case '0':
        testSerial();
        break;
      case '1':
        testMotorA();
        break;
      case '2':
        testMotorB();
        break;
      case '3':
        testBothMotors();
        break;
      case '4':
        testServos();
        break;
      case '5':
        testUltrasonic();
        break;
      case '6':
        testColorSensor();
        break;
      case '7':
        testIRSensors();
        break;
      case '8':
        continuousSensorReading();
        break;
      case '9':
        stopEverything();
        break;
      case '?':
      case 'h':
      case 'H':
        printMenu();
        break;
      case '\n':
      case '\r':
        // Ignore newlines
        break;
      default:
        Serial.print(F("Unknown command: "));
        Serial.println(cmd);
        Serial.println(F("Type ? for help"));
        break;
    }
  }
  
  delay(50);
}

// ============================================================================
// TEST FUNCTIONS
// ============================================================================

void testSerial() {
  Serial.println(F("\n=== SERIAL TEST ==="));
  Serial.println(F("If you can see this, Serial is working!"));
  Serial.println(F("Type anything and I'll echo it back."));
  Serial.println(F("Press ENTER alone to exit echo mode."));
  
  while (true) {
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      if (input.length() == 0) {
        Serial.println(F("Exiting echo mode."));
        break;
      }
      Serial.print(F("You typed: "));
      Serial.println(input);
    }
    delay(10);
  }
}

void testMotorA() {
  Serial.println(F("\n=== MOTOR A (LEFT WHEEL) TEST ==="));
  Serial.println(F("This tests pins: ENA(9), IN1(8), IN2(7)"));
  Serial.println();
  
  Serial.println(F("Step 1: Setting IN1=HIGH, IN2=LOW (forward direction)"));
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  delay(100);
  
  Serial.println(F("Step 2: Setting ENA=150 (PWM speed)"));
  Serial.println(F(">>> Motor A (LEFT) should spin now for 3 seconds..."));
  analogWrite(PIN_MOTOR_ENA, 150);
  
  delay(3000);
  
  Serial.println(F("Step 3: Stopping motor"));
  analogWrite(PIN_MOTOR_ENA, 0);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  
  Serial.println(F("\nDid Motor A (LEFT wheel) spin?"));
  Serial.println(F("If NO - check: ENA->Pin9, IN1->Pin8, IN2->Pin7, OUT1/OUT2->Motor"));
  Serial.println();
}

void testMotorB() {
  Serial.println(F("\n=== MOTOR B (RIGHT WHEEL) TEST ==="));
  Serial.println(F("This tests pins: ENB(10), IN3(11), IN4(12)"));
  Serial.println();
  
  Serial.println(F("Step 1: Setting IN3=HIGH, IN4=LOW (forward direction)"));
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  delay(100);
  
  Serial.println(F("Step 2: Setting ENB=150 (PWM speed)"));
  Serial.println(F(">>> Motor B (RIGHT) should spin now for 3 seconds..."));
  analogWrite(PIN_MOTOR_ENB, 150);
  
  delay(3000);
  
  Serial.println(F("Step 3: Stopping motor"));
  analogWrite(PIN_MOTOR_ENB, 0);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  
  Serial.println(F("\nDid Motor B (RIGHT wheel) spin?"));
  Serial.println(F("If NO - check: ENB->Pin10, IN3->Pin11, IN4->Pin12, OUT3/OUT4->Motor"));
  Serial.println();
}

void testBothMotors() {
  Serial.println(F("\n=== BOTH MOTORS TEST ==="));
  Serial.println(F("Both motors should spin forward for 3 seconds..."));
  Serial.println(F("Motor A = LEFT, Motor B = RIGHT"));
  Serial.println(F("Speed compensation applied: LEFT=150, RIGHT=135 (90%)"));
  Serial.println();
  
  // Set direction
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  
  // Set speed with compensation (right motor is faster, reduce it)
  Serial.println(F(">>> Starting motors..."));
  analogWrite(PIN_MOTOR_ENA, 150);  // LEFT motor
  analogWrite(PIN_MOTOR_ENB, 135);  // RIGHT motor (reduced to match left)
  
  delay(3000);
  
  // Stop
  Serial.println(F(">>> Stopping motors..."));
  analogWrite(PIN_MOTOR_ENA, 0);
  analogWrite(PIN_MOTOR_ENB, 0);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  
  Serial.println(F("\nResults:"));
  Serial.println(F("- Both wheels spun equally: GOOD!"));
  Serial.println(F("- Only LEFT spun: Check Motor B (RIGHT) wiring"));
  Serial.println(F("- Only RIGHT spun: Check Motor A (LEFT) wiring"));
  Serial.println(F("- Neither spun: Check motor driver power (12V) and GND"));
  Serial.println();
}

void testServos() {
  Serial.println(F("\n=== SERVO TEST ==="));
  Serial.println(F("Base servo on Pin 12, Clamp servo on Pin A4"));
  Serial.println();
  
  Serial.println(F("Step 1: Moving BASE servo to 0 degrees..."));
  baseServo.write(0);
  delay(1000);
  
  Serial.println(F("Step 2: Moving BASE servo to 90 degrees..."));
  baseServo.write(90);
  delay(1000);
  
  Serial.println(F("Step 3: Moving BASE servo to 45 degrees..."));
  baseServo.write(45);
  delay(500);
  
  Serial.println(F("Step 4: Moving CLAMP servo to 0 degrees (close)..."));
  clampServo.write(0);
  delay(1000);
  
  Serial.println(F("Step 5: Moving CLAMP servo to 90 degrees (open)..."));
  clampServo.write(90);
  delay(1000);
  
  Serial.println(F("\nDid both servos move?"));
  Serial.println(F("If NO - check: VCC->5V, GND->GND, Signal wires to Pin12 and A4"));
  Serial.println();
}

void testUltrasonic() {
  Serial.println(F("\n=== ULTRASONIC TEST ==="));
  Serial.println(F("Trigger on Pin A0, Echo on Pin A1"));
  Serial.println(F("Taking 5 readings..."));
  Serial.println();
  
  for (int i = 0; i < 5; i++) {
    digitalWrite(PIN_ULTRA_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_ULTRA_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRA_TRIG, LOW);
    
    unsigned long duration = pulseIn(PIN_ULTRA_ECHO, HIGH, 30000);
    
    Serial.print(F("Reading "));
    Serial.print(i + 1);
    Serial.print(F(": Duration="));
    Serial.print(duration);
    Serial.print(F("us -> Distance="));
    
    if (duration == 0) {
      Serial.println(F("TIMEOUT (no echo)"));
    } else {
      float distance = (duration * 0.034) / 2.0;
      Serial.print(distance);
      Serial.println(F(" cm"));
    }
    
    delay(500);
  }
  
  Serial.println(F("\nIf all TIMEOUT: Check VCC->5V, GND->GND, Trig->A0, Echo->A1"));
  Serial.println();
}

void testColorSensor() {
  Serial.println(F("\n=== COLOR SENSOR TEST ==="));
  Serial.println(F("S0=Pin2, S1=Pin3, S2=Pin4, S3=Pin5, OUT=Pin6"));
  Serial.println(F("Taking 5 readings..."));
  Serial.println();
  
  for (int i = 0; i < 5; i++) {
    // Red
    digitalWrite(PIN_COLOR_S2, LOW);
    digitalWrite(PIN_COLOR_S3, LOW);
    delay(20);
    uint16_t r = pulseIn(PIN_COLOR_OUT, LOW, 50000);
    
    // Green
    digitalWrite(PIN_COLOR_S2, HIGH);
    digitalWrite(PIN_COLOR_S3, HIGH);
    delay(20);
    uint16_t g = pulseIn(PIN_COLOR_OUT, LOW, 50000);
    
    // Blue
    digitalWrite(PIN_COLOR_S2, LOW);
    digitalWrite(PIN_COLOR_S3, HIGH);
    delay(20);
    uint16_t b = pulseIn(PIN_COLOR_OUT, LOW, 50000);
    
    Serial.print(F("Reading "));
    Serial.print(i + 1);
    Serial.print(F(": R="));
    Serial.print(r);
    Serial.print(F(" G="));
    Serial.print(g);
    Serial.print(F(" B="));
    Serial.println(b);
    
    delay(300);
  }
  
  Serial.println(F("\nIf all zeros: Check wiring and VCC->5V, GND->GND"));
  Serial.println();
}

void testIRSensors() {
  Serial.println(F("\n=== IR SENSORS TEST ==="));
  Serial.println(F("Left on Pin A2, Right on Pin A3"));
  Serial.println(F("Taking 10 readings... Move sensors over black/white surfaces"));
  Serial.println();
  
  for (int i = 0; i < 10; i++) {
    int leftRaw = analogRead(PIN_IR_LEFT);
    int rightRaw = analogRead(PIN_IR_RIGHT);
    bool leftDigital = (digitalRead(PIN_IR_LEFT) == LOW);
    bool rightDigital = (digitalRead(PIN_IR_RIGHT) == LOW);
    
    Serial.print(F("Reading "));
    Serial.print(i + 1);
    Serial.print(F(": L_analog="));
    Serial.print(leftRaw);
    Serial.print(F(" L_digital="));
    Serial.print(leftDigital ? "LINE" : "no");
    Serial.print(F(" | R_analog="));
    Serial.print(rightRaw);
    Serial.print(F(" R_digital="));
    Serial.println(rightDigital ? "LINE" : "no");
    
    delay(500);
  }
  
  Serial.println();
}

void continuousSensorReading() {
  Serial.println(F("\n=== CONTINUOUS SENSOR READING ==="));
  Serial.println(F("Press any key to stop..."));
  Serial.println();
  
  while (!Serial.available()) {
    // Ultrasonic
    digitalWrite(PIN_ULTRA_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_ULTRA_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_ULTRA_TRIG, LOW);
    unsigned long duration = pulseIn(PIN_ULTRA_ECHO, HIGH, 25000);
    float dist = (duration == 0) ? 999 : (duration * 0.034) / 2.0;
    
    // IR
    bool irL = (digitalRead(PIN_IR_LEFT) == LOW);
    bool irR = (digitalRead(PIN_IR_RIGHT) == LOW);
    
    // Color (quick read)
    digitalWrite(PIN_COLOR_S2, HIGH);
    digitalWrite(PIN_COLOR_S3, HIGH);
    delay(5);
    uint16_t g = pulseIn(PIN_COLOR_OUT, LOW, 20000);
    
    Serial.print(F("Dist:"));
    Serial.print(dist, 1);
    Serial.print(F("cm IR:"));
    Serial.print(irL ? "L" : "-");
    Serial.print(irR ? "R" : "-");
    Serial.print(F(" Color:"));
    Serial.println(g);
    
    delay(200);
  }
  
  // Clear input
  while (Serial.available()) Serial.read();
  Serial.println(F("\nStopped."));
}

void stopEverything() {
  Serial.println(F("\n=== STOPPING EVERYTHING ==="));
  
  // Stop motors
  analogWrite(PIN_MOTOR_ENA, 0);
  analogWrite(PIN_MOTOR_ENB, 0);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  
  // Center servos
  baseServo.write(45);
  clampServo.write(90);
  
  Serial.println(F("All stopped."));
  Serial.println();
}
