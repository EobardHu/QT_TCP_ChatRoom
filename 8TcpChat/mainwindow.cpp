#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString strip = GetLocalIpAddress();
    ui->comboBox_IP->addItem(strip);
    ui->comboBox_IP->addItem("127.0.0.1");

    tcpserver = new QTcpServer(this);
    connect(tcpserver, &QTcpServer::newConnection, this, &MainWindow::newconnect);

    ui->pushButton_start->setEnabled(true);
    ui->pushButton_stop->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::GetLocalIpAddress()
{
    QString hostname = QHostInfo::localHostName();
    QHostInfo hostinfo = QHostInfo::fromName(hostname);

    QString localip = "";

    QList<QHostAddress> addrlist = hostinfo.addresses();
    if (!addrlist.isEmpty()) {
        for (int i = 0; i < addrlist.size(); i++) {
            QHostAddress addrhost = addrlist.at(i);
            if (addrhost.protocol() == QAbstractSocket::IPv4Protocol) {
                localip = addrhost.toString();
                break;
            }
        }
    }
    return localip;
}

void MainWindow::newconnect()
{
    // 接收新连接
    QTcpSocket *newSocket = tcpserver->nextPendingConnection();

    // 保存新连接
    clients.append(newSocket);

    // 输出新连接信息
    QString clientInfo = QString("客户端已连接：%1:%2")
                             .arg(newSocket->peerAddress().toString())
                             .arg(newSocket->peerPort());
    ui->plainTextEdit->appendPlainText(clientInfo);

    // 绑定信号与槽
    connect(newSocket, &QTcpSocket::readyRead, this, [=]() { socketreaddata(newSocket); });
    connect(newSocket, &QTcpSocket::disconnected, this, [=]() { clientdisconnect(newSocket); });
}

void MainWindow::socketreaddata(QTcpSocket *clientSocket)
{
    while (clientSocket->canReadLine()) {
        QString message = clientSocket->readLine().trimmed();
        QString clientInfo = QString("[%1]").arg(clientSocket->peerPort());
                                 // .arg(clientSocket->peerAddress().toString())


        // 在 UI 上显示接收到的客户端消息
        ui->plainTextEdit->appendPlainText("[Client]"+clientInfo +"：" + message);

        // 构建转发的消息，添加前缀 "1:"
        QString forwardedMessage = "1:[Client]"+clientInfo + "：" + message;

        // 向其他客户端转发消息
        for (QTcpSocket *client : clients) {
            if (client != clientSocket && client->state() == QAbstractSocket::ConnectedState) {
                client->write(forwardedMessage.toUtf8() + "\n");
            }
        }
    }
}


void MainWindow::clientdisconnect(QTcpSocket *clientSocket)
{
    QString clientInfo = QString("客户端断开连接：%1:%2")
                             .arg(clientSocket->peerAddress().toString())
                             .arg(clientSocket->peerPort());
    ui->plainTextEdit->appendPlainText(clientInfo);

    clients.removeAll(clientSocket);
    clientSocket->deleteLater();
}

void MainWindow::on_pushButton_start_clicked()
{
    QString ip = ui->comboBox_IP->currentText();
    quint16 port = ui->spinBox_PORT->value();

    QHostAddress address(ip);
    tcpserver->listen(address, port);

    ui->plainTextEdit->appendPlainText("$$$$$开始监听$$$$$");
    ui->plainTextEdit->appendPlainText("$$$$$服务器地址：" + tcpserver->serverAddress().toString());
    ui->plainTextEdit->appendPlainText("$$$$$服务器端口：" + QString::number(tcpserver->serverPort()) + "\n");

    ui->pushButton_start->setEnabled(false);
    ui->pushButton_stop->setEnabled(true);
}

void MainWindow::on_pushButton_stop_clicked()
{
    if (tcpserver->isListening()) {
        tcpserver->close();
        ui->plainTextEdit->appendPlainText("$$$$$关闭服务$$$$$");

        foreach (QTcpSocket *client, clients) {
            client->disconnectFromHost();
            client->deleteLater();
        }
        clients.clear();

        ui->pushButton_start->setEnabled(true);
        ui->pushButton_stop->setEnabled(false);
    }
}

void MainWindow::on_pushButton_send_clicked()
{
    QString strmsg = ui->lineEdit_Input->text();
    QString strsend = QString("0:[Server][%1]").arg(QString::number(tcpserver->serverPort())) + "：" + strmsg;
    QByteArray str = strsend.toUtf8();
    str.append("\n");

    for (QTcpSocket *client : clients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->write(str);
            // QString clientInfo = QString("[%1]").arg(client->peerPort());
                                     // .arg(client->peerAddress().toString())
        }
    }
    ui->plainTextEdit->appendPlainText(strsend.mid(2));

    ui->lineEdit_Input->clear();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (tcpserver->isListening()) {
        tcpserver->close();
    }
    event->accept();
}
