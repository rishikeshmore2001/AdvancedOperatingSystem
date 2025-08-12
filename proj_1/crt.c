#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern void ep();
extern void et();

int main(int argc, char *argv[]) {
     if (argc < 2) {
         fprintf(stderr, "Usage: %s -a|-b memory_size\n", argv[0]);
         exit(EXIT_FAILURE);
     }

     if (strstr(argv[0], "pcrt")) {
         ep();
     } else if (strstr(argv[0], "tcrt")) {
         et();
     } else {
         fprintf(stderr, "Unknown executable name\n");
         exit(EXIT_FAILURE);
     }
     return 0;
}