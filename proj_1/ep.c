#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void start_timer();
float stop_timer();

void ep() {
     pid_t child_pid;
     start_timer();

     if ((child_pid = fork()) == 0) {
         exit(0);
     } else {
         waitpid(child_pid, NULL, 0);
     }

     printf("Process creation/deletion time: %f seconds\n", stop_timer());
}