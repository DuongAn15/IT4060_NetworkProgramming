#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

#define PORT 8080

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Chat Server is listening on port %d...\n", PORT);

    fd_set fdread, fdtest;
    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);

    // Mảng lưu tên client, nếu name[i][0] == 0 nghĩa là chưa đăng nhập
    char client_names[FD_SETSIZE][50];
    memset(client_names, 0, sizeof(client_names));

    char buf[1024];

    while (1) {
        fdtest = fdread;
        int ret = select(FD_SETSIZE, &fdtest, NULL, NULL, NULL);

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &fdtest)) {
                if (i == listener) {
                    int client = accept(listener, NULL, NULL);
                    FD_SET(client, &fdread);
                    char *msg = "Hay gui ten theo cu phap 'client_id: client_name'\n";
                    send(client, msg, strlen(msg), 0);
                } else {
                    ret = recv(i, buf, sizeof(buf) - 1, 0);
                    if (ret <= 0) {
                        FD_CLR(i, &fdread);
                        memset(client_names[i], 0, 50);
                        close(i);
                    } else {
                        buf[ret] = 0;
                        // Xóa ký tự xuống dòng nếu có
                        if (buf[ret-1] == '\n') buf[ret-1] = 0;

                        if (client_names[i][0] == 0) {
                            // Chưa đăng nhập, kiểm tra cú pháp [cite: 4, 5]
                            char name[50];
                            if (sscanf(buf, "client_id: %s", name) == 1) {
                                strcpy(client_names[i], name);
                                send(i, "Dang nhap thanh cong!\n", 22, 0);
                            } else {
                                char *err = "Sai cu phap! Yeu cau: 'client_id: client_name'\n";
                                send(i, err, strlen(err), 0);
                            }
                        } else {
                            // Đã đăng nhập, tiến hành broadcast kèm timestamp 
                            time_t now = time(NULL);
                            struct tm *t = localtime(&now);
                            char time_buf[30];
                            strftime(time_buf, sizeof(time_buf), "%Y/%m/%d %I:%M:%S%p", t);

                            char out_buf[2048];
                            sprintf(out_buf, "%s %s: %s\n", time_buf, client_names[i], buf);

                            for (int j = 0; j < FD_SETSIZE; j++) {
                                // Gửi cho các client khác đã đăng nhập
                                if (FD_ISSET(j, &fdread) && j != listener && j != i && client_names[j][0] != 0) {
                                    send(j, out_buf, strlen(out_buf), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}