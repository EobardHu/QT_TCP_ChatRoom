#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QString strip = GetLocalIP();
    ui->comboBox_IP->addItem(strip);
    ui->comboBox_IP->addItem("127.0.0.1");

    ui->pushButton_start->setEnabled(true);
    ui->pushButton_stop->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;

    // 清理所有连接的服务器
    for (QTcpSocket *client : tcpClients) {
        client->disconnectFromHost();
        client->deleteLater();
    }
}

QString MainWindow::GetLocalIP()
{
    QString hostname = QHostInfo::localHostName();
    QHostInfo hostinfo = QHostInfo::fromName(hostname);

    QString localip = "";
    QList<QHostAddress> addList = hostinfo.addresses();
    if (!addList.isEmpty()) {
        for (int i = 0; i < addList.count(); i++) {
            QHostAddress ahost = addList.at(i);
            if (ahost.protocol() == QAbstractSocket::IPv4Protocol) {
                localip = ahost.toString();
                break;
            }
        }
    }
    return localip;
}

void MainWindow::connectToServer()
{
    QString addr = ui->comboBox_IP->currentText();
    quint16 port = ui->spinBox_PORT->value();

    // 创建新的 QTcpSocket 并连接到服务器
    QTcpSocket *newClient = new QTcpSocket(this);
    newClient->connectToHost(addr, port);

    // 绑定信号与槽
    connect(newClient, &QTcpSocket::connected, this, [=]() { connectfunc(newClient); });
    connect(newClient, &QTcpSocket::disconnected, this, [=]() { disconnectfunc(newClient); });
    connect(newClient, &QTcpSocket::readyRead, this, [=]() { socketreaddata(newClient); });

    tcpClients.append(newClient);

    // if (newClient->state() != QAbstractSocket::ConnectedState) {
    //     ui->plainTextEdit->appendPlainText("服务器未启动，连接失败");
    // }
}

void MainWindow::connectfunc(QTcpSocket *client)
{
    QString clientInfo = QString("********连接服务器********\nPeer Address: %1\nPeer Port: %2\n")
                             .arg(client->peerAddress().toString())
                             .arg(client->peerPort());

    ui->plainTextEdit->appendPlainText(clientInfo);

    ui->pushButton_start->setEnabled(false);
    ui->pushButton_stop->setEnabled(true);
}

void MainWindow::disconnectfunc(QTcpSocket *client)
{
    QString clientInfo = QString("********断开连接********\nPeer Address: %1\nPeer Port: %2\n")
                             .arg(client->peerAddress().toString())
                             .arg(client->peerPort());

    ui->plainTextEdit->appendPlainText(clientInfo);

    tcpClients.removeAll(client);
    client->deleteLater();

    if (tcpClients.isEmpty()) {
        ui->pushButton_start->setEnabled(true);
        ui->pushButton_stop->setEnabled(false);
    }
}

void MainWindow::socketreaddata(QTcpSocket *client)
{
    while (client->canReadLine()) {
        QString message = client->readLine().trimmed();

        if (message.startsWith("0:")) {
            // 消息来自服务器
            ui->plainTextEdit->appendPlainText(message.mid(2));
        } else if (message.startsWith("1:")) {
            // 消息是服务器转发的其他客户端的消息
            ui->plainTextEdit->appendPlainText(message.mid(2));
        } else {
            // 未知消息格式
            ui->plainTextEdit->appendPlainText("[Unknown] " + message);
        }
    }
}


void MainWindow::on_pushButton_start_clicked()
{
    connectToServer();
}

void MainWindow::on_pushButton_stop_clicked()
{
    for (QTcpSocket *client : tcpClients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->disconnectFromHost();
        }
    }
}

void MainWindow::on_pushButton_send_clicked()
{
    for (QTcpSocket *tcpclient : tcpClients) {
    if (tcpclient && tcpclient->state() == QAbstractSocket::ConnectedState) {
        QString strmsg = ui->lineEdit_Input->text();
        quint16 localPort = tcpclient->localPort();  // 获取本地端口号

        // 在 UI 上显示客户端端口号和发送的消息
        ui->plainTextEdit->appendPlainText(QString("[Client][%1]").arg(localPort) +"："+ strmsg);

        // 发送消息到服务器
        ui->lineEdit_Input->clear();
        QByteArray str = strmsg.toUtf8();
        str.append("\n");
        tcpclient->write(str);
    } else {
        ui->plainTextEdit->appendPlainText("连接失败，服务未连接");
    }
    }
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    for (QTcpSocket *client : tcpClients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->disconnectFromHost();
        }
    }
    event->accept();
}
