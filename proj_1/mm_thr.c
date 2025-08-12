#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define SIZE 320
void start_timer();
float stop_timer();

int A[SIZE][SIZE], B[SIZE][SIZE], C[SIZE][SIZE];
int thread_count;

void *multiply(void *arg) {
     int thread_id = *(int *)arg;
     int rows_per_thread = SIZE / thread_count;
     int start = thread_id * rows_per_thread;
     int end = (thread_id == thread_count - 1) ? SIZE : start + rows_per_thread;

     for (int i = start; i < end; i++) {
         for (int j = 0; j < SIZE; j++) {
             C[i][j] = 0;
             for (int k = 0; k < SIZE; k++) {
                 C[i][j] += A[i][k] * B[k][j];
             }
         }
     }
     return NULL;
}
int main(int argc, char *argv[]) {
     if (argc != 2) {
         fprintf(stderr, "Usage: %s thread_count\n", argv[0]);
         exit(EXIT_FAILURE);
     }
     thread_count = atoi(argv[1]);
     pthread_t threads[thread_count];
     int thread_ids[thread_count];

     start_timer();
     for (int i = 0; i < thread_count; i++) {
         thread_ids[i] = i;
         pthread_create(&threads[i], NULL, multiply, &thread_ids[i]);
     }
     for (int i = 0; i < thread_count; i++) {
         pthread_join(threads[i], NULL);
     }
     printf("Matrix multiplication time: %f seconds\n", stop_timer());

     return 0;
}
