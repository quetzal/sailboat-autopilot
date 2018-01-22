#include <WiFi.h>
#include <SSD1306.h> //https://github.com/squix78/esp8266-oled-ssd1306
#include <PID_v1.h>
#include <Define.h>
#include "WiFiParameters.h"
#include <Connection_Network.h>
#include <NMEA.h>
#include <Display.h>
#include <Auto_Pilot.h>

String Nmea_display[DISPLAY_LINES_NMEA];

const int lf = 10;  // Linefeed in ASCII

bool pilotEngaged;
bool Button_Target_Cap_More_10_Pressed = false;
bool Button_Target_Cap_Less_10_Pressed = false;

const double Available_Ram_Length = (Electric_Ram_Length - Avoid_Stop_Electric_Ram)/2; //Available ram length in centimeters

double Target_Cap = 0;
double Current_Cap;
double Output;
double Kp=4, Ki=0.2, Kd=1;

String NMEA_Line;
String NMEA_Header;

PID myPID(&Current_Cap, &Output, &Target_Cap, Kp, Ki, Kd, DIRECT);

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
    unsigned long temps = millis();
    if (Target_Cap < capToKeep) {
      Turn_Left(3000);
      Setlastturn(LAST_TURN_LEFT);
    }
    else {
      Turn_Right(3000);
      Setlastturn(LAST_TURN_RIGHT);
    }
  }
}

void Retained_Cap_Pressed() {
  if(Current_Cap != CURRENT_CAP_UNINITIALIZED) {
    pilotEngaged = true;
    Target_Cap = Current_Cap;
  }
}

void Target_Cap_More_10_Pressed() {
  Target_Cap += 10;
}

void Target_Cap_Less_10_Pressed() {
  Target_Cap -= 10;
}

void setup() {

  Serial.begin(115200);

  myPID.SetMode(AUTOMATIC);
  myPID.SetTunings(Kp, Ki, Kd);

  pilotEngaged = false;
  Current_Cap = CURRENT_CAP_UNINITIALIZED;

  pinMode(Button_Retained_Cap, INPUT_PULLDOWN);
  pinMode(Button_Target_Cap_More_10, INPUT_PULLDOWN);
  pinMode(Button_Target_Cap_Less_10, INPUT_PULLDOWN);

  attachInterrupt(digitalPinToInterrupt(Button_Retained_Cap), Retained_Cap_Pressed, RISING);
  attachInterrupt(digitalPinToInterrupt(Button_Target_Cap_More_10), Target_Cap_More_10_Pressed, RISING);
  attachInterrupt(digitalPinToInterrupt(Button_Target_Cap_Less_10), Target_Cap_Less_10_Pressed, RISING);

  String textTmp[2];
  textTmp[0] = "Connection to";
  textTmp[1] = (WIFI_SSID);
  initDisplay();
  display_text(textTmp, 2);

  initNMEA();
}

void loop() {

  if(!getClient().available()){
    if (WiFi.status() != WL_CONNECTED) {
      ConnecteToWiFi(WIFI_SSID, WIFI_PASSWORD);
      String wifi_display[3];
      wifi_display[0] = "WiFi connected";
      wifi_display[1] = "Address IP: ";
      wifi_display[2] = GetIp().toString();
      display_text(wifi_display, 3);
    }
    ConnectToNmeaSocket(NMEA_HOST, NMEA_PORT);
  }
  else {
    NMEA_Line = getClient().readStringUntil(lf);
    NMEA_Header = getValue(NMEA_Line, ',', 0).substring(3);
    if (isNMEAChecksumValid(NMEA_Line)) {

      if (NMEA_Header == "HDT") {
        Current_Cap = getValue(NMEA_Line, ',', 1).toInt();
        Nmea_display[0] = "Cap = " + String(Current_Cap) + "°";

        if (pilotEngaged) {
          Nmea_display[1] = "T_Cap = " + String(computeTrueTargetCap(Target_Cap)) + "°";

          if ((Target_Cap - Current_Cap) >= Accepted_Cap_Error && (Current_Cap - Target_Cap) <= Accepted_Cap_Error) {
            keepCap(Current_Cap);
          }
          myPID.Compute();
          Serial.print(String(Output) + "\n");
        }
        else {
          Nmea_display[1] = "Pilot is OFF";
        }
      }
      else if (NMEA_Header == "VTG") {
        float Speed = getValue(NMEA_Line, ',', 7).toFloat();
        Nmea_display[2] = "Speed = " + String(Speed) + "K/H";
      }
      else if (NMEA_Header == "RMC") {
        String Time = getValue(NMEA_Line, ',', 1);
        Nmea_display[3] = "Time = " + Time.substring(0, 2) + ":" + Time.substring(2, 4);
      }
      display_text(Nmea_display, DISPLAY_LINES_NMEA);
    }
  }
}
