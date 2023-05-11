#include "cepoll.h"

cEpoll::cEpoll(int _max, int maxevents):max(_max),
    epoll_fd(-1),
    epoll_timeout(0),
    epoll_maxevents(maxevents),
    backEvents(0)
{

}

cEpoll::~cEpoll()
{
    if (isValid()) {
        close(epoll_fd);
    }
    delete[] backEvents;
}


inline bool cEpoll::isValid() const
{
    return epoll_fd > 0;
}

inline void cEpoll::setTimeout(int timeout)
{
    epoll_timeout = timeout;
}

inline void cEpoll::setMaxEvents(int maxevents)
{
    epoll_maxevents = maxevents;
}

inline const epoll_event* cEpoll::events() const
{
    return backEvents;
}

const epoll_event& cEpoll::operator[](int index)
{
    return backEvents[index];
}


int cEpoll::create()
{
    epoll_fd = ::epoll_create(max);
    if (isValid()) {
        backEvents = new epoll_event[epoll_maxevents];
    } else {

    }
    return epoll_fd;
}

int cEpoll::add(int fd, epoll_event *event)
{
    if (isValid()) {
        return ::epoll_ctl(epoll_fd, ADD, fd, event);
    }
    return -1;

}

int cEpoll::mod(int fd, epoll_event *event)
{
    if (isValid()) {
        return ::epoll_ctl(epoll_fd, MOD, fd, event);
    }
    return -1;

}

int cEpoll::del(int fd, epoll_event *event)
{
    if (isValid()) {
        return ::epoll_ctl(epoll_fd, DEL, fd, event);
    }
    return -1;
}

int cEpoll::wait()
{
    if (isValid()) {
        return ::epoll_wait(epoll_fd, backEvents, epoll_maxevents, epoll_timeout);
    }
    return -1;
}
