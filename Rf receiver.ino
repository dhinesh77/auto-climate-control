#include <RCSwitch.h>

RCSwitch rf = RCSwitch();

#define RF_PIN 21

void setup() {
  Serial.begin(115200);

  rf.enableReceive(RF_PIN);  // GPIO21

  Serial.println("RF Learning Mode Started");
  Serial.println("Press any RF remote button...");
}

void loop() {

  if (rf.available()) {

    long code = rf.getReceivedValue();
    int bitLength = rf.getReceivedBitlength();
    int protocol = rf.getReceivedProtocol();
    int delayTime = rf.getReceivedDelay();

    Serial.println("------ RF Signal Received ------");
    Serial.print("Code: ");
    Serial.println(code);

    Serial.print("Bit Length: ");
    Serial.println(bitLength);

    Serial.print("Protocol: ");
    Serial.println(protocol);

    Serial.print("Pulse Length: ");
    Serial.println(delayTime);

    Serial.println("-------------------------------");

    rf.resetAvailable();
  }

} 
