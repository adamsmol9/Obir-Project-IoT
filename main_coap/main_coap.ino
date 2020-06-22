#include "coap.h"

coapServer coap;
void setup() {
  Serial.begin(115200);
  Serial.println("Essa");
  coap.start();
}

void loop() {

  coap.loop();

}
