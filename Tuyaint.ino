#include <WiFi.h>
#include <TuyaWifi.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22

#define WIFI_SSID "YOUR_WIFI"
#define WIFI_PASS "YOUR_PASSWORD"

#define DP_TEMP 101
#define DP_HUM 102

DHT dht(DHTPIN, DHTTYPE);
TuyaWifi my_device;

unsigned char pid[] = "quk29pjxe6w5r3hi";
unsigned char mcu_ver[] = "1.0.0";

float temp;
float hum;

void setup()
{
  Serial.begin(115200);
  delay(1000);

  Serial.println("ESP32 START");

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("[WIFI] Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("[WIFI] Connected");

  dht.begin();
  Serial.println("[DHT] Sensor started");

  my_device.init(pid, mcu_ver);
  Serial.println("[TUYA] Init done");
}

void loop()
{
  my_device.run();

  temp = dht.readTemperature();
  hum = dht.readHumidity();

  Serial.println("----- SENSOR DATA -----");

  Serial.print("Temp: ");
  Serial.println(temp);

  Serial.print("Humidity: ");
  Serial.println(hum);

  if(!isnan(temp))
  {
    Serial.println("[TUYA] Sending temp");
    my_device.mcu_dp_value_update(DP_TEMP, (int)temp);
  }

  if(!isnan(hum))
  {
    Serial.println("[TUYA] Sending humidity");
    my_device.mcu_dp_value_update(DP_HUM, (int)hum);
  }

  Serial.println("-----------------------");

  delay(5000);
}
