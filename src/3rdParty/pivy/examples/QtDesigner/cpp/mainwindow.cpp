#include "mainwindow.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoRotationXYZ.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/engines/SoGate.h>
#include <Inventor/engines/SoElapsedTime.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
#include <QButtonGroup>
#include <stdlib.h>

MainWindow::MainWindow(QWidget *parent)
  :QMainWindow(parent)
{
  setupUi(this);
  QButtonGroup *buttonGroup = new QButtonGroup(this->groupBox);
  buttonGroup->addButton(this->button_x, 0);
  buttonGroup->addButton(this->button_y, 1);
  buttonGroup->addButton(this->button_z, 2);
  QObject::connect(buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(change_axis(int)));
  QObject::connect(this->button, SIGNAL(clicked()), this, SLOT(change_color()));
  QObject::connect(this->checkbox, SIGNAL(clicked()), this, SLOT(rotate()));
  
  setupSoQt();
}

void
MainWindow::change_axis(int axis)
{
  this->rotxyz->axis = axis;
}

void
MainWindow::change_color()
{
  this->material->diffuseColor = SbColor(1.0f*(random()%256)/255,
                                         1.0f*(random()%256)/255,
                                         1.0f*(random()%256)/255);
}

void
MainWindow::rotate()
{
  this->gate->enable = !this->gate->enable.getValue();
}

void
MainWindow::setupSoQt()
{
  SoSeparator *root = new SoSeparator();
  this->rotxyz = new SoRotationXYZ();
  this->gate = new SoGate(SoMFFloat::getClassTypeId());
  SoElapsedTime *elapsedTime = new SoElapsedTime();
  this->gate->enable = false;
  this->gate->input->connectFrom(&elapsedTime->timeOut);
  this->rotxyz->angle.connectFrom(this->gate->output);
  this->material = new SoMaterial();
  this->material->diffuseColor = SbColor(0.0, 1.0, 1.0);
  SoCone *cone = new SoCone();
  root->addChild(this->rotxyz);
  root->addChild(this->material);
  root->addChild(cone);
    
  this->exam = new SoQtExaminerViewer(this->examiner);
  this->exam->setSceneGraph(root);
}
