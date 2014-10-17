#ifndef EPOLL_H_
#define EPOLL_H_

#include <sys/epoll.h>

typedef void (*callback_t) (const char *, size_t cnt, int fd);

typedef struct {
    int epoll_fd_;
    int listenfd_;
    struct epoll_event events_[2048];
    int nready_; //准备好的数目
    callback_t callback_;
} epoll_t;


void epoll_init(epoll_t *et, int listenfd, callback_t callback);
void epoll_loop(epoll_t *et);
void epoll_handle_fd(epoll_t *et);
void epoll_destroy(epoll_t *et);



#endif //EPOLL_H_