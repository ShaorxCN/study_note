#include <stdio.h>


// fat32 文件系统短目录检验和计算
int fat32_fileName_checksum()
{

    int i = 0;
    // 示例文件名
    unsigned char string[] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x7e, 0x31, 0x54, 0x58, 0x54}; /* “ABCDEF~1TXT” */
    unsigned char checksum = 0;                                                                  // 8位截取
    for (i = 0; i < 11; i++)
        checksum = ((checksum & 1) ? 0x80 : 0) + (checksum >> 1) + string[i];
    printf("checksum  = %02x\n", checksum);
    return 0;
}