#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int main() {
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9000);

    int ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        perror("connect() failed");
        return 1;
    }

    printf("Ket noi thanh cong. Nhap du lieu de gui (Ctrl+C de dung):\n");

    char buf[256];
    while (1) {
        printf("> ");
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
            buf[strcspn(buf, "\n")] = 0;
            
            if (strlen(buf) > 0) {
                send(client, buf, strlen(buf), 0);
            }
        } else {
            break;
        }
    }

    close(client);
    return 0;
}