#include "epoll_t.h"
#include "sysutil.h"


void foo(const char *buf, size_t cnt, int peerfd)
{
    printf("receive msg : %d\n", cnt);
    send_msg_with_len(peerfd, buf, cnt);
}


int main(int argc, char const *argv[])
{
    int listenfd = tcp_server("192.168.153.131", 8976);

    epoll_t et;
    epoll_init(&et, listenfd, &foo);

    while(1)
    {
        epoll_loop(&et);
        epoll_handle_fd(&et);
    }

    epoll_destroy(&et);

    close(listenfd);

    return 0;
}
