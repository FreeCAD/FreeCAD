/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionpanelscheme.h"


namespace QSint
{


const QString ActionPanelScheme::minimumStyle = QString::fromLatin1(
    "QSint--ActionGroup QToolButton[class='header'] {"
        "text-align: left;"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "font-weight: bold;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "background-color: transparent;"
        "border: 1px solid transparent;"
        "text-align: left;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:hover {"
        "text-decoration: underline;"
    "}"
);

QString ActionPanelScheme::systemStyle(const QPalette& p)
{
    QString headerBackground = p.color(QPalette::Highlight).name();
    QString headerLabelText = p.color(QPalette::HighlightedText).name();
    QString headerLabelTextOver = p.color(QPalette::BrightText).name();
    QString groupBorder = p.color(QPalette::Mid).name();
    QString disabledActionText = p.color(QPalette::Disabled, QPalette::Text).name();
    QString actionSelectedBg = p.color(QPalette::Active, QPalette::Light).name();
    QString actionSelectedText = p.color(QPalette::Active, QPalette::ButtonText).name();
    QString actionSelectedBorder = p.color(QPalette::Active, QPalette::Highlight).name();
    QString panelBackground = p.color(QPalette::Window).name();
    QString groupBackground = p.color(QPalette::Button).name();

    QHash<QString, QString> replacements;
    replacements["headerBackground"] = headerBackground;
    replacements["headerLabelText"] = headerLabelText;
    replacements["headerLabelTextOver"] = headerLabelTextOver;
    replacements["groupBorder"] = groupBorder;
    replacements["disabledActionText"] = disabledActionText;
    replacements["actionSelectedBg"] = actionSelectedBg;
    replacements["actionSelectedText"] = actionSelectedText;
    replacements["actionSelectedBorder"] = actionSelectedBorder;
    replacements["panelBackground"] = panelBackground;
    replacements["groupBackground"] = groupBackground;

    QString style = QString::fromLatin1(
        "QFrame[class='panel'] {"
            "background-color: {panelBackground};"
        "}"

        "QSint--ActionGroup QFrame[class='header'] {"
            "border: 1px solid {headerBackground};"
            "background-color: {headerBackground};"
        "}"

        "QSint--ActionGroup QToolButton[class='header'] {"
            "color: {headerLabelText};"
        "}"

        "QSint--ActionGroup QToolButton[class='header']:hover {"
            "color: {headerLabelTextOver};"
        "}"

        "QSint--ActionGroup QFrame[class='content'] {"
            "border: 1px solid {groupBorder};"
            "background-color: {groupBackground};"
        "}"

        "QSint--ActionGroup QFrame[class='content'][header='true'] {"
            "border-top: none;"
        "}"

        "QSint--ActionGroup QToolButton[class='action']:!enabled {"
            "color: {disabledActionText};"
        "}"

        "QSint--ActionGroup QToolButton[class='action']:focus {"
            "color: {actionSelectedText};"
            "border: 1px dotted {actionSelectedBorder};"
        "}"

        "QSint--ActionGroup QToolButton[class='action']:on {"
            "background-color: {actionSelectedBg};"
            "color: {actionSelectedText};"
        "}"
    );

    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
        style.replace("{" + it.key() + "}", it.value());
    }

    return style;
}

// Draws fold/unfold icons based on the palette
QPixmap ActionPanelScheme::drawFoldIcon(const QPalette& palette, bool fold, bool hover) const
{
    QSize bSize = headerButtonSize;
    QImage img(bSize.width(), bSize.height(), QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);

    qreal penWidth = bSize.width() / 14.0;
    qreal lef_X = 0.25 * bSize.width();
    qreal mid_X = 0.50 * bSize.width();
    qreal rig_X = 0.75 * bSize.width();
    qreal bot_Y = 0.40 * bSize.height();
    qreal top_Y = 0.64 * bSize.height();

    if (hover) {
        penWidth *= 1.8;
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(palette.color(QPalette::HighlightedText), penWidth));

    QPolygon chevron;
    if (fold) {
        // Upward chevron
        chevron << QPoint(lef_X, top_Y)
                << QPoint(mid_X, bot_Y)
                << QPoint(rig_X, top_Y);
    } else {
        // Downward chevron
        chevron << QPoint(lef_X, bot_Y)
                << QPoint(mid_X, top_Y)
                << QPoint(rig_X, bot_Y);
    }

    painter.drawPolyline(chevron);
    return QPixmap::fromImage(img);
}

ActionPanelScheme::ActionPanelScheme()
{
    headerSize = 28;
    headerAnimation = true;

    QPalette p = QApplication::palette();

    headerButtonSize = QSize(17, 17);
    headerButtonFold = drawFoldIcon(p, true, false);
    headerButtonFoldOver = drawFoldIcon(p, true, true);
    headerButtonUnfold = drawFoldIcon(p, false, false);
    headerButtonUnfoldOver = drawFoldIcon(p, false, true);

    builtinFold = headerButtonFold;
    builtinFoldOver = headerButtonFoldOver;
    builtinUnfold = headerButtonUnfold;
    builtinUnfoldOver = headerButtonUnfoldOver;

    groupFoldSteps = 20;
    groupFoldDelay = 15;
    groupFoldEffect = NoFolding;
    groupFoldThaw = true;

    actionStyle = minimumStyle + systemStyle(p);
    builtinScheme = actionStyle;
}

ActionPanelScheme* ActionPanelScheme::defaultScheme()
{
    static ActionPanelScheme scheme;
    return &scheme;
}

void ActionPanelScheme::clearActionStyle()
{
    headerButtonFold = QPixmap();
    headerButtonFoldOver = QPixmap();
    headerButtonUnfold = QPixmap();
    headerButtonUnfoldOver = QPixmap();

    actionStyle = minimumStyle;
}

void ActionPanelScheme::restoreActionStyle()
{
    headerButtonFold = builtinFold;
    headerButtonFoldOver = builtinFoldOver;
    headerButtonUnfold = builtinUnfold;
    headerButtonUnfoldOver = builtinUnfoldOver;

    actionStyle = builtinScheme;
}

} // namespace QSint
