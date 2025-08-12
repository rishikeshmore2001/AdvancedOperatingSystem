#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void start_timer();
float stop_timer();

void *thread_execution(void *arg) {
     return NULL;
}

void et() {
     pthread_t thread;
     start_timer();

     pthread_create(&thread, NULL, thread_execution, NULL);
     pthread_join(thread, NULL);

     printf("Thread creation/deletion time: %f seconds\n", stop_timer());
}