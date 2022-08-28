<h1 align="center">
   RESTuino
</h1>

<div align="center">
   A firmware for ESP32 to handle arduino GPIOs via REST API  
</div>


# Features
Until now, IoT systems have only communicated *data*. RESTuino makes it possible to communicate *system functions* in the IoT.

`RESTuino` allows us to make the following: 
- `Interactive Microcomputer Programming` via [curl](https://github.com/curl/curl), [Talend API Tester](https://chrome.google.com/webstore/detail/talend-api-tester-free-ed/aejoelaoggembcahagimdiliamlcdmfm?hl=ja) or similer.
- `IoT client`. In this system, the arduino's GPIO is manipulated by the REST API.

`RESTuino` can be operated with
- [curl](https://github.com/curl/curl)
- [Talend API Tester](https://chrome.google.com/webstore/detail/talend-api-tester-free-ed/aejoelaoggembcahagimdiliamlcdmfm?hl=ja)
- [Homebridge](https://github.com/homebridge/homebridge)
- [Python(requests)](https://requests.readthedocs.io/en/latest/)  
or similer.


# Requirements
- ESP32-based board  

    The tested boards are as follows. Other products can also be used if they are equipped with esp32.  
    - [ESP32 DevKitC](https://www.espressif.com/en/products/devkits/esp32-devkitc) 
    - [MH ET LIVE ESP32DevKIT](https://ja.aliexpress.com/item/32880702799.html?spm=a2g0s.8937460.0.0.72832e0edJMMVm&gatewayAdapt=glo2jpn)    

- PlatformIO

# Installation
1. Clone the repository
    ```sh
    $ git clone https://github.com/takeyamayuki/RESTuino.git
    ```
2. Define `ssid`, `password` of your wifi router by changing [ssid_define.cpp](src/ssid_define.cpp).
    ```sh
    $ cd RESTuino
    $ vi src/ssid_define.cpp
    ``` 
    Change `*ssid_def[]`, `*ssid_pass[]` to your wifi SSID, password and `len_ssid` to the number of ssid, password sets you defined.
    ```cpp 
    uint8_t len_ssid = 2;
    const char *ssid_def[] = {"your first SSID here", "your second SSID here"};
    const char *ssid_pass[] = {"your first password here", "your second password here"};
    ```

4. Clear EEPROM.
    ```sh
    # for esp32-devkit
    $ pio run -e esp32dev-setup -t upload
    # for MH ET LIVE ESP32DevKIT
    $ pio run -e mhetesp-setup -t upload
    ```

5. Build and upload [RESTuino](src).
    ```sh
    # for esp32-devkit
    $ pio run -e esp32dev -t upload
    # for MH ET LIVE ESP32DevKIT
    $ pio run -e mhetesp -t upload
    ```

6. Install this system wherever you like!  


# URI
## root
- http://(IP_address)   
    The IP address can be obtained via serial communication or `http://restuino.local` via `GET` method.

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

Request body:  
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

### PUT  
Use `PUT` to change or define the output value of any pin.
- digitalWrite

    Request body: `HIGH` or `LOW` or `0` or `1`  
    Available pins: 0-5, 12-19, 21-23, 25-27, 32-33.  

    e.g.
    ```sh
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d 'LOW'
    ```
- ledcWrite(alternative to `analogRead` in esp32)  
    Request body: `0~256 numbers`  
    Available pins: 0, 2, 4, 12-15, 25-27, 32, 33. 

    e.g.
    ```sh
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d '100'
    ```
- Servo  
    Request body: `0~180 numbers` or `switch`
    - `0~180 numbers`: a servo motor moves to the angle specified by value.  
    - `switch`: Each time the following command is sent, the servo motor moves back and forth between angle and angle0.  
    
    e.g.
    ```sh
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d '88'
    ```

### GET
Use `GET` to get the status of any pin.  
Of course, the following information can also be obtained by opening any URI in a browser.
- analogRead

    Available pins: 0, 2, 4, 12-15, 25-27, 32-36, 39.  

    e.g.
    ```sh
    $ curl restuino.local/gpio0 -X GET
    100
    ```

- digitalRead  

    Available pins: 1, 2, 4, 5, 7, 8, 12-19, 21-23, 25-27, 32-37.   

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
Request body: `save` or `reboot` or `reflect`
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


# Tasks
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

# Contribution
There are no specific guidelines for contributions, so please feel free to send me pull requests and issues.

