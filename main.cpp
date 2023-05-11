#include "mainwindow.h"
#include <QApplication>

#include "cepollserver.h"
#include "cepollclient.h"
#include "cshmbuf.h"
#include "cshmbuffer.h"
int main(int argc, char *argv[])
{
//    QApplication a(argc, argv);
//    MainWindow w;
//    w.hide();

//    CEpollServer  server;
//    server.InitServer("127.0.0.1", 12345);
//    server.Run();

    CShmBuffer mCShmBuffer;
    mCShmBuffer.open(1234567890, 32);

    char buffer[32] = {0};
    while (1) {
        mCShmBuffer.write("sleep1234567890", 15);
        sleep(2);
    }

    CShmBuf mCShmBuf;

    mCShmBuf.open(123456789, 32);

    while (1) {
        mCShmBuf.read(buffer,15,0);
        sleep(2);
    }

    cEpollClient m_cEpollClient(5,"127.0.0.1", 12345);

    //m_cEpollClient.InitServer("127.0.0.1", 8000);
    //m_cEpollClient.Run();

    while (1) {
       // m_cEpollClient.SendToServerData(,"1234567;1234567;1234567;1234567;", 32);
    }
    return 0;
}
