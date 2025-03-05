/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionpanelscheme.h"


namespace QSint
{


const QString ActionPanelScheme::minimumStyle = QStringLiteral(
    "QSint--ActionGroup QFrame[class='header'] {"
        "border: none;"
    "}"

    "QSint--ActionGroup QToolButton[class='header'] {"
        "border: none;"
        "font-weight: bold;"
        "text-align: center;"
        "background: none;"
    "}"

    "QSint--ActionGroup QToolButton[class='action'] {"
        "border: none;"
        "background: none;"
    "}"

    "QSint--ActionGroup QToolButton[class='action']:hover {"
        "text-decoration: underline;"
    "}"

    "QSint--ActionGroup QFrame[class='content'][header='true'] {"
        "border: none;"
    "}"

);

QString ActionPanelScheme::systemStyle(const QPalette& p)
{
    const QColor& highlightColor = p.color(QPalette::Highlight);
    QColor headerBackground = highlightColor.darker(150);
    const QColor& groupBackground = p.color(QPalette::Button);

    QHash<QString, QString> replacements;
    replacements["headerBackground"] = headerBackground.name();
    replacements["groupBackground"] = groupBackground.name();;

    QString style = QStringLiteral(
        "QSint--ActionGroup QFrame[class='header'] {"
            "background-color: {headerBackground};"
        "}"
        "QSint--ActionGroup QFrame[class='content'] {"
            "background-color: {groupBackground};"
        "}"
    );

    for (auto it = replacements.begin(); it != replacements.end(); ++it) {
        style.replace("{" + it.key() + "}", it.value());
    }

    return style;
}

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
        // Upward
        chevron << QPoint(lef_X, top_Y)
                << QPoint(mid_X, bot_Y)
                << QPoint(rig_X, top_Y);
    } else {
        // Downward
        chevron << QPoint(lef_X, bot_Y)
                << QPoint(mid_X, top_Y)
                << QPoint(rig_X, bot_Y);
    }

    painter.drawPolyline(chevron);
    painter.end();
    return QPixmap::fromImage(img);
}

ActionPanelScheme::ActionPanelScheme()
{
    QFontMetrics fm(QApplication::font());
    headerSize = fm.height() + 10;
    headerAnimation = true;

    QPalette p = QApplication::palette();

    headerButtonSize = QSize(16, 16);
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
