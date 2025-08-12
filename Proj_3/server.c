// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


#define DBFILE "db25"
#define SERVICE_NAME "CISBANK"
#define SERVICE_MAP_PORT 58576  // Same as in servicemap.c
#define RECORD_SIZE sizeof(struct record)

struct record {
    int acctnum;
    char name[20];
    float value;
    int age;
};

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void register_service(int port) {
    int sockfd;
    struct sockaddr_in serv_addr, local_addr;
    char buffer[128];

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Service mapper address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVICE_MAP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // Dummy connect so getsockname gives us the real local IP
    connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // Get local address assigned for this connection
    socklen_t len = sizeof(local_addr);
    getsockname(sockfd, (struct sockaddr *)&local_addr, &len);

    // Convert to dotted IP
    char ip_text[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local_addr.sin_addr, ip_text, sizeof(ip_text));

    // Print as required
    printf("Registration OK from %s\n", ip_text);

    // Split port into high/low bytes
    unsigned char port_high = (port >> 8) & 0xFF;
    unsigned char port_low = port & 0xFF;

    // Format PUT message using dotted IP
    // Convert dots to commas for the PUT message (mapper expects commas)
    char ip_commas[32];
    strncpy(ip_commas, ip_text, sizeof(ip_commas));
    for (char *p = ip_commas; *p; ++p) {
        if (*p == '.') *p = ',';
    }

    snprintf(buffer, sizeof(buffer), "PUT %s %s,%d,%d", SERVICE_NAME, ip_commas, port_high, port_low);


    // Send registration
    sendto(sockfd, buffer, strlen(buffer), 0,
           (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL);

    close(sockfd);
}


void handle_client(int client_sock) {
    int fd = open(DBFILE, O_RDWR);
    if (fd < 0) {
        perror("open db");
        exit(1);
    }

    int acctnum;
    int n = recv(client_sock, &acctnum, sizeof(int), 0);
    if (n <= 0) return;

    acctnum = ntohl(acctnum);

    float amount;
    int is_update = 0;

    if ((n = recv(client_sock, &amount, sizeof(float), MSG_DONTWAIT)) > 0) {
        is_update = 1;
        // Assume float is in network byte order (convert manually)
        unsigned char *p = (unsigned char *)&amount;
        unsigned char tmp[4] = { p[3], p[2], p[1], p[0] };
        memcpy(&amount, tmp, 4);
    }

    struct record rec;
    off_t pos = 0;
    int found = 0;

    while (read(fd, &rec, RECORD_SIZE) == RECORD_SIZE) {
        if (rec.acctnum == acctnum) {
            found = 1;
            lseek(fd, pos, SEEK_SET);

            if (is_update) {
                lockf(fd, F_LOCK, RECORD_SIZE);
                rec.value += amount;
                write(fd, &rec, RECORD_SIZE);
                lockf(fd, F_ULOCK, RECORD_SIZE);
            }

            char response[128];
            snprintf(response, sizeof(response), "%s %d %.1f\n",
                     rec.name, rec.acctnum, rec.value);
            send(client_sock, response, strlen(response), 0);
            break;
        }
        pos += RECORD_SIZE;
    }

    if (!found) {
        char msg[] = "Account not found\n";
        send(client_sock, msg, strlen(msg), 0);
    }

    close(fd);
    close(client_sock);
    exit(0);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);

    signal(SIGCHLD, sigchld_handler);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = 0; // Let OS pick port

    bind(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // Get assigned port
    socklen_t len = sizeof(serv_addr);
    getsockname(server_sock, (struct sockaddr *)&serv_addr, &len);
    int assigned_port = ntohs(serv_addr.sin_port);

    // Register with service mapper
    register_service(assigned_port);

    listen(server_sock, 5);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addrlen);
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Service Requested from %s\n", client_ip);

        if (fork() == 0) {
            close(server_sock);
            handle_client(client_sock);
        } else {
            close(client_sock);
        }
    }

    close(server_sock);
    return 0;
}
