#include "JASA_scheduler.h"
#include <Arduino.h>

JASA_Scheduler::JASA_Scheduler(int intervallMillisec, type mode) {
  this->intervallMillisec = intervallMillisec;
  this->lastMillisec = 0;
  this->mode = mode;
}

bool JASA_Scheduler::doTask() {
  if (this->lastMillisec == 0 || millis() - this->lastMillisec >= this->intervallMillisec) {
    this->lastMillisec = millis();
    return true;
  } else {
    return false;
  }
}

void JASA_Scheduler::setTimer(int timervalue) {
  if(this->mode ==1) {
    this->intervallMillisec = timervalue;
    this->lastMillisec = millis();
  }
}

bool JASA_Scheduler::isTimer() {
  if ((this->lastMillisec != 0) && ((millis() - this->lastMillisec) >= this->intervallMillisec)) {
    lastMillisec=0;
    return true;
  } else {
    return false;
  }
}