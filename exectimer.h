#ifndef EXECTIMER_H
#define EXECTIMER_H

struct timeval itime, ftime;

void startTimer();
void stopTimer();
void getExecTime();

#endif