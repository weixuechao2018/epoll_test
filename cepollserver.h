#ifndef CEPOLLSERVER_H
#define CEPOLLSERVER_H

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
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <pthread.h>

#define _MAX_SOCKFD_COUNT       65535
#define _MAX_SENDDATA_COUNT     4096

using namespace std;

class CEpollServer
{
public:
        CEpollServer();
        ~CEpollServer();

        bool InitServer(const char* chIp, int iPort);
        void Listen();
        static void ListenThread( void* lpVoid );
        void Run();

private:
        int        m_iEpollFd;
        int        m_isock;
        pthread_t       m_ListenThreadId;// 监听线程句柄

};

#endif // CEPOLLSERVER_H
