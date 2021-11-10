#include "mainwindow.h"
#include "./service/receiver.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  QObject::connect(ui->StartListenButton, &QRadioButton::toggled, this,
                   &MainWindow::switchListenerStatus);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::switchListenerStatus(bool status) {
  m_status = status;
  m_port = ui->PortInput->value();
  ui->ListenPortDisplay->display(m_port);
}
