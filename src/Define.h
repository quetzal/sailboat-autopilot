#define DISPLAY_LINES_NMEA 4

#define CURRENT_CAP_UNINITIALIZED -1000 //Impossible value for cap when no data
#define Accepted_Cap_Error 5 // error in ° accepted by the autopilot

#define Button_Retained_Cap 16  // digital pin of your button
#define Button_Target_Cap_More_10 2 // digital pin of your button
#define Button_Target_Cap_Less_10 14  // digital pin of your button

#define Electric_Ram_Length 40 // length max of the electric ram in centimeter
#define Avoid_Stop_Electric_Ram 10 // length to avoid the stop of the electric ram in centimeter
#define Speed_Ram 2 //Elapsed time for your cylinder to move 1 centimeter in seconds
