
// Double include guard
#ifndef UTILITY_FUNCTIONS_H
#define UTILITY_FUNCTIONS_H

#include "Arduino.h"
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char *sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif // __arm__

int available_memory();
void utility_nicely_print_bool(bool input);

#endif
