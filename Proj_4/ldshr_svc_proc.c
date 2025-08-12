#include "ldshr.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>  // Required for printf

// Server-side getload RPC implementation
load_result *getload_1_svc(void *arg, struct svc_req *rqstp) {
    static load_result res;
    double load[3];

    if (getloadavg(load, 3) == -1) {
        res.load_avg = -1.0; // Error case
    } else {
        res.load_avg = load[0]; // 1-minute load average
    }

    return &res;
}

// GPU wrapper function declared from getmax.cu
extern double getmax_cpu(int N, double M, int seed);

// Server-side getmax_gpu RPC implementation
max_result *getmax_gpu_1_svc(max_params *params, struct svc_req *rqstp) {
    static max_result res;

    // Call the CUDA max computation
    res.max_value = getmax_cpu(params->N, params->M, params->S);

    // Print result on server for debug/log
    printf("GPU result (printed on server): %f\n", res.max_value);

    return &res;
}

// Server-side upd_lst RPC implementation
list_result *upd_lst_1_svc(list_result *lst, struct svc_req *rqstp) {
    static list_result res;
    node *current = lst->head;
    node *new_head = NULL, *new_tail = NULL;

    // Traverse and update the linked list
    while (current) {
        node *new_node = (node *)malloc(sizeof(node));
        if (!new_node) {
            // Free previously allocated nodes on error
            while (new_head) {
                node *temp = new_head;
                new_head = new_head->next;
                free(temp);
            }
            res.head = NULL;
            return &res;
        }

        new_node->value = sqrt(current->value) * 10.0;
        new_node->next = NULL;

        if (!new_head) {
            new_head = new_tail = new_node;
        } else {
            new_tail->next = new_node;
            new_tail = new_node;
        }

        current = current->next;
    }

    res.head = new_head;
    return &res;
}
