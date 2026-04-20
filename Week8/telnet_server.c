#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define MAX_CLIENTS 1000

int main() {
    // 1. Khởi tạo socket
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9090);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("Server is listening on port 9090...\n");

    struct pollfd fds[MAX_CLIENTS];
    int login[MAX_CLIENTS] = {0}; 
    int nfds = 1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    char buf[256];

    while (1) {
        int ret = poll(fds, nfds, 60000); 
        if (ret < 0) {
            perror("poll() failed");
            break;
        }
        if (ret == 0) {
            printf("Timed out.\n"); 
            continue;
        }

        if (fds[0].revents & POLLIN) {
            int client = accept(listener, NULL, NULL);
            if (nfds < MAX_CLIENTS) {
                printf("New client connected: %d\n", client);
                
                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;
                login[nfds] = 0;
                nfds++;
                
                char *msg = "Hay dang nhap theo cu phap 'user pass':\n";
                send(client, msg, strlen(msg), 0);
            } else {
                close(client);
            }
        }

        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                ret = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);
                
                if (ret <= 0) {
                    printf("Client %d disconnected\n", fds[i].fd);
                    close(fds[i].fd);
                    
                    fds[i] = fds[nfds - 1];
                    login[i] = login[nfds - 1];
                    nfds--;
                    i--;
                } else {
                    buf[ret] = 0;
                    if (buf[strlen(buf) - 1] == '\n') buf[strlen(buf) - 1] = 0;
                    if (buf[strlen(buf) - 1] == '\r') buf[strlen(buf) - 1] = 0;

                    printf("Received from %d: %s\n", fds[i].fd, buf);

                    if (login[i] == 0) {
                        char user[32], pass[32], tmp[64];
                        int n = sscanf(buf, "%s %s", user, pass);
                        
                        if (n != 2) {
                            char *msg = "Sai cu phap. Hay dang nhap lai.\n";
                            send(fds[i].fd, msg, strlen(msg), 0);
                        } else {
                            sprintf(tmp, "%s %s", user, pass);
                            int found = 0;
                            char line[64];
                            FILE *f = fopen("users.txt", "r");
                            if (f != NULL) {
                                while (fgets(line, sizeof(line), f) != NULL) {
                                    if (line[strlen(line) - 1] == '\n') line[strlen(line) - 1] = 0;
                                    if (strcmp(line, tmp) == 0) {
                                        found = 1;
                                        break;
                                    }
                                }
                                fclose(f);
                            }

                            if (found == 1) {
                                send(fds[i].fd, "OK. Hay nhap lenh.\n", 20, 0);
                                login[i] = 1;
                                printf("Client %d: Logged in successfully\n", fds[i].fd); 
                            } else {
                                send(fds[i].fd, "Sai username hoac password.\n", 28, 0);
                                printf("Client %d: Login failed\n", fds[i].fd); 
                            }
                        }
                    } else {
                        char cmd[512];
                        sprintf(cmd, "%s > out.txt 2>&1", buf); 
                        system(cmd);

                        FILE *f = fopen("out.txt", "rb");
                        if (f != NULL) {
                            while (1) {
                                int len = fread(buf, 1, sizeof(buf), f);
                                if (len <= 0) break;
                                send(fds[i].fd, buf, len, 0);
                            }
                            fclose(f);
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}