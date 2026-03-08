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

void setup()
{
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[WIFI] Connected");

  dht.begin();

  my_device.init(pid, mcu_ver);
}

void loop()
{
  my_device.run();

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  Serial.print("Temp: ");
  Serial.println(temp);

  Serial.print("Humidity: ");
  Serial.println(hum);

  my_device.mcu_dp_value_update(DP_TEMP, (int)temp);
  my_device.mcu_dp_value_update(DP_HUM, (int)hum);

  delay(5000);
}
