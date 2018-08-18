#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include <QObject>
#include <QApplication>
#include <QTimer>
#include <QMessageBox>
#include <QTcpServer>
#include <QTcpSocket>

class TcpSocket : public QTcpServer
{
public:
    TcpSocket(QWidget *parent = 0);
    ~TcpSocket();
};

#endif // TCPSOCKET_H
