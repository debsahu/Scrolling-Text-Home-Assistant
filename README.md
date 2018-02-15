# Scrolling-Text-Home-Assistant
Using Dot Matrix display to show data from Home Assistant MQTT Stream

Hardware

1. Wemos D1 mini (ESP8266)
2. Max7219 Dot Matrix Display
3. LDR + 10 kOhm resistor

Software

* Setup Arduino IDE to be able to program an ESP8266 (Instructions on how to do this is here: https://github.com/esp8266/Arduino as well as in the video below).
* Install PubSubClient library
* Install NTPClientLib library
* Upload the code found here on your ESP8266

Make these following connections between Max 7219 display and Wemos D1:
* VCC -> 5V
* GND -> GND
* DIN -> D7
* CS -> D8
* CLK -> D5
