#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    int opt = 1; 
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { 
        perror("setsockopt failed"); 
        exit(EXIT_FAILURE); 
    }

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        exit(1);
    }

    if (listen(listener, 5) < 0) {
        perror("listen() failed");
        exit(1);
    }

    printf("Waiting for client\n");
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client = accept(listener, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client < 0) {
        perror("accept() failed");
        exit(1);
    }

    printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));

    char buf[256];
    // Buffer luu phan du cuoi cung de xet ket hop voi du lieu moi
    char remain_buf[10] = ""; 
    char target[] = "0123456789";
    int total_occurrences = 0;

    while (1) {
        int n = recv(client, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            printf("Disconnected\n");
            break;
        }
        buf[n] = 0;

        //Tao xau tam thoi ket hop du lieu con lai va du lieu moi nhan duoc
        char temp[512]; 
        strcpy(temp, remain_buf);
        strcat(temp, buf);

        // Dem so lan xuat hien cua xau 0123456789
        char *ptr = temp;
        while ((ptr = strstr(ptr, target)) != NULL) {
            total_occurrences++;
            ptr += strlen(target);
        }

        printf("Nhan %d bytes. Tong so lan xuat hien '%s': %d\n", n, target, total_occurrences);

        // Cap nhat lai remain_buf
        int temp_len = strlen(temp);
        if (temp_len >= 9) {
            strcpy(remain_buf, temp + temp_len - 9);
        } else {
            strcpy(remain_buf, temp);
        }
    }

    close(client);
    close(listener);
    return 0;
}