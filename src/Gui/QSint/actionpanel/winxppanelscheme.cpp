/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "winxppanelscheme.h"


namespace QSint
{


const char* ActionPanelWinXPBlueStyle1 =

    "QFrame[class='panel'] {"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #7ba2e7, stop: 1 #6375d6);"
    "}"

    "QSint--ActionGroup QFrame[class='header'] {"
        "background-color: #225aca;"
        "border: 1px solid #225aca;"
        "border-top-left-radius: 4px;"
        "border-top-right-radius: 4px;"
    "}"

    "QSint--ActionGroup QToolButton[class='header'] {"
        "text-align: left;"
        "color: #ffffff;"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "font-weight: bold;"
    "}"

    "QSint--ActionGroup QToolButton[class='header']:hover {"
        "color: #428eff;"
    "}"

    "QSint--ActionGroup QFrame[class='content'] {"
        "background-color: #eff3ff;"
        "border: 1px solid #ffffff;"
    "}"

    "QSint--ActionGroup QFrame[class='content'][header='true'] {"
        "border-top: none;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "color: #215dc6;"
        "text-align: left;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:!enabled {"
        "color: #999999;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:hover {"
        "color: #428eff;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:focus {"
        "border: 1px dotted black;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
    "}"
;

const char* ActionPanelWinXPBlueStyle2 =

    "QFrame[class='panel'] {"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #7ba2e7, stop: 1 #6375d6);"
    "}"

    "QSint--ActionGroup QFrame[class='header'] {"
        "border: 1px solid #ffffff;"
        "border-top-left-radius: 4px;"
        "border-top-right-radius: 4px;"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #ffffff, stop: 1 #c6d3f7);"
    "}"

    "QSint--ActionGroup QToolButton[class='header'] {"
        "text-align: left;"
        "color: #215dc6;"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "font-weight: bold;"
    "}"

    "QSint--ActionGroup QToolButton[class='header']:hover {"
        "color: #428eff;"
    "}"

    "QSint--ActionGroup QFrame[class='content'] {"
        "background-color: #d6dff7;"
        "border: 1px solid #ffffff;"
    "}"

    "QSint--ActionGroup QFrame[class='content'][header='true'] {"
        "border-top: none;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "color: #215dc6;"
        "text-align: left;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:!enabled {"
        "color: #999999;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:hover {"
        "color: #428eff;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:focus {"
        "border: 1px dotted black;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
    "}"
    ;



WinXPPanelScheme::WinXPPanelScheme() : ActionPanelScheme()
{
  headerSize = 25;
  headerAnimation = false;

  //headerLabelScheme.iconSize = 22;

  headerButtonFold = QPixmap(":/xp/Fold_Blue1.png");
  headerButtonFoldOver = QPixmap(":/xp/FoldOver_Blue1.png");
  headerButtonUnfold = QPixmap(":/xp/Unfold_Blue1.png");
  headerButtonUnfoldOver = QPixmap(":/xp/UnfoldOver_Blue1.png");
  headerButtonSize = QSize(17,17);

  groupFoldSteps = 20; groupFoldDelay = 15;
  groupFoldThaw = true;
  groupFoldEffect = SlideFolding;

  actionStyle = QString(ActionPanelWinXPBlueStyle1);
}


WinXPPanelScheme2::WinXPPanelScheme2() : ActionPanelScheme()
{
  headerSize = 25;
  headerAnimation = false;

//  headerLabelScheme.iconSize = 22;

  headerButtonFold = QPixmap(":/xp/Fold_Blue2.png");
  headerButtonFoldOver = QPixmap(":/xp/FoldOver_Blue2.png");
  headerButtonUnfold = QPixmap(":/xp/Unfold_Blue2.png");
  headerButtonUnfoldOver = QPixmap(":/xp/UnfoldOver_Blue2.png");
  headerButtonSize = QSize(17,17);

  groupFoldSteps = 20; groupFoldDelay = 15;
  groupFoldThaw = true;
  groupFoldEffect = SlideFolding;

  actionStyle = QString(ActionPanelWinXPBlueStyle2);
}


}
