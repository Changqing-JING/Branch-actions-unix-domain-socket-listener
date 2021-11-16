#pragma once

#include <QMainWindow>
#include <QtCore/QTimer>
#include <QtGui/QStandardItemModel>
#include <QtNetwork/QtNetwork>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  Ui::MainWindow *ui;
  QTcpSocket *tcpSocket;
  uint32_t tcpdatalen;
  QTimer *m_timer;
  int table_row;

  void switchListenerStatus(bool status);
  void onListenerEnable();
  void onListenerDisable();
  void addLine();
private slots:
  void displayError(QAbstractSocket::SocketError);
};
