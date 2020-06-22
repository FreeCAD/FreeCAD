/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#include "iisiconlabel.h"
#include "iistaskpanelscheme.h"

iisIconLabel::iisIconLabel(const QIcon &icon, const QString &title, QWidget *parent)
    : QWidget(parent),
    myPixmap(icon),
    myText(title),
    mySchemePointer(0),
    m_over(false),
    m_pressed(false),
    m_changeCursorOver(true),
    m_underlineOver(true)
{
    setFocusPolicy(Qt::StrongFocus);
    setCursor(Qt::PointingHandCursor);

    myFont.setWeight(0);
    myPen.setStyle(Qt::NoPen);

    myColor = myColorOver = myColorDisabled = QColor();
}

iisIconLabel::~iisIconLabel()
{
    //if (m_changeCursorOver)
    //	QApplication::restoreOverrideCursor();
}

void iisIconLabel::setSchemePointer(iisIconLabelScheme **pointer)
{
    mySchemePointer = pointer;
    update();
}

void iisIconLabel::setColors(const QColor &color, const QColor &colorOver, const QColor &colorOff)
{
    myColor = color;
    myColorOver = colorOver;
    myColorDisabled = colorOff;
    update();
}

void iisIconLabel::setFont(const QFont &font)
{
    myFont = font;
    update();
}

void iisIconLabel::setFocusPen(const QPen &pen)
{
    myPen = pen;
    update();
}

QSize iisIconLabel::sizeHint() const
{
    return minimumSize();
}

QSize iisIconLabel::minimumSizeHint() const
{
    int s = (mySchemePointer && *mySchemePointer) ? (*mySchemePointer)->iconSize : 16;
    QPixmap px = myPixmap.pixmap(s,s,
        isEnabled() ? QIcon::Normal	: QIcon::Disabled);

    int h = 4+px.height();
    int w = 8 + px.width();
    if (!myText.isEmpty()) {
        QFontMetrics fm(myFont);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
        w += fm.horizontalAdvance(myText);
#else
        w += fm.width(myText);
#endif
        h = qMax(h, 4+fm.height());
    }

    return QSize(w+2,h+2);
}

void iisIconLabel::paintEvent ( QPaintEvent * event )
{
    Q_UNUSED(event);
    QPainter p(this);

    QRect textRect(rect().adjusted(0,0,-1,0));

    int x = 2;

    if (!myPixmap.isNull()) {
        int s = (mySchemePointer && *mySchemePointer) ? (*mySchemePointer)->iconSize : 16;
        QPixmap px = myPixmap.pixmap(s,s,
            isEnabled() ? QIcon::Normal	: QIcon::Disabled);
        p.drawPixmap(x,0,px);
        x += px.width() + 4;
    }

    if (!myText.isEmpty()) {
        QColor text = myColor, textOver = myColorOver, textOff = myColorDisabled;
        QFont fnt = myFont;
        QPen focusPen = myPen;
        bool underline = m_underlineOver/*, cursover = m_changeCursorOver*/;
        if (mySchemePointer && *mySchemePointer) {
            if (!text.isValid()) text = (*mySchemePointer)->text;
            if (!textOver.isValid()) textOver = (*mySchemePointer)->textOver;
            if (!textOff.isValid()) textOff = (*mySchemePointer)->textOff;
            if (!fnt.weight()) fnt = (*mySchemePointer)->font;
            if (focusPen.style() == Qt::NoPen) focusPen = (*mySchemePointer)->focusPen;
            underline = (*mySchemePointer)->underlineOver;
            //cursover = (*mySchemePointer)->cursorOver;
        }

        p.setPen(isEnabled() ? m_over ? textOver : text : textOff);

        if (isEnabled() && underline && m_over)
            fnt.setUnderline(true);
        p.setFont(fnt);

        textRect.setLeft(x);
        QRect boundingRect;

        QFontMetrics fm(fnt);
#if QT_VERSION >= 0x040203
        QString txt(fm.elidedText(myText, Qt::ElideRight, textRect.width()));
#else
        QString txt = myText;
#endif

        p.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, txt, &boundingRect);

        if (hasFocus()) {
            p.setPen(focusPen);
            p.drawRect(boundingRect.adjusted(-2,-1,0,0));
        }
    }
}


void iisIconLabel::enterEvent ( QEvent * /*event*/ )
{
    m_over = true;

    //if (m_changeCursorOver)
    //	QApplication::setOverrideCursor(Qt::PointingHandCursor);

    update();
}

void iisIconLabel::leaveEvent ( QEvent * /*event*/ )
{
    m_over = false;
    update();

    //if (m_changeCursorOver)
    //	QApplication::restoreOverrideCursor();
}

void iisIconLabel::mousePressEvent ( QMouseEvent * event )
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = true;
        Q_EMIT pressed();
    } else
        if (event->button() == Qt::RightButton)
            Q_EMIT contextMenu();

    update();
}

void iisIconLabel::mouseReleaseEvent ( QMouseEvent * event )
{
    if (event->button() == Qt::LeftButton) {
        m_pressed = false;
            Q_EMIT released();

        if (rect().contains( event->pos() )) {
            Q_EMIT clicked();
            Q_EMIT activated();
        }
    }

    update();
}

void iisIconLabel::keyPressEvent ( QKeyEvent * event )
{
    switch (event->key()) {
        case Qt::Key_Space:
        case Qt::Key_Return:
            Q_EMIT activated();
            break;

        default:;
    }

    QWidget::keyPressEvent(event);
}

