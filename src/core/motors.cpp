/**
 * @file motors.cpp
 * @brief Motor control abstraction layer implementation
 */

#include "motors.h"

// ============================================================================
// INITIALIZATION
// ============================================================================
void Motors::init() {
  // Set all motor pins as outputs
  pinMode(PIN_MOTOR_ENA, OUTPUT);
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_MOTOR_ENB, OUTPUT);
  pinMode(PIN_MOTOR_IN3, OUTPUT);
  pinMode(PIN_MOTOR_IN4, OUTPUT);
  
  // Start with motors stopped
  stop();
}

// ============================================================================
// BASIC MOVEMENTS - O(1)
// ============================================================================
void Motors::stop() {
  analogWrite(PIN_MOTOR_ENA, 0);
  analogWrite(PIN_MOTOR_ENB, 0);
  digitalWrite(PIN_MOTOR_IN1, LOW);
  digitalWrite(PIN_MOTOR_IN2, LOW);
  digitalWrite(PIN_MOTOR_IN3, LOW);
  digitalWrite(PIN_MOTOR_IN4, LOW);
}

void Motors::forward(uint8_t speed) {
  set(speed, DIR_FORWARD, speed, DIR_FORWARD);
}

void Motors::backward(uint8_t speed) {
  set(speed, DIR_BACKWARD, speed, DIR_BACKWARD);
}

void Motors::turnLeft(uint8_t speed) {
  // Pivot turn: left backward, right forward
  set(speed, DIR_BACKWARD, speed, DIR_FORWARD);
}

void Motors::turnRight(uint8_t speed) {
  // Pivot turn: left forward, right backward
  set(speed, DIR_FORWARD, speed, DIR_BACKWARD);
}

void Motors::curveLeft(uint8_t speed) {
  // Gentle curve: slow down left motor
  set(speed / 2, DIR_FORWARD, speed, DIR_FORWARD);
}

void Motors::curveRight(uint8_t speed) {
  // Gentle curve: slow down right motor
  set(speed, DIR_FORWARD, speed / 2, DIR_FORWARD);
}

// ============================================================================
// LOW-LEVEL CONTROL - O(1)
// ============================================================================
// Speed compensation: right motor is faster, reduce its speed by 10%
#define SPEED_COMP 0.9

void Motors::set(uint8_t leftSpeed, MotorDirection leftDir,
                 uint8_t rightSpeed, MotorDirection rightDir) {
  // Left motor (Motor A - ENA, IN1, IN2)
  setMotor(PIN_MOTOR_IN1, PIN_MOTOR_IN2, PIN_MOTOR_ENA, leftSpeed, leftDir);
  
  // Right motor (Motor B - ENB, IN3, IN4) with speed compensation
  setMotor(PIN_MOTOR_IN3, PIN_MOTOR_IN4, PIN_MOTOR_ENB, (uint8_t)(rightSpeed * SPEED_COMP), rightDir);
}

void Motors::setMotor(uint8_t in1, uint8_t in2, uint8_t en, 
                      uint8_t speed, MotorDirection dir) {
  switch (dir) {
    case DIR_FORWARD:
      digitalWrite(in1, HIGH);
      digitalWrite(in2, LOW);
      break;
    case DIR_BACKWARD:
      digitalWrite(in1, LOW);
      digitalWrite(in2, HIGH);
      break;
    case DIR_STOP:
    default:
      digitalWrite(in1, LOW);
      digitalWrite(in2, LOW);
      speed = 0;
      break;
  }
  analogWrite(en, speed);
}
