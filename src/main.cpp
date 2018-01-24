#include <Define.h>
#include <SSD1306.h> //https://github.com/squix78/esp8266-oled-ssd1306
#include <Display.h>
#include <Connection_Network.h>
#include <WiFiParameters.h>
#include <NMEA.h>
#include <PID_v1.h> //https://github.com/br3ttb/Arduino-PID-Library
#include <Auto_Pilot.h>

String Nmea_display[DISPLAY_LINES_NMEA];

bool pilotEngaged;
bool Button_Target_Cap_More_10_Pressed = false;
bool Button_Target_Cap_Less_10_Pressed = false;

const double Available_Ram_Length = (Electric_Ram_Length - Avoid_Stop_Electric_Ram)/2; //Available ram length in centimeters

double Target_Cap = 0;
double Current_Cap;
double Output;
double Kp=1, Ki=0.2, Kd=1;
//PID::SetOutputLimits(0, 360); Go to PID_v1.cpp and change to PID::SetOutputLimits(0, 255) from PID::SetOutputLimits(your, values).

String NMEA_Line;
String NMEA_Header;

unsigned long millis_button_retained = 0;
unsigned long milllis_button_more_10 = 0;
unsigned long millis_button_less_10 = 0;
int test_direction;

PID myPID(&Current_Cap, &Output, &Target_Cap, Kp, Ki, Kd, test_direction);

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
  if (millis_button_retained + 500 < millis()) {
    if(Current_Cap != CURRENT_CAP_UNINITIALIZED) {
      pilotEngaged = true;
      Target_Cap = Current_Cap;
    }
    millis_button_retained = millis();
  }
}

void Target_Cap_More_10_Pressed() {
  if (milllis_button_more_10 + 500 < millis()) {
    Target_Cap += 10;
    milllis_button_more_10 = millis();
  }
}

void Target_Cap_Less_10_Pressed() {
  if (millis_button_less_10 + 500 < millis()) {
    Target_Cap -= 10;
    millis_button_less_10 = millis();
  }
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

  if(!nmeaConnected()) {
    if (!isWifiConnected()) {
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
    NMEA_Line = readNmeaLine();
    NMEA_Header = getValue(NMEA_Line, ',', 0).substring(3);
    if (isNMEAChecksumValid(NMEA_Line)) {

      if (NMEA_Header == "HDG") {
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
