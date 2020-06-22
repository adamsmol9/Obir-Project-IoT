#include "coap.h"

coapServer coap;
void setup() {
  Serial.begin(115200);
  Serial.println("Witamy w pRoGrAmIe cOaP sErVeR !!");
  coap.start();
}

void loop() {

  coap.loop();

}
