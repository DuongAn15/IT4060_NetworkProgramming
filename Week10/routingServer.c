#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#define MAX_CLIENTS 1000
#define MAX_TOPICS_PER_CLIENT 10

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9090);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Server is listening on port 9090...\n");

    struct pollfd fds[MAX_CLIENTS];
    char *topics[MAX_CLIENTS][MAX_TOPICS_PER_CLIENT];
    memset(topics, 0, sizeof(topics));
    
    int nfds = 1;
    fds[0].fd = listener;
    fds[0].events = POLLIN;

    char buf[512];

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) break;

        if (fds[0].revents & POLLIN) {
            int client = accept(listener, NULL, NULL);
            if (nfds < MAX_CLIENTS) {
                printf("New client connected: %d\n", client);
                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;
                for (int j = 0; j < MAX_TOPICS_PER_CLIENT; j++) 
                    topics[nfds][j] = NULL;
                nfds++;
                send(client, "Welcome! Commands: SUB <topic>, UNSUB <topic>, PUB <topic> <msg>\n", 65, 0);
            } else {
                close(client);
            }
        }

        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                ret = recv(fds[i].fd, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) {
                    printf("Client %d disconnected\n", fds[i].fd);

                    for (int j = 0; j < MAX_TOPICS_PER_CLIENT; j++) {
                        if (topics[i][j]) free(topics[i][j]);
                    }
                    close(fds[i].fd);
                    fds[i] = fds[nfds - 1];
                    for (int j = 0; j < MAX_TOPICS_PER_CLIENT; j++) {
                        topics[i][j] = topics[nfds - 1][j];
                    }
                    nfds--;
                    i--;
                } else {
                    buf[ret] = 0;
                    if (buf[ret-1] == '\n') buf[ret-1] = 0;
                    printf("Log from %d: %s\n", fds[i].fd, buf);

                    char cmd[16], topic_name[32];
                    int n = sscanf(buf, "%s %s", cmd, topic_name);

                    if (n < 2) continue;

                    if (strcmp(cmd, "SUB") == 0) {
                        int added = 0;
                        for (int j = 0; j < MAX_TOPICS_PER_CLIENT; j++) {
                            if (topics[i][j] == NULL) {
                                topics[i][j] = malloc(strlen(topic_name) + 1);
                                strcpy(topics[i][j], topic_name);
                                added = 1;
                                break;
                            }
                        }
                        if (added) send(fds[i].fd, "SUB OK\n", 7, 0);
                    } 
                    else if (strcmp(cmd, "UNSUB") == 0) {
                        for (int j = 0; j < MAX_TOPICS_PER_CLIENT; j++) {
                            if (topics[i][j] && strcmp(topics[i][j], topic_name) == 0) {
                                free(topics[i][j]);
                                topics[i][j] = NULL;
                                send(fds[i].fd, "UNSUB OK\n", 9, 0);
                                break;
                            }
                        }
                    }
                    else if (strcmp(cmd, "PUB") == 0) {
                        char *msg = buf + strlen(cmd) + strlen(topic_name) + 2;
                        char out_buf[1024];
                        sprintf(out_buf, "[%s]: %s\n", topic_name, msg);

                        for (int j = 1; j < nfds; j++) {
                            for (int k = 0; k < MAX_TOPICS_PER_CLIENT; k++) {
                                if (topics[j][k] && strcmp(topics[j][k], topic_name) == 0) {
                                    send(fds[j].fd, out_buf, strlen(out_buf), 0);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}