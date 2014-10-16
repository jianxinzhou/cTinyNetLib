#epoll编程接口（event poll）
1. epoll API为Linux系统专有，在2.6版本中新增。
2. epoll API的核心数据结构称为epoll实例，它和一个打开的文件描述符相关联。                 
   这个文件描述符不是用来做I/O操作的，相反，它是内核数据结构的句柄，      
   这些内核数据结构实现了两个目的：          
   a) 记录了在进程中声明过的感兴趣的文件描述符列表——interest list（兴趣列表）。                        
   b) 维护了处于I/O就绪态的文件描述符列表——ready list（就绪列表）。            
3. ready list中的成员时interest list的子集。            
   对于由epoll检查的每一个文件描述符，我们可以指定一个位掩码来表示我们感兴趣的事件。       
   这些位掩码同poll()所使用的位掩码有着紧密的关联。
4. epoll API由以下3个系统调用组成。
   a) 系统调用epoll_create()创建一个epoll实例，返回代表该实例的文件描述符。            
   b) 系统调用epoll_ctl()操作同epoll实例相关联的兴趣列表。     
      通过epoll_ctl()，我们可以增加新的描述符到列表中，     
      将已有的文件描述符从该列表中移除，   
      以及修改代表文件描述符上事件类型的位掩码。          
   c) 系统调用epoll_wait()返回与epoll实例相关联的就绪列表中的成员。
   
##1. 创建epoll实例：epoll_create()            
1. 系统调用epoll_create()创建了一个新的epoll实例，其对应的兴趣列表初始化为空。
<pre><code>
   #include &lt;sys/epoll.h>      
   int epoll_create(int size);
</code></pre>
2. 参数size指定了我们想要通过epoll实例来检查的文件描述符个数。
   该参数并不是一个上限，而是告诉内核应该如何为内部数据结构划分初始大小。     
3. 作为函数返回值，epoll_create()返回了代表新创建的epoll实例的文件描述符。           
   这个文件描述符在其他几个epoll系统调用中用来表示epoll实例。       
   当这个文件描述符不再需要时，应该通过close()来关闭。
   当所有与epoll实例相关的文件描述符都被关闭时，实例被销毁，
   相关的资源都返还给系统。（多个文件描述符可能引用到想用的epoll实例，              
   这是由于调用fork()或者dup()这样类似的函数所致）。

##2. 修改epoll的兴趣列表：epoll_ctl()   
1. 系统调用epoll_ctl()能够修改由文件描述符epfd所代表的epoll实例中的兴趣列表。 
<pre><code>
   #include &lt;sys/epoll.h>     
   int epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev);  
</code></pre>
2. 参数fd指明了要修改兴趣列表中的哪一个文件描述符的设定。    
   该参数可以是代表管道、FIFO、套接字、POSIX消息队列、inotify实例、终端、设备，   
   甚至是另一个epoll实例的文件描述符（例如，我们可以为受检查的描述符建立起一种层次关系）。  
   但是，这里fd不能作为普通文件或目录的文件描述符（会出现EPERM错误）。    
3. 参数op用来指定需要执行的操作，它可以是：    
   a) EPOLL_CTL_ADD    
      将描述符fd添加到epoll实例中的兴趣列表中。对于fd上我们感兴趣的事件，    
      都指定在ev所指向的结构体中。如果我们试图向兴趣列表中添加一个已存在    
      的文件描述符，epoll_ctl()将出现EEXIST错误。    
   b) EPOLL_CTL_MOD    
      修改描述符fd上设定的事件，需要用到ev所指向的结构体中的信息。如果我们试图修改不在    
      兴趣列表中的文件描述符，epoll_ctl()将出现ENOENT错误。    
   c) EPOLL_CTL_DEL    
      将文件描述符fd从epfd的兴趣列表中移除。该操作忽略参数ev。如果我们试图移除一个不在   
      epfd的兴趣列表中的文件描述符，epoll_ctl()将出现ENOENT错误。关闭一个文件描述符会   
      自动将其从所有的epoll实例中的兴趣列表中移除。   
   d) 参数ev是指向结构体epoll_event的指针，结构体定义如下：    
<pre><code>
      struct epoll_event {                        
          uint32_t     events;    /\* epoll events (bit mask) \*/             
          epoll_data_t data;      /\* User data \*/          
      };
</code></pre>
      结构体epoll_event中的data字段的类型为：  
<pre><code>
      typedef union epoll_data {    
          void      \*ptr;             /\* Pointer to user-defined data \*/    
          int        fd;               /\* File descriptor \*/   
          uint32_t   u32;              /\* 32-bit interger \*/   
          uint64_t   u64;              /\* 64-bit interger \*/
      } epoll_data_t;  
</code></pre>


