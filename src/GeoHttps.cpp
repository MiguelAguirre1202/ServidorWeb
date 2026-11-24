#if defined(ESP32)
#include "GeoHttps.h"
// #include "geo_cert.h"
#include "geo_cert_example.h"
#include "geo_page.h"
#include <esp_log.h>
#include "mbedtls/base64.h"
#include "mbedtls/sha1.h"

GeoHttpsClass GeoHttps;

static portMUX_TYPE geoMux = portMUX_INITIALIZER_UNLOCKED;

void GeoHttpsClass::begin(const char* user, const char* pass_sha1_hex) {
  strlcpy(user_, user ? user : "", sizeof(user_));
  strlcpy(pass_sha1_, pass_sha1_hex ? pass_sha1_hex : "", sizeof(pass_sha1_));
}

// ------------------------------------------------------------------ autenticación (igual que el WebUI)
bool GeoHttpsClass::authorized(httpd_req_t* req) {
  if (!user_[0] || !pass_sha1_[0]) return true;  // WebUI sin login configurado
  char hdr[160];
  if (httpd_req_get_hdr_value_str(req, "Authorization", hdr, sizeof(hdr)) != ESP_OK) return false;
  if (strncmp(hdr, "Basic ", 6) != 0) return false;
  unsigned char dec[100];
  size_t n = 0;
  if (mbedtls_base64_decode(dec, sizeof(dec) - 1, &n, (const unsigned char*)hdr + 6, strlen(hdr + 6)) != 0) return false;
  dec[n] = '\0';
  char* sep = strchr((char*)dec, ':');
  if (!sep) return false;
  *sep = '\0';
  if (strcmp((char*)dec, user_) != 0) return false;

  unsigned char sha[20];
  mbedtls_sha1((const unsigned char*)(sep + 1), strlen(sep + 1), sha);
  char hex[41];
  for (int i = 0; i < 20; i++) snprintf(hex + 2 * i, 3, "%02x", sha[i]);
  return strcasecmp(hex, pass_sha1_) == 0;
}

esp_err_t GeoHttpsClass::askLogin(httpd_req_t* req) {
  httpd_resp_set_status(req, "401 Unauthorized");
  httpd_resp_set_hdr(req, "WWW-Authenticate", "Basic realm=\"Login Required\"");
  return httpd_resp_sendstr(req, "Login requerido");
}

// ------------------------------------------------------------------ rutas
esp_err_t GeoHttpsClass::onRoot(httpd_req_t* req) {
  httpd_resp_set_status(req, "302 Found");
  httpd_resp_set_hdr(req, "Location", "/ubicacion");
  return httpd_resp_send(req, NULL, 0);
}

esp_err_t GeoHttpsClass::onPage(httpd_req_t* req) {
  GeoHttps.last_ms_ = millis();
  if (!GeoHttps.authorized(req)) return GeoHttps.askLogin(req);
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, GEO_PAGE, HTTPD_RESP_USE_STRLEN);
}

esp_err_t GeoHttpsClass::onState(httpd_req_t* req) {  // {"lat":4.694367,"lon":-74.042511}
  GeoHttps.last_ms_ = millis();
  if (!GeoHttps.authorized(req)) return GeoHttps.askLogin(req);
  char json[64];
  snprintf(json, sizeof(json), "{\"lat\":%.6f,\"lon\":%.6f}", GeoHttps.cur_lat_ / 1e6, GeoHttps.cur_lon_ / 1e6);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_sendstr(req, json);
}

static bool geo_parse_deg(const char* s, double lim, int32_t& out_e6) {
  char* end;
  const double d = strtod(s, &end);
  if (end == s || *end != '\0' || d < -lim || d > lim) return false;
  out_e6 = (int32_t)lround(d * 1e6);
  return true;
}

// POST /ubicacion  cuerpo "lat=4.694367&lon=-74.042511&acc=12&src=navegador"
esp_err_t GeoHttpsClass::onSave(httpd_req_t* req) {
  GeoHttps.last_ms_ = millis();
  if (!GeoHttps.authorized(req)) return GeoHttps.askLogin(req);
  char body[128];
  const int len = (int)req->content_len;
  if (len <= 0 || len >= (int)sizeof(body)) return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "cuerpo invalido");
  int got = 0;
  while (got < len) {
    const int r = httpd_req_recv(req, body + got, len - got);
    if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
    if (r <= 0) return ESP_FAIL;
    got += r;
  }
  body[got] = '\0';

  char lat[24] = "", lon[24] = "", acc[12] = "", src[16] = "manual";
  httpd_query_key_value(body, "lat", lat, sizeof(lat));
  httpd_query_key_value(body, "lon", lon, sizeof(lon));
  httpd_query_key_value(body, "acc", acc, sizeof(acc));
  httpd_query_key_value(body, "src", src, sizeof(src));
  int32_t lat_e6, lon_e6;
  if (!geo_parse_deg(lat, 90.0, lat_e6) || !geo_parse_deg(lon, 180.0, lon_e6)) {
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "latitud o longitud invalida");
  }
  const int32_t acc_m = acc[0] ? (int32_t)atol(acc) : -1;
  const bool ok = GeoHttps.cb_ ? GeoHttps.cb_(lat_e6, lon_e6, acc_m, src) : true;
  if (!ok) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no se pudo guardar");
  GeoHttps.cur_lat_  = lat_e6;
  GeoHttps.cur_lon_  = lon_e6;
  GeoHttps.saved_ms_ = millis() | 1;
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_sendstr(req, "{\"ok\":true}");
}

// ------------------------------------------------------------------ ciclo de vida
bool GeoHttpsClass::start() {
  last_ms_  = millis();
  saved_ms_ = 0;
  if (https_) return true;

  // El navegador corta el handshake mientras muestra la advertencia del certificado: no ensuciar el log.
  esp_log_level_set("esp-tls-mbedtls", ESP_LOG_NONE);
  esp_log_level_set("esp_https_server", ESP_LOG_NONE);
  esp_log_level_set("httpd", ESP_LOG_NONE);

  const uint32_t heap0 = ESP.getFreeHeap();
  httpd_ssl_config_t c = HTTPD_SSL_CONFIG_DEFAULT();
  c.servercert     = (const uint8_t*)GEO_CERT;
  c.servercert_len = sizeof(GEO_CERT);  // PEM: el largo incluye el '\0'
  c.prvtkey_pem    = (const uint8_t*)GEO_KEY;
  c.prvtkey_len    = sizeof(GEO_KEY);
  c.httpd.max_open_sockets = 2;
  c.httpd.lru_purge_enable = true;
  c.httpd.ctrl_port        = 32769;     // distinto del de cualquier otro esp_http_server

  httpd_handle_t h = NULL;
  const esp_err_t e = httpd_ssl_start(&h, &c);
  if (e != ESP_OK) {
    Serial.printf(">>GEO: no se pudo iniciar HTTPS: %s (heap libre %u)\n", esp_err_to_name(e), (unsigned)heap0);
    return false;
  }
  httpd_uri_t u = {};
  u.uri = "/";                  u.method = HTTP_GET;  u.handler = onRoot;  httpd_register_uri_handler(h, &u);
  u.uri = "/ubicacion";         u.method = HTTP_GET;  u.handler = onPage;  httpd_register_uri_handler(h, &u);
  u.uri = "/ubicacion/estado";  u.method = HTTP_GET;  u.handler = onState; httpd_register_uri_handler(h, &u);
  u.uri = "/ubicacion";         u.method = HTTP_POST; u.handler = onSave;  httpd_register_uri_handler(h, &u);
  https_ = h;
  Serial.printf(">>GEO: HTTPS encendido (heap libre %u -> %u)\n", (unsigned)heap0, (unsigned)ESP.getFreeHeap());
  return true;
}

void GeoHttpsClass::stop() {
  portENTER_CRITICAL(&geoMux);  // quien llegue primero se queda con el handle: nunca se detiene dos veces
  httpd_handle_t h = https_;
  https_ = NULL;
  portEXIT_CRITICAL(&geoMux);
  if (!h) return;
  httpd_ssl_stop(h);
  Serial.printf(">>GEO: HTTPS apagado (heap libre %u)\n", (unsigned)ESP.getFreeHeap());
}

void GeoHttpsClass::loop() {
  if (!https_) return;
  const uint32_t now = millis();
  if (saved_ms_ && now - saved_ms_ >= 3000) { stop(); return; }   // margen para que la respuesta salga
  if (now - last_ms_ >= idle_ms_) stop();
}

#endif  // ESP32
