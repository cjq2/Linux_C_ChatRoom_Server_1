#include "server.h"

//子线程函数
void* PthreadFun(void *arg)
{
    int bytes=0;
    char buf[123]={0};
    int acceptfd=*(int*)arg;
    struct Msg msg;
    
    while(1)
    {
        //进行通信
        //第一步：先接收客户端注册或登陆的信息
        if((bytes=recv(acceptfd,&msg,sizeof(struct Msg),0))==-1)
        {
            ERRLOG("recv");
            break;
        }
        else if(bytes==0)
        {
            printf("客户端：断开了\n");
            break; 
        }
        printf("收到客户端：%s--%d\n",msg.name,msg.cmd);
        switch (msg.cmd)
        {
            case 1:
                UsrReg(acceptfd,&msg);
                break;
            case 2:
                UsrLog(acceptfd,&msg);
                pthread_exit(NULL);  //只有当客户端,退出时，断开了，跳出死循环，该线程应该结束
                return NULL;
        }
    }

     
}

//用户注册  
void UsrReg(int acceptfd,struct Msg *msg)
{
    printf("\n正查找用户是否已注册....\n");
    if(Search_name(msg)==TRUE)  //该用户已注册
    {
        printf("该用户已注册");
        msg->cmd=0;       //注册失败标志位
        
    }
    else                   //该用户还未注册
    {
        if(sql_insert(msg)==TRUE) //添加到数据库
        {
            msg->cmd=1;    //成功添加到数据库标志位--注册成功标志位
        }
    }

    //发送注册成功与否的标志给客户端
    send(acceptfd,msg,sizeof(struct Msg),0);
    return;
}

//用户登录
void UsrLog(int acceptfd,struct Msg *msg)
{
    int flags=0;          //登录成功的标志位  0---失败，1---成功
    if(Search_name(msg)==TRUE)    //已注册
    {
        if(Search_NP(msg)==TRUE)  //登录的信息是否正确
        {
        
           if(Search_online(msg)==TRUE)  //该用户是否在线
            {
                printf("该用户已在线\n");
                msg->cmd=3;             //在线标记为3
            }
            else
            {
                printf("该用户还未登录\n");  
                msg->cmd=0;             //不在线标记为0

                
                printf("正在查看该用户权限.....\n");
                if(UsrIsRoot(msg)==TRUE)
                {
                    printf("该用户为超级用户\n");
                    msg->flag=1;                //用户标志位-1---超级用户
                }
                else
                {
                    printf("该用户为普通用户\n");
                    msg->flag=0;               //  0---普通用户
                }
                flags=1;
                On_line(acceptfd,msg);   //用户上线 
                printf("登录成功\n");
            }
        }
        else
        {
            printf("用户%s-密码输入错误\n",msg->name);
            msg->cmd=1;
        }
    }
    else
    {
        printf("该用户还未注册\n");
        msg->cmd=2;
    }
    
    send(acceptfd,msg,sizeof(struct Msg),0); //发送注册或登录的成功与否的标志反馈给客户端

    if(flags==1)
    {
        if(msg->flag==1)
        {
            RootUsr(acceptfd);
        } 
        if(msg->flag==0)
        {
            CommonUsr(acceptfd);
        }
        
    }
    printf("服务器正在结束该客户端的线程....\n");
    return;
    
}


//查看用户权限
int UsrIsRoot(struct Msg *msg)    
{
    if(strcmp(msg->name,"Admin")==0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//管理员
void RootUsr(int acceptfd)
{
    struct Msg msg;
    printf("等待客户端发消息\n");
    while(1)
    {
       
        int ret=recv(acceptfd,&msg,sizeof(struct Msg),0);//接受客户端的用户选择
        if(ret ==-1)
        {
            ERRLOG("recv");
            break;
        }
        if(ret==0)
        {
            printf("客户端断开了\n");
            printf("客户端下线了\n");
            off_line(acceptfd,&msg);
            break ;
        }
    
        switch(msg.cmd)
        {
            case 1:
                    see_line(acceptfd,&msg);     //查看线上人员
                    break;
            case 2:
                    chat_group(acceptfd,&msg);     //群聊
                    break;
            case 3:
                    chat_private(acceptfd,&msg);   //私聊
                    break;
            case 4:
                    break;
            case 5:
                    break;
            case 6:
                    forbid_speak(acceptfd,&msg);    //禁言
                    break;
            case 7:
                    releve_forbid(acceptfd,&msg);   //解除禁言
                    break;
            case 8:
                    off_line(acceptfd,&msg);         //下线
                    return;
            case 9:
                    kickout_group(acceptfd,&msg);         //踢出聊天室
                    break;
             
        }

    }
}

void CommonUsr(int acceptfd)
{
    struct Msg msg;
    printf("等待客户端发消息\n");
    while(1)
    {
        int ret=recv(acceptfd,&msg,sizeof(struct Msg),0);  //等待客户端发消息
        if(ret==-1)
        {
            ERRLOG("recv");
            break;
        }
        else if(ret ==0)
        {
            printf("客户端断开了\n");
            printf("客户端下线了\n");
            off_line(acceptfd,&msg);
            break;
        }

        switch(msg.cmd)
        {
            case 1:
                    see_line(acceptfd,&msg);
                    break;
            case 2:
                    chat_group(acceptfd,&msg);
                    break;
            case 3:
                    chat_private(acceptfd,&msg);
                    break;
            case 6:
                    send_file(acceptfd,&msg);
                    break;
            case 7:
                    logout(acceptfd,&msg);
                    return;
            case 8:
                    off_line(acceptfd,&msg);
                    return;
            case 9:
                    download_file(acceptfd,&msg);
                    break;            
        }
    }
}

//查看线上人员，将线上人员的一些信息发送给对应客户端
void see_line(int acceptfd,struct Msg *msg)
{
    printf("客户端正在抢锁.....\n");
    pthread_mutex_lock(&mutex);
    int i;
    printf("客户端抢锁成功\n");
    for(i=0;i<count;i++)
    {
        msg->usr[i]=Usr[i];
        printf("在线用户%s\n",Usr[i].name);
    }
    pthread_mutex_unlock(&mutex);
    send(acceptfd,msg,sizeof(struct Msg),0);
}

//群聊
void chat_group(int acceptfd,struct Msg *msg)
{
    int i=0;
    for(i=0;i<N;i++)
    {
        if(Usr[i].socket !=0 && strcmp(msg->fronname,Usr[i].name)==0)
        {
            break;
        }
    }
    //记录一条聊天消息
    insert_record(msg);   //将群聊消息记录起来，存放在数据库

    if(Usr[i].flag==0)  //本用户没有被禁言
    {
        printf("用户%s发送了一条群消息\n",msg->fronname);
        insert_record(msg);
        for(i=0;i<N;i++)
        {
            if(Usr[i].socket !=0 && strcmp(msg->fronname,Usr[i].name) !=0)  //给除了本用户的所有用户发消息
            {
                send(Usr[i].socket,msg,sizeof(struct Msg),0);
            }
        }
    }
    else
    {
        msg->cmd=1003;
        send(acceptfd,msg,sizeof(struct Msg),0);
    }

}

//私聊
void chat_private(int acceptfd,struct Msg *msg)
{
    int i=0,j=0;
    for(i=0;i<N;i++)
    {
        if(Usr[i].socket !=0 && strcmp(msg->fronname,Usr[i].name)==0)  //发消息的用户
        {
            break;
        }
    }
    if(Usr[i].flag==0)        //发消息的用户未被禁言
    {
        printf("用户%s给用户%s发了一条消息\n",msg->fronname,msg->toname);
        if(Search_online(msg)==FALSE)    //不在线
        {
            msg->cmd=1002;
            printf("该用户未登录，消息发送失败\n");
            send(acceptfd,msg,sizeof(struct Msg),0);
        }
        else
        {
            for(j=0;j<N;j++)
            {
                if(Usr[j].socket !=0 && strcmp(msg->toname,Usr[j].name)==0)
                {
                    send(Usr[j].socket,msg,sizeof(struct Msg),0);
                    break;
                }
            }
        }
    }
    else
    {
        msg->cmd=1003;
        send(Usr[i].socket,msg,sizeof(struct Msg),0);
    }    
}

//下线
void off_line(int acceptfd,struct Msg *msg)
{
    pthread_mutex_lock(&mutex);
    int i,j;
    for(i=0;i<count;i++)
    {
        if(strcmp(Usr[i].name,msg->name)==0)
        {
            for(j=i;j<count;j++)
            {
                Usr[j]=Usr[j+1];
            }
            break;
        }
    }
    count=count-1;

    pthread_mutex_unlock(&mutex);
    
}

//用户注销
void logout(int acceptfd,struct Msg *msg)
{
    printf("用户注销中....\n");  
    off_line(acceptfd,msg);   //下线
    delefrom_sql(msg);             //数据库删除用户数据   
}

//禁言
void forbid_speak(int acceptfd,struct Msg *msg)
{
    msg->cmd=1003;          //无法聊天的标志
    printf("用户%s:已被禁言\n",msg->toname);

    pthread_mutex_lock(&mutex);
    int i;
    for(i=0;i<N;i++)
    {
        if(Usr[i].socket !=0 && strcmp(Usr[i].name,msg->toname)==0)
        {
            send(Usr[i].socket,msg,sizeof(struct Msg),0);  //通知该用户，他已被管理员禁言
            Usr[i].flag=1;   //将在线顺序表中的禁言标志位，置1
            break;
        }
    }

    pthread_mutex_unlock(&mutex);
}


//解除禁言
void releve_forbid(int acceptfd,struct Msg *msg)
{
    msg->cmd=1004;
    printf("用户%s:已被解除禁言\n",msg->toname);

    pthread_mutex_lock(&mutex);
    int i;
    for(i=0;i<N;i++)
    {
        if(Usr[i].socket !=0 && strcmp(Usr[i].name,msg->toname)==0)
        {
            send(Usr[i].socket,msg,sizeof(struct Msg),0);
            Usr[i].flag=0;
            break;
        }
    }

    pthread_mutex_unlock(&mutex);
}

//踢出群聊
void kickout_group(int acceptfd,struct Msg *msg)
{
    msg->cmd=1005;
    printf("用户%s:被踢出了聊天室\n",msg->toname);
 
    int i;
    for(i=0;i<N;i++)
    {
        if(Usr[i].socket !=0 && strcmp(Usr[i].name,msg->toname) !=0)   //通知其他在线用户该用户已被踢出群聊
        {
            send(Usr[i].socket,msg,sizeof(struct Msg),0);
        }
    }
    
    
    for(i=0;i<N;i++)
    {
        if(Usr[i].socket !=0 && strcmp(Usr[i].name,msg->toname)==0)
        {
            break;
        }
    }

    msg->cmd=1006;
    send(Usr[i].socket,msg,sizeof(struct Msg),0);  //通知被踢出的用户

    pthread_mutex_lock(&mutex);  //下线
    for(int j=i;j<count;j++)
    {
        Usr[j]=Usr[j+1];
    }
    count--;
    pthread_mutex_unlock(&mutex);

}

//发送文件
void send_file(int acceptfd,struct Msg *msg)
{
    int i;
    for(i=0;i<N;i++)   //服务器接收到某客户端一条上传文件的信息，并转发给其他用户
    {
        if(Usr[i].socket !=0 && strcmp(msg->fronname,Usr[i].name) !=0)
        {
            send(acceptfd,msg,sizeof(struct Msg),0);  //给除了发送文件者的所有人发一个上传了文件的消息 
        }
    }
    
    char buf[65535]={0};
    recv(acceptfd,buf,65535,0);   //服务器接收该客户端的文件传输
    int fd=open(msg->msg,O_WRONLY|O_CREAT|O_APPEND,0777);
    if(fd==-1)
    {
        perror("open");
        printf("\n上传文件失败\n");
    }

    write(fd,buf,65535);
    printf("上传文件成功\n");
    close(fd);
}

//下载文件
void download_file(int acceptfd,struct Msg *msg)
{
    printf("\n\t用户%s正在下载文件%s....\n",msg->name,msg->msg);
    int size =file_size(msg->msg);
    printf("文件大小为%d个字节\n",size);
    msg->filesize=size;
    send(acceptfd,msg,sizeof(struct Msg),0);

    usleep(100000);

    int fd=open(msg->msg,O_RDONLY|O_CREAT,0777);
    if(fd==-1)
    {
        perror("open");
        printf("下载文件失败\n");
        return;
    }

    char buf[65535]={0};
    memset(buf,0,65535);
    
    int ret=read(fd,buf,size);
    printf("读取到%d个字节\n",ret);
    if(ret==-1)
    {
        perror("read");
        printf("\n下载文件失败\n");
        return ;
    }
    int ret1=send(acceptfd,buf,ret,0);

    close(fd);
    sleep(1);
    printf("发送了%d个字节\n",ret1);
    printf("\n文件下载完成\n");
}

//计算文件大小
int file_size(char *s)
{
    int fd=open(s,O_RDONLY,0777);
    if(fd==-1)
    {
        perror("open");
        printf("计算文件大小失败\n");
        return -1;
    }

    int size=lseek(fd,0,SEEK_END);

    close(fd);

    return size;
}

//将用户从数据库中删除
void delefrom_sql(struct Msg *msg)
{
    sqlite3 *ppdb;
    int ret=sqlite3_open("user.db",&ppdb);
    if(ret !=SQLITE_OK)
    {
        printf("sqlite3 open:%s\n",sqlite3_errmsg(ppdb));
        return ;
    }

    char sql[128]={0};
    sprintf(sql,"delete from usr where name ='%s';",msg->name);
    ret=sqlite3_exec(ppdb,sql,NULL,NULL,NULL);
    if(ret !=SQLITE_OK)
    {
        printf("sqlite3 exec:%s\n",sqlite3_errmsg(ppdb));
        printf("注销用户失败\n");
        return ;
    }
     printf("用户%s注销成功\n",msg->name);
    
}

//保存一条群聊的聊天记录
void insert_record(struct Msg *msg)
{
    sqlite3 *ppdb;
    
    int ret=sqlite3_open("allrecord.db",&ppdb);
    if(ret !=SQLITE_OK)
    {
        ERRLOG("sqlite3_open");
        printf("打开或创建一个allrecord数据库失败\n");
        return ;
    }

    char sql[65535]={0};
    sprintf(sql,"create table if not exists record(time text,fronname char,toname char,word char);");
    ret=sqlite3_exec(ppdb,sql,NULL,NULL,NULL);
    if(ret !=SQLITE_OK)
    {
        ERRLOG("sqlite3_exec");
        printf("创建一个表(record)\n");
        return ;
    }

    //获取当前系统时间

    time_t curtime;
    char s[30]={0};
    char YMD[15]={0};  //存储格式化的年月日时间，例如：2020-12-11 
    char HMS[10]={0};  //存储格式化的24小时时间，例如：15:33
    time(&curtime);    //获得从1970年1月1日0时0分0秒到当前时间的秒数
    struct tm *nowtime;

    nowtime=localtime(&curtime);

    strftime(YMD,sizeof(YMD),"%F",nowtime);
    strftime(HMS,sizeof(HMS),"%T",nowtime);

    strncat(s,YMD,11);
    strncat(s,HMS,8);


    memset(sql,0,sizeof(sql));
    sprintf(sql,"insert into record values('%s','%s','%s','%s');",s,msg->fronname,msg->toname,msg->msg);
    ret=sqlite3_exec(ppdb,sql,NULL,NULL,NULL);
    if(ret !=SQLITE_OK)
    {
        ERRLOG("sqlite3_exec");
        printf("添加一条聊天记录失败\n");
        return ;
    }

    sqlite3_close(ppdb);
    return ;
}

//服务器通信初始化
int Socket_init()
{
    int sockfd;
    struct sockaddr_in serveraddr;
    socklen_t addrlen=sizeof(serveraddr);

    //第一步：建立套接字
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        ERRLOG("socket");
        return FALSE;
    }


    //第二步：填充服务器网络信息结构体
    serveraddr.sin_addr.s_addr=inet_addr("192.168.91.78");
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(atoi("8866"));


    //第三步：将网络信息结构体与套接字绑定
    if(bind(sockfd,(struct sockaddr*)&serveraddr,addrlen)==-1)
    {
        ERRLOG("bind");
        return FALSE;
    }

    //第四步：将服务器设置为被动监听状态
    if(listen(sockfd,10)==-1)
    {
        ERRLOG("listen");
        return FALSE;
    }

    return sockfd;

}

//与客户端连接
int ClientLink(int sockfd)
{
    //第五步：将服务器设置为TCP多线程并发服务器
    //主线程负责accept函数阻塞等待客户端发起的连接请求，每当有客户端连接客户端，则创建子线程
    //子线程负责服务器与客户端之间的通信
    //第五步：accept阻塞等待客户端连接
        struct sockaddr_in clientaddr;
        socklen_t addrlen =sizeof(clientaddr);
        int acceptfd;
        if((acceptfd=accept(sockfd,(struct sockaddr*)&clientaddr,&addrlen))==-1)
        {
            ERRLOG("accept");
            return -1;
        }

        printf("客户端：%s-%d连上了\n",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
        return acceptfd;
}

//数据库初始化，添加超级用户
void sql_init()            
{
    sqlite3 *ppdb;

    //打开或创建一个数据库
    int ret=sqlite3_open("user.db",&ppdb);
    if(ret !=SQLITE_OK)
    {
        printf("sqlite3_open:%s\n",sqlite3_errmsg(ppdb));
        return ;
    }

    //创建一个表
    char sql[128]={0};
    sprintf(sql,"create table if not exists usr(name text,passwd text);");
    ret=sqlite3_exec(ppdb,sql,NULL,NULL,NULL);
    if(ret !=SQLITE_OK)
    {
        printf("sqlite3_exec:%s\n",sqlite3_errmsg(ppdb));
        return ;
    }

    //关闭数据库
    sqlite3_close(ppdb);

}

//添加用户数据
int sql_insert(struct Msg *msg)
{
    sqlite3 *ppdb;

    //打开数据库
    int ret =sqlite3_open("user.db",&ppdb);
    if(ret !=SQLITE_OK)
    {
        printf("sqlite3_open:%s\n",sqlite3_errmsg(ppdb));
        return FALSE;
    }
    printf("123\n");
    printf("正在注册%s -%s\n",msg->name,msg->passwd);
    char sql[128]={0};
    //添加数据
    sprintf(sql,"insert into usr values('%s','%s');",msg->name,msg->passwd);
    ret=sqlite3_exec(ppdb,sql,NULL,NULL,NULL);
    if(ret !=SQLITE_OK)
    {
        printf("sqlite3_exec:%s\n",sqlite3_errmsg(ppdb));
        return FALSE;   
    }
    
    //关闭数据
    sqlite3_close(ppdb);

    return TRUE;
}

//数据库查找用户名  有--TRUE ,无---FALSE
int Search_name(struct Msg *msg)
{
    sqlite3 *ppdb;

    //打开或创建一个数据库
    int ret=sqlite3_open("user.db",&ppdb);
    if(ret !=SQLITE_OK)
    {
        printf("sqlite3_open:%s\n",sqlite3_errmsg(ppdb));
        return FALSE;
    }

    //查找数据--用户名是否存在
    char sql[123]={0};
    char **result;
    int row,column;
    sprintf(sql,"select *from usr;");
    ret=sqlite3_get_table(ppdb,sql,&result,&row,&column,NULL);
    if(ret !=SQLITE_OK)
    {
        printf("sqlite3_get_table:%s\n",sqlite3_errmsg(ppdb));
        printf("查找用户失败\n");
        return FALSE;
    }
    int j;
    for(j=column;j<(row+1)*column;j++)  //从字段末尾第一个数据开始
    {
        if(strcmp(result[j],msg->name)==0)
        {
            
            return TRUE;
        }
    }
    return FALSE;
}

//数据库查找密码和用户名
int Search_NP(struct Msg *msg)
{
    sqlite3 *ppdb;

    //打开或创建一个数据库
    int ret=sqlite3_open("user.db",&ppdb);
    if(ret !=SQLITE_OK)
    {
        printf("sqlite3_open:%s\n",sqlite3_errmsg(ppdb));
        return FALSE;
    }

   //查找数据--用户名是否存在
    char sql[123]={0};
    char **result;
    int row,column;
    sprintf(sql,"select *from usr;");
    ret=sqlite3_get_table(ppdb,sql,&result,&row,&column,NULL);
    if(ret !=SQLITE_OK)
    {
        printf("sqlite3_open:%s\n",sqlite3_errmsg(ppdb));
        return FALSE;
    }
    int j,flag1=0;  //数据信息在实际存储时是一个一维数组下标为0-(row+1)*column-1,row是不包含字段一行的
    for(j=column;j<(row+1)*column;j++)    //查找name和passwd一列的数据
    {
        if(strcmp(result[j],msg->name)==0&&
       strcmp(result[j+1],msg->passwd)==0 )
        {
           flag1=1;
            return TRUE;
        }
    }
    if(flag1==0)
    {
        return FALSE;
    }
    
}

//查看用户是否在线
int Search_online(struct Msg *msg)
{
    int i,flag1=0;
    for(i=0;i<N;i++)
    {
        if(Usr[i].socket !=0 && strcmp(Usr[i].name,msg->name)==0)
        {
            flag1=1;
            return TRUE;
        }
    }
    if(flag1==0)
    {
        return FALSE;
    }  
}

//用户上线
void On_line(int acceptfd,struct Msg *msg)
{
    pthread_mutex_lock(&mutex);
    strcpy(Usr[count].name,msg->name);
    Usr[count].socket=acceptfd;
    Usr[count].flag=0;             //用户刚上线为非禁言状态
    count=count+1;
    pthread_mutex_unlock(&mutex);
}

