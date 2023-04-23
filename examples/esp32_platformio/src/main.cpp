#include <Arduino.h>
#include <RESTuino.h>

static RESTuino restuino;

void setup()
{
	restuino.host_name = "restuino";
	restuino.ssid_def = "YOUR_SSID";
	restuino.ssid_pass = "YOUR_PASSWORD";
	restuino.serial_baud = 9600;
	restuino.setup();
}
void loop()
{
	restuino.loop();
}