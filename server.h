#ifndef _SERVER_H_
#define _SERVER_H_

#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sqlite3.h>

#include<sys/stat.h>
#include<fcntl.h>

#include<time.h>

#define ERRLOG(errmsg) do{\
                            perror(errmsg);\
                            printf("%s - %s - %d\n",__FILE__,__func__,__LINE__);\
                            exit(1);\
                         }while(0)


#define N        20
#define TRUE     1
#define FALSE   -1
#define PORT     8888



struct USR
{ //在线人员的信息
    char name[N];
    int socket;
    int flag; //用来禁言标志位  0---没有被禁言，1---被禁言
};




struct USR Usr[N]; //结构体数组，用来存放在线用户
int count;
           



//客户端接收的消息
struct Msg
{
    struct USR usr[N];
    char msg[1024];
    char buf[1024];
    char name[N];
    char fronname[N];
    char toname[N];
    char passwd[N];
    int cmd;
    int filesize;
    int flag; //用来判断用户权限 0--代表普通用户， 1---代表超级用户
};

pthread_mutex_t mutex; //互斥锁，用于避免不同线程同时访问全局变量

//数据库初始化，添加超级用户
void sql_init() ;

//添加用户数据
int  sql_insert(struct Msg *msg);

//数据库查找用户名  有--TRUE ,无---FALSE
int Search_name(struct Msg *msg);

//数据库查找密码和用户名
int Search_NP(struct Msg *msg);

//查看用户是否在线
int Search_online(struct Msg *msg);

//套接字初始化
int Socket_init();

//与客户端连接
int ClientLink(int sockfd);

//子线程函数
void* PthreadFun(void *arg);

//用户注册  
void UsrReg(int acceptfd,struct Msg *msg);

//用户登录
void UsrLog(int acceptfd,struct Msg *msg);

//查看用户权限
int UsrIsRoot(struct Msg *msg);

//用户上线
void On_line(int acceptfd,struct Msg *msg);

//管理员
void RootUsr(int acceptfd);

//普通用户
void CommonUsr(int acceptfd);

//查看线上人员，将线上人员的一些信息发送给对应客户端
void see_line(int acceptfd,struct Msg *msg);

//群聊
void chat_group(int acceptfd,struct Msg *msg);

//私聊
void chat_private(int acceptfd,struct Msg *msg);

//保存一条群聊的聊天记录
void insert_record(struct Msg *msg);

//下线
void off_line(int acceptfd,struct Msg *msg);

//用户注销
void logout(int acceptfd,struct Msg *msg);

//解除禁言
void releve_forbid(int acceptfd,struct Msg *msg);

//禁言
void forbid_speak(int acceptfd,struct Msg *msg);

//踢出群聊
void kickout_group(int acceptfd,struct Msg *msg);

//发送文件
void send_file(int acceptfd,struct Msg *msg);

//下载文件
void download_file(int acceptfd,struct Msg *msg);

//计算文件大小
int file_size(char *s);

//将用户从数据库中删除
void delefrom_sql(struct Msg *msg);


#endif
