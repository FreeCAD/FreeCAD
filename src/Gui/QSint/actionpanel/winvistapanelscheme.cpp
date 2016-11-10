/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "winvistapanelscheme.h"


namespace QSint
{


const char* ActionPanelWinVistaStyle =

    "QFrame[class='panel'] {"
        "background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #416FA6, stop: 1 #6BB86E);"
    "}"

    "QSint--ActionGroup QFrame[class='header'] {"
        "background: transparent;"
        "border: 1px solid transparent;"
    "}"

    "QSint--ActionGroup QFrame[class='header']:hover {"
        "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 rgba(249,253,255,100), stop: 0.5 rgba(234,247,255,20), stop: 1 rgba(249,253,255,100));"
        "border: 1px solid transparent;"
    "}"

    "QSint--ActionGroup QToolButton[class='header'] {"
        "text-align: left;"
        "color: #ffffff;"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "font-size: 12px;"
    "}"

    "QSint--ActionGroup QFrame[class='content'] {"
        "background-color: transparent;"
        "color: #ffffff;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "color: #ffffff;"
        "text-align: left;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:!enabled {"
        "color: #666666;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:hover {"
        "color: #DAF2FC;"
        "text-decoration: underline;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:focus {"
        "border: 1px dotted black;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
    "}"
;


WinVistaPanelScheme::WinVistaPanelScheme() : ActionPanelScheme()
{
  headerSize = 25;
  headerAnimation = false;

  //headerLabelScheme.iconSize = 22;

  headerButtonFold = QPixmap(":/vista/Fold.png");
  headerButtonFoldOver = QPixmap(":/vista/FoldOver.png");
  headerButtonUnfold = QPixmap(":/vista/Unfold.png");
  headerButtonUnfoldOver = QPixmap(":/vista/UnfoldOver.png");
  headerButtonSize = QSize(17,17);

  groupFoldSteps = 20; groupFoldDelay = 15;
  groupFoldThaw = true;
  groupFoldEffect = SlideFolding;

  actionStyle = QString(ActionPanelWinVistaStyle);
}


}

