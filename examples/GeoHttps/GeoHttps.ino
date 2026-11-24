/*
  GeoHttps — ejemplo del módulo GeoHttps (ubicación del equipo con el GPS del celular/PC, por HTTPS).

  1. Conectarse al WiFi "GeoTest" (clave 12345678).
  2. Abrir http://192.168.4.1 -> /geo/start enciende el HTTPS y redirige a https://192.168.4.1/ubicacion.
  3. Aceptar la advertencia del certificado, "Obtener ubicación" y "Guardar".
  4. El HTTPS se apaga solo 3 s después de guardar, o tras 5 min sin uso.

  En el producto, /geo/start va en el WebServer del WebUI y pide el mismo login (ElegantOTA.authenticateSha1()).
*/
#include <WiFi.h>
#include <WebServer.h>
#include <GeoHttps.h>

static WebServer http(80);
static int32_t   lat_e6 = 0, lon_e6 = 0;  // en el producto: gp_clat / gp_clon en NVM

static bool onLocation(int32_t lat, int32_t lon, int32_t acc_m, const char* src) {
  Serial.printf(">>GEO: lat=%.6f lon=%.6f precision=%ld m origen=%s\n", lat / 1e6, lon / 1e6, (long)acc_m, src);
  lat_e6 = lat;
  lon_e6 = lon;
  return true;  // false = no se pudo guardar (la página muestra el error)
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  WiFi.softAP("GeoTest", "12345678");
  Serial.printf(">>GEO: AP en %s, heap libre %u\n", WiFi.softAPIP().toString().c_str(), (unsigned)ESP.getFreeHeap());

  GeoHttps.begin("", "");  // sin login en el ejemplo; en el producto: usuario y clave SHA1 del WebUI
  GeoHttps.onLocation(onLocation);

  http.on("/", HTTP_GET, []() {
    http.sendHeader("Location", "/geo/start");
    http.send(302, "text/plain", "");
  });
  http.on("/geo/start", HTTP_GET, []() {
    GeoHttps.setCurrent(lat_e6, lon_e6);
    if (!GeoHttps.start()) {
      http.send(503, "text/plain; charset=utf-8", "No se pudo iniciar HTTPS (memoria)");
      return;
    }
    http.sendHeader("Location", "https://" + http.hostHeader() + "/ubicacion");
    http.send(302, "text/plain", "");
  });
  http.begin();
}

void loop() {
  http.handleClient();
  GeoHttps.loop();
  delay(2);
}
