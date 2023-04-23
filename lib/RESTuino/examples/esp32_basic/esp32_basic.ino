#include <RESTuino.h>

static RESTuino restuino;

void setup()
{
	restuino.host_name = "restuino";
	restuino.ssid_def = "";
	restuino.ssid_pass = "";
	restuino.serial_baud = 9600;
	restuino.setup();
}
void loop()
{
	restuino.loop();
}