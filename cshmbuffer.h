#ifndef CSHMBUFFER_H
#define CSHMBUFFER_H

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

typedef struct cShmBuffer_s
{
    int          shmid;			// 共享内存ID
    unsigned int size;          // 块大小
    unsigned int rd_index;		// 读索引
    unsigned int wr_index;		// 写索引

}cShmBuffer_t;

class CShmBuffer
{
public:
    CShmBuffer();

public:
    //静态删除共享内存方法
    static void Destroy(int key);

public:
    //创建和销毁
    void destroy(void);

    // 打开和关闭
    bool open(int key, int size);
    void close(void);

    //读取和存储
    void write(const void *buf, int size);
    void read(void *buf, int size);

private:
    bool init(int key, int size);
    // 利用文件node id做为共享内存key，保证key唯一性
    bool getKey(int keyFile);
    // 加锁共享内存对应的key文件，防止进程重复初始化共享内存
    bool lockFile(int fd, int lockPos = 0);
    bool unLockFile(int fd, int lockPos = 0);

private:
    int     fd;
    //进程控制信息块
    bool    m_open;
    void    *m_shmhead;    // 共享内存头部指针
    char    *m_payload;    // 有效负载的起始地址

    char    shmKeyFile[256];
};

#endif // CSHMBUFFER_H
