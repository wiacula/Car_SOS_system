#include "DemoWeb.h"
#include "config.h"
#include "EmergencyNumbers.h"

#include <WebServer.h>
#include <WiFi.h>
#include <vector>

static WebServer server(80);
static String json_state_ =
    "{\"state\":\"URUCHAMIANIE\",\"kind\":\"BRAK\",\"g\":0,\"roll\":0,"
    "\"pitch\":0,\"yaw\":0,\"countdown\":0,\"alarm\":false,"
    "\"gps\":\"GPS: BRAK FIX\",\"gps_map_url\":\"\","
    "\"gps_diag\":\"Oczekiwanie na dane GPS\","
    "\"gsm_diag\":\"Oczekiwanie na modem GSM\",\"gps_satellites\":0,"
    "\"gsm_bars\":0,\"sms_result\":\"Nie wysylano jeszcze SMS\","
    "\"temperature_valid\":false,\"temperature_c\":null,"
    "\"clock\":\"T+00:00:00\",\"clock_synced\":false,\"log\":\"\"}";
static constexpr uint8_t EVENT_LOG_CAPACITY = 24;
static String event_entries_[EVENT_LOG_CAPACITY];
static uint8_t event_count_ = 0;
static uint8_t next_event_ = 0;
static String sms_result_ = "Nie wysylano jeszcze SMS";
static String clock_text_ = "T+00:00:00";
static bool clock_synced_ = false;

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="pl">
<head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1"/>
<title>System ratunkowy</title>
<style>
 :root{font-family:system-ui,-apple-system,"Segoe UI",sans-serif;color:#17233b;background:#f2f5fa}
 *{box-sizing:border-box}
 body{margin:0;padding:24px}
 .shell{max-width:1040px;margin:0 auto}
 header{display:flex;justify-content:space-between;align-items:center;gap:16px;margin-bottom:20px}
 h1{font-size:clamp(24px,4vw,34px);margin:2px 0}
 h2,h3,p{margin-top:0}
 .kicker,.label{font-size:12px;font-weight:700;letter-spacing:.08em;color:#62718b;text-transform:uppercase}
 .clock{text-align:right;background:#fff;border:1px solid #e0e6ef;border-radius:12px;padding:10px 14px}
 .clock strong{display:block;font:600 16px ui-monospace,monospace;margin-top:3px}
 .clock small{color:#62718b}
 .card{background:#fff;border:1px solid #e0e6ef;border-radius:16px;padding:20px;margin-bottom:16px;box-shadow:0 4px 16px #17233b0b}
 .hero{border-left:5px solid #2e8b70}
 .hero-row{display:flex;justify-content:space-between;align-items:center;gap:16px}
 .state{font-size:clamp(23px,4vw,32px);font-weight:750;margin:4px 0}
 .state.good{color:#167653}.state.bad{color:#bd2d3d}
 .badge{background:#edf1f7;border-radius:999px;padding:8px 12px;font-weight:700}
 .sms{border-top:1px solid #e8edf4;margin-top:14px;padding-top:12px}
 .sms strong{display:block;margin-top:4px}.sms.bad{color:#bd2d3d}.sms.warn{color:#936400}
 .grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:16px}
 .metric h3{font-size:15px;color:#62718b;margin-bottom:10px}
 .value{font-size:25px;font-weight:750;line-height:1.2;overflow-wrap:anywhere}
 .detail{font-size:14px;color:#56647b;line-height:1.5;margin:8px 0 0;overflow-wrap:anywhere}
 .section-title{display:flex;justify-content:space-between;align-items:baseline;gap:12px}
 .section-title h3{margin-bottom:14px}
 .map-link{display:none;color:#1769aa;font-weight:650}
 .map-link.visible{display:inline}
 .map-container{display:none}
 .map-container.visible{display:block}
 .map-frame{display:block;width:100%;height:360px;border:0;border-radius:12px}
 .map-placeholder{display:none;min-height:170px;place-items:center;text-align:center;padding:20px;background:#f2f5fa;border-radius:12px;color:#62718b}
 .map-placeholder.visible{display:grid}
 .section-title small{color:#6c7890}
 form{display:flex;gap:8px;flex-wrap:wrap}
 input{flex:1;min-width:190px;padding:11px 12px;border:1px solid #cbd4e2;border-radius:9px;font:inherit}
 button{font:600 15px system-ui;padding:11px 15px;border:0;border-radius:9px;cursor:pointer}
 button:focus-visible,input:focus-visible{outline:3px solid #6fa7ff;outline-offset:2px}
 #sos{background:#c53242;color:white}#cancel{background:#e8edf4;color:#26344c}
 #contacts{padding-left:22px}
 #contacts li{padding:5px 0}
 #contacts button{margin-left:8px;padding:6px 10px;font-size:13px;background:#edf1f7}
 .error{color:#b42332;min-height:1.2em;margin:8px 0 0}
 .actions{display:flex;gap:10px;flex-wrap:wrap}
 .event-log{list-style:none;padding:0;margin:0;max-height:300px;overflow:auto}
 .event-log li{display:flex;gap:12px;padding:9px 0;border-bottom:1px solid #edf0f5;font-size:14px}
 .event-log li:last-child{border-bottom:0}
 .event-time{font:12px ui-monospace,monospace;color:#53657f;min-width:150px}
 .empty{color:#718096}
 #connection{color:#b42332}
 @media(max-width:640px){body{padding:14px}.grid{grid-template-columns:1fr}.card{padding:16px}header{align-items:flex-start;flex-direction:column}.clock{text-align:left}.event-log li{display:block}.event-time{display:block;margin-bottom:4px}.map-frame{height:280px}}
</style>
</head>
<body>
<main class="shell">
 <header>
  <div><div class="kicker">Panel ESP32</div><h1>System ratunkowy</h1></div>
  <div class="clock"><small id="clock-note">Oczekiwanie na czas GPS</small><strong id="clock">T+00:00:00</strong></div>
 </header>
 <p id="connection" role="status"></p>
 <section class="card hero">
  <div class="hero-row">
   <div><div class="label">Stan systemu</div><h2 id="st" class="state">Laczenie...</h2><p id="kind" class="detail"></p></div>
   <div id="countdown" class="badge">Brak alarmu</div>
  </div>
  <div id="sms-box" class="sms"><span class="label">Ostatni wynik SMS</span><strong id="sms-result">Nie wysylano jeszcze SMS</strong></div>
 </section>
 <section class="grid">
  <article class="card metric"><h3>Temperatura kabiny</h3><div id="temperature" class="value">--</div><p class="detail">Odczyt z czujnika DHT11</p></article>
  <article class="card metric"><h3>Ruch i orientacja</h3><div id="g" class="value">--</div><p id="angles" class="detail"></p></article>
  <article class="card metric"><h3>GPS</h3><div id="gps" class="value">Brak FIX</div><p id="gps-meta" class="detail"></p><p id="gpsdiag" class="detail"></p></article>
  <article class="card metric"><h3>SIM / GSM</h3><div id="gsmdiag" class="value">Oczekiwanie</div><p id="gsm-meta" class="detail"></p></article>
 </section>
 <section class="card">
  <div class="section-title"><h3>Aktualna pozycja</h3><a id="map-link" class="map-link" target="_blank" rel="noopener noreferrer">Otworz w Google Maps</a></div>
  <div id="map-placeholder" class="map-placeholder visible">Oczekiwanie na pozycje GPS z FIX.</div>
  <div id="map-container" class="map-container"><iframe id="map-frame" class="map-frame" title="Aktualna pozycja GPS na Google Maps" loading="lazy" referrerpolicy="no-referrer-when-downgrade"></iframe></div>
  <p class="detail">Mapa wymaga dostepu do internetu. Wspolrzedne sa przekazywane do Google Maps.</p>
 </section>
 <section class="card">
  <div class="section-title"><h3>Numery alarmowe SMS</h3><small>Maksymalnie 10 numerow</small></div>
  <form id="contact-form">
   <input id="contact-number" type="tel" placeholder="+48123456789" pattern="\+[0-9]{7,15}" required/>
   <button type="submit">Dodaj numer</button>
  </form>
  <p id="contact-error" class="error" role="alert"></p>
  <ul id="contacts"></ul>
 </section>
 <section class="card">
  <div class="section-title"><h3>Przyciski alarmowe</h3></div>
  <div class="actions"><button id="sos" type="button">Uruchom SOS</button><button id="cancel" type="button">Anuluj / reset</button></div>
 </section>
 <section class="card">
  <div class="section-title"><h3>Dziennik zdarzen</h3><small>24 wpisy · czyszczone przy restarcie</small></div>
  <ol id="log" class="event-log"><li class="empty">Brak zdarzen</li></ol>
 </section>
</main>
<script>
const el=id=>document.getElementById(id);
let lastMapRefresh=0;
async function tick(){
 try{
  const r=await fetch('/state',{cache:'no-store'});
  if(!r.ok) throw new Error('Blad odczytu stanu');
  const j=await r.json();
  el('connection').textContent='';
  el('st').textContent=j.state;
  el('st').className='state '+(j.alarm||j.state.includes('BŁĄD')?'bad':'good');
  el('kind').textContent='Typ zdarzenia: '+j.kind;
  el('countdown').textContent=j.countdown>0?'Odliczanie: '+j.countdown+' s':
   (j.alarm?'Alarm aktywny':'Brak aktywnego alarmu');
  el('clock').textContent=j.clock;
  el('clock-note').textContent=j.clock_synced?'Czas GPS · strefa lokalna':'Czas od uruchomienia · GPS oczekuje';
  el('sms-result').textContent=j.sms_result;
  el('sms-box').className='sms '+(j.sms_result.includes('NIE WYSŁANO')||
   j.sms_result.includes('NIEUDANA')?'bad':
   (j.sms_result.includes('pozostale nie')||j.sms_result.startsWith('Nie wysylano')?'warn':''));
  el('temperature').textContent=j.temperature_valid?j.temperature_c.toFixed(1)+' °C':'Brak odczytu';
  el('g').textContent=j.g.toFixed(2)+' G';
  el('angles').textContent='Roll '+j.roll.toFixed(1)+'° · Pitch '+j.pitch.toFixed(1)+'° · Yaw '+j.yaw.toFixed(1)+'°';
  el('gps').textContent=j.gps;
  el('gps-meta').textContent='Satelity: '+j.gps_satellites;
  el('gpsdiag').textContent=j.gps_diag;
  const mapFrame=el('map-frame');
  const mapContainer=el('map-container');
  const mapPlaceholder=el('map-placeholder');
  const mapLink=el('map-link');
  if(j.gps_map_url){
   const embedUrl=new URL(j.gps_map_url);
   embedUrl.pathname='/maps';
   embedUrl.searchParams.set('z','15');
   embedUrl.searchParams.set('output','embed');
   const now=Date.now();
   if(!mapFrame.dataset.mapUrl||
      (j.gps_map_url!==mapFrame.dataset.mapUrl&&now-lastMapRefresh>=10000)){
    mapFrame.src=embedUrl.toString();
    mapFrame.dataset.mapUrl=j.gps_map_url;
    lastMapRefresh=now;
   }
   mapLink.href=j.gps_map_url;
   mapLink.classList.add('visible');
   mapContainer.classList.add('visible');
   mapPlaceholder.classList.remove('visible');
  }else{
   mapFrame.removeAttribute('src');
   mapFrame.dataset.mapUrl='';
   mapLink.removeAttribute('href');
   mapLink.classList.remove('visible');
   mapContainer.classList.remove('visible');
   mapPlaceholder.classList.add('visible');
  }
  el('gsmdiag').textContent=j.gsm_diag;
  el('gsm-meta').textContent='Zasieg: '+j.gsm_bars+'/4';
  const log=el('log');
  log.replaceChildren();
  const entries=(j.log||'').split('\n').filter(Boolean);
  if(entries.length===0){
   const empty=document.createElement('li');empty.className='empty';empty.textContent='Brak zdarzen';log.append(empty);
  }else{
   for(const entry of entries){
    const item=document.createElement('li');
    const separator=entry.indexOf(' | ');
    const time=document.createElement('span');time.className='event-time';
    const message=document.createElement('span');
    time.textContent=separator>=0?entry.slice(0,separator):'';
    message.textContent=separator>=0?entry.slice(separator+3):entry;
    item.append(time,message);log.append(item);
   }
  }
 }catch(error){el('connection').textContent='Brak polaczenia z plytka: '+error.message;}
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
async function sendCommand(path){
 try{const r=await fetch(path);if(!r.ok)throw new Error('Blad polecenia');}
 catch(error){el('connection').textContent=error.message;}
}
el('sos').onclick=()=>sendCommand('/sos');
el('cancel').onclick=()=>sendCommand('/cancel');
loadContacts().catch(e=>document.getElementById('contact-error').textContent=e.message);
setInterval(tick,1000); tick();
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

static void appendEvent(const String& message) {
  if (message.length() == 0) {
    return;
  }
  event_entries_[next_event_] = clock_text_ + " | " + message;
  next_event_ = (next_event_ + 1) % EVENT_LOG_CAPACITY;
  if (event_count_ < EVENT_LOG_CAPACITY) {
    ++event_count_;
  }
}

static String eventLogText() {
  String log;
  for (uint8_t i = 0; i < event_count_; ++i) {
    const uint8_t index =
        (next_event_ + EVENT_LOG_CAPACITY - 1 - i) % EVENT_LOG_CAPACITY;
    if (i > 0) {
      log += "\n";
    }
    log += event_entries_[index];
  }
  return log;
}

static String jsonEscape(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char character = value[i];
    switch (character) {
      case '"':
        escaped += "\\\"";
        break;
      case '\\':
        escaped += "\\\\";
        break;
      case '\b':
        escaped += "\\b";
        break;
      case '\f':
        escaped += "\\f";
        break;
      case '\n':
        escaped += "\\n";
        break;
      case '\r':
        escaped += "\\r";
        break;
      case '\t':
        escaped += "\\t";
        break;
      default:
        if (static_cast<uint8_t>(character) >= 0x20) {
          escaped += character;
        }
        break;
    }
  }
  return escaped;
}

static const char* stateName(SystemState s) {
  switch (s) {
    case SystemState::Boot:
      return "URUCHAMIANIE";
    case SystemState::Ready:
      return "MONITORING";
    case SystemState::Countdown:
      return "ALARM - ODLICZANIE";
    case SystemState::Sending:
      return "PROBA 1/2 - WYSYŁANIE SMS";
    case SystemState::RetryWait:
      return "PROBA 1 NIEUDANA";
    case SystemState::Retrying:
      return "PROBA 2/2 - PONAWIAM";
    case SystemState::Sent:
      return "WYSŁANO SMS";
    case SystemState::SendPartial:
      return "CZĘŚCIOWO WYSŁANO SMS";
    case SystemState::SendFailed:
      return "NIE WYSŁANO SMS";
    case SystemState::Fault:
      return "BŁĄD CZUJNIKA";
  }
  return "?";
}

void demoWebSetClock(const String& timestamp, bool gpsTimeSynced) {
  clock_text_ = timestamp;
  clock_synced_ = gpsTimeSynced;
}

void demoWebLog(const String& message) { appendEvent(message); }

void demoWebPush(SystemState state,
                 CrashKind kind,
                 float g,
                 float roll,
                 float pitch,
                 float yaw,
                 const String& gps,
                 const String& gpsMapsUrl,
                 const String& gpsDiagnostics,
                 const String& gsmDiagnostics,
                 uint8_t gpsSatellites,
                 uint8_t gsmSignalBars,
                 float temperatureC,
                 bool temperatureValid,
                 int countdown_s,
                 const String& logLine) {
  if (logLine.length() > 0) {
    appendEvent(logLine);
    sms_result_ = logLine;
  }

  json_state_ = "{";
  json_state_ += "\"state\":\"";
  json_state_ += jsonEscape(stateName(state));
  json_state_ += "\",\"kind\":\"";
  json_state_ += jsonEscape(crashKindName(kind));
  json_state_ += "\",\"g\":";
  json_state_ += String(g, 3);
  json_state_ += ",\"roll\":";
  json_state_ += String(roll, 2);
  json_state_ += ",\"pitch\":";
  json_state_ += String(pitch, 2);
  json_state_ += ",\"yaw\":";
  json_state_ += String(yaw, 2);
  json_state_ += ",\"countdown\":";
  json_state_ += String(countdown_s);
  json_state_ += ",\"alarm\":";
  json_state_ += (state == SystemState::Ready || state == SystemState::Boot ||
                  state == SystemState::Fault)
                     ? "false"
                     : "true";
  json_state_ += ",\"gps\":\"";
  json_state_ += jsonEscape(gps);
  json_state_ += "\",\"gps_map_url\":\"";
  json_state_ += jsonEscape(gpsMapsUrl);
  json_state_ += "\",\"gps_diag\":\"";
  json_state_ += jsonEscape(gpsDiagnostics);
  json_state_ += "\",\"gsm_diag\":\"";
  json_state_ += jsonEscape(gsmDiagnostics);
  json_state_ += "\",\"gps_satellites\":";
  json_state_ += String(gpsSatellites);
  json_state_ += ",\"gsm_bars\":";
  json_state_ += String(gsmSignalBars);
  json_state_ += ",\"sms_result\":\"";
  json_state_ += jsonEscape(sms_result_);
  json_state_ += "\"";
  json_state_ += ",\"temperature_valid\":";
  json_state_ += temperatureValid ? "true" : "false";
  json_state_ += ",\"temperature_c\":";
  if (temperatureValid) {
    json_state_ += String(temperatureC, 1);
  } else {
    json_state_ += "null";
  }
  json_state_ += ",\"clock\":\"";
  json_state_ += jsonEscape(clock_text_);
  json_state_ += "\",\"clock_synced\":";
  json_state_ += clock_synced_ ? "true" : "false";
  json_state_ += ",\"log\":\"";
  json_state_ += jsonEscape(eventLogText());
  json_state_ += "\"}";
}
