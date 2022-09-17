#include "restuino_func.hpp"
#include "ssid_define.hpp"

const char *host_name = "restuino"; // RESTuino
const uint8_t n = 40;
uint16_t gpio_arr[n] = {}; //すべて0で初期化
// Pubished values for SG90 servos; adjust if needed
const uint32_t minUs = 0;
const uint32_t maxUs = 5000;
// 180> angle > angle0 >= 0にすること
const uint8_t angle0 = 5;
const uint8_t angle = 60;

enum restuino::status request_to_num(String req)
{
  if (req == "nan")
    return restuino::nan;
  else if (req == "digitalRead")
    return restuino::digitalread;
  else if (req == "digitalWrite")
    return restuino::digitalwrite;
  else if (req == "analogRead")
    return restuino::analogread;
  else if (req == "ledcWrite")
    return restuino::ledcwrite;
  else if (req == "Servo")
    return restuino::servo;
  else if (req == "save")
    return restuino::save;
  else if (req == "load")
    return restuino::load;
  else if (req == "reboot")
    return restuino::reboot;
  else
    return restuino::not_found;
}

void handle_not_found(void)
{
  server.send(404, "text/plain", "Not Found.\r\n");
}

// to0 flag check
bool to0_flag()
{
  return (servo1.read() > (angle + angle0) / 2) ? true : false;
}

// mode=true:angle0,angleのスイッチ, mode=false:自由角度への移動
void move_sg90(bool mode, uint8_t to_angle)
{
  if (mode)
    to0_flag() ? servo1.write(angle0) : servo1.write(angle);
  else
    servo1.write(to_angle);
}

String read_eeprom()
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
bool put_to_control(uint8_t pin, String target)
{
  Serial.println(gpio_arr[pin]);

  switch (gpio_arr[pin])
  {
  case restuino::servo:
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

  case restuino::ledcwrite:
    ledcWrite(0, target.toInt());
    return true;
    break;

  case restuino::digitalwrite:
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
bool post_to_setup(uint8_t pin, uint8_t setup_mode)
{
  Serial.println(setup_mode);

  switch (setup_mode)
  {
  case restuino::servo:
    servo1.setPeriodHertz(50); // Standard 50hz servo
    servo1.attach(pin, minUs, maxUs);
    gpio_arr[pin] = setup_mode; // 0-99はgpio statusとして保存
    return true;
    break;

  case restuino::ledcwrite:
    ledcSetup(0, 12800, 8);
    ledcAttachPin(pin, 0);
    gpio_arr[pin] = setup_mode;
    return true;
    break;

  case restuino::digitalwrite:
    pinMode(pin, OUTPUT);
    gpio_arr[pin] = setup_mode;
    return true;
    break;

  case restuino::digitalread:
    pinMode(pin, INPUT);
    gpio_arr[pin] = setup_mode;
    return true;
    break;

  case restuino::analogread:
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

void load_status()
{
  read_eeprom();
  for (uint8_t i = 0; i < n; i++)
  {
    post_to_setup(i, gpio_arr[i]); // server.send含まないこと
  }
}

// server.send含んで良い
void put_to_control_root(uint8_t setup_mode)
{
  switch (setup_mode)
  {
  case restuino::reboot:
    server.send(202, "text/plain", "Rebooting...\r\n");
    ESP.restart();
    break;

  case restuino::save:
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

  case restuino::load: //作業途中でloadすると、キャッシュされたgpio_arrが消える
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
String ip_to_String(uint32_t ip)
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

void handle_root(void)
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
void handle_gpio(int pin)
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
    case restuino::servo:
      server.send(200, "text/plain", String(servo1.read()) + "\r\n"); // statusをクライアントに返す
      break;

    case restuino::digitalread:
      server.send(200, "text/plain", String(digitalRead(pin)) + "\r\n");
      break;

    case restuino::analogread:
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
    gpio_arr[pin] = (uint8_t)restuino::nan;
    server.send(202, "text/plain", "Change the pin to nan status... \r\n");
    ESP.restart();
  }
  else
  {
    handle_not_found();
  }
}

void restuino_setup()
{
  // EEPROM setup
  EEPROM.begin(1024); // 1kB     156byte=39*4
  // load
  load_status();
  // シリアルコンソールのセットアップ
  Serial.begin(9600);
  delay(100);

  // WiFiに接続
  for (uint8_t i = 0; i < len_ssid; i++)
  {
    Serial.print("Connecting to ");
    Serial.print(ssid_def[i]);
    WiFi.begin(ssid_def[i], ssid_pass[i]);
    uint32_t cnt = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(100);
      Serial.print(".");
      cnt++;
      if (cnt >= 80) // 8sec
        break;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED)
      break;
  }

  MDNS.begin(host_name); // activate host_name.local
  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // root
  server.on("/", handle_root);

  // 17, 20, 24, 27~31, 37, 38なし
  server.on("/gpio0", []()
            { handle_gpio(0); });
  server.on("/gpio1", []()
            { handle_gpio(1); });
  server.on("/gpio2", []()
            { handle_gpio(2); });
  server.on("/gpio3", []()
            { handle_gpio(3); });
  server.on("/gpio4", []()
            { handle_gpio(4); });
  server.on("/gpio5", []()
            { handle_gpio(5); });
  server.on("/gpio6", []()
            { handle_gpio(6); });
  server.on("/gpio7", []()
            { handle_gpio(7); });
  server.on("/gpio8", []()
            { handle_gpio(8); });
  server.on("/gpio9", []()
            { handle_gpio(9); });
  server.on("/gpio10", []()
            { handle_gpio(10); });
  server.on("/gpio11", []()
            { handle_gpio(11); });
  server.on("/gpio12", []()
            { handle_gpio(12); });
  server.on("/gpio13", []()
            { handle_gpio(13); });
  server.on("/gpio14", []()
            { handle_gpio(14); });
  server.on("/gpio15", []()
            { handle_gpio(15); });
  server.on("/gpio16", []()
            { handle_gpio(16); });

  server.on("/gpio18", []()
            { handle_gpio(18); });
  server.on("/gpio19", []()
            { handle_gpio(19); });

  server.on("/gpio21", []()
            { handle_gpio(21); });
  server.on("/gpio22", []()
            { handle_gpio(22); });
  server.on("/gpio23", []()
            { handle_gpio(23); });

  server.on("/gpio25", []()
            { handle_gpio(25); });
  server.on("/gpio26", []()
            { handle_gpio(26); });

  server.on("/gpio32", []()
            { handle_gpio(32); });
  server.on("/gpio33", []()
            { handle_gpio(33); });
  server.on("/gpio34", []()
            { handle_gpio(34); });
  server.on("/gpio35", []()
            { handle_gpio(35); });
  server.on("/gpio36", []()
            { handle_gpio(36); });

  server.on("/gpio39", []()
            { handle_gpio(39); });

  // onNotFound
  server.onNotFound(handle_not_found);

  server.begin();
}