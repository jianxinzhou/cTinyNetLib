#include "sysutil.h"
#include <signal.h>
#include <poll.h>
#define ERR_EXIT(m) \
    do { \
        perror(m);\
        exit(EXIT_FAILURE);\
    }while(0)

void do_service(int sockfd);

int main(int argc, const char *argv[])
{
    int listenfd = tcp_server("192.168.153.131", 8976);

    //初始化
    struct pollfd fds[2048]; //保存套接字
    int i;
    for(i = 0; i < 2048; ++i)
    {
        fds[i].fd = -1;
    }
    fds[0].fd = listenfd;
    fds[0].events = POLLIN;
    int maxi = 0; //最大数组下标

    //轮询
    int nready; //准备好的套接字数量
    while(1)
    {
        //执行poll
        nready = poll(fds, maxi+1, 5000);
        if(nready == -1)
        {
            if(errno == EINTR)
                continue;
            ERR_EXIT("poll");
        }
        else if(nready == 0)
        {
            printf("poll timeout ..\n");
            continue;
        }

        //处理listenfd
        if(fds[0].revents & POLLIN)
        {
            int peerfd = accept(listenfd, NULL, NULL);
            if(peerfd == -1)
                ERR_EXIT("accept");

            printf("%s connected.\n", get_tcp_info(peerfd));
            //加入到数组中
            int i;
            for(i = 0; i < 2048; ++i)
            {
                if(fds[i].fd == -1)
                {
                    fds[i].fd = peerfd;
                    fds[i].events = POLLIN;
                    if(i > maxi)
                        maxi = i; //更新数组下标
                    break;
                }
            }

            if(i == 2048)
            {
                fprintf(stderr, "too many clients\n");
                exit(EXIT_FAILURE);
            }
        }

        //依次轮询clients
        int i;
        for(i = 1; i <= maxi; ++i)
        {
            int fd = fds[i].fd;
            if(fd == -1)
                continue;
            if(fds[i].revents & POLLIN) //此fd可读
            {
                char recvbuf[1024] = {0};
                size_t nread = recv_msg_with_len(fd, recvbuf, sizeof recvbuf);
                if(nread == 0)  //close
                {
                    fds[i].fd = -1;
                    close(fd);
                    continue;
                }
                printf("receive msg : %d\n", nread);
                send_msg_with_len(fd, recvbuf, nread);
            }
        }

    }

    close(listenfd);

    return 0;
}
