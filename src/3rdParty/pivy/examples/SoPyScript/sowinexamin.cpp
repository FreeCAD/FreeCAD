#include <Inventor/SoInput.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>

#include <Inventor/Win/SoWin.h>
#include <Inventor/Win/viewers/SoWinExaminerViewer.h>

#include "SoPyScript.h"

int
main(int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: %s file.iv\n", argv[0]);
    exit(1);
  }

  // initialize Inventor and Qt
  HWND window = SoWin::init(argv[0]);

  SoPyScript::initClass();

  SoInput * input = new SoInput();
  input->openFile(argv[1]);

  SoSeparator * root = new SoSeparator;
  root->ref();

  root->addChild(SoDB::readAll(input));
  
  // initialize an Examiner Viewer
  SoWinExaminerViewer * examinerViewer = new SoWinExaminerViewer(window);
  examinerViewer->setSceneGraph(root);
  examinerViewer->show();

  SoWin::show(window);
  SoWin::mainLoop();

  return 0;
}
