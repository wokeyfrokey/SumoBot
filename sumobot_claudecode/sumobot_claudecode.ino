// ============================================================
//  SUMO BOT - Arduino Nano
//  Hardware:
//    - Arduino Nano Expansion Board
//    - XY-160D Motor Driver
//    - E3Z-D62 Opponent Sensors (x5): Left, NE, North, NW, West, South
//    - TCRT5000 Line Sensors (x2): frontIR1, frontIR2
//    - Back IR Sensor: backIR
//    - 3S LiPo Battery
// ============================================================

// --- Motor Driver Pins (XY-160D) ---
const int ENA = 5;   // Left  motor PWM speed
const int IN1 = 7;   // Left  motor direction A
const int IN2 = 8;   // Left  motor direction B
const int ENB = 6;   // Right motor PWM speed
const int IN3 = 9;   // Right motor direction A
const int IN4 = 10;  // Right motor direction B

// --- E3Z-D62 Opponent Sensors (LOW = opponent detected) ---
const int LS_E  = A0;  // East  (Right side)
const int LS_NE = A1;  // Northeast (Front-Right)
const int LS_N  = A2;  // North (Front Center)
const int LS_NW = A3;  // Northwest (Front-Left)
const int LS_W  = A4;  // West  (Left side)
const int LS_S  = A5;  // South (Rear)

// --- TCRT5000 Line / Edge Sensors (HIGH = white/edge detected) ---
const int backIR  = 2;   // Rear edge sensor
const int frontIR1 = 3;  // Front-left edge sensor
const int frontIR2 = 11; // Front-right edge sensor

// --- Motor Speed Constants ---
const int FULL_SPEED   = 255;
const int TURN_SPEED   = 200;
const int SEARCH_SPEED = 180;
const int BACK_SPEED   = 220;

// --- Timing ---
const unsigned long START_DELAY_MS   = 5000;  // 5-second mandatory start delay
const unsigned long STRATEGY_TIME_MS = 1800;  // ~half-moon sweep duration
const unsigned long BACK_AVOID_MS    = 280;   // reverse duration on edge detect
const unsigned long TURN_AVOID_MS    = 350;   // turn duration after reversing

// ============================================================
//  LOW-LEVEL MOTOR HELPERS
// ============================================================

void motorLeft(int speed) {
  // Positive = forward, Negative = backward
  if (speed >= 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    speed = -speed;
  }
  analogWrite(ENA, constrain(speed, 0, 255));
}

void motorRight(int speed) {
  if (speed >= 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    speed = -speed;
  }
  analogWrite(ENB, constrain(speed, 0, 255));
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

// ============================================================
//  DRIVE PRIMITIVES
// ============================================================

void driveForward(int spd = FULL_SPEED) {
  motorLeft(spd);
  motorRight(spd);
}

void driveBackward(int spd = BACK_SPEED) {
  motorLeft(-spd);
  motorRight(-spd);
}

// Spin in place: left wheel forward, right wheel backward
void spinRight(int spd = TURN_SPEED) {
  motorLeft(spd);
  motorRight(-spd);
}

// Spin in place: right wheel forward, left wheel backward
void spinLeft(int spd = TURN_SPEED) {
  motorLeft(-spd);
  motorRight(spd);
}

// Gentle curve left (opponent slightly left)
void curveLeft(int spd = FULL_SPEED) {
  motorLeft(spd / 2);
  motorRight(spd);
}

// Gentle curve right (opponent slightly right)
void curveRight(int spd = FULL_SPEED) {
  motorLeft(spd);
  motorRight(spd / 2);
}

// ============================================================
//  SENSOR READERS
//  E3Z-D62: LOW when object detected (NPN output)
//  TCRT5000: HIGH when over white/edge (depending on wiring)
// ============================================================

bool opponentE()  { return digitalRead(LS_E)  == LOW; }
bool opponentNE() { return digitalRead(LS_NE) == LOW; }
bool opponentN()  { return digitalRead(LS_N)  == LOW; }
bool opponentNW() { return digitalRead(LS_NW) == LOW; }
bool opponentW()  { return digitalRead(LS_W)  == LOW; }
bool opponentS()  { return digitalRead(LS_S)  == LOW; }

// TCRT5000: HIGH = white line (edge of dohyo)
bool edgeFrontLeft()  { return digitalRead(frontIR1) == HIGH; }
bool edgeFrontRight() { return digitalRead(frontIR2) == HIGH; }
bool edgeBack()       { return digitalRead(backIR)   == HIGH; }

bool anyFrontOpponent() {
  return opponentN() || opponentNE() || opponentNW();
}

bool anyOpponent() {
  return opponentN() || opponentNE() || opponentNW() ||
         opponentE() || opponentW()  || opponentS();
}

// ============================================================
//  EDGE AVOIDANCE  (highest priority — called before anything)
// ============================================================

bool handleEdges() {
  bool fl = edgeFrontLeft();
  bool fr = edgeFrontRight();
  bool bk = edgeBack();

  // Both front sensors on edge → reverse hard
  if (fl && fr) {
    driveBackward(FULL_SPEED);
    delay(BACK_AVOID_MS + 100);
    spinRight(TURN_SPEED);
    delay(TURN_AVOID_MS + 100);
    return true;
  }

  // Front-left on edge → reverse + spin right
  if (fl) {
    driveBackward(FULL_SPEED);
    delay(BACK_AVOID_MS);
    spinRight(TURN_SPEED);
    delay(TURN_AVOID_MS);
    return true;
  }

  // Front-right on edge → reverse + spin left
  if (fr) {
    driveBackward(FULL_SPEED);
    delay(BACK_AVOID_MS);
    spinLeft(TURN_SPEED);
    delay(TURN_AVOID_MS);
    return true;
  }

  // Back sensor on edge → drive forward
  if (bk) {
    driveForward(FULL_SPEED);
    delay(BACK_AVOID_MS);
    return true;
  }

  return false;  // No edge detected
}

// ============================================================
//  ATTACK / TRACK OPPONENT
// ============================================================

void attackOpponent() {
  // Front center — charge!
  if (opponentN()) {
    driveForward(FULL_SPEED);
    return;
  }

  // Slightly right of front
  if (opponentNE()) {
    curveRight(FULL_SPEED);
    return;
  }

  // Slightly left of front
  if (opponentNW()) {
    curveLeft(FULL_SPEED);
    return;
  }

  // Pure right side → spin right to face
  if (opponentE()) {
    spinRight(TURN_SPEED);
    return;
  }

  // Pure left side → spin left to face
  if (opponentW()) {
    spinLeft(TURN_SPEED);
    return;
  }

  // Opponent behind → reverse-ram or spin 180
  if (opponentS()) {
    // Spin 180 to re-engage from front
    spinRight(FULL_SPEED);
    delay(400);
    return;
  }
}

// ============================================================
//  SEARCH PATTERN (no opponent visible)
// ============================================================

void searchForOpponent() {
  // Slow clockwise spin to scan arena
  spinRight(SEARCH_SPEED);
}

// ============================================================
//  STRATEGY  — runs once after start delay
//  Half-moon sweep: the robot arcs around the edge of the
//  dohyo to approach the opponent from the side/rear.
// ============================================================

void strategy() {
  // Arc right: left motor full, right motor slow → wide right curve
  // This traces roughly a semicircle around the dohyo perimeter.
  unsigned long startTime = millis();

  while (millis() - startTime < STRATEGY_TIME_MS) {
    // Edge safety during strategy
    if (handleEdges()) continue;

    // If we spot the opponent during sweep, break early & attack
    if (anyOpponent()) return;

    // Half-moon arc: inner wheel at ~40% gives a wide sweep
    motorLeft(FULL_SPEED);
    motorRight(100);
  }

  stopMotors();
  delay(50);
}

// ============================================================
//  SETUP
// ============================================================

void setup() {
  // Motor driver pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Opponent sensor inputs (E3Z-D62 NPN → pulled HIGH internally)
  pinMode(LS_E,  INPUT_PULLUP);
  pinMode(LS_NE, INPUT_PULLUP);
  pinMode(LS_N,  INPUT_PULLUP);
  pinMode(LS_NW, INPUT_PULLUP);
  pinMode(LS_W,  INPUT_PULLUP);
  pinMode(LS_S,  INPUT_PULLUP);

  // Edge sensor inputs (TCRT5000)
  pinMode(backIR,   INPUT);
  pinMode(frontIR1, INPUT);
  pinMode(frontIR2, INPUT);

  stopMotors();

  // ---- 5-Second mandatory start delay (competition rule) ----
  delay(START_DELAY_MS);

  // ---- Run opening strategy (half-moon repositioning arc) ----
  strategy();
}

// ============================================================
//  MAIN LOOP
// ============================================================

void loop() {
  // 1. Edge avoidance — always highest priority
  if (handleEdges()) return;

  // 2. Attack if opponent is visible
  if (anyOpponent()) {
    attackOpponent();
    return;
  }

  // 3. Search — spin to find opponent
  searchForOpponent();
}