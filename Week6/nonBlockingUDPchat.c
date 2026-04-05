#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h> 

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <port_s> <ip_d> <port_d>\n", argv[0]);
        return 1;
    }

    int port_s = atoi(argv[1]);
    char *ip_d = argv[2];
    int port_d = atoi(argv[3]);

    int sockfd;
    struct sockaddr_in servaddr, destaddr;
    char buffer[BUF_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port_s);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    memset(&destaddr, 0, sizeof(destaddr));
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(port_d);
    inet_pton(AF_INET, ip_d, &destaddr.sin_addr);

    printf("UDP Chat started. Listening on %d...\n", port_s);

    while (1) {
        int bytes_available;

        ioctl(sockfd, FIONREAD, &bytes_available);
        if (bytes_available > 0) {
            struct sockaddr_in sender_addr;
            socklen_t addr_len = sizeof(sender_addr);
            int n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0, 
                             (struct sockaddr *)&sender_addr, &addr_len);
            if (n > 0) {
                buffer[n] = '\0';
                printf("\n[Received]: %s", buffer);
                printf("> "); fflush(stdout);
            }
        }

        ioctl(STDIN_FILENO, FIONREAD, &bytes_available);
        if (bytes_available > 0) {
            if (fgets(buffer, BUF_SIZE, stdin)) {
                sendto(sockfd, buffer, strlen(buffer), 0, 
                       (struct sockaddr *)&destaddr, sizeof(destaddr));
                printf("> "); fflush(stdout);
            }
        }

        usleep(10000); 
    }

    close(sockfd);
    return 0;
}