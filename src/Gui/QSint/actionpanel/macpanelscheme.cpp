/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "macpanelscheme.h"


namespace QSint
{


const char* MacPanelStyle =

    "QFrame[class='panel'] {"
        "background-color: #D9DEE5;"
        "border: 1px solid #BBBBBB;"
    "}"

    "QSint--ActionGroup QFrame[class='header'] {"
        "border: none;"
    "}"

    "QSint--ActionGroup QToolButton[class='header'] {"
        "text-align: left;"
        "color: #828CA8;"
        "background-color: transparent;"
        "font-weight: bold;"
    "}"

    "QSint--ActionGroup QFrame[class='content'] {"
        "background-color: #D9DEE5;"
        "margin-left: 12px;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "color: #000000;"
        "text-align: left;"
        "background-color: transparent;"
        "border: 1px solid transparent;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:!enabled {"
        "color: #ffffff;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:focus {"
        "border: 1px solid #1E6CBB;"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #6FA6DE, stop: 1 #1E6CBB);"
        "color: #ffffff;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
    "}"
;

MacPanelScheme::MacPanelScheme() : ActionPanelScheme()
{
    actionStyle = QString(MacPanelStyle);

    headerButtonFold = QPixmap();
    headerButtonFoldOver = QPixmap(":/mac/FoldOver.png");
    headerButtonUnfold = QPixmap();
    headerButtonUnfoldOver = QPixmap(":/mac/UnfoldOver.png");
    headerButtonSize = QSize(30,16);
}


}
