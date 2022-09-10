<h1 align="center">
   RESTuino
</h1>

<div align="center">
   A firmware for ESP32 to handle arduino GPIO via REST API  
</div>

# Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
    - [Control GPIO](#control-gpio)
    - [Control of the entire system](#control-of-the-entire-system)
    - [Connect to homebridge](#connect-to-homebridge)
- [Examples](#examples)
- [Videos](#videos)

# Features
![IMG_2850-アニメーションイメージ（大）](https://user-images.githubusercontent.com/22733958/188927218-d310dea3-8fe5-4b1a-8fdd-ffdac5e5f4da.gif)

🌱 `RESTuino` makes it possible to communicate *system functions & data* in the IoT.

✨ `RESTuino` allows us to make... 
- `IoT client` In this system, the arduino's GPIO is manipulated by the REST API.
- `Interactive Microcomputer Programming` via [curl](https://github.com/curl/curl), [Talend API Tester](https://chrome.google.com/webstore/detail/talend-api-tester-free-ed/aejoelaoggembcahagimdiliamlcdmfm?hl=ja) or similer.


✍ `RESTuino` can be operated with...
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

    - [PlatformIO VSCode extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide)  

    - [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html)
        ```sh
        $ pip install -U platformio
        ```

# Installation
1. Clone this repository.
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
    const char *ssid_def[] = {"ssid1", "ssid2"};
    const char *ssid_pass[] = {"pass1", "pass2"};
    ```
<!-- 
4. Clear EEPROM.
    ```sh
    # for esp32-devkit
    $ pio run -e esp32dev-setup -t upload
    # for MH ET LIVE ESP32DevKIT
    $ pio run -e mhetesp-setup -t upload
    ``` -->

3. Build and upload [RESTuino](src).
    ```sh
    # for esp32-devkit
    $ pio run -e esp32dev -t upload
    # for MH ET LIVE ESP32DevKIT
    $ pio run -e mhetesp -t upload
    ```

4. Clear EEPROM.

    Initialize the RESTuino system when you see `WiFi connected.` on the serial monitor.
    
    ```sh
    $ curl restuino.local/ -X DELETE  
    Change all pins to nan status...  
    ```  

6. Use this the way you want to use it.


# Usage

1. Specify the GPIO number in the URL
2. `POST` to define the pin status (digitalWrite, digitalRead, etc.)
3. `PUT` to update the value (HIGH for digitalWrite, etc.)
4. `GET` to obtain current information (digitalRead, etc.)
5. `DELETE` to disable the pin status

> **Note**   
> When sending the request body, always specify `Content-Type: text/plain` as the header.

## Control GPIO

### URL

http://restuino.local/gpio(pin_number)     

(pin_number) is the pin number of the GPIO. For example, `http://restuino.local/gpio15` is the GPIO15 of ESP32.

Specify the target GPIO pin by URL. 

### POST 
Use `POST` to set the status of a pin.       

Request body: `digitalRead` | `digitalWrite` | `analogRead` | `ledcWrite` | `Servo` | `(touch)` | `(dacWrite)`

```sh
# e.g. digitalWrite
$ curl restuino.local/gpio15 -X POST -H 'Content-Type: text/plain' -d 'digitalWrite'
digitalWrite
```

### PUT

Use `PUT` to change or define the output value of any pin.  
The following request body can be sent when the GPIO pin set by `POST` is in the following state.

- `digitalWrite`

    Request body: `HIGH` | `LOW` | `0` | `1`  

    ```sh
    # e.g.
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d 'LOW'
    LOW
    ```

- `ledcWrite` (alternative to `analogWrite` in esp32)  

    Request body: `0~256 numbers`  

    ```sh
    # e.g.
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d '100'
    100
    ```

- `Servo`  
    Request body: `0~180 numbers` | `switch`

    - `0~180 numbers`: a servo motor moves to the angle specified by value.  
    - `switch`: Each time the following command is sent, the servo motor moves back and forth between `angle` and `angle0`.  
    
    ```sh
    # e.g.
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d '88'
    88
    ```

### GET

Use `GET` to get the status of any pin.   
The following response body is received when the pin set by `POST` is in the following state.

- `analogRead`: `0~4095 numbers`
- `digitalRead`: `0` | `1`
- `Servo`: `0~180 numbers`

```sh
# e.g. digitalRead
$ curl restuino.local/gpio1 -X GET
0
```

### DELETE

Use `DELETE` to disable any pin (actually, save the setting in EEPROM and restart).

```sh
# e.g.
$ curl restuino.local/gpio1 -X DELETE
Change the pin to nan status... 
```


## Control of the entire system

### URL
- http://(IP_address)   
    The IP address can be obtained via serial communication or `http://restuino.local` via `GET` method (It's easy with a browser).

- http://restuino.local


### PUT

Request body: `save` | `load` | `reboot` 

- `save`: Save the current GPIO setting status to EEPROM.   
- `load`: Load GPIO settings stored in EEPROM and reflect to actual pin settings.
- `reboot`: Reboot ESP32.

> **Note**   
> The `load` is done automatically at startup, but the user must `save` before turning off the power.  
> When operating as an IoT client, once the pin state is saved, it will be saved in eeprom, so there is no need to `save` after that.

```sh
# e.g.
$ curl restuino.local/ -X POST -H 'Content-Type: text/plain' -d 'save'
Wrote
```

### GET

Obtain IP address and GPIO status. GPIO status is output in chunks of 40. Starting from the first left digit, GPIO0, 1,2.... and its value is defined as follows.
```sh
$ curl restuino.local/ -X GET
{
  "IP address": "192.168.0.29",
  "GPIO status": "2000000050000002200000000000000000000000"
}
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

### DELETE

Disable all pins (actually, save the setting in EEPROM and restart).

```sh
$ curl restuino.local/ -X DELETE
Change all pins to nan status...
```

### POST

Not defined.

## Connect to [homebridge](https://github.com/homebridge/homebridge)

In the homebridge config editor, add the following to the `accessories` section:
```json
"accessories": [
    {
        "accessory": "CMD",
        "name": "light",
        "on_cmd": "curl restuino.local -X PUT -H 'Content-Type: text/plain' -d '56'",
        "off_cmd": "curl restuino.local -X PUT -H 'Content-Type: text/plain' -d '6'"
    }
]
```

https://user-images.githubusercontent.com/22733958/189354008-0065546a-124d-4562-aaaa-19c268d264d2.mov


# Examples

See [examples](examples/).

## Python 

- [python-L-chika](examples/L-chika/) : Programs to turn on/off the LED connected to GPIO15 every second.

- [python-Servo-switch](examples/Servo-switch/) : Programs to move the servo (connected to GPIO15) to the angle defined by `angle`, `angle0` every second.

1. Run setup.py
    ```sh
    $ python setup.py
    ```
    Configure GPIO settings. Run it only once.

2. Run main.py
    ```sh
    $ python main.py
    ```
    This is the main program. Use only this program when incorporating it.

The program is [here](examples/L-chika/).

https://user-images.githubusercontent.com/22733958/189347822-e8469ee5-c92b-40f8-a5f1-4262459ac20f.mp4

## curl

- [curl-digitalRead](examples/digitalRead_with_curl.sh): Scripts that executes `digitalRead` to GPIO15 and obtains its status.

- [curl-Servo](examples/digitalRead_with_curl.sh): Programs to move the servo (connected to GPIO15) to 88°.

1. Run setup.sh
    ```sh
    $ chmod +x setup.sh
    $ ./setup.sh
    ```
    Configure GPIO settings. Run it only once.

2. Run main.sh
    ```sh
    $ chmod +x main.sh
    $ ./main.sh
    ```
    This is the main program. Use only this program when incorporating it.

The program is [here](examples/digitalRead_with_curl.sh).

https://user-images.githubusercontent.com/22733958/189353947-962c44ff-effe-4be4-992b-4562dd6cd37f.mp4


## Talend API tester

    https://user-images.githubusercontent.com/22733958/189336576-649f115f-5116-4f43-890a-9500fc9b182a.mp4


