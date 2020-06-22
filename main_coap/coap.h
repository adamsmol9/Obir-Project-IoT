#ifndef COAP_H
#define COAP_H
#include <ObirDhcp.h>            
#include <ObirEthernet.h>       
#include <ObirEthernetUdp.h>
#define PACKET_BUFFER_LENGTH 100

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
  const uint8_t *payload = NULL;
  size_t payloadlen = 0;
  uint16_t  messageId = 0;
  uint8_t optionnum = 0;
  
  void bufferToPacket(uint8_t buffer[],int32_t packetlen);
  
};

class coapServer{
  public:
  ObirEthernetUDP Udp;
  unsigned char packetBuffer[PACKET_BUFFER_LENGTH];
  unsigned char packetMessage[PACKET_BUFFER_LENGTH]; 
  bool start();
  bool loop();
};



#endif
