#include "tcpsocket.h"

TcpSocket::TcpSocket(QWidget *parent) :
    QTcpServer(parent)
{

}

TcpSocket::~TcpSocket()
{
    if(isListening())
        close();
}

