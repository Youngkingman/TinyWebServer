#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#include<assert.h>
#include<stdio.h>
#include<string.h>

// static const char* IP = "127.0.0.1";
// static const int PORT = 8080;
// static const int BACKLOG = 5;

int main(int argc, char* argv[]) {
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&server_address);
    server_address.sin_port = htons(port);

    int sockfd = socket(PF_INET,SOCK_STREAM,0);
    printf("sockfd %d \n",sockfd);

    int ret = connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address));

    if (ret < 0) {
                printf("connection failed with code %d \n", ret);
    } else {
        const char* oob_data = "fuck";
        const char* normal_data = "you312";
        send(sockfd,normal_data,strlen(normal_data),0);
        send(sockfd,oob_data,strlen(oob_data),MSG_OOB);
        send(sockfd,normal_data,strlen(normal_data),0);
    }

    close(sockfd);
    return 0;
}