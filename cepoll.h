#ifndef CEPOLL_H
#define CEPOLL_H

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <stdbool.h>
#include<stdarg.h>
#include<unistd.h>
#include <string.h>

#include <sys/epoll.h>

/**
 * @brief The Epoll class 对epoll的封装
 */
class cEpoll
{
public:
    /**
     *
     */
    enum EPOLL_OP {ADD = EPOLL_CTL_ADD, MOD = EPOLL_CTL_MOD, DEL = EPOLL_CTL_DEL};
    /**
     * 最大的连接数和最大的回传事件数
     */
    cEpoll(int _max = 30, int maxevents = 20);
    ~cEpoll();

    int create();
    int add(int fd, epoll_event *event);
    int mod(int fd, epoll_event *event);
    int del(int fd, epoll_event *event);
    void setTimeout(int timeout);
    void setMaxEvents(int maxevents);
    int wait();
    const epoll_event* events() const;
    const epoll_event& operator[](int index);

private:
    bool isValid() const;
    int max;
    int epoll_fd;
    int epoll_timeout;
    int epoll_maxevents;

    epoll_event *backEvents;
};


#endif // CEPOLL_H
