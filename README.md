# MIT6.S081-2020-labs
MIT6.S081实验官方纯净源代码，原作者（Calvin Haynes）转载于MIT官方仓库git clone git://g.csail.mit.edu/xv6-labs-2020（未在GitHub上放出），在此Fork过来，感谢原作者Calvin Haynes以及MIT6.S081授课团队。

## 学习目标
- 加深对**操作系统**的理解，从更底层的视角了解并实现操作系统的相关功能
- 加深对**UNIX**的理解，熟悉UNIX的常用指令
- 加深对C/C++的理解
- 在工作简历上作为一段项目经历  

## 学习记录

- 记录自己的学习进度和心得，督促自己不断学习，打好基础，为明年（2023）找工作做好准备，冲冲冲！
- 一个分支对应一个lab，会在尽量在make grade取得满分后上传上来，每份代码都会包含详细的注释，方便自己复习。
### lab1 : Util ###
- 开始日期：2022.12.04
- 完成日期：2022.12.15
- 心得：
  - 环境配置：万事开头难啊，配置实验的运行和调试环境花了我好几天的时间，由于电脑的CPU是M2，网上能够搜到的参考资料十分有限（CSDN真是个垃圾），因此踩了不少的坑，期间一度认为M2无法兼容xv6，差点放弃，好在最后还是比较完美的解决了环境配置问题。不过反过来想一下，这段折磨的经历或许可以帮助我更好地坚持下去。~~在即将放弃的时候能够提醒自己计算一下沉没成本。~~
  - 额外的小知识：我对C++的接触比较少（仅限于刷算法题），学习的也不够系统，更是没有任何C++相关的项目经历，这间接导致了我在环境配置中遇到了很多奇怪的问题，为此我不得不花费额外的时间去学习相关的知识，比如：Makefile，IDE配置和远程调试，Git。这些实用型的小知识不需要太过全面地学习，但是从0到1是必要的，之后的话可以在工作中继续熟能生巧。
  - 学习收获（以后将用知识导图展现）：第一章主要还是在"管中窥豹"，介绍了一些常用的系统调用（pipe，fork，exec，wait等等），以及基于这些系统调用实现的shell功能（IO重定向，管道）。
  - 实验收获（以后将用知识导图展现）：课程中学到的知识其实是有限且抽象的，做实验、阅读源码和参考文档会带来更多的收获，第一个lab并不难，需要注意的是：1.管道通讯中要及时关闭不必要的文件描述符。 2.大多数命令都不接受标准输入作为参数，只能直接在命令行输入参数，这会导致无法用管道命令传递参数。

### lab2 : System Calls ###
- 开始时间：2022.12.20
- 完成时间：2022.12.21
- 进度：
  | index  | lab| state|
    | --- | :--- | :---: |
    | 1  | Trace| √ |
    | 2 | Sysinfo| √ |
- 心得
  - 在lab1中，我在user下通过系统调用完成了几个小程序的编写，而在lab2中，我需要在kernel下完成系统调用的编写。
  - 系统调用都需要在核心态下运行，因此我需要了解：1.用户程序如何完成系统调用。 2.用户态的数据与核心态的数据如何完成交换。
    - 1.系统调用的完成：汇编代码通过使用RISC-V的ecall指令触发中断，将程序从用户态切换到内核态，并将待执行的系统调用函数编码存放在寄存器a7内。ecall会触发中断后，中断代码会将寄存器中的数据保存到当前进程的中断帧trapframe中。进入核心态之后，syscall.c会读取trapframe中保存的寄存器值，取出a7，映射到相应的系统调用函数并运行。
    - 2.数据交换：
      - 用户态 -> 核心态：用户程序会把系统调用所需的参数放在寄存器a0和a1内，然后复制到当前进程的trapframe内，系统调用直接读取trapframe的a0和a1，即可得到用户态的数据。如果用户程序传递的是一个指针，walkaddr会检查用户提供的地址是否为当前进程用户地址空间的一部分，因此程序不能欺骗内核读取其他内存。
      - 核心态 -> 用户态：用户程序会把地址作为参数传给系统调用，该地址将用来接受需要的数据。而系统调用的返回值将只作为程序是否成功运行的标志，返回值将保存在trapframe的a0中。
      - 谨慎地完成数据交换可以很好地保证操作系统的"防御性"以及程序之间的"隔离性"。
  - `任务1 Trace`
    - 描述：添加一个系统调用跟踪功能。
    - syscall中监测到trace函数后，将当前进程的proc结构体中的mask设置为待追踪的系统调用函数编号。
    - 为了跟踪子进程，需要将mask值赋给子进程。
    - 追踪到目标系统调用后，等待该系统调用返回，然后从trapframe->a0中输出他的返回值。
    - 收获：熟悉了系统调用的流程以及用户态与核心态之间数据交流的方式。
  - `任务2 Sysinfo`
    - 描述：添加一个系统调用用来收集系统的运行信息，包括空闲内存量和运行的进程数。
    - sysinfo需要将一个结构体复制回用户空间，这涉及到核心态到用户态的数据交换，需要使用argaddr()把需要返回的数据放入用户程序提供的地址内。
    - 空闲内存使用量：阅读kalloc.c代码可以发现，内存是通过链表进行保存的，所有空闲的内存保存在freelist内，通过遍历整个链表即可获得当前空闲的内存数。
    - 运行的进程数量：阅读proc.c代码可以发现，proc数组保存了所有的进程状态，遍历数组，找到所有状态不为UNUSED的进程并统计数量即可。
    - 收获：任务2使我进一步加深了核心态到用户态的数据传输方式的理解，并且还涉及到了内存管理和进程管理，这部分还没有系统地进行学习，好在题目不难，应该是在抛砖引玉吧。
### lab3 : Page Tables ###
- 开始时间：2022.12.27
- 完成时间：2023.01.01
- 进度：
  | index  | lab| state|
  | --- | :--- | :---: |
  | 1  | Print a page table| √ |
  | 2 | A kernel page table per process| √  |
  | 3  | Simplify copyin/copyinstr | √  |
- 心得
  - 实验目的：
    - 在用户空间，xv6会分别为每个进程提供单独的用户页表；然而在内核中，所有进程都会共享一个唯一的内核页表。每个进程的用户页表和内核页表是不同的，这样可以防止程序在用户空间修改内核数据，有利于保证隔离性。
    - 然而这种隔离策略也会给内核访问用户数据带来困难，内核接收到用户空间的虚拟地址之后，必须调用walk()，通过遍历三级页表获取对应的物理地址，然后将才能把数据传送到用户空间。
    - 使用walk()相比与直接解引用地址，不仅代码实现更复杂，运行速度也会更慢（无法利用硬件内的TLB进行快速映射）。
    - lab3的任务便是实现内核中直接解引用访问用户数据。
  - `任务1 打印页表`
    - 编写vmprint()，vmprint()接受一个页表，并输出页表内各级的PTE，将页表可视化对后续的调试有帮助。这题比较简单，模仿freewalk()对页表进行递归即可获取每级的PTE。需要注意的是PTE的前10位是flags，去掉flags后剩下的44位再在低位加上12位0（地址的格式需要对齐），即可得到下一级的物理地址了。
  - `任务2 各进程内核页表隔离`
    - 分析：要使内核能够直接解引用用户数据，直接让内核使用用户页表显然是没有用的，那么就需要在内核页表内重新建立用户页表上的映射，用户空间的虚拟地址范围为0～PLIC，内核从PLIC开始，只要保证用户空间的虚拟地址不超过PLIC就不会发生重复映射。在重新建立映射之前，首先需要实现各进程的内核页表隔离（xv6内核中所有进程共享唯一内核页表，建立新映射后无法实现隔离性）。
    - 首先需要为每个进程创建新的内核页表，页表跟进程是一一绑定的，因此显然要在proc结构体内新增一个字段用于存储新的专属内核页表。
    - 进程生成时，页表和内核栈也同时生成，需要修改allocproc()实现。
    - 进程切换时，页表也同时切换，需要修改scheduler()实现。scheduler()是进程的调度函数，切换进程之后，需要更新SATP寄存器，SATP寄存器存储着页表起始位置的物理地址。最后还要刷新TLB。
    - 进程释放时，页表和内核栈也同时释放，需要修改freeproc()。只需要释放三级内核页表，不需要释放物理内存，物理内存在用户页表的释放中已经被释放过了。
    - 至此，实现了各进程独享内核页表。
  - `任务3 重新建立映射 简化copyin/copyinstr`
    - 分析：该任务将在内核页表内建立用户空间的映射，在每一个涉及添加/修改/删除用户页表映射的地方都需要进行修改。
    - 首先编写复制映射的函数uvmcopy_u2k()，相比uvmcopy()，uvmcopy_u2k()不需要分配新的物理地址。
    - fork(), exec(), sbrk()三个函数中都涉及到用户映射的改变，因此在这三个函数中都需要使用uvmcopy_u2k()。
    - 第一个进程的创建由userinit()执行，其中并未设计以上三个函数，因此还需要在userinit()里手动进行映射，确保第一个进程的内核页表也能正确访问用户数据。
    - 当虚拟页表内存在用户数据映射时候，便能直接使用解引用访问虚拟地址了，硬件会自动将通过三级页表将虚拟地址转化为物理地址。
### lab4 : Traps ###
- 开始时间：2023.01.05
- 完成时间：2023.01.06
- 进度：
  | index  | lab| state|
  | --- | :--- | :---: |
  | 1  | RISC-V assembly| √ |
  | 2 | Backtrace| √ |
  | 3 | Alarm| √ |
- 心得
Trap过程梳理：
  ![Image text](https://raw.githubusercontent.com/JennyTurtles/MIT6.S081-2020-labs/traps/user/Trap.png)
- `任务1 回溯`
  - 编写backtrace()，backtrace()将获取当前栈的栈顶指针，通过读取栈中内容得到函数的调用过程。
  - 栈从高地址开始向低地址增长，sp（Stack Frame）指向栈结构（Stack Frame）的底部，fp（Frame Pointer）指向栈结构的顶部，这里的栈结构不是栈，可以把它当作栈内存放的元素。
  - 栈结构从上到下依次是：返回地址（RA）,上个栈结构的fp，寄存器，本地变量，前两个项各占8个字节（这两个项各保存一个地址，地址空间为64位）。
  - 因此，已知一个栈结构的fp（指向顶部），fp-8即为栈的返回地址，fp-16即为上一个栈结构的fp，前者即需要输出的数据，后者为调用当前函数的函数栈地址，需要进入该地址进一步递归。
  - 用户栈会占用一个页，所有栈结构的地址都在当前页内，分别使用PGROUNDDOWN和PGROUNDUP即可得到页的下界和上界，即递归的边界条件。
- `任务2 定期警报`
  - 实现一个定期的报警器，每隔一定时钟周期就运行报警函数，打印"alarm！"。
  - 分析：时钟周期的统计需要在内核中进行，而报警函数在用户空间，因此需要修改内核返回用户空间的目的地。从用户空间进入内核后，会将当前运行的位置（返回地址）存放在寄存器SEPC内，内核运行完trap代码后读取SEPC返回用户空间，因此只要将寄存器SEPC的值修改为报警函数，就可以直接在trap结束后进入报警函数。
  - 报警函数在执行的过程中会修改寄存器，这会导致原用户空间的函数执行出错，因此运行完报警函数必须返回内核恢复原寄存器内的值。具体实现：trap内修改SEPC之前把trapframe进行备份，保存在proc里面，直到报警函数运行结束，恢复备份里的trapframe。

### lab5 : Xv6 Lazy Page Allocation ###
- 开始时间：2023.01.09
- 完成时间：2023.01.11
- 进度：
  | index  | lab| state|
  | --- | :--- | :---: |
  | 1  | Eliminate allocation from sbrk()| √ |
  | 2 | Lazy allocation| √ |
  | 3 | Lazytests and Usertests| √ |
- 心得
  - 本次实验需要基于page fault实现lazy allocation，即：应用程序向系统请求堆内存空间时，系统延迟分配堆内存。这样做有两个好处，一是当程序一次性请求大量内存时，能够迅速地完成；二是程序申请分配的空间往往大于实际使用的空间，lazy allocation只在内存被实际用到的时候才进行内存分配，因此可以在一定程度上节约内存空间。
- `任务1 修改内存分配方式`
  - 原内存分配方式：使用sbrk(n)，n为新增/减少的内存字节数，sbrk首先会修改进程的sz（内存大小），然后调用growproc为虚拟内存分配物理空间并进行映射。
  - 要实现lazy allocation首先需要将growproc删除，阻止sbrk直接进行内存分配，正确的内存分配时刻应该为触发缺页异常时。
- `任务2 实现内存懒分配`
  - 由于在sbrk里没有为程序分配物理内存，当程序试图读写内存时，就会触发page fault并进入内核中的usertrap()。
  - 要想在usertrap中处理page fault并为正确的地址分配物理内存，必须要获取出错的虚拟内存地址和触发trap的原因。这两者可以分别从STVAL寄存器和SCAUSE寄存器获取，page fault对应的SCAUSE编号是13和15（读和写）。
  - 当SCAUSE编号为13和15时，进一步判断待分配的虚拟地址是否落在进程的用户栈内，该地址必须小于p->sz且大于p->trapframe->sp（sp指向栈的最底部）。
  - 满足以上条件即可为虚拟内存分配一个page的物理地址，分配完之后使用mappages()建立他们之间的映射。
  - 以上步骤初步完成了内存懒分配。
- `任务3 完善内存懒分配`
  - 内存懒分配会带来一系列问题，以下将一一进行处理。
  - uvmunmap：程序释放内存时默认所有内存都进行了分配（都映射到了物理地址），然而使用了懒分配以后，会有一部分尚未被使用的内存未被映射到物理地址。在释放内存的过程中，程序无法找到它们的页表，因此会触发panic。解决方法是直接删除panic让程序继续运行即可。
  - uvmcopy：fork()会把父进程的内存拷贝到子进程，拷贝的过程中uvmcopy会遍历父进程的页表，这时就会出现上文一样的问题：无法找到内存的映射，触发panic，处理方法也同上文：直接删除panic。
  - walkaddr：read()和write()同样涉及到内存的调用，然而与前面不同的是，它们是系统调用，是在内核态中运行的，内核态中无法触发page fault，因此read()和write()访问内存失败时无法进入usertrap分配新的内存。进一步分析，read()和write()会分别调用copyin()和copyout()，这两者都会使用walkaddr()查询物理地址，因此解决方法是在walkaddr内分配新的内存并建立映射，这与usertrap中的方法是类似的。

### lab6 : Copy-on-Write Fork for xv6 ###
- 开始日期：2023.01.18
- 完成日期：2023.01.19
- 心得
  - copy-on-write (COW) 的作用是当通过fork()创建子进程的时候，不会立刻为子进程分配新的物理内存，而是让子进程的页表指向父进程的物理地址，子进程将于父进程共享物理内存，这些共享的内存会被标记为不可读。因此子进程或父进程试图写入共享的物理内存时，会触发page fault，这时应当将该物理内存进行复制并建立新的映射（设置为可读）。
  - COW使得父子进程的部分内存实现共享，从而节约物理内存。
- `任务 实现COW`
  - uvmpcopy()
    - uvmpcopy会在fork()时为子进程复制父进程的页表和物理内存，为了实现COW首先需要修改uvmpcopy的内存分配方式。
    - 删除物理内存的分配，直接把子进程的新页表映射到父进程的物理内存，并且分别将父子进程的页表的所有项目都设置为PTE_W。
    - 为了方便识别cow内存，需要将第8为PTE定义为PTE_COW，此处父子进程的页表项的PTE_COW都要设置为1。
    - 最后，所有被映射的物理内存的引用计数加一。
  - usertrap()
    - 子进程或父进程访问cow内存时由于没有写权限，会发生page fault。我们需要在usertrap中识别并处理由cow内存导致的page fault。
    - usertrap需要检查scause(13或15)和PTE_COW位是否同时符合要求，满足条件则进行后续操作。(假设子进程试图写入父进程的物理内存)
    - 创建一片新物理内存并将父进程的物理内存复制到里面，取消子进程对父进程物理内存的映射，以免发生remap，为新物理内存建立映射。
    - 最后，该片内存的引用计数减一，该操作将在kfree中完成。
  - 引用计数
    - 父子进程可能会一起共享同一片内存，对于这部分的内存需要使用引用计数，防止物理内存被删除后另一个进程无法访问的问题。
    - 在kalloc.c中声明了一个全局数组，数组长度为（PHYSTOP-KERNBASE）/ PGSIZE，物理地址除以4096即数组的索引，对应的值就是该物理地址的引用计数。
    - kfree()在释放物理内存前需要对引用计数减一，若此时引用计数等于0则释放物理内存，大于0则不释放内存。
    - kalloc()在分配物理内存时需要把引用计数初始化为1。
    - kinit()初始化物理内存时会对每片内存调用kfree，kfree中会将引用计数减一然后检测是否为0，即在进入kfree前只有引用计数为1的内存才会被初始化，因此kinit()需要设置每片内存的引用计数为1。
  - 锁
    - 所有涉及到修改引用计数的地方都需要加锁，防止进程切换导致引用计数的更新错误。
    
## 环境说明
- mac os 13.1
- Clion 2021
- Apple M2
