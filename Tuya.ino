#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "mbedtls/md.h"

#define DHTPIN 4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_WIFI_PASSWORD";

const char* mqttServer = "m1.tuyacn.com";
const int mqttPort = 1883;

String productKey = "quk29pjxe6w5r3hi";
String deviceName = "2663f3e78e1d5d1ba2aovv";
String deviceSecret = "Sh9rRYu8JYO1PI2A";

WiFiClient espClient;
PubSubClient client(espClient);

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

void connectMQTT()
{
  while (!client.connected())
  {
    Serial.println("[TUYA] Connecting MQTT");

    String timestamp = String(millis());

    String clientId = deviceName;

    String username =
      deviceName +
      "|signMethod=hmacSha256,timestamp=" +
      timestamp +
      ",secureMode=1,accessType=1|";

    String signContent =
      "clientId" + deviceName +
      "deviceName" + deviceName +
      "productKey" + productKey +
      "timestamp" + timestamp;

    String passwordMQTT = hmacSha256(signContent, deviceSecret);

    if (client.connect(clientId.c_str(), username.c_str(), passwordMQTT.c_str()))
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

  Serial.println("[DHT] Temp: " + String(t));
  Serial.println("[DHT] Hum: " + String(h));

  StaticJsonDocument<200> doc;

  doc["id"] = millis();
  doc["version"] = "1.0";
  doc["method"] = "thing.event.property.post";

  JsonObject params = doc.createNestedObject("params");
  params["temp"] = (int)t;
  params["humidity"] = (int)h;

  char buffer[256];
  serializeJson(doc, buffer);

  String topic =
  "/sys/" + productKey + "/" +
  deviceName +
  "/thing/event/property/post";

  client.publish(topic.c_str(), buffer);

  Serial.println("[TUYA] Data sent");
}

void setup()
{
  Serial.begin(115200);
  dht.begin();

  connectWiFi();

  client.setServer(mqttServer, mqttPort);
}

void loop()
{
  if (!client.connected())
    connectMQTT();

  client.loop();

  sendSensor();

  delay(5000);
}
