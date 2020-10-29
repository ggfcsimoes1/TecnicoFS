#include "exectimer.h"
#include <sys/time.h>
#include <stdlib.h>


void startTimer(){
    gettimeofday(&itime ,NULL);
}

void stopTimer(){
    gettimeofday(&ftime ,NULL);
}


void getExecTime(){ /* Function that calculates the program's run time*/
    double milliseconds = (double) (ftime.tv_usec - itime.tv_usec) / 1000000; /* tv_usec/tv_sec are ints, so a cast is required in order to get an accurate reading */
    double seconds = (double) (ftime.tv_sec - itime.tv_sec);        
    printf("TecnicoFS completed in %.4f seconds.\n", seconds + milliseconds);
}