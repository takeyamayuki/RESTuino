#include <Arduino.h>
#include <RESTuino.h>

static RESTuino restuino;

void setup()
{
	restuino.host_name = "restuino";
	restuino.ssid_def = "Extender-G-CE1C";
	restuino.ssid_pass = "jifmckdmk6inr";
	restuino.serial_baud = 9600;
	restuino.setup();
}
void loop()
{
	restuino.loop();
}