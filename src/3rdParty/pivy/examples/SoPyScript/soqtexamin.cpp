#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>

#include <Inventor/Qt/SoQt.h>
#include <Inventor/Qt/viewers/SoQtExaminerViewer.h>

#include "SoPyScript.h"

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: %s file.iv\n", argv[0]);
    exit(1);
  }

  // initialize Inventor and Qt
  QWidget * window = SoQt::init(argv[0]);

  SoPyScript::initClass();

  SoInput * input = new SoInput();
  input->openFile(argv[1]);

  SoSeparator * root = new SoSeparator;
  root->ref();

  root->addChild(SoDB::readAll(input));
  
  // initialize an Examiner Viewer
  SoQtExaminerViewer * examinerViewer = new SoQtExaminerViewer(window);
  examinerViewer->setSceneGraph(root);
  examinerViewer->show();

  SoQt::show(window);
  SoQt::mainLoop();

  return 0;
}
