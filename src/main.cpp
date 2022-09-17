#include "ssid_define.hpp"
#include "restuino_func.hpp"

void setup()
{
  restuino_setup();
}

void loop()
{
  server.handleClient();
}