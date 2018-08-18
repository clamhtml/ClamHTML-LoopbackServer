#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    minimizeAction = new QAction(tr("&Minimize"), this);
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(allHide()));
    restoreAction = new QAction(tr("&Show"), this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(allShow()));
    quitAction = new QAction(tr("&Exit"), this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(on_actionExit_triggered()));
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addAction(quitAction);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    QIcon icon = QIcon(":/images/icon.png");
    trayIcon->setIcon(icon);
    setWindowIcon(icon);
    trayIcon->show();
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->setToolTip(windowTitle());

    initLocalNamedSocket();
    initLocalTcpSocket();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete localnamedsock;
}

void MainWindow::allHide(){
    setVisible(false);
}

void MainWindow::allShow(){
    setVisible(true);
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason){
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        if(isVisible()){
            allHide();
        }else{
            allShow();
        }
        break;
    default:
        ;
    }
}

void MainWindow::on_actionExit_triggered(){
    qApp->quit();
}

void MainWindow::on_actionAbout_triggered(){
    QMessageBox::information(this, windowTitle(), tr("ClamAV HTML Socket Handler\n\nA proxy to connect the Anti-Virus Engine\nto the Browser Plugin (SnifFace)"));
}

void MainWindow::closeEvent(QCloseEvent *event){
    allHide();
    event->ignore();
}

void MainWindow::initLocalNamedSocket(){
    localsockname = getClamavSocketName();
    if(!localsockname.isEmpty() && QFile(localsockname).exists()){
        localnamedsock = new QLocalSocket(this);
        pingLocalNamedSocket();
    }else{
        QTimer::singleShot(250, qApp, SLOT(quit()));
    }
}

void MainWindow::pingLocalNamedSocket(){
    if(transceiveLocalNamedSocket("PING") != QByteArray("PONG\n", 5))
        QTimer::singleShot(250, qApp, SLOT(quit()));
}

QByteArray MainWindow::transceiveLocalNamedSocket(QByteArray input){
    QByteArray output;
    localnamedsock->setServerName(localsockname);
    if(localnamedsock->open(QIODevice::ReadWrite) &&
            localnamedsock->waitForConnected() &&
            localnamedsock->write(input) == (quint64)input.length() &&
            localnamedsock->waitForReadyRead(5000) ){
        output = localnamedsock->readAll();
        if(!localnamedsock->waitForDisconnected()){
            QTimer::singleShot(250, qApp, SLOT(quit()));
        }
        localnamedsock->close();
    }else{
        QTimer::singleShot(250, qApp, SLOT(quit()));
    }
    return output;
}

void MainWindow::initLocalTcpSocket(){
    tcpsock = new TcpSocket(this);
    tcpsock->listen(QHostAddress("127.0.0.1"), (quint16)CLAMAV_HTML_PORT);
    if(!tcpsock->isListening()){
        QMessageBox::critical(this, windowTitle(), tr("TcpServer: Error: can't connect to localhost@")+QString::number(CLAMAV_HTML_PORT));
        QTimer::singleShot(250, qApp, SLOT(quit()));
    }
    connect(tcpsock, &TcpSocket::newConnection, this, &MainWindow::receiveData);
}

void MainWindow::receiveData(){
    QTcpSocket *clientConnection = tcpsock->nextPendingConnection();
    connect(clientConnection, &QAbstractSocket::disconnected, clientConnection, &QObject::deleteLater);
    clientConnection->waitForConnected(5000);
    if(clientConnection->waitForReadyRead(5000)){
        QByteArray lines = clientConnection->readAll();
        if(lines.length() > 4 && lines.indexOf(QByteArray("\r\n\r\n", 4)) > 0){
            QByteArray line = lines.mid(lines.indexOf(QByteArray("\r\n\r\n", 4))+4);
            QByteArray test;
            if(line.length() == 0){
                test = QByteArray("EMPTY", 5);
            }else if((quint64)line.length() <= (quint64)0xFFFFFFFF){
                QByteArray namedPipeSend("zINSTREAM\0", 10);
                QByteArray dataBE;
                QDataStream datastream(&dataBE, QIODevice::ReadWrite);
                datastream.setByteOrder(QDataStream::BigEndian);
                quint32 linelength = line.length();
                datastream << linelength;
                namedPipeSend.append(dataBE);
                namedPipeSend.append(line);
                namedPipeSend.append(QByteArray("\x00\x00\x00\x00", 4));
                test = transceiveLocalNamedSocket(namedPipeSend);
            }else{
                test = QByteArray("stream: OK\0", 11);
            }
            QByteArray res("HTTP/1.1 200 OK\r\n"
                "Date: "+QDateTime::currentDateTime().toUTC().toString("ddd, dd MMM yyyy hh:mm:ss t").toLatin1()+"\r\n"
                "Server: ClamHTML\r\n"
                "Content-Length: "+QByteArray::number(test.length())+"\r\n"
                "Connection: close\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "Content-Type: text/plain\r\n\r\n"+test); //application/json
            clientConnection->write(res);
        }
        clientConnection->disconnectFromHost();
    }
    tcpsock->disconnect(clientConnection);
}

QString MainWindow::getClamavSocketName(){
    QStringList configs({
        "/etc",
        "/usr/local/etc",
        "/etc/clamav",
        "/usr/local/etc/clamav"
    });
    QString ret("");
    QFile found;
    foreach(QString dirs, configs){
        QFile tmp(dirs+tr("/clamd.conf"));
        if(tmp.exists()){
            found.setFileName(dirs+tr("/clamd.conf"));
            break;
        }
    }
    if(found.fileName().isEmpty()){
        return tr("");
    }
    if (found.open(QIODevice::ReadOnly)){
        QTextStream in(&found);
        while (!in.atEnd()){
            QString line = in.readLine();
            if(QRegularExpression("^LocalSocket\\s+.*$").match(line).hasMatch()){
                found.close();
                return line.replace(QRegularExpression("^LocalSocket\\s+(.*)$"),"\\1");
            }
        }
        found.close();
    }
    return tr("");
}
