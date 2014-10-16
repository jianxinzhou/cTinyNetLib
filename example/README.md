# 并发
###1.IO复用模型主要实现并发。使得程序不会阻塞在任何一个fd上，而是在poll或者epoll_wait上等待fd可用。   
###2.采用poll或者epoll编写客户端的步骤：      
a)	初始化 加入stdin和sockfd两个fd    
b)	执行poll或者epollwait调用    
c)	根据返回结果，逐个执行处理函数
###3.服务器的并发，第一种最简单的是采用循环。    
<pre><code>
while(1)
{
  int peerfd = accept(listenfd, .....)
}
</code></pre>
这种称为迭代服务器。
###4.多进程编写并发
</code></pre>
while(1)
{
  int peerfd = accept(...)
  fork()
  if(pid == 0)
  {
    close(listenfd);
    ///XXXXX
    exit(EXIT_SUCCESS);
  }
  close(peerfd)
}
</code></pre>
###5.多进程编写并发服务器需要处理僵尸信号，处理信号有：   
a) 直接忽略SIGCHLD，这种适合于子进程消亡不需要额外处理的情况       
b) 为SIGCHLD编写处理函数，内部调用waitpid       
c) 直接使用waitpid，但是在并发服务器中不能采用这种，          
   因为waitpid需要阻塞，这就把并发服务器降低为迭代服务器             
###6.SIGCHLD为处理子进程，提供了一种异步处理能力。
###7.多线程：
</code></pre>
while(1)
{
  int fd = accept()
  创建线程
}
</code></pre>     
注意给线程传递fd，需要使用malloc分配内存空间。
###8.服务器并发的基本思路：
a) 多进程   
b) 多线程   
c) IO复用模型    
###9.使用poll/epoll模型：
a) 初始化数组 fds maxi   
b) 加入listen套接字
</code></pre>
while(1)
{
  poll  
  处理监听套接字，如果可读，accept，将客户加入
  处理客户套接字，如果对方关闭，需要将其从数组中删除
}
</code></pre>
###10.poll编写的注意点：
a) maxi值得是最大数组下标，那么数组长度是maxi+1  
b) 加入client的时候，一定要写入要监听的事件，例如POLLIN  
c) 对方close时，注意删除   
d) 遍历clients时，遍历区间是[1, maxi] 闭区间或者[1, maxi + 1)左闭右开区间   
