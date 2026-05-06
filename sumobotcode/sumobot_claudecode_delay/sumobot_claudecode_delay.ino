// --- Motor Driver Pins (XY-160D) ---
const int ENA = 5;   // Left  motor PWM speed
const int IN1 = 7;   // Left  motor direction A
const int IN2 = 8;   // Left  motor direction B
const int ENB = 6;   // Right motor PWM speed
const int IN3 = 9;   // Right motor direction A
const int IN4 = 10;  // Right motor direction B

// --- E3Z-D62 Opponent Sensors (LOW = opponent detected, NPN) ---
const int LS_E  = A0;  // East  (Right side)
const int LS_NE = A1;  // Northeast (Front-Right)
const int LS_N  = A2;  // North (Front Center)
const int LS_NW = A3;  // Northwest (Front-Left)
const int LS_W  = A4;  // West  (Left side)

// --- Sharp IR Sensor (analog, distance-based) ---
const int LS_S  = A5;  // South (Rear) - Sharp GP2Y0A21 or similar

// --- TCRT5000 Edge Sensors (HIGH = white/edge detected) ---
const int backIR   = 2;   // Rear edge sensor
const int frontIR1 = 3;   // Front-left edge sensor
const int frontIR2 = 11;  // Front-right edge sensor

// --- Motor Speed Constants ---
const int FULL_SPEED   = 255;
const int TURN_SPEED   = 200;
const int SEARCH_SPEED = 180;
const int BACK_SPEED   = 220;

// --- Sharp IR Threshold ---
// Sharp GP2Y0A21: voltage-to-distance formula used below.
// Opponent counted as detected if distance < 20 cm.
const float SHARP_DETECT_CM = 20.0;
// const float SHARP_VCC       = 5.0;    // Arduino 5V reference
// const int   SHARP_ADC_RES   = 1023;   // 10-bit ADC

// --- Timing ---
const unsigned long START_DELAY_MS   = 5000;  // 5-second mandatory start delay
const unsigned long STRATEGY_TIME_MS = 1800;  // ~half-moon sweep duration
const unsigned long BACK_AVOID_MS    = 280;   // reverse duration on edge detect
const unsigned long TURN_AVOID_MS    = 350;   // turn duration after reversing
const unsigned long PAPER_NUDGE_MS   = 200;   // forward nudge for paper test
const unsigned long PAPER_BACK_MS    = 250;   // reverse after failed paper test

// ============================================================
//  SHARP IR - DISTANCE HELPER
//  Converts analogRead value to distance in cm.
//  Formula for Sharp GP2Y0A21YK0F (10-80 cm range):
//    distance (cm) = 27.728 / (voltage ^ 1.2045)
//  Adjust constants if using a different Sharp model.
// ============================================================

float sharpReadCm() {
  int raw = analogRead(LS_S);
  if (raw == 0) return 9999.0;  // avoid divide-by-zero
  float voltage = analogRead(LS_S)*0.0048828125;
  float distanceCm = 13*pow(voltage, -1);
  return distanceCm;
}

bool opponentS() {
  return sharpReadCm() < SHARP_DETECT_CM;
}

// ============================================================
//  LOW-LEVEL MOTOR HELPERS
// ============================================================

void motorLeft(int speed) {
  if (speed >= 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    speed = -speed;
  }
  for(int s = 100; speed>s; s++){
    analogWrite(ENA, constrain(s, 0, 255));
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
  for(int s = 100; speed>s; s++){
    analogWrite(ENB, constrain(s, 0, 255));
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

void driveForward(int spd) {
  motorLeft(spd);
  motorRight(spd);
}

void driveBackward(int spd) {
  motorLeft(-spd);
  motorRight(-spd);
}

void spinRight(int spd) {
  motorLeft(spd);
  motorRight(-spd);
}

void spinLeft(int spd) {
  motorLeft(-spd);
  motorRight(spd);
}

void curveLeft(int spd) {
  motorLeft(spd / 2);
  motorRight(spd);
}

void curveRight(int spd) {
  motorLeft(spd);
  motorRight(spd / 2);
}

// ============================================================
//  SENSOR READERS - E3Z-D62 (NPN: LOW = detected)
// ============================================================

bool opponentE()  { return digitalRead(LS_E)  == LOW; }
bool opponentNE() { return digitalRead(LS_NE) == LOW; }
bool opponentN()  { return digitalRead(LS_N)  == LOW; }
bool opponentNW() { return digitalRead(LS_NW) == LOW; }
bool opponentW()  { return digitalRead(LS_W)  == LOW; }
// opponentS() is defined above using Sharp IR distance

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
//  EDGE AVOIDANCE (highest priority)
// ============================================================

bool handleEdges() {
  bool fl = edgeFrontLeft();
  bool fr = edgeFrontRight();
  bool bk = edgeBack();

  if (fl && fr) {
    driveBackward(FULL_SPEED);
    delay(BACK_AVOID_MS + 100);
    spinRight(TURN_SPEED);
    delay(TURN_AVOID_MS + 100);
    return true;
  }
  if (fl) {
    driveBackward(FULL_SPEED);
    delay(BACK_AVOID_MS);
    spinRight(TURN_SPEED);
    delay(TURN_AVOID_MS);
    return true;
  }
  if (fr) {
    driveBackward(FULL_SPEED);
    delay(BACK_AVOID_MS);
    spinLeft(TURN_SPEED);
    delay(TURN_AVOID_MS);
    return true;
  }
  if (bk) {
    driveForward(FULL_SPEED);
    delay(BACK_AVOID_MS);
    return true;
  }

  return false;
}

// ============================================================
//  PAPER TEST
//  Called only when LS_N triggers during attack in loop().
//
//  The robot nudges forward (~200ms) to lift any paper on the
//  ground beneath the sensor, then re-checks LS_N:
//    - Still HIGH (detected) -> real opponent -> return true
//    - Gone                  -> was paper     -> back up, return false
//
//  After returning false, loop() will fall through to
//  searchForOpponent() on the next iteration.
// ============================================================

bool paperTest() {
  // Nudge forward to push past / lift any paper obstacle
  driveForward(FULL_SPEED);
  delay(PAPER_NUDGE_MS);
  stopMotors();

  if (opponentN()) {
    // LS_N still triggered - confirmed real opponent
    return true;
  } else {
    // LS_N cleared - it was just paper, back up and search
    driveBackward(BACK_SPEED);
    delay(PAPER_BACK_MS);
    stopMotors();
    return false;
  }
}

// ============================================================
//  ATTACK / TRACK OPPONENT
//  Paper test is applied only when LS_N is the trigger.
// ============================================================

void attackOpponent() {
  // Front center detected - run paper test first
  if (opponentN()) {
    if (paperTest()) {
      // Confirmed real opponent - full charge
      driveForward(FULL_SPEED);
    }
    // If paperTest() returned false, it already backed us up.
    // loop() will call searchForOpponent() on the next iteration.
    return;
  }

  // Angled front sensors - no paper test needed
  if (opponentNE()) {
    curveRight(FULL_SPEED);
    return;
  }
  if (opponentNW()) {
    curveLeft(FULL_SPEED);
    return;
  }

  // Side sensors - spin to face opponent
  if (opponentE()) {
    spinRight(TURN_SPEED);
    return;
  }
  if (opponentW()) {
    spinLeft(TURN_SPEED);
    return;
  }

  // Opponent detected behind by Sharp IR - spin 180 to re-engage
  if (opponentS()) {
    spinRight(FULL_SPEED);
    delay(400);
    return;
  }

  // // alt: turns slightly, moves backward
  // if (opponentS()) {
  //   spinRight(TURN_SPEED);
  //   delay(200);
  //   driveBackward(FULL_SPEED);
  //   return;
  // }
}

// ============================================================
//  SEARCH PATTERN (no opponent visible)
// ============================================================

void searchForOpponent() {
  spinRight(SEARCH_SPEED);
}

// ============================================================
//  STRATEGY - runs once after start delay
//  Half-moon sweep: arcs around the dohyo perimeter to
//  approach the opponent from the side or rear.
// ============================================================

void strategy() {
  unsigned long startTime = millis();

  while (millis() - startTime < STRATEGY_TIME_MS) {
    if (handleEdges()) continue;
    if (anyOpponent()) return;  // spotted early - break into loop

    // Wide right arc: left motor full, right motor ~40%
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
  // Motor driver outputs
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // E3Z-D62 opponent sensors (NPN, pulled HIGH internally)
  pinMode(LS_E,  INPUT_PULLUP);
  pinMode(LS_NE, INPUT_PULLUP);
  pinMode(LS_N,  INPUT_PULLUP);
  pinMode(LS_NW, INPUT_PULLUP);
  pinMode(LS_W,  INPUT_PULLUP);
  // LS_S is analog - no pinMode needed (A5 defaults to analog input)

  // TCRT5000 edge sensors
  pinMode(backIR,   INPUT);
  pinMode(frontIR1, INPUT);
  pinMode(frontIR2, INPUT);

  stopMotors();

  // ---- 5-second mandatory start delay ----
  delay(START_DELAY_MS);

  // ---- Opening strategy: half-moon arc ----
  strategy();
}

// ============================================================
//  MAIN LOOP
// ============================================================

void loop() {
  // 1. Edge avoidance - always highest priority
  if (handleEdges()) return;

  // 2. Attack if opponent visible (includes paper test for LS_N)
  if (anyOpponent()) {
    attackOpponent();
    return;
  }

  // 3. Search - spin to find opponent
  searchForOpponent();
}