#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// ------------ WiFi ------------
const char* WIFI_SSID     = "Yash";
const char* WIFI_PASSWORD = "69638701";

// ------------ Pins ------------
#define DHTPIN 4
#define DHTTYPE DHT11
#define PIRPIN 5
#define MQ2PIN 34  // analog pin

// ------------ Objects ------------
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

// ------------ HTML/CSS UI ------------
const char HTML_PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>ESP32 Live Sensors</title>
  <style>
    body { font-family: system-ui, -apple-system, Segoe UI, Roboto, Arial; margin: 0; padding: 24px; background: #0b1020; color: #e9eef9; }
    .card { background: #151b2f; border-radius: 16px; padding: 20px; max-width: 720px; margin: 0 auto; box-shadow: 0 10px 25px rgba(0,0,0,.35); }
    h1 { margin: 0 0 8px; font-size: 22px; letter-spacing: .3px; }
    .sub { opacity: .75; margin-bottom: 16px; font-size: 13px; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px,1fr)); gap: 12px; }
    .tile { background: #0f1426; border: 1px solid #232b46; border-radius: 14px; padding: 14px; }
    .label { font-size: 12px; opacity: .7; margin-bottom: 6px; }
    .value { font: 700 26px/1.2 ui-sans-serif, system-ui; }
    .unit { font-size: 14px; opacity: .7; margin-left: 6px; }
    .muted { opacity: .6; font-size: 12px; margin-top: 10px; }
    .ok { color: #8be28b; }
    .warn { color: #ffd479; }
    .bad { color: #ff8a8a; }
    footer { text-align: center; font-size: 12px; opacity: .6; margin-top: 14px; }
  </style>
</head>
<body>
  <div class="card">
    <h1>ESP32 Live Sensors</h1>
    <div class="sub">Auto-refreshing every <span id="interval">1</span>s</div>

    <div class="grid">
      <div class="tile">
        <div class="label">Temperature</div>
        <div class="value"><span id="temp">--</span><span class="unit">°C</span></div>
      </div>
      <div class="tile">
        <div class="label">Humidity</div>
        <div class="value"><span id="hum">--</span><span class="unit">%</span></div>
      </div>
      <div class="tile">
        <div class="label">Motion</div>
        <div class="value"><span id="motion">--</span></div>
      </div>
      <div class="tile">
        <div class="label">MQ-2 Gas</div>
        <div class="value"><span id="mq2">--</span></div>
      </div>
    </div>

    <footer>JSON API: <code>/api</code></footer>
  </div>

<script>
const refreshMs = 1000;
document.getElementById("interval").textContent = refreshMs/1000;

async function refresh() {
  try {
    const res = await fetch('/api', { cache: 'no-store' });
    const j = await res.json();

    const set = (id, v) => document.getElementById(id).textContent = (v ?? '--');

    set('temp', j.temperature_c?.toFixed(1));
    set('hum', j.humidity_pct?.toFixed(0));
    set('motion', j.motion ? 'YES' : 'NO');
    set('mq2', j.mq2_raw);
  } catch (e) {
    // ignore errors
  } finally {
    setTimeout(refresh, refreshMs);
  }
}
refresh();
</script>
</body>
</html>
)HTML";

// ------------ API ------------
void handleAPI() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int motion = digitalRead(PIRPIN);
  int mq2raw = analogRead(MQ2PIN);

  String json = "{";
  json += "\"temperature_c\":"; json += isnan(temp) ? "null" : String(temp, 1);
  json += ",\"humidity_pct\":"; json += isnan(hum) ? "null" : String(hum, 0);
  json += ",\"motion\":" + String(motion);
  json += ",\"mq2_raw\":" + String(mq2raw);
  json += "}";

  server.send(200, "application/json", json);
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", HTML_PAGE);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(PIRPIN, INPUT);

  Serial.println("\nConnecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected ✅");
  Serial.print("IP address: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/api", handleAPI);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
