#include "coap.h"
#include "string.h"

#define PACKET_BUFFER_LENGTH        100
#define UDP_SERVER_PORT 1234
bool debug = true;
byte MAC[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01}; //MAC adres karty sieciowej, to powinno byc unikatowe - proforma dla ebsim'a
unsigned int localPort = UDP_SERVER_PORT;

 //odsyla wiadomosc do Copper
    

//setup poczatkowy
bool coapServer::start() {

  ObirEthernet.begin(MAC);
  
  Serial.print(F("My IP address: "));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    Serial.print(ObirEthernet.localIP()[thisByte], DEC); Serial.print(F("."));
  }
  Serial.println();
  Udp.begin(localPort);
}

//glowna petla serwera
bool coapServer::loop() {
  
  int packetSize = Udp.parsePacket();
  if (packetSize > 0) {
    Serial.println("Packet size ");
    Serial.println((char*)packetSize);
    int len = Udp.read(packetBuffer, PACKET_BUFFER_LENGTH);
    Serial.print("Received: ");
    packetBuffer[len] = '\0';
    packetMessage[len] = '\0';
    Serial.println((char*)packetBuffer);
   
    coapPacket cPacket;
    cPacket.coapVersion = packetBuffer[0] & 196 >> 6;
    cPacket.type = packetBuffer[0] & 48 >> 4;
    cPacket.tokenlen = packetBuffer[0] & 15;
    cPacket.code = packetBuffer[1];
    cPacket.bitClass = packetBuffer[1] >> 5;
    unsigned char temp = packetBuffer[1] >> 5;
    cPacket.bitCode = packetBuffer[1] ^ (temp << 5);
    
    cPacket.messageId = (packetBuffer[2] << 8) | packetBuffer[3];     //łaczenie 2 bajtów
    
    //obsluga tokena
    if (cPacket.tokenlen > 0) {
      uint8_t token = new uint8_t[cPacket.tokenlen]; //uwaga moze nie byc dobrze - tkl to nie token morddy
    } else {
      uint8_t token = NULL;
    }
    
    //obsługa opcji - iterujemy po byte opcji
    int optionNumber = 0;
    bool isNextOption = true;
    int currentByte = 4 + cPacket.tokenlen;
    while (isNextOption) {
      cPacket.cOption[optionNumber].delta = packetBuffer[currentByte] & 244 >> 4;
      cPacket.cOption[optionNumber].optionLength = packetBuffer[currentByte] & 15;
    
      if (cPacket.cOption[optionNumber].delta == 13) {
        currentByte++;
        cPacket.cOption[optionNumber].delta += packetBuffer[currentByte];
      }
      
      if (cPacket.cOption[optionNumber].optionLength == 13) {
        currentByte++;
        cPacket.cOption[optionNumber].optionLength += packetBuffer[currentByte];
      }
      
      Serial.print("delta");
      Serial.println(cPacket.cOption[optionNumber].delta);
      Serial.print("lenght");
      Serial.println(cPacket.cOption[optionNumber].optionLength);
      
      //reading option Value
      currentByte++;
      if (cPacket.cOption[optionNumber].optionLength > 0) {
        cPacket.cOption[optionNumber].optionValue = new uint8_t[cPacket.cOption[optionNumber].optionLength];
        for (int i = 0 ; i < cPacket.cOption[optionNumber].optionLength; i++) {
          cPacket.cOption[optionNumber].optionValue[i] = packetBuffer[currentByte];
          currentByte++;
        }
      }
      
      //checking if there are more options
      if (packetBuffer[currentByte] == NULL) {
        isNextOption = false;
      }
      if (packetBuffer[currentByte] == 255) {
        isNextOption = false;
      }
      optionNumber++;
      Serial.println("LoOoOoOoP");
    }

    for(int i = 0 ; i < 5 ; i++){
      Serial.print("Coap option nr :");
      Serial.print(i);
      Serial.print(" delta of option: ");
      Serial.print(cPacket.cOption[i].delta);
      Serial.print(" lenght of option value is :");  
      Serial.print(cPacket.cOption[i].optionLength);
      Serial.println();
    }
    Serial.print("Size od udp");
    Serial.println(packetSize);
    Serial.print("current Byte");
    Serial.println(currentByte);
    
    //handling payload
    int i =0;
    
       while(i < packetSize){
        if (packetBuffer[i] == 255){ //payload zaczyna sie od bajtu ff, czyli 255
          int j = 0;
          while(packetBuffer[i]){
            ++i;
            cPacket.payload[j] = packetBuffer[i]; //zapisanie bajtu payloadu do struktury
            ++j;
            if((cPacket.payload[j] != '\0') and (cPacket.payload[j] > '9' or cPacket.payload[j] < '0' or j > 5)){ //payload nie musi by liczba i miec mniej niz 5 cyf
            }
          }
        }
        ++i;
      }
    
   
  
  //testy se robimy a co
  if (debug) {
    Serial.println();
  }
  Serial.print("coapver ");
  Serial.println(cPacket.coapVersion);
  Serial.print("code ");
  Serial.println(cPacket.code);
  Serial.print("tokenlen ");
  Serial.println(cPacket.tokenlen);
  Serial.print("type ");
  Serial.println(+cPacket.type);
  Serial.print("messageId ");
  Serial.println(cPacket.messageId);
  Serial.println("payload");
  Serial.println(cPacket.payload);
 
}
}
