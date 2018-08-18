#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QTimer>
#include <QMessageBox>
#include <QLocalSocket>
#include <QDateTime>

#include <QDebug>

#include "tcpsocket.h"

#define CLAMAV_HTML_PORT 8080

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
private slots:
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void allHide();
    void allShow();
    void on_actionExit_triggered();
    void on_actionAbout_triggered();
    void receiveData();

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QAction *minimizeAction;
    QAction *restoreAction;
    QAction *quitAction;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;

    QLocalSocket *localnamedsock;
    QString localsockname;
    TcpSocket *tcpsock;
    QString getClamavSocketName();
    void initLocalNamedSocket();
    void initLocalTcpSocket();
    void pingLocalNamedSocket();
    QByteArray transceiveLocalNamedSocket(QByteArray input);

protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H
