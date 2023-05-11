#include "cshmbuffer.h"

#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <string.h>

#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>

void lock_init(flock *lock, short type, short whence, off_t start, off_t len)
{
    if (lock == NULL)
        return;

    lock->l_type = type;
    lock->l_whence = whence;
    lock->l_start = start;
    lock->l_len = len;
}

int readw_lock(int fd)
{
    if (fd < 0) {
        return -1;
    }

    struct flock lock;
    lock_init(&lock, F_RDLCK, SEEK_SET, 0, 0);
    if (fcntl(fd, F_SETLKW, &lock) != 0) {
        return -1;
    }

    return 0;
}

int writew_lock(int fd)
{
    if (fd < 0) {
        return -1;
    }

    struct flock lock;
    lock_init(&lock, F_WRLCK, SEEK_SET, 0, 0);
    if (fcntl(fd, F_SETLKW, &lock) != 0) {
        return -1;
    }

    return 0;
}

int unlock(int fd)
{
    if (fd < 0) {
        return -1;
    }

    struct flock lock;
    lock_init(&lock, F_UNLCK, SEEK_SET, 0, 0);
    if (fcntl(fd, F_SETLKW, &lock) != 0) {
        return -1;
    }

    return 0;
}


CShmBuffer::CShmBuffer()
{

}

//返回头地址
bool CShmBuffer::init(int key, int size)
{
    int shmid = 0;
    //1. 查看是否已经存在共享内存，如果有则删除旧的
    shmid = shmget((key_t)key, 0, 0);
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL); 	//	删除已经存在的共享内存
    }

    //2. 创建共享内存
    shmid = shmget((key_t)key, sizeof(cShmBuffer_t) + size, 0666 | IPC_CREAT | IPC_EXCL);
    if(shmid == -1) {
        return false;
    }

    //3.连接共享内存
    m_shmhead = shmat(shmid, (void*)0, 0);					//连接共享内存
    if(m_shmhead == (void*)-1) {
        return false;
    }
    memset(m_shmhead, 0, sizeof(cShmBuffer_t) + size);		//初始化

    //4. 初始化共享内存信息
    cShmBuffer_t * pHead = (cShmBuffer_t *)(m_shmhead);
    pHead->shmid	= shmid;				//共享内存shmid
    pHead->size     = size;              //共享信息写入
    pHead->rd_index = 0;					//一开始位置都是第一块
    pHead->wr_index = 0;					//

    //5. 填充控制共享内存的信息
    m_payload = (char *)(pHead + 1);        //实际负载起始位置
    m_open = true;

    return true;
}

void CShmBuffer::destroy()
{
    cShmBuffer_t *pHead = (cShmBuffer_t *)m_shmhead;
    int shmid = pHead->shmid;

    //删除信号量
    shmdt(m_shmhead); //共享内存脱离

    //销毁共享内存
    if(shmctl(shmid, IPC_RMID, 0) == -1) {		//删除共享内存
        printf("Delete shmid=%d \n", shmid);
    }

    m_shmhead = NULL;
    m_payload = NULL;
    m_open = false;
}

void CShmBuffer::Destroy(int key)
{
    int shmid = 0;

    //1. 查看是否已经存在共享内存，如果有则删除旧的
    shmid = shmget((key_t)key, 0, 0);
    if (shmid != -1) {
        printf("Delete shmid=%d \n", shmid);
        shmctl(shmid, IPC_RMID, NULL); 	//	删除已经存在的共享内存
    }
}

//返回头地址
bool CShmBuffer::open(int key, int size)
{
    int shmid;

    if(getKey(key) == false) {
        return false;
    }

    fd = ::open(shmKeyFile, O_RDWR);  // 进程运行期间不关闭文件，进程退出自动关闭
    if (fd < 0) {
        printf("open key file = %s to do lock shm error = %d, info = %s",
               shmKeyFile, errno, strerror(errno));
        return false;
    }

    //1. 查看是否已经存在共享内存，如果有则删除旧的
    shmid = shmget((key_t)key, 0, 0);
    if (shmid == -1) {
        return this->init(key, size);
    }

    //2.连接共享内存
    m_shmhead = shmat(shmid, (void*)0, 0);					//连接共享内存
    if(m_shmhead == (void*)-1) {
        return false;
    }

    //3. 填充控制共享内存的信息
    m_payload = (char *)((cShmBuffer_t *)m_shmhead + 1);	//实际负载起始位置
    m_open = true;

    return true;
}

void CShmBuffer::close(void)
{
    if(m_open) {
        shmdt(m_shmhead); //共享内存脱离
        m_shmhead = NULL;
        m_payload = NULL;
        m_open = false;
    }
}

void CShmBuffer::write(const void *buf, int size)
{
    cShmBuffer_t *pHead = (cShmBuffer_t *)m_shmhead;
    writew_lock(fd);
    unlock(fd);
//    memcpy(m_payload + (pHead->wr_index) * (pHead-> size), buf, pHead-> blksize);
//    pHead->wr_index = ((pHead->wr_index) % (pHead->blocks))+1;	//写位置偏移
}

void CShmBuffer::read(void *buf, int size)
{
    cShmBuffer_t *pHead = (cShmBuffer_t *)m_shmhead;

    readw_lock(fd);
    unlock(fd);
//    lockFile(fd);
//    unLockFile(fd);
//    memcpy(buf, m_payload + (pHead->rd_index) * (pHead-> blksize), pHead-> blksize);
//    //读位置偏移
//    pHead->rd_index = ((pHead->rd_index) % (pHead->blocks))+1;
}

// 根据文件路径取共享内存key值
// 直接使用文件节点号作为共享内存的key值
bool CShmBuffer::getKey(int keyFile)
{
    // 从key文件节点数据获取共享内存key值

    int fLen = snprintf(shmKeyFile, 256 - 1, "/tmp/shm_key_path/.file_%d_.shm", keyFile);
    shmKeyFile[fLen] = '\0';

    struct stat fInfo;
    int key = 0;
    if (stat(shmKeyFile, &fInfo) != 0)	{
        if (ENOENT == errno) { // 不存在则创建
            char cmd[1024] = {0};
            snprintf(cmd, sizeof(cmd), "mkdir -p %s", shmKeyFile);
            *strrchr(cmd, '/') = 0;
            system(cmd);   // 创建目录
            snprintf(cmd, sizeof(cmd), "touch %s", shmKeyFile);
            system(cmd);   // 创建空白文件
            if (stat(shmKeyFile, &fInfo) == 0)	{
                key = fInfo.st_ino;
            }
        }
    } else {
        key = fInfo.st_ino;
    }

    if (key == 0) {
        printf("create key file = %s error = %d, info = %s", keyFile, errno, strerror(errno));
    }

    return key;
}

// 锁定key文件，防止重复多次打开共享内存
// 进程退出后文件自动解锁，不需要显示解锁key文件动作
// 解锁动作：1）进程退出；2）close文件描述符；3）主动调用api解锁
bool CShmBuffer::lockFile(int fd, int lockPos)
{
    struct flock fLock;
    fLock.l_type = F_WRLCK;
    fLock.l_whence = SEEK_SET;
    fLock.l_start = lockPos;
    fLock.l_len = 1;
    // 设置阻塞锁
    if (fcntl(fd, F_SETLK, &fLock) < 0) {
        printf("lock shm key file = %s error = %d, info = %s", errno, strerror(errno));
        return false;
    }

    return true;
}

bool CShmBuffer::unLockFile(int fd, int lockPos)
{
    struct flock fLock;
    fLock.l_type = F_UNLCK;
    fLock.l_whence = SEEK_SET;
    fLock.l_start = lockPos;
    fLock.l_len = 1;
    if (fcntl(fd, F_SETLK, &fLock) < 0) {
        printf("unlock shm key file = %d error = %d, info = %s", fd, errno, strerror(errno));
        return false;
    }

    return true;
}

