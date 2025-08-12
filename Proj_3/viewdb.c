// viewdb.c
#include <stdio.h>
#include <stdlib.h>

struct record {
    int acctnum;
    char name[20];
    float value;
    int age;
};

int main() {
    FILE *fp = fopen("db25", "rb");
    if (!fp) {
        perror("Failed to open db25");
        return 1;
    }

    struct record rec;
    int count = 0;

    printf("Account Database (db25):\n");
    printf("--------------------------\n");

    while (fread(&rec, sizeof(struct record), 1, fp) == 1) {
        printf("Account: %d\n", rec.acctnum);
        printf("Name   : %.20s\n", rec.name);
        printf("Balance: %.2f\n", rec.value);
        printf("Age    : %d\n", rec.age);
        printf("--------------------------\n");
        count++;
    }

    fclose(fp);

    if (count == 0) {
        printf("No records found.\n");
    }

    return 0;
}