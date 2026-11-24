// Página de captura de ubicación (GeoHttps). Sin gzip: ~5 KB.
#pragma once

static const char GEO_PAGE[] = R"HTML(<!doctype html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Ubicación del equipo</title>
<style>
body{font-family:system-ui,sans-serif;margin:0;padding:16px;background:#f4f6f9;color:#1d2733}
.c{max-width:420px;margin:auto;background:#fff;border-radius:10px;padding:16px;box-shadow:0 1px 4px #0002}
h1{font-size:1.2em;color:#2666af;margin:0 0 12px}
button{width:100%;padding:12px;margin:8px 0 0;border:0;border-radius:8px;font-size:1em;background:#2666af;color:#fff}
button.s{background:#2e7d32}
label{display:block;margin-top:10px;font-size:.9em}
input{width:100%;box-sizing:border-box;padding:8px;font-size:1em;border:1px solid #c5ccd6;border-radius:6px}
#m{margin-top:12px;font-size:.9em}#cur{font-size:.85em;color:#55606e}
a{display:block;margin-top:14px;text-align:center;color:#2666af}
.err{color:#c62828}.ok{color:#2e7d32}
</style></head><body><div class="c">
<h1>Ubicación del equipo</h1>
<div id="cur">Guardada en el equipo: ...</div>
<button onclick="geo()">Obtener ubicación de este dispositivo</button>
<label>Latitud<input id="lat" inputmode="decimal" placeholder="4.694367"></label>
<label>Longitud<input id="lon" inputmode="decimal" placeholder="-74.042511"></label>
<label>Precisión (m)<input id="acc" readonly></label>
<button class="s" onclick="save()">Guardar en el equipo</button>
<div id="m"></div>
<a id="back" href="#">Volver a la configuración</a></div>
<script>
var src = 'manual', w = null, best = null;
function $(i) { return document.getElementById(i); }
function msg(t, c) { $('m').textContent = t; $('m').className = c || ''; }
$('back').href = 'http://' + location.hostname + '/modemConfiguration';
$('lat').oninput = $('lon').oninput = function () { src = 'manual'; $('acc').value = ''; };
fetch('/ubicacion/estado', { cache: 'no-store' }).then(function (r) { return r.json(); }).then(function (j) {
  $('cur').textContent = (j.lat || j.lon) ? 'Guardada en el equipo: ' + j.lat.toFixed(6) + ', ' + j.lon.toFixed(6)
                                          : 'El equipo todavía no tiene ubicación';
}).catch(function () {});

function geo() {
  if (!navigator.geolocation || !window.isSecureContext) {
    msg('Este navegador no entrega la ubicación aquí: escriba las coordenadas', 'err'); return;
  }
  if (w !== null) navigator.geolocation.clearWatch(w);
  best = null;
  var t0 = Date.now();
  msg('Buscando ubicación... (al aire libre es más rápido)');
  w = navigator.geolocation.watchPosition(function (p) {
    var c = p.coords;
    if (best && c.accuracy >= best.accuracy) return;   // quedarse con la más precisa
    best = c;
    $('lat').value = c.latitude.toFixed(6);
    $('lon').value = c.longitude.toFixed(6);
    $('acc').value = Math.round(c.accuracy);
    src = 'navegador';
    msg('Precisión ±' + Math.round(c.accuracy) + ' m a los ' + Math.round((Date.now() - t0) / 1000) +
        ' s. Espere a que baje y luego guarde.', 'ok');
    if (c.accuracy <= 10) { navigator.geolocation.clearWatch(w); w = null; }
  }, function (e) {
    msg(['', 'Permiso denegado: habilite la ubicación para este sitio',
          'Ubicación no disponible (sin GPS o sin señal)',
          'Tiempo agotado: intente al aire libre'][e.code] || e.message, 'err');
  }, { enableHighAccuracy: true, timeout: 60000, maximumAge: 0 });
}

function save() {
  var la = parseFloat($('lat').value), lo = parseFloat($('lon').value);
  if (!(la >= -90 && la <= 90) || !(lo >= -180 && lo <= 180)) { msg('Latitud o longitud inválida', 'err'); return; }
  if (w !== null) { navigator.geolocation.clearWatch(w); w = null; }
  var body = 'lat=' + la.toFixed(6) + '&lon=' + lo.toFixed(6) + '&acc=' + ($('acc').value || '') + '&src=' + src;
  fetch('/ubicacion', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: body })
    .then(function (r) {
      if (!r.ok) { msg('El equipo respondió ' + r.status, 'err'); return; }
      $('cur').textContent = 'Guardada en el equipo: ' + la.toFixed(6) + ', ' + lo.toFixed(6);
      msg('Guardado. Esta página se cierra en el equipo en unos segundos; vuelva a la configuración.', 'ok');
    })
    .catch(function () { msg('No se pudo contactar el equipo', 'err'); });
}
</script></body></html>
)HTML";
