#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    printf("Server dang lang nghe tren cong %s...\n", argv[1]);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);

    printf(">> Client da ket noi!\n");
    printf("----------------------------------------\n");

    FILE *stream = fdopen(client_sock, "r");
    char buffer[1024];

    // Doc ten thu muc tu client (dong dau tien)
    if (fgets(buffer, sizeof(buffer), stream) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';
        printf("[Thu muc]: %s\n", buffer);
        printf("[Danh sach tap tin va kich thuoc]:\n");
    }

    // Doc xen ke ten file (text) va kich thuoc (binary)
    while (fgets(buffer, sizeof(buffer), stream) != NULL) {
        // Kiem tra ket thuc 
        if (strcmp(buffer, "\n") == 0) break;

        buffer[strcspn(buffer, "\n")] = '\0';
        char filename[1024];
        strcpy(filename, buffer);

        // Doc kich thuoc file (4 bytes)
        uint32_t net_size;
        if (fread(&net_size, sizeof(uint32_t), 1, stream) != 1) {
            break;
        }

        uint32_t host_size = ntohl(net_size);
        printf("  + %s - %u bytes\n", filename, host_size);
    }

    printf("----------------------------------------\n");
    printf("Da nhan xong. Dong ket noi.\n");

    fclose(stream);
    close(server_sock);

    return 0;
}