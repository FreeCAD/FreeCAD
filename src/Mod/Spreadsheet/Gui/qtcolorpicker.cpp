// SPDX-License-Identifier: LGPL-2.1-only OR GPL-3.0-only

/****************************************************************************
**
** This file is part of a Qt Solutions component.
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Modified 2022 by 0penBrain under LGPL : fix issues about popup positioning
** Modified 2026 by JonasVgt under LGPL : add additional color picker functionality
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Solutions Commercial License Agreement provided
** with the Software or, alternatively, in accordance with the terms
** contained in a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** Please note Third Party Software included with Qt Solutions may impose
** additional restrictions and it is the user's responsibility to ensure
** that they have met the licensing requirements of the GPL, LGPL, or Qt
** Solutions Commercial license and the relevant license of the Third
** Party Software they are using.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
****************************************************************************/

#include <cmath>

#include <QApplication>
#include <QColorDialog>
#include <QGridLayout>
#include <QLayout>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QtCore/QMap>
#include <QtGui/QFocusEvent>
#include <QtGui/QHideEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPixmap>
#include <QtGui/QShowEvent>

#include <Gui/FileDialog.h>

#include "qtcolorpicker.h"

// clang-format off
/*! \class QtColorPicker

    \brief The QtColorPicker class provides a widget for selecting
    colors from a popup color grid.

    Users can invoke the color picker by clicking on it, or by
    navigating to it and pressing Space. They can use the mouse or
    arrow keys to navigate between colors on the grid, and select a
    color by clicking or by pressing Enter or Space. The
    colorChanged() signal is emitted whenever the color picker's color
    changes.

    The widget also supports negative selection: Users can click and
    hold the mouse button on the QtColorPicker widget, then move the
    mouse over the color grid and release the mouse button over the
    color they wish to select.

    The color grid shows a customized selection of colors. An optional
    "Custom Colors" button can be added below the grid; if the user
    presses this, a QColorDialog pops up and lets them choose any
    color they like. This button is made available by using
    setColorDialogEnabled().

    When the widget is created, the default color is selected. This color
    can be set using setDefaultColor(). When a color is selected, the
    QtColorPicker widget shows the color and its name. If the name cannot
    be determined, the translatable name "Custom" is used. Using the
    "Reset" button above the grid, the selected color can be cleared,
    such that the default color is selected again.

    The QtColorPicker object is optionally initialized with the number
    of columns in the color grid. Colors are then added left to right,
    top to bottom using insertColor(). If the number of columns is not
    set, QtColorPicker calculates the number of columns and rows that
    will make the grid as square as possible.

    \code
    DrawWidget::DrawWidget(QWidget *parent, const char *name)
    {
        QtColorPicker *picker = new QtColorPicker(this);
        picker->insertColor(red, "Red"));
        picker->insertColor(QColor("green"), "Green"));
        picker->insertColor(QColor(0, 0, 255), "Blue"));
        picker->insertColor(white);

        connect(colors, SIGNAL(colorChanged(const QColor &)), SLOT(setCurrentColor(const QColor &)));
    }
    \endcode

    An alternative to adding colors manually is to initialize the grid
    with QColorDialog's standard colors using setStandardColors().

    \sa QColorDialog
*/

/*! \fn QtColorPicker::colorSet(const QColor &color)

    This signal is emitted when a color is selected. It is also emitted, if the color does not change.
    \a color is the new color.

    To obtain the color's name, use text().
*/

/*! \fn QtColorPicker::colorCleared()

    This signal is emitted when the QtColorPicker's color is cleared and no color is selected anymore.
*/

/*! \fn QtColorPicker::colorChanged(const QColor &color)

    This signal is emitted when the QtColorPicker's color is changed.
    \a color is the new color.

    To obtain the color's name, use text().
*/

/*
    This class represents each "color" or item in the color grid.
*/
class ColorPickerItem : public QFrame
{
    Q_OBJECT

public:
    ColorPickerItem(const QColor &color = Qt::white, const QString &text = QString(),
              QWidget *parent = nullptr);
    ~ColorPickerItem() override;

    QColor color() const;
    QString text() const;

    void setSelected(bool);
    bool isSelected() const;
Q_SIGNALS:
    void clicked();
    void selected();

public Q_SLOTS:
    void setColor(const QColor &color, const QString &text = QString());

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void paintEvent(QPaintEvent *e) override;

private:
    QColor c;
    QString t;
    bool sel;
};

/*
    This class represents the grid of colors.
*/
class ColorPickerGrid : public QWidget
{
    Q_OBJECT

public:
    ColorPickerGrid(int width, QWidget *parent = nullptr);
    ~ColorPickerGrid() override;

    void insertColor(const QColor &col, const QString &text, int index);

    ColorPickerItem *find(const QColor &col) const;
    ColorPickerItem *findSelected() const;
    void unselectAll() const;
    QColor color(int index) const;

Q_SIGNALS:
    void selected(const QColor &);

protected Q_SLOTS:
    void updateSelected();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void focusInEvent(QFocusEvent *e) override;

    void regenerateGrid();

private:
    QMap<int, QMap<int, QWidget *> > widgetAt;
    QList<ColorPickerItem *> items;
    QGridLayout *grid;

    int cols;
};

/*
    This class represents the popup widget of the color picker.
    It contains the color grid, a reset button and an optional
    custom colors button.
*/
class ColorPickerPopup : public QFrame
{
    Q_OBJECT

public:
    ColorPickerPopup(int width, bool withColorDialog,
               QWidget *parent = nullptr);
    ~ColorPickerPopup() override;

    void insertColor(const QColor &col, const QString &text, int index);
    void exec();

    ColorPickerItem *find(const QColor &col) const;
    QColor color(int index) const;

Q_SIGNALS:
    void selected(const QColor &);
    void cleared();
    void hid();

public Q_SLOTS:
    void getColorFromDialog();

protected:
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;

    void unselectColor();

private:
    ColorPickerGrid *grid;
    QPushButton *customColorsButton;
    QPushButton *resetButton;
    QEventLoop *eventLoop;
};

/*!
    Constructs a QtColorPicker widget. The popup will display a grid
    with \a cols columns, or if \a cols is -1, the number of columns
    will be calculated automatically.

    If \a enableColorDialog is true, the popup will also have a
    "Custom Colors" button that presents a QColorDialog when clicked.

    The default color can be set with \a defaultColor.

    After constructing a QtColorPicker, call insertColor() to add
    individual colors to the popup grid, or call setStandardColors()
    to add all the standard colors in one go.

    The \a parent argument is passed to QFrame's constructor.

    \sa QFrame
*/
QtColorPicker::QtColorPicker(QWidget *parent, QColor defaultColor1,
                 int cols, bool enableColorDialog)
    : QPushButton(parent), popup(nullptr), withColorDialog(enableColorDialog)
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setAutoDefault(false);
    setAutoFillBackground(true);
    setCheckable(true);

    // Set text
    setText(tr("Default"));

    // Create and set icon
    col = Qt::black;
    dirty = true;
    colSet = false;
    defaultColor = defaultColor1;

    // Create color grid popup and connect to it.
    popup = new ColorPickerPopup(cols, withColorDialog, this);
    connect(popup, &ColorPickerPopup::cleared, this, &QtColorPicker::clearCurrentColor);
    connect(popup, &ColorPickerPopup::selected, this, &QtColorPicker::setCurrentColor);
    connect(popup, &ColorPickerPopup::hid, this, &QtColorPicker::popupClosed);

    // Connect this push button's pressed() signal.
    connect(this, &QtColorPicker::toggled, this, &QtColorPicker::buttonPressed);
}

/*!
    Destructs the QtColorPicker.
*/
QtColorPicker::~QtColorPicker()
{
}

/*! \internal

    Pops up the color grid, and makes sure the status of
    QtColorPicker's button is right.
*/
void QtColorPicker::buttonPressed(bool toggled)
{
    if (!toggled)
        return;

    const QRect desktop = QApplication::activeWindow()->geometry();

    // Make sure the popup is inside the desktop.
    QPoint pos = mapToGlobal(rect().bottomLeft());
    if (pos.x() < desktop.left())
       pos.setX(desktop.left());
    if (pos.y() < desktop.top())
       pos.setY(desktop.top());

    if ((pos.x() + popup->sizeHint().width()) > desktop.right())
       pos.setX(desktop.right() - popup->sizeHint().width());
    if ((pos.y() + popup->sizeHint().height()) > desktop.bottom())
       pos.setY(desktop.bottom() - popup->sizeHint().height());
    popup->move(pos);

    ColorPickerItem *item = popup->find(col);
    if (colSet && item)
        item->setSelected(true);

    // Remove focus from this widget, preventing the focus rect
    // from showing when the popup is shown. Order an update to
    // make sure the focus rect is cleared.
    clearFocus();
    update();

    // Allow keyboard navigation as soon as the popup shows.
    popup->setFocus();

    // Execute the popup. The popup will enter the event loop.
    popup->show();
}

/*!
    \internal
*/
void QtColorPicker::paintEvent(QPaintEvent *e)
{
    if (dirty) {
        int iconSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
        QPixmap pix(iconSize, iconSize);
        pix.fill(palette().button().color());

        QPainter p(&pix);

        int w = pix.width();            // width of cell in pixels
        int h = pix.height();           // height of cell in pixels
        p.setPen(QPen(Qt::gray));
        if(colSet)
            p.setBrush(col);
        else
            p.setBrush(defaultColor);
        p.drawRect(2, 2, w - 5, h - 5);
        setIcon(QIcon(pix));

        dirty = false;
    }
    QPushButton::paintEvent(e);
}

/*! \internal

    Makes sure the button isn't pressed when the popup hides.
*/
void QtColorPicker::popupClosed()
{
    setChecked(false);
    setFocus();
}

/*!
    Returns the currently selected color.

    \sa text()
*/
QColor QtColorPicker::currentColor() const
{
    return col;
}

/*!
    Returns the color at position \a index.
*/
QColor QtColorPicker::color(int index) const
{
    return popup->color(index);
}

/*!
    Adds the 17 predefined colors from the Qt namespace.

    (The names given to the colors, "Black", "White", "Red", etc., are
    all translatable.)

    \sa insertColor()
*/
void QtColorPicker::setStandardColors()
{
    insertColor(Qt::black, tr("Black"));
    insertColor(Qt::white, tr("White"));
    insertColor(Qt::red, tr("Red"));
    insertColor(Qt::darkRed, tr("Dark red"));
    insertColor(Qt::green, tr("Green"));
    insertColor(Qt::darkGreen, tr("Dark green"));
    insertColor(Qt::blue, tr("Blue"));
    insertColor(Qt::darkBlue, tr("Dark blue"));
    insertColor(Qt::cyan, tr("Cyan"));
    insertColor(Qt::darkCyan, tr("Dark cyan"));
    insertColor(Qt::magenta, tr("Magenta"));
    insertColor(Qt::darkMagenta, tr("Dark magenta"));
    insertColor(Qt::yellow, tr("Yellow"));
    insertColor(Qt::darkYellow, tr("Dark yellow"));
    insertColor(Qt::gray, tr("Gray"));
    insertColor(Qt::darkGray, tr("Dark gray"));
    insertColor(Qt::lightGray, tr("Light gray"));
}

/*!

    Sets the default color to \a color.
*/
void QtColorPicker::setDefaultColor(QColor color)
{
    defaultColor = color;
}


/*!

    Resets the current color to the default color.
    This function emits the colorCleared() signal.
*/
void QtColorPicker::clearCurrentColor()
{
    colSet = false;
    setText(tr("Default"));

    dirty = true;

    popup->hide();
    repaint();

    Q_EMIT colorCleared();
}

/*!
    Makes \a color current. If \a color is not already in the color grid, it
    is inserted with the text "Custom".

    This function emits the colorSet() signal if the new color is valid and
    the colorChanged() signal if the new color is valid, and different from
    the old one.
*/
void QtColorPicker::setCurrentColor(const QColor &color)
{
    if (!color.isValid())
    return;

    if (colSet && col == color) {
        popup->hide();
        Q_EMIT colorSet(color);
        return;
    }

    ColorPickerItem *item = popup->find(color);
    if (!item) {
    insertColor(color, tr("Custom Color"));
    item = popup->find(color);
    }

    col = color;
    colSet = true;
    setText(item->text());

    dirty = true;

    popup->hide();
    repaint();

    Q_EMIT colorChanged(color);
    Q_EMIT colorSet(color);
}

/*!
    Adds the color \a color with the name \a text to the color grid,
    at position \a index. If index is -1, the color is assigned
    automatically assigned a position, starting from left to right,
    top to bottom.
*/
void QtColorPicker::insertColor(const QColor &color, const QString &text, int index)
{
    popup->insertColor(color, text, index);
}

/*! \property QtColorPicker::colorDialog
    \brief Whether the ellipsis "..." (more) button is available.

    If this property is set to true, the color grid popup will include
    a "More" button (signified by an ellipsis, "...") which pops up a
    QColorDialog when clicked. The user will then be able to select
    any custom color they like.
*/
void QtColorPicker::setColorDialogEnabled(bool enabled)
{
    withColorDialog = enabled;
}
bool QtColorPicker::colorDialogEnabled() const
{
    return withColorDialog;
}

/*! \internal

    Constructs the popup widget.
*/
ColorPickerPopup::ColorPickerPopup(int width, bool withColorDialog,
                       QWidget *parent)
    : QFrame(parent, Qt::Popup)
{
    setFrameStyle(QFrame::StyledPanel);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setProperty("class", "popup");

    grid = new ColorPickerGrid(width, this);
    connect(grid, &ColorPickerGrid::selected, this, &ColorPickerPopup::selected);

    resetButton = new QPushButton(tr("Reset"));
    connect(resetButton, &QPushButton::clicked, this, &ColorPickerPopup::unselectColor);

    if(withColorDialog){
        customColorsButton = new QPushButton(tr("Custom Colors"));
        connect(customColorsButton, &QPushButton::clicked, this, &ColorPickerPopup::getColorFromDialog);
    }else {
        customColorsButton = nullptr;
    }

    eventLoop = nullptr;

    QLayout *layout = new QVBoxLayout(this);
    layout->addWidget(resetButton);
    layout->addWidget(grid);
    if(withColorDialog)
        layout->addWidget(customColorsButton);

    setTabOrder(resetButton, grid);
    if(withColorDialog)
    setTabOrder(grid, customColorsButton);
}

/*! \internal

    Destructs the popup widget.
*/
ColorPickerPopup::~ColorPickerPopup()
{
    if (eventLoop)
        eventLoop->exit();
}

/*! \internal

    Adds \a item to the grid. The items are added from top-left to
    bottom-right.
*/
void ColorPickerPopup::insertColor(const QColor & col, const QString & text, int index)
{
    grid->insertColor(col, text, index);
}

/*! \internal

*/
void ColorPickerPopup::exec()
{
    grid->show();

    QEventLoop e;
    eventLoop = &e;
    (void) e.exec();
    eventLoop = nullptr;
}

/*! \internal

    If there is an item whole color is equal to \a col, returns a
    pointer to this item; otherwise returns 0.
*/
ColorPickerItem * ColorPickerPopup::find(const QColor & col) const
{
    return grid->find(col);
}

/*! \internal

    Returns the color at \a index in the grid.
*/
QColor ColorPickerPopup::color(int index) const
{
    return grid->color(index);
}

/*! \internal

    Copies the color dialog's currently selected item and emits
    selected().
*/
void ColorPickerPopup::getColorFromDialog()
{
    //bool ok;
    //QRgb rgb = QColorDialog::getRgba(lastSel.rgba(), &ok, parentWidget());
    QColor col = Qt::white;
    if(ColorPickerItem *selected = grid->findSelected())
        col = selected->color();

    if (Gui::DialogOptions::dontUseNativeColorDialog()){
        col = QColorDialog::getColor(col, parentWidget(), QString(),
            QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    } else {
        col = QColorDialog::getColor(col, parentWidget(), QString(),
            QColorDialog::ShowAlphaChannel);
    }
    if (!col.isValid())
    return;

    //QColor col = QColor::fromRgba(rgb);
    grid->insertColor(col, tr("Custom Color"), -1);
    grid->unselectAll();
    Q_EMIT selected(col);
}

/*! \internal

    Sets focus on the popup to enable keyboard navigation. Draws
    focusRect and selection rect.
*/
void ColorPickerPopup::showEvent(QShowEvent *)
{
    if (grid->findSelected()){
        grid->setFocus();
    }else {
        resetButton->setFocus();
    }
}

/*! \internal

*/
void ColorPickerPopup::hideEvent(QHideEvent *e)
{
    if (eventLoop)
        eventLoop->exit();

    setFocus();

    Q_EMIT hid();
    QFrame::hideEvent(e);
}

/*! \internal

    unselect all colors and emits cleared().
*/
void ColorPickerPopup::unselectColor()
{
    grid->unselectAll();
    Q_EMIT cleared();
}

/*! \internal

    Constructs the color grid widget.
*/
ColorPickerGrid::ColorPickerGrid(int width, QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    setFocusPolicy(Qt::StrongFocus);
    cols = width;

    grid = nullptr;
    regenerateGrid();
}

/*! \internal

    Destructs the popup widget.
*/
ColorPickerGrid::~ColorPickerGrid()
{

}

/*! \internal

    Adds \a item to the grid. The items are added from top-left to
    bottom-right.
*/
void ColorPickerGrid::insertColor(const QColor &col, const QString &text, int index)
{
    // Don't add colors that we have already.
    if (find(col)) {
        return;
    }

    ColorPickerItem *item = new ColorPickerItem(col, text, this);
    connect(item, &ColorPickerItem::selected, this, &ColorPickerGrid::updateSelected);
    item->installEventFilter(this);

    if (index == -1)
        index = items.count();

    items.insert((unsigned int)index, item);
    regenerateGrid();

    update();
}

/*! \internal

    If there is an item whole color is equal to \a col, returns a
    pointer to this item; otherwise returns 0.
*/
ColorPickerItem *ColorPickerGrid::find(const QColor &col) const
{
    for (int i = 0; i < items.size(); ++i) {
    if (items.at(i) && items.at(i)->color() == col)
        return items.at(i);
    }

    return nullptr;
}

/*! \internal

    If there is an item whole color is equal to \a col, returns a
    pointer to this item; otherwise returns 0.
*/
ColorPickerItem * ColorPickerGrid::findSelected() const
{
    for (int i = 0; i < items.size(); ++i) {
    if (items.at(i) && items.at(i)->isSelected())
        return items.at(i);
    }

    return nullptr;
}

/*! \internal

    Unselects all colors in the grid
*/
void ColorPickerGrid::unselectAll() const
{
    for (int i = 0; i < items.size(); ++i) {
    if (items.at(i))
        items.at(i)->setSelected(false);
    }
}

/*! \internal

    Returns the color at \a index in the grid.
*/
QColor ColorPickerGrid::color(int index) const
{
    if (index < 0 || index > (int) items.count() - 1)
        return QColor();

    ColorPickerGrid *that = (ColorPickerGrid *)this;
    return that->items.at(index)->color();
}

/*! \internal

*/
void ColorPickerGrid::updateSelected()
{
    unselectAll();

    if (sender() && sender()->inherits("ColorPickerItem")) {
    ColorPickerItem *item = (ColorPickerItem *)sender();
    item->setSelected(true);
    Q_EMIT selected(item->color());
    }

}

/*! \internal

    Controls keyboard navigation and selection on the color grid.
*/
bool ColorPickerGrid::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    int curRow = 0;
    int curCol = 0;

    bool foundFocus = false;
    for (int j = 0; !foundFocus && j < grid->rowCount(); ++j) {
    for (int i = 0; !foundFocus && i < grid->columnCount(); ++i) {
        if (widgetAt[j][i] && widgetAt[j][i]->hasFocus()) {
        curRow = j;
        curCol = i;
        foundFocus = true;
        break;
        }
    }
    }

    switch (keyEvent->key()) {
    case Qt::Key_Tab: {
        QWidget *nextWidget = this->nextInFocusChain();
        if (nextWidget) {
            nextWidget->setFocus();
            return true;
        }
        }
        break;
    case Qt::Key_Backtab: {
        QWidget *prevWidget = this->previousInFocusChain();
        if (prevWidget) {
            prevWidget->setFocus();
            return true;
        }
        }
        break;
    case Qt::Key_Left:
        if (curCol > 0) --curCol;
        else if (curRow > 0) { --curRow; curCol = grid->columnCount() - 1; }
        break;
    case Qt::Key_Right:
        if (curCol < grid->columnCount() - 1 && widgetAt[curRow][curCol + 1]) ++curCol;
        else if (curRow < grid->rowCount() - 1) { ++curRow; curCol = 0; }
        break;
    case Qt::Key_Up:
        if (curRow > 0) --curRow;
        else curCol = 0;
        break;
    case Qt::Key_Down:
        if (curRow < grid->rowCount() - 1) {
        QWidget *w = widgetAt[curRow + 1][curCol];
        if (w) {
            ++curRow;
        } else for (int i = 1; i < grid->columnCount(); ++i) {
            if (!widgetAt[curRow + 1][i]) {
            curCol = i - 1;
            ++curRow;
            break;
            }
        }
        }
        break;
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        unselectAll();
        QWidget *w = widgetAt[curRow][curCol];
        if (w && w->inherits("ColorPickerItem")) {
            ColorPickerItem *wi = reinterpret_cast<ColorPickerItem *>(w);
            wi->setSelected(true);

            Q_EMIT selected(wi->color());
        }
    }
    break;
    default:
        return false;
        break;
    }

    widgetAt[curRow][curCol]->setFocus();
    return true;
    }
    return QWidget::eventFilter(obj, event);
}

/*! \internal

    Sets the focus to the selected item in the grid.
*/
void ColorPickerGrid::focusInEvent(QFocusEvent *e)
{
    QWidget::focusInEvent(e);

    bool foundSelected = false;
    for (int i = 0; i < grid->columnCount(); ++i) {
        for (int j = 0; j < grid->rowCount(); ++j) {
            QWidget* w = widgetAt[j][i];
            if (w && w->inherits("ColorPickerItem")) {
                if (((ColorPickerItem*)w)->isSelected()) {
                    w->setFocus();
                    foundSelected = true;
                    break;
                }
            }
        }
    }

    if (!foundSelected) {
    if (items.isEmpty())
        setFocus();
    else
        widgetAt[0][0]->setFocus();
    }
}

/*!

*/
void ColorPickerGrid::regenerateGrid()
{
    widgetAt.clear();

    int columns = cols;
    if (columns == -1)
    columns = (int) ceil(sqrt((float) items.count()));

    // When the number of columns grows, the number of rows will
    // fall. There's no way to shrink a grid, so we create a new
    // one.
    if (grid) delete grid;
    grid = new QGridLayout(this);
    grid->setContentsMargins(1, 1, 1, 1);
    grid->setSpacing(0);

    int ccol = 0, crow = 0;
    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i)) {
            widgetAt[crow][ccol] = items.at(i);
            grid->addWidget(items.at(i), crow, ccol++);
            if (ccol == columns) {
                ++crow;
                ccol = 0;
            }
        }
    }

    updateGeometry();
}

/*!
    Constructs a ColorPickerItem whose color is set to \a color, and
    whose name is set to \a text.
*/
ColorPickerItem::ColorPickerItem(const QColor &color, const QString &text,
                     QWidget *parent)
    : QFrame(parent), c(color), t(text), sel(false)
{
    setToolTip(t);
    setFixedWidth(24);
    setFixedHeight(21);
}

/*!
    Destructs a ColorPickerItem.
 */
ColorPickerItem::~ColorPickerItem()
{
}

/*!
    Returns the item's color.

    \sa text()
*/
QColor ColorPickerItem::color() const
{
    return c;
}

/*!
    Returns the item's text.

    \sa color()
*/
QString ColorPickerItem::text() const
{
    return t;
}

/*!

*/
bool ColorPickerItem::isSelected() const
{
    return sel;
}

/*!

*/
void ColorPickerItem::setSelected(bool selected)
{
    sel = selected;
    update();
}

/*!
    Sets the item's color to \a color, and its name to \a text.
*/
void ColorPickerItem::setColor(const QColor &color, const QString &text)
{
    c = color;
    t = text;
    setToolTip(t);
    update();
}

/*!

*/
void ColorPickerItem::mouseMoveEvent(QMouseEvent *)
{
    setFocus();
    update();
}

/*!

*/
void ColorPickerItem::mouseReleaseEvent(QMouseEvent *)
{
    sel = true;
    Q_EMIT selected();
}

/*!

*/
void ColorPickerItem::mousePressEvent(QMouseEvent *)
{
    setFocus();
    update();
}

/*!

*/
void ColorPickerItem::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    int w = width();            // width of cell in pixels
    int h = height();           // height of cell in pixels

    p.setPen( QPen( Qt::gray, 0, Qt::SolidLine ) );

    if (sel)
    p.drawRect(1, 1, w - 3, h - 3);

    p.setPen( QPen( Qt::black, 0, Qt::SolidLine ) );
    p.drawRect(3, 3, w - 7, h - 7);
    p.fillRect(QRect(4, 4, w - 8, h - 8), QBrush(c));

    if (hasFocus())
    p.drawRect(0, 0, w - 1, h - 1);
}

// clang-format on
#include "moc_qtcolorpicker.cpp"
#include <moc_qtcolorpicker-internal.cpp>
