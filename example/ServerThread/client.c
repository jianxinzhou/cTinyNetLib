#include "sysutil.h"
#include <sys/epoll.h>
#define ERR_EXIT(m) \
    do { \
        perror(m);\
        exit(EXIT_FAILURE);\
    }while(0)

void do_service(int sockfd);

int main(int argc, const char *argv[])
{
    int peerfd = tcp_client(0);
    connect_host(peerfd, "192.168.153.131", 8976);

    printf("%s connected\n", get_tcp_info(peerfd));

    do_service(peerfd);

    close(peerfd);
    return 0;
}


void do_service(int sockfd)
{
    char sendbuf[1024] = {0};
    char recvbuf[1024] = {0};

    while(1)
    {

        //初始化 sockfd STDIN_FILENO
        int epoll_fd = epoll_create(2);

        struct epoll_event ev;
        ev.events = EPOLLIN; //read事件
        ev.data.fd = sockfd;
        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) == -1)
            ERR_EXIT("epoll_ctl sockfd");

        ev.events = EPOLLIN;
        ev.data.fd = STDIN_FILENO;
        if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1)
            ERR_EXIT("epoll_ctl STDIN_FILENO");

        struct epoll_event events[2];

        //监听
        //int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
       
        int nready = epoll_wait(epoll_fd, events, sizeof events, 5000);
        if(nready == -1)
        {
            if(errno == EINTR)
                continue;
            ERR_EXIT("epoll_wait");
        }
        else if(nready == 0)
        {
            printf("timeout ....\n");
            continue;
        }


        int i = 0;
        for(i = 0; i < 2; ++i)
        {
            int fd = events[i].data.fd;
            if(fd == sockfd && events[i].events & EPOLLIN)
            {
                int nread = recv_msg_with_len(sockfd, recvbuf, sizeof recvbuf);
                if(nread == 0)
                {
                    printf("close ...\n");
                    exit(EXIT_SUCCESS);
                }
                printf("receive msg : %s", recvbuf);
            }

            if(fd == STDIN_FILENO && events[i].events & EPOLLIN)
            {
                fgets(sendbuf, sizeof sendbuf, stdin);
                send_msg_with_len(sockfd, sendbuf, strlen(sendbuf));
            }
        }

        memset(sendbuf, 0, sizeof sendbuf);
        memset(recvbuf, 0, sizeof recvbuf);
    }
    

}

