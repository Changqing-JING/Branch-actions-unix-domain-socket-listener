#include "mainwindow.h"
#include "./ui_mainwindow.h"

const QStringList labels =
    QObject::tr("fd,path,R/W,data,str").simplified().split(",");

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_recviver(),
      m_timer(new QTimer{this}) {
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
  ui->Data->setColumnWidth(1, 100);
  ui->Data->setColumnWidth(3, 600);
  ui->Data->setColumnWidth(4, 200);

  ui->Data->show();

  QObject::connect(ui->StartListenButton, &QRadioButton::toggled, this,
                   &MainWindow::switchListenerStatus);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::switchListenerStatus(bool status) {
  if (status) {
    onListenerEnable();
  } else {
    onListenerDisable();
  }
}

void MainWindow::onListenerEnable() {
  m_status = true;
  m_port = ui->PortInput->value();
  ui->ListenPortDisplay->display(m_port);
  m_recviver.start(m_port);

  QObject::connect(m_timer, &QTimer::timeout, this, &MainWindow::display);
  m_timer->start(1000);
}

void MainWindow::onListenerDisable() {
  m_status = false;
  m_port = 0;
  ui->ListenPortDisplay->display(m_port);
  m_recviver.stop();
}

std::shared_ptr<std::string> hexData(void *data, int len) {
  auto res = std::make_shared<std::string>();
  for (int i = 0; i < len; i++) {
    char t[3];
    sprintf(t, "%02x", static_cast<uint8_t *>(data)[i]);
    res->push_back(t[0]);
    res->push_back(t[1]);
    res->push_back(' ');
  }
  return res;
}
std::shared_ptr<std::string> strData(void *data, int len) {
  auto res = std::make_shared<std::string>();
  for (int i = 0; i < len; i++) {
    int ascii = static_cast<uint8_t *>(data)[i];
    res->push_back(ascii > 20 ? ascii : 46);
  }
  return res;
}

void MainWindow::display() {
  static int row = 0;
  while (m_recviver.canPopMessage()) {
    Data tmp = m_recviver.popMessage();

    QStandardItemModel *model =
        dynamic_cast<QStandardItemModel *>(ui->Data->model());
    //定义item
    QStandardItem *item;
    item = new QStandardItem(QString("%1").arg(tmp.fd));
    model->setItem(row, 0, item);
    item = new QStandardItem(QString("%1").arg(tmp.path));
    model->setItem(row, 1, item);
    item = new QStandardItem(QString("%1").arg(tmp.isRead ? "R" : "W"));
    model->setItem(row, 2, item);
    item = new QStandardItem(
        QString("%1").arg(hexData(tmp.p_data, tmp.len)->data()));
    model->setItem(row, 3, item);
    item = new QStandardItem(
        QString("%1").arg(strData(tmp.p_data, tmp.len)->data()));
    model->setItem(row, 4, item);

    row++;

    ui->Data->show();
  }
}
