#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

const uint16_t RECV_PIN = 19;

IRrecv irrecv(RECV_PIN);
decode_results results;

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();
}

void loop() {

  if (irrecv.decode(&results)) {

    // Ignore noise signals
    if (results.decode_type != UNKNOWN && results.bits > 20) {

      Serial.print("Protocol: ");
      Serial.println(typeToString(results.decode_type));

      Serial.print("HEX: ");
      Serial.println(resultToHexidecimal(&results));

      Serial.print("Bits: ");
      Serial.println(results.bits);

      Serial.println("----------------");
    }

    irrecv.resume();
  }
}
