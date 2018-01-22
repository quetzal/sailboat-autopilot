#include <SSD1306.h>

SSD1306  display(0x3c, 5, 4);

int lines = 0;
String* displayedText;

void process_display_text() {
  display.clear();
  if(lines != 0) {
    int display_zones_lines = DISPLAY_HEIGHT/lines;
    for (int i = 0; i < lines; i++) {
      int current_zones = display_zones_lines/2 + i*display_zones_lines;
      display.drawString(DISPLAY_WIDTH/2, current_zones, displayedText[i]);
    }
    display.display();
  }
}

void initDisplay(){
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_10);
}

void display_text(String* text_to_display, int size) {
  lines = size;
  displayedText = text_to_display;
  process_display_text();
}
