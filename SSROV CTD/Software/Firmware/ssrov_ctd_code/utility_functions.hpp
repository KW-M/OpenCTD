
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
void print_plot_value(const char * name, float value);

void print(const __FlashStringHelper *fsh);
void print(const String str);
void print(const char *cstr);

void println(const __FlashStringHelper *fsh);
void println(const String str);
void println(const char *cstr);

class RollingAverage {
private:
  int* values;
  int index;
  int count;
  int size;

public:
  RollingAverage(int n);
  ~RollingAverage();
  void add(int new_value);
  double getAverage() const;
};

#endif
