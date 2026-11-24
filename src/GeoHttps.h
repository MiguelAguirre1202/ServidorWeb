/*
  GeoHttps — página HTTPS para capturar la ubicación del equipo con el GPS del celular/PC.

  El navegador solo entrega la ubicación (Geolocation API) en páginas seguras, por eso esta página va
  por un servidor HTTPS aparte (puerto 443, esp_https_server de ESP-IDF) y el WebUI normal sigue en HTTP.
  Certificado autofirmado fijo (geo_cert.h): el navegador muestra una advertencia la primera vez.

  RAM: ~16 KB con el servidor encendido + ~45 KB por sesión TLS abierta (máx. 2). Por eso está apagado
  por defecto: start() desde el WebUI (/geo/start), y se apaga solo tras guardar o tras el tiempo sin uso.

  Uso (solo ESP32):
    GeoHttps.begin(user, pass_sha1_hex);       // mismas credenciales del WebUI (clave guardada en SHA1 hex)
    GeoHttps.onLocation(cb);                   // cb corre en la tarea del servidor HTTPS
    GeoHttps.setCurrent(lat_e6, lon_e6);       // lo que la página muestra al abrir
    GeoHttps.start(); / GeoHttps.stop();
    GeoHttps.loop();                           // en el mismo lugar que ElegantOTA.loop()
*/
#ifndef GeoHttps_h
#define GeoHttps_h

#if defined(ESP32)
#include <Arduino.h>
#include <esp_https_server.h>

class GeoHttpsClass {
 public:
  // lat/lon en micro-grados; acc_m = precisión en metros (-1 = no informada); src = "navegador" o "manual".
  // Devolver false si no se pudo guardar (la página muestra el error).
  typedef bool (*LocationCb)(int32_t lat_e6, int32_t lon_e6, int32_t acc_m, const char* src);

  void begin(const char* user, const char* pass_sha1_hex);
  void onLocation(LocationCb cb) { cb_ = cb; }
  void setCurrent(int32_t lat_e6, int32_t lon_e6) { cur_lat_ = lat_e6; cur_lon_ = lon_e6; }
  void setIdleTimeout(uint32_t ms) { idle_ms_ = ms; }

  bool start();          // enciende el HTTPS; si ya está encendido solo renueva el tiempo sin uso
  void stop();           // seguro de llamar desde cualquier tarea, también si ya está apagado
  bool running() const { return https_ != NULL; }
  void loop();           // apaga tras guardar (con 3 s de margen) o tras idle_ms_ sin peticiones

 private:
  static esp_err_t onRoot(httpd_req_t* req);
  static esp_err_t onPage(httpd_req_t* req);
  static esp_err_t onState(httpd_req_t* req);
  static esp_err_t onSave(httpd_req_t* req);
  bool authorized(httpd_req_t* req);
  esp_err_t askLogin(httpd_req_t* req);

  httpd_handle_t    https_ = NULL;
  LocationCb        cb_ = NULL;
  char              user_[33] = "";
  char              pass_sha1_[41] = "";
  volatile int32_t  cur_lat_ = 0;
  volatile int32_t  cur_lon_ = 0;
  uint32_t          idle_ms_ = 5UL * 60UL * 1000UL;
  volatile uint32_t last_ms_ = 0;     // última petición (la escribe la tarea del servidor)
  volatile uint32_t saved_ms_ = 0;    // != 0: se guardó, apagar 3 s después
};

extern GeoHttpsClass GeoHttps;

#endif  // ESP32
#endif  // GeoHttps_h
