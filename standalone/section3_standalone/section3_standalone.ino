/**
 * @file section3_standalone.ino
 * @brief SECTION 3: OBSTACLE COURSE - Standalone version
 * @details All code in one file - works on any computer
 * 
 * MISSION: Red line -> Pick box -> Avoid obstacles -> Drop at blue -> Return
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

#define SPEED_COMPENSATION  0.9  // Right motor is faster, reduce its speed

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
#define SPEED_TURN        120

#define DIST_OBSTACLE     15
#define DIST_WALL_HUG     10
#define DIST_BOX_PICKUP   5

#define COLOR_FREQ_MAX    150
#define COLOR_FREQ_BLACK  200
#define COLOR_MARGIN      20

#define SERVO_CLAMP_OPEN    90
#define SERVO_CLAMP_CLOSED  0
#define SERVO_ARM_DOWN      0
#define SERVO_ARM_CARRY     45

#define TIME_TURN_90      500
#define TIME_SERVO_MOVE   300

// ============================================================================
// ENUMS & STATE
// ============================================================================
enum Color { COLOR_NONE, COLOR_BLACK, COLOR_WHITE, COLOR_RED, COLOR_GREEN, COLOR_BLUE };
enum State {
  STATE_FIND_RED,
  STATE_FOLLOW_RED,
  STATE_APPROACH_BOX,
  STATE_PICKUP,
  STATE_TO_OBSTACLES,
  STATE_AVOID_OBS,
  STATE_FIND_BLUE,
  STATE_DROP,
  STATE_FIND_BLACK,
  STATE_RETURN_HOME,
  STATE_COMPLETE
};

// ============================================================================
// GLOBALS
// ============================================================================
Servo baseServo, clampServo;
State currentState = STATE_FIND_RED;
bool holding = false;
uint32_t stateStartTime = 0;
uint8_t obstacleCount = 0;

// ============================================================================
// SENSOR FUNCTIONS
// ============================================================================
float readDistance() {
  digitalWrite(PIN_ULTRA_TRIG, LOW); delayMicroseconds(2);
  digitalWrite(PIN_ULTRA_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  unsigned long d = pulseIn(PIN_ULTRA_ECHO, HIGH, 25000);
  return d == 0 ? 999.0 : (d * 0.034) / 2.0;
}

Color readColor() {
  digitalWrite(PIN_COLOR_S2, LOW); digitalWrite(PIN_COLOR_S3, LOW);
  delay(10);
  uint16_t r = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  digitalWrite(PIN_COLOR_S2, HIGH); digitalWrite(PIN_COLOR_S3, HIGH);
  delay(10);
  uint16_t g = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  digitalWrite(PIN_COLOR_S2, LOW); digitalWrite(PIN_COLOR_S3, HIGH);
  delay(10);
  uint16_t b = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  if (r == 0) r = 999; if (g == 0) g = 999; if (b == 0) b = 999;
  
  if (r > COLOR_FREQ_BLACK && g > COLOR_FREQ_BLACK && b > COLOR_FREQ_BLACK) return COLOR_BLACK;
  if (r < g - COLOR_MARGIN && r < b - COLOR_MARGIN && r < COLOR_FREQ_MAX) return COLOR_RED;
  if (g < r - COLOR_MARGIN && g < b - COLOR_MARGIN && g < COLOR_FREQ_MAX) return COLOR_GREEN;
  if (b < r - COLOR_MARGIN && b < g - COLOR_MARGIN && b < COLOR_FREQ_MAX) return COLOR_BLUE;
  return COLOR_NONE;
}

void readIR(bool& left, bool& right) {
  left = (digitalRead(PIN_IR_LEFT) == LOW);
  right = (digitalRead(PIN_IR_RIGHT) == LOW);
}

// ============================================================================
// MOTOR FUNCTIONS (Motor A = LEFT, Motor B = RIGHT)
// ============================================================================
void stopMotors() {
  analogWrite(PIN_MOTOR_ENA, 0); analogWrite(PIN_MOTOR_ENB, 0);
}

void moveForward(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, HIGH); digitalWrite(PIN_MOTOR_IN2, LOW);  // LEFT forward
  digitalWrite(PIN_MOTOR_IN3, HIGH); digitalWrite(PIN_MOTOR_IN4, LOW);  // RIGHT forward
  analogWrite(PIN_MOTOR_ENA, speed);                                    // LEFT
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION));    // RIGHT (reduced)
}

void turnLeft(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, LOW); digitalWrite(PIN_MOTOR_IN2, HIGH);  // LEFT backward
  digitalWrite(PIN_MOTOR_IN3, HIGH); digitalWrite(PIN_MOTOR_IN4, LOW);  // RIGHT forward
  analogWrite(PIN_MOTOR_ENA, speed);
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION));
}

void turnRight(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, HIGH); digitalWrite(PIN_MOTOR_IN2, LOW);  // LEFT forward
  digitalWrite(PIN_MOTOR_IN3, LOW); digitalWrite(PIN_MOTOR_IN4, HIGH);  // RIGHT backward
  analogWrite(PIN_MOTOR_ENA, speed);
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION));
}

void curveLeft(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, HIGH); digitalWrite(PIN_MOTOR_IN2, LOW);  // LEFT forward
  digitalWrite(PIN_MOTOR_IN3, HIGH); digitalWrite(PIN_MOTOR_IN4, LOW);  // RIGHT forward
  analogWrite(PIN_MOTOR_ENA, speed / 2);                                // LEFT half speed
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION));    // RIGHT full (reduced)
}

void curveRight(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, HIGH); digitalWrite(PIN_MOTOR_IN2, LOW);  // LEFT forward
  digitalWrite(PIN_MOTOR_IN3, HIGH); digitalWrite(PIN_MOTOR_IN4, LOW);  // RIGHT forward
  analogWrite(PIN_MOTOR_ENA, speed);                                    // LEFT full
  analogWrite(PIN_MOTOR_ENB, (uint8_t)((speed / 2) * SPEED_COMPENSATION)); // RIGHT half (reduced)
}

// ============================================================================
// SERVO FUNCTIONS
// ============================================================================
void pickup() {
  baseServo.write(SERVO_ARM_DOWN); delay(TIME_SERVO_MOVE + 200);
  clampServo.write(SERVO_CLAMP_CLOSED); delay(TIME_SERVO_MOVE);
  baseServo.write(SERVO_ARM_CARRY); delay(TIME_SERVO_MOVE);
  holding = true;
}

void drop() {
  baseServo.write(SERVO_ARM_DOWN); delay(TIME_SERVO_MOVE + 200);
  clampServo.write(SERVO_CLAMP_OPEN); delay(TIME_SERVO_MOVE);
  baseServo.write(SERVO_ARM_CARRY); delay(TIME_SERVO_MOVE);
  holding = false;
}

// ============================================================================
// OBSTACLE AVOIDANCE
// ============================================================================
void avoidObstacle() {
  Serial.println(F(">>> AVOIDING OBSTACLE <<<"));
  stopMotors(); delay(100);
  
  // Turn right
  turnRight(SPEED_TURN);
  delay(TIME_TURN_90);
  stopMotors(); delay(100);
  
  // Move forward
  moveForward(SPEED_NORMAL);
  delay(800);
  
  // Turn left
  turnLeft(SPEED_TURN);
  delay(TIME_TURN_90);
  stopMotors(); delay(100);
  
  // Wall hug
  for (int i = 0; i < 20; i++) {
    float d = readDistance();
    if (d < DIST_WALL_HUG - 3) curveRight(SPEED_NORMAL);
    else if (d < DIST_WALL_HUG + 5) moveForward(SPEED_NORMAL);
    else break;
    delay(50);
  }
  stopMotors();
  
  // Turn left
  turnLeft(SPEED_TURN);
  delay(TIME_TURN_90);
  stopMotors(); delay(100);
  
  // Clear
  moveForward(SPEED_NORMAL);
  delay(800);
  
  // Resume direction
  turnRight(SPEED_TURN);
  delay(TIME_TURN_90);
  stopMotors();
  
  obstacleCount++;
}

// ============================================================================
// LINE FOLLOWING
// ============================================================================
void followBlackLine() {
  bool left, right;
  readIR(left, right);
  if (left && right) moveForward(SPEED_NORMAL);
  else if (left && !right) curveLeft(SPEED_NORMAL);
  else if (!left && right) curveRight(SPEED_NORMAL);
  else moveForward(SPEED_SLOW);
}

void followRedLine() {
  Color c = readColor();
  if (c == COLOR_RED) moveForward(SPEED_NORMAL);
  else moveForward(SPEED_SLOW);
}

// ============================================================================
// STATE MACHINE
// ============================================================================
void transitionTo(State s) {
  currentState = s;
  stateStartTime = millis();
  Serial.print(F("STATE: ")); Serial.println(s);
}

void processState() {
  Color color = readColor();
  float dist = readDistance();
  
  switch (currentState) {
    case STATE_FIND_RED:
      turnLeft(SPEED_TURN);
      delay(TIME_TURN_90);
      stopMotors();
      if (readColor() == COLOR_RED) transitionTo(STATE_FOLLOW_RED);
      else { moveForward(SPEED_SLOW); delay(300); stopMotors(); }
      if (millis() - stateStartTime > 3000) transitionTo(STATE_FOLLOW_RED);
      break;
      
    case STATE_FOLLOW_RED:
      followRedLine();
      if (!holding && dist < DIST_BOX_PICKUP + 10 && dist > 0) {
        transitionTo(STATE_APPROACH_BOX);
      }
      if (holding && dist < DIST_OBSTACLE && dist > 0) {
        transitionTo(STATE_TO_OBSTACLES);
      }
      if (holding && obstacleCount >= 2 && color == COLOR_BLUE) {
        transitionTo(STATE_FIND_BLUE);
      }
      break;
      
    case STATE_APPROACH_BOX:
      if (dist <= DIST_BOX_PICKUP) { stopMotors(); transitionTo(STATE_PICKUP); }
      else moveForward(SPEED_SLOW);
      break;
      
    case STATE_PICKUP:
      pickup();
      transitionTo(STATE_TO_OBSTACLES);
      break;
      
    case STATE_TO_OBSTACLES:
      followRedLine();
      if (dist < DIST_OBSTACLE && dist > 0) {
        transitionTo(STATE_AVOID_OBS);
      }
      if (obstacleCount >= 2 && color == COLOR_BLUE) {
        transitionTo(STATE_FIND_BLUE);
      }
      break;
      
    case STATE_AVOID_OBS:
      avoidObstacle();
      if (obstacleCount >= 2) transitionTo(STATE_FIND_BLUE);
      else transitionTo(STATE_TO_OBSTACLES);
      break;
      
    case STATE_FIND_BLUE:
      if (color == COLOR_BLUE) {
        moveForward(SPEED_SLOW);
        delay(500);
        stopMotors();
        transitionTo(STATE_DROP);
      } else {
        followRedLine();
        if (millis() - stateStartTime > 5000) {
          // Search for blue
          turnLeft(SPEED_TURN); delay(200);
          stopMotors();
        }
      }
      break;
      
    case STATE_DROP:
      drop();
      transitionTo(STATE_FIND_BLACK);
      break;
      
    case STATE_FIND_BLACK:
      if (color == COLOR_BLACK) transitionTo(STATE_RETURN_HOME);
      else if (color == COLOR_RED) followRedLine();
      else { moveForward(SPEED_SLOW); delay(200); }
      if (millis() - stateStartTime > 5000) transitionTo(STATE_RETURN_HOME);
      break;
      
    case STATE_RETURN_HOME:
      followBlackLine();
      if (millis() - stateStartTime > 5000) {
        stopMotors();
        transitionTo(STATE_COMPLETE);
      }
      break;
      
    case STATE_COMPLETE:
      stopMotors();
      Serial.println(F("\n═══════════════════════════════════"));
      Serial.println(F("     COMPETITION COMPLETE!"));
      Serial.println(F("═══════════════════════════════════"));
      while (true) delay(1000);
      break;
  }
}

// ============================================================================
// SETUP & LOOP
// ============================================================================
void setup() {
  Serial.begin(9600);
  
  pinMode(PIN_COLOR_S0, OUTPUT); pinMode(PIN_COLOR_S1, OUTPUT);
  pinMode(PIN_COLOR_S2, OUTPUT); pinMode(PIN_COLOR_S3, OUTPUT);
  pinMode(PIN_COLOR_OUT, INPUT);
  digitalWrite(PIN_COLOR_S0, HIGH); digitalWrite(PIN_COLOR_S1, LOW);
  
  pinMode(PIN_ULTRA_TRIG, OUTPUT); pinMode(PIN_ULTRA_ECHO, INPUT);
  pinMode(PIN_IR_LEFT, INPUT); pinMode(PIN_IR_RIGHT, INPUT);
  
  pinMode(PIN_MOTOR_ENA, OUTPUT); pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT); pinMode(PIN_MOTOR_ENB, OUTPUT);
  pinMode(PIN_MOTOR_IN3, OUTPUT); pinMode(PIN_MOTOR_IN4, OUTPUT);
  
  baseServo.attach(PIN_SERVO_BASE);
  clampServo.attach(PIN_SERVO_CLAMP);
  baseServo.write(SERVO_ARM_CARRY);
  clampServo.write(SERVO_CLAMP_OPEN);
  
  stopMotors();
  delay(1000);
  
  Serial.println(F("=== SECTION 3: OBSTACLE COURSE ==="));
  transitionTo(STATE_FIND_RED);
}

void loop() {
  processState();
  delay(50);
}
