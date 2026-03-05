#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const uint16_t RECV_PIN = 19;

IRrecv irrecv(RECV_PIN);
decode_results results;

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("ESP32 LG AC IR Decoder Ready");
  irrecv.enableIRIn();
}

void loop() {

  if (irrecv.decode(&results)) {

    Serial.println("===== IR RECEIVED =====");

    Serial.print("Protocol: ");
    Serial.println(typeToString(results.decode_type));

    Serial.print("HEX: ");
    Serial.println(resultToHexidecimal(&results));

    Serial.print("Bits: ");
    Serial.println(results.bits);

    Serial.println("Decoded Data:");
    Serial.println(resultToHumanReadableBasic(&results));

    Serial.println("========================");

    irrecv.resume();
  }
}
