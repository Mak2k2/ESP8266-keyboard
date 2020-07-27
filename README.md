# ESP8266-keyboard
This is arduino sketch for esp8266-12e. For use as a keyboard you need connect to the ATmega16u2.
ATmega16u2 must be flashed with firmware "arduino-keyboard-0.3.tar.gz".
It should connect the port GPIO2 esp8266 to port 8(rx) ATmega16u2.
Transfer keys should be through telnet on port 23.
The approximate scheme:
![telnet_keyboard2](https://github.com/Mak2k2/ESP8266-keyboard/blob/master/31236607-42218e0a-a9fd-11e7-9f3c-baeca8538283.jpg)
