#include <sys/time.h>
#include <stdio.h>

void start_timer();
float stop_timer();

static struct timeval start, end;

void start_timer() {
    gettimeofday(&start, NULL);
}

float stop_timer() {
    gettimeofday(&end, NULL);
    return (end.tv_sec - start.tv_sec) + ((double)(end.tv_usec - start.tv_usec) / 1000000.0);
}
