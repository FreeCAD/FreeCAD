
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QTextEdit;
QT_END_NAMESPACE

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

private Q_SLOTS:
    void loadFreeCAD();
    void newDocument();
    void embedWindow();
    void about();

private:
    void createActions();
    void createMenus();

    QMenu* fileMenu;
    QMenu* editMenu;
    QMenu* helpMenu;
    QAction* loadAct;
    QAction* newAct;
    QAction* embedAct;
    QAction* exitAct;
    QAction* aboutAct;
};

#endif
