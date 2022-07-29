# RESTuino

## Usage

### build and upload
1. Clone the repository
    ```
    $ git clone https://github.com/takeyamayuki/RESTful-servo-motor.git
    ```
2. Define `ssid`, `password` of your wifi router by creating your own header file.
    ```sh
    $ cd RESTful-servo-motor
    $ vi src/ssid_define.h
    ``` 
    Change MY_SSID, MY_SSID_PASS to your wifi SSID, password.
    ```cpp
    #ifndef _SSID_DEFINE_
    #define _SSID_DEFINE_

    #define MY_SSID "your ssid here"
    #define MY_SSID_PASS "your ssid password here"

    #endif
    ```
3. Change `upload_port`, `monitor_port` in [platformio.ini](platformio.ini) to your own.
4. Install `ESP32Servo` on PlatformIO
4. Build and upload using platformIO.
5. Install this system wherever you like!  


### PUT method

`Free angle mode` (Request body={0~180 arbitrary number})

```sh
# a servo motor moves to the angle specified by 'angle'
$ curl {your IP address here} -X PUT -H 'Content-Type: text/plain' -d 'angle'

# e.g.
$ curl 192.168.0.28 -X PUT -H 'Content-Type: text/plain' -d '120'
$ curl rsm.local -X PUT -H 'Content-Type: text/plain' -d '120'
# rsm means RESTful-servo-motor
```

`SwitchBot mode` (Request body='switch')  

```sh
# Each time the following command is sent, the servo motor moves back and forth between angle and angle0
$ curl {your IP address here} -X PUT -H 'Content-Type: text/plain' -d 'switch'

# e.g.
$ curl 192.168.0.28 -X PUT -H 'Content-Type: text/plain' -d 'switch'
$ curl rsm.local -X PUT -H 'Content-Type: text/plain' -d 'switch'
```

### GET method

```sh
# Get the current angle.
$ curl {your IP address here} -X GET
# e.g.
$ curl rsm.local
$ curl rsm.local -X GET
```
or you can browse current servo angle by using browser.

http://rsm.local

<img width="751" alt="スクリーンショット 2022-05-21 午後1 59 16" src="https://user-images.githubusercontent.com/22733958/169636157-6faa444a-d949-414c-90da-792bb149acd0.png">

