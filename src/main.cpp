#include <Arduino.h>

#include "restuino_func.h"

static RestuinoFunc func;

void setup()
{
  func.restuino_setup();
}
void loop()
{
  func.restuino_loop();
}