#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)))
    {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 5))
    {
        perror("listen() failed");
        close(listener);
        return 1;
    }

    printf("Server is listening on port 8080...\n");

    int clients[64];
    int nclients = 0;

    int isHotenReceived[64] = {0};
    int isMSSVReceived[64] = {0};
    int isGreeted[64] = {0};

    char buf1[64][256];
    char buf2[64][256];

    while (1)
    {
        // accept
        int client = accept(listener, NULL, NULL);
        if (client != -1)
        {
            printf("New client: %d\n", client);
            clients[nclients] = client;
            isHotenReceived[nclients] = 0;
            isMSSVReceived[nclients] = 0;
            isGreeted[nclients] = 0;

            ioctl(client, FIONBIO, &ul);
            nclients++;
        }

        // xử lý client
        for (int i = 0; i < nclients; i++)
        {
            // gửi greeting 1 lần
            if (!isGreeted[i])
            {
                send(clients[i], "Hello!\nHo ten: ", 17, 0);
                isGreeted[i] = 1;
                continue;
            }

            int len;

            // nhận họ tên
            if (!isHotenReceived[i])
            {
                len = recv(clients[i], buf1[i], sizeof(buf1[i]) - 1, 0);
                if (len > 0)
                {
                    buf1[i][len] = 0;
                    buf1[i][strcspn(buf1[i], "\r\n")] = 0; // xóa newline

                    isHotenReceived[i] = 1;
                    printf("Hoten %d: %s\n", clients[i], buf1[i]);

                    send(clients[i], "MSSV: ", 6, 0);
                }
                continue;
            }

            // nhận MSSV
            if (!isMSSVReceived[i])
            {
                len = recv(clients[i], buf2[i], sizeof(buf2[i]) - 1, 0);
                if (len > 0)
                {
                    buf2[i][len] = 0;
                    buf2[i][strcspn(buf2[i], "\r\n")] = 0; // xóa newline

                    isMSSVReceived[i] = 1;
                    printf("MSSV %d: %s\n", clients[i], buf2[i]);
                }
                continue;
            }

            // tạo email
            if (isHotenReceived[i] && isMSSVReceived[i])
            {
                char *word[20];
                int wordCount = 0;

                char temp[256];
                strcpy(temp, buf1[i]);

                char *token = strtok(temp, " ");
                while (token != NULL)
                {
                    word[wordCount++] = token;
                    token = strtok(NULL, " ");
                }

                char response[512] = {0};

                // lastname
                strcat(response, word[wordCount - 1]);
                strcat(response, ".");

                // chữ cái đầu (T + D)
                for (int j = 0; j < wordCount - 1; j++)
                {
                    char tmp[2];
                    tmp[0] = word[j][0];
                    tmp[1] = '\0';
                    strcat(response, tmp);
                }

                // bỏ 2 số đầu MSSV
                if (strlen(buf2[i]) > 2)
                    strcat(response, buf2[i] + 2);
                else
                    strcat(response, buf2[i]);

                strcat(response, "@sis.hust.edu.vn\n");

                send(clients[i], response, strlen(response), 0);

                printf("Email: %s", response);

                // reset
                isHotenReceived[i] = 0;
                isMSSVReceived[i] = 0;
                isGreeted[i] = 0; //Cho phep client gui lai hoten va MSSV de tao email moi
            }
        }

        usleep(10000);
    }

    close(listener);
    return 0;
}