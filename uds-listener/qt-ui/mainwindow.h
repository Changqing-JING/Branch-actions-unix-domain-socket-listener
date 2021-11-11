#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore/QTimer>
#include <QtGui/QStandardItemModel>

#include "../kern-service/receiver.h"

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
  Receiver m_recviver;
  QTimer *m_timer;
  bool m_status;
  int m_port;

  void switchListenerStatus(bool status);
  void onListenerEnable();
  void onListenerDisable();
  void display();
};
#endif // MAINWINDOW_H
