#include "ldshr.h"
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HOSTS 100
#define MAX_LINE 256

int main(int argc, char *argv[]) {
    char hosts[MAX_HOSTS][MAX_LINE];
    int host_count = 0;
    FILE *fp;
    char line[MAX_LINE];

    // Read machinefile
    fp = fopen("machinefile", "r");
    if (!fp) {
        fprintf(stderr, "Error opening machinefile\n");
        exit(1);
    }
    while (fgets(line, MAX_LINE, fp) && host_count < MAX_HOSTS) {
        line[strcspn(line, "\n")] = 0; // Remove newline
        strncpy(hosts[host_count], line, MAX_LINE - 1);
        hosts[host_count][MAX_LINE - 1] = 0;
        host_count++;
    }
    fclose(fp);

    if (host_count == 0) {
        fprintf(stderr, "No hosts in machinefile\n");
        exit(1);
    }

    // Get load averages
    double min_load = 1e9;
    char *min_host = NULL;
    for (int i = 0; i < host_count; i++) {
        CLIENT *clnt = clnt_create(hosts[i], LOADSHR_PROG, LOADSHR_VERS, "tcp");
        if (!clnt) {
            fprintf(stderr, "Failed to connect to %s\n", hosts[i]);
            continue;
        }
        load_result *load = getload_1(NULL, clnt);
        if (!load) {
            fprintf(stderr, "Failed to get load from %s\n", hosts[i]);
            clnt_destroy(clnt);
            continue;
        }
        printf("%s: %.2f ", hosts[i], load->load_avg);
        if (load->load_avg >= 0 && load->load_avg < min_load) {
            min_load = load->load_avg;
            min_host = hosts[i];
        }
        clnt_destroy(clnt);
    }
    printf("\n");

    if (!min_host) {
        fprintf(stderr, "No valid hosts found\n");
        exit(1);
    }
    printf("(executed on %s)\n", min_host);

    // Connect to chosen machine
    CLIENT *clnt = clnt_create(min_host, LOADSHR_PROG, LOADSHR_VERS, "tcp");
    if (!clnt) {
        fprintf(stderr, "Failed to connect to %s\n", min_host);
        exit(1);
    }

    // Process command-line arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s -max N M S | -upd val1 val2 ...\n", argv[0]);
        clnt_destroy(clnt);
        exit(1);
    }

    if (strcmp(argv[1], "-max") == 0) {
        if (argc != 5) {
            fprintf(stderr, "Usage: %s -max N M S\n", argv[0]);
            clnt_destroy(clnt);
            exit(1);
        }
        max_params params;
        params.N = atoi(argv[2]);
        params.M = atof(argv[3]);
        params.S = atoi(argv[4]);
        max_result *res = getmax_gpu_1(&params, clnt);
        if (!res) {
            fprintf(stderr, "Failed to execute getmax_gpu\n");
            clnt_destroy(clnt);
            exit(1);
        }
        printf("%.6f\n", res->max_value);

    } else if (strcmp(argv[1], "-upd") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s -upd val1 val2 val3 ...\n", argv[0]);
            clnt_destroy(clnt);
            exit(1);
        }

        list_result lst;
        lst.head = NULL;
        node *tail = NULL;

        // Build linked list from argv[2] to argv[argc - 1]
        for (int i = 2; i < argc; i++) {
            node *new_node = (node *)malloc(sizeof(node));
            if (!new_node) {
                fprintf(stderr, "Memory allocation failed\n");
                clnt_destroy(clnt);
                exit(1);
            }
            new_node->value = atof(argv[i]);
            new_node->next = NULL;

            if (!lst.head) {
                lst.head = tail = new_node;
            } else {
                tail->next = new_node;
                tail = new_node;
            }
        }

        list_result *res = upd_lst_1(&lst, clnt);
        if (!res) {
            fprintf(stderr, "Failed to execute upd_lst\n");
            clnt_destroy(clnt);
            exit(1);
        }

        node *current = res->head;
        while (current) {
            printf("%.1f", current->value);
            current = current->next;
            if (current) printf(" ");
        }
        printf("\n");

        // Free updated list
        current = res->head;
        while (current) {
            node *temp = current;
            current = current->next;
            free(temp);
        }

        // Free original list
        current = lst.head;
        while (current) {
            node *temp = current;
            current = current->next;
            free(temp);
        }

    } else {
        fprintf(stderr, "Invalid option: %s\n", argv[1]);
        clnt_destroy(clnt);
        exit(1);
    }

    clnt_destroy(clnt);
    return 0;
}
