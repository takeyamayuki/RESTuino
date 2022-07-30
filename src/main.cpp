#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <ESPmDNS.h>
#include <EEPROM.h>

#include <ssid_define.h>

WebServer server(80);

const char *ssid = MY_SSID;          // 自分のSSIDに書き換える
const char *password = MY_SSID_PASS; // 自分のパスワードに書き換える
const char *host_name = "restuino";  // RESTful servo motor

// String target; // この変数をPUTメソッドで書き換える
// String req;    // この変数をPOSTメソッドで書き換える

/* 各種アクション */
const int dnan = 0;
const int ddigitalread = 1;
const int ddigitalwrite = 2;
const int danalogread = 3;
const int dledcwrite = 4;
const int dservo = 5;

const int n = 50;
int gpio_arr[n] = {}; //すべて0で初期化

/* servo */
Servo servo1;
// Published values for SG90 servos; adjust if needed
int minUs = 0;
int maxUs = 5000;
bool to0_flag = false;
// angle > angle0 >= 0にすること
int angle = 60;
int angle0 = 5;

// to0_flag check
void flag_check()
{
  // to0_flag
  int status = servo1.read();
  if (status > (angle + angle0) / 2)
    to0_flag = true; //現在angle側なので0に持っていく
  else if (status < (angle + angle0) / 2)
    to0_flag = false; //現在angle0側なので30に持っていく
}

// mode=true:angle0,angleのスイッチ, mode=false:自由角度への移動
void move_sg90(bool mode, int k)
{
  if (mode)
  {
    flag_check();
    if (to0_flag)
      servo1.write(angle0);
    else
      servo1.write(angle);
  }
  else
    servo1.write(k);
}

void handleRoot(void)
{
  // server.send(200, "text/plain", "Nice!!\n");
  if (server.method() == HTTP_PUT)
  {
    String req = server.arg("plain"); // get request body
    if (req == "reboot")
    {
      server.send(200, "text/plain", "Rebooting...\n");
      ESP.restart();
    }
    else if (req == "save")
    {
      server.send(200, "text/plain", "EEPROM write\n");
      int rom_ad = 0;
      for (int i = 0; i < n; i++)
      {
        EEPROM.put(rom_ad, gpio_arr[i]);
        rom_ad += 4;
      }
      EEPROM.commit();
    }
  }
  else if (server.method() == HTTP_GET)
  {
    int rom_ad = 0;
    String mes;
    // server.send(100, "text/plain", "EEPROM reading...\n");
    // mes += "status ";
    for (int i = 0; i < 40; i++)//GPIO:0-39
    {
      EEPROM.get(rom_ad, gpio_arr[i]);
      mes += String(gpio_arr[i]) + " ";
      rom_ad += 4;
    }
    mes += "\n";
    server.send(200, "text/plain", mes);
    // pinに反映させる
  }
}

void handleNotFound(void)
{
  server.send(404, "text/plain", "Not Found.\n");
}

// すべてのGPIOを制御
void handle_gpio(int pin)
{
  int rom_ad = pin * 4;
  String message = "OK ";
  /* POST ペリフェラル初期設定 */
  // request body = servoだったら, servoの設定 & pin*4のアドレスのEEPROMに0(servo)を格納
  if (server.method() == HTTP_POST)
  {
    String req = server.arg("plain"); // get request body

    if (req == "servo") // servo
    {
      servo1.setPeriodHertz(50); // Standard 50hz servo
      servo1.attach(pin, minUs, maxUs);
      flag_check();
      gpio_arr[pin] = dservo;
    }
    // ledcWrite enable pin
    // 0, 2, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33
    else if (req == "ledcwrite")
    {
      ledcSetup(0, 12800, 8);
      ledcAttachPin(pin, 0);
      gpio_arr[pin] = dledcwrite;
    }
    // digitalWrite enable pin
    // 0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
    else if (req == "digitalwrite")
    {
      pinMode(pin, OUTPUT);
      gpio_arr[pin] = ddigitalwrite;
    }
    // digitalRead enable pin
    // 1, 2, 4, 5, 7, 8, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 37
    else if (req == "digitalread")
    {
      pinMode(pin, INPUT);
      gpio_arr[pin] = ddigitalread;
    }
    // analogRead anable pin
    // 0, 2, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33, 34, 35, 36, 39
    else if (req == "analogread")
    {
      if (gpio_arr[pin] != danalogread)
      {
        // EEPROM書き込み
        ESP.restart();
      }
      gpio_arr[pin] = danalogread;
    }
    else
      handleNotFound();
    message += req + "\n";
    server.send(200, "text/plain", message);
  }

  /* PUT 更新 */
  // request bodyへの変更
  else if (server.method() == HTTP_PUT)
  {
    String target = server.arg("plain"); // server.arg("plain")でリクエストボディが取れる。targetに格納
    if (gpio_arr[pin] == dservo)
    {
      if (target == "switch")
        move_sg90(true, 0);
      else
        move_sg90(false, target.toInt());
      server.send(200, "text/plain", "servo.write " + target + "\n"); //リクエストされた角度を返す
    }
    else if (gpio_arr[pin] == dledcwrite)
    {
      ledcWrite(0, target.toInt());
      server.send(200, "text/plain", "dledcWrite " + target + "\n");
    }
    else if (gpio_arr[pin] == ddigitalwrite)
    {
      if (target == "HIGH")
        digitalWrite(pin, HIGH);
      else if (target == "LOW")
        digitalWrite(pin, LOW);
      server.send(200, "text/plain", "digitalWrite " + target + "\n");
    }
    else
      handleNotFound();
  }
  /* GET 情報取得 */
  else if (server.method() == HTTP_GET)
  {
    if (gpio_arr[pin] == dservo)
      server.send(200, "text/plain", String(servo1.read()) + "\n"); // statusをクライアントに返す
    else if (gpio_arr[pin] == ddigitalread)
      server.send(200, "text/plain", String(digitalRead(pin)) + "\n");
    else if (gpio_arr[pin] == danalogread)
      server.send(200, "text/plain", String(analogRead(pin)) + "\n");
    else
      handleNotFound();
  }
}

void setup()
{
  // EEPROM setup
  EEPROM.begin(1024); // 1kB     156byte=39*4

  // シリアルコンソールのセットアップ
  Serial.begin(9600);
  delay(100);
  Serial.println();

  // WiFiに接続
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  MDNS.begin(host_name); // ホスト名 host_name.local
  Serial.println();
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // root
  server.on("/", handleRoot);

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
  server.onNotFound(handleNotFound);

  server.begin();
}

void loop()
{
  server.handleClient();
}