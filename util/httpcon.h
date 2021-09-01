#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "util/locker.h"

class http_conn {
public:
    static const int FILENAME_LEN = 200;//文件名最大长度
    static const int READ_BUFFER_SIZE = 2048;//读缓冲区大小
    static const int WRITE_BUFFER_SIZE = 1024;//写缓冲区大小
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH };//支持的HTTP请求方法类型（暂时只支持GET）
    enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };//主状态机的状态
    enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, 
                        NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, 
                        INTERNAL_ERROR, CLOSED_CONNECTION };//请求可能的返回结果
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };//行的读取状态

public:
    http_conn(){}
    ~http_conn(){}

public:
    void init( int sockfd, const sockaddr_in& addr );//初始化连接
    void close_conn( bool real_close = true );//关闭链接
    void process();//处理HTTP请求
    bool read();//非阻塞读操作
    bool write();//非阻塞写操作

private:
    void init();//初始化连接
    HTTP_CODE process_read();//HTTP请求读取
    bool process_write( HTTP_CODE ret );//;HTTP应答填充

    /*该组函数被process_read调用用于请求读取*/
    HTTP_CODE parse_request_line( char* text );//解析HTTP请求行
    HTTP_CODE parse_headers( char* text );//解析HTTP请求头部
    HTTP_CODE parse_content( char* text );//解析HTTP请求内容
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();//行解析

    /*该组函数被process_write调用用于填充HTTP应答*/
    void unmap();
    bool add_response( const char* format, ... );
    bool add_content( const char* content );
    bool add_status_line( int status, const char* title );
    bool add_headers( int content_length );
    bool add_content_length( int content_length );
    bool add_linger();
    bool add_blank_line();

public:
    static int m_epollfd;//所有socket事件都被注册到同一个epoll之中，因此设置为static
    static int m_user_count;//当前用户数量，被所有HTTP连接共享，设为static

private:
    int m_sockfd; //当前连接的sock文件描述符
    sockaddr_in m_address;//客户端socket地址

    char m_read_buf[ READ_BUFFER_SIZE ];//读缓冲区
    int m_read_idx;//已读最后一个字节的下一位
    int m_checked_idx;//正在分析的字符在读缓冲区的位置
    int m_start_line;//解析行的起始位置
    char m_write_buf[ WRITE_BUFFER_SIZE ];//写缓冲区
    int m_write_idx;//写缓冲区待发送的字节数

    CHECK_STATE m_check_state;//当前主状态机状态
    METHOD m_method;//本次请求的方法

    char m_real_file[ FILENAME_LEN ];//客户请求的文件的完整路径 网站根目录+m_url
    char* m_url;//目标文件文件名
    char* m_version;//HTTP版本号,仅仅支持1.1
    char* m_host;//主机名
    int m_content_length;//HTTP请求消息体长度
    bool m_linger;//HTTP连接是否要保持

    char* m_file_address;//请求文件的起始位置   
    struct stat m_file_stat;//请求文件的状态
    //writev所需的结构
    struct iovec m_iv[2];
    int m_iv_count;//被写内存块数量
};

#endif