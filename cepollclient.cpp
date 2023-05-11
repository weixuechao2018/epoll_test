#include "cepollclient.h"

cEpollClient::cEpollClient(int iUserCount, const char* pIP, int iPort)
{
    strcpy(m_ip, pIP);
    m_iPort = iPort;
    m_iUserCount = iUserCount;
    m_iEpollFd = epoll_create(_MAX_SOCKFD_COUNT);
    m_pAllUserStatus = (struct UserStatus*)malloc(iUserCount*sizeof(struct UserStatus));
    for(int iuserid=0; iuserid<iUserCount ; iuserid++) {
        m_pAllUserStatus[iuserid].iUserStatus = FREE;
        m_pAllUserStatus[iuserid].iSockFd = -1;
    }
    memset(m_iSockFd_UserId, 0xFF, sizeof(m_iSockFd_UserId));
}

cEpollClient::~cEpollClient()
{
    free(m_pAllUserStatus);
}

/**
 * @brief  连接到服务器
 * @note
 * @param[in]
 * @param[out] none
 * @return          返回Fd
 * @section Others
 */
int cEpollClient::ConnectToServer(int iUserId, const char *pServerIp, unsigned short uServerPort)
{
    if (0 == m_iUserCount || m_iUserCount < (iUserId+1)) {
        return -1;
    }
    // 创建套接字
    if( (m_pAllUserStatus[iUserId].iSockFd = socket(AF_INET,SOCK_STREAM,0) ) < 0 ) {
        cout <<"[CEpollClient error]: init socket fail, reason is:"<<strerror(errno)<<",errno is:"<<errno<<endl;
        m_pAllUserStatus[iUserId].iSockFd = -1;
        m_pAllUserStatus[iUserId].iUserStatus = FREE;
        return -1;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(uServerPort);
    addr.sin_addr.s_addr = inet_addr(pServerIp);

    int ireuseadd_on = 1;//支持端口复用
    if ( -1 == setsockopt(m_pAllUserStatus[iUserId].iSockFd,
                          SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on))) {
        m_pAllUserStatus[iUserId].iUserStatus = FREE;
        return -1;
    }

    unsigned long ul = 1;
    ioctl(m_pAllUserStatus[iUserId].iSockFd, FIONBIO, &ul); //设置为非阻塞模式

    while (-1 == connect(m_pAllUserStatus[iUserId].iSockFd, (const sockaddr*)&addr, sizeof(addr))) {
        m_pAllUserStatus[iUserId].iUserStatus = CONNECT_ERR;
        return -1;
    }

    m_pAllUserStatus[iUserId].iUserStatus = CONNECT_OK;
    m_pAllUserStatus[iUserId].iSockFd = m_pAllUserStatus[iUserId].iSockFd;

    return m_pAllUserStatus[iUserId].iSockFd;
}

// 创建IO事件
int cEpollClient::creat_epoll_link(int iUserId)
{
    if (0 == m_iUserCount || m_iUserCount < (iUserId+1)) {
        return -1;
    }

    int isocketfd = -1;

    struct epoll_event event;
    bzero(&event, sizeof(event));
    isocketfd = ConnectToServer(iUserId, m_ip, m_iPort);
    if(isocketfd < 0) {
        cout <<"[CEpollClient error]: RunFun, connect fail" <<endl;
        CloseUser(iUserId);
        return -1;
    }
    m_iSockFd_UserId[isocketfd] = iUserId;//将用户ID和socketid关联起来

    event.data.fd = isocketfd;
    event.events = EPOLLIN|EPOLLOUT|EPOLLERR|EPOLLHUP;

    m_pAllUserStatus[iUserId].uEpollEvents = event.events;
    epoll_ctl(m_iEpollFd, EPOLL_CTL_ADD, event.data.fd, &event);

    return 0;
}

int cEpollClient::creat_epoll_link()
{
    for(int iuserid=0; iuserid<m_iUserCount; iuserid++) {
        creat_epoll_link(iuserid);
    }

    return 0;
}

// 修改
int cEpollClient::mod_epoll_fd(int iUserId, bool ETorNot)
{
    if (0 == m_iUserCount || m_iUserCount < (iUserId+1)) {
        return -1;
    }

    struct epoll_event event;
    bzero(&event, sizeof(event));

    event.events = m_pAllUserStatus[iUserId].uEpollEvents;
    event.data = event.data;
    if (ETorNot) {
        event.events |= EPOLLET;
    }

    if( ::epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, event.data.fd, &event) == -1 ) {
        return -1;
    }
    return 0;
}

int cEpollClient::SendToServerData(int iUserId, char *pRxBuff, int iBuffLen)
{
    if (0 == m_iUserCount || m_iUserCount < (iUserId+1)) {
        return -1;
    }

    int isendsize = -1;
    if( CONNECT_OK == m_pAllUserStatus[iUserId].iUserStatus || RECV_OK == m_pAllUserStatus[iUserId].iUserStatus) {
        isendsize = send(m_pAllUserStatus[iUserId].iSockFd, pRxBuff, iBuffLen, MSG_NOSIGNAL);
        if(isendsize < 0)  {
            cout <<"[CEpollClient error]: SendToServerData, send fail, reason is:"
                <<strerror(errno)<<",errno is:"<<errno<<endl;
        } else {
#if _CEPOLL_CLIENT_DEBUG_INFO
            printf("[CEpollClient info]: iUserId: %d Send Msg Content:%s\n", iUserId, pRxBuff);
#endif
            m_pAllUserStatus[iUserId].iUserStatus = SEND_OK;
        }
    }
    return isendsize;
}

int cEpollClient::RecvFromServer(int iUserId, char *pRecvBuff,int iBuffLen)
{
    int irecvsize = -1;
    if(SEND_OK == m_pAllUserStatus[iUserId].iUserStatus) {
        irecvsize = recv(m_pAllUserStatus[iUserId].iSockFd, pRecvBuff, iBuffLen, 0);
        if(0 > irecvsize) {
            cout <<"[CEpollClient error]: iUserId: " << iUserId
                << "RecvFromServer, recv fail, reason is:"<<strerror(errno)<<",errno is:"<<errno<<endl;
        } else if(0 == irecvsize) {
            cout <<"[warning:] iUserId: "<< iUserId
                << "RecvFromServer, STB收到数据为0，表示对方断开连接,irecvsize:"<<irecvsize
                <<",iSockFd:"<< m_pAllUserStatus[iUserId].iSockFd << endl;
        } else {
#if _CEPOLL_CLIENT_DEBUG_INFO
            printf("Recv Server Msg Content:%s\n", pRecvBuff);
#endif
            m_pAllUserStatus[iUserId].iUserStatus = RECV_OK;
        }
    }
    return irecvsize;
}

bool cEpollClient::CloseUser(int iUserId)
{
    //
    close(m_pAllUserStatus[iUserId].iSockFd);
    m_pAllUserStatus[iUserId].iUserStatus = FREE;
    m_pAllUserStatus[iUserId].iSockFd = -1;
    return true;
}

bool cEpollClient::DelEpoll(int iSockFd)
{
    bool bret = false;
    struct epoll_event event_del;
    if(0 < iSockFd) {
      event_del.data.fd = iSockFd;
      event_del.events = 0;
      if( 0 == epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, event_del.data.fd, &event_del) ) {
          bret = true;
      } else {
          cout <<"[SimulateStb error:] DelEpoll,epoll_ctl error,iSockFd:"<<iSockFd<<endl;
      }
      m_iSockFd_UserId[iSockFd] = -1;
    } else {
      bret = true;

    }
    return bret;
}
