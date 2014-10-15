#include "sysutil.h"
#include <signal.h>
#define ERR_EXIT(m) \
    do { \
        perror(m);\
        exit(EXIT_FAILURE);\
    }while(0)

void do_service(int sockfd);


void sigpipe_handler(int sig)
{
    printf("SIGPIPE\n");
}

int main(int argc, const char *argv[])
{
    int listenfd = tcp_server("192.168.44.136", 8976);

    // if(signal(SIGPIPE, sigpipe_handler) == SIG_ERR)
    //     ERR_EXIT("signal");


    int peerfd = accept(listenfd, NULL, NULL);

    printf("%s connected\n", get_tcp_info(peerfd));

    do_service(peerfd);

    close(peerfd);
    close(listenfd);

    return 0;
}

void do_service(int sockfd)
{
    int cnt = 0;
    char recvbuf[1024] = {0};

    while(1)
    {
        int nread = read(sockfd, recvbuf, sizeof recvbuf);
        sleep(2);
        write(sockfd, recvbuf, nread);
    }
}



