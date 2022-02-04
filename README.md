# WebhooksTriggeredESP32WiFi

*Turn your ESP32 into an easy to use and manage wireless micro web-server allowing it to process reliably and asynchronouly incoming http/https requests (webhooks).*

Features:

* **User-friendly auto WiFi configuration:** Built-in admin web portal for fast and easy configuration without technical skills (Soft AP mode).
* **Accepts http and https requests**
* **Configure any GPIO you like** 
* **The configured GPIOs are by automatically strapped-in:** Turn any GPIO HIGH or LOW (`http[s]://{IP address}/toggle?gpio={number}&action={0|1}`), Toggle any GPIO for a given duration (`http://{IP address}/toggle?gpio={number}&duration={milliseconds}`)
* **Non-blocking:** Uses Async Web Server making it multi-threaded, super quick and responsive. 

The code can be re-used as boilerplate to build your own self-hosted low-energy, uncensorable webserver (Async web server).
Why reinvent the wheel?
Just branch this project and start from there with all you need to get running in no time.

### Connects to the WiFi in no time, in whatever environment, every time

WiFi configuration is always a pain in the back on electronic projects: They don't have a keyboard, often not even a screen, a dozens of clunky approaches for the use case "get wifi up".  

**A consistent, reliable, user-friendly way is now available!**

This project uses the library ESPAsync_WiFiManager_Lite [details](https://github.com/khoih-prog/ESPAsync_WiFiManager_Lite). It is compatible for the ESP32/ESP8266 boards and also allows to persist data (WiFi credentials and whatever extra parameters you'd like) in EEPROM/SPIFFS/LittleFS for easy configuration/reconfiguration and autoconnect/autoreconnect of WiFi and other services without Hardcoding.
  
### Installation
This project consists in 4 files: 
* 1 source code file (WebhooksTriggeredESP32WiFi.ino)
* * Use it as is but you could as well adapt the code to your needs.
* 3 header files (defines.h, Credentials.h, dynamicParams.h)
* * Contains various self-explainatory configuration parameters that you can adjust to your needs.

### Accessing the built-in admin web portal
This behaviour can be changed in the .h file.

You need to get the unit out of normal operations (station mode) and enter "Soft AP mode" to access its built-in admin web portal:
* Press the RST button twice within less than 10 seconds.
* The unit will start in "Soft AP mode" 
* You can now connect the WiFi of your laptop or handheld device to it.
* Access the built-in admin portal at the url `http://192.168.4.42`

### Configuration
* Access the built-in web portal.
* Modify the parameters of your choice.
* Click "save".
* The unit has stored your parameters and resumed normal operations (station mode).
