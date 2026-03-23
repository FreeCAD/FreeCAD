#ifndef mainwindow_h_
#define mainwindow_h_

#include <QMainWindow>
#include "ui_test.h"

class MainWindow : public QMainWindow, public Ui::MainWindow
{
  Q_OBJECT;

  class SoQtExaminerViewer *exam;
  class SoMaterial *material;
  class SoGate *gate;
  class SoRotationXYZ *rotxyz;

  void setupSoQt();
 public:
  MainWindow(QWidget *parent = 0);

 public slots:
  void change_axis(int axis);
  void change_color();
  void rotate();

};

#endif
