#include <ObirDhcp.h>            
#include <ObirEthernet.h>       
#include <ObirEthernetUdp.h>
#include "string.h"

#define PACKET_BUFFER_LENGTH 100

// struktura sluzaca do zapisu opcji zawartych w otrzymanej wiadomosci CoAP
struct AllowedOptions {
  char uriPath[255]; // opcja nr 11
  byte contentFormat[2]; // opcja nr 12
  byte acceptOption[2]; // opcja nr 17
};

// struktura sluzaca do zapisu wiadomosci CoAP z wylaczeniem opcji
struct Message {
  byte coapVersion; // wersja protokolu CoAP
  byte typeMessage; // typ wiadomosci - CON = 0, NON = 1, ACK = 2, RST = 3
  byte tokenLength; // dlugosc tokena
  byte code; // klasa pola kod
  byte messageId[2]; // identyfikator wiadomosci sluzacy do wykrywania duplikatow
  byte codeDetails; // detale pola kod
  byte token[8]; // token sluzacy do matchowania wiadomosci
  char payload[5]; // wiadomosc niesiona przez wiadomosc
};

// typ wyliczeniowy sluzacy do okreslenia sposobu obsluzenia wiadomosci
enum RequestType{
  DISCOVER, LIGHT_GET, LIGHT_PUT, METRYKA_GET, METRYKA_PUT, STATS, CLIENT_ERROR, RST, UNKNOWN, METHOD_NOT_ALLOWED, NOT_FOUND // rodzaje wiadomosci
};

// klasa sluzaca do zarzadzania CoAPem
class CoapModule{
  private:
    Message message;
    AllowedOptions allowedOptions;
    ObirEthernetUDP udp;

    // parsuje wiadomosc CoAP i zwraca rodzaj wiadomosci
    RequestType getRequestType(int ps, byte pb[]) {
      for(int i = 0; i<255; ++i) allowedOptions.uriPath[i] = 0; //  czysci uriPath
      for(int i = 0; i<5; ++i) message.payload[i] = 0;
      RequestType requestType = UNKNOWN; // inicjalizacja rodzaju zadania na wartosc domyslna
      byte verBits = pb[0] >> 6; // zczytanie wersji CoAP
      message.coapVersion = verBits; // zapisanie wersji CoAP do struktury
      byte typeBits = pb[0] >> 4 ^ verBits << 2; //  zczytanie pola typ
      message.typeMessage = typeBits; // zapisanie pola typ do struktury
      byte tokenLengthBits = pb[0] ^ (verBits << 6 | typeBits << 4); //  zczytanie dlugosci tokena
      message.tokenLength = tokenLengthBits; // zapisanie dlugosci tokena do struktury
      byte bitClass = pb[1] >> 5; // zczytanie klasy pola kod
      message.code = bitClass; // zapisanie wartosci klasy pola kod do struktury 
      message.codeDetails = pb[1] ^ (bitClass << 5); // zczytanie szczegolow pola kod
      message.messageId[0] = pb[2]; // zczytanie pierwszego bajtu identyfikatora wiadomosci 
      message.messageId[1] = pb[3]; // zczytanie drugiego bajtu identyfikatora wiadomosci
      
      for(int i = 0; i < (int)tokenLengthBits; i++) message.token[i] = pb[i + 4]; // zapisanie tokena do struktury
      unsigned int j = 0;
      unsigned int optionIndex = (int)tokenLengthBits + 4; // indeks opcji
      unsigned int currentOption = 0; // aktualna wartosc opcji
      unsigned int optionBytesLength = 0; // zmienna do przechowania dlugosci aktualnej opcji
      
      // petla do zczytywania opcji
      while(pb[optionIndex]){
        int start = 0; // zmienna uzywana do dostepu do indeksu aktualnej opcji
        byte byteOption = pb[optionIndex] >> 4; //  zczytanie option delta
        
        // ponizsze ify sluza do obslugi option delta 13 i 14 wedlug specyfikacji protokolu CoAP
        if((unsigned int)byteOption == 13){
          currentOption += (unsigned int)pb[++optionIndex] + 13;
          optionIndex++;
          continue;
        }
        else if((unsigned int)byteOption == 14){
          currentOption += (unsigned int)pb[optionIndex + 1] << 8 | (unsigned int)pb[optionIndex + 2];
          optionIndex += 2;
          continue;
        }
        
        currentOption += (unsigned int)byteOption; // zwiekszenie aktualnej opcji o option delta
        if((unsigned int)byteOption == 0) start = optionBytesLength; // jesli option delta rowne 0 to indeks ustawiany tak, aby kontynuowac dana opcje
        optionBytesLength = (unsigned int)(pb[optionIndex++] ^ (byteOption << 4)); //  zczytanie dlugosci aktualnej opcji

        switch(currentOption){
          case 11: // uri-path
            // iteracja po bajtach wchodzacych w sklad danej opcji
            for(int k = 0; k < optionBytesLength; k++) { 
              allowedOptions.uriPath[k + start] = (char)pb[k + optionIndex]; // zapis opcji do struktury
            }
            break;
          case 12: // content-format
            // iteracja po bajtach wchodzacych w sklad danej opcji
            for(int k = 0; k < optionBytesLength; k++) {
              allowedOptions.contentFormat[k + start] = pb[k + optionIndex]; // zapis opcji do struktury
            }
            break;
          case 17: // accept
            // iteracja po bajtach wchodzacych w sklad danej opcji
            for(int k = 0; k < optionBytesLength; k++) {
              allowedOptions.acceptOption[k + start] = pb[k + optionIndex]; // zapis opcji do struktury
            }
            break;
        }
        optionIndex += optionBytesLength; // inkrementacja option index, tak, aby byl ustawiony na indeks nastepnej opcji
      }

      int i = 0;
      // petla do zczytania payloadu
      while(i < ps){
        if (pb[i] == 255){ // payload zaczyna sie od bajtu ff, czyli 255
          int j = 0;
          while(pb[i]){
            ++i;
            message.payload[j] = pb[i]; // zapisanie bajtu payloadu do struktury
            ++j;
            if((message.payload[j] != '\0') and (message.payload[j] > '9' or message.payload[j] < '0' or j > 5)){ // payload nie musi by liczba i miec mniej niz 5 cyfr
              return CLIENT_ERROR; // jesli payload jest nieprawidlowy to zwracany jest blad klienta
            }
          }
        }
        ++i;
      }

      Serial.println(allowedOptions.uriPath);
      if(message.typeMessage == 3){ // jesli typ wiadomosci 3, to RST
        requestType = RST;
        return requestType;
        Serial.println("RESET");
      }
        else if(message.codeDetails == 1){ // jesli detail=1, to GET
        if(!strcmp(allowedOptions.uriPath, "light")){ 
          requestType = LIGHT_GET; // jesli light, to ustaw request type
        }
        
        else if(!strcmp(allowedOptions.uriPath, "stats")){
          requestType = STATS; // jesli stats, to ustaw request type
        }
        
        else if(!strcmp(allowedOptions.uriPath, ".well-knowncore")){
          requestType = DISCOVER; // jesli .well-knowncore, to ustaw request type
        }  
        
        else{
          requestType = NOT_FOUND; // jesli nie znaleziono, to ustaw request type
        }
      }
      else if(!strcmp(allowedOptions.uriPath, "light") || 
              !strcmp(allowedOptions.uriPath, "metryka2") ||
              !strcmp(allowedOptions.uriPath, "stats")) {
              requestType = METHOD_NOT_ALLOWED; // w innym przypadku metoda nie dozwolona
      }
      else {
        requestType = NOT_FOUND; // ostatecznie zwroc nie znaleziono
      }
      return requestType; // zwroc request type
    }

  public:
    uint16_t messageIdCounter = 0;

    // zmien MESSAGE ID o odpowiednia delte
    void changeMessageId(int mDelta){
      int messageId = (int)message.messageId[0] << 8 | message.messageId[1]; // przeksztalcenie bajtow MESSAGE ID na int
      messageId += mDelta; // dodaj zadana delte
      message.messageId[0] = messageId >> 8; // zamien MESSAGE ID na pierwszy bajt
      message.messageId[1] = messageId; // zamien MESSAGE ID na drugi bajt
    }

    // zamienia chary na int
    int extractValueFromPayload(){
      int i;
      sscanf(message.payload, "%d", &i); // zamien payload z tablicy char na int
      return i; // zwroc rezultat
    }

    // inicjalizacja polaczenia internetowego
    void setup(byte mac[], unsigned int localPort){
      ObirEthernet.begin(mac); // przypisanie mac
      Serial.println("Pobieranie adresu Internet.");
      Serial.println(ObirEthernet.localIP()); // wypisanie uzyskanego adresu ip
      udp.begin(localPort);
    }

    // metoda do przechwycenia zadania
    RequestType receiveRequest(){
      int packetSize = udp.parsePacket(); // pobranie dlugosci otrzymanego pakietu
      byte packetBuffer[UDP_TX_PACKET_MAX_SIZE] = { NULL }; //  wyczyszczenie bufora
      udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE); // odczytanie wiadomosci do packetBuffer
      return packetSize ? getRequestType(packetSize, packetBuffer) : UNKNOWN; // zwrocenie rodzaju wiadomosci
    }

    // odsylanie wiadomosci do Copper
    void sendResponse(byte options[]=NULL, int optionsSize=0, char payload[]=NULL, int payloadSize=0, byte code=0, byte codeDetails=0){
      byte responseMessage[256 + 12]; //  inicjalizacja tablicy do utworzenia wiadomosci
      
      responseMessage[0] = message.coapVersion << 6 | 0b1 << 4 | message.tokenLength; // ustawienie wartosci pierwszego bajtu - wersja, type, tokenLength
      responseMessage[1] = code << 5 | codeDetails; //  ustawienie pola code
      responseMessage[2] = messageIdCounter >> 8; // ustawienie 1 bajtu MESSAGE ID
      responseMessage[3] = messageIdCounter; // ustawienie 2 bajtu MESSAGE ID
      
      messageIdCounter += 1; // inkrementacja MESSAGE ID wiadomosci
      
      int index = (int)message.tokenLength + 4; // ustawienie indeksu na bajt po tokenie
      for(int i = 0; i < optionsSize; i++){
        responseMessage[index++] = (byte)options[i]; // dolacz opcje
      }
      responseMessage[index++] = 0b11111111; // bajt rozpoczecia payloadu (payload zaczyna sie po 8 jedynkach)
      for(int i = 0; i < payloadSize; i++){
        responseMessage[index++] = (byte)payload[i]; // wpisanie payloadu
      }
      udp.beginPacket(udp.remoteIP(), udp.remotePort()); // ustal adres i port, na ktory wyslany zostanie pakiet
      udp.write(responseMessage, index - 1); // wyslij pakiet
      udp.endPacket(); // zakoncz wysylanie pakietu
    }
};

byte mac[] = {
   0x00, 0xAA, 0xBB, 0xCC, 0xDD, 0xF5 // ustawienie adresu mac
};

CoapModule coapServer; // inicjalizacja obiektu klasy CoapModule
unsigned int localPort = 1234; // ustawienie portu do komunikacji

// liczenie cyfr zawartych w 'n'
int countDigit(long n){
  int r = 0;
  if(!n) return 1;
  while(n > 0){
    ++r;
    n/=10;
  }
  return r;
}

// inicjalizacja polaczenia internetowego
void setup() {
   Serial.begin(115200);
   coapServer.setup(mac, localPort);
}

// wyslanie wiadomosci z kodem 4.00
void sendBadClientReq(char *b, int bSize){
  byte a[1];
  coapServer.sendResponse(a, 0, b, bSize+1, 4, 0);
}

// wyslanie wiadomosci z kodem 2.05 i payloadem
void sendNumberToCoap(long n){
      char buffer[countDigit(n) + 1];
      int ret = snprintf(buffer, sizeof(buffer)+1, "%ld", n);
      byte passOptions[] = { 0b1100 << 4 | 0b1, 0b0,
                             0b1011 << 4 | 0b1, 0b10,
                             0b0101 << 4 | 0b1, (byte)(countDigit(n) + 1)
                           };
      coapServer.sendResponse(passOptions, sizeof(passOptions), buffer, countDigit(n) + 1, 2, 5);
}

// wyslanie wiadomosci z kodem 2.04 i payloadem w odpowiedzi na put
void sendPutRespons(long n){
      char buffer[countDigit(n) + 1];
      int ret = snprintf(buffer, sizeof(buffer)+1, "%ld", n);
      byte passOptions[] = { 0b1100 << 4 | 0b1, 0b0,
                             0b0101 << 4 | 0b1, (byte)sizeof(buffer)  };
      coapServer.sendResponse(passOptions, sizeof(passOptions), buffer, countDigit(n) + 1, 2, 4);
}

// wyslanie wiadomosci z kodem 4.00
void sendMethodNotAllowed(){
  byte a[1];
  char b[1];
  coapServer.sendResponse(a, 0, b, 0, 4, 5);  
}

// wyslanie wiadomosci z kodem 4.04
void sendNotFound(){
  byte a[1];
  char b[1];
  coapServer.sendResponse(a, 0, b, 0, 4, 4);
}

// wyslanie wiadomosci z kodem 2.05 w odpowiedzi na zadanie pobrania statystyk
void sendStatTest(){
      char buffer[] = "TESTTESTTESTTESTTESTTESTTTESTTESTTEST";
      byte passOptions[] = { 0b1100 << 4 | 0b1, 0b0,
                             0b1011 << 4 | 0b1, 0b10,
                           };
      coapServer.sendResponse(passOptions, sizeof(passOptions), buffer, (byte)(sizeof(buffer) + 1), 2, 5);
}



// glowna wykonywalna petla
void loop() {
  unsigned long full_time = 0; // laczny czas testow
  unsigned long statsCounter = 0; // liczy ilosc wyslanych wiadomosci do utworzenia statystyk
  int requestType = coapServer.receiveRequest(); // ustawienie request type

    // obsluzenie rodzaju wiadomosci get light
    if(requestType == LIGHT_GET){
      Serial.println("Light Get");
    }
    
    // obsluzenie rodzaju wiadomosci put light
    else if(requestType == LIGHT_PUT){
      Serial.println("Light Put");
      int value = coapServer.extractValueFromPayload();
      Serial.println(value);
      if(!(value >= 0 and value <= 1000)){
        Serial.println(value);
        char buffer[] = "Wartosc nie jest z dozwolonego przedzialu.";
        sendBadClientReq(buffer, sizeof(buffer)/sizeof(char));
      }
    }

    // obsluzenie rodzaju wiadomosci get metryka
    if(requestType == METRYKA_GET){
      Serial.println("metryka Get");
    }
    
    // obsluzenie rodzaju wiadomosci put metryka
    else if(requestType == METRYKA_PUT){
      Serial.println("metryka Put");
      int value = coapServer.extractValueFromPayload();
      Serial.println(value);
      if(!(value >= 0 and value <= 1000)){
        Serial.println(value);
        char buffer[] = "Wartosc nie jest z dozwolonego przedzialu.";
        sendBadClientReq(buffer, sizeof(buffer)/sizeof(char));
      }
    }
    
    // obsluzenie rodzaju wiadomosci discover
    else if(requestType == DISCOVER){
      Serial.println("Discover");
      char discoverPayload1[] = "</light>;</metryka2>;";
      char discoverPayload2[] = "</dupa>";
      
      byte passOptionsD1[] = { 0b100 << 4 |0b1, 0b1111111,// etag
                               0b1000 << 4 | 0b1, 0b101000}; // content-format
      
      byte passOptionsD2[] = { 0b100 << 4 |0b1, 0b1111111, // etag
                               0b1000 << 4 | 0b1, 0b101000}; // content-format
                               
      Serial.println(discoverPayload1);
      Serial.println(discoverPayload2);
      
      coapServer.sendResponse(passOptionsD1, sizeof(passOptionsD1), discoverPayload1, (byte)sizeof(discoverPayload1), 2, 5); // odeslij 64 bajty wiadomosci
      coapServer.sendResponse(passOptionsD2, sizeof(passOptionsD2), discoverPayload2, (byte)sizeof(discoverPayload2), 2, 5); // odeslij reszte wiadomosci
    }
    
    // obsluzenie bledu klienta
    else if(requestType == CLIENT_ERROR){
      char buffer[] = "Not allowed chars contain in payload.";
      sendBadClientReq(buffer, sizeof(buffer)/sizeof(char));
    }
    
    // obsluzenie niedozwolonej metody
    else if(requestType == METHOD_NOT_ALLOWED){
      sendMethodNotAllowed();
    }
    
    // obsluzenie reseta
    else if(requestType == RST){
      
    }
    
    // obsluzenie not found
    else if(requestType == NOT_FOUND){
      sendNotFound();
    }
    
    // obsluzenie wiadomosci dotyczacej statystyk
    else if(requestType == STATS){
       ++statsCounter;
       unsigned long start = millis();
       int counter = 20;
       int counter2 = 20;
       int good = 0;
       while(counter--){
         sendStatTest();
         if(coapServer.receiveRequest() == RST){
           ++good;
         }
         delay(100);
       }
       unsigned long time = ((millis() - start) - 100*counter2)/counter2;
       full_time += time;
       int ratio = double(good)/counter2 * 100;
       unsigned long avg_time = full_time/statsCounter;
       char buffer[50];
       sprintf(buffer,"%ld/%d/%ld", time, ratio, avg_time);
       Serial.println(buffer);
       byte passOptions[] = { 0b1100 << 4 | 0b1, 0b0 };
       coapServer.sendResponse(passOptions, sizeof(passOptions), buffer, (byte)(countDigit(time) + countDigit(ratio) + countDigit(avg_time) + 3), 2, 5);
       coapServer.receiveRequest();
    } 
}
