#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <IRremoteESP8266.h>
#include <IRac.h>

const char* ssid="ESP32-AC";
const char* password="12345678";

AsyncWebServer server(80);

const uint16_t IR_PIN = 4;

IRac ac(IR_PIN);

bool power=false;
uint8_t temp=24;
uint8_t mode=kCool;
uint8_t fan=kFanAuto;
bool swing=false;

void sendAC(){

ac.sendAc(
"LG",
power,
mode,
temp,
fan,
swing,
false
);

}

String processor(){

String json="{";
json+="\"temp\":"+String(temp)+",";
json+="\"power\":"+String(power);
json+="}";
return json;

}

const char html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>

<meta name="viewport" content="width=device-width,initial-scale=1">

<style>

body{
font-family:Arial;
text-align:center;
background:#111;
color:white;
}

button{

width:140px;
height:55px;
font-size:18px;
margin:8px;
border-radius:12px;
border:none;
background:#00BCD4;
color:white;

}

.temp{

font-size:50px;
margin:20px;

}

</style>

<body>

<h2>LG AC Control</h2>

<button onclick="fetch('/power')">Power</button>

<button onclick="fetch('/mode')">Mode</button>

<div class="temp" id="temp">24°C</div>

<button onclick="fetch('/tempup')">Temp +</button>

<button onclick="fetch('/tempdown')">Temp -</button>

<br>

<button onclick="fetch('/fan')">Fan Speed</button>

<button onclick="fetch('/swing')">Swing</button>

<script>

setInterval(()=>{

fetch('/status')
.then(r=>r.json())
.then(data=>{

document.getElementById("temp").innerHTML=data.temp+"°C";

})

},1000)

</script>

</body>
</html>

)rawliteral";

void setup(){

Serial.begin(115200);

ac.begin();

WiFi.softAP(ssid,password);

Serial.println(WiFi.softAPIP());

server.on("/",HTTP_GET,[](AsyncWebServerRequest *req){

req->send_P(200,"text/html",html);

});

server.on("/power",HTTP_GET,[](AsyncWebServerRequest *req){

power=!power;
sendAC();
req->send(200);

});

server.on("/mode",HTTP_GET,[](AsyncWebServerRequest *req){

if(mode==kCool) mode=kDry;
else if(mode==kDry) mode=kFan;
else mode=kCool;

sendAC();
req->send(200);

});

server.on("/tempup",HTTP_GET,[](AsyncWebServerRequest *req){

if(temp<30) temp++;
sendAC();
req->send(200);

});

server.on("/tempdown",HTTP_GET,[](AsyncWebServerRequest *req){

if(temp>18) temp--;
sendAC();
req->send(200);

});

server.on("/fan",HTTP_GET,[](AsyncWebServerRequest *req){

if(fan==kFanAuto) fan=kFanLow;
else if(fan==kFanLow) fan=kFanMed;
else if(fan==kFanMed) fan=kFanHigh;
else fan=kFanAuto;

sendAC();
req->send(200);

});

server.on("/swing",HTTP_GET,[](AsyncWebServerRequest *req){

swing=!swing;
sendAC();
req->send(200);

});

server.on("/status",HTTP_GET,[](AsyncWebServerRequest *req){

req->send(200,"application/json",processor());

});

server.begin();

}

void loop(){} 
