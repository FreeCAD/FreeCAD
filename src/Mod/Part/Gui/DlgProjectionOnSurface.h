#ifndef DLGPROJECTIONONSURFACE_H
#define DLGPROJECTIONONSURFACE_H

#include <QDialog>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

namespace Ui {
}

namespace PartGui {

  class Ui_DlgProjectionOnSurface;

class DlgProjectionOnSurface : public QDialog
{
    //Q_OBJECT

public:
    explicit DlgProjectionOnSurface(QWidget *parent = 0);
    ~DlgProjectionOnSurface();

private:
    Ui_DlgProjectionOnSurface*ui;
};

class TaskProjectionOnSurface : public Gui::TaskView::TaskDialog
{
  //Q_OBJECT

public:
  TaskProjectionOnSurface();
  ~TaskProjectionOnSurface();

public:
  bool accept();
  bool reject();
  void clicked(int);

  virtual QDialogButtonBox::StandardButtons getStandardButtons() const
  {
    return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Close;
  }

private:
  DlgProjectionOnSurface* widget;
  Gui::TaskView::TaskBox* taskbox;
};

} // namespace PartGui
#endif // DLGPROJECTIONONSURFACE_H
