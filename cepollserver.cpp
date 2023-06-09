#include "cepollserver.h"

CEpollServer::CEpollServer()
{
}

CEpollServer::~CEpollServer()
{
    close(m_isock);
}

bool CEpollServer::InitServer(const char* pIp, int iPort)
{
    m_iEpollFd = epoll_create(_MAX_SOCKFD_COUNT);

    //设置非阻塞模式
    int opts = O_NONBLOCK;
    if(fcntl(m_iEpollFd,F_SETFL,opts)<0) {
        printf("设置非阻塞模式失败!\n");
        return false;
    }

    m_isock = socket(AF_INET,SOCK_STREAM,0);
    if ( 0 > m_isock ) {
        printf("socket error!\n");
        return false;
    }

    sockaddr_in listen_addr;
    listen_addr.sin_family=AF_INET;
    listen_addr.sin_port=htons ( iPort );
    listen_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    listen_addr.sin_addr.s_addr=inet_addr(pIp);

    int ireuseadd_on = 1;//支持端口复用
    setsockopt(m_isock, SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on) );

    int nRecvBuf=0;//设置为32K
    setsockopt(m_isock, SOL_SOCKET, SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));

    if ( bind ( m_isock, ( sockaddr * ) &listen_addr,sizeof ( listen_addr ) ) !=0 ) {
      printf("bind error\n");
      return false;
    }

    if ( listen ( m_isock, 20) <0 ) {
      printf("listen error!\n");
      return false;
    } else {
      printf("服务端监听中...\n");
    }

    // 监听线程，此线程负责接收客户端连接，加入到epoll中
    if ( pthread_create( &m_ListenThreadId, 0, ( void * ( * ) ( void * ) ) ListenThread, this ) != 0 ) {
      printf("Server 监听线程创建失败!!!");
      return false;
    }
}

// 监听线程
void CEpollServer::ListenThread( void* lpVoid )
{
  CEpollServer *pTerminalServer = (CEpollServer*)lpVoid;
  sockaddr_in remote_addr;
  int len = sizeof (remote_addr);

  while ( true ) {
      int client_socket = accept (pTerminalServer->m_isock
                                  , ( sockaddr * ) &remote_addr
                                  , (socklen_t*)&len );
      if ( client_socket < 0 ) {
          printf("Server Accept失败!, client_socket: %d\n", client_socket);
          continue;
      } else {
          struct epoll_event    ev;
          ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
          ev.data.fd = client_socket;     //记录socket句柄
          epoll_ctl(pTerminalServer->m_iEpollFd, EPOLL_CTL_ADD, client_socket, &ev);
      }
  }
}

void CEpollServer::Run()
{
    while ( true ) {
        struct epoll_event    events[_MAX_SOCKFD_COUNT];
        int nfds = epoll_wait( m_iEpollFd, events,  _MAX_SOCKFD_COUNT, -1 );

        for (int i = 0; i < nfds; i++) {

            int client_socket = events[i].data.fd;
            char buffer[_MAX_SENDDATA_COUNT];//每次收发的字节数小于1024字节
            memset(buffer, 0, _MAX_SENDDATA_COUNT);

            if (events[i].events & EPOLLIN) { //监听到读事件，接收数据

                int rev_size = recv(events[i].data.fd,buffer, _MAX_SENDDATA_COUNT,0);
                if( rev_size <= 0 ) {
                    cout << "recv error: recv size: " << rev_size << endl;
                    struct epoll_event event_del;
                    event_del.data.fd = events[i].data.fd;
                    event_del.events = 0;
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);
                } else {
                    printf("Terminal Received Msg Content:%s\n",buffer);
                    struct epoll_event    ev;
                    ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
                    ev.data.fd = client_socket;     //记录socket句柄
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, client_socket, &ev);
                }
            } else if(events[i].events & EPOLLOUT) { //监听到写事件，发送数据

                char sendbuff[_MAX_SENDDATA_COUNT];
                sprintf(sendbuff, "Hello, client fd: %d\n", client_socket);
                int sendsize = send(client_socket, sendbuff, strlen(sendbuff)+1, MSG_NOSIGNAL);
                if(sendsize <= 0) {
                    struct epoll_event event_del;
                    event_del.data.fd = events[i].data.fd;
                    event_del.events = 0;
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);
                } else {
                    printf("Server reply msg ok! buffer: %s\n", sendbuff);
                    struct epoll_event    ev;
                    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
                    ev.data.fd = client_socket;     //记录socket句柄
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, client_socket, &ev);
                }
            } else {
                cout << "EPOLL ERROR\n" <<endl;
                epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
            }
        }
    }
}


//地址复用
void set_reuseaddr(int sockfd, int optval)
{
    int on = (optval != 0) ? 1 : 0;
    //int setsockopt(int sockfd, int level, int optname,const void *optval, socklen_t optlen);
    /*　sockfd：标识一个套接口的描述字。
    level：选项定义的层次；支持SOL_SOCKET、IPPROTO_TCP、IPPROTO_IP和IPPROTO_IPV6。
    optname：需设置的选项。
    optval：指针，指向存放选项值的缓冲区。
    optlen：optval缓冲区长度。
     返回值：  成功返回0，失败返回 -1.  */

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
//        ERR_EXIT("setsockopt SO_REUSEADDR");
    }
}

//端口复用
void set_reuseport(int sockfd, int optval)
{
#ifdef SO_REUSEPORT
    int on = (optval != 0) ? 1 : 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0) {
//        ERR_EXIT("setsockopt SO_REUSEPORT");
    }
#else
    fprintf(stderr, "SO_REUSEPORT is not supported.\n");
#endif //SO_REUSEPORT
}
