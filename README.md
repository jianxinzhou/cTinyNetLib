# cTinyNetLib
Welcome to the cTinyNetLib wiki!

## 开发环境
1. ArchLinux
2. gcc version 4.9.1 (GCC)

## 概要说明
1. 使用Linux API进行网络编程时，会有很多重复的操作，使用起来十分繁琐，因此我编写了cTinyNetLib，将诸多操作封装成了函数，使用时直接调用即可。                    
2. cTinyNetLib提供的函数接口请参看头文件sysutil.h                 

## 部分源码分析
###1. 辅助函数readn和writen函数                
  a) ssize_t readn(int fd, void *buf, size_t count)            
     readn函数初衷是保证我们可以从套接字缓冲区中read走count个字节。              
     注意以下情况：                   
      如果在read的过程中遇到EOF（对方关闭套接字），那么read到多少字节就返回多少字节                  
      出错返回-1                    
 
  b) ssize_t writen(int fd, const void *buf, size_t count)                  
     writen函数初衷是保证我们可以向套接字缓冲区中write满count个字节                  
     注意出错返回-1                  
 
  Tip：无论是readn还是writen，出错时均返回-1，把对出错的处理交给调用它们的函数来进行。                    

###2. send_int32和recv_int32               
  a) void send_int32(int sockfd, int32_t val)                                  
     用于告知对方会发送多少字节的数据量               
     send_int32的内部实现是使用writen，向套接字缓冲区write32个字节               
     显然如果在write的过程中发生错误，writen会返回-1，此时send_int32的处理是直接将程序挂掉。           
     注意，虽然发送者的程序已经挂掉，但是套接字缓冲区中可能会有发送者挂掉之前发送的内容，当然也有可能是由于发送者网络突然断线，      但无论如何，套接字缓冲区可能会有部分不完整的发送者所write的内容，因此请看以下recv_int32的处理                      
 
  b) int32_t recv_int32(int sockfd)                       
     用于接收对方将要发送的数据量的字节数                                   
     recv_int32的内部实现是使用readn，从套接字缓冲区中取走内容                       
     显然如果在read的过程中发生错误，readn会返回-1，此时recv_int32的处理是直接将程序挂掉                              
     注意：套接字缓冲区可能会有部分不完整的发送者所write的内容，因此此时readn返回的字节数必将会小于32个字节，此时recv_int32的处      理是返回0，也就是视作对方关闭套接字，而实际情况也正是如此，因为如果对方出错，就直接挂掉程序了，套接字也就关闭了。    
     
###3. send_msg_with_len和recv_msg_with_len    
  a) void send_msg_with_len(int sockfd, const void *usrbuf, size_t count)    
     内部实现首先使用send_int32（包含错误处理），之后在发送count字节的数据量过程中如果出错(writen会返回-1)也直接挂掉程序    
  
  b) size_t recv_msg_with_len(int sockfd, void *usrbuf, size_t bufsize)       
     内部实现首先使用recv_int32，如果recv_int32返回0，那么size_t recv_msg_with_len也返回0，表示对方关闭套接字      
     否则从套接字缓冲区中读取bufszie个字节，如果读不够，返回0，视作对方关闭套接字。    
     
