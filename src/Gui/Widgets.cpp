/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QAction>
# include <QColorDialog>
# include <QDesktopWidget>
# include <QDialogButtonBox>
# include <QDrag>
# include <QEventLoop>
# include <QKeyEvent>
# include <QMessageBox>
# include <QMimeData>
# include <QPainter>
# include <QPlainTextEdit>
# include <QStylePainter>
# include <QTextBlock>
# include <QTimer>
# include <QToolTip>
#endif

#include <Base/Exception.h>
#include <Base/Interpreter.h>

#include "Widgets.h"
#include "Application.h"
#include "Action.h"
#include "PrefWidgets.h"
#include "BitmapFactory.h"

using namespace Gui;

/**
 * Constructs an empty command view with parent \a parent.
 */
CommandIconView::CommandIconView ( QWidget * parent )
  : QListWidget(parent)
{
    connect(this, SIGNAL (currentItemChanged(QListWidgetItem *, QListWidgetItem *)), 
            this, SLOT (onSelectionChanged(QListWidgetItem *, QListWidgetItem *)) );
}

/**
 * Destroys the icon view and deletes all items.
 */
CommandIconView::~CommandIconView ()
{
}

/**
 * Stores the name of the selected commands for drag and drop. 
 */
void CommandIconView::startDrag (Qt::DropActions supportedActions)
{
    Q_UNUSED(supportedActions);
    QList<QListWidgetItem*> items = selectedItems();
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);

    QPixmap pixmap;
    dataStream << items.count();
    for (QList<QListWidgetItem*>::ConstIterator it = items.begin(); it != items.end(); ++it) {
        if (it == items.begin())
            pixmap = ((*it)->data(Qt::UserRole)).value<QPixmap>();
        dataStream << (*it)->text();
    }

    QMimeData *mimeData = new QMimeData;
    mimeData->setData(QString::fromLatin1("text/x-action-items"), itemData);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
    drag->setPixmap(pixmap);
    drag->start(Qt::MoveAction);
}

/**
 * This slot is called when a new item becomes current. \a item is the new current item 
 * (or 0 if no item is now current). This slot emits the emitSelectionChanged()
 * signal for its part.
 */
void CommandIconView::onSelectionChanged(QListWidgetItem * item, QListWidgetItem *)
{
    if (item)
        emitSelectionChanged(item->toolTip());
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::ActionSelector */

ActionSelector::ActionSelector(QWidget* parent)
  : QWidget(parent)
{
    addButton = new QPushButton(this);
    addButton->setObjectName(QLatin1String("addButton"));
    addButton->setMinimumSize(QSize(30, 30));
    addButton->setIcon(BitmapFactory().pixmap("button_right"));
    gridLayout = new QGridLayout(this);
    gridLayout->addWidget(addButton, 1, 1, 1, 1);

    spacerItem = new QSpacerItem(33, 57, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(spacerItem, 5, 1, 1, 1);
    spacerItem1 = new QSpacerItem(33, 58, QSizePolicy::Minimum, QSizePolicy::Expanding);
    gridLayout->addItem(spacerItem1, 0, 1, 1, 1);

    removeButton = new QPushButton(this);
    removeButton->setObjectName(QLatin1String("removeButton"));
    removeButton->setMinimumSize(QSize(30, 30));
    removeButton->setIcon(BitmapFactory().pixmap("button_left"));
    removeButton->setAutoDefault(true);
    removeButton->setDefault(false);

    gridLayout->addWidget(removeButton, 2, 1, 1, 1);

    upButton = new QPushButton(this);
    upButton->setObjectName(QLatin1String("upButton"));
    upButton->setMinimumSize(QSize(30, 30));
    upButton->setIcon(BitmapFactory().pixmap("button_up"));

    gridLayout->addWidget(upButton, 3, 1, 1, 1);

    downButton = new QPushButton(this);
    downButton->setObjectName(QLatin1String("downButton"));
    downButton->setMinimumSize(QSize(30, 30));
    downButton->setIcon(BitmapFactory().pixmap("button_down"));
    downButton->setAutoDefault(true);

    gridLayout->addWidget(downButton, 4, 1, 1, 1);

    vboxLayout = new QVBoxLayout();
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    labelAvailable = new QLabel(this);
    vboxLayout->addWidget(labelAvailable);

    availableWidget = new QTreeWidget(this);
    availableWidget->setObjectName(QLatin1String("availableTreeWidget"));
    availableWidget->setRootIsDecorated(false);
    availableWidget->setHeaderLabels(QStringList() << QString());
    availableWidget->header()->hide();
    vboxLayout->addWidget(availableWidget);

    gridLayout->addLayout(vboxLayout, 0, 0, 6, 1);

    vboxLayout1 = new QVBoxLayout();
    vboxLayout1->setContentsMargins(0, 0, 0, 0);
    labelSelected = new QLabel(this);
    vboxLayout1->addWidget(labelSelected);

    selectedWidget = new QTreeWidget(this);
    selectedWidget->setObjectName(QLatin1String("selectedTreeWidget"));
    selectedWidget->setRootIsDecorated(false);
    selectedWidget->setHeaderLabels(QStringList() << QString());
    selectedWidget->header()->hide();
    vboxLayout1->addWidget(selectedWidget);

    gridLayout->addLayout(vboxLayout1, 0, 2, 6, 1);

    addButton->setText(QString());
    removeButton->setText(QString());
    upButton->setText(QString());
    downButton->setText(QString());

    connect(addButton, SIGNAL(clicked()),
            this, SLOT(on_addButton_clicked()) );
    connect(removeButton, SIGNAL(clicked()),
            this, SLOT(on_removeButton_clicked()) );
    connect(upButton, SIGNAL(clicked()),
            this, SLOT(on_upButton_clicked()) );
    connect(downButton, SIGNAL(clicked()),
            this, SLOT(on_downButton_clicked()) );
    connect(availableWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)) );
    connect(selectedWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(onItemDoubleClicked(QTreeWidgetItem*,int)) );
    connect(availableWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem *)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem *,QTreeWidgetItem *)) );
    connect(selectedWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem *)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem *,QTreeWidgetItem *)) );
    retranslateUi();
    setButtonsEnabled();
}

ActionSelector::~ActionSelector()
{
}

void ActionSelector::setSelectedLabel(const QString& label)
{
    labelSelected->setText(label);
}

QString ActionSelector::selectedLabel() const
{
    return labelSelected->text();
}

void ActionSelector::setAvailableLabel(const QString& label)
{
    labelAvailable->setText(label);
}

QString ActionSelector::availableLabel() const
{
    return labelAvailable->text();
}


void ActionSelector::retranslateUi()
{
    labelAvailable->setText(QApplication::translate("Gui::ActionSelector", "Available:"));
    labelSelected->setText(QApplication::translate("Gui::ActionSelector", "Selected:"));
    addButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Add"));
    removeButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Remove"));
    upButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Move up"));
    downButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Move down"));
}

void ActionSelector::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}

void ActionSelector::keyPressEvent(QKeyEvent* event)
{
    if ((event->modifiers() & Qt::ControlModifier)) {
        switch (event->key())
        {
        case Qt::Key_Right:
            on_addButton_clicked();
            break;
        case Qt::Key_Left:
            on_removeButton_clicked();
            break;
        case Qt::Key_Up:
            on_upButton_clicked();
            break;
        case Qt::Key_Down:
            on_downButton_clicked();
            break;
        default:
            event->ignore();
            return;
        }
    }
}

void ActionSelector::setButtonsEnabled()
{
    addButton->setEnabled(availableWidget->indexOfTopLevelItem(availableWidget->currentItem()) > -1);
    removeButton->setEnabled(selectedWidget->indexOfTopLevelItem(selectedWidget->currentItem()) > -1);
    upButton->setEnabled(selectedWidget->indexOfTopLevelItem(selectedWidget->currentItem()) > 0);
    downButton->setEnabled(selectedWidget->indexOfTopLevelItem(selectedWidget->currentItem()) > -1 &&
                           selectedWidget->indexOfTopLevelItem(selectedWidget->currentItem()) < selectedWidget->topLevelItemCount() - 1);
}

void ActionSelector::onCurrentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)
{
    setButtonsEnabled();
}

void ActionSelector::onItemDoubleClicked(QTreeWidgetItem * item, int column)
{
    Q_UNUSED(column);
    QTreeWidget* treeWidget = item->treeWidget();
    if (treeWidget == availableWidget) {
        int index = availableWidget->indexOfTopLevelItem(item);
        item = availableWidget->takeTopLevelItem(index);
        availableWidget->setCurrentItem(0);
        selectedWidget->addTopLevelItem(item);
        selectedWidget->setCurrentItem(item);
    }
    else if (treeWidget == selectedWidget) {
        int index = selectedWidget->indexOfTopLevelItem(item);
        item = selectedWidget->takeTopLevelItem(index);
        selectedWidget->setCurrentItem(0);
        availableWidget->addTopLevelItem(item);
        availableWidget->setCurrentItem(item);
    }
}

void ActionSelector::on_addButton_clicked()
{
    QTreeWidgetItem* item = availableWidget->currentItem();
    if (item) {
        int index = availableWidget->indexOfTopLevelItem(item);
        item = availableWidget->takeTopLevelItem(index);
        availableWidget->setCurrentItem(0);
        selectedWidget->addTopLevelItem(item);
        selectedWidget->setCurrentItem(item);
    }
}

void ActionSelector::on_removeButton_clicked()
{
    QTreeWidgetItem* item = selectedWidget->currentItem();
    if (item) {
        int index = selectedWidget->indexOfTopLevelItem(item);
        item = selectedWidget->takeTopLevelItem(index);
        selectedWidget->setCurrentItem(0);
        availableWidget->addTopLevelItem(item);
        availableWidget->setCurrentItem(item);
    }
}

void ActionSelector::on_upButton_clicked()
{
    QTreeWidgetItem* item = selectedWidget->currentItem();
    if (item && selectedWidget->isItemSelected(item)) {
        int index = selectedWidget->indexOfTopLevelItem(item);
        if (index > 0) {
            selectedWidget->takeTopLevelItem(index);
            selectedWidget->insertTopLevelItem(index-1, item);
            selectedWidget->setCurrentItem(item);
        }
    }
}

void ActionSelector::on_downButton_clicked()
{
    QTreeWidgetItem* item = selectedWidget->currentItem();
    if (item && selectedWidget->isItemSelected(item)) {
        int index = selectedWidget->indexOfTopLevelItem(item);
        if (index < selectedWidget->topLevelItemCount()-1) {
            selectedWidget->takeTopLevelItem(index);
            selectedWidget->insertTopLevelItem(index+1, item);
            selectedWidget->setCurrentItem(item);
        }
    }
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::AccelLineEdit */

/**
 * Constructs a line edit with no text.
 * The \a parent and \a name arguments are sent to the QLineEdit constructor.
 */
AccelLineEdit::AccelLineEdit ( QWidget * parent )
  : QLineEdit(parent)
{
    noneStr = tr("none");
    setText(noneStr);
    keyPressedCount = 0;
}

bool AccelLineEdit::isNone() const
{
    QString t = text();
    return t.isEmpty() || t == noneStr;
}

/**
 * Checks which keys are pressed and show it as text.
 */
void AccelLineEdit::keyPressEvent ( QKeyEvent * e)
{
    QString txtLine = text();

    int key = e->key();
    Qt::KeyboardModifiers state = e->modifiers();

    // Backspace clears the shortcut
    // If a modifier is pressed without any other key, return.
    // AltGr is not a modifier but doesn't have a QtSring representation.
    switch(key) {
    case Qt::Key_Backspace:
        if (state == Qt::NoModifier) {
            keyPressedCount = 0;
            setText(noneStr);
        }
    case Qt::Key_Control:
    case Qt::Key_Shift:
    case Qt::Key_Alt:
    case Qt::Key_Meta:
    case Qt::Key_AltGr:
        return;
    default:
        break;
    }

    // 4 keys are allowed for QShortcut
    switch(keyPressedCount) {
    case 4:
        keyPressedCount = 0;
        txtLine.clear();
        break;
    case 0:
        txtLine.clear();
        break;
    default:
        txtLine += QString::fromLatin1(",");
        break;
    }

    // Handles modifiers applying a mask.
    if ((state & Qt::ControlModifier) == Qt::ControlModifier) {
        QKeySequence ks(Qt::CTRL);
        txtLine += ks.toString(QKeySequence::NativeText);
    }
    if ((state & Qt::AltModifier) == Qt::AltModifier) {
        QKeySequence ks(Qt::ALT);
        txtLine += ks.toString(QKeySequence::NativeText);
    }
    if ((state & Qt::ShiftModifier) == Qt::ShiftModifier) {
        QKeySequence ks(Qt::SHIFT);
        txtLine += ks.toString(QKeySequence::NativeText);
    }
    if ((state & Qt::MetaModifier) == Qt::MetaModifier) {
        QKeySequence ks(Qt::META);
        txtLine += ks.toString(QKeySequence::NativeText);
    }

    // Handles normal keys
    QKeySequence ks(key);
    txtLine += ks.toString(QKeySequence::NativeText);

    setText(txtLine);
    keyPressedCount++;
}

// ------------------------------------------------------------------------------

#if QT_VERSION >= 0x050200
ClearLineEdit::ClearLineEdit (QWidget * parent)
  : QLineEdit(parent)
{
    clearAction = this->addAction(QIcon(QString::fromLatin1(":/icons/edit-cleartext.svg")),
                                        QLineEdit::TrailingPosition);
    connect(clearAction, SIGNAL(triggered()), this, SLOT(clear()));
    connect(this, SIGNAL(textChanged(const QString&)),
            this, SLOT(updateClearButton(const QString&)));
}

void ClearLineEdit::resizeEvent(QResizeEvent *e)
{
    QLineEdit::resizeEvent(e);
}

void ClearLineEdit::updateClearButton(const QString& text)
{
    clearAction->setVisible(!text.isEmpty());
}
#else
ClearLineEdit::ClearLineEdit (QWidget * parent)
  : QLineEdit(parent)
{
    clearButton = new QToolButton(this);
    QPixmap pixmap(BitmapFactory().pixmapFromSvg(":/icons/edit-cleartext.svg", QSize(18, 18)));
    clearButton->setIcon(QIcon(pixmap));
    clearButton->setIconSize(pixmap.size());
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->setStyleSheet(QString::fromLatin1("QToolButton { border: none; padding: 0px; }"));
    clearButton->hide();
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    connect(this, SIGNAL(textChanged(const QString&)),
            this, SLOT(updateClearButton(const QString&)));
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(QString::fromLatin1("QLineEdit { padding-right: %1px; } ")
                  .arg(clearButton->sizeHint().width() + frameWidth + 1));
    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), clearButton->sizeHint().height() + frameWidth * 2 + 2),
                   msz.height());
}

void ClearLineEdit::resizeEvent(QResizeEvent *)
{
    QSize sz = clearButton->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    clearButton->move(rect().right() - frameWidth - sz.width(),
                      (rect().bottom() + 1 - sz.height())/2);
}

void ClearLineEdit::updateClearButton(const QString& text)
{
    clearButton->setVisible(!text.isEmpty());
}
#endif

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::CheckListDialog */

/**
 *  Constructs a CheckListDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
CheckListDialog::CheckListDialog( QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
    ui.setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
CheckListDialog::~CheckListDialog()
{
    // no need to delete child widgets, Qt does it all for us
}

/**
 * Sets the items to the dialog's list view. By default all items are checkable..
 */
void CheckListDialog::setCheckableItems( const QStringList& items )
{
    for ( QStringList::ConstIterator it = items.begin(); it != items.end(); ++it ) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui.treeWidget);
        item->setText(0, *it);
        item->setCheckState(0, Qt::Unchecked);
    }
}

/**
 * Sets the items to the dialog's list view. If the boolean type of a CheckListItem
 * is set to false the item is not checkable any more.
 */
void CheckListDialog::setCheckableItems( const QList<CheckListItem>& items )
{
    for ( QList<CheckListItem>::ConstIterator it = items.begin(); it != items.end(); ++it ) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui.treeWidget);
        item->setText(0, (*it).first);
        item->setCheckState(0, ( (*it).second ? Qt::Checked : Qt::Unchecked));
    }
}

/**
 * Returns a list of the check items.
 */
QStringList CheckListDialog::getCheckedItems() const
{
    return checked;
}

/**
 * Collects all checked items to be able to return them by call \ref getCheckedItems().
 */
void CheckListDialog::accept ()
{
    QTreeWidgetItemIterator it(ui.treeWidget, QTreeWidgetItemIterator::Checked);
    while (*it) {
        checked.push_back((*it)->text(0));
        ++it;
    }

    QDialog::accept();
}

// ------------------------------------------------------------------------------

namespace Gui {
struct ColorButtonP
{
    QColor old, col;
    QPointer<QColorDialog> cd;
    bool allowChange;
    bool autoChange;
    bool drawFrame;
    bool modal;
    bool dirty;

    ColorButtonP()
        : cd(0)
        , allowChange(true)
        , autoChange(false)
        , drawFrame(true)
        , modal(true)
        , dirty(true)
    {
    }
};
}

/**
 * Constructs a colored button called \a name with parent \a parent.
 */
ColorButton::ColorButton(QWidget* parent)
    : QPushButton(parent)
{
    d = new ColorButtonP();
    d->col = palette().color(QPalette::Active,QPalette::Midlight);
    connect(this, SIGNAL(clicked()), SLOT(onChooseColor()));

#if 1
    int e = style()->pixelMetric(QStyle::PM_ButtonIconSize);
    setIconSize(QSize(2*e, e));
#endif
}

/**
 * Destroys the button.
 */
ColorButton::~ColorButton()
{
    delete d;
}

/** 
 * Sets the color \a c to the button. 
 */
void ColorButton::setColor(const QColor& c)
{
    d->col = c;
    d->dirty = true;
    update();
}

/** 
 * Returns the current color of the button.
 */
QColor ColorButton::color() const
{
    return d->col;
}

void ColorButton::setAllowChangeColor(bool ok)
{
    d->allowChange = ok;
}

bool ColorButton::allowChangeColor() const
{
    return d->allowChange;
}

void ColorButton::setDrawFrame(bool ok)
{
    d->drawFrame = ok;
}

bool ColorButton::drawFrame() const
{
    return d->drawFrame;
}

void ColorButton::setModal(bool b)
{
    d->modal = b;
}

bool ColorButton::isModal() const
{
    return d->modal;
}

void ColorButton::setAutoChangeColor(bool on)
{
    d->autoChange = on;
}

bool ColorButton::autoChangeColor() const
{
    return d->autoChange;
}

/**
 * Draws the button label.
 */
void ColorButton::paintEvent (QPaintEvent * e)
{
#if 0
    // first paint the complete button
    QPushButton::paintEvent(e);

    // repaint the rectangle area
    QPalette::ColorGroup group = isEnabled() ? hasFocus() ? QPalette::Active : QPalette::Inactive : QPalette::Disabled;
    QColor pen = palette().color(group,QPalette::ButtonText);
    {
        QPainter paint(this);
        paint.setPen(pen);

        if (d->drawFrame) {
            paint.setBrush(QBrush(d->col));
            paint.drawRect(5, 5, width()-10, height()-10);
        }
        else {
            paint.fillRect(5, 5, width()-10, height()-10, QBrush(d->col));
        }
    }

    // overpaint the rectangle to paint icon and text 
    QStyleOptionButton opt;
    opt.init(this);
    opt.text = text();
    opt.icon = icon();
    opt.iconSize = iconSize();

    QStylePainter p(this);
    p.drawControl(QStyle::CE_PushButtonLabel, opt);
#else
    if (d->dirty) {
        QSize isize = iconSize();
        QPixmap pix(isize);
        pix.fill(palette().button().color());

        QPainter p(&pix);

        int w = pix.width();
        int h = pix.height();
        p.setPen(QPen(Qt::gray));
        if (d->drawFrame) {
            p.setBrush(d->col);
            p.drawRect(2, 2, w - 5, h - 5);
        }
        else {
            p.fillRect(0, 0, w, h, QBrush(d->col));
        }
        setIcon(QIcon(pix));

        d->dirty = false;
    }

    QPushButton::paintEvent(e);
#endif
}

/**
 * Opens a QColorDialog to set a new color.
 */
void ColorButton::onChooseColor()
{
    if (!d->allowChange)
        return;
#if QT_VERSION >= 0x040500
    if (d->modal) {
#endif
        QColor currentColor = d->col;
        QColorDialog cd(d->col, this);
#if QT_VERSION >= 0x050000
        cd.setOptions(QColorDialog::DontUseNativeDialog);
#endif

        if (d->autoChange) {
            connect(&cd, SIGNAL(currentColorChanged(const QColor &)),
                    this, SLOT(onColorChosen(const QColor&)));
        }

        if (cd.exec() == QDialog::Accepted) {
            QColor c = cd.selectedColor();
            if (c.isValid()) {
                setColor(c);
                changed();
            }
        }
        else if (d->autoChange) {
            setColor(currentColor);
            changed();
        }
#if QT_VERSION >= 0x040500
    }
    else {
        if (d->cd.isNull()) {
            d->old = d->col;
            d->cd = new QColorDialog(d->col, this);
#if QT_VERSION >= 0x050000
            d->cd->setOptions(QColorDialog::DontUseNativeDialog);
#endif
            d->cd->setAttribute(Qt::WA_DeleteOnClose);
            connect(d->cd, SIGNAL(rejected()),
                    this, SLOT(onRejected()));
            connect(d->cd, SIGNAL(currentColorChanged(const QColor &)),
                    this, SLOT(onColorChosen(const QColor&)));
        }
        d->cd->show();
    }
#endif
}

void ColorButton::onColorChosen(const QColor& c)
{
    setColor(c);
    changed();
}

void ColorButton::onRejected()
{
    setColor(d->old);
    changed();
}

// ------------------------------------------------------------------------------

UrlLabel::UrlLabel(QWidget * parent, Qt::WindowFlags f)
  : QLabel(parent, f)
{
    _url = QString::fromLatin1("http://localhost");
    setToolTip(this->_url);
}

UrlLabel::~UrlLabel()
{
}

void UrlLabel::enterEvent ( QEvent * )
{
    setCursor(Qt::PointingHandCursor);
}

void UrlLabel::leaveEvent ( QEvent * )
{
    setCursor(Qt::ArrowCursor);
}

void UrlLabel::mouseReleaseEvent (QMouseEvent *)
{
    // The webbrowser Python module allows to start the system browser in an OS-independent way
    Base::PyGILStateLocker lock;
    PyObject* module = PyImport_ImportModule("webbrowser");
    if (module) {
        // get the methods dictionary and search for the 'open' method
        PyObject* dict = PyModule_GetDict(module);
        PyObject* func = PyDict_GetItemString(dict, "open");
        if (func) {
            PyObject* args = Py_BuildValue("(s)", (const char*)this->_url.toLatin1());
            PyObject* result = PyEval_CallObject(func,args);
            // decrement the args and module reference
            Py_XDECREF(result);
            Py_DECREF(args);
            Py_DECREF(module);
        }
    }
}

QString UrlLabel::url() const
{
    return this->_url;
}

void UrlLabel::setUrl(const QString& u)
{
    this->_url = u;
    setToolTip(this->_url);
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::LabelButton */

/**
 * Constructs a file chooser called \a name with the parent \a parent.
 */
LabelButton::LabelButton (QWidget * parent)
  : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(1);

    label = new QLabel(this);
    label->setAutoFillBackground(true);
    layout->addWidget(label);

    button = new QPushButton(QLatin1String("..."), this);
#if defined (Q_OS_MAC)
    button->setAttribute(Qt::WA_LayoutUsesWidgetRect); // layout size from QMacStyle was not correct
#endif
    layout->addWidget(button);

    connect(button, SIGNAL(clicked()), this, SLOT(browse()));
    connect(button, SIGNAL(clicked()), this, SIGNAL(buttonClicked()));
}

LabelButton::~LabelButton()
{
}

void LabelButton::resizeEvent(QResizeEvent* e)
{
    button->setFixedWidth(e->size().height());
    button->setFixedHeight(e->size().height());
}

QLabel *LabelButton::getLabel() const
{
    return label;
}

QPushButton *LabelButton::getButton() const
{
    return button;
}

QVariant LabelButton::value() const
{
    return _val;
}

void LabelButton::setValue(const QVariant& val)
{
    _val = val;
    showValue(_val);
    valueChanged(_val);
}

void LabelButton::showValue(const QVariant& data)
{
    label->setText(data.toString());
}

void LabelButton::browse()
{
}

// ----------------------------------------------------------------------

ToolTip* ToolTip::inst = 0;

ToolTip* ToolTip::instance()
{
    if (!inst)
        inst = new ToolTip();
    return inst;
}

ToolTip::ToolTip() : installed(false), hidden(true)
{
}

ToolTip::~ToolTip()
{
}

void ToolTip::installEventFilter()
{
    if (this->installed)
        return;
    qApp->installEventFilter(this);
    this->installed = true;
}

void ToolTip::removeEventFilter()
{
    if (!this->installed)
        return;
    qApp->removeEventFilter(this);
    this->installed = false;
}

void ToolTip::showText(const QPoint & pos, const QString & text, QWidget * w)
{
    ToolTip* tip = instance();
    if (!text.isEmpty()) {
        // install this object to filter timer events for the tooltip label
        tip->installEventFilter();
        tip->pos = pos;
        tip->text = text;
        tip->w = w;
        // show text with a short delay
        tip->tooltipTimer.start(80, tip);
    }
    else {
        // do immediately
        QToolTip::showText(pos, text, w);
    }
}

void ToolTip::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == tooltipTimer.timerId()) {
        QToolTip::showText(pos, text, w);
        tooltipTimer.stop();
        displayTime.restart();
    }
}

bool ToolTip::eventFilter(QObject* o, QEvent*e)
{
    // This is a trick to circumvent that the tooltip gets hidden immediately
    // after it gets visible. We just filter out all timer events to keep the
    // label visible.
    if (o->inherits("QLabel")) {
        QLabel* label = qobject_cast<QLabel*>(o);
        // Ignore the timer events to prevent from being closed
        if (label->windowFlags() & Qt::ToolTip) {
            if (e->type() == QEvent::Show) {
                this->hidden = false;
            }
            else if (e->type() == QEvent::Hide) {
                removeEventFilter();
                this->hidden = true;
            }
            else if (e->type() == QEvent::Timer && 
                !this->hidden && displayTime.elapsed() < 5000) {
                return true;
            }
        }
    }
    return false;
}

// ----------------------------------------------------------------------

StatusWidget::StatusWidget(QWidget* parent)
  : QWidget(parent, Qt::Dialog | Qt::FramelessWindowHint)
{
    //setWindowModality(Qt::ApplicationModal);
    label = new QLabel(this);
    label->setAlignment(Qt::AlignCenter);

    QGridLayout* gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(9);
    gridLayout->addWidget(label, 0, 0, 1, 1);
}

StatusWidget::~StatusWidget()
{
}

void StatusWidget::setStatusText(const QString& s)
{
    label->setText(s);
}

void StatusWidget::showText(int ms)
{
    show();
    QTimer timer;
    QEventLoop loop;
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    timer.start(ms);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    hide();
}

QSize StatusWidget::sizeHint () const
{
    return QSize(250,100);
}

void StatusWidget::showEvent(QShowEvent*)
{
    adjustPosition(parentWidget());
}

void StatusWidget::hideEvent(QHideEvent*)
{
}

// taken from QDialog::adjustPosition(QWidget*)
void StatusWidget::adjustPosition(QWidget* w)
{
    QPoint p(0, 0);
    int extraw = 0, extrah = 0, scrn = 0;
    if (w)
        w = w->window();
    QRect desk;
    if (w) {
        scrn = QApplication::desktop()->screenNumber(w);
    } else if (QApplication::desktop()->isVirtualDesktop()) {
        scrn = QApplication::desktop()->screenNumber(QCursor::pos());
    } else {
        scrn = QApplication::desktop()->screenNumber(this);
    }
    desk = QApplication::desktop()->availableGeometry(scrn);

    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; (extraw == 0 || extrah == 0) && i < list.size(); ++i) {
        QWidget * current = list.at(i);
        if (current->isVisible()) {
            int framew = current->geometry().x() - current->x();
            int frameh = current->geometry().y() - current->y();

            extraw = qMax(extraw, framew);
            extrah = qMax(extrah, frameh);
        }
    }

    // sanity check for decoration frames. With embedding, we
    // might get extraordinary values
    if (extraw == 0 || extrah == 0 || extraw >= 10 || extrah >= 40) {
        extrah = 40;
        extraw = 10;
    }


    if (w) {
        // Use mapToGlobal rather than geometry() in case w might
        // be embedded in another application
        QPoint pp = w->mapToGlobal(QPoint(0,0));
        p = QPoint(pp.x() + w->width()/2,
                    pp.y() + w->height()/ 2);
    } else {
        // p = middle of the desktop
        p = QPoint(desk.x() + desk.width()/2, desk.y() + desk.height()/2);
    }

    // p = origin of this
    p = QPoint(p.x()-width()/2 - extraw,
                p.y()-height()/2 - extrah);


    if (p.x() + extraw + width() > desk.x() + desk.width())
        p.setX(desk.x() + desk.width() - width() - extraw);
    if (p.x() < desk.x())
        p.setX(desk.x());

    if (p.y() + extrah + height() > desk.y() + desk.height())
        p.setY(desk.y() + desk.height() - height() - extrah);
    if (p.y() < desk.y())
        p.setY(desk.y());

    move(p);
}

// --------------------------------------------------------------------

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(PropertyListEditor *editor) : QWidget(editor) {
        codeEditor = editor;
    }

    QSize sizeHint() const {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    PropertyListEditor *codeEditor;
};

PropertyListEditor::PropertyListEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, SIGNAL(blockCountChanged(int)),
            this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)),
            this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()),
            this, SLOT(highlightCurrentLine()));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int PropertyListEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
}

void PropertyListEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void PropertyListEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void PropertyListEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void PropertyListEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void PropertyListEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::lightGray);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
    int bottom = top + (int) blockBoundingRect(block).height();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}

class PropertyListDialog : public QDialog
{
    int type;

public:
    PropertyListDialog(int type, QWidget* parent) : QDialog(parent),type(type)
    {
    }

    void accept()
    {
        PropertyListEditor* edit = this->findChild<PropertyListEditor*>();
        QStringList lines;
        if (edit) {
            QString inputText = edit->toPlainText();
            if (!inputText.isEmpty()) // let pass empty input, regardless of the type, so user can void the value
                lines = inputText.split(QString::fromLatin1("\n"));
        }
        if (!lines.isEmpty()) {
            if (type == 1) { // floats
                bool ok;
                int line=1;
                for (QStringList::iterator it = lines.begin(); it != lines.end(); ++it, ++line) {
                    it->toDouble(&ok);
                    if (!ok) {
                        QMessageBox::critical(this, tr("Invalid input"), tr("Input in line %1 is not a number").arg(line));
                        return;
                    }
                }
            }
            else if (type == 2) { // integers
                bool ok;
                int line=1;
                for (QStringList::iterator it = lines.begin(); it != lines.end(); ++it, ++line) {
                    it->toInt(&ok);
                    if (!ok) {
                        QMessageBox::critical(this, tr("Invalid input"), tr("Input in line %1 is not a number").arg(line));
                        return;
                    }
                }
            }
        }
        QDialog::accept();
    }
};

// --------------------------------------------------------------------

LabelEditor::LabelEditor (QWidget * parent)
  : QWidget(parent)
{
    type = String;
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);

    lineEdit = new QLineEdit(this);
    layout->addWidget(lineEdit);

    connect(lineEdit, SIGNAL(textChanged(const QString &)),
            this, SLOT(validateText(const QString &)));

    button = new QPushButton(QLatin1String("..."), this);
#if defined (Q_OS_MAC)
    button->setAttribute(Qt::WA_LayoutUsesWidgetRect); // layout size from QMacStyle was not correct
#endif
    layout->addWidget(button);

    connect(button, SIGNAL(clicked()), this, SLOT(changeText()));

    setFocusProxy(lineEdit);
}

LabelEditor::~LabelEditor()
{
}

void LabelEditor::resizeEvent(QResizeEvent* e)
{
    button->setFixedWidth(e->size().height());
    button->setFixedHeight(e->size().height());
}

QString LabelEditor::text() const
{
    return this->plainText;
}

void LabelEditor::setText(const QString& s)
{
    this->plainText = s;

    QString text = QString::fromLatin1("[%1]").arg(this->plainText);
    lineEdit->setText(text);
}

void LabelEditor::changeText()
{
    PropertyListDialog dlg(static_cast<int>(type), this);
    dlg.setWindowTitle(tr("List"));
    QVBoxLayout* hboxLayout = new QVBoxLayout(&dlg);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(&dlg);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    PropertyListEditor *edit = new PropertyListEditor(&dlg);
    edit->setPlainText(this->plainText);

    hboxLayout->addWidget(edit);
    hboxLayout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dlg, SLOT(reject()));
    if (dlg.exec() == QDialog::Accepted) {
        QString inputText = edit->toPlainText();
        QString text = QString::fromLatin1("[%1]").arg(inputText);
        lineEdit->setText(text);
    }
}

/**
 * Validates if the input of the lineedit is a valid list.
 */
void LabelEditor::validateText(const QString& text)
{
    if (text.startsWith(QLatin1String("[")) && text.endsWith(QLatin1String("]"))) {
        this->plainText = text.mid(1, text.size()-2);
        Q_EMIT textChanged(this->plainText);
    }
}

/**
 * Sets the browse button's text to \a txt.
 */
void LabelEditor::setButtonText(const QString& txt)
{
    button->setText(txt);
    int w1 = 2*button->fontMetrics().width(txt);
    int w2 = 2*button->fontMetrics().width(QLatin1String(" ... "));
    button->setFixedWidth((w1 > w2 ? w1 : w2));
}

/**
 * Returns the browse button's text.
 */
QString LabelEditor::buttonText() const
{
    return button->text();
}

void LabelEditor::setInputType(InputType t)
{
    this->type = t;
}

#include "moc_Widgets.cpp"
