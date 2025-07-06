#ifndef OLED_H
#define OLED_H

#include <Adafruit_SSD1306.h>

//Global display object
extern Adafruit_SSD1306 display;

void initDisplay();
void updateOLEDgps();
void updateOLEDcal();

#endif