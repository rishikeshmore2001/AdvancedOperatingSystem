// client.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVICE_MAP_PORT 58576
#define SERVICE_NAME "CISBANK"
#define BUFLEN 128

void error_exit(const char *msg) {
    perror(msg);
    exit(1);
}

void get_service_location(char *ip_str, int *port) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVICE_MAP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    snprintf(buffer, sizeof(buffer), "GET %s", SERVICE_NAME);

    sendto(sockfd, buffer, strlen(buffer), 0,
           (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    socklen_t addrlen = sizeof(serv_addr);
    int n = recvfrom(sockfd, buffer, BUFLEN - 1, 0,
                     (struct sockaddr *)&serv_addr, &addrlen);
    buffer[n] = '\0';

    if (strncmp(buffer, "ERR", 3) == 0) {
        printf("Service lookup failed: %s\n", buffer);
        exit(1);
    }

    int a, b, c, d, p1, p2;
    sscanf(buffer, "%d,%d,%d,%d,%d,%d", &a, &b, &c, &d, &p1, &p2);
    sprintf(ip_str, "%d.%d.%d.%d", a, b, c, d);
    *port = (p1 << 8) + p2;

    close(sockfd);
}

void connect_and_send(const char *ip, int port, int acctnum, float *update) {
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return;
    }

    int net_acct = htonl(acctnum);
    send(sockfd, &net_acct, sizeof(net_acct), 0);

    if (update) {
        unsigned char *p = (unsigned char *)update;
        unsigned char net_float[4] = { p[3], p[2], p[1], p[0] };
        send(sockfd, net_float, 4, 0);
    }

    char response[128] = {0};
    int n = recv(sockfd, response, sizeof(response) - 1, 0);
    if (n > 0) {
        response[n] = '\0';
        printf("Server Response: %s", response);
    } else {
        printf("No response from server.\n");
    }

    close(sockfd);
}

int main() {
    char ip[32];
    int port;
    get_service_location(ip, &port);
    printf("Service Provided by %s at %d\n", ip, port);

    char input[128];
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        // Remove newline
        input[strcspn(input, "\n")] = 0;

        if (strncmp(input, "quit", 4) == 0) {
            break;
        }

        int acctnum;
        float amount;
        if (sscanf(input, "query %d", &acctnum) == 1) {
            connect_and_send(ip, port, acctnum, NULL);
        } else if (sscanf(input, "update %d %f", &acctnum, &amount) == 2) {
            connect_and_send(ip, port, acctnum, &amount);
        } else {
            printf("Invalid command. Use: query <acctnum> | update <acctnum> <amount> | quit\n");
        }
    }
    return 0;
}
