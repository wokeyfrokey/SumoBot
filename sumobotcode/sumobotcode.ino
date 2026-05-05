#include <stdint.h>
#include <stdlib.h>
#DEFINE MAXMOTOR 255
#DEFINE MINMOTOR 0

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

typedef enum enum_Direction{
  FORWARD,
  BACKWARD,
  LEFT,
  RIGHT
}direction;

typedef enum{
  SEARCH,
  ATTACK,
  RECOVERY
}STATES;

typedef struct wheelStateStruct{
  int L; //0 - 255
  int R;
}wheelState;

typedef struct vector2DStruct{
  int X;
  int Y;
}vector2D;

wheelState motorState = {0, 0};
STATES currentState;

wheelState ConvertToWheelState(vector2D input){
    wheelState output;
    output.L = input.X + input.Y;
    output.R = -input.X + input.Y;
    return output;
}

void MotorFunction(wheelState input){
    if(input.L > 0){
      Motor1_Forward(input.L);
    }else if(input.L < 0){
      Motor1_Backward(-input.L);
    }else{
      Motor1_Brake();
    }
    if(input.R > 0){
      Motor2_Forward(input.R);
    }else if(input.R < 0){
      Motor2_Backward(-input.R);
    }else{
      Motor2_Brake();
    }
}

void Motor1_Forward(uint8_t Speed)
{
 digitalWrite(IN1,HIGH);
 digitalWrite(IN2,LOW);
 analogWrite(ENA,Speed);
}

void Motor1_Backward(uint8_t Speed)
{
 digitalWrite(IN1,LOW);
 digitalWrite(IN2,HIGH);
 analogWrite(ENA,Speed);
}
void Motor1_Brake()
{
 digitalWrite(IN1,LOW);
 digitalWrite(IN2,LOW);
}
void Motor2_Forward(uint8_t Speed)
{
 digitalWrite(IN3,HIGH);
 digitalWrite(IN4,LOW);
 analogWrite(ENB,Speed);
}

void Motor2_Backward(uint8_t Speed)
{
 digitalWrite(IN3,LOW);
 digitalWrite(IN4,HIGH);
 analogWrite(ENB,Speed);
}
void Motor2_Brake()
{
 digitalWrite(IN3,LOW);
 digitalWrite(IN4,LOW);
}

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
  Serial.begin(9600);

  // 🔴 FORCE SAFE STATE FIRST
  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  // or just:
  Motor1_Brake();
  Motor2_Brake();

}
int lerp(int a, int b, float factor){
  return a + (b - a) * factor;
}

void readIR(){
  back_reading = digitalRead(backIR);
  front_reading1 = digitalRead(frontIR1);
  front_reading2 = digitalRead(frontIR2);
}

void loop() {
  unsigned long currentTime = millis(); // Get the current time
  if (!isMovingForward) { //starts timer
          startTime = currentTime; 
          isMovingForward = true;
  }
  switch(currentState){
    case SEARCH:
      //reads IR sensor aka checks if need to go to RECOVERY
      readIR();
      if(back_reading == LOW || front_reading1 == LOW || front_reading2 == LOW){
        currentState = RECOVERY;
        break;
      }

      //moves forward a bit
      if (currentTime - startTime < 2000) { //moves forward for 2 seconds
        vector2D move = {0, 150}; 
        motorState = ConvertToWheelState(move);
      } 
      //scans the area and adjusts accordingly 
      else{
        if(LS_E == HIGH){

        } else if(LS_NE == HIGH){

        } else if(LS_N == HIGH){
          
        } else if(LS_NW == HIGH){
          
        } else if(LS_W == HIGH){
          
        }
      }
      break;
    case RECOVERY:
      break;
    case ATTACK:
      break;
    default:
      break;
  }
  MotorFunction(motorState);
}





 