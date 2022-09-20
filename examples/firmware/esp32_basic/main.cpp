#include "RESTuino/restuino_func.h"

uint8_t len_ssid = 2;
const char *ssid_def[] = {"ssid1", "ssid2"};
const char *ssid_pass[] = {"pass1", "pass2"};

static RestuinoFunc func;

void setup()
{
  func.restuino_setup();
}
void loop()
{
  func.restuino_loop();
}