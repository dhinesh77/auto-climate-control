#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

Preferences preferences;
WebServer server(80);

WiFiClient espClient;
PubSubClient client(espClient);

String savedSSID;
String savedPASS;

float temperature;
float humidity;

bool wifiConnected=false;
bool debugEnabled=true;

const char* mqtt_server="m1.tuyacn.com";
const int mqtt_port=1883;

const char* deviceID="YOUR_DEVICE_ID";
const char* deviceSecret="YOUR_DEVICE_SECRET";

void debug(String msg)
{
if(debugEnabled)
Serial.println(msg);
}

void startAP()
{
debug("[AP] Starting configuration AP");

WiFi.softAP("ESP32_Setup");

server.on("/",setupPage);
server.on("/save",saveWiFi);

server.begin();
}

void setupPage()
{
String page="<h2>WiFi Setup</h2>"
"<form action='/save'>"
"SSID:<input name='ssid'><br>"
"Password:<input name='pass'><br>"
"<input type='submit' value='Save'>"
"</form>";

server.send(200,"text/html",page);
}

void saveWiFi()
{
savedSSID=server.arg("ssid");
savedPASS=server.arg("pass");

preferences.putString("ssid",savedSSID);
preferences.putString("pass",savedPASS);

debug("[WIFI] Credentials saved");

server.send(200,"text/html","Saved. Restarting");

delay(2000);
ESP.restart();
}

void connectWiFi()
{
debug("[WIFI] Connecting to WiFi");

WiFi.begin(savedSSID.c_str(),savedPASS.c_str());

int timeout=20;

while(WiFi.status()!=WL_CONNECTED && timeout--)
{
delay(500);
Serial.print(".");
}

if(WiFi.status()==WL_CONNECTED)
{
wifiConnected=true;
debug("\n[WIFI] Connected");
debug(WiFi.localIP().toString());
}
else
{
debug("[WIFI] Failed → Starting AP");
startAP();
}
}

void connectMQTT()
{
while(!client.connected())
{
debug("[TUYA] Connecting MQTT");

if(client.connect(deviceID,deviceID,deviceSecret))
{
debug("[TUYA] Connected");
}
else
{
debug("[TUYA] Retry MQTT");
delay(2000);
}
}
}

void dashboard()
{
String page=
"<meta http-equiv='refresh' content='5'>"
"<h1>ESP32 Sensor Dashboard</h1>"
"<h2>Temperature:"+String(temperature)+" C</h2>"
"<h2>Humidity:"+String(humidity)+" %</h2>"
"<a href='/settings'>Settings</a>";

server.send(200,"text/html",page);
}

void settingsPage()
{
String debugState = debugEnabled ? "ON" : "OFF";

String page=
"<h2>Settings</h2>"
"<form action='/save'>"
"SSID:<input name='ssid'><br>"
"Password:<input name='pass'><br>"
"<input type='submit' value='Update WiFi'>"
"</form>"

"<br><h3>Serial Debug: "+debugState+"</h3>"
"<a href='/debugon'>Enable Debug</a><br>"
"<a href='/debugoff'>Disable Debug</a><br>"

"<br><a href='/delete'>Delete WiFi</a>";

server.send(200,"text/html",page);
}

void enableDebug()
{
debugEnabled=true;
preferences.putBool("debug",true);
Serial.println("[DEBUG] Enabled");
server.send(200,"text/html","Debug Enabled");
}

void disableDebug()
{
preferences.putBool("debug",false);
debugEnabled=false;
server.send(200,"text/html","Debug Disabled");
}

void deleteWiFi()
{
preferences.clear();
debug("[WIFI] Deleted");

server.send(200,"text/html","WiFi Deleted. Restarting");
delay(2000);
ESP.restart();
}

void sendTuyaData()
{
StaticJsonDocument<200> doc;

doc["temp"]=(int)temperature;
doc["humidity"]=(int)humidity;

char buffer[256];
serializeJson(doc,buffer);

client.publish("tylink/dev/report",buffer);

debug("[TUYA] Data sent");
}

void setup()
{
Serial.begin(115200);

preferences.begin("wifi",false);

debugEnabled=preferences.getBool("debug",true);

debug("[BOOT] Starting ESP32");

dht.begin();

savedSSID=preferences.getString("ssid","");
savedPASS=preferences.getString("pass","");

if(savedSSID=="")
{
debug("[BOOT] No WiFi saved");
startAP();
}
else
{
connectWiFi();
}

client.setServer(mqtt_server,mqtt_port);

server.on("/",dashboard);
server.on("/settings",settingsPage);
server.on("/delete",deleteWiFi);
server.on("/save",saveWiFi);
server.on("/debugon",enableDebug);
server.on("/debugoff",disableDebug);

server.begin();

debug("[SERVER] HTTP server started");
}

void loop()
{
server.handleClient();

temperature=dht.readTemperature();
humidity=dht.readHumidity();

debug("[DHT] Temp:"+String(temperature));
debug("[DHT] Hum:"+String(humidity));

if(wifiConnected)
{
if(!client.connected())
connectMQTT();

client.loop();

sendTuyaData();
}

delay(5000);
}
