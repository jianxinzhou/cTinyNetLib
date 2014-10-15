#include "sysutil.h"
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>

#define ERR_EXIT(m) \
    do { \
        perror(m);\
        exit(EXIT_FAILURE);\
    }while(0)

ssize_t recv_peek(int sockfd, void *buf, size_t len);


// 第一次向一个关闭的socket write时，会收到RST置位报文
// 当第二次再write时，即往收到RST报文的socket写入数据，
// 会触发SIGPIPE信号
// 为了防止服务器因为客户端的关闭而挂掉
// 着了设置信号处理函数，当收到SIGPIPE信号时，什么也不做
void handle_sigpipe()
{
    if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        ERR_EXIT("signal");
}

void nano_sleep(double val)
{
    struct timespec tv;
    tv.tv_sec = val; //取整
    tv.tv_nsec = (val - tv.tv_sec) * 1000 * 1000 * 1000;

    int ret;
    // 睡眠可能会被信号中断
    // 遇到这种情况继续睡够剩余的时间
    do
    {
        ret = nanosleep(&tv, &tv);
    }while(ret == -1 && errno == EINTR);
}

// 告诉对方会发送多少字节的数据量
void send_int32(int sockfd, int32_t val)
{
    //先转化为网络字节序
    int32_t tmp = htonl(val);
    if(writen(sockfd, &tmp, sizeof(int32_t)) != sizeof(int32_t)) // 实际上这就一种情况，writen出错返回-1
        ERR_EXIT("send_int32");
}

// 接收对方将要发送的数据量大小
/*
int32_t recv_int32(int sockfd)
{
    int32_t tmp;
    int nread = readn(sockfd, &tmp, sizeof(int32_t));
    if(nread == 0) //EOF 对方close套接字
        return 0;
    else if(nread != sizeof(int32_t))
        ERR_EXIT("recv_int32");
    return ntohl(tmp); //转化为主机字节序
}
*/
int32_t recv_int32(int sockfd)
{
    int32_t tmp;
    int nread = readn(socket, &tmp, sizeof(int32_t));
    if(nread == -1) // ERROR
        ERR_EXIT("recv_int32");
    else if(0< nread < sizeof(int32_t) || nread == 0)
        return 0;  // EOF 与 所读字节数小于32字节，均作为关闭处理

    return ntohs(tmp); //转化为主机字节序
}

// 内核缓冲区(协议栈)中未必有count个字节，因此一次read未必能读走count个字节
// 为什么说好要发count个字节，而内核缓冲区未必有count个字节，
// 因为涉及到滑动窗口等因素，一次向对方发送write个字节，
// 未必就一定能发送count个字节到内核缓冲区中
// 因此需要readn函数不断从内核缓冲区read来保证可以读取count个字节
// 有一种情况需要注意：当遇到EOF（即对方关闭写端时，此时read会返回0），
// 此时可能并未读满count个字节（因为对方没有发满count个字节），
// 那么读了多少字节就返回多少字节
// 因此readn要么出错返回-1，要么读满返回count，要么遇到EOF读到多少返回多少
/**
 * readn - 读取固定字节数
 * @fd:    文件描述符
 * @buf:   接收缓冲区
 * @count: 要读取的字节数
 * 成功返回count，失败返回-1，读到EOF返回小于count
 */
ssize_t readn(int fd, void *buf, size_t count)
{
    size_t nleft = count;  //剩余的字节数
    ssize_t nread; //用作返回值
    char *bufp = (char*)buf; //缓冲区的偏移量

    while(nleft > 0)
    {
        nread = read(fd, bufp, nleft);
        if(nread == -1)
        {
            if(errno == EINTR)
                continue;
            return -1; // ERROR
        }
        else if(nread == 0) //EOF
            break;

        nleft -= nread;
        bufp += nread;
    }

    return (count - nleft);
}

// 需要向对方发送count个字节（注意是，发送到协议栈内核缓冲区，对方read也是从协议栈中饭read）
// 由于网络拥塞等问题（滑动窗口技术），发送方未必一次就可以将count个字节发送到协议栈中，
// 因此需要writen函数，不断向协议栈中write
// 注意一种情况：发送方可能向协议栈中write数次后，在下一次wrire中发生错误，此时直接返回-1，表示出错
// 因此writen要么出错返回-1，要么返回count
// 无论是read和write，被信号中断时，一定是处于阻塞状态
/**
 * writen - 发送固定字节数
 * @fd:     文件描述符
 * @buf:    发送缓冲区
 * @count:  要读取的字节数
 * 成功返回count，失败返回-1
 */
ssize_t writen(int fd, const void *buf, size_t count)
{
    size_t nleft = count;
    ssize_t nwrite;
    const char *bufp = (const char*)buf;
    
    while(nleft > 0)
    {
        nwrite = write(fd, bufp, nleft);
        if(nwrite <= 0) // ERROR
        {
            if(nwrite == -1 && errno == EINTR)
                continue;
            return -1;
        }

        nleft -= nwrite;
        bufp += nwrite;
    }
    
    return count; // 一定时返回count个字节
}

//预览内核缓冲区数据，一次最多预览len个
/**
 * recv_peek - 仅仅查看套接字缓冲区数据，但不移出数据
 * @socket: 套接字
 * @buf:    接收缓冲区
 * @len:    长度
 * 成功返回>=0，失败返回-1
 */ 
ssize_t recv_peek(int sockfd, void *buf, size_t len)
{ 
    int nread;
    do
    {
        nread = recv(sockfd, buf, len, MSG_PEEK); // 这个过程只成功调用一次，peek到多少算多少
    }
    while(nread == -1 && errno == EINTR); // 如果当recv在阻塞的状态下被中断，会继续读取
    
    // 1. 出错返回-1
    // 2. EOF返回0
    // 3. 读到多少返回多少
    return nread;
}


/**
 * readline - 按行读取数据
 * @sockfd:   套接字
 * @buf:      接收缓冲区
 * @maxlen:   每行最大长度
 * 成功返回>=0，失败返回-1
 */
// 出错返回1
// EOF返回0（只要没遇到\n前，就EOF，返回0，因为默认发送方发送的每条消息以'\n'结束。此处可能一开始就是EOF）
// maxlen个字节都没有\n ，返回maxlen-1,最后一位存'\0'
// 当中遇到\n，直接返回\n之前的字节数，包括\n
ssize_t readline(int sockfd, void *usrbuf, size_t maxlen)
{
    //
    size_t nleft = maxlen - 1;
    char *bufp = usrbuf; //缓冲区位置
    size_t total = 0; //读取的字节数

    ssize_t nread;
    while(nleft > 0)
    {
        //预读取
        nread = recv_peek(sockfd, bufp, nleft);
        if(nread <= 0) // 出错或者EOF，直接返回
            return nread;
        
        //检查\n
        //检查到\n，就从套接字缓冲区中取走\n以前的数据(包括\n)
        int i;
        for(i = 0; i < nread; ++i)
        {
            if(bufp[i] == '\n')
            {
                //找到\n
                size_t nsize = i+1;
                if(readn(sockfd, bufp, nsize) != nsize)
                    return -1;
                bufp += nsize;
                total += nsize;
                *bufp = 0; // 最后一位置0
                return total;
            }
        }

        //没找到\n，就把peek到的都取走
        if(readn(sockfd, bufp, nread) != nread)
            return -1;
        bufp += nread;
        total += nread;
        nleft -= nread;
    }
    *bufp = 0;
    return maxlen - 1;
}

// 以下这种方式读取一行的效率是很差的
// 每次从套接字缓冲区的读取一个字符，判断是否是'\n'
// 不推荐这种做法
ssize_t readline_slow(int fd, void *usrbuf, size_t maxlen)
{
    char *bufp = usrbuf;  //记录缓冲区当前位置
    ssize_t nread;
    size_t nleft = maxlen - 1;  //留一个位置给 '\0'
    char c;
    while(nleft > 0)
    {
        if((nread = read(fd, &c, 1)) < 0)
        {
            if(errno == EINTR)
                continue;
            return -1;
        }else if(nread == 0) // EOF
        {
            break;
        }

        //普通字符
        *bufp++ = c;
        nleft--;

        if(c == '\n')
            break;
    }
    *bufp = '\0';
    return (maxlen - nleft - 1);
}

//地址复用
void set_reuseaddr(int sockfd, int optval)
{
    int on = (optval != 0) ? 1 : 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
        ERR_EXIT("setsockopt SO_REUSEADDR");
}

//端口复用
void set_reuseport(int sockfd, int optval)
{
#ifdef SO_REUSEPORT
    int on = (optval != 0) ? 1 : 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0)
        ERR_EXIT("setsockopt SO_REUSEPORT");
#else
    fprintf(stderr, "SO_REUSEPORT is not supported.\n");
#endif //SO_REUSEPORT
}


//设置nagle算法是否可用
void set_tcpnodelay(int sockfd, int optval)
{
    int on = (optval != 0) ? 1 : 0;
    if(setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == -1)
        ERR_EXIT("setsockopt TCP_NODELAY");
}

//是否开启tcpAlive机制
void set_keepalive(int sockfd, int optval)
{
    int on = (optval != 0) ? 1 : 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) == -1)
        ERR_EXIT("setsockopt SO_KEEPALIVE");
}


//获取一个listen fd
int tcp_server(const char *host, uint16_t port)
{

    //处理SIGPIPE信号
    //handle_sigpipe();

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(listenfd == -1)
        ERR_EXIT("socket");

    set_reuseaddr(listenfd, 1);
    set_reuseport(listenfd, 1);
    set_tcpnodelay(listenfd, 0);
    set_keepalive(listenfd, 0);

    SAI addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    //addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);
    if(host == NULL)
    {
        addr.sin_addr.s_addr = INADDR_ANY; // 绑定任意网卡
    }
    else
    {
        //int inet_aton(const char *cp, struct in_addr *inp);
        if(inet_aton(host, &addr.sin_addr) == 0)
        {
            //DNS
            //struct hostent *gethostbyname(const char *name);
            struct hostent *hp = gethostbyname(host);
            if(hp == NULL)
                ERR_EXIT("gethostbyname");
            addr.sin_addr = *(struct in_addr*)hp->h_addr;
        }
    }

    if(bind(listenfd, (SA*)&addr, sizeof addr) == -1)
        ERR_EXIT("bind");

    if(listen(listenfd, SOMAXCONN) == -1)
        ERR_EXIT("listen");

    return listenfd;
}


int tcp_client(uint16_t port)
{
    int peerfd = socket(PF_INET, SOCK_STREAM, 0);
    if(peerfd == -1)
        ERR_EXIT("socket");

    //如果port为0，则不去绑定
    if(port == 0)
        return peerfd;

    SAI addr;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(get_local_ip());
    if(bind(peerfd, (SA*)&addr, sizeof(addr)) == -1)
        ERR_EXIT("bind client");

    return peerfd;
}

void connect_host(int sockfd, const char *des_host, uint16_t des_port)
{
    if(des_host == NULL)
    {
        fprintf(stderr, "des_host can not be NULL\n");
        exit(EXIT_FAILURE);
    }

    SAI servaddr;
    memset(&servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(des_port);
    if(inet_aton(des_host, &servaddr.sin_addr) == 0)
    {
        //DNS
        //struct hostent *gethostbyname(const char *name);
        struct hostent *hp = gethostbyname(des_host);
        if(hp == NULL)
            ERR_EXIT("gethostbyname");
        servaddr.sin_addr = *(struct in_addr*)hp->h_addr;
    }

    if(connect(sockfd, (SA*)&servaddr, sizeof servaddr) == -1)
        ERR_EXIT("connect");
}

//获取点分十进制的ip字符串
const char *get_local_ip()
{
    static char ip[16];

    int sockfd;
    if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        ERR_EXIT("socket");
    }

    struct ifreq req;
    bzero(&req, sizeof(struct ifreq));
    strcpy(req.ifr_name, "eth0");
    if(ioctl(sockfd, SIOCGIFADDR, &req) == -1)
        ERR_EXIT("ioctl");

    struct sockaddr_in *host = (struct sockaddr_in*)&req.ifr_addr;
    strcpy(ip, inet_ntoa(host->sin_addr));
    close(sockfd);
    
    return ip;
}

//getpeername
SAI get_peer_addr(int peerfd)
{
    SAI addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    socklen_t len = sizeof addr;
    if(getpeername(peerfd, (SA*)&addr, &len) == -1)
        ERR_EXIT("getpeername");

    return addr;
}

//getsockname
SAI get_local_addr(int peerfd)
{
    SAI addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    socklen_t len = sizeof addr;
    if(getsockname(peerfd, (SA*)&addr, &len) == -1)
        ERR_EXIT("getpeername");

    return addr;
}


const char *get_addr_ip(const SAI *addr)
{
    return inet_ntoa(addr->sin_addr);
}

uint16_t get_addr_port(const SAI *addr)
{
    return ntohs(addr->sin_port);
}

const char *get_tcp_info(int peerfd)
{
    static char info[100] = {0};

    SAI localaddr = get_local_addr(peerfd);
    SAI peeraddr = get_peer_addr(peerfd);
    snprintf(info, sizeof info, "%s:%d -> %s:%d", 
        get_addr_ip(&localaddr),
        get_addr_port(&localaddr), 
        get_addr_ip(&peeraddr),
        get_addr_port(&peeraddr));

    return info;
}


void send_msg_with_len(int sockfd, const void *usrbuf, size_t count)
{
    send_int32(sockfd, count);                 // 出错情况已在send_int32中处理
    if(writen(sockfd, usrbuf, count) != count) // 其实就一种情况，即writen出错返回-1
        ERR_EXIT("send_msg_with_len");
}


//最后一个参数，指的是buffer的size，而不是用户要求的数目
size_t recv_msg_with_len(int sockfd, void *usrbuf, size_t bufsize)
{
    int32_t len =  recv_int32(sockfd);
    if(len == 0) //对方关闭
        return 0;
    else if(len > (int32_t)bufsize)
    {
        fprintf(stderr, "bufsize is not enough.\n");
        exit(EXIT_FAILURE);
    }

    ssize_t nread = readn(sockfd, usrbuf, len);
    if(nread == -1)
        ERR_EXIT("recv_msg_with_len");
    else if((size_t)nread < len) //字节不够做关闭处理（实际上包含了nread == 0 以及 0<nread<len）
        return 0;

    return len;
}
