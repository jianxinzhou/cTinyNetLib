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
   > #include &lt;sys/epoll.h>      
   > int epoll_create(int size);
2. 参数size指定了我们想要通过epoll实例来检查的文件描述符个数。
   该参数并不是一个上限，而是告诉内核应该如何为内部数据结构划分初始大小。     
3. 作为函数返回值，epoll_create()返回了代表新创建的epoll实例的文件描述符。           
   这个文件描述符在其他几个epoll系统调用中用来表示epoll实例。       
   当这个文件描述符不再需要时，应该通过close()来关闭。
   当所有与epoll实例相关的文件描述符都被关闭时，实例被销毁，
   相关的资源都返还给系统。（多个文件描述符可能引用到想用的epoll实例，              
   这是由于调用fork()或者dup()这样类似的函数所致）。            


