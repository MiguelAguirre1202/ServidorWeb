# CLAUDE.md

Este archivo proporciona orientación a Claude Code (claude.ai/code) al trabajar con el código de este repositorio.

## Descripción general del proyecto

ElegantOTA es una librería de Arduino para actualizaciones de firmware Over-The-Air (OTA) con interfaz web. Soporta ESP8266, ESP32, RP2040 y RP2350. Este fork (por MiguelAguirre1202/Gatria SAS) extiende el original con un portal de configuración de modem y una interfaz industrial personalizada.

## Sistema de compilación

Este proyecto usa **PlatformIO**. El `platformio.ini` define `lib_dir = .` (la raíz del repositorio actúa como librería) y `src_dir = examples/Demo` (o `examples/AsyncDemo`).

```bash
# Compilar
pio build -e esp32      # ESP32-S3
pio build -e esp8266    # ESP8266
pio build -e picow      # Raspberry Pi Pico W
pio build -e pico2w     # Raspberry Pi Pico 2W

# Compilar y cargar
pio run -e esp32 -t upload

# Monitorear salida serial
pio device monitor
```

Para alternar entre los ejemplos de servidor web síncrono y asíncrono, editar `src_dir` en `platformio.ini`.

## Archivos fuente principales

| Archivo | Propósito |
|---------|-----------|
| `src/ElegantOTA.h` | API pública — declaración de clase, macros de plataforma, enums de modo |
| `src/ElegantOTA.cpp` | Implementación principal — manejadores de rutas HTTP, lógica de actualización OTA |
| `src/elop.h` | Declara los arreglos de bytes HTML embebidos (`ELEGANT_HTML`, `CONFIG_MODEM_HTML`) |
| `src/elop.cpp` | Define el contenido HTML comprimido como arreglos `uint8_t` |

## Recursos HTML embebidos

Las interfaces web se almacenan como **arreglos de bytes comprimidos con gzip** en `src/elop.cpp` y se declaran en `src/elop.h`. Se sirven con `Content-Encoding: gzip`.

- `ELEGANT_HTML[84912]` → servido en `GET /update` (portal de actualización OTA)
- `CONFIG_MODEM_HTML[56394]` → servido en `GET /modemConfiguration` (página de configuración del modem)

**Al actualizar un archivo HTML:** Después de editar los `.html` en `src/`, se debe:
1. Comprimir el archivo HTML con gzip
2. Convertirlo a un arreglo de bytes en C
3. Reemplazar el arreglo correspondiente en `src/elop.cpp`
4. Actualizar la constante del tamaño del arreglo en `src/elop.h` con el nuevo conteo de bytes

Los archivos HTML fuente son:
- `src/FocusLiteModem_WebUI_and_OTA.html` — genera `ELEGANT_HTML`
- `src/ConfiguracionModem_WebUI_OTA.html` — genera `CONFIG_MODEM_HTML`

## Arquitectura

**Soporte dual de servidor web** controlado por `#define ELEGANTOTA_USE_ASYNC_WEBSERVER`:
- `0` (por defecto): `WebServer` / `ESP8266WebServer` síncrono
- `1`: `ESPAsyncWebServer` — requerido para el ejemplo `AsyncDemo`

**Abstracción de plataforma** mediante guardas de preprocesador (`#if defined(TARGET_RP2040)`, `ESP8266`, `ESP32`) en `ElegantOTA.cpp` para selección de sistema de archivos (LittleFS vs SPIFFS) y tipos de partición OTA.

**Flujo OTA:**
1. `GET /ota/start?mode=firmware|fs` — inicializa `Update` con hash MD5
2. `POST /ota/upload` — recibe binario en fragmentos, llama a `Update.write()`
3. Al completar: dispara el callback `onEnd`, opcionalmente reinicia automáticamente (vía `loop()`)

**API de callbacks:**
```cpp
ElegantOTA.begin(&server);           // registra todas las rutas
ElegantOTA.onStart([](){});          // llamado antes de iniciar OTA
ElegantOTA.onProgress([](size_t cur, size_t total){});
ElegantOTA.onEnd([](bool success){}); // llamado al completar OTA
ElegantOTA.loop();                   // debe llamarse en loop() para el manejo del reinicio
```

## Tecnología de la interfaz web

Las interfaces HTML usan **Tailwind CSS** (minificado, embebido inline) con la marca Gatria SAS (`#2666af`). Las UIs se comunican con el dispositivo mediante llamadas `fetch()` a los endpoints del firmware.

## Simulador de pantalla OLED

La página principal (`FocusLiteModem_WebUI_and_OTA.html`) incluye un widget que simula visualmente la pantalla OLED física del dispositivo (resolución 128×64 px). El widget está ubicado entre el encabezado de marca y los acordeones de configuración.

### Pantallas del simulador

| Estado (`state`) | Descripción | Información mostrada |
|-----------------|-------------|----------------------|
| `boot` | Inicialización del dispositivo | Nombre del producto, versiones HW/FW, texto parpadeante |
| `connecting` | Conectando a la red celular | Operador, señal, APN, SIM, modo |
| `waiting` | Esperando llamada entrante | Operador, señal, IP, puerto, modo |
| `in_call` | Llamada/sesión activa | Operador, señal, IP, protocolo, modo |
| `error` | Error de red | Indicador de error en rojo, SIM, modo |

### Ciclo de simulación (demo)

En modo demo, el widget cicla automáticamente entre estados con estos tiempos:

```
boot (2.5s) → connecting (3s) → waiting (5s) → in_call (5s) → waiting (4s) → ...
```

### Estructura de datos del OLED

```javascript
window.oledData = {
  state:    'waiting',     // boot | connecting | waiting | in_call | error
  operator: 'CLARO',       // nombre del operador celular
  rssi:     3,             // barras de señal (0–4)
  apn:      'internet.claro.co',
  ip:       '10.82.45.67',
  port:     5000,
  sim:      1,             // número de SIM activa
  mode:     'NORMAL',      // NORMAL | TRANSPARENTE
  protocol: 'MODBUS RTU',  // protocolo de comunicación con el medidor
  fw:       'v1.0.0',
  hw:       'v1.0'
};
```

### API pública para integración con el firmware

```javascript
window.updateOLED(data);
```

Llamar esta función fusiona `data` con `window.oledData` y redibuja el widget. Al llamarla, el ciclo de simulación se mantiene activo pero el estado cambia al valor provisto. Para integración real, el firmware debe exponer un endpoint (p. ej. `/oled-status`) que retorne JSON con los campos anteriores, y el frontend debe llamar `window.updateOLED(responseData)` con polling o WebSocket.

### Clases CSS relevantes

| Clase | Propósito |
|-------|-----------|
| `.oled-widget` | Contenedor externo del widget |
| `.oled-frame` | Carcasa física simulada (fondo gris oscuro) |
| `.oled-screen` | Pantalla negra con texto verde `#00e676` y efecto de fósforo |
| `.oled-hdr` | Cabecera con nombre del dispositivo y operador/señal |
| `.oled-sig` | Barras de señal animadas |
| `.oled-status` | Línea de estado centrada en negrita |
| `.oled-sep` | Separador horizontal |
| `.oled-row` | Fila etiqueta/valor |
| `.oled-blink` | Animación de parpadeo (step, 1s) |
| `.oled-err` | Color rojo para estados de error |
