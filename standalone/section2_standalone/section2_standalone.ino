/**
 * @file section2_standalone.ino
 * @brief SECTION 2: TARGET SHOOTING - Standalone version
 * @details All code in one file - works on any computer
 * 
 * MISSION: Climb ramp -> Navigate to center -> Shoot ball -> Return
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

// ============================================================================
// CONSTANTS
// ============================================================================
#define SPEED_SLOW        100
#define SPEED_NORMAL      150
#define SPEED_FAST        200
#define SPEED_TURN        120

#define DIST_BALL         20
#define COLOR_FREQ_MAX    150
#define COLOR_FREQ_BLACK  200
#define COLOR_MARGIN      20

#define SERVO_ARM_UP      90
#define SERVO_ARM_DOWN    0

#define TIME_TURN_90      500

// ============================================================================
// ENUMS & STATE
// ============================================================================
enum Color { COLOR_NONE, COLOR_BLACK, COLOR_WHITE, COLOR_RED, COLOR_GREEN, COLOR_BLUE };
enum State {
  STATE_CLIMB_RAMP,
  STATE_ON_TARGET,
  STATE_NAV_BLUE,
  STATE_NAV_RED,
  STATE_NAV_GREEN,
  STATE_REACH_CENTER,
  STATE_FIND_BALL,
  STATE_SHOOT,
  STATE_RETURN,
  STATE_COMPLETE
};

// ============================================================================
// GLOBALS
// ============================================================================
Servo baseServo, clampServo;
State currentState = STATE_CLIMB_RAMP;
uint32_t stateStartTime = 0;
int8_t searchDir = 1;
uint8_t searchCount = 0;

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

// ============================================================================
// MOTOR FUNCTIONS (Motor A = LEFT, Motor B = RIGHT)
// ============================================================================
void stopMotors() {
  analogWrite(PIN_MOTOR_ENA, 0); analogWrite(PIN_MOTOR_ENB, 0);
}

void moveForward(uint8_t speed) {
  digitalWrite(PIN_MOTOR_IN1, HIGH); digitalWrite(PIN_MOTOR_IN2, LOW);  // LEFT forward
  digitalWrite(PIN_MOTOR_IN3, HIGH); digitalWrite(PIN_MOTOR_IN4, LOW);  // RIGHT forward
  analogWrite(PIN_MOTOR_ENA, speed);                                    // LEFT speed
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION));    // RIGHT speed (reduced)
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

// ============================================================================
// NAVIGATION TO CENTER
// ============================================================================
void navigateToCenter() {
  moveForward(SPEED_SLOW);
  delay(100);
  
  searchCount++;
  if (searchCount > 5) {
    if (searchDir > 0) turnRight(SPEED_TURN);
    else turnLeft(SPEED_TURN);
    delay(200);
    stopMotors();
    searchDir *= -1;
    searchCount = 0;
  }
}

// ============================================================================
// STATE MACHINE
// ============================================================================
void transitionTo(State s) {
  currentState = s;
  stateStartTime = millis();
  searchCount = 0;
  Serial.print(F("STATE: ")); Serial.println(s);
}

void processState() {
  Color color = readColor();
  float dist = readDistance();
  
  switch (currentState) {
    case STATE_CLIMB_RAMP:
      moveForward(SPEED_FAST);
      if (color != COLOR_NONE && color != COLOR_WHITE) {
        stopMotors();
        transitionTo(STATE_ON_TARGET);
      }
      if (millis() - stateStartTime > 5000) {
        stopMotors();
        transitionTo(STATE_ON_TARGET);
      }
      break;
      
    case STATE_ON_TARGET:
      if (color == COLOR_BLUE) transitionTo(STATE_NAV_BLUE);
      else if (color == COLOR_RED) transitionTo(STATE_NAV_RED);
      else if (color == COLOR_GREEN) transitionTo(STATE_NAV_GREEN);
      else if (color == COLOR_BLACK) transitionTo(STATE_REACH_CENTER);
      else { moveForward(SPEED_SLOW); delay(200); stopMotors(); }
      break;
      
    case STATE_NAV_BLUE:
      navigateToCenter();
      color = readColor();
      if (color == COLOR_RED) transitionTo(STATE_NAV_RED);
      else if (color == COLOR_GREEN) transitionTo(STATE_NAV_GREEN);
      else if (color == COLOR_BLACK) transitionTo(STATE_REACH_CENTER);
      break;
      
    case STATE_NAV_RED:
      navigateToCenter();
      color = readColor();
      if (color == COLOR_GREEN) transitionTo(STATE_NAV_GREEN);
      else if (color == COLOR_BLACK) transitionTo(STATE_REACH_CENTER);
      break;
      
    case STATE_NAV_GREEN:
      navigateToCenter();
      color = readColor();
      if (color == COLOR_BLACK) transitionTo(STATE_REACH_CENTER);
      break;
      
    case STATE_REACH_CENTER:
      stopMotors();
      Serial.println(F(">>> CENTER REACHED <<<"));
      delay(500);
      transitionTo(STATE_FIND_BALL);
      break;
      
    case STATE_FIND_BALL:
      if (dist < DIST_BALL && dist > 0) {
        transitionTo(STATE_SHOOT);
      } else {
        moveForward(SPEED_SLOW);
        delay(200);
        stopMotors();
        if (millis() - stateStartTime > 3000) transitionTo(STATE_SHOOT);
      }
      break;
      
    case STATE_SHOOT:
      Serial.println(F(">>> SHOOTING BALL <<<"));
      baseServo.write(SERVO_ARM_DOWN);
      delay(300);
      moveForward(SPEED_FAST);
      delay(400);
      stopMotors();
      baseServo.write(SERVO_ARM_UP);
      delay(150);
      Serial.println(F(">>> BALL LAUNCHED <<<"));
      delay(1000);
      transitionTo(STATE_RETURN);
      break;
      
    case STATE_RETURN:
      turnRight(SPEED_TURN);
      delay(TIME_TURN_90 * 2);
      stopMotors();
      moveForward(SPEED_NORMAL);
      delay(4000);
      stopMotors();
      transitionTo(STATE_COMPLETE);
      break;
      
    case STATE_COMPLETE:
      stopMotors();
      Serial.println(F("\n=== SECTION 2 COMPLETE ==="));
      Serial.println(F("Upload section3_standalone.ino"));
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
  
  pinMode(PIN_MOTOR_ENA, OUTPUT); pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT); pinMode(PIN_MOTOR_ENB, OUTPUT);
  pinMode(PIN_MOTOR_IN3, OUTPUT); pinMode(PIN_MOTOR_IN4, OUTPUT);
  
  baseServo.attach(PIN_SERVO_BASE);
  clampServo.attach(PIN_SERVO_CLAMP);
  baseServo.write(SERVO_ARM_DOWN);
  
  stopMotors();
  delay(1000);
  
  Serial.println(F("=== SECTION 2: TARGET SHOOTING ==="));
  transitionTo(STATE_CLIMB_RAMP);
}

void loop() {
  processState();
  delay(50);
}
