#include <WiFi.h>
#include "constantes.h"

#define MAX_NMEA_LENGHT 90

const int lf = 10;    // Linefeed in ASCII
const int   watchdog = 5000;        // Fréquence du watchdog - Watchdog frequency
unsigned long previousMillis = millis();

char* tmpNmeaSentence; // Temp line to check NMEA checksum when line received

void setup() {
  Serial.begin(115200);
  Serial.print("Connexion à ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi Connecté");
  Serial.println("Adresse IP: ");
  Serial.println(WiFi.localIP());
}

 bool isNMEAChecksumValid(String sentence){
   int checksum = 0;

   char* tmpLine = tmpNmeaSentence;
   sentence.toCharArray(tmpLine, sentence.length()+1);

   tmpLine++;
   while (tmpLine[0] != '*') {
     checksum = checksum ^ tmpLine[0];
     tmpLine++;
   }

  tmpLine++;

  int printedChecksum = strtol(tmpLine, NULL, 16);
  tmpLine = NULL;

  return printedChecksum == checksum;
 }

void loop() {
  unsigned long currentMillis = millis();

  if ( currentMillis - previousMillis > watchdog ) {
    previousMillis = currentMillis;
    WiFiClient client;

    if (!client.connect(NMEA_HOST, NMEA_PORT)) {
      Serial.println("Connexion échoué");
      return;
    }

    String url = "/watchdog?command=watchdog&uptime=";
    url += String(millis());
    url += "&ip=";
    url += WiFi.localIP().toString();


    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "NMEA_HOST: " + NMEA_HOST + "\r\n" +
               "Connection: close\r\n\r\n");
    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> Client expiré !");
        client.stop();
        return;
      }
    }

   tmpNmeaSentence = (char*)malloc(MAX_NMEA_LENGHT*sizeof(char));

    while(client.available()){
      String line = client.readStringUntil(lf);
      Serial.print("Check line " + line + '\n');
      bool checksum = isNMEAChecksumValid(line);
      Serial.printf("Is checksum valid ? %d\n", checksum);
    }
    free(tmpNmeaSentence);

  }
}
