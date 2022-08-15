# RESTuino
An arduino application for ESP32 to handle arduino GPIOs via REST API.

# Features
Until now, IoT systems have only communicated  `data`. RESTuino makes it possible to communicate `system functions` in the IoT.

It allows interactive programming via [curl](https://github.com/curl/curl), [Talend API Tester](https://chrome.google.com/webstore/detail/talend-api-tester-free-ed/aejoelaoggembcahagimdiliamlcdmfm?hl=ja) or similer as well as serving as an IoT client!
In this system, the arduino's GPIO is mainly manipulated by the REST API.

Use the aforementioned [curl](https://github.com/curl/curl), [Talend API Tester](https://chrome.google.com/webstore/detail/talend-api-tester-free-ed/aejoelaoggembcahagimdiliamlcdmfm?hl=ja) and [IFTTT(webhooks)](https://ifttt.com/maker_webhooks), [Python(requests)](https://requests.readthedocs.io/en/latest/), etc. to operate.


# Requirements
- ESP32-based board  
    > The tested boards are as follows.
    > - [ESP32 Dev Board](https://www.espressif.com/en/products/esp32-devkit)
    > - [MH ET LIVE ESP32DevKIT](https://ja.aliexpress.com/item/32880702799.html?spm=a2g0s.8937460.0.0.72832e0edJMMVm&gatewayAdapt=glo2jpn)  

    > Other products can also be used if they are equipped with esp32.

- PlatformIO

# Installation
1. Clone the repository
    ```
    $ git clone https://github.com/takeyamayuki/RESTuino.git
    ```
2. Define `ssid`, `password` of your wifi router by changing `ssid_define.cpp`.
    ```sh
    $ cd RESTuino
    $ vi src/ssid_define.cpp
    ``` 
    Change `*ssid_def[]`, `*ssid_pass[]` to your wifi SSID, password and `len_ssid` to the number of ssid,password sets you defined.
    ```cpp 
    int len_ssid = 2;
    const char *ssid_def[] = {"your first SSID here", "your second SSID here"};
    const char *ssid_pass[] = {"your first password here", "your second password here"};
    ```
3. Change `upload_port`, `monitor_port` in [platformio.ini](platformio.ini) to your own.
    ```sh
    $ ls /dev/tty.*
    /dev/tty.BLTH       /dev/tty.usbmodem529A0097081
    /dev/tty.Bluetooth-Incoming-Port	/dev/tty.wchusbserial529A0097081
    $ cd ..
    $ vi platformio.ini
    ```

    ```platformio.ini
    upload_port = /dev/tty.wchusbserial529A0097081
    monitor_port = /dev/tty.wchusbserial529A0097081
    ```
5. Build and upload using platformIO.
6. Install this system wherever you like!  


# URI
## root
- http://(IP_address)   
    The IP address can be obtained via serial communication or `http://restuino.local` with a browser.

- http://restuino.local

## GPIO access

- http://restuino.local/gpio(pin_number)     
    (pin_number) is the pin number of the GPIO.  For example, `http://restuino.local/gpio15` is the GPIO15 of ESP32.


# RESTful API
> **Note**   
> When sending the request body, always specify `Content-Type: text/plain` as the header.


## @`http://restuino.local/gpio(pin_number)`  
Specify the target GPIO pin by URI. 

### POST  
Use `POST` to set the status of a pin in the same way as arduino.        
request body:  
- `digitalRead`  
- `digitalWrite`  
- `analogRead`
- `ledcWrite`
- `Servo`
- `(touch)`
- `(dacWrite)`

e.g. digtalWrite
```sh
$ curl restuino.local/gpio15 -X POST -H 'Content-Type: text/plain' -d 'digitalWrite'
```

### `PUT`  
Use `PUT` to change or define the output value of any pin.
- digitalWrite  
    request body: `HIGH` or `LOW` or `0` or `1`  
    available pins: 0-5, 12-19, 21-23, 25-27, 32-33.  
    e.g.
    ```sh
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d 'LOW'
    ```
- ledcWrite(alternative to `analogRead` in esp32)  
    request body: `0~256 numbers`  
    available pins: 0, 2, 4, 12-15, 25-27, 32, 33.  
    e.g.
    ```sh
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d '100'
    ```
- Servo  
    request body: `0~180 numbers` or `switch`
    - 0~180 numbers: a servo motor moves to the angle specified by value.  
    - 'switch': Each time the following command is sent, the servo motor moves back and forth between angle and angle0.  
    
    e.g.
    ```sh
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d '88'
    ```

### `GET`
Use `GET` to get the status of any pin.  
Of course, the following information can also be obtained by opening any URI in a browser.
- analogRead  
    available pins: 0, 2, 4, 12-15, 25-27, 32-36, 39.  
    e.g.
    ```sh
    $ curl restuino.local/gpio0 -X GET
    100
    ```

- digitalRead  
    available pins: 1, 2, 4, 5, 7, 8, 12-19, 21-23, 25-27, 32-37.   
    e.g.
    ```sh
    $ curl restuino.local/gpio1 -X GET
    0
    ```
- Servo  
    e.g.
    ```sh
    $ curl restuino.local/gpio1 -X GET
    0
    ```

### `DELETE`
Use `DELETE` to disable any pin (actually, save the setting in EEPROM and restart).

## @`http://restuino.local/`

### `POST`  
request body: `save` or `reboot` or `reflect`
```sh
$ curl restuino.local/ -X POST -H 'Content-Type: text/plain' -d 'save|reboot|reflect'
```
- `save`: Save the current GPIO setting status to EEPROM.   
- `reboot`: Reboot ESP32.
- `reflect`: Reflect GPIO settings stored in EEPROM.

### `GET`  
Obtain IP address and GPIO status. GPIO status is output in chunks of 40. Starting from the first left digit, GPIO0, 1,2.... and its value is defined as follows.
```sh
$ curl restuino.local/ -X GET
IP address: 192.168.3.20
GPIO status: 2000000050000005000000000000000000000000
```

|  number  |  meaning  |
| ---- | ---- |
|  0  |  nan  |
|  1  |  digitalread  |
|  2  |  digitalwrite  |
|  3  |  analogread  |
|  4  |  ledcwrite  |
|  5  |  servo  |
|  6  |  (touch)  |
|  7  |  (dacwrite)  |

e.g.   
Looking at the first digit of GPIO status(GPIO0), there is a number 2, which means `digitalwrite`. Therefore, `pinMode(0,OUTPUT)` is executed internally so that `GPIO0` becomes `digitalwrite`.


# tasks
- [x] digitalread  
- [ ] pullupread  
- [x] digitalwrite PUT  
- [ ] digitalwrite GET  
- [x] analogread  
- [x] ledcwrite (analogwrite for esp32)  
- [x] servo  
- [ ] touch  
- [ ] dacwrite  
- [x] eeprom save of settings   
- [x] eeprom load of settings   
- [x] Apply settings saved in eeprom   
- [ ] Multiple servo, multiple ledcwrite   