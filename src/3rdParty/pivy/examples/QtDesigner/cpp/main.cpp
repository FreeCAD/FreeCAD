#include <QApplication>
#include "mainwindow.h"
#include <Inventor/Qt/SoQt.h>

int main(int argc, char *argv[])
{
  SoQt::init(argc, argv, argv[0]);
  QApplication app(argc, argv);
  MainWindow *mw = new MainWindow();
  app.connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));
  mw->show();
  return app.exec();
}
