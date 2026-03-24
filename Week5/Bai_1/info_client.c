#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <IP_address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect() failed");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected successfully!\n");

    char send_buf[1050];

    // Gui duong dan thu muc + \n
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Thu muc hien tai: %s\n", cwd);
        sprintf(send_buf, "%s\n", cwd); 
        send(client_sock, send_buf, strlen(send_buf), 0); 
    }
    else
    {
        perror("getcwd() failed");
        close(client_sock);
        return 1;
    }

    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    dir = opendir(".");
    if (dir == NULL)
    {
        perror("Khong the mo thu muc");
        close(client_sock);
        return 1;
    }

    printf("Dang gui danh sach tap tin...\n");
    while ((entry = readdir(dir)) != NULL)
    {
        // Bỏ qua thư mục "." và ".." 
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (stat(entry->d_name, &file_stat) == 0) {
            // Gui ten file + \n
            sprintf(send_buf, "%s\n", entry->d_name);
            send(client_sock, send_buf, strlen(send_buf), 0);

            // Gui kich thuoc file so nguyen 4 bytes theo dinh dang big-endian
            uint32_t net_size = htonl((uint32_t)file_stat.st_size);
            send(client_sock, &net_size, sizeof(net_size), 0);

            printf(">> Da gui: %s - %lld bytes\n", entry->d_name, (long long)file_stat.st_size);
        }
    }

    //Bao ket thuc
    send(client_sock, "\n", 1, 0);

    closedir(dir);
    close(client_sock);
    printf("Hoan thanh va dong ket noi.\n");
    return 0;
}