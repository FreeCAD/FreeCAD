#include "PreCompiled.h"

#include "DlgProjectionOnSurface.h"
#include "ui_DlgProjectionOnSurface.h"

#include <Gui/BitmapFactory.h>



using namespace  PartGui;

DlgProjectionOnSurface::DlgProjectionOnSurface(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgProjectionOnSurface)
{
    ui->setupUi(this);
}

DlgProjectionOnSurface::~DlgProjectionOnSurface()
{
    delete ui;
}

TaskProjectionOnSurface::TaskProjectionOnSurface()
{
  widget = new DlgProjectionOnSurface();
  taskbox = new Gui::TaskView::TaskBox(
    Gui::BitmapFactory().pixmap("Part_Extrude"),
    widget->windowTitle(), true, 0);
  taskbox->groupLayout()->addWidget(widget);
  Content.push_back(taskbox);
}

TaskProjectionOnSurface::~TaskProjectionOnSurface()
{
  // automatically deleted in the sub-class
}

bool TaskProjectionOnSurface::accept()
{
  widget->accept();
  return (widget->result() == QDialog::Accepted);
}

bool TaskProjectionOnSurface::reject()
{
  widget->reject();
  return true;
}

void TaskProjectionOnSurface::clicked(int id)
{
  if (id == QDialogButtonBox::Apply) {
    try {
      //widget->apply();
    }
    catch (Base::AbortException&) {

    };
  }
}

