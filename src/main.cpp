#include <Arduino.h>

#include <WiFi.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ESP32Servo.h>

#include "ssid_define.hpp"

WebServer server(80);

const char *host_name = "restuino"; // RESTuino
const uint32_t n = 40;
uint32_t gpio_arr[n] = {}; //すべて0で初期化

/* 各種アクション */
const int _nan = 0;
const int _digitalread = 1;
const int _digitalwrite = 2;
const int _analogread = 3;
const int _ledcwrite = 4;
const int _servo = 5;
const int _touch = 6;    /////////////////////////kore
const int _dacwrite = 7; ///////////////////////kore

const int _save = 100;
const int _reflect = 101;
const int _reboot = 102;

/* servo */
Servo servo1;
// Published values for SG90 servos; adjust if needed
const uint8_t minUs = 0;
const uint32_t maxUs = 5000;
// bool to0_flag = false;
// angle > angle0 >= 0にすること
const uint8_t angle = 60;
const uint8_t angle0 = 5;

uint32_t RequestToNum(String req)
{
  if (req == "digitalRead")
    return 1;
  else if (req == "digitalWrite")
    return 2;
  else if (req == "analogRead")
    return 3;
  else if (req == "ledcWrite")
    return 4;
  else if (req == "Servo")
    return 5;
  else if (req == "save")
    return 100;
  else if (req == "reflect")
    return 101;
  else if (req == "reboot")
    return 102;
  else
    return 0; // nan
}

void handle_not_found(void)
{
  server.send(404, "text/plain", "Not Found.\n");
}

// to0_flag check
bool to0_flag()
{
  // to0_flag
  int32_t status = servo1.read();
  // return (status > (angle + angle0) / 2) ? true : false;
  if (status > (angle + angle0) / 2)
    return true; //現在angle側なので0に持っていく
  else if (status < (angle + angle0) / 2)
    return false; //現在angle0側なので30に持っていく
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
  for (uint32_t i = 0; i < n; i++) // GPIO:0-39
  {
    EEPROM.get(i * 4, gpio_arr[i]); // rom_ad=i*4
    mes += String(gpio_arr[i]);
  }
  return mes;
}

void PutToControl(uint32_t pin, uint32_t setup_mode, String target)
{
  if (setup_mode == 5) // servo
  {
    if (target == "switch")
      move_sg90(true, 0);
    else
      move_sg90(false, target.toInt());
    server.send(200, "text/plain", "servo.write " + target + "\n"); //リクエストされた角度を返す
  }
  else if (setup_mode == 4) // ledcwrite
  {
    ledcWrite(0, target.toInt());
    server.send(200, "text/plain", "dledcWrite " + target + "\n");
  }
  else if (setup_mode == 2) // digitalwrite
  {
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
  }
  else
  {
    handle_not_found();
  }
}

void PostToSetup(uint32_t pin, uint32_t setup_mode)
{
  if (setup_mode == 5) // servo
  {
    servo1.setPeriodHertz(50); // Standard 50hz servo
    servo1.attach(pin, minUs, maxUs);
  }
  // ledcWrite enable pin
  // 0, 2, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33
  else if (setup_mode == 4) // ledcwrite
  {
    ledcSetup(0, 12800, 8);
    ledcAttachPin(pin, 0);
  }
  // digitalWrite enable pin
  // 0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
  else if (setup_mode == 2) // digitalwrite
  {
    pinMode(pin, OUTPUT);
  }
  // digitalRead enable pin
  // 1, 2, 4, 5, 7, 8, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 37
  else if (setup_mode == 1) // digitalread
  {
    pinMode(pin, INPUT);
  }
  // analogRead anable pin
  // 0, 2, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33, 34, 35, 36, 39
  else if (setup_mode == 3) // analogread
  {
    pinMode(pin, INPUT);
  }
  else if (setup_mode == 102) // reboot //root
  {
    server.send(200, "text/plain", "Rebooting...\n");
    ESP.restart();
  }
  else if (setup_mode == 100) // save //root
  {
    for (int i = 0; i < n; i++)
    {
      EEPROM.put(i * 4, gpio_arr[i]); // address=i*4
    }
    EEPROM.commit();
    server.send(200, "text/plain", "EEPROM write\n");
  }
  else if (setup_mode == 101) // reflect //root
  {
    read_eeprom();
    for (uint32_t i = 0; i < n; i++)
    {
      PostToSetup(i, gpio_arr[i]);
    }
    server.send(200, "text/plain", "EEPROM read & reflect\n");
  }
  else if (setup_mode != 0)
  {
    handle_not_found();
  }
  if (setup_mode < 100) // 0-99はgpio statusとして保存
  {
    gpio_arr[pin] = setup_mode;
  }
}

// WiFi.localIP()->IP
String ipToString(uint32_t ip)
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
  // server.send(200, "text/plain", "Nice!!\n");
  if (server.method() == HTTP_POST)
  {
    PostToSetup(0, RequestToNum(server.arg("plain")));
  }

  else if (server.method() == HTTP_GET)
  {
    String mes;
    mes += "IP address: ";
    mes += ipToString(WiFi.localIP());
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
  uint32_t rom_ad = pin * 4;
  String message;
  /* POST ペリフェラル初期設定 */
  // request body = servoだったら, servoの設定 & pin*4のアドレスのEEPROMに0(servo)を格納
  if (server.method() == HTTP_POST)
  {
    String req = server.arg("plain"); // get request body
    PostToSetup(pin, RequestToNum(req));
    message += req + "\n";
    server.send(200, "text/plain", message);
  }
  /* PUT 更新 */
  else if (server.method() == HTTP_PUT)
  {
    PutToControl(pin, gpio_arr[pin], server.arg("plain"));
  }

  /* GET 情報取得 */
  else if (server.method() == HTTP_GET)
  {
    if (gpio_arr[pin] == 5) // servo
    {
      server.send(200, "text/plain", String(servo1.read()) + "\n"); // statusをクライアントに返す
    }
    else if (gpio_arr[pin] == 1) // digitalread
    {
      server.send(200, "text/plain", String(digitalRead(pin)) + "\n");
    }
    else if (gpio_arr[pin] == 3) // analogread
    {
      server.send(200, "text/plain", String(analogRead(pin)) + "\n");
    }
    else
    {
      handle_not_found();
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
    PostToSetup(i, gpio_arr[i]);
  }
  // シリアルコンソールのセットアップ
  Serial.begin(9600);
  delay(100);

  // WiFiに接続
  for (int i = 0; i < len_ssid; i++)
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