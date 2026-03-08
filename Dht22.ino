#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "mbedtls/md.h"

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";

const char* deviceID = "2663f3e78e1d5d1ba2aovv";
const char* deviceSecret = "Sh9rRYu8JYO1PI2A";

const char* mqtt_server = "m1.tuyacn.com";

WiFiClient espClient;
PubSubClient client(espClient);

String timestamp;
String clientId;
String username;
String password_mqtt;

String hmacSha256(String data, String key)
{
  byte hmacResult[32];
  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t* info;

  info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, info, 1);

  mbedtls_md_hmac_starts(&ctx, (const unsigned char*) key.c_str(), key.length());
  mbedtls_md_hmac_update(&ctx, (const unsigned char*) data.c_str(), data.length());
  mbedtls_md_hmac_finish(&ctx, hmacResult);

  char output[65];
  for (int i = 0; i < 32; i++)
    sprintf(output + i * 2, "%02x", hmacResult[i]);

  return String(output);
}

void connectWiFi()
{
  Serial.println("[WIFI] Connecting");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[WIFI] Connected");
}

void generateAuth()
{
  timestamp = String(millis());

  clientId = String(deviceID);

  username = String(deviceID) +
  "|signMethod=hmacSha256,timestamp=" + timestamp +
  ",secureMode=1,accessType=1|";

  String signContent = "clientId" + String(deviceID) +
                       "deviceName" + String(deviceID) +
                       "productKeyquk29pjxe6w5r3hi" +
                       "timestamp" + timestamp;

  password_mqtt = hmacSha256(signContent, deviceSecret);
}

void connectMQTT()
{
  while (!client.connected())
  {
    Serial.println("[TUYA] Connecting MQTT");

    generateAuth();

    if (client.connect(clientId.c_str(),
                       username.c_str(),
                       password_mqtt.c_str()))
    {
      Serial.println("[TUYA] Connected");
    }
    else
    {
      Serial.print("[TUYA] Failed ");
      Serial.println(client.state());
      delay(3000);
    }
  }
}

void sendSensor()
{
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  Serial.println("[DHT] Temp " + String(t));
  Serial.println("[DHT] Hum " + String(h));

  StaticJsonDocument<200> doc;

  JsonObject data = doc.createNestedObject("data");
  data["temp"] = (int)t;
  data["humidity"] = (int)h;

  doc["id"] = millis();
  doc["version"] = "1.0";
  doc["method"] = "thing.event.property.post";

  char buffer[256];
  serializeJson(doc, buffer);

  String topic = "/sys/quk29pjxe6w5r3hi/" + String(deviceID) + "/thing/event/property/post";

  client.publish(topic.c_str(), buffer);

  Serial.println("[TUYA] Data sent");
}

void setup()
{
  Serial.begin(115200);

  dht.begin();

  connectWiFi();

  client.setServer(mqtt_server, 1883);
}

void loop()
{
  if (!client.connected())
    connectMQTT();

  client.loop();

  sendSensor();

  delay(5000);
}
