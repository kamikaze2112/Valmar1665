#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

namespace Encoder {
  void begin(int pinA);
  void resetRevolutions();
  void update();

  extern float rpm;
  extern float revs;
  extern bool isMoving;
}

#endif
