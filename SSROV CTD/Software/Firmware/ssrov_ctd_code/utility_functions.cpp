#include "utility_functions.hpp"

void utility_nicely_print_bool(bool input) {
  if (input == true) {
    if (Serial)
      Serial.print("on");
    else if (Serial)
      Serial.print("off");
  }
}

void print_plot_value(const char * name, float value) {
  Serial.print(name);
  Serial.print(":");
  Serial.print(value,5);
  Serial.print(",");
}

// functions to print strings without triggering the Arduino IDE Serial Plotter:

String sanitize_string(String a) {
  a.replace(" "," ");
  a.replace(":","꞉");
  a.replace("\t","     ");
  a.replace(",","⹁");
  return a;
}


void print(const __FlashStringHelper *fsh) {
  String str = String(fsh);
  Serial.print(sanitize_string(str));
}

void print(const String str) {
  Serial.print(sanitize_string(str));
}

void print(const char *cstr) {
  String str = String(cstr);
  Serial.print(sanitize_string(str));
}

void println(const __FlashStringHelper *fsh) {
  String str = String(fsh);
  Serial.println(sanitize_string(str));
}

void println(const String str) {
  Serial.println(sanitize_string(str));
}

void println(const char *cstr) {
  String str = String(cstr);
    Serial.println(sanitize_string(str));
}



// ================

int available_memory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else   // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

// ================

RollingAverage::RollingAverage(int n) {
  size = n;
  index = 0;
  count = 0;
  values = new int[n];
  for (int i = 0; i < n; i++) {
    values[i] = 0;
  }
}

RollingAverage::~RollingAverage() {
  delete[] values;
}

void RollingAverage::add(int new_value) {
  values[index] = new_value;
  index = (index + 1) % size;
  if (count < size) {
    count++;
  }
}

double RollingAverage::getAverage() const {
  int sum = 0;
  for (int i = 0; i < count; i++) {
    sum += values[i];
  }
  return static_cast<double>(sum) / count;
}
