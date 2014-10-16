#ifndef SYSUTIL_H_
#define SYSUTIL_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <time.h> 
extern int h_errno;

typedef struct sockaddr SA;
typedef struct sockaddr_in SAI;

void nano_sleep(double val);

ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);
ssize_t readline(int fd, void *usrbuf, size_t maxlen);

void send_int32(int sockfd, int32_t val);
int32_t recv_int32(int sockfd);


void set_reuseaddr(int sockfd, int optval);
void set_reuseport(int sockfd, int optval);
void set_tcpnodelay(int sockfd, int optval);
void set_keepalive(int sockfd, int optval);


int tcp_server(const char *host, uint16_t port);
int tcp_client(uint16_t port);
void connect_host(int sockfd, const char *des_host, uint16_t des_port);

const char *get_local_ip();

SAI get_peer_addr(int peerfd);
SAI get_local_addr(int peerfd);
const char *get_addr_ip(const SAI *addr);
uint16_t get_addr_port(const SAI *addr);
const char *get_tcp_info(int peerfd);

void send_msg_with_len(int sockfd, const void *usrbuf, size_t count);
size_t recv_msg_with_len(int sockfd, void *usrbuf, size_t bufsize);


#endif //SYSUTIL_H_
