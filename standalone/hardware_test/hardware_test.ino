/**
 * @file hardware_test.ino
 * @brief Simple Interactive Hardware Tester
 * @details Test each component with Serial commands
 * 
 * COMMANDS:
 *   f - Move forward (2 seconds)
 *   b - Move backward (2 seconds)
 *   l - Turn left (1 second)
 *   r - Turn right (1 second)
 *   s - Stop motors
 *   o - Open clamp
 *   c - Close clamp
 *   u - Arm up
 *   d - Arm down
 *   p - Pickup sequence
 *   x - Drop sequence
 *   1 - Read ultrasonic
 *   2 - Read color sensor
 *   3 - Read IR sensors
 *   a - Read ALL sensors
 *   t - Test ALL motors
 *   h - Show help
 */

#include <Servo.h>

// ============================================================================
// PIN DEFINITIONS
// ============================================================================
#define PIN_COLOR_S0      2
#define PIN_COLOR_S1      3
#define PIN_COLOR_S2      4
#define PIN_COLOR_S3      5
#define PIN_COLOR_OUT     6

#define PIN_MOTOR_ENA     9
#define PIN_MOTOR_IN1     8
#define PIN_MOTOR_IN2     7
#define PIN_MOTOR_ENB     10
#define PIN_MOTOR_IN3     11
#define PIN_MOTOR_IN4     13

#define PIN_SERVO_BASE    12
#define PIN_SERVO_CLAMP   A4

#define PIN_ULTRA_TRIG    A0
#define PIN_ULTRA_ECHO    A1

#define PIN_IR_LEFT       A2
#define PIN_IR_RIGHT      A3

// ============================================================================
// GLOBALS
// ============================================================================
Servo baseServo, clampServo;

// ============================================================================
// MOTOR FUNCTIONS
// ============================================================================
void stopMotors() {
  analogWrite(PIN_MOTOR_ENA, 0);
  analogWrite(PIN_MOTOR_ENB, 0);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, LOW);
}

void moveForward() {
  Serial.println(F(">>> Moving FORWARD..."));
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  analogWrite(PIN_MOTOR_ENA, 150);
  analogWrite(PIN_MOTOR_ENB, 150);
  delay(2000);
  stopMotors();
  Serial.println(F(">>> Stopped"));
}

void moveBackward() {
  Serial.println(F(">>> Moving BACKWARD..."));
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, HIGH);
  analogWrite(PIN_MOTOR_ENA, 150);
  analogWrite(PIN_MOTOR_ENB, 150);
  delay(2000);
  stopMotors();
  Serial.println(F(">>> Stopped"));
}

void turnLeft() {
  Serial.println(F(">>> Turning LEFT..."));
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  analogWrite(PIN_MOTOR_ENA, 120);
  analogWrite(PIN_MOTOR_ENB, 120);
  delay(1000);
  stopMotors();
  Serial.println(F(">>> Stopped"));
}

void turnRight() {
  Serial.println(F(">>> Turning RIGHT..."));
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, HIGH);
  analogWrite(PIN_MOTOR_ENA, 120);
  analogWrite(PIN_MOTOR_ENB, 120);
  delay(1000);
  stopMotors();
  Serial.println(F(">>> Stopped"));
}

void testAllMotors() {
  Serial.println(F("\n=== TESTING ALL MOTORS ===\n"));
  
  Serial.println(F("1. Forward..."));
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  analogWrite(PIN_MOTOR_ENA, 150);
  analogWrite(PIN_MOTOR_ENB, 150);
  delay(1500);
  stopMotors();
  delay(500);
  
  Serial.println(F("2. Backward..."));
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, HIGH);
  analogWrite(PIN_MOTOR_ENA, 150);
  analogWrite(PIN_MOTOR_ENB, 150);
  delay(1500);
  stopMotors();
  delay(500);
  
  Serial.println(F("3. Left turn..."));
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  analogWrite(PIN_MOTOR_ENA, 120);
  analogWrite(PIN_MOTOR_ENB, 120);
  delay(1000);
  stopMotors();
  delay(500);
  
  Serial.println(F("4. Right turn..."));
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, HIGH);
  analogWrite(PIN_MOTOR_ENA, 120);
  analogWrite(PIN_MOTOR_ENB, 120);
  delay(1000);
  stopMotors();
  
  Serial.println(F("\n=== MOTOR TEST COMPLETE ===\n"));
}

// ============================================================================
// SERVO FUNCTIONS
// ============================================================================
void openClamp() {
  Serial.println(F(">>> Clamp OPEN"));
  clampServo.write(90);
  delay(300);
}

void closeClamp() {
  Serial.println(F(">>> Clamp CLOSED"));
  clampServo.write(0);
  delay(300);
}

void armUp() {
  Serial.println(F(">>> Arm UP"));
  baseServo.write(90);
  delay(500);
}

void armDown() {
  Serial.println(F(">>> Arm DOWN"));
  baseServo.write(0);
  delay(500);
}

void pickup() {
  Serial.println(F(">>> PICKUP sequence..."));
  baseServo.write(0);   // Arm down
  delay(500);
  clampServo.write(0);  // Close clamp
  delay(300);
  baseServo.write(45);  // Arm carry
  delay(300);
  Serial.println(F(">>> Pickup complete"));
}

void dropItem() {
  Serial.println(F(">>> DROP sequence..."));
  baseServo.write(0);   // Arm down
  delay(500);
  clampServo.write(90); // Open clamp
  delay(300);
  baseServo.write(45);  // Arm carry
  delay(300);
  Serial.println(F(">>> Drop complete"));
}

// ============================================================================
// SENSOR FUNCTIONS
// ============================================================================
void readUltrasonic() {
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRA_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  
  unsigned long duration = pulseIn(PIN_ULTRA_ECHO, HIGH, 25000);
  float distance = (duration == 0) ? 999.0 : (duration * 0.034) / 2.0;
  
  Serial.print(F(">>> Ultrasonic: "));
  Serial.print(distance);
  Serial.println(F(" cm"));
  
  if (distance < 15) {
    Serial.println(F("    [OBSTACLE DETECTED]"));
  }
}

void readColorSensor() {
  // Read RED
  digitalWrite(PIN_COLOR_S2, LOW);
  digitalWrite(PIN_COLOR_S3, LOW);
  delay(10);
  uint16_t r = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  // Read GREEN
  digitalWrite(PIN_COLOR_S2, HIGH);
  digitalWrite(PIN_COLOR_S3, HIGH);
  delay(10);
  uint16_t g = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  // Read BLUE
  digitalWrite(PIN_COLOR_S2, LOW);
  digitalWrite(PIN_COLOR_S3, HIGH);
  delay(10);
  uint16_t b = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  if (r == 0) r = 999;
  if (g == 0) g = 999;
  if (b == 0) b = 999;
  
  Serial.print(F(">>> Color: R="));
  Serial.print(r);
  Serial.print(F(" G="));
  Serial.print(g);
  Serial.print(F(" B="));
  Serial.print(b);
  Serial.print(F(" -> "));
  
  // Detect color
  if (r > 200 && g > 200 && b > 200) {
    Serial.println(F("BLACK"));
  } else if (r < g - 20 && r < b - 20 && r < 150) {
    Serial.println(F("RED"));
  } else if (g < r - 20 && g < b - 20 && g < 150) {
    Serial.println(F("GREEN"));
  } else if (b < r - 20 && b < g - 20 && b < 150) {
    Serial.println(F("BLUE"));
  } else if (r < 50 && g < 50 && b < 50) {
    Serial.println(F("WHITE"));
  } else {
    Serial.println(F("UNKNOWN"));
  }
}

void readIRSensors() {
  bool left = (digitalRead(PIN_IR_LEFT) == LOW);
  bool right = (digitalRead(PIN_IR_RIGHT) == LOW);
  
  Serial.print(F(">>> IR Sensors: Left="));
  Serial.print(left ? "ON_LINE" : "off");
  Serial.print(F("  Right="));
  Serial.println(right ? "ON_LINE" : "off");
}

void readAllSensors() {
  Serial.println(F("\n=== ALL SENSORS ===\n"));
  readUltrasonic();
  readColorSensor();
  readIRSensors();
  Serial.println();
}

// ============================================================================
// HELP
// ============================================================================
void printHelp() {
  Serial.println(F("\n╔═══════════════════════════════════════════╗"));
  Serial.println(F("║       HARDWARE TEST COMMANDS              ║"));
  Serial.println(F("╠═══════════════════════════════════════════╣"));
  Serial.println(F("║  MOTORS:                                  ║"));
  Serial.println(F("║    f - Forward (2s)    b - Backward (2s)  ║"));
  Serial.println(F("║    l - Turn left (1s)  r - Turn right (1s)║"));
  Serial.println(F("║    s - Stop            t - Test all motors║"));
  Serial.println(F("║                                           ║"));
  Serial.println(F("║  SERVOS:                                  ║"));
  Serial.println(F("║    o - Open clamp      c - Close clamp    ║"));
  Serial.println(F("║    u - Arm up          d - Arm down       ║"));
  Serial.println(F("║    p - Pickup          x - Drop           ║"));
  Serial.println(F("║                                           ║"));
  Serial.println(F("║  SENSORS:                                 ║"));
  Serial.println(F("║    1 - Ultrasonic      2 - Color          ║"));
  Serial.println(F("║    3 - IR sensors      a - All sensors    ║"));
  Serial.println(F("║                                           ║"));
  Serial.println(F("║    h - Show this help                     ║"));
  Serial.println(F("╚═══════════════════════════════════════════╝\n"));
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(9600);
  
  // Color sensor
  pinMode(PIN_COLOR_S0, OUTPUT);
  pinMode(PIN_COLOR_S1, OUTPUT);
  pinMode(PIN_COLOR_S2, OUTPUT);
  pinMode(PIN_COLOR_S3, OUTPUT);
  pinMode(PIN_COLOR_OUT, INPUT);
  digitalWrite(PIN_COLOR_S0, HIGH);
  digitalWrite(PIN_COLOR_S1, LOW);
  
  // Ultrasonic
  pinMode(PIN_ULTRA_TRIG, OUTPUT);
  pinMode(PIN_ULTRA_ECHO, INPUT);
  
  // IR sensors
  pinMode(PIN_IR_LEFT, INPUT);
  pinMode(PIN_IR_RIGHT, INPUT);
  
  // Motors
  pinMode(PIN_MOTOR_ENA, OUTPUT);
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_MOTOR_ENB, OUTPUT);
  pinMode(PIN_MOTOR_IN3, OUTPUT);
  pinMode(PIN_MOTOR_IN4, OUTPUT);
  stopMotors();
  
  // Servos
  baseServo.attach(PIN_SERVO_BASE);
  clampServo.attach(PIN_SERVO_CLAMP);
  baseServo.write(0);
  clampServo.write(90);
  
  delay(500);
  
  Serial.println(F("\n═══════════════════════════════════════════"));
  Serial.println(F("       HARDWARE TESTER READY"));
  Serial.println(F("═══════════════════════════════════════════"));
  printHelp();
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    // Clear buffer
    while (Serial.available()) Serial.read();
    
    switch (cmd) {
      // Motors
      case 'f': case 'F': moveForward(); break;
      case 'b': case 'B': moveBackward(); break;
      case 'l': case 'L': turnLeft(); break;
      case 'r': case 'R': turnRight(); break;
      case 's': case 'S': stopMotors(); Serial.println(F(">>> Motors STOPPED")); break;
      case 't': case 'T': testAllMotors(); break;
      
      // Servos
      case 'o': case 'O': openClamp(); break;
      case 'c': case 'C': closeClamp(); break;
      case 'u': case 'U': armUp(); break;
      case 'd': case 'D': armDown(); break;
      case 'p': case 'P': pickup(); break;
      case 'x': case 'X': dropItem(); break;
      
      // Sensors
      case '1': readUltrasonic(); break;
      case '2': readColorSensor(); break;
      case '3': readIRSensors(); break;
      case 'a': case 'A': readAllSensors(); break;
      
      // Help
      case 'h': case 'H': case '?': printHelp(); break;
      
      default:
        if (cmd != '\n' && cmd != '\r') {
          Serial.print(F("Unknown command: "));
          Serial.println(cmd);
          Serial.println(F("Type 'h' for help"));
        }
        break;
    }
  }
  delay(50);
}
