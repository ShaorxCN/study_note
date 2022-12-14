# 途中遇到的各类和正文不相关的基础问题

- [linux](#linux)
- [bochs](#bochs)
- [汇编](#asm)



<div id ="linux"><h2>linux</h2</div>

### 分区 挂载硬盘

```shell

#查看 
sudo fdisk -l
lsblk -f

# 分区
sudo fdisk /dev/xxx

#然后根据提示操作  比如 d n等 记得 最后w才是写入生效 不w就可以重新来


# 格式化 ntfs ext4等等

mkfs -t ntfs xxx
mkfs.ntfs xxx
```

<div id="bochs"><h2>bochs</h2></div>

### make错误

参见[这篇文章](https://www.cnblogs.com/oasisyang/p/15358137.html)


<div id ="asm"><h2>汇编</h2</div>

### 寄存器补充知识

rax eax ax al/ah 其实应该算是一个寄存器。为了兼容以及拓展
rax代表64位寄存器。eax代表32，ax 16  ah，al则是ax的高低8位