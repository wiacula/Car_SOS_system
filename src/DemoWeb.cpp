#include "DemoWeb.h"
#include "config.h"
#include "EmergencyNumbers.h"

#include <WebServer.h>
#include <WiFi.h>
#include <vector>

static WebServer server(80);
static String json_state_ = "{}";
static String event_log_ = "";

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="pl">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>System ratunkowy</title>
<style>
 body{font-family:sans-serif;background:#0b1220;color:#e8eefc;margin:24px}
 .card{background:#162033;padding:16px;border-radius:12px;max-width:520px}
 .ok{color:#6cffb2} .bad{color:#ff6b7a} .warn{color:#ffd36b}
 button{font-size:16px;margin:8px 8px 0 0;padding:10px 14px;border-radius:8px;border:0;cursor:pointer}
 #sos{background:#c62828;color:#fff} #cancel{background:#455a64;color:#fff}
 pre{white-space:pre-wrap;background:#0b1220;padding:8px;border-radius:8px}
 .reading{background:#0b1220;padding:12px;border-radius:8px}
 .reading strong{display:block;font-size:28px;margin-top:4px}
</style>
</head>
<body>
<div class="card">
 <h2>Inteligentny system ratunkowy</h2>
 <p id="st">laczenie...</p>
 <p id="m"></p>
 <div class="reading">
  <span>Temperatura kabiny</span>
  <strong id="temperature">Oczekiwanie na DHT11...</strong>
 </div>
 <h3>Diagnostyka GPS na zywo</h3>
 <p id="gpsdiag">Oczekiwanie na dane GPS...</p>
 <h3>Numery alarmowe SMS</h3>
 <form id="contact-form">
  <input id="contact-number" type="tel" placeholder="+48123456789"
         pattern="\+[0-9]{7,15}" required/>
  <button type="submit">Dodaj numer</button>
 </form>
 <p id="contact-error" class="bad"></p>
 <ul id="contacts"></ul>
 <button id="sos" onclick="fetch('/sos')">SOS reczny</button>
 <button id="cancel" onclick="fetch('/cancel')">Anuluj / reset</button>
 <h3>Dziennik</h3>
 <pre id="log"></pre>
</div>
<script>
async function tick(){
 const r=await fetch('/state'); const j=await r.json();
 const st=document.getElementById('st');
 st.className=j.alarm?'bad':'ok';
 st.textContent=j.state+'  |  '+j.kind;
 document.getElementById('m').textContent=
  'g='+j.g.toFixed(2)+'  roll='+j.roll.toFixed(1)+'  pitch='+j.pitch.toFixed(1)+
  '  odliczanie='+j.countdown+'s\n'+j.gps;
 document.getElementById('temperature').textContent=j.temperature_valid
  ? j.temperature_c.toFixed(1)+' °C'
  : 'Brak odczytu z DHT11';
 document.getElementById('gpsdiag').textContent=j.gps_diag;
 document.getElementById('log').textContent=j.log||'';
}
async function loadContacts(){
 const r=await fetch('/contacts'); const j=await r.json();
 if(!r.ok) throw new Error(j.error||'Nie udalo sie pobrac numerow');
 const list=document.getElementById('contacts');
 list.replaceChildren();
 for(const number of j){
  const item=document.createElement('li');
  const label=document.createElement('span');
  label.textContent=number+' ';
  const button=document.createElement('button');
  button.textContent='Usun';
  button.onclick=()=>changeContact('/contacts/remove',number);
  item.append(label,button);
  list.append(item);
 }
}
async function changeContact(path,number){
 const error=document.getElementById('contact-error');
 try{
  const r=await fetch(path,{method:'POST',
   headers:{'Content-Type':'application/x-www-form-urlencoded'},
   body:new URLSearchParams({number})});
  const j=await r.json();
  if(!r.ok) throw new Error(j.error||'Nie udalo sie zapisac numeru');
  error.textContent='';
  await loadContacts();
 }catch(e){error.textContent=e.message;}
}
document.getElementById('contact-form').onsubmit=async e=>{
 e.preventDefault();
 const input=document.getElementById('contact-number');
 await changeContact('/contacts/add',input.value);
 if(!document.getElementById('contact-error').textContent) input.value='';
};
loadContacts().catch(e=>document.getElementById('contact-error').textContent=e.message);
setInterval(tick,400); tick();
</script>
</body>
</html>
)HTML";

extern void demoRequestSos();
extern void demoRequestCancel();

void demoWebBegin() {
  if (!WIFI_DEMO_ENABLED) {
    return;
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);

  server.on("/contacts", HTTP_GET, []() {
    std::vector<String> numbers;
    if (!loadEmergencyNumbers(numbers)) {
      server.send(500, "application/json", "{\"error\":\"Blad odczytu pamieci\"}");
      return;
    }
    String json = "[";
    for (size_t i = 0; i < numbers.size(); ++i) {
      if (i > 0) {
        json += ",";
      }
      json += "\"";
      json += numbers[i];
      json += "\"";
    }
    json += "]";
    server.send(200, "application/json", json);
  });
  server.on("/contacts/add", HTTP_POST, []() {
    const EmergencyNumberResult result =
        addEmergencyNumber(server.arg("number"));
    if (result == EmergencyNumberResult::Success) {
      server.send(200, "application/json", "{\"ok\":true}");
      return;
    }
    if (result == EmergencyNumberResult::StorageError) {
      server.send(500, "application/json", "{\"error\":\"Blad zapisu pamieci\"}");
      return;
    }
    if (result == EmergencyNumberResult::InvalidNumber) {
      server.send(400, "application/json", "{\"error\":\"Podaj numer w formacie +48123456789\"}");
      return;
    }
    if (result == EmergencyNumberResult::Duplicate) {
      server.send(409, "application/json", "{\"error\":\"Ten numer juz jest na liscie\"}");
      return;
    }
    server.send(409, "application/json", "{\"error\":\"Lista numerow jest pelna (maks. 10)\"}");
  });
  server.on("/contacts/remove", HTTP_POST, []() {
    const EmergencyNumberResult result =
        removeEmergencyNumber(server.arg("number"));
    if (result == EmergencyNumberResult::Success) {
      server.send(200, "application/json", "{\"ok\":true}");
      return;
    }
    if (result == EmergencyNumberResult::StorageError) {
      server.send(500, "application/json", "{\"error\":\"Blad zapisu pamieci\"}");
      return;
    }
    server.send(404, "application/json", "{\"error\":\"Nie znaleziono numeru\"}");
  });
  server.on("/", []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/state", []() { server.send(200, "application/json", json_state_); });
  server.on("/sos", []() {
    demoRequestSos();
    server.send(200, "text/plain", "SOS");
  });
  server.on("/cancel", []() {
    demoRequestCancel();
    server.send(200, "text/plain", "CANCEL");
  });
  server.begin();
}

void demoWebLoop() {
  if (!WIFI_DEMO_ENABLED) {
    return;
  }
  server.handleClient();
}

static const char* stateName(SystemState s) {
  switch (s) {
    case SystemState::Boot:
      return "BOOT";
    case SystemState::Ready:
      return "GOTOWY";
    case SystemState::Countdown:
      return "ODLICZANIE";
    case SystemState::Sending:
      return "WYSYLKA";
    case SystemState::Sent:
      return "WYSŁANO";
    case SystemState::Fault:
      return "BLAD";
  }
  return "?";
}

void demoWebPush(SystemState state,
                 CrashKind kind,
                 float g,
                 float roll,
                 float pitch,
                 const String& gps,
                 const String& gpsDiagnostics,
                 float temperatureC,
                 bool temperatureValid,
                 int countdown_s,
                 const String& logLine) {
  if (logLine.length() > 0) {
    event_log_ = logLine + "\n" + event_log_;
    if (event_log_.length() > 1200) {
      event_log_.remove(1200);
    }
  }

  json_state_ = "{";
  json_state_ += "\"state\":\"";
  json_state_ += stateName(state);
  json_state_ += "\",\"kind\":\"";
  json_state_ += crashKindName(kind);
  json_state_ += "\",\"g\":";
  json_state_ += String(g, 3);
  json_state_ += ",\"roll\":";
  json_state_ += String(roll, 2);
  json_state_ += ",\"pitch\":";
  json_state_ += String(pitch, 2);
  json_state_ += ",\"countdown\":";
  json_state_ += String(countdown_s);
  json_state_ += ",\"alarm\":";
  json_state_ += (state == SystemState::Ready || state == SystemState::Boot ||
                  state == SystemState::Fault)
                     ? "false"
                     : "true";
  json_state_ += ",\"gps\":\"";
  String gesc = gps;
  gesc.replace("\"", "'");
  json_state_ += gesc;
  json_state_ += "\",\"gps_diag\":\"";
  String gpsDiagEsc = gpsDiagnostics;
  gpsDiagEsc.replace("\"", "'");
  json_state_ += gpsDiagEsc;
  json_state_ += "\",\"temperature_valid\":";
  json_state_ += temperatureValid ? "true" : "false";
  json_state_ += ",\"temperature_c\":";
  if (temperatureValid) {
    json_state_ += String(temperatureC, 1);
  } else {
    json_state_ += "null";
  }
  json_state_ += ",\"log\":\"";
  String lesc = event_log_;
  lesc.replace("\\", "\\\\");
  lesc.replace("\"", "'");
  lesc.replace("\n", "\\n");
  json_state_ += lesc;
  json_state_ += "\"}";
}
