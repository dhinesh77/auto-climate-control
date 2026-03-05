#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <DHT.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <ir_LG.h>
#include <RCSwitch.h>

/******** VERSION ********/
#define FW_NAME "Auto Climate Control"
#define FW_VERSION "v1.0"

/******** PINS ********/
#define DHTPIN 4
#define DHTTYPE DHT22
#define IR_SEND_PIN 23
#define IR_RECV_PIN 19
#define RF_TX_PIN 18
#define RF_RX_PIN 21

/******** OBJECTS ********/
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);
Preferences storage;

IRrecv irrecv(IR_RECV_PIN);
decode_results results;

IRLgAc ac(IR_SEND_PIN);
RCSwitch rf;

/******** SETTINGS ********/
float setpoint = 26;
float hysteresis = 1;

bool automationEnabled = true;
bool irReceiverEnabled = true;
bool rfReceiverEnabled = true;

unsigned long minACruntime = 300000;
unsigned long acStartTime = 0;

/******** STATE ********/
bool acState=false;
int fanSpeed=0;

float temperature=0;
float humidity=0;
float filteredTemp=0;

/******** RF CODES ********/
unsigned long FAN1=0;
unsigned long FAN2=0;
unsigned long FAN3=0;
unsigned long FAN4=0;
unsigned long FAN5=0;
unsigned long FANOFF=0;

/******** STORAGE ********/
void saveCodes(){
storage.begin("codes",false);
storage.putULong("fan1",FAN1);
storage.putULong("fan2",FAN2);
storage.putULong("fan3",FAN3);
storage.putULong("fan4",FAN4);
storage.putULong("fan5",FAN5);
storage.putULong("fanoff",FANOFF);
storage.end();
}

void loadCodes(){
storage.begin("codes",true);
FAN1=storage.getULong("fan1",0);
FAN2=storage.getULong("fan2",0);
FAN3=storage.getULong("fan3",0);
FAN4=storage.getULong("fan4",0);
FAN5=storage.getULong("fan5",0);
FANOFF=storage.getULong("fanoff",0);
storage.end();
}

/******** SENSOR ********/
void readSensor(){

float t=dht.readTemperature();
float h=dht.readHumidity();

if(!isnan(t))
filteredTemp = filteredTemp*0.7 + t*0.3;

temperature=t;

if(!isnan(h))
humidity=h;

}

/******** FAN ********/
void fan1(){ if(FAN1) rf.send(FAN1,24); fanSpeed=1; }
void fan2(){ if(FAN2) rf.send(FAN2,24); fanSpeed=2; }
void fan3(){ if(FAN3) rf.send(FAN3,24); fanSpeed=3; }
void fan4(){ if(FAN4) rf.send(FAN4,24); fanSpeed=4; }
void fan5(){ if(FAN5) rf.send(FAN5,24); fanSpeed=5; }
void fanOff(){ if(FANOFF) rf.send(FANOFF,24); fanSpeed=0; }

/******** SMART FAN ********/
void smartFan(float t){

if(t < 24) fanOff();
else if(t < 24.5) fan1();
else if(t < 25) fan2();
else if(t < 26) fan3();
else if(t < 27) fan4();
else fan5();

}

/******** AC ********/
void acOn(){

ac.setPower(true);
ac.setTemp(setpoint);
ac.send();

acState=true;
acStartTime=millis();

server.sendHeader("Location","/");
server.send(303);

}

void acOff(){

if(acState && millis()-acStartTime>minACruntime){

ac.setPower(false);
ac.send();
acState=false;

}

server.sendHeader("Location","/");
server.send(303);

}

/******** AUTOMATION ********/
void climateLogic(){

float t=filteredTemp;

smartFan(t);

if(t > setpoint + hysteresis){
if(!acState) acOn();
}

else if(t < setpoint - hysteresis){
acOff();
}

}

/******** LEARN IR ********/
void learnIR(){

if(!irReceiverEnabled){
server.send(200,"text/plain","IR receiver disabled");
return;
}

server.send(200,"text/plain","Press AC remote");

unsigned long start=millis();

while(millis()-start < 10000){

if(irrecv.decode(&results)){

Serial.println(results.value);
irrecv.resume();
break;

}

}

}

/******** LEARN RF ********/
void learnRF(){

if(!rfReceiverEnabled){
server.send(200,"text/plain","RF receiver disabled");
return;
}

server.send(200,"text/plain","Press fan remote");

rf.enableReceive(digitalPinToInterrupt(RF_RX_PIN));

unsigned long start=millis();

while(millis()-start < 10000){

if(rf.available()){

FAN1=rf.getReceivedValue();
saveCodes();
rf.resetAvailable();
break;

}

}

rf.disableReceive();

}

/******** DASHBOARD ********/
String dashboard(){

String html;

html+="<html><head>";
html+="<meta name='viewport' content='width=device-width'>";
html+="<style>";
html+="body{font-family:Arial;text-align:center;background:#f2f2f2}";
html+=".card{background:white;margin:15px;padding:20px;border-radius:10px}";
html+="button{padding:12px;margin:5px}";
html+="</style></head><body>";

html+="<h2>"+String(FW_NAME)+"</h2>";
html+="<p>"+String(FW_VERSION)+"</p>";

html+="<div class='card'>";
html+="Temp: "+String(filteredTemp)+"°C<br>";
html+="Humidity: "+String(humidity)+"%";
html+="</div>";

html+="<div class='card'>";
html+="Setpoint "+String(setpoint)+"°C<br>";
html+="<form action='/set'>";
html+="<input name='temp' type='number'>";
html+="<button>Update</button>";
html+="</form>";
html+="</div>";

html+="<div class='card'>";
html+="<a href='/acon'><button>AC ON</button></a>";
html+="<a href='/acoff'><button>AC OFF</button></a>";
html+="</div>";

html+="<div class='card'>";
html+="Fan<br>";
html+="<a href='/fan1'><button>1</button></a>";
html+="<a href='/fan2'><button>2</button></a>";
html+="<a href='/fan3'><button>3</button></a>";
html+="<a href='/fan4'><button>4</button></a>";
html+="<a href='/fan5'><button>5</button></a>";
html+="<a href='/fanoff'><button>OFF</button></a>";
html+="</div>";

html+="<div class='card'>";
html+="Learning<br>";
html+="<a href='/learnir'><button>Learn IR</button></a>";
html+="<a href='/learnrf'><button>Learn RF</button></a>";
html+="</div>";

html+="<div class='card'>";
html+="Automation: ";
html+=automationEnabled?"ON ":"OFF ";
html+="<a href='/autoToggle'><button>Toggle</button></a><br>";

html+="IR Receiver: ";
html+=irReceiverEnabled?"ON ":"OFF ";
html+="<a href='/irToggle'><button>Toggle</button></a><br>";

html+="RF Receiver: ";
html+=rfReceiverEnabled?"ON ":"OFF ";
html+="<a href='/rfToggle'><button>Toggle</button></a>";
html+="</div>";

html+="</body></html>";

return html;

}

/******** ROUTES ********/
void handleRoot(){ server.send(200,"text/html",dashboard()); }

void setTemp(){
if(server.hasArg("temp"))
setpoint=server.arg("temp").toFloat();
server.sendHeader("Location","/");
server.send(303);
}

/******** SETUP ********/
void setup(){

Serial.begin(115200);

dht.begin();

ac.begin();
ac.setModel(LG_AC);
ac.setMode(kLgAcCool);
ac.setFan(kLgAcFanAuto);

rf.enableTransmit(RF_TX_PIN);
loadCodes();

if(irReceiverEnabled)
irrecv.enableIRIn();

WiFiManager wm;
wm.autoConnect("AutoClimateSetup");

ArduinoOTA.begin();

server.on("/",handleRoot);
server.on("/set",setTemp);

server.on("/acon",acOn);
server.on("/acoff",acOff);

server.on("/fan1",fan1);
server.on("/fan2",fan2);
server.on("/fan3",fan3);
server.on("/fan4",fan4);
server.on("/fan5",fan5);
server.on("/fanoff",fanOff);

server.on("/learnir",learnIR);
server.on("/learnrf",learnRF);

server.on("/autoToggle",[](){
automationEnabled=!automationEnabled;
server.sendHeader("Location","/");
server.send(303);
});

server.on("/irToggle",[](){
irReceiverEnabled=!irReceiverEnabled;
server.sendHeader("Location","/");
server.send(303);
});

server.on("/rfToggle",[](){
rfReceiverEnabled=!rfReceiverEnabled;
server.sendHeader("Location","/");
server.send(303);
});

server.begin();

Serial.println(WiFi.localIP());

}

/******** LOOP ********/
void loop(){

ArduinoOTA.handle();
server.handleClient();

readSensor();

if(automationEnabled)
climateLogic();

delay(5000);

}
