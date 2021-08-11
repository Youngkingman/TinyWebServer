#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<netdb.h>
#include<assert.h>
#include<stdio.h>

int main(int argc, char* argv[]) {
    assert(argc == 2);
    char *host = argv[1];

    struct hostent* hostinfo = gethostbyname(host);
    /*
        struct hostent {
            char* h_name;       //主机名
            char *h_aliases;    //主机别名列表
            int h_addrtype;     //地址类型（地址族）
            int h_length;       //地址长度
            char** h_addr_list;//按网络字节序列出的主机IP列表
        }
    */
   
   assert(hostinfo);

   struct servent* servinfo = getservbyname("daytime","tcp");
    /*
        struct servent {
            char* s_name; //服务名称
            char** s_aliases //服务别名的列表（字符串数组）
            int s_port //端口号
            char *s_proto; //服务类型
        }
    */
   assert(servinfo);
   printf("daytime port is %d \n", ntohs(servinfo->s_port));

   struct sockaddr_in address;
   address.sin_family = AF_INET;
   address.sin_port = servinfo->s_port;
   address.sin_addr = *(struct in_addr* )*hostinfo->h_addr_list;

   int sockfd = socket(AF_INET, SOCK_STREAM, 0);
   int result = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
   assert(result != -1);
   char buffer[128];
   result = read(sockfd, buffer, sizeof(buffer));
   assert(result > 0);
   buffer[result] = '\0';
   printf("the day tiem is : %s",buffer);
   
   close(sockfd);
   return 0;
}