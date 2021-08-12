#include "../util/http_parser.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/uio.h>

static bool stop = false;
static void handle_term(int sig) {
    stop = true;
}

int main(int argc, char *argv[]) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *file_name = argv[3];

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(ret != -1);
    printf("bind success, socketfd %d \n", sock);

    ret = listen(sock, 5);
    assert(ret != -1);
    printf("listen success, socketfd %d \n", sock);

    //等待Ctrl+C信号终止循环
    signal(SIGINT, handle_term);
    while (!stop) {
        struct sockaddr_in client;
        socklen_t client_addrlength = sizeof(client);

        int connfd = accept(sock, (struct sockaddr *)&client, &client_addrlength);

        if (connfd < 0) {
            printf("error is: %d \n", errno);
        }
        else {
            char buffer[BUFFER_SIZE];
            memset(buffer, '\0', BUFFER_SIZE);
            int data_read = 0;
            int read_index = 0;
            int checked_index = 0;
            int start_line = 0;
            /* 设置主机的初始状态 */
            CHECK_STATE check_state = CHECK_STATE_REQUESTLINE;
            while (1) { /*循环读取用户数据并进行分析*/
                data_read = recv(connfd, buffer + read_index, BUFFER_SIZE - read_index, 0);
                if (data_read == -1) {
                    printf("reading failed\n");
                    break;
                }
                else if (data_read == 0) {
                    printf("remote client has closed the connection\n");
                    break;
                }
                read_index += data_read;
                /*分析当前所有的获得用户数据*/
                HTTP_CODE result = parse_content(buffer, checked_index, check_state, read_index, start_line);
                
                if (result == NO_REQUEST) { //仍未获得完整的HTTP请求
                    continue;
                } else if(result == GET_REQUEST) {  //得到完整而正确的HTTP请求(此处可进行自定义处理)
                    send(connfd,szret[0],strlen(szret[0]),0);
                    break;
                } else {
                   send(connfd,szret[1],strlen(szret[1]),0);
                   break;
                }
            }
        }
        close(connfd);
    }
    close(sock);
    return 0;
}
