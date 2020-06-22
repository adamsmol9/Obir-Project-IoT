#ifndef COAP_H
#define COAP_H
#include <ObirDhcp.h>            
#include <ObirEthernet.h>       
#include <ObirEthernetUdp.h>
#define PACKET_BUFFER_LENGTH 100

//typ wyliczeniowy sluzacy do okreslenia sposobu obsluzenia wiadomosci
enum RequestType{
  DISCOVER, GET, PUT, STATS, CLIENT_ERROR, RST, METHOD_NOT_ALLOWED, NOT_FOUND //rodzaje wiadomosci
};

struct payload_t {
  unsigned long val; //wartosc przenoszona przez wiadomosc
  unsigned short type; //typ przenoszonej wiadomosci
};

class coapOption{
  public:
  uint8_t delta = 0;
  uint8_t optionLength = 0;  
  uint8_t *optionValue;
};


class coapPacket{
  public:
  uint8_t coapVersion;
  coapOption cOption[5];
  uint8_t type = 0;
  uint8_t code = 0;
  const uint8_t *token = NULL;
  uint8_t tokenlen = 0;
  //const uint8_t *payload = NULL;
  size_t payloadlen = 0;
  uint16_t  messageId = 0;
  uint8_t optionnum = 0;
  char payload[5]; 
  byte bitClass; //req\response class
  byte bitCode; //req\response code
  //void sendResponse(byte options[]=NULL, int optionsSize=0, char payload[]=NULL, int payloadSize=0, byte c=0, byte dd=0, bool isObs = false);
  

  
  void bufferToPacket(uint8_t buffer[],int32_t packetlen);
  
};

class coapServer{
  public:
  ObirEthernetUDP Udp;
  unsigned char packetBuffer[PACKET_BUFFER_LENGTH];
  unsigned char packetMessage[PACKET_BUFFER_LENGTH]; 
  bool start();
  bool loop();

  void sendResponse(byte options[]=NULL, int optionsSize=0, char payload[]=NULL, int payloadSize=0, byte c=0, byte dd=0,  uint8_t coapVersion = 0, uint8_t tokenlen = 0 ){
      byte responseMessage[256 + 12]; // inicjalizacja tablicy do utworzenia wiadomosci
      uint16_t midCounter = 0;

      
      responseMessage[0] = coapVersion << 6 | 0b1 << 4 | tokenlen; //ustawienie wartosci pierwszego bajtu
      responseMessage[1] = c << 5 | dd; // ustawienie pola code
      responseMessage[2] = midCounter >> 8; //ustawienie bajtu mid
      responseMessage[3] = midCounter; //ustawienie bajtu mid
      
      midCounter += 1; //inkrementacja mid wiadomosci
      
      
      
      int index = (int)tokenlen + 4; //ustawienie indeksu na bajt po tokenie
      for(int i = 0; i < optionsSize; i++){
        responseMessage[index++] = (byte)options[i]; //dolacz opcje
      }
      responseMessage[index++] = 0b11111111; //bajt rozpoczecia payloadu
      for(int i = 0; i < payloadSize; i++){
        responseMessage[index++] = (byte)payload[i]; //wpisanie payloadu
      }
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort()); //ustal adres i port, na ktory wysalny bedzie pakiet
      Udp.write(responseMessage, index - 1); //wyslij pakiet
      Udp.endPacket(); //zakoncz wysylanie
    }
};
  




#endif
