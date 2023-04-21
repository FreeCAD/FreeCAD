/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionpanelscheme.h"


class StaticLibInitializer
{
public:
    StaticLibInitializer()
    {
        Q_INIT_RESOURCE(schemes);
    }
};

StaticLibInitializer staticLibInitializer;


namespace QSint
{


const char* ActionPanelDefaultStyle =

    "QFrame[class='panel'] {"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #99cccc, stop: 1 #EAF7FF);"
    "}"

    "QSint--ActionGroup QFrame[class='header'] {"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F9FDFF, stop: 1 #EAF7FF);"
        "border: 1px solid #00aa99;"
        "border-bottom: 1px solid #99cccc;"
        "border-top-left-radius: 3px;"
        "border-top-right-radius: 3px;"
    "}"

    "QSint--ActionGroup QFrame[class='header']:hover {"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #EAF7FF, stop: 1 #F9FDFF);"
    "}"

    "QSint--ActionGroup QToolButton[class='header'] {"
        "text-align: left;"
        "font: 14px;"
        "color: #006600;"
        "background-color: transparent;"
        "border: 1px solid transparent;"
    "}"

    "QSint--ActionGroup QToolButton[class='header']:hover {"
        "color: #00cc00;"
        "text-decoration: underline;"
    "}"

    "QSint--ActionGroup QFrame[class='content'] {"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F9FDFF, stop: 1 #EAF7FF);"
        "border: 1px solid #00aa99;"
    "}"

    "QSint--ActionGroup QFrame[class='content'][header='true'] {"
        "border-top: none;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "color: #0033ff;"
        "text-align: left;"
        "font: 11px;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:!enabled {"
        "color: #999999;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:hover {"
        "color: #0099ff;"
        "text-decoration: underline;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:focus {"
        "border: 1px dotted black;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:on {"
        "background-color: #ddeeff;"
        "color: #006600;"
    "}"

    // set a QGroupBox to avoid that the OS style is used, see
    // https://github.com/FreeCAD/FreeCAD/issues/6102
    // the px values are taken from Behave-dark.qss, except the padding
    "QSint--ActionGroup QFrame[class='content'] QGroupBox {"
    "border: 1px solid #bbbbbb;"
    "border-radius: 3px;"
    "margin-top: 10px;"
    "padding: 2px;"
    "}"
    // since we set a custom frame we also need to set the title
    "QSint--ActionGroup QFrame[class='content'] QGroupBox::title {"
    "padding-left: 3px;"
    "top: -6px;"
    "left: 12px;"
    "}"
;


ActionPanelScheme::ActionPanelScheme()
{
    headerSize = 28;
    headerAnimation = true;

    headerButtonFold = QPixmap(":/default/Fold.png");
    headerButtonFoldOver = QPixmap(":/default/FoldOver.png");
    headerButtonUnfold = QPixmap(":/default/Unfold.png");
    headerButtonUnfoldOver = QPixmap(":/default/UnfoldOver.png");
    headerButtonSize = QSize(18,18);

    groupFoldSteps = 20; groupFoldDelay = 15;
    groupFoldEffect = NoFolding;
    groupFoldThaw = true;

    actionStyle = QString(ActionPanelDefaultStyle);
}

ActionPanelScheme* ActionPanelScheme::defaultScheme()
{
    static ActionPanelScheme scheme;
    return &scheme;
}


}
