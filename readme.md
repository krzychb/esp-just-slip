#esp-just-slip

[SLIP (Serial Line Internet Protocol)](https://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol) provides easy to implement method to encapsulate data packets sent over a serial connection. The esp-just-slip is an implementation of SLIP prepared for [ESP8266 SoC](http://espressif.com/en/products/esp8266/). It is provided as C module [justslip](justslip/) and includes verification of data integrity with crc16. Sample implementation of this module is avialable in file [user_main.c](user/user_main.c) and demonstrates sending of data packets between arduino and ESP8266.



##Implementation

esp-just-slip is using [esp8266 software uart](https://github.com/plieningerweb/esp8266-software-uart) by [Andreas Plieninger](https://github.com/plieningerweb) to provide flexibility which pins to use for serial communication. With simple code modification esp-just-slip may be switched to using ESP8266’s hardware UART. To provide verification if data has been transmitted correctly [CRC16](https://en.wikipedia.org/wiki/Crc16) is used.
 


##Sample Test Configuration

Testing of this application may be perfomed with sample h/w set up defined below:



##Hardware
* Arduino Pro Mini 3.3V
* ESP-12E LoLin V3 with serial by wemos.cc
* Two USB-UART generic dongles used to
  * monitor serial trafic received by ESP-12E
  * download Arduino and then monitor serial trafic received by Arduino

![alt Sample configuration - components](documents/just-slip-components.jpg)

The following pins are used to connect particular modules:

Module   | Pin Desription | Pin No. | <> | Pin No. | Pin Desription | Module
-------- | -------------- | ------- |----| --------|----------------| ---------------
ESP-12E  | Soft UART Rx   | GIPIO14 | <> | 11      | Soft UART Tx   | Arduino
ESP-12E  | Soft UART Tx   | GIPIO15 | <> | 10      | Soft UART Rx   | Arduino
ESP-12E  | Ground         | GND     | <> | GND     | Ground         | Arduino
ESP-12E  | UART1 Tx       | GPIO02  | <> | Rx      | UART Rx        | USB-UART ESP 
ESP-12E  | Ground         | GND     | <> | GND     | Ground         | USB-UART ESP 
Arduino  | UART Tx        | TXO     | <> | Rx      | UART Rx        | USB-UART INO 
Arduino  | UART Rx        | RXI     | <> | Tx      | UART Tx        | USB-UART INO 
Arduino  | Reset          | RST     | <> | DTR     | UART DTR       | USB-UART INO 
Arduino  | Power Supply   | RAW     | <> | 5V      | Power Supply   | USB-UART INO 
Arduino  | Ground         | GND     | <> | GND     | Ground         | USB-UART INO 

Please note that most of Arduino modules operate on 5V while ESP8266 on 3.3V To make the connection easeir Arduin Pro Mini 3.3V has been used that provides direct compatibility of pin voltage to ESP8266. In other cases a 5V <> 3.3V voltage level shifter should be used or ESP8266 may be damaged if connected directly to 5V Arduino pins. 

![alt Sample configuration - connections](documents/just-slip-connections.jpg)
Please refer to folder [documents](documents/) for additinal pictures of connections.



##Software Overview
ESP8255 application should be compiled and loaded to module using provided make file. Arduino should be loaded using [Arduino IDE](https://www.arduino.cc/en/Main/Software). The sketch is available in folder [ino-just-slip](ino-just-slip/). Functionally of this sketch is identicatl to the application loaded on ESP8266. There are some (slight) differences in code as Arduino is using C++ while ESP8266 is using pure C. 

Once applications are started the following information is displayed on [Arduino Serial](documents/just-slip-arduino-terminal.png) and [ES8266 UART1](documents/just-slip-esp8266-terminal.png) terminals:
 
``` 
...
4F 2E 00 00 AB E6 71 77 4E E0 76 D8 :  2% 
50 2E 00 00 23 80 4D 60 3D B5 DA C8 :  2% 
51 2E 00 00 51 3C 15 59 C6 E6 32 0E :  2% 
...
```
First four columns contain couter of packets received, next 8 bytes contain random data and last column contains percentage of packets lost or currupt. Packets are sent eveery 30 ms at 57600 bps. Checking of UART input buffer for data is done every 10 ms. Basing on observation of the above sample h/w configuration, on averagw about 1% of packets received by Aruino are lost or currupt. On ESP8266 side this value is below 0.5% (0% reported on terminal). The reason of bigger number of packet  lost/corrupt on Arduino side is likely ESP8266 WiFi routines that interrupt operation of SoftUART sending the packets. This is only a hypothesis that should be verified by specific testing.


The following s/w version have been used when developing and testing of esp-just-slip:
* [Unofficial Development Kit for Espressif ESP8266](http://programs74.ru/udkew-en.html) v2.0.8 (esp_iot_sdk_v1.3.0_15_08_08)
* [Arduino iDE 1.6.5](https://www.arduino.cc/en/Main/Software)
* [Arduino SoftwareSerial 1.0.0](https://www.arduino.cc/en/Reference/SoftwareSerial) - avialable through Arduino iDE > Sketch > Include Library > Manage Libraries



##Software API

The following functions are implemented:
```c
uint8_t ICACHE_FLASH_ATTR slipDecodeSerial(Softuart *softuart, uint8_t *dataBuffer);
void ICACHE_FLASH_ATTR slipEncodeSerial(Softuart *softuart, uint8_t *dataBuffer, uint8_t nCount);
uint8_t ICACHE_FLASH_ATTR readKeyboard(uint8_t *dataBuffer);
uint8_t ICACHE_FLASH_ATTR appendCrc16(uint8_t *dataBuffer, uint8_t nCount);
bool ICACHE_FLASH_ATTR checkCrc16(uint8_t *dataBuffer, uint8_t nCount);
```

###Read and Decode Data
```c
//
// *softuart - pointer to software UART
// *dataBuffer - pointer to data buffer to store received data
//
// returned value - number of bytes read to dataBuffer
//
uint8_t ICACHE_FLASH_ATTR slipDecodeSerial(Softuart *softuart, uint8_t *dataBuffer)
```
Read SLIP encoded data from software serial port, decode them and store in data buffer.


###Encode and Write Data
```c
//
// *softuart - pointer to software UART
// *dataBuffer - pointer to data buffer to read data from
// nCount - number of bytes to read form dataBuffer
//
void ICACHE_FLASH_ATTR slipEncodeSerial(Softuart *softuart, uint8_t *dataBuffer, uint8_t nCount)
```
SLIP encode data taken from data buffer and send the over the software serial port.


###Append CRC16 to Data
```c
//
// *dataBuffer - pointer to data buffer to calculate and append crc16
// nCount - number of data bytes in dataBuffer
//
// returned value - number of bytes in data buffer including crc16
//
uint8_t ICACHE_FLASH_ATTR appendCrc16(uint8_t *dataBuffer, uint8_t nCount)
```
Calculate CRC16 for data contained in data buffer and append result to the end of data buffer


###Check Data Integrity
```c
//
// *dataBuffer - pointer to data buffer
// nCount - number of data bytes in dataBuffer including crc16
//
// returned value - true of false depending on result of crc16 check
//
bool ICACHE_FLASH_ATTR checkCrc16(uint8_t *dataBuffer, uint8_t nCount)
```
Perfrom data integrity check by comparing CRC16 calulated for input data against CRC16 received.



##Acknowledgments

Development of this esp-just-slip was done using the following resources:

* [esp8266 software uart](https://github.com/plieningerweb/esp8266-software-uart) by [Andreas Plieninger](https://github.com/plieningerweb)
* [Unofficial Development Kit for Espressif ESP8266 by Mikhail Grigorev](http://programs74.ru/udkew-en.html)

Thank you guys for your contribution to ESP8266 community. It is a great pleasure to work with applications you have developed.



##Contributing

Please report any [issues](https://github.com/krzychb/esp-just-slip/issues) and submit [pull requests](https://github.com/krzychb/esp-just-slip/pulls). Feel free to contribute to the project in any way you like! 



##Author

krzychb



##Donations

Invite me to freshly squizzed orange juice.


##LICENSE - "MIT License"

Copyright (c) 2015 krzychb

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


