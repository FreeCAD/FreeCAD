/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "androidpanelscheme.h"


namespace QSint
{


const char* ActionPanelAndroidStyle =

    "QFrame[class='panel'] {"
        "background-color: #424242;"
        "color: #ffffff;"
        "font-size: 10px;"
    "}"

    "QSint--ActionGroup QFrame[class='header'] {"
        "background-color: #171717;"
        "border-top: 1px solid white;"
        "border-bottom: 1px solid white;"
        "color: #ffffff;"
    "}"

    "QSint--ActionGroup QToolButton[class='header'] {"
        "text-align: left;"
        "color: #ffffff;"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "font-size: 14px;"
    "}"

    "QSint--ActionGroup QFrame[class='content'] {"
        "background-color: #171717;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "color: #ffffff;"
        "text-align: left;"
        "font-size: 14px;"
        "min-height: 32px;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:!enabled {"
        "color: #666666;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:focus {"
        "border: 1px dotted black;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
    "}"

;


AndroidPanelScheme::AndroidPanelScheme() : ActionPanelScheme()
{
    headerSize = 40;

    headerButtonFold = QPixmap(":/android/Fold.png");
    headerButtonFoldOver = QPixmap(":/android/FoldOver.png");
    headerButtonUnfold = QPixmap(":/android/Unfold.png");
    headerButtonUnfoldOver = QPixmap(":/android/UnfoldOver.png");
    headerButtonSize = QSize(33,33);

    actionStyle = QString(ActionPanelAndroidStyle);
}


}

