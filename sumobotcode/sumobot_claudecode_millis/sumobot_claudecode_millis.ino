// --- Motor Driver Pins (XY-160D) ---
const int ENA = 5;
const int IN1 = 7;
const int IN2 = 8;
const int ENB = 6;
const int IN3 = 9;
const int IN4 = 10;

// --- E3Z-D62 Opponent Sensors (LOW = opponent detected, NPN) ---
const int LS_E  = A0;
const int LS_NE = A1;
const int LS_N  = A2;
const int LS_NW = A3;
const int LS_W  = A4;

// --- Sharp IR Sensor (analog, distance-based) ---
const int LS_S = A5;

// --- TCRT5000 Edge Sensors (HIGH = white/edge detected) ---
const int backIR   = 2;
const int frontIR1 = 3;
const int frontIR2 = 11;

// --- Motor Speed Constants ---
const int FULL_SPEED   = 255;
const int TURN_SPEED   = 200;
const int SEARCH_SPEED = 180;
const int BACK_SPEED   = 220;

// --- Sharp IR Threshold ---
const float SHARP_DETECT_CM = 20.0;

// --- Timing (all in milliseconds) ---
const unsigned long START_DELAY_MS   = 5000;
const unsigned long STRATEGY_TIME_MS = 1800;
const unsigned long BACK_AVOID_MS    = 280;
const unsigned long TURN_AVOID_MS    = 350;
const unsigned long PAPER_NUDGE_MS   = 200;
const unsigned long PAPER_BACK_MS    = 250;
const unsigned long SPIN_180_MS      = 400;

// ============================================================
//  STATE MACHINE
//  Every distinct timed action is its own state.
//  loop() checks which state is active and whether its timer
//  has expired, then transitions to the next state.
// ============================================================

enum State {
  STATE_START_WAIT,   // waiting out the 5-second start delay
  STATE_STRATEGY,     // executing the opening half-moon arc
  STATE_SEARCH,       // spinning to find opponent
  STATE_ATTACK,       // charging / steering toward opponent
  STATE_PAPER_NUDGE,  // nudging forward to lift paper
  STATE_PAPER_BACK,   // backing up after failed paper test
  STATE_EDGE_REVERSE, // reversing away from edge
  STATE_EDGE_TURN,    // turning after reversing
  STATE_SPIN_180,     // spinning 180 to face rear opponent
};

State currentState    = STATE_START_WAIT;
State edgeReturnState = STATE_SEARCH;  // where to go after edge avoidance
unsigned long stateTimer = 0;          // millis() timestamp of last state entry
bool edgeTurnRight = true;             // direction to turn during edge avoidance

// ============================================================
//  SHARP IR - DISTANCE HELPER
// ============================================================

float sharpReadCm() {
  int raw = analogRead(LS_S);
  if (raw == 0) return 9999.0;
  float voltage = raw * 0.0048828125;  // raw * (5.0 / 1023)
  return 13.0 * pow(voltage, -1);
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
  for (int s = 100; s < speed; s++) {
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
  for (int s = 100; s < speed; s++) {
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

void driveForward(int spd)  { motorLeft(spd);    motorRight(spd);   }
void driveBackward(int spd) { motorLeft(-spd);   motorRight(-spd);  }
void spinRight(int spd)     { motorLeft(spd);    motorRight(-spd);  }
void spinLeft(int spd)      { motorLeft(-spd);   motorRight(spd);   }
void curveLeft(int spd)     { motorLeft(spd/2);  motorRight(spd);   }
void curveRight(int spd)    { motorLeft(spd);    motorRight(spd/2); }

// ============================================================
//  SENSOR READERS
// ============================================================

bool opponentE()  { return digitalRead(LS_E)  == LOW; }
bool opponentNE() { return digitalRead(LS_NE) == LOW; }
bool opponentN()  { return digitalRead(LS_N)  == LOW; }
bool opponentNW() { return digitalRead(LS_NW) == LOW; }
bool opponentW()  { return digitalRead(LS_W)  == LOW; }

bool edgeFrontLeft()  { return digitalRead(frontIR1) == HIGH; }
bool edgeFrontRight() { return digitalRead(frontIR2) == HIGH; }
bool edgeBack()       { return digitalRead(backIR)   == HIGH; }

bool anyOpponent() {
  return opponentN() || opponentNE() || opponentNW() ||
         opponentE() || opponentW()  || opponentS();
}

// ============================================================
//  STATE TRANSITION HELPERS
// ============================================================

void enterState(State s) {
  currentState = s;
  stateTimer   = millis();
}

bool timerExpired(unsigned long duration) {
  return (millis() - stateTimer) >= duration;
}

// ============================================================
//  SETUP
// ============================================================

void setup() {
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);

  pinMode(LS_E,  INPUT_PULLUP);
  pinMode(LS_NE, INPUT_PULLUP);
  pinMode(LS_N,  INPUT_PULLUP);
  pinMode(LS_NW, INPUT_PULLUP);
  pinMode(LS_W,  INPUT_PULLUP);
  // LS_S is analog - no pinMode needed

  pinMode(backIR,   INPUT);
  pinMode(frontIR1, INPUT);
  pinMode(frontIR2, INPUT);

  stopMotors();
  enterState(STATE_START_WAIT);
}

// ============================================================
//  MAIN LOOP - pure state machine, zero blocking delays
// ============================================================

void loop() {

  // ----------------------------------------------------------
  //  STATE_START_WAIT
  //  Sit still for 5 seconds (competition rule).
  // ----------------------------------------------------------
  if (currentState == STATE_START_WAIT) {
    stopMotors();
    if (timerExpired(START_DELAY_MS)) {
      enterState(STATE_STRATEGY);
    }
    return;
  }

  // ----------------------------------------------------------
  //  STATE_STRATEGY
  //  Half-moon arc around the dohyo perimeter.
  //  Exits early if edge or opponent is detected.
  // ----------------------------------------------------------
  if (currentState == STATE_STRATEGY) {
    bool fl = edgeFrontLeft();
    bool fr = edgeFrontRight();
    bool bk = edgeBack();

    if (fl || fr || bk) {
      edgeTurnRight   = !fl;           // front-left edge -> turn right; else turn left
      edgeReturnState = STATE_STRATEGY; // resume strategy after avoidance
      driveBackward(FULL_SPEED);
      enterState(STATE_EDGE_REVERSE);
      return;
    }

    if (anyOpponent()) {
      enterState(STATE_ATTACK);
      return;
    }

    if (!timerExpired(STRATEGY_TIME_MS)) {
      // Wide right arc: left motor full, right motor ~40%
      motorLeft(FULL_SPEED);
      motorRight(100);
      return;
    }

    stopMotors();
    enterState(STATE_SEARCH);
    return;
  }

  // ----------------------------------------------------------
  //  EDGE AVOIDANCE - shared check for all combat states
  //  Placed here so every combat state below is protected.
  // ----------------------------------------------------------
  if (currentState == STATE_SEARCH  ||
      currentState == STATE_ATTACK  ||
      currentState == STATE_SPIN_180) {
    bool fl = edgeFrontLeft();
    bool fr = edgeFrontRight();
    bool bk = edgeBack();

    if (fl || fr || bk) {
      edgeTurnRight   = !fl;
      edgeReturnState = STATE_SEARCH;
      driveBackward(FULL_SPEED);
      enterState(STATE_EDGE_REVERSE);
      return;
    }
  }

  // ----------------------------------------------------------
  //  STATE_EDGE_REVERSE
  //  Reverse for BACK_AVOID_MS, then transition to EDGE_TURN.
  // ----------------------------------------------------------
  if (currentState == STATE_EDGE_REVERSE) {
    driveBackward(FULL_SPEED);
    if (timerExpired(BACK_AVOID_MS)) {
      edgeTurnRight ? spinRight(TURN_SPEED) : spinLeft(TURN_SPEED);
      enterState(STATE_EDGE_TURN);
    }
    return;
  }

  // ----------------------------------------------------------
  //  STATE_EDGE_TURN
  //  Hold turn for TURN_AVOID_MS, then return to saved state.
  // ----------------------------------------------------------
  if (currentState == STATE_EDGE_TURN) {
    edgeTurnRight ? spinRight(TURN_SPEED) : spinLeft(TURN_SPEED);
    if (timerExpired(TURN_AVOID_MS)) {
      enterState(edgeReturnState);
    }
    return;
  }

  // ----------------------------------------------------------
  //  STATE_SEARCH
  //  Spin slowly until any opponent sensor fires.
  // ----------------------------------------------------------
  if (currentState == STATE_SEARCH) {
    if (anyOpponent()) {
      enterState(STATE_ATTACK);
      return;
    }
    spinRight(SEARCH_SPEED);
    return;
  }

  // ----------------------------------------------------------
  //  STATE_ATTACK
  //  Steer toward the opponent.
  //  LS_N fires the paper test via STATE_PAPER_NUDGE.
  // ----------------------------------------------------------
  if (currentState == STATE_ATTACK) {
    if (!anyOpponent()) {
      stopMotors();
      enterState(STATE_SEARCH);
      return;
    }

    if (opponentN()) {
      // Start nudge - motor call happens in STATE_PAPER_NUDGE
      driveForward(FULL_SPEED);
      enterState(STATE_PAPER_NUDGE);
      return;
    }
    if (opponentNE()) { curveRight(FULL_SPEED); return; }
    if (opponentNW()) { curveLeft(FULL_SPEED);  return; }
    if (opponentE())  { spinRight(TURN_SPEED);  return; }
    if (opponentW())  { spinLeft(TURN_SPEED);   return; }
    if (opponentS())  {
      spinRight(FULL_SPEED);
      enterState(STATE_SPIN_180);
      return;
    }
    return;
  }

  // ----------------------------------------------------------
  //  STATE_PAPER_NUDGE
  //  Keep moving forward for PAPER_NUDGE_MS, then re-check
  //  LS_N to determine if the detection was paper or opponent.
  // ----------------------------------------------------------
  if (currentState == STATE_PAPER_NUDGE) {
    if (!timerExpired(PAPER_NUDGE_MS)) {
      driveForward(FULL_SPEED);  // keep nudging
      return;
    }

    stopMotors();

    if (opponentN()) {
      // Real opponent confirmed - go back to attacking
      enterState(STATE_ATTACK);
    } else {
      // Was paper - back up
      driveBackward(BACK_SPEED);
      enterState(STATE_PAPER_BACK);
    }
    return;
  }

  // ----------------------------------------------------------
  //  STATE_PAPER_BACK
  //  Reverse for PAPER_BACK_MS after failed paper test,
  //  then search.
  // ----------------------------------------------------------
  if (currentState == STATE_PAPER_BACK) {
    driveBackward(BACK_SPEED);
    if (timerExpired(PAPER_BACK_MS)) {
      stopMotors();
      enterState(STATE_SEARCH);
    }
    return;
  }

  // ----------------------------------------------------------
  //  STATE_SPIN_180
  //  Spin for SPIN_180_MS to face a rear opponent, then attack.
  // ----------------------------------------------------------
  if (currentState == STATE_SPIN_180) {
    spinRight(FULL_SPEED);
    if (timerExpired(SPIN_180_MS)) {
      enterState(STATE_ATTACK);
    }
    return;
  }
}