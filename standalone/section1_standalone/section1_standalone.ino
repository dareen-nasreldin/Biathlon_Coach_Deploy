/**
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║                      SECTION 1: START SECTION                             ║
 * ║                                                                           ║
 * ║  MISSION: Follow black line → Pick up box → Take green path →            ║
 * ║           Drop box at blue zone → Go to re-upload point                   ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 * 
 * HOW THIS CODE WORKS:
 * ====================
 * This code uses a "State Machine" - the robot is always in one specific STATE,
 * and based on what the sensors detect, it transitions to the next STATE.
 * 
 * Think of it like a flowchart:
 * 
 *   [START]
 *      ↓
 *   [FOLLOW BLACK LINE] ──(box detected)──→ [APPROACH BOX]
 *      ↓                                         ↓
 *      ↓                                    [PICKUP BOX]
 *      ↓                                         ↓
 *   [FIND INTERSECTION] ←────────────────────────┘
 *      ↓
 *   (sees green or red line)
 *      ↓
 *   [SELECT GREEN PATH] ──→ [FOLLOW GREEN LINE]
 *                                  ↓
 *                           (sees blue zone)
 *                                  ↓
 *                           [DROP BOX]
 *                                  ↓
 *                           [GO TO RE-UPLOAD]
 *                                  ↓
 *                           [COMPLETE!]
 * 
 * COMPATIBLE WITH: Arduino UNO R4 Minima (Pin 13 changed to Pin 4)
 */

#include <Servo.h>  // Library to control servo motors

// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                           PIN DEFINITIONS                                  ║
// ║  These define which Arduino pins connect to which components.             ║
// ║  If you wire something to a different pin, change it here.                ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

// --- COLOR SENSOR (TCS3200) ---
// The color sensor shines light on a surface and measures how much
// red, green, and blue light reflects back. This tells us the color.
#define PIN_COLOR_S0      2   // Frequency scaling selector bit 0
#define PIN_COLOR_S1      3   // Frequency scaling selector bit 1
#define PIN_COLOR_S2      4   // Photodiode selector bit 0 (selects R/G/B)
#define PIN_COLOR_S3      5   // Photodiode selector bit 1 (selects R/G/B)
#define PIN_COLOR_OUT     6   // Output frequency (we measure pulse width)

// --- MOTOR DRIVER (L298N) ---
// The motor driver takes LOW POWER signals from Arduino and converts them
// to HIGH POWER signals that can spin the motors.
// 
// How it works:
//   - ENA/ENB: Speed control (0-255 PWM). Higher = faster.
//   - IN1/IN2: Direction for Motor A. IN1=HIGH,IN2=LOW = forward
//   - IN3/IN4: Direction for Motor B. IN3=HIGH,IN4=LOW = forward
//
// Motor A = LEFT wheel, Motor B = RIGHT wheel
#define PIN_MOTOR_ENA     9   // Speed control for Motor A / LEFT (MUST be PWM pin)
#define PIN_MOTOR_IN1     8   // Direction pin 1 for Motor A / LEFT
#define PIN_MOTOR_IN2     7   // Direction pin 2 for Motor A / LEFT
#define PIN_MOTOR_ENB     10  // Speed control for Motor B / RIGHT (MUST be PWM pin)
#define PIN_MOTOR_IN3     11  // Direction pin 1 for Motor B / RIGHT
#define PIN_MOTOR_IN4     12  // Direction pin 2 for Motor B / RIGHT

// Speed compensation: Right motor is faster, reduce its speed
#define SPEED_COMPENSATION  0.9  // Multiply right motor speed by this

// --- SERVOS ---
// Servos are motors that rotate to a specific angle (0-180 degrees).
// We use two: one to lift/lower the arm, one to open/close the claw.
#define PIN_SERVO_BASE    A5  // Arm servo (up/down movement)
#define PIN_SERVO_CLAMP   A4  // Claw servo (open/close)

// --- ULTRASONIC SENSOR (HC-SR04) ---
// Measures distance by sending out a sound pulse and timing how long
// it takes to bounce back. Speed of sound = 343 m/s = 0.034 cm/µs
#define PIN_ULTRA_TRIG    A0  // Trigger: we send a pulse OUT
#define PIN_ULTRA_ECHO    A1  // Echo: we receive the pulse back

// --- IR LINE SENSORS ---
// These point at the ground and detect if they're over a BLACK line.
// Return LOW (0) when over black, HIGH (1) when over white.
#define PIN_IR_LEFT       A2  // Left IR sensor
#define PIN_IR_RIGHT      A3  // Right IR sensor


// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                              CONSTANTS                                     ║
// ║  Tunable values. Adjust these to calibrate robot behavior.                ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

// --- MOTOR SPEEDS (0-255) ---
// PWM value sent to motor driver. Higher = faster. Max = 255.
#define SPEED_SLOW        100  // Careful/precise movements
#define SPEED_NORMAL      150  // Normal driving speed
#define SPEED_TURN        120  // Speed when turning

// --- DISTANCE THRESHOLDS (centimeters) ---
#define DIST_BOX_PICKUP   5    // How close to get before grabbing box

// --- COLOR SENSOR THRESHOLDS ---
// The color sensor returns "frequency" values. LOWER frequency = STRONGER color.
// Black surfaces absorb light, so ALL frequencies are HIGH (weak reflection).
// Colored surfaces reflect their color strongly (LOW frequency for that color).
#define COLOR_FREQ_MAX    150  // Maximum frequency to consider a valid color
#define COLOR_FREQ_BLACK  200  // All colors above this = black surface
#define COLOR_MARGIN      20   // Minimum difference between colors to distinguish

// --- SERVO ANGLES (degrees) ---
#define SERVO_CLAMP_OPEN    90   // Claw fully open
#define SERVO_CLAMP_CLOSED  0    // Claw fully closed (gripping)
#define SERVO_ARM_DOWN      0    // Arm lowered (to pick up)
#define SERVO_ARM_CARRY     45   // Arm raised (carrying position)

// --- TIMING (milliseconds) ---
#define TIME_TURN_90      500   // How long to turn for ~90 degrees
#define TIME_SERVO_MOVE   300   // Time to wait for servo to reach position


// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                           DATA TYPES                                       ║
// ║  Custom types to make the code more readable.                             ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

/**
 * Color - Represents detected colors from the color sensor.
 * Used to identify: black line, green path, red path, blue drop zone.
 */
enum Color { 
  COLOR_NONE,    // No clear color detected
  COLOR_BLACK,   // Black surface (line or target center)
  COLOR_WHITE,   // White surface (floor)
  COLOR_RED,     // Red path (we DON'T want this one)
  COLOR_GREEN,   // Green path (we WANT this one!)
  COLOR_BLUE     // Blue zone (drop box here)
};

/**
 * State - The robot's current task/behavior.
 * The robot is always in ONE state. Based on sensor input,
 * it transitions to the next state.
 */
enum State {
  STATE_FOLLOW_BLACK,      // Following the initial black line
  STATE_APPROACH_BOX,      // Moving toward detected box
  STATE_PICKUP,            // Executing pickup sequence
  STATE_FIND_INTERSECTION, // Following black line to find green/red paths
  STATE_SELECT_GREEN,      // At intersection, choosing the green path
  STATE_FOLLOW_GREEN,      // Following the green line
  STATE_APPROACH_BLUE,     // Moving into the blue drop zone
  STATE_DROP,              // Executing drop sequence
  STATE_TO_REUPLOAD,       // Heading to re-upload point
  STATE_COMPLETE           // Section 1 finished!
};


// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                          GLOBAL VARIABLES                                  ║
// ║  Variables that persist throughout the program.                           ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

Servo baseServo;              // Servo object for arm (up/down)
Servo clampServo;             // Servo object for claw (open/close)
State currentState;           // What the robot is currently doing
bool holding = false;         // Is the robot holding a box?
uint32_t stateStartTime = 0;  // When did we enter the current state?


// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                         SENSOR FUNCTIONS                                   ║
// ║  Functions to read data from sensors.                                     ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

/**
 * readDistance() - Measure distance using ultrasonic sensor.
 * 
 * HOW IT WORKS:
 * 1. Send a 10 microsecond pulse on TRIG pin
 * 2. Sound wave travels out, hits object, bounces back
 * 3. ECHO pin goes HIGH while waiting for echo
 * 4. Measure how long ECHO was HIGH (in microseconds)
 * 5. Calculate distance: distance = (time × speed_of_sound) / 2
 *    (divide by 2 because sound travels TO object AND back)
 * 
 * RETURNS: Distance in centimeters (or 999 if no object detected)
 */
float readDistance() {
  // Ensure trigger is LOW before starting
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  delayMicroseconds(2);
  
  // Send 10µs pulse to trigger the sensor
  digitalWrite(PIN_ULTRA_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_ULTRA_TRIG, LOW);
  
  // Measure how long the echo pulse is HIGH
  // Timeout after 25000µs (~4.25m max distance)
  unsigned long duration = pulseIn(PIN_ULTRA_ECHO, HIGH, 25000);
  
  // If timeout (duration=0), return 999 to indicate "no object"
  if (duration == 0) {
    return 999.0;
  }
  
  // Convert time to distance
  // Speed of sound = 343 m/s = 0.034 cm/µs
  // Distance = (duration × 0.034) / 2
  return (duration * 0.034) / 2.0;
}

/**
 * readColor() - Detect surface color using TCS3200 color sensor.
 * 
 * HOW IT WORKS:
 * The TCS3200 has an 8x8 array of photodiodes with color filters.
 * We select which color filter to read using S2 and S3 pins:
 *   S2=LOW,  S3=LOW  → Read RED photodiodes
 *   S2=HIGH, S3=HIGH → Read GREEN photodiodes
 *   S2=LOW,  S3=HIGH → Read BLUE photodiodes
 * 
 * The sensor outputs a square wave. The FREQUENCY of this wave
 * indicates how much of that color is detected.
 * LOWER frequency = MORE of that color detected.
 * 
 * We use pulseIn() to measure the pulse width (inverse of frequency).
 * HIGHER pulse width = LOWER frequency = MORE color detected.
 * 
 * RETURNS: Detected Color enum value
 */
Color readColor() {
  // Read RED value (S2=LOW, S3=LOW)
  digitalWrite(PIN_COLOR_S2, LOW);
  digitalWrite(PIN_COLOR_S3, LOW);
  delay(10);  // Wait for sensor to stabilize
  uint16_t r = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  // Read GREEN value (S2=HIGH, S3=HIGH)
  digitalWrite(PIN_COLOR_S2, HIGH);
  digitalWrite(PIN_COLOR_S3, HIGH);
  delay(10);
  uint16_t g = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  // Read BLUE value (S2=LOW, S3=HIGH)
  digitalWrite(PIN_COLOR_S2, LOW);
  digitalWrite(PIN_COLOR_S3, HIGH);
  delay(10);
  uint16_t b = pulseIn(PIN_COLOR_OUT, LOW, 40000);
  
  // Handle timeout (pulseIn returns 0 if no pulse detected)
  if (r == 0) r = 999;
  if (g == 0) g = 999;
  if (b == 0) b = 999;
  
  // --- COLOR DETECTION LOGIC ---
  // Remember: HIGHER values = WEAKER color (less light reflected)
  
  // BLACK: All colors reflect weakly (high values)
  // Black absorbs most light, so all frequencies are high
  if (r > COLOR_FREQ_BLACK && g > COLOR_FREQ_BLACK && b > COLOR_FREQ_BLACK) {
    return COLOR_BLACK;
  }
  
  // RED: Red value is significantly lower than others
  // Red surface reflects red light strongly (low r value)
  if (r < g - COLOR_MARGIN && r < b - COLOR_MARGIN && r < COLOR_FREQ_MAX) {
    return COLOR_RED;
  }
  
  // GREEN: Green value is significantly lower than others
  // Green surface reflects green light strongly (low g value)
  if (g < r - COLOR_MARGIN && g < b - COLOR_MARGIN && g < COLOR_FREQ_MAX) {
    return COLOR_GREEN;
  }
  
  // BLUE: Blue value is significantly lower than others
  // Blue surface reflects blue light strongly (low b value)
  if (b < r - COLOR_MARGIN && b < g - COLOR_MARGIN && b < COLOR_FREQ_MAX) {
    return COLOR_BLUE;
  }
  
  // No clear color detected
  return COLOR_NONE;
}

/**
 * readIR() - Read IR line sensors.
 * 
 * HOW IT WORKS:
 * IR sensors emit infrared light and measure reflection.
 * Black surfaces ABSORB infrared → sensor returns LOW (0)
 * White surfaces REFLECT infrared → sensor returns HIGH (1)
 * 
 * We set left/right to TRUE if that sensor is over the line.
 */
void readIR(bool& left, bool& right) {
  // LOW means sensor is over BLACK line
  left = (digitalRead(PIN_IR_LEFT) == LOW);
  right = (digitalRead(PIN_IR_RIGHT) == LOW);
}


// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                          MOTOR FUNCTIONS                                   ║
// ║  Functions to control the robot's movement.                               ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

/**
 * stopMotors() - Immediately stop both motors.
 * Sets PWM to 0 (no power to motors).
 */
void stopMotors() {
  analogWrite(PIN_MOTOR_ENA, 0);  // Stop Motor A (LEFT)
  analogWrite(PIN_MOTOR_ENB, 0);  // Stop Motor B (RIGHT)
}

/**
 * moveForward() - Drive both motors forward.
 * 
 * HOW MOTOR DIRECTION WORKS:
 * Motor A (LEFT):  IN1=HIGH, IN2=LOW → Forward
 * Motor B (RIGHT): IN3=HIGH, IN4=LOW → Forward
 * 
 * The speed (0-255) is sent to ENA and ENB as PWM signal.
 * PWM (Pulse Width Modulation) rapidly switches power on/off.
 * Higher duty cycle = more average power = faster motor.
 * 
 * Speed compensation applied to right motor (it's faster).
 */
void moveForward(uint8_t speed) {
  // Set direction: forward
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  
  // Set speed with compensation (right motor is faster)
  analogWrite(PIN_MOTOR_ENA, speed);                              // LEFT motor
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION)); // RIGHT motor (reduced)
}

/**
 * curveLeft() - Move forward while curving to the left.
 * 
 * HOW CURVES WORK:
 * To curve left, slow down the LEFT motor (Motor A).
 * Right motor goes full speed, left motor goes half speed.
 * The robot arcs to the left while still moving forward.
 * 
 * Used for gentle line-following corrections.
 */
void curveLeft(uint8_t speed) {
  // Direction: both forward
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  
  // Left motor half speed, right motor full speed
  analogWrite(PIN_MOTOR_ENA, speed / 2);                          // LEFT: half
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION)); // RIGHT: full (compensated)
}

/**
 * curveRight() - Move forward while curving to the right.
 * Opposite of curveLeft: slow down the RIGHT motor.
 */
void curveRight(uint8_t speed) {
  // Direction: both forward
  digitalWrite(PIN_MOTOR_IN1, HIGH);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, HIGH);
  digitalWrite(PIN_MOTOR_IN4, LOW);
  
  // Left motor full speed, right motor half speed
  analogWrite(PIN_MOTOR_ENA, speed);                                      // LEFT: full
  analogWrite(PIN_MOTOR_ENB, (uint8_t)((speed / 2) * SPEED_COMPENSATION)); // RIGHT: half (compensated)
}

/**
 * turnLeft() - Pivot turn to the left (rotate in place).
 * 
 * HOW PIVOT TURNS WORK:
 * Right motor goes FORWARD, left motor goes BACKWARD.
 * The robot spins around its center point.
 * Used for sharp turns (like at intersections).
 */
void turnLeft(uint8_t speed) {
  // Left motor backward, right motor forward
  digitalWrite(PIN_MOTOR_IN1, LOW);   // LEFT backward
  digitalWrite(PIN_MOTOR_IN2, HIGH);
  digitalWrite(PIN_MOTOR_IN3, HIGH);  // RIGHT forward
  digitalWrite(PIN_MOTOR_IN4, LOW);
  
  analogWrite(PIN_MOTOR_ENA, speed);                              // LEFT
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION)); // RIGHT (compensated)
}

/**
 * turnRight() - Pivot turn to the right (rotate in place).
 * Opposite of turnLeft.
 */
void turnRight(uint8_t speed) {
  // Left motor forward, right motor backward
  digitalWrite(PIN_MOTOR_IN1, HIGH);  // LEFT forward
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);   // RIGHT backward
  digitalWrite(PIN_MOTOR_IN4, HIGH);
  
  analogWrite(PIN_MOTOR_ENA, speed);                              // LEFT
  analogWrite(PIN_MOTOR_ENB, (uint8_t)(speed * SPEED_COMPENSATION)); // RIGHT (compensated)
}


// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                          SERVO FUNCTIONS                                   ║
// ║  Functions to control the claw/arm mechanism.                             ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

/**
 * pickup() - Execute the full box pickup sequence.
 * 
 * SEQUENCE:
 * 1. Lower arm to ground level
 * 2. Close claw to grip the box
 * 3. Raise arm to carrying position
 */
void pickup() {
  // Step 1: Lower arm
  baseServo.write(SERVO_ARM_DOWN);
  delay(TIME_SERVO_MOVE + 200);  // Extra time for larger movement
  
  // Step 2: Close claw
  clampServo.write(SERVO_CLAMP_CLOSED);
  delay(TIME_SERVO_MOVE);
  
  // Step 3: Raise arm
  baseServo.write(SERVO_ARM_CARRY);
  delay(TIME_SERVO_MOVE);
  
  // Update state
  holding = true;
}

/**
 * drop() - Execute the full box drop sequence.
 * 
 * SEQUENCE:
 * 1. Lower arm
 * 2. Open claw to release box
 * 3. Raise arm back up
 */
void drop() {
  // Step 1: Lower arm
  baseServo.write(SERVO_ARM_DOWN);
  delay(TIME_SERVO_MOVE + 200);
  
  // Step 2: Open claw
  clampServo.write(SERVO_CLAMP_OPEN);
  delay(TIME_SERVO_MOVE);
  
  // Step 3: Raise arm
  baseServo.write(SERVO_ARM_CARRY);
  delay(TIME_SERVO_MOVE);
  
  // Update state
  holding = false;
}


// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                        LINE FOLLOWING FUNCTIONS                            ║
// ║  Functions for autonomous line-following behavior.                        ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

/**
 * followBlackLine() - Follow a black line using the COLOR sensor.
 * 
 * HOW IT WORKS:
 * Single color sensor detects if we're on the black line.
 * 
 * - BLACK detected → Go straight forward
 * - Not BLACK      → Slowly search by curving (alternating direction)
 * 
 * Since we only have ONE sensor, we can't tell which side we drifted.
 * We use a simple strategy: alternate search direction each time we lose the line.
 */
static bool searchDirection = false;  // Alternates left/right when searching

void followBlackLine() {
  Color c = readColor();
  
  if (c == COLOR_BLACK) {
    // On the black line - go straight!
    moveForward(SPEED_NORMAL);
    // Reset search direction for next time we lose line
  }
  else {
    // Lost the line - search for it
    // Alternate between curving left and right
    if (searchDirection) {
      curveLeft(SPEED_SLOW);
    } else {
      curveRight(SPEED_SLOW);
    }
    // Toggle direction for next search attempt
    searchDirection = !searchDirection;
  }
}

/**
 * followGreenLine() - Follow a green line using color sensor.
 * 
 * Unlike black line following (which uses IR), colored lines
 * use the color sensor. This is simpler - just check if we're
 * on green, and if so, go forward. If not, go slower.
 */
void followGreenLine() {
  Color c = readColor();
  
  if (c == COLOR_GREEN) {
    // On the green line - full speed ahead!
    moveForward(SPEED_NORMAL);
  }
  else {
    // Not on green - go slower to avoid overshooting
    moveForward(SPEED_SLOW);
  }
}


// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                           STATE MACHINE                                    ║
// ║  The brain of the robot. Decides what to do based on current state.       ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

/**
 * transitionTo() - Change to a new state.
 * Records when we entered the state (for timeouts).
 * Prints state to Serial Monitor for debugging.
 */
void transitionTo(State newState) {
  currentState = newState;
  stateStartTime = millis();  // Record when we entered this state
  
  // Print state name for debugging
  Serial.print(F("STATE: "));
  Serial.println(newState);
}

/**
 * processState() - Main decision-making function.
 * 
 * This is called every loop iteration. It:
 * 1. Reads sensors
 * 2. Checks what state we're in
 * 3. Performs actions for that state
 * 4. Checks if conditions are met to transition to next state
 * 
 * This is the HEART of the robot's autonomous behavior.
 */
void processState() {
  // Read sensors (used by multiple states)
  float dist = readDistance();
  Color color = readColor();
  
  // Act based on current state
  switch (currentState) {
    
    // ─────────────────────────────────────────────────────────────────────────
    // STATE: Follow the initial black line, looking for box
    // ─────────────────────────────────────────────────────────────────────────
    case STATE_FOLLOW_BLACK:
      // Keep following the black line
      followBlackLine();
      
      // Check if we detect a box ahead (distance < 15cm)
      if (dist < DIST_BOX_PICKUP + 10 && dist > 0) {
        transitionTo(STATE_APPROACH_BOX);
      }
      break;
    
    // ─────────────────────────────────────────────────────────────────────────
    // STATE: Slowly approach the box until close enough to grab
    // ─────────────────────────────────────────────────────────────────────────
    case STATE_APPROACH_BOX:
      if (dist <= DIST_BOX_PICKUP) {
        // Close enough! Stop and pick up
        stopMotors();
        transitionTo(STATE_PICKUP);
      }
      else {
        // Not close enough, keep moving slowly
        moveForward(SPEED_SLOW);
      }
      break;
    
    // ─────────────────────────────────────────────────────────────────────────
    // STATE: Execute the pickup sequence
    // ─────────────────────────────────────────────────────────────────────────
    case STATE_PICKUP:
      pickup();  // This function handles the full sequence
      transitionTo(STATE_FIND_INTERSECTION);
      break;
    
    // ─────────────────────────────────────────────────────────────────────────
    // STATE: Continue on black line until we reach green/red intersection
    // ─────────────────────────────────────────────────────────────────────────
    case STATE_FIND_INTERSECTION:
      followBlackLine();
      
      // Check if color sensor sees green or red (intersection!)
      if (color == COLOR_GREEN || color == COLOR_RED) {
        stopMotors();
        transitionTo(STATE_SELECT_GREEN);
      }
      break;
    
    // ─────────────────────────────────────────────────────────────────────────
    // STATE: We're at the intersection - find and select the GREEN path
    // ─────────────────────────────────────────────────────────────────────────
    case STATE_SELECT_GREEN:
      // Already on green? Great, start following it!
      if (color == COLOR_GREEN) {
        transitionTo(STATE_FOLLOW_GREEN);
        break;
      }
      
      // Not on green - search for it
      // First, try turning LEFT (based on competition map, green is left)
      turnLeft(SPEED_TURN);
      delay(300);
      stopMotors();
      
      // Check if we found green
      if (readColor() == COLOR_GREEN) {
        transitionTo(STATE_FOLLOW_GREEN);
      }
      else {
        // Green wasn't on left - try right
        turnRight(SPEED_TURN);
        delay(600);  // Turn past center to the right
        stopMotors();
        transitionTo(STATE_FOLLOW_GREEN);
      }
      break;
    
    // ─────────────────────────────────────────────────────────────────────────
    // STATE: Follow the green line, looking for blue drop zone
    // ─────────────────────────────────────────────────────────────────────────
    case STATE_FOLLOW_GREEN:
      followGreenLine();
      
      // Check if we've reached the blue drop zone
      if (color == COLOR_BLUE) {
        transitionTo(STATE_APPROACH_BLUE);
      }
      break;
    
    // ─────────────────────────────────────────────────────────────────────────
    // STATE: Move fully into the blue zone before dropping
    // ─────────────────────────────────────────────────────────────────────────
    case STATE_APPROACH_BLUE:
      // Move forward a bit to get fully into blue zone
      moveForward(SPEED_SLOW);
      delay(500);
      stopMotors();
      transitionTo(STATE_DROP);
      break;
    
    // ─────────────────────────────────────────────────────────────────────────
    // STATE: Drop the box
    // ─────────────────────────────────────────────────────────────────────────
    case STATE_DROP:
      drop();  // This function handles the full sequence
      transitionTo(STATE_TO_REUPLOAD);
      break;
    
    // ─────────────────────────────────────────────────────────────────────────
    // STATE: Continue to re-upload point
    // ─────────────────────────────────────────────────────────────────────────
    case STATE_TO_REUPLOAD:
      followGreenLine();
      
      // After 3 seconds, assume we've reached the re-upload point
      if (millis() - stateStartTime > 3000) {
        stopMotors();
        transitionTo(STATE_COMPLETE);
      }
      break;
    
    // ─────────────────────────────────────────────────────────────────────────
    // STATE: Section 1 complete!
    // ─────────────────────────────────────────────────────────────────────────
    case STATE_COMPLETE:
      stopMotors();
      
      Serial.println(F("\n============================="));
      Serial.println(F("   SECTION 1 COMPLETE!"));
      Serial.println(F("============================="));
      Serial.println(F("Now upload section2_standalone.ino"));
      
      // Halt here forever (until re-upload)
      while (true) {
        delay(1000);
      }
      break;
  }
}


// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║                          SETUP & MAIN LOOP                                 ║
// ║  setup() runs once at power-on. loop() runs repeatedly forever.           ║
// ╚═══════════════════════════════════════════════════════════════════════════╝

/**
 * setup() - Initialize everything.
 * This runs ONCE when the Arduino is powered on or reset.
 */
void setup() {
  // Start Serial communication for debugging
  Serial.begin(9600);
  
  // --- Initialize Color Sensor Pins ---
  pinMode(PIN_COLOR_S0, OUTPUT);
  pinMode(PIN_COLOR_S1, OUTPUT);
  pinMode(PIN_COLOR_S2, OUTPUT);
  pinMode(PIN_COLOR_S3, OUTPUT);
  pinMode(PIN_COLOR_OUT, INPUT);
  
  // Set color sensor to 20% frequency scaling
  // This gives a good balance of speed and accuracy
  digitalWrite(PIN_COLOR_S0, HIGH);
  digitalWrite(PIN_COLOR_S1, LOW);
  
  // --- Initialize Ultrasonic Sensor Pins ---
  pinMode(PIN_ULTRA_TRIG, OUTPUT);
  pinMode(PIN_ULTRA_ECHO, INPUT);
  
  // --- Initialize IR Sensor Pins ---
  pinMode(PIN_IR_LEFT, INPUT);
  pinMode(PIN_IR_RIGHT, INPUT);
  
  // --- Initialize Motor Pins ---
  pinMode(PIN_MOTOR_ENA, OUTPUT);
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_MOTOR_ENB, OUTPUT);
  pinMode(PIN_MOTOR_IN3, OUTPUT);
  pinMode(PIN_MOTOR_IN4, OUTPUT);
  
  // --- Initialize Servos ---
  baseServo.attach(PIN_SERVO_BASE);
  clampServo.attach(PIN_SERVO_CLAMP);
  
  // Set initial positions: arm down, claw open
  baseServo.write(SERVO_ARM_DOWN);
  clampServo.write(SERVO_CLAMP_OPEN);
  
  // Make sure motors are stopped
  stopMotors();
  
  // Wait a moment for everything to stabilize
  delay(1000);
  
  // --- Start the mission! ---
  Serial.println(F("============================="));
  Serial.println(F("   SECTION 1: START"));
  Serial.println(F("============================="));
  Serial.println(F("Mission: Black line -> Box -> Green -> Blue"));
  Serial.println();
  
  // Begin in the first state
  transitionTo(STATE_FOLLOW_BLACK);
}

/**
 * loop() - Main program loop.
 * This runs REPEATEDLY forever after setup() completes.
 * 
 * Each iteration:
 * 1. Process current state (read sensors, make decisions, act)
 * 2. Wait a short time (50ms = 20 times per second)
 */
void loop() {
  processState();  // Do the state machine stuff
  delay(50);       // Small delay to prevent overwhelming sensors
}
