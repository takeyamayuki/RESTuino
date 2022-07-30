# RESTuino
An arduino application for ESP32 to handle arduino GPIOs via REST API.

## design concept
- Interactive microcomputer programming
- Applications for IoT clients

## features
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
- [ ] Apply settings saved in eeprom   
- [ ] Multiple servo, multiple ledcwrite   

## Usage

### build and upload
1. Clone the repository
    ```
    $ git clone https://github.com/takeyamayuki/RESTuino.git
    ```
2. Define `ssid`, `password` of your wifi router by creating your own header file.
    ```sh
    $ cd RESTuino
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
    ```sh
    $ ls /dev/tty.*
    /dev/tty.BLTH				        /dev/tty.usbmodem529A0097081
    /dev/tty.Bluetooth-Incoming-Port	/dev/tty.wchusbserial529A0097081
    ```
    ```platformio.ini
    upload_port = /dev/tty.wchusbserial529A0097081
    monitor_port = /dev/tty.wchusbserial529A0097081
    ```
4. Install `ESP32Servo` on PlatformIO
4. Build and upload using platformIO.
5. Install this system wherever you like!  

## URI
### root
- http://{IP_address}  
    The IP address can be obtained via serial communication immediately after startup of this program.

- http://restuino.local

### GPIO access

- http://restuino.local/gpio{pin_number}   
    {pin_number} is the pin number of the GPIO.  For example, `http://restuino.local/gpio15` is the GPIO15 of ESP32.



## RESTful API
1. Use `POST` to set the status of a pin in the same way as arduino.  

    Specify the target GPIO pin by URI. The items to be set in the request body are all lower-case and specify arduino function names. For now, the following functions are available.  

    - digitalread
    - digitalwrite
    - analogread
    - servo
    - ledcwrite

    For example, digtalWrite
    ```sh
    $ curl restuino.local/gpio15 -X POST -H 'Content-Type: text/plain' -d 'digitalwrite'
    ```

2. Use `PUT` to change the output value of any pin.
- digitalwrite  
    ```sh
    $ curl restuino.local/gpio{i} -X PUT -H 'Content-Type: text/plain' -d 'HIGH|LOW'
    # i(digitalWrite enabled pin)=0, 1, 2, 3, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
    ```
    e.g.
    ```sh
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d 'LOW'
    ```
- ledcwrite
    ```sh
    $ curl restuino.local/gpio{j} -X PUT -H 'Content-Type: text/plain' -d '{value:0-256}'
    # j(ledclWrite enabled pin)=0, 2, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33
    ```
    e.g.
    ```sh
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d '100'
    ```
- servo
    ```sh
    $ curl restuino.local/gpio{k} -X PUT -H 'Content-Type: text/plain' -d '{value:0-180}|"switch"'
    # {value:0-180}:a servo motor moves to the angle specified by value.
    # "switch": Each time the following command is sent, the servo motor moves back and forth between angle and angle0.
    ```
    e.g.
    ```sh
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d '88'
    ```
- root
    ```sh
    $ curl restuino.local/gpio15 -X PUT -H 'Content-Type: text/plain' -d 'save|reboot'
    ```
    Save the current pin status to EEPROM with `save`.   
    Reboot with `reboot`.

3. Use `GET` to get the status of any pin.  

    Of course, the following information can also be obtained by opening any URI in a browser.
- analogRead
    ```sh
    # l(analogRead enabled pin)=0, 2, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33, 34, 35, 36, 39
    $ curl restuino.local/gpio{l}
    ```

- digitalRead
    ```sh
    # m(digitalRead enabled pin)=1, 2, 4, 5, 7, 8, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 37
    $ curl restuino.local/gpio{m}
    ```
- root
    ```sh
    $ curl restuino.local
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    # Returns the status of all GPIO pins. GPIO0-GPIO39 are separated by spaces.
    ```
4. Use `DELETE` to disable any pin (actually, save the setting in EEPROM and restart).



