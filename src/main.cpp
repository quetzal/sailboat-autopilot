#include <WiFi.h>
#include "constantes.h"

const int   watchdog = 5000;        // Fréquence du watchdog - Watchdog frequency
unsigned long previousMillis = millis();

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

 bool isChecksumValid(String line){
   int checksum = 0;
   Serial.println(line);

   char* tmpLine = (char*)malloc((line.length()+1)*sizeof(char));
   char* tmpLine2 = tmpLine;
   line.toCharArray(tmpLine, line.length()+1);
   Serial.printf("tmpline %s\n", tmpLine);

   tmpLine++;
   while (tmpLine[0] != '*') {
     checksum = checksum ^ tmpLine[0];
     tmpLine++;
   }

  Serial.printf("checksum %d\n", checksum);

  tmpLine++;

  int printedChecksum = strtol(tmpLine, NULL, 16);
  tmpLine = NULL;
  free(tmpLine2);

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



    // Envoi la requete au serveur - This will send the request to the server
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

    // Read all the lines of the reply from server and print them to Serial
    while(client.available()){
      String line = client.readStringUntil('\r');
      Serial.print("Check line " + line);
      bool checksum = isChecksumValid(line);
      Serial.printf("Is checksum valid ? %d\n", checksum);
    }
  }
}
