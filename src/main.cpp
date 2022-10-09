#include <RESTuino.h>

static RESTuino restuino;

void setup()
{
  restuino.host_name = "restuino";
  restuino.ssid_def = "";
  restuino.ssid_pass = "";
  restuino.setup();
}
void loop()
{
  restuino.loop();
}