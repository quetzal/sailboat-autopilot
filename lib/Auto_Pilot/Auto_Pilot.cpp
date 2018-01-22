#include <Arduino.h>
#include "Define_autopilot.h"

long start_turn = -1;
int lastTurn = 0;

void initPilot(){
  pinMode(Turn_Right_GPIO, OUTPUT);
  pinMode(Turn_Left_GPIO, OUTPUT);
}

void Setlastturn(int direction) {
  lastTurn = direction;
}

void Turn_Left(long turn_time) {
  if (start_turn == -1 || lastTurn == LAST_TURN_RIGHT) {
    digitalWrite(Turn_Left_GPIO, HIGH);
    start_turn = millis();
  }
  if (start_turn + turn_time < millis()) {
    digitalWrite(Turn_Left_GPIO, LOW);
  }
}

void Turn_Right(long turn_time) {
  if (start_turn == -1 || lastTurn == LAST_TURN_LEFT) {
    digitalWrite(Turn_Right_GPIO, HIGH);
    start_turn = millis();
  }
  if (start_turn + turn_time < millis()) {
    digitalWrite(Turn_Right_GPIO, LOW);

  }
}
