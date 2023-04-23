#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>

class RESTuino
{
public:
	void setup();
	void loop();
	const char *host_name;
	const char *ssid_def;
	const char *ssid_pass;
	uint16_t serial_baud = 9600;

private:
	enum status
	{
		nan,
		digitalread,
		digitalwrite,
		analogread,
		ledcwrite,
		servo,
		touch,	  // TODO
		dacwrite, // TODO
		save,
		load,
		reboot,
		not_found,
	};
	enum RESTuino::status request_to_num(String req);
	void handle_not_found(void);
	bool to0_flag();
	void move_sg90(bool mode, uint8_t to_angle);
	String read_eeprom();
	bool put_to_control(uint8_t pin, String target);
	bool post_to_setup(uint8_t pin, uint8_t setup_mode);
	void load_status();
	void put_to_control_root(uint8_t setup_mode);
	String ip_to_String(uint32_t ip);
	void handle_root(void);
	void handle_gpio(int pin);
};
