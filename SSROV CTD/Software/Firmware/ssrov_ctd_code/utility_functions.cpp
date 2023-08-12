#include "utility_functions.hpp"

void utility_nicely_print_bool(bool input)
{
    if (input == true)
    {
        if (Serial)
            Serial.print("on");
        else if (Serial)
            Serial.print("off");
    }
}

int available_memory()
{
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char *>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}
