/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionpanelscheme.h"


namespace QSint
{


constexpr QString ActionPanelScheme::minimumStyle = QStringLiteral(
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

// Draws fold/unfold icons based on the palette
QPixmap ActionPanelScheme::drawFoldIcon(const QPalette& palette, bool fold, bool /*hover*/) const
{
    QStyle::StandardPixmap iconType = fold ? QStyle::SP_ArrowUp : QStyle::SP_ArrowDown;
    QIcon icon = qApp->style()->standardIcon(iconType);

    QSize iconSize = headerButtonSize;
    QPixmap pixmap = icon.pixmap(iconSize);
    QImage img = pixmap.toImage();
    img = img.convertToFormat(QImage::Format_ARGB32);

    QPainter painter(&img);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(img.rect(), palette.color(QPalette::HighlightedText));
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
