#include "JASA_scheduler.h"
#include <Arduino.h>

JASA_Scheduler::JASA_Scheduler(int intervallMillisec) {
  this->intervallMillisec = intervallMillisec;
  this->lastMillisec = 0;
}

bool JASA_Scheduler::doTask() {
  if (this->lastMillisec == 0 || millis() - this->lastMillisec >= this->intervallMillisec) {
    this->lastMillisec = millis();
    return true;
  } else {
    return false;
  }
}