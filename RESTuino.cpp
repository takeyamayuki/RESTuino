#include "RESTuino.h"

static const uint8_t n = 40;
static uint16_t gpio_arr[n] = {}; // すべて0で初期化
// Pubished values for SG90 servos; adjust if needed
static const uint32_t minUs = 0;
static const uint32_t maxUs = 5000;
// 180> angle > angle0 >= 0にすること
static const uint8_t angle0 = 5;
static const uint8_t angle = 60;

static WebServer server(80);
static Servo servo1;

enum RESTuino::status RESTuino::request_to_num(String req)
{
	if (req == "nan")
		return RESTuino::status::nan;
	else if (req == "digitalRead")
		return RESTuino::status::digitalread;
	else if (req == "digitalWrite")
		return RESTuino::status::digitalwrite;
	else if (req == "analogRead")
		return RESTuino::status::analogread;
	else if (req == "ledcWrite")
		return RESTuino::status::ledcwrite;
	else if (req == "Servo")
		return RESTuino::status::servo;
	else if (req == "save")
		return RESTuino::status::save;
	else if (req == "load")
		return RESTuino::status::load;
	else if (req == "reboot")
		return RESTuino::status::reboot;
	else
		return RESTuino::status::not_found;
}

void RESTuino::handle_not_found(void)
{
	server.send(404, "text/plain", "Not Found.\r\n");
}

// to0 flag check
bool RESTuino::to0_flag()
{
	return (servo1.read() > (angle + angle0) / 2) ? true : false;
}

// mode=true:angle0,angleのスイッチ, mode=false:自由角度への移動
void RESTuino::move_sg90(bool mode, uint8_t to_angle)
{
	if (mode)
		to0_flag() ? servo1.write(angle0) : servo1.write(angle);
	else
		servo1.write(to_angle);
}

String RESTuino::read_eeprom()
{
	String mes;
	for (uint8_t i = 0; i < n; i++) // GPIO:0-39
	{
		EEPROM.get(i * 4, gpio_arr[i]); // rom_ad=i*4
		mes += String(gpio_arr[i]);
	}
	return mes;
}

// server.send含まない
bool RESTuino::put_to_control(uint8_t pin, String target)
{
	Serial.println(gpio_arr[pin]);

	switch (gpio_arr[pin])
	{
	case RESTuino::status::servo:
		if (target == "switch")
		{
			move_sg90(true, 0);
			return true;
		}
		else
		{
			move_sg90(false, target.toInt());
			return true;
		}
		break;

	case RESTuino::status::ledcwrite:
		ledcWrite(0, target.toInt());
		return true;
		break;

	case RESTuino::status::digitalwrite:
		if (target == "HIGH" or target == "1")
		{
			digitalWrite(pin, HIGH);
			return true;
		}
		else if (target == "LOW" or target == "0")
		{
			digitalWrite(pin, LOW);
			return true;
		}
		else
		{
			return false;
		}
		break;

	default: // nan, not found ○
		return false;
		break;
	}
}

// server.send含まない
bool RESTuino::post_to_setup(uint8_t pin, uint8_t setup_mode)
{
	Serial.println(setup_mode);

	switch (setup_mode)
	{
	case RESTuino::status::servo:
		servo1.setPeriodHertz(50); // Standard 50hz servo
		servo1.attach(pin, minUs, maxUs);
		gpio_arr[pin] = setup_mode; // 0-99はgpio statusとして保存
		return true;
		break;

	case RESTuino::status::ledcwrite:
		ledcSetup(0, 12800, 8);
		ledcAttachPin(pin, 0);
		gpio_arr[pin] = setup_mode;
		return true;
		break;

	case RESTuino::status::digitalwrite:
		pinMode(pin, OUTPUT);
		gpio_arr[pin] = setup_mode;
		return true;
		break;

	case RESTuino::status::digitalread:
		pinMode(pin, INPUT);
		gpio_arr[pin] = setup_mode;
		return true;
		break;

	case RESTuino::status::analogread:
		pinMode(pin, ANALOG);
		gpio_arr[pin] = setup_mode;
		return true;
		break;

	// nan, not found
	// nanのPOST=DELETEのため、実装なし
	default:
		return false;
		break;
	}
}

void RESTuino::load_status()
{
	read_eeprom();
	for (uint8_t i = 0; i < n; i++)
	{
		post_to_setup(i, gpio_arr[i]); // server.send含まないこと
	}
}

// server.send含んで良い
void RESTuino::put_to_control_root(uint8_t setup_mode)
{
	switch (setup_mode)
	{
	case RESTuino::status::reboot:
		server.send(202, "text/plain", "Rebooting...\r\n");
		ESP.restart();
		break;

	case RESTuino::status::save:
		for (uint8_t i = 0; i < n; i++)
		{
			EEPROM.put(i * 4, gpio_arr[i]); // address=i*4
		}
		if (EEPROM.commit())
			server.send(200, "text/plain", "Saved\r\n");

		else
			server.send(400, "text/plain", "Cannot write to EEPROM\r\n");
		ESP.restart();
		break;

	case RESTuino::status::load: // 作業途中でloadすると、キャッシュされたgpio_arrが消える
		load_status();
		server.send(200, "text/plain", "Loaded\r\n");
		break;

	// case restuino::not_found:
	// nan, not found
	default:
		handle_not_found();
		break;
	}
}

// WiFi.localIP()->IP
String RESTuino::ip_to_String(uint32_t ip)
{
	String result = "";

	result += String((ip & 0xFF), 10);
	result += ".";
	result += String((ip & 0xFF00) >> 8, 10);
	result += ".";
	result += String((ip & 0xFF0000) >> 16, 10);
	result += ".";
	result += String((ip & 0xFF000000) >> 24, 10);

	return result;
}

/*              main               */

void RESTuino::handle_root(void)
{
	/* PUT: reboot, saveなど */
	if (server.method() == HTTP_PUT)
		put_to_control_root((uint8_t)request_to_num(server.arg("plain")));

	/* GET: IP address, GPIO status(キャッシュ) */
	else if (server.method() == HTTP_GET)
	{
		StaticJsonDocument<500> doc;
		doc["IP address"] = ip_to_String(WiFi.localIP());

		String mes = "";
		for (uint8_t i = 0; i < n; i++) // GPIO:0-39
		{
			mes += String(gpio_arr[i]);
		}
		doc["GPIO status"] = mes;

		char json_buf[500];
		serializeJsonPretty(doc, json_buf, 500);
		server.send(200, "application/json", json_buf);
	}

	/* DELETE ：全ピンをnan状態にする */
	else if (server.method() == HTTP_DELETE)
	{
		for (uint32_t i = 0; i < 1024; i++)
		{
			EEPROM.put(i, 0); // address=i*4
		}
		EEPROM.commit();
		server.send(202, "text/plain", "Change all pins to nan status...\r\n");
		ESP.restart();
	}
	else
	{
		handle_not_found();
	}
}

// すべてのGPIOを制御
void RESTuino::handle_gpio(int pin)
{
	/* POST ペリフェラル初期設定 */
	if (server.method() == HTTP_POST)
	{
		if (post_to_setup(pin, (uint8_t)request_to_num(server.arg("plain"))))
			server.send(200, "text/plain", server.arg("plain") + "\r\n");

		else
			handle_not_found();
	}

	/* PUT 更新 */
	else if (server.method() == HTTP_PUT)
	{
		if (put_to_control(pin, server.arg("plain")))
			server.send(200, "text/plain", server.arg("plain") + "\r\n");

		else
			handle_not_found();
	}

	/* GET 情報取得 */
	else if (server.method() == HTTP_GET)
	{
		Serial.println(gpio_arr[pin]);
		switch (gpio_arr[pin])
		{
		case RESTuino::status::servo:
			server.send(200, "text/plain", String(servo1.read()) + "\r\n"); // statusをクライアントに返す
			break;

		case RESTuino::status::digitalread:
			server.send(200, "text/plain", String(digitalRead(pin)) + "\r\n");
			break;

		case RESTuino::status::analogread:
			server.send(200, "text/plain", String(analogRead(pin)) + "\r\n");
			break;

		default:
			handle_not_found();
			break;
		}
	}

	/* DELETE : ピンをnan状態にする */
	else if (server.method() == HTTP_DELETE)
	{
		gpio_arr[pin] = (uint8_t)RESTuino::status::nan;
		server.send(202, "text/plain", "Change the pin to nan status... \r\n");
		Serial.println(RESTuino::status::nan);
		ESP.restart();
	}
	else
	{
		handle_not_found();
	}
}

/*                 end main                */

void RESTuino::setup()
{
	// EEPROM setup
	EEPROM.begin(1024); // 1kB     156byte=39*4
	// load
	load_status();
	// シリアルコンソールのセットアップ
	Serial.begin(serial_baud);
	delay(100);

	// WiFiに接続
	Serial.print("Connecting to ");
	Serial.print(ssid_def);
	WiFi.begin(ssid_def, ssid_pass);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(100);
		Serial.print(".");
	}
	Serial.println();

	MDNS.begin(host_name); // activate host_name.local
	Serial.println();
	Serial.println("WiFi connected.");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	// root
	server.on("/", [&]()
			  { handle_root(); });
	// 17, 20, 24, 27~31, 37, 38なし
	server.on("/gpio0", [&]()
			  { handle_gpio(0); });
	server.on("/gpio1", [&]()
			  { handle_gpio(1); });
	server.on("/gpio2", [&]()
			  { handle_gpio(2); });
	server.on("/gpio3", [&]()
			  { handle_gpio(3); });
	server.on("/gpio4", [&]()
			  { handle_gpio(4); });
	server.on("/gpio5", [&]()
			  { handle_gpio(5); });
	server.on("/gpio6", [&]()
			  { handle_gpio(6); });
	server.on("/gpio7", [&]()
			  { handle_gpio(7); });
	server.on("/gpio8", [&]()
			  { handle_gpio(8); });
	server.on("/gpio9", [&]()
			  { handle_gpio(9); });
	server.on("/gpio10", [&]()
			  { handle_gpio(10); });
	server.on("/gpio11", [&]()
			  { handle_gpio(11); });
	server.on("/gpio12", [&]()
			  { handle_gpio(12); });
	server.on("/gpio13", [&]()
			  { handle_gpio(13); });
	server.on("/gpio14", [&]()
			  { handle_gpio(14); });
	server.on("/gpio15", [&]()
			  { handle_gpio(15); });
	server.on("/gpio16", [&]()
			  { handle_gpio(16); });

	server.on("/gpio18", [&]()
			  { handle_gpio(18); });
	server.on("/gpio19", [&]()
			  { handle_gpio(19); });

	server.on("/gpio21", [&]()
			  { handle_gpio(21); });
	server.on("/gpio22", [&]()
			  { handle_gpio(22); });
	server.on("/gpio23", [&]()
			  { handle_gpio(23); });

	server.on("/gpio25", [&]()
			  { handle_gpio(25); });
	server.on("/gpio26", [&]()
			  { handle_gpio(26); });

	server.on("/gpio32", [&]()
			  { handle_gpio(32); });
	server.on("/gpio33", [&]()
			  { handle_gpio(33); });
	server.on("/gpio34", [&]()
			  { handle_gpio(34); });
	server.on("/gpio35", [&]()
			  { handle_gpio(35); });
	server.on("/gpio36", [&]()
			  { handle_gpio(36); });

	server.on("/gpio39", [&]()
			  { handle_gpio(39); });

	// onNotFound
	server.onNotFound([&]()
					  { handle_not_found(); });

	server.begin();
}

void RESTuino::loop()
{
	server.handleClient();
}