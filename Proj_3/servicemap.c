// servicemap.c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 58576
#define MAX_SERVICES 10
#define BUFLEN 128

struct service_entry {
    char name[32];
    char ip_port[64];
};

struct service_entry registry[MAX_SERVICES];
int service_count = 0;

void handle_put(char *service, char *ip_port, int sock, struct sockaddr_in *client, socklen_t clientlen) {
    for (int i = 0; i < service_count; ++i) {
        if (strcmp(registry[i].name, service) == 0) {
            strncpy(registry[i].ip_port, ip_port, sizeof(registry[i].ip_port));
            sendto(sock, "OK (updated)", 12, 0, (struct sockaddr *)client, clientlen);
            return;
        }
    }
    if (service_count < MAX_SERVICES) {
        strncpy(registry[service_count].name, service, sizeof(registry[service_count].name));
        strncpy(registry[service_count].ip_port, ip_port, sizeof(registry[service_count].ip_port));
        service_count++;
        sendto(sock, "OK", 2, 0, (struct sockaddr *)client, clientlen);
    } else {
        sendto(sock, "ERR Registry Full", 18, 0, (struct sockaddr *)client, clientlen);
    }
}

void handle_get(char *service, int sock, struct sockaddr_in *client, socklen_t clientlen) {
    for (int i = 0; i < service_count; ++i) {
        if (strcmp(registry[i].name, service) == 0) {
            sendto(sock, registry[i].ip_port, strlen(registry[i].ip_port), 0,
                   (struct sockaddr *)client, clientlen);
            return;
        }
    }
    sendto(sock, "ERR Not Found", 13, 0, (struct sockaddr *)client, clientlen);
}

int main() {
    int sockfd;
    struct sockaddr_in serv_addr, client_addr;
    char buffer[BUFLEN];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind");
        exit(1);
    }

    while (1) {
        socklen_t len = sizeof(client_addr);
        int n = recvfrom(sockfd, buffer, BUFLEN - 1, 0,
                         (struct sockaddr *)&client_addr, &len);
        if (n < 0) continue;

        buffer[n] = '\0';

        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), ipstr, INET_ADDRSTRLEN);
        printf("Received from %s: %s\n", ipstr, buffer);

        char cmd[8], name[32], addr[64];
        int parsed = sscanf(buffer, "%s %s %s", cmd, name, addr);

        if (strcmp(cmd, "PUT") == 0 && parsed == 3) {
            handle_put(name, addr, sockfd, &client_addr, len);
        } else if (strcmp(cmd, "GET") == 0 && parsed >= 2) {
            handle_get(name, sockfd, &client_addr, len);
        } else {
            char *msg = "ERR Invalid Command";
            sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&client_addr, len);
        }
    }

    close(sockfd);
    return 0;
}