#include <WiFi.h>
#include "constantes.h"
#include "SSD1306.h" //https://github.com/squix78/esp8266-oled-ssd1306
#include <Wire.h>

SSD1306  display(0x3c, 5, 4);

#define LED_Tourner_droite 15
#define LED_Tourner_gauche 2
#define MAX_NMEA_LENGHT 90
#define DISPLAY_LINES_NMEA 4
#define Button_Retained_Cap 16  // digital pin of your button
#define Button_Target_Cap_More_10 26  // digital pin of your button
#define Button_Target_Cap_Less_10 25  // digital pin of your button

#define CURRENT_CAP_UNINITIALIZED -1000 //Impossible value for cap when no data

const int lf = 10;    // Linefeed in ASCII
const int watchdog = 5000;        // Fréquence du watchdog - Watchdog frequency

unsigned long previousMillis = millis();
bool pilotEngaged;

char* tmpNmeaSentence; // Temp line to check NMEA checksum when line received

WiFiClient client;

String NMEA_Line;
String NMEA_Header;
String displayedData[DISPLAY_LINES_NMEA];

int Target_Cap = 0;
int Current_Cap;

void ConnecteToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i++ > 10) {
      WiFi.reconnect();
      display.clear();
      display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, "WiFi reconnect");
      display.display();
      Serial.println(" WiFi reconnect");
      i = 0;
    }
  }
  Serial.println("");
  Serial.println("WiFi Connected");
  display.clear();
  display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, "WiFi connected");
  display.display();
  delay(1000);
  Serial.println("IP Adress: ");
  Serial.println(WiFi.localIP());
}

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

void display_nmea(){
  display.clear();
  int display_zones_size = DISPLAY_HEIGHT/DISPLAY_LINES_NMEA;

  for(int i = 0; i < DISPLAY_LINES_NMEA; i++){
    int current_zones = display_zones_size/2 + i*display_zones_size;
    display.drawString(DISPLAY_WIDTH/2, current_zones, displayedData[i]);
  }
  display.display();
}
void setup() {

  Serial.begin(115200);

  pilotEngaged = false;
  Current_Cap = CURRENT_CAP_UNINITIALIZED;

  pinMode(LED_Tourner_droite, OUTPUT);
  pinMode(LED_Tourner_gauche, OUTPUT);
  pinMode(Button_Retained_Cap, INPUT);
  pinMode(Button_Target_Cap_More_10, INPUT);
  pinMode(Button_Target_Cap_Less_10, INPUT);

  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_10);

  Serial.print("Connection to ");
  Serial.println(WIFI_SSID);
  display.clear();
  display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, "Connection to WiFi");
  display.display();

  tmpNmeaSentence = (char*)malloc(MAX_NMEA_LENGHT*sizeof(char));
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

int computeTrueTargetCap(int Target_Cap) {
  if (Target_Cap > 360) {
    Target_Cap = 0 + Target_Cap - 360;
  }
  else if (Target_Cap < 0) {
    Target_Cap = 360 + Target_Cap;
  }
  return Target_Cap;
}

void keepCap(int capToKeep) {

  if (capToKeep != computeTrueTargetCap(Target_Cap)) {
    if (Target_Cap < capToKeep) {
      digitalWrite(LED_Tourner_droite, LOW);
      digitalWrite(LED_Tourner_gauche, HIGH);
    }
    else {
      digitalWrite(LED_Tourner_gauche, LOW);
      digitalWrite(LED_Tourner_droite, HIGH);
    }
  }
}

void loop() {

  if(!client.available()){
    if (WiFi.status() != WL_CONNECTED) {
      ConnecteToWiFi();
    }
    connectToNmeaSocket();
  }
  else {
    if(digitalRead(Button_Retained_Cap) && Current_Cap != CURRENT_CAP_UNINITIALIZED) {
      pilotEngaged = true;
      Target_Cap = Current_Cap;
    }
    if (digitalRead(Button_Target_Cap_More_10)) {
      Target_Cap += 10;
    }
    if (digitalRead(Button_Target_Cap_Less_10)) {
      Target_Cap -= 10;
    }

    NMEA_Line = client.readStringUntil(lf);
    NMEA_Header = getValue(NMEA_Line, ',', 0).substring(3);
    if (isNMEAChecksumValid(NMEA_Line)) {

      if (NMEA_Header == "HDT") {
        Current_Cap = getValue(NMEA_Line, ',', 1).toInt();
        Serial.printf("Cap = %d °\n", Current_Cap);
        displayedData[0] = "Cap = " + String(Current_Cap) + "°";
        if (pilotEngaged) {
          keepCap(Current_Cap);
          displayedData[1] = "T_Cap = " + String(computeTrueTargetCap(Target_Cap)) + "°";
        }
        else {
          displayedData[1] = "Pilot is OFF";
        }
      }

      else if (NMEA_Header == "VTG") {
        float Speed = getValue(NMEA_Line, ',', 7).toFloat();
        Serial.printf("Speed = %.2f K/H \n", Speed);
        displayedData[2] = "Speed = " + String(Speed) + " K/H";

      }
      else if (NMEA_Header == "RMC") {
        String Time = getValue(NMEA_Line, ',', 1);
        Serial.print("Time = " + Time.substring(0, 2) + ":" + Time.substring(2, 4) + '\n');
        displayedData[3] = "Time = " + Time.substring(0, 2) + ":" + Time.substring(2, 4);
      }
      display_nmea();

    }
  }
}
