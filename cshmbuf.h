#ifndef CSHMBUF_H
#define CSHMBUF_H

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

struct cShmMsgBuf
{
    pthread_mutex_t     mMtx;
    int                 shmid;
};

class CShmBuf
{
public:
    CShmBuf();

public:
    static void Destroy(int key); //静态删除共享内存方法

public:
    //创建和销毁
    void destroy(void);

    bool open(int key, int size);
    void close(void);

    //读取和存储
    bool write(const void *buf, int size, int offest);
    bool read(void *buf, int size, int offest);

private:
    //进程控制信息块
    bool        mOpen;
    void        *mShmHead;		// 共享内存头部指针
    char        *mPayload;		// 有效负载的起始地址
    int         mSize;
};

#endif // CSHMBUF_H
