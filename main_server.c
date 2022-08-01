#include "server.h"




int main(int argc, char const *argv[])
{
    sql_init();                      //数据库初始化
    pthread_mutex_init(&mutex,NULL); //动态初始化一把锁
    int sockfd=Socket_init();        //获得套接字描述符sockfd
    
    while(1)
    {
        int acceptfd=ClientLink(sockfd);
        pthread_t thread;      //创建一个线程用于专门与客户端收发消息
        //创建子线程
        if(pthread_create(&thread,NULL,PthreadFun,&acceptfd) !=0)
        {
            ERRLOG("pthread_create");
            return -1;
        }

        //设置子线程回收资源属性：分离属性
        pthread_detach(thread);             //主进程自动回收线程资源
        
    }
    pthread_mutex_destroy(&mutex);
    close(sockfd);
    
    return 0;
}
