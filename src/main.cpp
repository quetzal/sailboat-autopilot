#include <WiFi.h>
#include "constantes.h"
#include "SSD1306.h" //https://github.com/squix78/esp8266-oled-ssd1306
#include <Wire.h>

SSD1306  display(0x3c, 5, 4);

#define MAX_NMEA_LENGHT 90

const int lf = 10;    // Linefeed in ASCII
const int   watchdog = 5000;        // Fréquence du watchdog - Watchdog frequency
unsigned long previousMillis = millis();

char* tmpNmeaSentence; // Temp line to check NMEA checksum when line received
WiFiClient client;
String NMEA_Line;
String NMEA_Header;

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void display_nmea(String s){
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, s);
  display.display();
}

void setup() {
  Serial.begin(115200);
  Serial.print("Connexion à ");
  Serial.println(WIFI_SSID);
  tmpNmeaSentence = (char*)malloc(MAX_NMEA_LENGHT*sizeof(char));

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i++ > 10) {
      WiFi.reconnect();
      Serial.println(" WiFi reconnect");
      i = 0;
    }
  }

  Serial.println("");
  Serial.println("WiFi Connecté");
  Serial.println("Adresse IP: ");
  Serial.println(WiFi.localIP());

  display.init();
  display.flipScreenVertically();

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

void connectToNmeaSocket() {
  unsigned long currentMillis = millis();

  if ( currentMillis - previousMillis > watchdog ) {
    previousMillis = currentMillis;

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
  }
}


void loop() {

  if(!client.available()){
    connectToNmeaSocket();
  } else {
    NMEA_Line = client.readStringUntil(lf);
    NMEA_Header = getValue(NMEA_Line, ',', 0).substring(3);
    if (isNMEAChecksumValid(NMEA_Line)) {

      if (NMEA_Header == "HDT") {
        float Cap = getValue(NMEA_Line, ',', 1).toFloat();
        Serial.printf("Cap = %.1f °\n", Cap);
        display_nmea("Cap = " + String(Cap));
      }
      else if (NMEA_Header == "VTG") {
        float Speed = getValue(NMEA_Line, ',', 7).toFloat();
        Serial.printf("Speed = %.2f K/H \n", Speed);

      }
      /*TODO à finir MVM
      else if (NMEA_Header == "MVM") {
    }*/
    else if (NMEA_Header == "RMC") {
      String Time = getValue(NMEA_Line, ',', 1);
      Serial.print("Time = " + Time.substring(0, 2) + ":" + Time.substring(2, 4) + '\n');
    }
  }

}
}
