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

#define BUFFER_SIZE 1024

static bool stop = false;
static const char* status_line[2] = {"200 OK", "500 Internal Server Error"};
// static const char* IP = "127.0.0.1";
// static const int PORT = 8080;
// static const int BACKLOG = 5;

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
            //HTTP应答行、头部字段和一个空行缓存区
            char header_buf[BUFFER_SIZE];
            memset(header_buf, '\0', BUFFER_SIZE);
            char* file_buf;
            struct stat file_stat;
            bool valid = true;
            int len = 0;

            if (stat(file_name, &file_stat) < 0) {
                valid = false;
            } else {
                if( S_ISDIR(file_stat.st_mode)) { //目标文件非目录
                    valid = false;
                } else if(file_stat.st_mode & S_IROTH) { //具有目标文件读取权限
                    /*--动态分配缓存区，指定大小为目标文件大小，目标文件写入缓存区--*/
                    int fd = open(file_name, O_RDONLY);
                    file_buf = new char[file_stat.st_size + 1];
                    memset(file_buf, '\0', file_stat.st_size + 1);
                    if (read(fd, file_buf, file_stat.st_size)<0) {
                        valid =false;
                    }
                } else {
                    valid =false;
                }
            }
            
            if (valid) {//目标文件有效，进行正常的HTTP应答      
                //HTTP协议的应答状态行、Content-Length以及一个空行
                ret = snprintf(header_buf, BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[0]);
                len += ret;
                
                ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len, "Content-Length: %d\r\n", file_stat.st_size );
                len += ret;

                ret = snprintf(header_buf + len, BUFFER_SIZE - 1 - len,"%s", "\r\n");
                //writev函数将header_buf和file_buf内容一起输出
                struct iovec iv[2];
                iv[0].iov_base = header_buf;
                iv[0].iov_len = strlen(header_buf);
                iv[1].iov_base = file_buf;
                iv[1].iov_len = file_stat.st_size;
                ret = writev(connfd, iv, 2);

                printf("current http request is answered \n");

            } else {//目标文件无效，通知客户端发生了“内部错误”
                ret = snprintf(header_buf,BUFFER_SIZE - 1, "%s %s\r\n", "HTTP/1.1", status_line[1]);
                len += ret; 
                ret = snprintf(header_buf,BUFFER_SIZE - 1 - len, "%s", "\r\n");
                send(connfd, header_buf, strlen(header_buf),0);
            }

            close(connfd);
            delete []file_buf;
            
        }
        /*缺乏退出机制*/
    }
    close(sock);
    return 0;
}
