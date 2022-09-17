#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include "ssid_define.hpp"
#include "restuino_func.hpp"


void setup()
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

void loop()
{
  server.handleClient();
}