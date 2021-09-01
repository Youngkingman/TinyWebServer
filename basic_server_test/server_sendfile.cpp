#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>
#include<errno.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<sys/uio.h>
#include<sys/sendfile.h>
#define BUFFER_SIZE 1024

static bool stop = false;
static const char* status_line[2] = {"200 OK", "500 Internal Server Error"};

static void handle_term(int sig) {
    stop = true;
}

int main(int argc, char* argv[]) {
    int sock = socket(PF_INET,SOCK_STREAM,0);
    assert(sock >= 0);
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    const char* file_name = argv[3];

    struct sockaddr_in address;
    /*
        struct sockaddr {
            sa_family_t sa_family;
            char sa_data[14];
        }

        struct sockaddr_in {
            sa_family_t sin family; //地址族
            u_int16_t sin_port;     //端口号
            struct in_addr sin_addr;//IPV4结构体
        }

        struct in_addr {
            u_int32_t s_addr;   //IPV4地址
        }
    */
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons( port );

    int ret = bind(sock, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    printf("bind success, socketfd %d \n", sock);

    ret = listen( sock ,5 );
    assert(ret != -1 );
    printf("listen success, socketfd %d \n", sock);

    while(1) {
        struct sockaddr_in client;
        socklen_t client_addrlength = sizeof(client);
        
        int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);

        if (connfd < 0) {
            printf("error is: %d \n", errno);
        } else {
            int filefd = open(file_name, O_RDONLY);
            struct stat stat_buf;
            fstat(filefd, &stat_buf);
            if (filefd < 0) {
                printf("fuck you.\n");
                //错误处理
                continue;
            }
            ssize_t size = sendfile(connfd,filefd,NULL, stat_buf.st_size);
            printf("file send  %d bytes \n", size);
            close(connfd);
        }
        /*缺乏退出机制*/
    }
    close(sock);
    return 0;
}