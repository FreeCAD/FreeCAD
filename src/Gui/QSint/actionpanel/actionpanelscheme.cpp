/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "actionpanelscheme.h"


namespace QSint
{

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

    painter.setBrush(Qt::NoBrush);
    // Outline
    QColor outlineColor = palette.color(QPalette::Button);
    painter.setPen(QPen(outlineColor, penWidth * 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolyline(chevron);

    // Chevron
    if (hover) {
        penWidth *= 1.8;
    }
    QColor mainColor = palette.color(QPalette::Text);
    painter.setPen(QPen(mainColor, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPolyline(chevron);

    painter.end();
    return QPixmap::fromImage(img);
}

ActionPanelScheme::ActionPanelScheme()
{
    QFontMetrics fm(QApplication::font());
    headerSize = fm.height() + 10;

    QPalette p = QApplication::palette();

    int iconSize = fm.height();
    headerButtonSize = QSize(iconSize, iconSize);
    builtinFold = drawFoldIcon(p, true, false);
    builtinFoldOver = drawFoldIcon(p, true, true);
    builtinUnfold = drawFoldIcon(p, false, false);
    builtinUnfoldOver = drawFoldIcon(p, false, true);

    if (qApp->styleSheet().isEmpty()) {
        headerButtonFold = builtinFold;
        headerButtonFoldOver = builtinFoldOver;
        headerButtonUnfold = builtinUnfold;
        headerButtonUnfoldOver = builtinUnfoldOver;
    } else {
        headerButtonFold = QPixmap();
        headerButtonFoldOver = QPixmap();
        headerButtonUnfold = QPixmap();
        headerButtonUnfoldOver = QPixmap();
    }

    groupFoldSteps = 20;
    groupFoldDelay = 15;
    groupFoldEffect = NoFolding;
    groupFoldThaw = true;
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
}

void ActionPanelScheme::restoreActionStyle()
{
    headerButtonFold = builtinFold;
    headerButtonFoldOver = builtinFoldOver;
    headerButtonUnfold = builtinUnfold;
    headerButtonUnfoldOver = builtinUnfoldOver;
}

} // namespace QSint
