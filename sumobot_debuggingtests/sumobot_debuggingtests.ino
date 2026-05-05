#include <stdint.h>
#include <stdlib.h>

const int ENA=5;
const int IN1=7;
const int IN2=8;

const int ENB=6;
const int IN3=9;
const int IN4=10;

const int LS_E = A0;
const int LS_NE = A1;
const int LS_N = A2;
const int LS_NW = A3;
const int LS_W = A4;
const int LS_S = A5;

const int backIR = 2;
const int frontIR1 = 3;
const int frontIR2 = 11;

volatile bool back_reading = 0;
volatile bool front_reading1 = 0;
volatile bool front_reading2 = 0;

int motor1_speed = 0;
int motor2_speed = 0;

//setup for the millis timer
unsigned long startTime = 0;   // Stores the moment the action started
bool isMovingForward = false;  // A "flag" to track if we are in the middle of a timed move

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(IN4, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(backIR, INPUT);
  pinMode(frontIR1, INPUT);
  pinMode(frontIR2, INPUT);

  pinMode(LS_E, INPUT_PULLUP);
  pinMode(LS_NE, INPUT_PULLUP);
  pinMode(LS_N, INPUT_PULLUP);
  pinMode(LS_NW, INPUT_PULLUP);
  pinMode(LS_W, INPUT_PULLUP);
  pinMode(LS_S, INPUT_PULLUP);
  Serial.begin(9600);

  // 🔴 FORCE SAFE STATE FIRST
  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void motorTest(){
  //just turns on the motors
  analogWrite(ENA, 255);
  analogWrite(ENB, 255);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void sensorTest(){
  //just prints out sensor values
  Serial.println("--- arena IR sensors ---");
  Serial.print("front IR1: ");
  Serial.println(digitalRead(frontIR1));
  Serial.print("front IR2: ");
  Serial.println(digitalRead(frontIR2));
  Serial.print("back IR: ");
  Serial.println(digitalRead(backIR));

  Serial.println("--- opp IR sensors ---");
  Serial.print("East: ");
  Serial.println(digitalRead(LS_E));
  Serial.print("North East: ");
  Serial.println(digitalRead(LS_NE));
  Serial.print("North: ");
  Serial.println(digitalRead(LS_N));
  Serial.print("North West: ");
  Serial.println(digitalRead(LS_NW));
  Serial.print("West: ");
  Serial.println(digitalRead(LS_W));

  //note: we can probs have a fixed "trigger distance" for this one
  float volts = analogRead(LS_S)*0.0048828125;  // value from sensor * (5/1024)
  int distance = 13*pow(volts, -1); // worked out from datasheet graph
  Serial.print("South: ");
  Serial.println(distance);

  delay(1000);
}

void motorSensorTest(){
  float volts = analogRead(LS_S)*0.0048828125;  // value from sensor * (5/1024)
  int distance = 13*pow(volts, -1); // worked out from datasheet graph

  if(digitalRead(LS_N) == HIGH){
    //just turns on the motors
    analogWrite(ENA, 255);
    analogWrite(ENB, 255);

    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  }

  if (distance<10){
    //tests south opp sensor
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
}

void motorMillisTest(){
  unsigned long currentTime = millis(); // Get the current time
  if (!isMovingForward) { //starts timer
          startTime = currentTime; 
          isMovingForward = true;
  }

  //runs motors for 2 seconds
  if (currentTime - startTime < 2000) { 
    analogWrite(ENA, 255);
    analogWrite(ENB, 255);
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else{
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
