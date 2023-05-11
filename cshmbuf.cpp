#include "cshmbuf.h"

CShmBuf::CShmBuf() :
    mOpen(false),
    mShmHead(nullptr),
    mPayload(nullptr),
    mSize(-1)
{

}

//返回头地址
bool CShmBuf::open(int key, int size)
{
    int shmid = 0;

    //1. 查看是否已经存在共享内存，如果有则删除旧的
    shmid = shmget((key_t)key, 0, 0);
    if(shmid == -1) {
        shmid = shmget((key_t)key, 0, 0);
        if (shmid != -1) {
            //	删除已经存在的共享内存
            shmctl(shmid, IPC_RMID, NULL);
        }

        //2. 创建共享内存
        shmid = shmget((key_t)key, sizeof(cShmMsgBuf) + size, 0666 | IPC_CREAT | IPC_EXCL);
        if(shmid == -1) {
            return false;
        }
    }
    //3.连接共享内存
    mShmHead = shmat(shmid, (void*)0, 0);
    if(mShmHead == (void*)-1) {
        return false;
    }

    memset(mShmHead, 0, sizeof(cShmMsgBuf) + size);		//初始化
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    //4. 初始化共享内存信息
    struct cShmMsgBuf *pHead = nullptr;
    pHead = (cShmMsgBuf *)(mShmHead);
    pHead->shmid = shmid;
    // 初始化互斥量
    pthread_mutex_init(&pHead->mMtx, &attr);

    //实际负载起始位置
    mPayload = (char *)(mShmHead + sizeof(struct cShmMsgBuf) + 1);
    mSize = size;
    mOpen = true;

    return true;
}

void CShmBuf::destroy()
{
    cShmMsgBuf *pHead = (cShmMsgBuf *)mShmHead;
    int shmid = -1;
    if(pHead) {
        shmid = pHead->shmid;
        pthread_mutex_destroy(&pHead->mMtx);
    }
    shmdt(mShmHead); //共享内存脱离

    //销毁共享内存
    if(shmctl(shmid, IPC_RMID, 0) == -1) {		//删除共享内存
        printf("Delete shmid=%d \n", shmid);
    }

    mShmHead = NULL;
    mOpen = false;
}

void CShmBuf::Destroy(int key)
{
    int shmid = 0;

    //1. 查看是否已经存在共享内存，如果有则删除旧的
    shmid = shmget((key_t)key, 0, 0);
    if (shmid != -1) {
        printf("Delete shmid=%d \n", shmid);
        shmctl(shmid, IPC_RMID, NULL); 	//	删除已经存在的共享内存
    }
}

void CShmBuf::close(void)
{
    if(mOpen) {
        shmdt(mShmHead); //共享内存脱离
        mShmHead = NULL;
        mPayload = NULL;
        mOpen = false;
    }
}

bool CShmBuf::write(const void *buf, int size, int offest)
{
    cShmMsgBuf *pHead = (cShmMsgBuf *)mShmHead;
    if(!pHead || offest > mSize) {
        return false;
    }

    pthread_mutex_lock(&pHead->mMtx);

    memcpy(mPayload + offest, buf, std::min(mSize - offest, size));

    pthread_mutex_unlock(&pHead->mMtx);

    return true;
}

bool CShmBuf::read(void *buf, int size, int offest)
{
    cShmMsgBuf *pHead = (cShmMsgBuf *)mShmHead;
    if(!pHead || offest > mSize) {
        return false;
    }

    pthread_mutex_lock(&pHead->mMtx);

    memcpy(buf, mPayload + offest, std::min(mSize - offest, size));

    pthread_mutex_unlock(&pHead->mMtx);

    return true;
}
