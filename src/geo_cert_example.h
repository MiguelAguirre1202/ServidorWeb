// Certificado autofirmado del WebUI de ubicación, igual en todos los equipos (EC P-256, CN/SAN = 192.168.4.1, 10 anos).
// Generado con openssl: req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes
//   -days 3650 -subj "/CN=192.168.4.1" -addext "subjectAltName=IP:192.168.4.1"
#pragma once

static const char GEO_CERT[]="";

static const char GEO_KEY[] ="";
