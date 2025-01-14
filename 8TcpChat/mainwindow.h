#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostInfo>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newconnect();
    void clientdisconnect(QTcpSocket *clientSocket);
    void socketreaddata(QTcpSocket *clientSocket);

    void on_pushButton_start_clicked();
    void on_pushButton_stop_clicked();
    void on_pushButton_send_clicked();

private:
    Ui::MainWindow *ui;
    QTcpServer *tcpserver;
    QList<QTcpSocket*> clients;

    QString GetLocalIpAddress();
    void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H
