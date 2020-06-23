#include "coap.h"
#include "string.h"

#define PACKET_BUFFER_LENGTH        100
#define UDP_SERVER_PORT 1234

bool debug = true;
byte MAC[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01}; // MAC adres karty sieciowej
unsigned int localPort = UDP_SERVER_PORT; // przypisanie portu

/**
 * Liczenie cyfr zawartych w 'n'
 */
int countDigit(long n){
  int r = 0;
  if(!n) return 1;
  while(n > 0){
    ++r;
    n/=10;
  }
  return r;
}

/**
 * Setup początkowy - Inicjacja biblioteki Ethernet, UDP i ustawień sieciowych
 */
bool coapServer::start() {
  
  ObirEthernet.begin(MAC);
  
  Serial.print(F("My IP address: "));
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    Serial.print(ObirEthernet.localIP()[thisByte], DEC); Serial.print(F("."));
  }
  Serial.println();
  Udp.begin(localPort);
}

/**
 * Główna wykonywalna pętla aplikacji serwera
 */
bool coapServer::loop() {
  
  int packetSize = Udp.parsePacket();   // pobranie długości otrzymanego pakietu
  if (packetSize > 0) {   // obsługa pakietu, jezeli odebrany pakiet jest niepusty
    Serial.println("Packet size: ");
    Serial.println(packetSize);  // wyświetlenie długości pakietu
    int len = Udp.read(packetBuffer, PACKET_BUFFER_LENGTH);   // DOPISAC @TODO
    Serial.print("Received: ");   // DOPISAC @TODO
    packetBuffer[len] = '\0';   // DOPISAC @TODO
    packetMessage[len] = '\0';    // DOPISAC @TODO
    Serial.println((char*)packetBuffer);    // DOPISAC @TODO


    // zczytywanie danych z wiadomości Coapa
    coapPacket cPacket;
    cPacket.coapVersion = packetBuffer[0] & 196 >> 6;   // zczytywanie wersji 
    cPacket.type = packetBuffer[0] & 48 >> 4;   // zczytywanie pola typ
    cPacket.tokenlen = packetBuffer[0] & 15;    // zczytywanie dlugosci tokena
    cPacket.code = packetBuffer[1];   // zczytywanie klasy pola kod
    cPacket.bitClass = packetBuffer[1] >> 5;    // zczytywanie detali pola kod
    
    Serial.print("BitClass: "); Serial.println(cPacket.bitClass);   // wyświetlenie detali pola kod
    unsigned char temp = packetBuffer[1] >> 5;    // zapisanie do zmiennej tymczasowej detali pola kod
    cPacket.bitCode = packetBuffer[1] ^ (temp << 5);    // zczytanie szczegółów pola kod
    Serial.print("BitCode: "); Serial.println(cPacket.bitCode);   // wyświetlenie szczegółów pola kod  
    
    cPacket.messageId = (packetBuffer[2] << 8) | packetBuffer[3];   // łączenie 2 bajtów (Message ID zajmuje łącznie 2 bajty)
    
    // obsluga tokena // DOPISAC @TODO
    if (cPacket.tokenlen > 0) {
      uint8_t token = new uint8_t[cPacket.tokenlen]; //uwaga moze nie byc dobrze - tkl to nie token morddy // DOPISAC @TODO
    } else {
      uint8_t token = NULL;
    }
    
    //obsługa opcji - iterujemy się po bitach opcji
    int optionNumber = 0;   // aktualna wartość opcji
    bool isNextOption = true;
    int currentByte = 4 + cPacket.tokenlen;   // indeks opcji

    // petla do zczytywania opcji
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
      
      Serial.print("Delta: ");
      Serial.println(cPacket.cOption[optionNumber].delta);
      Serial.print("Lenght: ");
      Serial.println(cPacket.cOption[optionNumber].optionLength);
      
      // zczytywanie wartości opcji
      currentByte++;
      if (cPacket.cOption[optionNumber].optionLength > 0) {
        cPacket.cOption[optionNumber].optionValue = new uint8_t[cPacket.cOption[optionNumber].optionLength];
        for (int i = 0 ; i < cPacket.cOption[optionNumber].optionLength; i++) {
          cPacket.cOption[optionNumber].optionValue[i] = packetBuffer[currentByte];
          currentByte++;
        }
      }
      
      // sprawdzenie czy są jeszcze dalsze opcje
      if (packetBuffer[currentByte] == NULL) {
        isNextOption = false;
      }
      
      if (packetBuffer[currentByte] == 255) {
        isNextOption = false;
      }
      optionNumber++;
      Serial.println("Wyjście z pętli LoOoOoOoP, która sprawdza opcje");
    }

    // Wyświetlenie opcji wiadomości Coap
    for(int i = 0 ; i < 5 ; i++){
      Serial.print("Coap option nr: ");
      Serial.print(i);
      Serial.print(" Delta of option: ");
      Serial.print(cPacket.cOption[i].delta);
      Serial.print(" Lenght of option value is: ");  
      Serial.print(cPacket.cOption[i].optionLength);
      Serial.println();
    }
    Serial.print("Size od udp: ");
    Serial.println(packetSize);
    Serial.print("Current Byte: ");
    Serial.println(currentByte);

    
    // Obsługa payload'u
    int i =0;
    
       while(i < packetSize){
        if (packetBuffer[i] == 255){ //payload zaczyna sie od bajtu ff, czyli 255
          int j = 0;
          while(packetBuffer[i]){
            ++i;
            cPacket.payload[j] = packetBuffer[i]; //zapisanie bajtu payloadu do struktury
            ++j;
            if((cPacket.payload[j] != '\0') and (cPacket.payload[j] > '9' or cPacket.payload[j] < '0' or j > 5)){ 
            }
          }
        }
        ++i;
      }
    
   // obsługa żądania GET
   if(cPacket.bitClass == 0 && cPacket.bitCode == 1){ // 01 to bity w polu 'Code' odpowiedzialne za wiadomość GET
      Serial.println("Dostałem żądanie GET");
      long n = 12;
      byte passOptions[] = { 0b1100 << 4 | 0b1, 0b0,
                             0b1011 << 4 | 0b1, 0b10,
                             0b0101 << 4 | 0b1, (byte)(countDigit(n) + 1)
                           };
                           
      sendResponse(passOptions, sizeof(passOptions), "69420", sizeof("69420"), 2, 5, cPacket.messageId); //wysyłanie payloadu z powrotem (tymczasowo)
   };


  
    //obsluga rodzaju wiadomosci DISCOVER (BRAKUJE WARUNKU W IF)
    if(cPacket.bitClass == 0 && cPacket.bitCode == 1){
      Serial.println("Dostałem żądanie DISCOVER");
      char discoverPayload1[] = "</button>;obs;rt=\"observe\",</light>;title=\"Light which can be";
      char discoverPayload2[] = "set to 0-100\";</stats>;";
      byte passOptionsD1[] = { 0b100 << 4 |0b1, 0b1111111,//etag
                               0b1000 << 4 | 0b1, 0b101000, //content-format
                               0b1011 << 4 | 0b1, 0b1010, //block2
                               0b0101 << 4 | 0b1, (byte)sizeof(discoverPayload1) + (byte)sizeof(discoverPayload2)}; //size2
                               
      byte passOptionsD2[] = { 0b100 << 4 |0b1, 0b1111111, //etag
                               0b1000 << 4 | 0b1, 0b101000, //content-format
                               0b1011 << 4 | 0b1, 0b100010, //block2
                               0b0101 << 4 | 0b1, (byte)sizeof(discoverPayload2) + (byte)sizeof(discoverPayload1)}; //size2
      Serial.println(discoverPayload1);
      Serial.println(discoverPayload2);
      sendResponse(passOptionsD1, sizeof(passOptionsD1), discoverPayload1, sizeof(discoverPayload1), 2, 5, cPacket.messageId); //odeslij 64 bajty wiadomosci
      sendResponse(passOptionsD2, sizeof(passOptionsD2), discoverPayload2, (byte)sizeof(discoverPayload2), 2, 5); //odeslij reszte wiadomosci
    }

  }
  
  /*
  // Wyświetlenie wszystkich parametrów wiadomości CoAPa
  if (debug) {
    Serial.println();
  }
  Serial.print("CoAP Version: ");
  Serial.println(cPacket.coapVersion);
  Serial.print("CoAP Code: ");
  Serial.println(cPacket.code);
  Serial.print("CoAP TokenLength: ");
  Serial.println(cPacket.tokenlen);
  Serial.print("CoAP MessageType: ");
  Serial.println(cPacket.type);
  Serial.print("CoAP MessageID: ");
  Serial.println(cPacket.messageId);
  Serial.println("CoAP Payload: ");
  Serial.println(cPacket.payload);
  */
}
