#include <iostream>

#include "./ui_mainwindow.h"
#include "mainwindow.h"

constexpr int DATA_PRE_LINE = 20;
const QStringList labels =
    QObject::tr("process,path,R/W,hex,str").simplified().split(",");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), table_row(0), tcpdatalen(0),
      m_timer(new QTimer{this}), tcpSocket(new QTcpSocket(this)) {
  ui->setupUi(this);

  QStandardItemModel *model = new QStandardItemModel();
  model->setHorizontalHeaderLabels(labels);

  ui->Data->setModel(model);
  ui->Data->resizeColumnToContents(0);
  ui->Data->resizeColumnToContents(1);
  ui->Data->resizeColumnToContents(2);
  ui->Data->horizontalHeader()->setSectionResizeMode(3,
                                                     QHeaderView::Interactive);
  ui->Data->horizontalHeader()->setSectionResizeMode(4,
                                                     QHeaderView::Interactive);
  ui->Data->setColumnWidth(1, 200);
  ui->Data->setColumnWidth(3, DATA_PRE_LINE * 8 * 3);
  ui->Data->setColumnWidth(4, DATA_PRE_LINE * 8);

  ui->Data->show();

  QObject::connect(ui->StartListenButton, &QRadioButton::toggled, this,
                   &MainWindow::switchListenerStatus);

  QObject::connect(tcpSocket, &QTcpSocket::readyRead, this,
                   &MainWindow::addLine);
  QObject::connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this,
                   SLOT(displayError(QAbstractSocket::SocketError)));
}

MainWindow::~MainWindow() {
  delete ui;
  delete tcpSocket;
  delete m_timer;
}

void MainWindow::switchListenerStatus(bool status) {
  if (status) {
    onListenerEnable();
  } else {
    onListenerDisable();
  }
}

void MainWindow::onListenerEnable() {
  int port = ui->PortInput->value();
  ui->ListenPortDisplay->display(port);
  tcpSocket->connectToHost(ui->IPInput->text(), port);
}

void MainWindow::onListenerDisable() {
  ui->ListenPortDisplay->display(0);
  tcpSocket->disconnectFromHost();
}

void MainWindow::addLine() {
  if (tcpdatalen == 0) {
    QByteArray rawlen = tcpSocket->read(sizeof(uint32_t));
    tcpdatalen = qFromBigEndian(*reinterpret_cast<uint32_t *>(rawlen.data()));
    std::cout << tcpdatalen << std::endl;
  }
  if (tcpSocket->bytesAvailable() < tcpdatalen) {
    return;
  }
  QByteArray rawmsg = tcpSocket->read(tcpdatalen);
  tcpdatalen = 0;
  QString msg{rawmsg};
  QStringList items = msg.split("\t");
  if (items.length() != 5) {
    std::cerr << "parser error\n";
    return;
  }

  QStandardItemModel *model =
      dynamic_cast<QStandardItemModel *>(ui->Data->model());
  QStandardItem *item;
  item = new QStandardItem(QString("%1").arg(items[0]));
  model->setItem(table_row, 0, item);
  item = new QStandardItem(QString("%1").arg(items[1]));
  model->setItem(table_row, 1, item);
  item = new QStandardItem(QString("%1").arg(items[2]));
  model->setItem(table_row, 2, item);
  item = new QStandardItem(QString("%1").arg(items[3]));
  model->setItem(table_row, 3, item);
  item = new QStandardItem(QString("%1").arg(items[4]));
  model->setItem(table_row, 4, item);
  ui->Data->resizeRowToContents(table_row);

  table_row++;
}

void MainWindow::displayError(QAbstractSocket::SocketError) {
  qDebug() << tcpSocket->errorString(); //输出错误信息
}