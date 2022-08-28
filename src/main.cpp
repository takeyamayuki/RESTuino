#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "ssid_define.hpp"

namespace restuino
{
  enum gpio_status
  {
    nan = 0,
    digitalread,
    digitalwrite,
    analogread,
    ledcwrite,
    servo,
    touch,    /////////////////////////kore
    dacwrite, ///////////////////////kore
    save = 100,
    reflect,
    reboot,
    not_found,
  };
}

static const char *host_name = "restuino"; // RESTuino
static const uint8_t n = 40;
static uint8_t gpio_arr[n] = {}; //すべて0で初期化

// Published values for SG90 servos; adjust if needed
static const uint32_t minUs = 0;
static const uint32_t maxUs = 5000;
// 180 > angle > angle0 >= 0にすること
static const uint8_t angle0 = 5;
static const uint8_t angle = 60;

WebServer server(80);

Servo servo1;

uint8_t request_to_num(String req)
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
  else if (req == "reflect")
    return restuino::reflect;
  else if (req == "reboot")
    return restuino::reboot;
  else
    return restuino::not_found;
}

void handle_not_found(void)
{
  server.send(404, "text/plain", "Not Found.\n");
}

// to0 flag check
bool to0_flag()
{
  return (servo1.read() > (angle + angle0) / 2) ? true : false;
}

// mode=true:angle0,angleのスイッチ, mode=false:自由角度への移動
void move_sg90(bool mode, uint32_t to_angle)
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

void put_to_control(uint8_t pin, String target)
{
  switch (gpio_arr[pin])
  {
  case restuino::servo:
    if (target == "switch")
      move_sg90(true, 0);
    else
      move_sg90(false, target.toInt());
    server.send(200, "text/plain", "servo.write " + target + "\n"); //リクエストされた角度を返す
    break;
  
  case restuino::ledcwrite:
    ledcWrite(0, target.toInt());
    server.send(200, "text/plain", "dledcWrite " + target + "\n");
    break;
  
  case restuino::digitalwrite:
    if (target == "HIGH" or target == "1")
    {
      digitalWrite(pin, HIGH);
      server.send(200, "text/plain", "digitalWrite: HIGH\n");
    }
    else if (target == "LOW" or target == "0")
    {
      digitalWrite(pin, LOW);
      server.send(200, "text/plain", "digitalWrite; LOW\n");
    }
    else
    {
      handle_not_found();
    }
    break;
  
  default:
    handle_not_found();
    break;
  }
}

void post_to_setup(uint8_t pin, uint8_t setup_mode)
{
  switch (setup_mode)
  {
  case restuino::servo:
    servo1.setPeriodHertz(50); // Standard 50hz servo
    servo1.attach(pin, minUs, maxUs);
    server.send(200, "text/plain", "Servo");
    break;

  // ledcWrite enable pin
  // 0, 2, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33
  case restuino::ledcwrite:
    ledcSetup(0, 12800, 8);
    ledcAttachPin(pin, 0);
    server.send(200, "text/plain", "ledcWrite");
    break;

  // digitalWrite enable pin
  // 0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
  case restuino::digitalwrite:
    pinMode(pin, OUTPUT);
    server.send(200, "text/plain", "digitalWrite");
    break;

  // digitalRead enable pin
  // 1, 2, 4, 5, 7, 8, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 37
  case restuino::digitalread:
    pinMode(pin, INPUT);
    server.send(200, "text/plain", "digitalRead");
    break;
  
  // analogRead anable pin
  // 0, 2, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33, 34, 35, 36, 39
  case restuino::analogread:
    pinMode(pin, INPUT);
    server.send(200, "text/plain", "analogRead");
    break;

  case restuino::reboot:
    server.send(200, "text/plain", "Rebooting...\n");
    ESP.restart();
    break;

  case restuino::save:
    for (uint8_t i = 0; i < n; i++)
    {
      EEPROM.put(i * 4, gpio_arr[i]); // address=i*4
    }
    EEPROM.commit();
    server.send(200, "text/plain", "EEPROM write\n");
    break;

  case restuino::reflect:
    read_eeprom();
    for (uint8_t i = 0; i < n; i++)
    {
      post_to_setup(i, gpio_arr[i]);
    }
    server.send(200, "text/plain", "EEPROM read & reflect\n");
    break;

  case restuino::not_found:
    handle_not_found();
    break;
  }

  if (setup_mode < 100) // 0-99はgpio statusとして保存
  {
    gpio_arr[pin] = setup_mode;
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
  if (server.method() == HTTP_POST)
  {
    post_to_setup(0, request_to_num(server.arg("plain")));
  }

  else if (server.method() == HTTP_GET)
  {
    String mes;
    mes += "IP address: ";
    mes += ip_to_String(WiFi.localIP());
    mes += "\n";
    mes += "GPIO status: ";
    mes += read_eeprom();
    mes += "\n";
    server.send(200, "text/plain", mes);
    // pinに反映させる
  }
}

// すべてのGPIOを制御
void handle_gpio(int pin)
{
  /* POST ペリフェラル初期設定 */
  // request body = servoだったら, servoの設定 & pin*4のアドレスのEEPROMに0(servo)を格納
  if (server.method() == HTTP_POST)
  {
    String req = server.arg("plain"); // get request body
    post_to_setup(pin, request_to_num(req));
  }
  /* PUT 更新 */
  else if (server.method() == HTTP_PUT)
  {
    put_to_control(pin, server.arg("plain"));
  }

  /* GET 情報取得 */
  else if (server.method() == HTTP_GET)
  {
    switch (gpio_arr[pin])
    {
    case restuino::servo:
      server.send(200, "text/plain", String(servo1.read()) + "\n"); // statusをクライアントに返す
      break;

    case restuino::digitalread:
      server.send(200, "text/plain", String(digitalRead(pin)) + "\n");
      break;
    
    case restuino::analogread:
      server.send(200, "text/plain", String(analogRead(pin)) + "\n");
      break;
    
    default:
      handle_not_found();
      break;
    }
  }
}

void setup()
{
  // EEPROM setup
  EEPROM.begin(1024); // 1kB     156byte=39*4
  // read reflect
  read_eeprom();
  for (uint8_t i = 0; i < n; i++)
  {
    post_to_setup(i, gpio_arr[i]);
  }
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

void loop()
{
  server.handleClient();
}