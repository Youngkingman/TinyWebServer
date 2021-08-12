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

#define BUFFER_SIZE 4096 /*读缓冲区大小*/
/*
    主状态机得两种可能状态
*/
enum CHECK_STATE {
    CHECK_STATE_REQUESTLINE = 0, //当前正在分析请求行
    CHECK_STATE_HEADER          //当前正在分析头部字段
};
/*
    从状态机的三种可能结果
*/
enum LINE_STATUS {
    LINE_OK = 0,    //读取到完整的行
    LINE_BAD,       //行出错
    LINE_OPEN       //行数据尚且不完整
};
/*
    服务器HTTP请求的可能结果，
*/
enum HTTP_CODE {
    NO_REQUEST,         //当前请求不完整，需要继续读取客户端数据
    GET_REQUEST,        //获得了完整的客户端数据
    BAD_REQUEST,        //客户端请求有语法错误
    FORBIDDEN_REQUEST,  //客户端对资源没有访问权限
    INTERNAL_ERROR,     //服务器内部错误
    CLOSE_CONNECTIONS   //客户端已经关闭链接
}; 

//可以替换为完整的HTTP报文信息，但是暂时没有必要
static const char* szret[] = {"I get a correct result\n","Something is fucking wrong"};

/*                                      
                                从状态机工作过程

    +-------------+   新的客户数据到达   +--------------+                +---------------+
    |             |=================> |              | 回车换行在       |                |
    |   LINE_OK   |                   |  LINE_OPEN   |===============>|   LINE_BAD      |
    |             |<================= |              | HTTP中单独出现   |                |
    +-------------+ 读取到回车和换行符   +--------------+                 +---------------+
                                     未读到完整行请求自转移
*/

/*从状态机，用于解析单行内容*/
LINE_STATUS parse_line(char* buffer, int& checked_index, int& read_index) {
    char temp; 

    for (;checked_index < read_index;checked_index++) {
        /*获取当前要解析的字节*/
        temp = buffer[checked_index];
        if (temp == '\r') {
            /*
                1.如果这是最后一个字节，那没有读到完整的行，返回LINE_OPEN
                2.如果这是倒数第二个字节且下一个是\n,，返回LINE_OK
                3.如果都不是，那么客户端请求有错,返回LINE_BAD
            */    
           if ((checked_index + 1 ) == read_index) {
               return LINE_OPEN;
            } else if ( buffer[checked_index + 1] == '\n' ) {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if (temp == '\n') { //处理输入checked_index略过了'\r'的情况
            if ( (checked_index > 1) && buffer[checked_index - 1] == '\r' ) {
                buffer[checked_index++] = '\0';
                buffer[checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    /*所有内容分析完仍未遇到'\r',返回LINE_OPEN表示继续读取内容*/
    return LINE_OPEN;
}

/* 分析请求行 */
HTTP_CODE parse_requestline(char* temp, CHECK_STATE& check_state) {
    char* url = strpbrk(temp, " \t");
    /*请求行若无空白字符或者"\t"字符，则请求有问题*/
    if (!url) {
        return BAD_REQUEST;
    }
    *(url++) = '\0';

    char* method =temp;
    if (strcasecmp( method, "GET") == 0){ //仅仅对于GET方法的支持和处理
        printf("The request method is GET\n");
    } else {
        return BAD_REQUEST;
    }

    url += strspn(url, " \t");
    char* version = strpbrk(url," \t");
    if (!version ){
        return BAD_REQUEST;
    }

    *(version++) = '\0';
    version += strspn(version," \t");

    /*仅支持HTTP1.1*/
    if ( strcasecmp(version,"HTTP1.1") != 0) {
        return BAD_REQUEST;
    }
    /*检查URL是否合法*/
    if (strncasecmp(url,"http://", 7) == 0) {
        url += 7;
        url =strchr(url,'/');
    }

    if (!url || url[0] != '/'){
        return BAD_REQUEST;
    }
    printf("the request URL is %s\n",url);
    check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

/* 分析请求头 */
HTTP_CODE parse_headers(char* temp) {
    /*存在空行，是一个正确的HTTP请求*/
    if (temp[0] == '\0') {
        return GET_REQUEST;
    } else if ( strncasecmp(temp,"HOST:",5) == 0 ) { /*处理HOST头部字段*/
        temp += 5;
        temp += strspn(temp," \t");
        printf("the request host is: %s\n",temp);
    } else {
        printf("No handle for %s\n",temp);
    }
    return NO_REQUEST;
}

/* HTTP请求解析的入口函数 */
HTTP_CODE parse_content(char* buffer, int& checked_index, CHECK_STATE& check_state, int& read_index, int& start_line) {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE retcode = NO_REQUEST;
    /*主状态机，从buffer中不断取出所有完整的行*/
    while( (line_status = parse_line(buffer, checked_index,read_index)) == LINE_OK ) {
        char* temp = buffer + start_line;
        start_line = checked_index;
        /*checkstate记录主状态机当前状态*/
        switch (check_state) {
            case CHECK_STATE_REQUESTLINE: { //处理分析请求行状态
                retcode = parse_requestline(temp,check_state);
                if (retcode == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {  //处理分析请求头状态
                retcode = parse_headers(temp);
                if (retcode == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if (retcode == GET_REQUEST) {
                    return GET_REQUEST;
                }
                break;
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
    }
    /*若未能读取到完整的行，则要继续都客户数据进行分析*/
    if (line_status == LINE_OPEN ) {
        return NO_REQUEST;
    } else {
        return BAD_REQUEST;
    }

}
