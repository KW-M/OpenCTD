// Double include guard
#ifndef INTERRUPT_TIMER_H
#define INTERRUPT_TIMER_H

#include "Arduino.h"

#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 1024

void startTimer(int frequencyHz, voidFuncPtr callback);
void setTimerFrequency(int frequencyHz);

#endif // include guarge