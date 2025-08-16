
# Web Server Gatria 

This Server provides user iterface to upload over-the-air (OTA)  firmware updates to your hardware with precise status and progress. It also handles MAC addresses for SPNOW communication between microcontrollers.


## Features

- Quick & simple OTA procedure
- Get useful insight on progress and status of your OTA update
-  Ready to use within 3 lines of code
- No need to learn HTML/CSS/JS
- Credential control
- Mac address management
- Visualization of spnow connectivity stage


## Documentation

[Documentation](https://linktodocumentation)


## Installation

__For Arduino IDE:__

For Windows:

- Download the [Repository.](https://github.com/MiguelAguirre1202/ServidorWeb.git)

- Extract the .zip in 
`Documents > Arduino > Libraries > {Place "ServidorWeb" folder Here}`.

For Linux:

- Download the [Repository.](https://github.com/MiguelAguirre1202/ServidorWeb.git)

- Extract the .zip in `Sketchbook > Libraries > {Place "ServidorWeb" folder Here}`

__For Plarformio:__

To include this library and be able to use it, it is necessary to define it in the platformio.ini in the following wing:

```bash
  lib_deps = 
		https://github.com/MiguelAguirre1202/ServidorWeb.git
```

It is recommended to update the libraries with the following command in the terminal:

```bash
  pio pkg update
```
    
## Integration Guide

Integrating Web Server in your existing code is pretty simple. This guide assumes that you already have a simple webserver code prepared and you just need to inject the following lines in your existing code:

__1. Include Dependency__
    
At the very beginning of sketch include the ElegantOTA library.

```bash
 #include <ElegantOTA.h>
```

__2. Add `begin` function__

Now add the `begin` function of ElegantOTA in setup block of your sketch. This will inject ElegantOTA routes and logic into the web server.

```bash
ElegantOTA.begin(&server);
```

__3. Add `loop` function__

Last part is to call the `loop` function of ElegantOTA in loop block of your sketch. This loop block is necessary for ElegantOTA to handle reboot after OTA update.

```bash
ElegantOTA.loop();
```

__4. Final Code__

This is how a ready to use example will look like. After uploading the code to your platform, you can access ElegantOTA portal on `http://<YOUR_DEVICE_IP>/update`

```javascript
 
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <WiFiClient.h>
  #include <ESP8266WebServer.h>
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <WebServer.h>
#endif
 
#include <ElegantOTA.h>
 
const char* ssid = "........";
const char* password = "........";
 
#if defined(ESP8266)
  ESP8266WebServer server(80);
#elif defined(ESP32)
  WebServer server(80);
#endif
 
void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  server.on("/", []() {
    server.send(200, "text/plain", "Hi! This is ElegantOTA Demo.");
  });
 
  ElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}
 
void loop(void) {
  server.handleClient();
  ElegantOTA.loop();
}
}
```


## Author

- [@ayushsharma82](https://github.com/ayushsharma82)

- [@MiguelAngel1202](https://www.github.com/octokatherine)

