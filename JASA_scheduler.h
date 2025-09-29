#ifndef JASA_scheduler_h
#define JASA_scheduler_h

#include <Arduino.h>

class JASA_Scheduler {
public:
    // Alias type for scheduler mode
    using type = int;

    // Constants for readability
    static constexpr type RUNNING = 0;
    static constexpr type TIMER   = 1;

private:
    unsigned long lastMillisec;
    int intervallMillisec;
    type mode;  // store the scheduler type

public:
    JASA_Scheduler(int intervallMillisec, type mode = RUNNING);
    bool doTask();
    void setTimer(int timervalue);
    bool isTimer();
};

#endif