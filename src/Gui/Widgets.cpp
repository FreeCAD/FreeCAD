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
# include <QColorDialog>
# include <QDesktopWidget>
# include <QDialogButtonBox>
# include <QDrag>
# include <QEventLoop>
# include <QKeyEvent>
# include <QMimeData>
# include <QPainter>
# include <QPlainTextEdit>
# include <QStylePainter>
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
void CommandIconView::startDrag ( Qt::DropActions supportedActions )
{
    QList<QListWidgetItem*> items = selectedItems();
    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);

    QPixmap pixmap;
    dataStream << items.count();
    for (QList<QListWidgetItem*>::ConstIterator it = items.begin(); it != items.end(); ++it) {
        if (it == items.begin())
            pixmap = qVariantValue<QPixmap>((*it)->data(Qt::UserRole));
        dataStream << (*it)->text();
    }

    QMimeData *mimeData = new QMimeData;
    mimeData->setData(QString::fromAscii("text/x-action-items"), itemData);

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
    labelAvailable->setText(QApplication::translate("Gui::ActionSelector", "Available:", 0, QApplication::UnicodeUTF8));
    labelSelected->setText(QApplication::translate("Gui::ActionSelector", "Selected:", 0, QApplication::UnicodeUTF8));
    addButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Add", 0, QApplication::UnicodeUTF8));
    removeButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Remove", 0, QApplication::UnicodeUTF8));
    upButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Move up", 0, QApplication::UnicodeUTF8));
    downButton->setToolTip(QApplication::translate("Gui::ActionSelector", "Move down", 0, QApplication::UnicodeUTF8));
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
    setText(tr("none"));
}

/**
 * Checks which keys are pressed and show it as text.
 */
void AccelLineEdit::keyPressEvent ( QKeyEvent * e)
{
    QString txt;
    setText(tr("none"));

    int key = e->key();
    Qt::KeyboardModifiers state = e->modifiers();

    if (key == Qt::Key_Control)
        return;
    else if (key == Qt::Key_Shift)
        return;
    else if (key == Qt::Key_Alt)
        return;
    else if (state == Qt::NoModifier && key == Qt::Key_Backspace)
        return; // clears the edit field

    switch(state)
    {
    case Qt::ControlModifier:
        {
            QKeySequence ks(Qt::CTRL+key);
            txt += (QString)(ks);
            setText(txt);
        }   break;
    case Qt::AltModifier:
        {
            QKeySequence ks(Qt::ALT+key);
            txt += (QString)(ks);
            setText(txt);
        }   break;
    case Qt::ShiftModifier:
        {
            QKeySequence ks(Qt::SHIFT+key);
            txt += (QString)(ks);
            setText(txt);
        }   break;
    case Qt::ControlModifier+Qt::AltModifier:
        {
            QKeySequence ks(Qt::CTRL+Qt::ALT+key);
            txt += (QString)(ks);
            setText(txt);
        }   break;
    case Qt::ControlModifier+Qt::ShiftModifier:
        {
            QKeySequence ks(Qt::CTRL+Qt::SHIFT+key);
            txt += (QString)(ks);
            setText(txt);
        }   break;
    case Qt::ShiftModifier+Qt::AltModifier:
        {
            QKeySequence ks(Qt::SHIFT+Qt::ALT+key);
            txt += (QString)(ks);
            setText(txt);
        }   break;
    case Qt::ControlModifier+Qt::AltModifier+Qt::ShiftModifier:
        {
            QKeySequence ks(Qt::CTRL+Qt::ALT+Qt::SHIFT+key);
            txt += (QString)(ks);
            setText(txt);
        }   break;
    default:
        {
            QKeySequence ks(key);
            txt += (QString)(ks);
            setText(txt);
        }   break;
    }
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::CheckListDialog */

/**
 *  Constructs a CheckListDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
CheckListDialog::CheckListDialog( QWidget* parent, Qt::WFlags fl )
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
    bool drawFrame;
    bool modal;

    ColorButtonP() : cd(0), allowChange(true), drawFrame(true), modal(true)
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

/**
 * Draws the button label.
 */
void ColorButton::paintEvent (QPaintEvent * e)
{
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
        QColor c = QColorDialog::getColor(d->col, this);
        if (c.isValid()) {
            setColor(c);
            changed();
        }
#if QT_VERSION >= 0x040500
    }
    else {
        if (d->cd.isNull()) {
            d->old = d->col;
            d->cd = new QColorDialog(d->col, this);
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

UrlLabel::UrlLabel(QWidget * parent, Qt::WFlags f)
  : QLabel(parent, f)
{
    _url = QString::fromAscii("http://localhost");
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
            PyObject* args = Py_BuildValue("(s)", (const char*)this->_url.toAscii());
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
    layout->addWidget(button);

    connect(button, SIGNAL(clicked()), this, SLOT(browse()));
}

LabelButton::~LabelButton()
{
}

void LabelButton::resizeEvent(QResizeEvent* e)
{
    button->setFixedWidth(e->size().height());
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

LabelEditor::LabelEditor (QWidget * parent)
  : QWidget(parent)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(2);

    lineEdit = new QLineEdit(this);
    layout->addWidget(lineEdit);

    connect(lineEdit, SIGNAL(textChanged(const QString &)),
            this, SIGNAL(textChanged(const QString &)));

    button = new QPushButton(QLatin1String("..."), this);
    button->setFixedWidth(2*button->fontMetrics().width(QLatin1String(" ... ")));
    layout->addWidget(button);

    connect(button, SIGNAL(clicked()), this, SLOT(changeText()));

    setFocusProxy(lineEdit);
}

LabelEditor::~LabelEditor()
{
}

QString LabelEditor::text() const
{
    return lineEdit->text();
}

void LabelEditor::setText(const QString& s)
{
    lineEdit->setText(s);
}

void LabelEditor::changeText()
{
    QDialog dlg(this);
    QVBoxLayout* hboxLayout = new QVBoxLayout(&dlg);
    QDialogButtonBox* buttonBox = new QDialogButtonBox(&dlg);
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Close);

    QPlainTextEdit *edit = new QPlainTextEdit(&dlg);
    edit->setPlainText(this->lineEdit->text());
    
    hboxLayout->addWidget(edit);
    hboxLayout->addWidget(buttonBox);
    connect(buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), &dlg, SLOT(reject()));
    if (dlg.exec() == QDialog::Accepted) {
        this->lineEdit->setText(edit->toPlainText());
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

#include "moc_Widgets.cpp"
