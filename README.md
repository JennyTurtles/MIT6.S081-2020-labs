# lab9：Large Files
### 开始日期：2023.03.04
### 完成日期：2023.03.05

- ### `心得`
- ### `任务1 大文件读写` 
  - 实验描述
    - xv6中一个inode包含12个“直接”块号和一个“间接”块号，“一级间接”块指一个最多可容纳256个块号的块，1个inode总共可以访问12+256=268个块。一个块占1024字节，因此xv6中一个inode可以访问268KB。
    - 该实验中要将一个“直接”块转换为“二级间接”块，“二级间接”块中保存256个“一级间接”块的地址，总共可以包含256*256个“直接”块。
    - 改进之后inode可以存储256*256+256+11个块，即65803KB。
  - 实验思路
    - 修改定义：NDIRECT改为11，原来的第12块作为“一级间接”块，第13块作为“二级间接”块。
    - bmap()：首先计算出bn在二级节点和一级节点中的位置，前者为bn/NINDIRECT，后者bn%NINDIRECT为。然后依次经过二级节点，一级节点，找到直接块，如果途径的节点为空的话，就使用balloc()分配一个，同时写入log。此外需要注意的是，通过bread()获取的buffer块是带锁的，必须在使用完成以后对其调用brelse()，否则buffer块很快就会被耗尽。
    - itrunc()：该函数用于释放inode及其包含的所有数据。对于二级节点，我们需要向下进行遍历，找到所有非空的直接块并清空，然后再清空对应的一级块，最后再清空二级块。
- ### `任务2 符号链接` 
  - 实验描述
    - 实现xv6中的符号链接，符号链接（或软链接）是指按路径名链接的文件；当一个符号链接打开时，内核跟随该链接指向引用的文件。
  - 实验思路
    - 修改定义：在stat.h中添加T_SYMLINK用于标识文件类型为符号链接。在fcntl.h添加O_NOFOLLOW用于标识打开文件的模式是否直接打开符号链接还是割跟随符号连接。
    - sys_symlink()：创建一个符号链接涉及到磁盘的写入操作，这里首先要调用begin_op()开启事物。使用create()为符号链接创建一个inode，使用writei()将符号链接存放在inode里面。
    - sys_open()：添加对符号链接的处理。如果inode的type是T_SYMLINK且标识符没有O_NOFOLLOW，则跟随符号链接。使用readi()读出inode中的path，使用namei(path)，获取path对应的inode，如果该inode不为符号链接则结束循环返回文件标识符，否则继续跟随符号链接。
- ### `完成！`
![Image text](https://raw.githubusercontent.com/JennyTurtles/MIT6.S081-2020-labs/fs/user/lab9%20完成.png)
