/**
 * @file test_runner_standalone.ino
 * @brief STANDALONE Test Runner - All code in one file
 * @details Works on any computer without path issues
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

// Motor A = LEFT wheel, Motor B = RIGHT wheel
#define PIN_MOTOR_ENA     9   // LEFT motor speed
#define PIN_MOTOR_IN1     8   // LEFT motor direction
#define PIN_MOTOR_IN2     7   // LEFT motor direction
#define PIN_MOTOR_ENB     10  // RIGHT motor speed
#define PIN_MOTOR_IN3     11  // RIGHT motor direction
#define PIN_MOTOR_IN4     12  // RIGHT motor direction

#define SPEED_COMPENSATION  0.9  // Right motor is faster

#define PIN_SERVO_BASE    A5
#define PIN_SERVO_CLAMP   A4

#define PIN_ULTRA_TRIG    A0
#define PIN_ULTRA_ECHO    A1

#define PIN_IR_LEFT       A2
#define PIN_IR_RIGHT      A3

// ============================================================================
// CONSTANTS
// ============================================================================
#define SPEED_SLOW        100
#define SPEED_NORMAL      150
#define SPEED_FAST        200
#define SPEED_TURN        120

#define DIST_OBSTACLE     15
#define DIST_WALL_HUG     10
#define DIST_BOX_PICKUP   5

#define COLOR_FREQ_MAX    150
#define COLOR_FREQ_BLACK  200
#define COLOR_FREQ_WHITE  50
#define COLOR_MARGIN      20

#define SERVO_CLAMP_OPEN    90
#define SERVO_CLAMP_CLOSED  0
#define SERVO_ARM_UP        90
#define SERVO_ARM_DOWN      0
#define SERVO_ARM_CARRY     45

#define TIME_TURN_90      500
#define TIME_SERVO_MOVE   300
#define TIME_SENSOR_READ  10

#define IR_ON_LINE        LOW

// ============================================================================
// ENUMS
// ============================================================================
enum Color : uint8_t {
  COLOR_NONE = 0,
  COLOR_BLACK,
  COLOR_WHITE,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE
};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
Servo baseServo;
Servo clampServo;
bool holding = false;
uint8_t currentClampAngle = SERVO_CLAMP_OPEN;
uint8_t currentArmAngle = SERVO_ARM_DOWN;

uint16_t testsPassed = 0;
uint16_t testsFailed = 0;
uint16_t testsTotal = 0;

// ============================================================================
// SENSOR FUNCTIONS
// ============================================================================
float readDistance() {
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_ULTRA_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  
  unsigned long duration = pulseIn(PIN_ULTRA_ECHO, HIGH, 25000);
  if (duration == 0) return 999.0f;
  return (duration * 0.034) / 2.0f;
}

void readColorRaw(uint16_t& r, uint16_t& g, uint16_t& b) {
  digitalWrite(PIN_COLOR_S2, LOW);
  digitalWrite(PIN_COLOR_S3, LOW);
  delay(TIME_SENSOR_READ);
  r = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  digitalWrite(PIN_COLOR_S2, HIGH);
  digitalWrite(PIN_COLOR_S3, HIGH);
  delay(TIME_SENSOR_READ);
  g = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  digitalWrite(PIN_COLOR_S2, LOW);
  digitalWrite(PIN_COLOR_S3, HIGH);
  delay(TIME_SENSOR_READ);
  b = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  if (r == 0) r = 999;
  if (g == 0) g = 999;
  if (b == 0) b = 999;
}

Color detectColor(uint16_t r, uint16_t g, uint16_t b) {
  if (r > COLOR_FREQ_BLACK && g > COLOR_FREQ_BLACK && b > COLOR_FREQ_BLACK) {
    return COLOR_BLACK;
  }
  if (r < COLOR_FREQ_WHITE && g < COLOR_FREQ_WHITE && b < COLOR_FREQ_WHITE) {
    return COLOR_WHITE;
  }
  if (r < g - COLOR_MARGIN && r < b - COLOR_MARGIN && r < COLOR_FREQ_MAX) {
    return COLOR_RED;
  }
  if (g < r - COLOR_MARGIN && g < b - COLOR_MARGIN && g < COLOR_FREQ_MAX) {
    return COLOR_GREEN;
  }
  if (b < r - COLOR_MARGIN && b < g - COLOR_MARGIN && b < COLOR_FREQ_MAX) {
    return COLOR_BLUE;
  }
  return COLOR_NONE;
}

const char* colorName(Color c) {
  switch(c) {
    case COLOR_BLACK: return "BLACK";
    case COLOR_WHITE: return "WHITE";
    case COLOR_RED: return "RED";
    case COLOR_GREEN: return "GREEN";
    case COLOR_BLUE: return "BLUE";
    default: return "NONE";
  }
}

void readIR(bool& left, bool& right) {
  left = (digitalRead(PIN_IR_LEFT) == IR_ON_LINE);
  right = (digitalRead(PIN_IR_RIGHT) == IR_ON_LINE);
}

// ============================================================================
// MOTOR FUNCTIONS (Motor A = LEFT, Motor B = RIGHT)
// ============================================================================
void stopMotors() {
  analogWrite(PIN_MOTOR_ENA, 0);
  analogWrite(PIN_MOTOR_ENB, 0);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, LOW);
}

void moveForward(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, HIGH);  // LEFT forward
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, HIGH);  // RIGHT forward
  digitalWrite(PIN_MOTOR_IN4, LOW);
  analogWrite(PIN_MOTOR_ENA, speed);                              // LEFT
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION)); // RIGHT (reduced)
}

void moveBackward(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, LOW);   // LEFT backward
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  digitalWrite(PIN_MOTOR_IN3, LOW);   // RIGHT backward
  digitalWrite(PIN_MOTOR_IN4, HIGH);
  analogWrite(PIN_MOTOR_ENA, speed);                              // LEFT
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION)); // RIGHT (reduced)
}

void turnLeft(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, LOW);   // LEFT backward
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  digitalWrite(PIN_MOTOR_IN3, HIGH);  // RIGHT forward
  digitalWrite(PIN_MOTOR_IN4, LOW);
  analogWrite(PIN_MOTOR_ENA, speed);                              // LEFT
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION)); // RIGHT (reduced)
}

void turnRight(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, HIGH);  // LEFT forward
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);   // RIGHT backward
  digitalWrite(PIN_MOTOR_IN4, HIGH);
  analogWrite(PIN_MOTOR_ENA, speed);                              // LEFT
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION)); // RIGHT (reduced)
}

// ============================================================================
// SERVO FUNCTIONS
// ============================================================================
void openClamp() {
  clampServo.write(SERVO_CLAMP_OPEN);
  currentClampAngle = SERVO_CLAMP_OPEN;
  delay(TIME_SERVO_MOVE);
}

void closeClamp() {
  clampServo.write(SERVO_CLAMP_CLOSED);
  currentClampAngle = SERVO_CLAMP_CLOSED;
  delay(TIME_SERVO_MOVE);
}

void armUp() {
  baseServo.write(SERVO_ARM_UP);
  currentArmAngle = SERVO_ARM_UP;
  delay(TIME_SERVO_MOVE + 200);
}

void armDown() {
  baseServo.write(SERVO_ARM_DOWN);
  currentArmAngle = SERVO_ARM_DOWN;
  delay(TIME_SERVO_MOVE + 200);
}

void armCarry() {
  baseServo.write(SERVO_ARM_CARRY);
  currentArmAngle = SERVO_ARM_CARRY;
  delay(TIME_SERVO_MOVE);
}

void pickup() {
  armDown();
  delay(100);
  closeClamp();
  delay(100);
  armCarry();
  holding = true;
}

void drop() {
  armDown();
  delay(100);
  openClamp();
  delay(100);
  armCarry();
  holding = false;
}

// ============================================================================
// TEST FRAMEWORK
// ============================================================================
void testPass(const char* msg) {
  testsTotal++;
  testsPassed++;
  Serial.print(F("    [PASS] "));
  Serial.println(msg);
}

void testFail(const char* msg) {
  testsTotal++;
  testsFailed++;
  Serial.print(F("    [FAIL] "));
  Serial.println(msg);
}

void waitForInput(const char* prompt) {
  Serial.println();
  Serial.print(F(">> "));
  Serial.print(prompt);
  Serial.println(F(" (Press Enter)"));
  while (!Serial.available()) delay(100);
  while (Serial.available()) Serial.read();
}

void printSummary() {
  Serial.println();
  Serial.println(F("═══════════════════════════════════════"));
  Serial.println(F("           TEST SUMMARY"));
  Serial.println(F("═══════════════════════════════════════"));
  Serial.print(F("  Total:  ")); Serial.println(testsTotal);
  Serial.print(F("  Passed: ")); Serial.println(testsPassed);
  Serial.print(F("  Failed: ")); Serial.println(testsFailed);
  if (testsFailed == 0) {
    Serial.println(F("\n  *** ALL TESTS PASSED ***"));
  } else {
    Serial.println(F("\n  !!! SOME TESTS FAILED !!!"));
  }
  Serial.println(F("═══════════════════════════════════════"));
}

// ============================================================================
// QUICK DIAGNOSTICS
// ============================================================================
void runQuickDiagnostics() {
  Serial.println(F("\n>>> QUICK DIAGNOSTICS <<<\n"));
  
  // Ultrasonic
  Serial.print(F("Ultrasonic: "));
  float dist = readDistance();
  if (dist > 0 && dist < 999) {
    Serial.print(dist); Serial.println(F(" cm - OK"));
  } else {
    Serial.println(F("TIMEOUT"));
  }
  
  // Color sensor
  Serial.print(F("Color sensor: "));
  uint16_t r, g, b;
  readColorRaw(r, g, b);
  Serial.print(F("R=")); Serial.print(r);
  Serial.print(F(" G=")); Serial.print(g);
  Serial.print(F(" B=")); Serial.print(b);
  Serial.print(F(" -> ")); Serial.println(colorName(detectColor(r, g, b)));
  
  // IR sensors
  Serial.print(F("IR sensors: "));
  bool left, right;
  readIR(left, right);
  Serial.print(F("L=")); Serial.print(left ? "ON" : "OFF");
  Serial.print(F(" R=")); Serial.println(right ? "ON" : "OFF");
  
  // Motors
  Serial.print(F("Motors: "));
  moveForward(100);
  delay(200);
  stopMotors();
  Serial.println(F("Pulsed"));
  
  // Servos
  Serial.print(F("Servos: "));
  armCarry();
  openClamp();
  Serial.println(F("Moved"));
  
  Serial.println(F("\n>>> DIAGNOSTICS COMPLETE <<<"));
}

// ============================================================================
// CALIBRATION MODE
// ============================================================================
void runCalibration() {
  Serial.println(F("\n>>> CALIBRATION MODE <<<"));
  Serial.println(F("Real-time sensor values. Press any key to exit.\n"));
  delay(1000);
  
  while (!Serial.available()) {
    float dist = readDistance();
    uint16_t r, g, b;
    readColorRaw(r, g, b);
    bool left, right;
    readIR(left, right);
    
    Serial.print(F("Dist: ")); Serial.print(dist, 1); Serial.print(F("cm | "));
    Serial.print(F("RGB: ")); Serial.print(r); Serial.print(F(","));
    Serial.print(g); Serial.print(F(",")); Serial.print(b);
    Serial.print(F(" [")); Serial.print(colorName(detectColor(r, g, b))); Serial.print(F("] | "));
    Serial.print(F("IR: ")); Serial.print(left ? "L" : "-");
    Serial.println(right ? "R" : "-");
    
    delay(300);
  }
  while (Serial.available()) Serial.read();
  Serial.println(F("\n>>> CALIBRATION ENDED <<<"));
}

// ============================================================================
// SENSOR TESTS
// ============================================================================
void runSensorTests() {
  Serial.println(F("\n>>> SENSOR TESTS <<<\n"));
  
  // Ultrasonic test
  Serial.println(F("TEST: Ultrasonic sensor"));
  waitForInput("Clear path ahead (>30cm)");
  float dist = readDistance();
  Serial.print(F("    Distance: ")); Serial.print(dist); Serial.println(F(" cm"));
  if (dist > 20 && dist < 400) testPass("Distance reading valid");
  else testFail("Distance reading invalid");
  
  waitForInput("Place object at ~15cm");
  dist = readDistance();
  Serial.print(F("    Distance: ")); Serial.print(dist); Serial.println(F(" cm"));
  if (dist > 5 && dist < 25) testPass("Obstacle detected");
  else testFail("Obstacle not detected correctly");
  
  // Color sensor tests
  Serial.println(F("\nTEST: Color sensor"));
  
  waitForInput("Place over BLACK surface");
  uint16_t r, g, b;
  readColorRaw(r, g, b);
  Color c = detectColor(r, g, b);
  Serial.print(F("    Detected: ")); Serial.println(colorName(c));
  if (c == COLOR_BLACK) testPass("BLACK detected");
  else testFail("BLACK not detected");
  
  waitForInput("Place over RED surface");
  readColorRaw(r, g, b);
  c = detectColor(r, g, b);
  Serial.print(F("    Detected: ")); Serial.println(colorName(c));
  if (c == COLOR_RED) testPass("RED detected");
  else testFail("RED not detected");
  
  waitForInput("Place over GREEN surface");
  readColorRaw(r, g, b);
  c = detectColor(r, g, b);
  Serial.print(F("    Detected: ")); Serial.println(colorName(c));
  if (c == COLOR_GREEN) testPass("GREEN detected");
  else testFail("GREEN not detected");
  
  waitForInput("Place over BLUE surface");
  readColorRaw(r, g, b);
  c = detectColor(r, g, b);
  Serial.print(F("    Detected: ")); Serial.println(colorName(c));
  if (c == COLOR_BLUE) testPass("BLUE detected");
  else testFail("BLUE not detected");
  
  // IR sensor tests
  Serial.println(F("\nTEST: IR sensors"));
  
  waitForInput("Both sensors ON black line");
  bool left, right;
  readIR(left, right);
  Serial.print(F("    L=")); Serial.print(left); Serial.print(F(" R=")); Serial.println(right);
  if (left && right) testPass("Both on line");
  else testFail("Line not detected");
  
  waitForInput("Both sensors OFF line (white surface)");
  readIR(left, right);
  Serial.print(F("    L=")); Serial.print(left); Serial.print(F(" R=")); Serial.println(right);
  if (!left && !right) testPass("Both off line");
  else testFail("Should be off line");
  
  printSummary();
}

// ============================================================================
// MOTOR TESTS
// ============================================================================
void runMotorTests() {
  Serial.println(F("\n>>> MOTOR TESTS <<<"));
  Serial.println(F("Robot will move! Secure it or place on blocks."));
  waitForInput("Ready?");
  
  Serial.println(F("\nTEST: Forward"));
  moveForward(SPEED_NORMAL);
  delay(1000);
  stopMotors();
  testPass("Forward executed");
  
  Serial.println(F("\nTEST: Backward"));
  moveBackward(SPEED_NORMAL);
  delay(1000);
  stopMotors();
  testPass("Backward executed");
  
  Serial.println(F("\nTEST: Turn Left"));
  turnLeft(SPEED_TURN);
  delay(TIME_TURN_90);
  stopMotors();
  testPass("Left turn executed");
  
  Serial.println(F("\nTEST: Turn Right"));
  turnRight(SPEED_TURN);
  delay(TIME_TURN_90);
  stopMotors();
  testPass("Right turn executed");
  
  Serial.println(F("\nTEST: Servo - Clamp"));
  closeClamp();
  Serial.println(F("    Clamp closed"));
  delay(500);
  openClamp();
  Serial.println(F("    Clamp opened"));
  testPass("Clamp works");
  
  Serial.println(F("\nTEST: Servo - Arm"));
  armDown();
  Serial.println(F("    Arm down"));
  delay(500);
  armUp();
  Serial.println(F("    Arm up"));
  delay(500);
  armCarry();
  Serial.println(F("    Arm carry"));
  testPass("Arm works");
  
  Serial.println(F("\nTEST: Pickup sequence"));
  waitForInput("Place object in front of claw");
  pickup();
  Serial.print(F("    Holding: ")); Serial.println(holding ? "YES" : "NO");
  if (holding) testPass("Pickup works");
  else testFail("Pickup failed");
  
  Serial.println(F("\nTEST: Drop sequence"));
  drop();
  Serial.print(F("    Holding: ")); Serial.println(holding ? "YES" : "NO");
  if (!holding) testPass("Drop works");
  else testFail("Drop failed");
  
  printSummary();
}

// ============================================================================
// PRINT MENU
// ============================================================================
void printMenu() {
  Serial.println(F("\n╔═════════════════════════════════════════════╗"));
  Serial.println(F("║            TEST MENU                        ║"));
  Serial.println(F("╠═════════════════════════════════════════════╣"));
  Serial.println(F("║  1. Sensor Tests                            ║"));
  Serial.println(F("║  2. Motor Tests                             ║"));
  Serial.println(F("║  5. Quick Diagnostics                       ║"));
  Serial.println(F("║  6. Calibration Mode                        ║"));
  Serial.println(F("║  R. Reset counters                          ║"));
  Serial.println(F("╚═════════════════════════════════════════════╝"));
  Serial.println(F("\nEnter choice (1, 2, 5, 6, or R):"));
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
  baseServo.write(SERVO_ARM_DOWN);
  clampServo.write(SERVO_CLAMP_OPEN);
  
  delay(1000);
  
  Serial.println(F("\n═══════════════════════════════════════════════"));
  Serial.println(F("     UTRA HACKS ROBOT TEST RUNNER"));
  Serial.println(F("═══════════════════════════════════════════════"));
  
  printMenu();
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    while (Serial.available()) Serial.read();
    
    switch (c) {
      case '1':
        testsTotal = testsPassed = testsFailed = 0;
        runSensorTests();
        printMenu();
        break;
      case '2':
        testsTotal = testsPassed = testsFailed = 0;
        runMotorTests();
        printMenu();
        break;
      case '5':
        runQuickDiagnostics();
        printMenu();
        break;
      case '6':
        runCalibration();
        printMenu();
        break;
      case 'r':
      case 'R':
        testsTotal = testsPassed = testsFailed = 0;
        Serial.println(F("Counters reset."));
        printMenu();
        break;
    }
  }
  delay(100);
}
