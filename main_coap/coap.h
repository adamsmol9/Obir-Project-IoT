#ifndef COAP_H
#define COAP_H
#include <ObirDhcp.h>            
#include <ObirEthernet.h>       
#include <ObirEthernetUdp.h>
#define PACKET_BUFFER_LENGTH 100

/**
 * Typ wyliczeniowy służący do określenia sposobu obsłużenia wiadomości
 */
enum RequestType{
  DISCOVER, GET, PUT, STATS, CLIENT_ERROR, RST, METHOD_NOT_ALLOWED, NOT_FOUND //rodzaje wiadomosci
};

/**
 * Struktura payloadu wiadomości CoAP
 */
struct payload_t {
  unsigned long val; //Wartość przenoszona przez wiadomość
  unsigned short type; // Typ przenoszonej wiadomości
};

/**
 * Klasa przechowująca informacje (delte i dlugość opcji) o opcjach wiadomości CoAP
 */
class coapOption{
  public:
  uint8_t delta = 0;
  uint8_t optionLength = 0;  
  uint8_t *optionValue;
};

/**
 * Klasa przechowująca informacje o danych pakietu CoAP
 */
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

  
  void bufferToPacket(uint8_t buffer[],int32_t packetlen);
  
};

/**
 * Klasa serwera zajmująca się przechowywaniem buforów, komunikacją oraz zawierająca główną pętle programu
 */
class coapServer{
  public:
  ObirEthernetUDP Udp;
  unsigned char packetBuffer[PACKET_BUFFER_LENGTH];
  unsigned char packetMessage[PACKET_BUFFER_LENGTH]; 
  bool start();
  bool loop();

  
  // Metoda odpowiedzialna za odsyłanie wiadomość do Copper'a
  void sendResponse(byte options[]=NULL, int optionsSize=0, char payload[]=NULL, int payloadSize=0, byte c=0, byte dd=0, uint16_t MID = 0,  uint8_t coapVersion = 1, uint8_t tokenlen = 0 ){
      byte responseMessage[256 + 12]; // inicjalizacja tablicy do utworzenia wiadomosci
      uint16_t midCounter = 0;

      responseMessage[0] = coapVersion << 6 | 0b10 << 4 | tokenlen;   // ustawienie wartości pierwszego bajtu (wersja, Type, TokenLength)
      responseMessage[1] = c << 5 | dd;   // ustawienie pola Code
      responseMessage[2] = MID >> 8;    // ustawienie bajtu Message ID (bajt 1)
      responseMessage[3] = MID;   // ustawienie bajtu Message ID (bajt 2)
      
      midCounter += 1; //inkrementacja Message ID wiadomości
      
      int index = (int)tokenlen + 4; // ustawienie indeksu na bajt po tokenie
      for(int i = 0; i < optionsSize; i++){
        responseMessage[index++] = (byte)options[i]; // dołączenie opcji
      }
      responseMessage[index++] = 0b11111111; // ustawienie indeksu na bajt Payload'u (po ośmiu jedynkach)
      for(int i = 0; i < payloadSize; i++){
        responseMessage[index++] = (byte)payload[i]; // wpisanie payloadu
      }
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort()); // ustalenie adresu i portu, na który wysłany zostanie pakiet
      Udp.write(responseMessage, index - 1); // wysłanie pakietu
      Udp.endPacket(); // zakończenie wysyłania
    }
};
#endif
