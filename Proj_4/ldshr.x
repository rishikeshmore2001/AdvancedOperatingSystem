struct load_result {
    double load_avg;
};

struct max_params {
    int N;
    double M;
    int S;
};

struct max_result {
    double max_value;
};

struct node {
    double value;
    node *next;
};

struct list_result {
    node *head;
};

program LOADSHR_PROG {
    version LOADSHR_VERS {
        load_result GETLOAD(void) = 1;
        max_result GETMAX_GPU(max_params) = 2;
        list_result UPD_LST(list_result) = 3;
    } = 1;
} = 0x20000001;