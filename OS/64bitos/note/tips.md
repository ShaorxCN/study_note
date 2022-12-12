# 途中遇到的各类和正文不相关的基础问题

- [linux](#linux)
- [bochs](#bochs)



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


