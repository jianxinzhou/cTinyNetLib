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
          void       \*ptr;            /\* Pointer to user-defined data \*/    
          int        fd;               /\* File descriptor \*/   
          uint32_t   u32;              /\* 32-bit interger \*/   
          uint64_t   u64;              /\* 64-bit interger \*/
      } epoll_data_t;  
</code></pre>    
      参数ev为文件描述符fd所做的设置如下：
      1) 结构体epoll_event中的events字段是一个位掩码，它指定了我们为待检查的描述符   
         fd上所感兴趣的事件集合。    
      2) data字段是一个联合体，当描述符fd稍后称为就绪态时，联合体的成员可用来指定传  
         回给调用进程的信息。   
   e) 示例如下：   
<pre><code>
int epfd;
struct epoll_event ev;
// epoll_create
epfd = epoll_create(5);
if(epfd == -1)
   ERR_EXIT("epoll_create");
// epoll_ctl  
ev.data.fd = fd; //fd为外部传入的文件描述符
ev.events  = EPOLLIN;
if(epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
   ERR_EXIT("epoll_ctl");
</code></pre>

##3. 事件等待：epoll_wait()
1. 系统调用epoll_wait()返回epoll实例中处于就绪态的文件描述符信息。单个epoll_wait()调用   
   能返回多个就绪态文件描述符的信息
<pre><code>
   #include <sys/epoll.h>
   int epoll_wait(int epfd, struct epoll_event *evlist,
                  int maxevents, int timeout);
   Returns number of ready file descriptors, 0 on timeout, or -1 on error
</code></pre>   
2. 参数evlist所指向的结构体数组中返回的是有关就绪态文件描述符的信息。   
   数组evlist的空间由调用者负责申请，所包含的元素个数在参数maxevents中指定。   
3. 在数组evlist中，每个元素返回的都是单个就绪态文件描述符的信息。events字段返回了   
   在该描述符上已经发生的事件掩码。data字段返回的是我们在描述符上使用epoll_ctl()注册感  
   兴趣的事件时在ev.data 中所指定的值。注意，data字段是唯一可获知同这个事件相关的文件   
   描述符的途径。因此，当我们调用epoll_ctl()将文件描述符添加到兴趣列表中时，应该要么   
   将ev.data.fd设为文件描述符，要么将ev.data.ptr设为指向包含文件描述符号的结构体。   
4. 参数timeout用来确定epoll_wait()的阻塞行为，有如下几种：   
   a) 如果timeout等于-1，调用将一直阻塞，直到兴趣列表中的文件描述符上有事件产生，   
      或者直到捕获到一个信号为止。   
   b) 如果timeout等于0，执行一次非阻塞式的检查，看兴趣列表中的文件描述符产生了哪个事件。   
   c) 如果timeout大于0，调用将阻塞至多timeout毫秒，直到文件描述符上有事件发生，   
      或者直到捕获到一个信号为止。    
5. 调用成功后，epoll_wait()返回数组evlist中的元素个数。如果在timeout的时间间隔内没有   
   任何文件描述符处于就绪状态的话，返回0。出错时返回-1，并在errno中设定错误码以表示错误原因。   























