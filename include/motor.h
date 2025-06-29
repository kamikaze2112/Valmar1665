#ifndef MOTOR_H
#define MOTOR_H

extern float actualRate;
extern bool motorActive;
extern unsigned long lastUpdate;
extern const unsigned long updateInterval;

void handleCalButton();
void setMotorPWM(int pwm);



#endif