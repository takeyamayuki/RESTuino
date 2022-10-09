#include <RESTuino.h>

static RESTuino restuino;

restuino.host_name = "restuino";
restuino.ssid_def = " ";
restuino.ssid_pass = " ";

void setup()
{
  restuino.setup();
}
void loop()
{
  restuino.loop();
}