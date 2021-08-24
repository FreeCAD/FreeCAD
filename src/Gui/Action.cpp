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
# include <boost_signals2.hpp>
# include <boost_bind_bind.hpp>
# include <QAbstractItemView>
# include <QActionEvent>
# include <QApplication>
# include <QDesktopWidget>
# include <QEvent>
# include <QMessageBox>
# include <QTimer>
# include <QToolBar>
# include <QToolButton>
# include <QElapsedTimer>
# include <QClipboard>
# include <QCheckBox>
#endif

#include <QWidgetAction>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
# include <QWindow>
# include <QScreen>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <cctype>
#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>


#include <Base/Tools.h>
#include <Base/Parameter.h>
#include <App/DocumentObserver.h>
#include <App/AutoTransaction.h>
#include "Action.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "DlgUndoRedo.h"
#include "DlgWorkbenchesImp.h"
#include "Document.h"
#include "EditorView.h"
#include "FileDialog.h"
#include "Macro.h"
#include "MainWindow.h"
#include "PythonEditor.h"
#include "WhatsThis.h"
#include "Widgets.h"
#include "Workbench.h"
#include "WorkbenchManager.h"
#include "View3DInventor.h"
#include "Document.h"
#include "SelectionView.h"
#include "ViewParams.h"
#include "BitmapFactory.h"
#include "PieMenu.h"

#include <Base/Exception.h>
#include <App/Application.h>

FC_LOG_LEVEL_INIT("Gui", true, true)

using namespace Gui;
using namespace Gui::Dialog;
namespace bp = boost::placeholders;

/**
 * Constructs an action called \a name with parent \a parent. It also stores a pointer
 * to the command object.
 */
Action::Action (Command* pcCmd, QObject * parent)
  : QObject(parent), _action(new QAction( this )), _pcCmd(pcCmd)
{
    _action->setObjectName(QString::fromLatin1(_pcCmd->getName()));
    connect(_action, SIGNAL(triggered(bool)), this, SLOT(onActivated()));
}

Action::Action (Command* pcCmd, QAction* action, QObject * parent)
  : QObject(parent), _action(action), _pcCmd(pcCmd)
{
    _action->setParent(this);
    _action->setObjectName(QString::fromLatin1(_pcCmd->getName()));
    connect(_action, SIGNAL(triggered(bool)), this, SLOT(onActivated()));
}

Action::~Action()
{
    delete _action;
}

/**
 * Adds this action to widget \a w.
 */
void Action::addTo(QWidget *w)
{
    w->addAction(_action);
}

/**
 * Activates the command.
 */
void Action::onActivated ()
{
    _pcCmd->invoke(0,Command::TriggerAction);
}

/**
 * Sets whether the command is toggled.
 */
void Action::onToggled(bool b)
{
    _pcCmd->invoke( b ? 1 : 0 , Command::TriggerAction);
}

void Action::setCheckable(bool b)
{
    if(b == _action->isCheckable())
        return;
    _action->setCheckable(b);
    if (b) {
        disconnect(_action, SIGNAL(triggered(bool)), this, SLOT(onActivated()));
        connect(_action, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
    }
    else {
        connect(_action, SIGNAL(triggered(bool)), this, SLOT(onActivated()));
        disconnect(_action, SIGNAL(toggled(bool)), this, SLOT(onToggled(bool)));
    }
}

void Action::setChecked(bool b, bool no_signal)
{
    bool blocked;
    if(no_signal)
        blocked = _action->blockSignals(true);
    _action->setChecked(b);
    if(no_signal)
        _action->blockSignals(blocked);
}

bool Action::isChecked() const
{
    return _action->isChecked();
}

/**
 * Sets whether the action is enabled.
 */
void Action::setEnabled(bool b)
{
    _action->setEnabled(b);
}

void Action::setVisible(bool b)
{
    _action->setVisible(b);
}

void Action::setShortcut(const QString & key)
{
    _action->setShortcut(key);
    setToolTip(_tooltip, _title);
}

QKeySequence Action::shortcut() const
{
    return _action->shortcut();
}

void Action::setIcon (const QIcon & icon)
{
    _action->setIcon(icon);
}

QIcon Action::icon () const
{
    return _action->icon();
}

void Action::setStatusTip(const QString & s)
{
    _action->setStatusTip(s);
}

QString Action::statusTip() const
{
    return _action->statusTip();
}

void Action::setText(const QString & s)
{
    _action->setText(s);
    if (_title.isEmpty())
        setToolTip(_tooltip);
}

QString Action::text() const
{
    return _action->text();
}

void Action::setToolTip(const QString & s, const QString & title)
{
    _tooltip = s;
    _title = title;
    _action->setToolTip(createToolTip(s,
                title.isEmpty() ? _action->text() : title, 
                _action->font(),
                _action->shortcut().toString(QKeySequence::NativeText)));
}

QString Action::createToolTip(QString _tooltip,
                              const QString & title,
                              const QFont &font,
                              const QString &sc)
{
    QString text = title;
    text.remove(QLatin1Char('&'));;
    while(text.size() && text[text.size()-1].isPunct())
        text.resize(text.size()-1);

    if (text.isEmpty())
        return _tooltip;

    // The follow code tries to make a more useful tooltip by inserting at the
    // begining of the tooltip the action title in bold followed by the
    // shortcut.
    //
    // The long winding code is to deal with the fact that Qt will auto wrap
    // a rich text tooltip but the width is too short. We can escape the auto
    // wrappin using <p style='white-space:pre'>.

    QString shortcut = sc;
    if (shortcut.size() && _tooltip.endsWith(shortcut))
        _tooltip.resize(_tooltip.size() - shortcut.size());
    if (shortcut.size())
        shortcut = QString::fromLatin1(" (%1)").arg(shortcut);

    QString tooltip = QString::fromLatin1(
            "<p style='white-space:pre'><b>%1</b>%2</p>").arg(
            text.toHtmlEscaped(), shortcut.toHtmlEscaped());

    if (shortcut.size() && _tooltip.endsWith(shortcut))
        _tooltip.resize(_tooltip.size() - shortcut.size());

    if (_tooltip.isEmpty()
            || _tooltip == text
            || _tooltip == title)
    {
        return tooltip;
    }
    if (Qt::mightBeRichText(_tooltip)) {
        // already rich text, so let it be to avoid duplicated unwrapping
        return tooltip + _tooltip;
    }

    tooltip += QString::fromLatin1(
            "<style>p { margin: 0 }</style>" // remove any margin of a paragraph
            "<p style='white-space:pre'>");

    // If the user supplied tooltip contains line break, we shall honour it.
    if (_tooltip.indexOf(QLatin1Char('\n')) >= 0)
        tooltip += _tooltip.toHtmlEscaped() + QString::fromLatin1("</p>") ;
    else {
        // If not, try to end the non wrapping paragraph at some pre defined
        // width, so that the following text can wrap at that width.
        float tipWidth = 400;
        QFontMetrics fm(font);
        int width = fm.width(_tooltip);
        if (width <= tipWidth)
            tooltip += _tooltip.toHtmlEscaped() + QString::fromLatin1("</p>") ;
        else {
            int index = tipWidth / width * _tooltip.size();
            // Try to only break at white space
            for(int i=0; i<50 && index<_tooltip.size(); ++i, ++index) {
                if (_tooltip[index] == QLatin1Char(' '))
                    break;
            }
            tooltip += _tooltip.left(index).toHtmlEscaped()
                + QString::fromLatin1("</p>")
                + _tooltip.right(_tooltip.size()-index).trimmed().toHtmlEscaped();
        }
    }
    return tooltip;
}

QString Action::toolTip() const
{
    return _tooltip;
}

void Action::setWhatsThis(const QString & s)
{
    _action->setWhatsThis(s);
}

QString Action::whatsThis() const
{
    return _action->whatsThis();
}

void Action::setMenuRole(QAction::MenuRole menuRole)
{
    _action->setMenuRole(menuRole);
}

QAction *
Action::addCheckBox(QMenu *menu,
                    const QString &txt,
                    const QString &tooltip,
                    const QIcon &icon,
                    bool checked,
                    QCheckBox **_checkbox)
{
    QWidget *widget = new QWidget(menu);
    QHBoxLayout *layout = new QHBoxLayout(widget);
    widget->setLayout(layout);
    auto checkbox = new QCheckBox(widget);
    if (_checkbox) *_checkbox = checkbox;
    layout->addWidget(checkbox);
    layout->setContentsMargins(4,0,4,0);
    QWidgetAction *wa = new QWidgetAction(menu);
    wa->setDefaultWidget(widget);
    wa->setToolTip(tooltip);
    wa->setStatusTip(tooltip);
    wa->setVisible(true);
    wa->setText(txt);
    checkbox->setText(txt);
    checkbox->setChecked(checked);
    checkbox->setIcon(icon);
    wa->setCheckable(true);
    wa->setChecked(checked);
    menu->addAction(wa);
    QObject::connect(checkbox, SIGNAL(toggled(bool)), wa, SLOT(setChecked(bool)));
    QObject::connect(checkbox, SIGNAL(toggled(bool)), wa, SIGNAL(toggled(bool)));
    return wa;
}

// --------------------------------------------------------------------

/**
 * Constructs an action called \a name with parent \a parent. It also stores a pointer
 * to the command object.
 */
ActionGroup::ActionGroup ( Command* pcCmd,QObject * parent)
  : Action(pcCmd, parent), _group(0), _dropDown(false),_external(false),_toggle(false)
{
    _group = new QActionGroup(this);
    connect(_group, SIGNAL(triggered(QAction*)), this, SLOT(onActivated (QAction*)));
    connect(_group, SIGNAL(hovered(QAction*)), this, SLOT(onHovered(QAction*)));
}

ActionGroup::~ActionGroup()
{
    delete _group;
}

/**
 * Adds this action to widget \a w.
 */
void ActionGroup::addTo(QWidget *w)
{
    // When adding an action that has defined a menu then shortcuts
    // of the menu actions don't work. To make this working we must
    // set the menu explicitly. This means calling QAction::setMenu()
    // and adding this action to the widget doesn't work.
    if (_dropDown) {
        if (w->inherits("QMenu")) {
            QMenu *menu = new QMenu(w);
            QAction* action = qobject_cast<QMenu*>(w)->addMenu(menu);
            action->setMenuRole(_action->menuRole());
            menu->setTitle(_action->text());
            menu->addActions(_group->actions());
        }
        else if (w->inherits("QToolBar")) {
            w->addAction(_action);
            QToolButton* tb = w->findChildren<QToolButton*>().last();
            tb->setPopupMode(QToolButton::MenuButtonPopup);
            tb->setObjectName(QString::fromLatin1("qt_toolbutton_menubutton"));
            QList<QAction*> acts = _group->actions();
            QMenu* menu = new QMenu(tb);
            menu->addActions(acts);
            tb->setMenu(menu);
            //tb->addActions(_group->actions());
        }
        else {
            w->addActions(_group->actions()); // no drop-down
        }
    }
    else {
        w->addActions(_group->actions());
    }
}

void ActionGroup::setEnabled( bool b )
{
    Action::setEnabled(b);
    _group->setEnabled(b);
}

void ActionGroup::setDisabled (bool b)
{
    Action::setEnabled(!b);
    _group->setDisabled(b);
}

void ActionGroup::setExclusive (bool b)
{
    _group->setExclusive(b);
}

bool ActionGroup::isExclusive() const
{
    return _group->isExclusive();
}

void ActionGroup::setVisible( bool b )
{
    Action::setVisible(b);
    _group->setVisible(b);
}

QAction* ActionGroup::addAction(QAction* action)
{
    int index = _group->actions().size();
    action = _group->addAction(action);
    action->setData(QVariant(index));
    return action;
}

QAction* ActionGroup::addAction(const QString& text)
{
    int index = _group->actions().size();
    QAction* action = _group->addAction(text);
    action->setData(QVariant(index));
    return action;
}

QList<QAction*> ActionGroup::actions() const
{
    return _group->actions();
}

int ActionGroup::checkedAction() const
{
    QAction* checked = _group->checkedAction();
    return checked ? checked->data().toInt() : -1;
}

void ActionGroup::setCheckedAction(int i)
{
    QAction* a = _group->actions()[i];
    a->setChecked(true);
    this->setIcon(a->icon());
    this->setToolTip(a->toolTip());
    this->setProperty("defaultAction", QVariant(i));
}

/**
 * Activates the command.
 */
void ActionGroup::onActivated ()
{
    _pcCmd->invoke(this->property("defaultAction").toInt(), Command::TriggerAction);
}

void ActionGroup::onToggled(bool checked)
{
    (void)checked;
    onActivated();
}

/**
 * Activates the command.
 */
void ActionGroup::onActivated (QAction* a)
{
    int index = _group->actions().indexOf(a);

    // Calling QToolButton::setIcon() etc. has no effect if it has QAction set.
    // We have to change the QAction icon instead
#if 0
    QList<QWidget*> widgets = a->associatedWidgets();
    for (QList<QWidget*>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
        QMenu* menu = qobject_cast<QMenu*>(*it);
        if (menu) {
            QToolButton* button = qobject_cast<QToolButton*>(menu->parent());
            if (button) {
                button->setIcon(a->icon());
                button->setText(a->text());
                button->setToolTip(a->toolTip());
                this->setProperty("defaultAction", QVariant(index));
            }
        }
    }
#endif

    // The following logic is moved to Command::onInvoke()
#if 0
    this->setIcon(a->icon());
    this->setToolTip(a->toolTip());
    this->setProperty("defaultAction", QVariant(index));
#endif

    _pcCmd->invoke(index, Command::TriggerChildAction);
}

void ActionGroup::onHovered (QAction *a)
{
    Gui::ToolTip::showText(QCursor::pos(), a->toolTip());
}


// --------------------------------------------------------------------

namespace Gui {

/**
 * The WorkbenchActionEvent class is used to send an event of which workbench must be activated.
 * We cannot activate the workbench directly as we will destroy the widget that emits the signal.
 * @author Werner Mayer
 */
class WorkbenchActionEvent : public QEvent
{
public:
    WorkbenchActionEvent(QAction* a)
      : QEvent(QEvent::User), act(a)
    { }
    ~WorkbenchActionEvent()
    { }
    QAction* action() const
    { return act; }
private:
    QAction* act;
};
}

WorkbenchComboBox::WorkbenchComboBox(WorkbenchGroup* wb, QWidget* parent) : QComboBox(parent), group(wb)
{
    connect(this, SIGNAL(activated(int)), this, SLOT(onActivated(int)));
    connect(getMainWindow(), SIGNAL(workbenchActivated(const QString&)),
            this, SLOT(onWorkbenchActivated(const QString&)));
    connect(wb, SIGNAL(workbenchListUpdated()), this, SLOT(populate()));
}

WorkbenchComboBox::~WorkbenchComboBox()
{
}

void WorkbenchComboBox::showPopup()
{
    int rows = count();
    if (rows > 0) {
        int height = view()->sizeHintForRow(0);
        int maxHeight = QApplication::desktop()->availableGeometry(getMainWindow()).height();
        view()->setMinimumHeight(qMin(height * rows, maxHeight/2));
    }

    QComboBox::showPopup();
}

void WorkbenchComboBox::populate()
{
    clear();
    auto actions = group->actions();
    if (ViewParams::getAutoSortWBList()) {
        std::sort(actions.begin(), actions.end(),
            [](const QAction *a, const QAction *b) {
                return a->text().compare(b->text(), Qt::CaseInsensitive) < 0;
            });
    }

    auto wb = WorkbenchManager::instance()->active();
    QString current;
    if (wb)
        current = QString::fromUtf8(wb->name().c_str());

    for (auto action : actions) {
        if (!action->isVisible())
            continue;
        QIcon icon = action->icon();
        if (icon.isNull())
            this->addItem(action->text(), action->data());
        else
            this->addItem(icon, action->text(), action->data());
        if (current == action->objectName())
            this->setCurrentIndex(this->count()-1);
    }
}

void WorkbenchComboBox::onActivated(int i)
{
    // Send the event to the workbench group to delay the destruction of the emitting widget.
    int index = itemData(i).toInt();
    WorkbenchActionEvent* ev = new WorkbenchActionEvent(this->group->actions()[index]);
    QApplication::postEvent(this->group, ev);
    // TODO: Test if we can use this instead
    //QTimer::singleShot(20, this->actions()[i], SLOT(trigger()));
}

void WorkbenchComboBox::onActivated(QAction* action)
{
    // set the according item to the action
    QVariant data = action->data();
    int index = this->findData(data);
    if (index >= 0)
        setCurrentIndex(index);
    else
        populate();
}

void WorkbenchComboBox::onWorkbenchActivated(const QString& name)
{
    // There might be more than only one instance of WorkbenchComboBox there.
    // However, all of them share the same QAction objects. Thus, if the user
    // has  selected one it also gets checked. Then Application::activateWorkbench
    // also invokes this slot method by calling the signal workbenchActivated in
    // MainWindow. If calling activateWorkbench() from within the Python console
    // the matching action must be set by calling this function.
    // To avoid to recursively (but only one recursion level) call Application::
    // activateWorkbench the method refreshWorkbenchList() shouldn't set the
    // checked item.
    //QVariant item = itemData(currentIndex());
    QList<QAction*> a = group->actions();
    for (QList<QAction*>::Iterator it = a.begin(); it != a.end(); ++it) {
        if ((*it)->objectName() == name) {
            if (/*(*it)->data() != item*/!(*it)->isChecked())
                (*it)->trigger();
            break;
        }
    }
}

// --------------------------------------------------------------------

class WorkbenchGroup::Private: public ParameterGrp::ObserverType
{
public:
    Private(WorkbenchGroup *master, const char *path):master(master)
    {
        hGeneral = App::GetApplication().GetUserParameter().GetGroup(
                "BaseApp/Preferences/General");
        handle = App::GetApplication().GetParameterGroupByPath(path);
        handle->Attach(this);
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, [master](){
            master->refreshWorkbenchList();
        });
    }

    virtual ~Private()
    {
        handle->Detach(this);
    }

    bool showText() {
        return handle->GetBool("TabBarShowText", false);
    }

    void setShowText(bool enable) {
        handle->SetBool("TabBarShowText", enable);
    }

    int toolbarIconSize() {
        int pixel = hGeneral->GetInt("ToolbarIconSize");
        if (pixel <= 0)
            pixel = 24;
        return pixel;
    }

    bool showTabBar() {
        return handle->GetBool("ShowTabBar", false);
    }

    void setShowTabBar(bool enable) {
        handle->SetBool("ShowTabBar", enable);
    }

    void OnChange(Base::Subject<const char*> &, const char *reason)
    {
        if (!reason)
            return;
        if (boost::equals(reason, "Disabled")
                || boost::equals(reason, "Enabled"))
        {
            timer.start(100);
        }
    }

public:
    WorkbenchGroup *master;
    ParameterGrp::handle handle;
    ParameterGrp::handle hGeneral;
    QTimer timer;
};

// --------------------------------------------------------------------

WorkbenchTabBar::WorkbenchTabBar(WorkbenchGroup* wb, QWidget* parent)
    : QTabWidget(parent), group(wb)
{
    connect(this, SIGNAL(currentChanged(int)), this, SLOT(onCurrentChanged()));
    connect(this->tabBar(), SIGNAL(tabMoved(int, int)), this, SLOT(onTabMoved(int, int)));
    connect(getMainWindow(), SIGNAL(workbenchActivated(const QString&)),
            this, SLOT(onWorkbenchActivated(const QString&)));
    connect(wb, SIGNAL(workbenchListUpdated()), this, SLOT(updateWorkbenches()));
    updateWorkbenches();
    this->setMovable(true);
    this->tabBar()->setDrawBase(true);
    this->setDocumentMode(true);
    // this->setUsesScrollButtons(true);

    this->tabBar()->installEventFilter(this);

    connParam = App::GetApplication().GetUserParameter().signalParamChanged.connect(
        [this](ParameterGrp *Param, ParameterGrp::ParamType, const char *Name, const char *) {
            if (!Name)
                return;
            if (Param == this->group->_pimpl->handle) {
                if (boost::equals(Name, "TabBarShowText") || boost::equals(Name, "ShowTabBar"))
                    timer.start(100);
            } else if (Param == this->group->_pimpl->hGeneral) {
                if (boost::equals(Name, "ToolbarIconSize"))
                    timer.start(100);
            }
        });

    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, [this]() {
        updateWorkbenches();
        setupVisibility();
    });

    timerCurrentChange.setSingleShot(true);
    connect(&timerCurrentChange, SIGNAL(timeout()), this, SLOT(onCurrentChanged()));

    onChangeOrientation();
}

WorkbenchTabBar::~WorkbenchTabBar()
{
}

QToolBar *WorkbenchTabBar::getToolBar()
{
    for (auto parent = parentWidget(); parent; parent = parent->parentWidget()) {
        if (auto tb = qobject_cast<QToolBar*>(parent))
            return tb;
    }
    return nullptr;
}

void WorkbenchTabBar::setupVisibility()
{
    if (!action)
        return;

    bool vis = this->group->_pimpl->showTabBar()
        || this->tabPosition() == QTabWidget::West
        || this->tabPosition() == QTabWidget::East;
    if (vis == this->action->isVisible())
        return;

    if (auto parent = qobject_cast<QToolBar*>(this->parentWidget())) {
        auto combo = parent->findChild<WorkbenchComboBox*>();
        if (combo && combo->getAction()) {
            for (auto w = QApplication::focusWidget();
                    w && !w->isWindow();
                    w = w->parentWidget())
            {
                if (w == parent) {
                    getMainWindow()->setFocus();
                    break;
                }
            }
            combo->getAction()->setVisible(!vis);
            this->action->setVisible(vis);
        }
    }
}

void WorkbenchTabBar::onChangeOrientation()
{
    auto parent = qobject_cast<QToolBar*>(parentWidget());
    if (!parent)
        return;

    auto area = getMainWindow()->toolBarArea(parent);
    QTabWidget::TabPosition pos;
    if (parent->orientation() == Qt::Horizontal) {
        if(area == Qt::BottomToolBarArea)
            pos = QTabWidget::South;
        else
            pos = QTabWidget::North;
    } else if (area == Qt::RightToolBarArea)
        pos = QTabWidget::East;
    else
        pos = QTabWidget::West;
    if (pos != this->tabPosition()) {
        this->setTabPosition(pos);
        // inform layout change
        qApp->postEvent(parent, new QActionEvent(QEvent::ActionChanged, this->action));
    }

    setupVisibility();
}

void WorkbenchTabBar::updateWorkbenches()
{
    auto tab = this->tabBar();
    int s = std::max(16, this->group->_pimpl->toolbarIconSize());
    if (this->iconSize() != QSize(s,s))
        this->setIconSize(QSize(s,s));

    auto wb = WorkbenchManager::instance()->active();
    QString current;
    if (wb)
        current = QString::fromUtf8(wb->name().c_str());

    QSignalBlocker block(this);
    bool showText = this->group->_pimpl->showText();
    if (showText) {
        if (this->styleSheet().size())
            this->setStyleSheet(QString());
    } else if (this->styleSheet().isEmpty()) {
        this->setStyleSheet(
                QStringLiteral("::tab:top,"
                               "::tab:bottom {min-width: -1;}"
                               "::tab:left,"
                               "::tab:right {min-height: -1;}"));
    }
    int i=0;
    for (auto action : this->group->actions()) {
        if (!action->isVisible())
            continue;
        bool changed = true;
        if (i >= this->count())
            this->addTab(new QWidget(), action->icon(), QString());
        else if (tab->tabData(i).toString() == action->objectName())
            changed = false;

        if (changed) {
            tab->setTabIcon(i, action->icon());
            tab->setTabData(i, action->objectName());
            tab->setTabToolTip(i, 
                    Action::createToolTip(action->toolTip(),
                                        action->text(),
                                        action->font(),
                                        action->shortcut().toString(QKeySequence::NativeText)));
        }
        if (showText) {
            if (tab->tabText(i) != action->text())
                tab->setTabText(i, action->text());
        } else if (tab->tabText(i).size())
            tab->setTabText(i, QString());

        if (current == action->objectName() && tab->currentIndex() != i) {
            setCurrentIndex(i);
            // if (!showText)
            //     tab->setTabText(i, action->text());
        }
        ++i;
    }
    while (this->count() > i)
        this->removeTab(this->count()-1);
}

void WorkbenchTabBar::onCurrentChanged()
{
    if (QApplication::mouseButtons() & Qt::LeftButton) {
        timerCurrentChange.start(10);
        return;
    }

    auto tab = this->tabBar();
    int i = tab->currentIndex();
    QString name = tab->tabData(i).toString();
    // if (!this->group->_pimpl->showText()) {
    //     for (int j = 0, c = tab->count(); j<c; ++j) {
    //         if (j == i)
    //             tab->setTabText(i, Application::Instance->workbenchMenuText(name));
    //         else
    //             tab->setTabText(j, QString());
    //     }
    // }
    Application::Instance->activateWorkbench(name.toUtf8());
}

void WorkbenchTabBar::onTabMoved(int, int)
{
    moved = true;
}

bool WorkbenchTabBar::eventFilter(QObject *o, QEvent *ev)
{
    if (moved && o == this->tabBar()
              && ev->type() == QEvent::MouseButtonRelease
              && static_cast<QMouseEvent*>(ev)->button() == Qt::LeftButton)
    {
        moved = false;
        QStringList enabled;
        auto tab = this->tabBar();
        for (int i=0, c=tab->count(); i<c; ++i)
            enabled << tab->tabData(i).toString();
        Base::ConnectionBlocker blocker(connParam);
        QSignalBlocker blocker2(group);
        DlgWorkbenchesImp::save_workbenches(enabled);
    }
    return QTabWidget::eventFilter(o, ev);
}

void WorkbenchTabBar::onWorkbenchActivated(const QString& name)
{
    QSignalBlocker block(this);
    auto tab = this->tabBar();
    // bool showText = this->group->_pimpl->showText();
    int current = tab->currentIndex();
    if (current >= 0 && tab->tabData(current).toString() != name) {
        // if (!showText)
        //     tab->setTabText(current, QString());
        for (int i=0, c=tab->count(); i<c; ++i) {
            if (tab->tabData(i).toString() == name) {
                setCurrentIndex(i);
                // if (!showText)
                //     tab->setTabText(i, Application::Instance->workbenchMenuText(name));
            }
        }
    }
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::WorkbenchGroup */
WorkbenchGroup::WorkbenchGroup (  Command* pcCmd, QObject * parent )
  : ActionGroup( pcCmd, parent )
{
    _pimpl.reset(new Private(this, "User parameter:BaseApp/Workbenches"));

    // Start a list with 50 elements but extend it when requested
    for (int i=0; i<50; i++) {
        QAction* action = _group->addAction(QLatin1String(""));
        action->setVisible(false);
        action->setCheckable(true);
        action->setData(QVariant(i)); // set the index
    }

    Application::Instance->signalActivateWorkbench.connect(boost::bind(&WorkbenchGroup::slotActivateWorkbench, this, bp::_1));
    Application::Instance->signalAddWorkbench.connect(boost::bind(&WorkbenchGroup::slotAddWorkbench, this, bp::_1));
    Application::Instance->signalRemoveWorkbench.connect(boost::bind(&WorkbenchGroup::slotRemoveWorkbench, this, bp::_1));
}

WorkbenchGroup::~WorkbenchGroup()
{
    delete _menu;
}

void WorkbenchGroup::onContextMenuRequested(const QPoint &)
{
    bool combo = qobject_cast<QComboBox*>(sender()) != nullptr;
    QMenu menu;
    bool showText = _pimpl->showText();
    QAction *actText = nullptr;
    if (!combo)
        actText = menu.addAction(showText ? tr("Hide text") : tr("Show text"));
    auto actShow = menu.addAction(combo ? tr("Expand") : tr("Collapse"));
    menu.addSeparator();

    QMenu subMenu(tr("Disabled workbenches"));
    std::vector<QAction*> actions;
    QStringList disabled = DlgWorkbenchesImp::load_disabled_workbenches(true);
    for (auto &wb : disabled) {
        QString name = Application::Instance->workbenchMenuText(wb);
        QPixmap px = Application::Instance->workbenchIcon(wb);
        QString tip = Application::Instance->workbenchToolTip(wb);
        auto action = Action::addCheckBox(&subMenu, name, tip, px, false);
        action->setData(wb);
        actions.push_back(action);
    }
    if (!subMenu.isEmpty()) {
        menu.addMenu(&subMenu);
        menu.addSeparator();
    }
    QStringList enabled = DlgWorkbenchesImp::load_enabled_workbenches(true);
    for (auto &wb : enabled) {
        QString name = Application::Instance->workbenchMenuText(wb);
        QPixmap px = Application::Instance->workbenchIcon(wb);
        QString tip = Application::Instance->workbenchToolTip(wb);
        auto action = Action::addCheckBox(&menu, name, tip, px, true);
        action->setData(wb);
        actions.push_back(action);
    }

    auto act = menu.exec(QCursor::pos());
    if (act && act == actText)
        _pimpl->setShowText(!showText);
    else if (act && act == actShow)
        _pimpl->setShowTabBar(combo);

    QStringList enabled2, disabled2;
    for (auto action : actions) {
        if (action->isChecked())
            enabled2 << action->data().toString();
        else
            disabled2 << action->data().toString();
    }
    if (enabled != enabled2)
        DlgWorkbenchesImp::save_workbenches(enabled2, disabled2);
}

void WorkbenchGroup::addTo(QWidget *w)
{
    refreshWorkbenchList();
    if (w->inherits("QToolBar")) {
        QToolBar* bar = qobject_cast<QToolBar*>(w);
        auto box = new WorkbenchComboBox(this, w);
        box->setIconSize(QSize(16, 16));
        box->setToolTip(_tooltip);
        box->setStatusTip(_action->statusTip());
        box->setWhatsThis(_action->whatsThis());
        connect(_group, SIGNAL(triggered(QAction*)), box, SLOT(onActivated (QAction*)));
        box->populate();
        auto actBox = bar->addWidget(box);
        box->setAction(actBox);
        box->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(box, SIGNAL(customContextMenuRequested(QPoint)),
                this, SLOT(onContextMenuRequested(QPoint)));

        auto tabbar = new WorkbenchTabBar(this, w);
        auto actTab = bar->addWidget(tabbar);
        tabbar->setAction(actTab);
        tabbar->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(tabbar, SIGNAL(customContextMenuRequested(QPoint)),
                this, SLOT(onContextMenuRequested(QPoint)));
        connect(bar, SIGNAL(orientationChanged(Qt::Orientation)),
                tabbar, SLOT(onChangeOrientation()));
        connect(bar, SIGNAL(topLevelChanged(bool)),
                tabbar, SLOT(onChangeOrientation()));

        if (_pimpl->showTabBar())
            actBox->setVisible(false);
        else
            actTab->setVisible(false);
    }
    else if (w->inherits("QMenu")) {
        QMenu* menu = qobject_cast<QMenu*>(w);
        if (!_menu) {
            _menu = new QMenu(_action->text());
            onShowMenu();
            connect(_menu, SIGNAL(aboutToShow()), this, SLOT(onShowMenu()));
        }
        menu->addMenu(_menu);
    }
}

void WorkbenchGroup::onShowMenu()
{
    _menu->clear();
    auto actions = _group->actions();
    if (ViewParams::getAutoSortWBList()) {
        std::sort(actions.begin(), actions.end(),
            [](const QAction *a, const QAction *b) {
                return a->text().compare(b->text(), Qt::CaseInsensitive) < 0;
            });
    }
    _menu->addActions(actions);
}

void WorkbenchGroup::setWorkbenchData(int i, const QString& wb)
{
    QList<QAction*> workbenches = _group->actions();
    QString name = Application::Instance->workbenchMenuText(wb);
    QPixmap px = Application::Instance->workbenchIcon(wb);
    QString tip = Application::Instance->workbenchToolTip(wb);

    workbenches[i]->setObjectName(wb);
    workbenches[i]->setIcon(px);
    workbenches[i]->setText(name);
    workbenches[i]->setToolTip(tip);
    workbenches[i]->setStatusTip(tr("Select the '%1' workbench").arg(name));
    workbenches[i]->setVisible(true);
    if (i < 9)
        workbenches[i]->setShortcut(QKeySequence(QString::fromUtf8("W,%1").arg(i+1)));
}

void WorkbenchGroup::refreshWorkbenchList()
{
    QStringList items = Application::Instance->workbenches();
    QStringList enabled_wbs_list = DlgWorkbenchesImp::load_enabled_workbenches();
    QStringList disabled_wbs_list = DlgWorkbenchesImp::load_disabled_workbenches();
    QStringList enable_wbs;

    // Go through the list of enabled workbenches and verify that they really exist because
    // it might be possible that a workbench has been removed after setting up the list of
    // enabled workbenches.
    for (QStringList::Iterator it = enabled_wbs_list.begin(); it != enabled_wbs_list.end(); ++it) {
        int index = items.indexOf(*it);
        if (index >= 0) {
            enable_wbs << *it;
            items.removeAt(index);
        }
    }

    // Filter out the actively disabled workbenches
    for (QStringList::Iterator it = disabled_wbs_list.begin(); it != disabled_wbs_list.end(); ++it) {
        int index = items.indexOf(*it);
        if (index >= 0) {
            items.removeAt(index);
        }
    }

    // Now add the remaining workbenches of 'items'. They have been added to the application
    // after setting up the list of enabled workbenches.
    enable_wbs.append(items);
    QList<QAction*> workbenches = _group->actions();
    int numActions = workbenches.size();
    int extend = enable_wbs.size() - numActions;
    if (extend > 0) {
        for (int i=0; i<extend; i++) {
            QAction* action = _group->addAction(QLatin1String(""));
            action->setCheckable(true);
            action->setData(QVariant(numActions++)); // set the index
        }
    } else {
        for (;extend < 0; ++extend) {
            auto action = workbenches[numActions + extend];
            action->setVisible(false);
            action->setObjectName(QString());
        }
    }

    // Show all enabled wb
    int index = 0;
    for (QStringList::Iterator it = enable_wbs.begin(); it != enable_wbs.end(); ++it) {
        setWorkbenchData(index++, *it);
    }

    workbenchListUpdated();
}

void WorkbenchGroup::customEvent( QEvent* e )
{
    if (e->type() == QEvent::User) {
        Gui::WorkbenchActionEvent* ce = (Gui::WorkbenchActionEvent*)e;
        ce->action()->trigger();
    }
}

void WorkbenchGroup::slotActivateWorkbench(const char* /*name*/)
{
}

void WorkbenchGroup::slotAddWorkbench(const char* name)
{
    QList<QAction*> workbenches = _group->actions();
    QAction* action = nullptr;
    for (QList<QAction*>::Iterator it = workbenches.begin(); it != workbenches.end(); ++it) {
        if (!(*it)->isVisible()) {
            action = *it;
            break;
        }
    }

    if (!action) {
        int index = workbenches.size();
        action = _group->addAction(QLatin1String(""));
        action->setCheckable(true);
        action->setData(QVariant(index)); // set the index
    }

    QString wb = QString::fromLatin1(name);
    QPixmap px = Application::Instance->workbenchIcon(wb);
    QString text = Application::Instance->workbenchMenuText(wb);
    QString tip = Application::Instance->workbenchToolTip(wb);
    action->setIcon(px);
    action->setObjectName(wb);
    action->setText(text);
    action->setToolTip(tip);
    action->setStatusTip(tr("Select the '%1' workbench").arg(wb));
    action->setVisible(true); // do this at last
}

void WorkbenchGroup::slotRemoveWorkbench(const char* name)
{
    QString workbench = QString::fromLatin1(name);
    QList<QAction*> workbenches = _group->actions();
    for (QList<QAction*>::Iterator it = workbenches.begin(); it != workbenches.end(); ++it) {
        if ((*it)->objectName() == workbench) {
            (*it)->setObjectName(QString());
            (*it)->setIcon(QIcon());
            (*it)->setText(QString());
            (*it)->setToolTip(QString());
            (*it)->setStatusTip(QString());
            (*it)->setVisible(false); // do this at last
            break;
        }
    }
}

// --------------------------------------------------------------------

class RecentFilesAction::Private: public ParameterGrp::ObserverType
{
public:
    Private(RecentFilesAction *master, const char *path):master(master)
    {
        handle = App::GetApplication().GetParameterGroupByPath(path);
        handle->Attach(this);
    }

    virtual ~Private()
    {
        handle->Detach(this);
    }

    void OnChange(Base::Subject<const char*> &, const char *reason)
    {
        if (!updating && reason && strcmp(reason, "RecentFiles")==0) {
            Base::StateLocker guard(updating);
            master->restore();
        }
    }

public:
    RecentFilesAction *master;
    ParameterGrp::handle handle;
    bool updating = false;
};

// --------------------------------------------------------------------

/* TRANSLATOR Gui::RecentFilesAction */

RecentFilesAction::RecentFilesAction ( Command* pcCmd, QObject * parent )
  : ActionGroup( pcCmd, parent ), visibleItems(4), maximumItems(20)
{
    _pimpl.reset(new Private(this, "User parameter:BaseApp/Preferences/RecentFiles"));
    restore();
}

RecentFilesAction::~RecentFilesAction()
{
}

/** Adds the new item to the recent files. */
void RecentFilesAction::appendFile(const QString& filename)
{
    // empty file name signals the intention to rebuild recent file
    // list without changing the list content.
    if (filename.isEmpty()) {
        save();
        return;
    }

    // restore the list of recent files
    QStringList files = this->files();

    // if already inside remove and prepend it
    files.removeAll(filename);
    files.prepend(filename);
    setFiles(files);
    save();

    // update the XML structure and save the user parameter to disk (#0001989)
    bool saveParameter = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/General")->GetBool("SaveUserParameter", true);
    if (saveParameter) {
        ParameterManager* parmgr = App::GetApplication().GetParameterSet("User parameter");
        parmgr->SaveDocument(App::Application::Config()["UserParameter"].c_str());
    }
}

/**
 * Set the list of recent files. For each item an action object is
 * created and added to this action group.
 */
void RecentFilesAction::setFiles(const QStringList& files)
{
    QList<QAction*> recentFiles = _group->actions();

    int numRecentFiles = std::min<int>(recentFiles.count(), files.count());
    for (int index = 0; index < numRecentFiles; index++) {
        QFileInfo fi(files[index]);
        recentFiles[index]->setText(QString::fromLatin1("%1 %2").arg(index+1).arg(fi.fileName()));
        recentFiles[index]->setStatusTip(tr("Open file %1").arg(files[index]));
        recentFiles[index]->setToolTip(files[index]); // set the full name that we need later for saving
        recentFiles[index]->setData(QVariant(index));
        recentFiles[index]->setVisible(true);
    }

    // if less file names than actions
    numRecentFiles = std::min<int>(numRecentFiles, this->visibleItems);
    for (int index = numRecentFiles; index < recentFiles.count(); index++) {
        recentFiles[index]->setVisible(false);
        recentFiles[index]->setText(QString());
        recentFiles[index]->setToolTip(QString());
    }
}

/**
 * Returns the list of defined recent files.
 */
QStringList RecentFilesAction::files() const
{
    QStringList files;
    QList<QAction*> recentFiles = _group->actions();
    for (int index = 0; index < recentFiles.count(); index++) {
        QString file = recentFiles[index]->toolTip();
        if (file.isEmpty())
            break;
        files.append(file);
    }

    return files;
}

void RecentFilesAction::activateFile(int id)
{
    // restore the list of recent files
    QStringList files = this->files();
    if (id < 0 || id >= files.count())
        return; // no valid item

    QString filename = files[id];
    QFileInfo fi(filename);
    if (!fi.exists()) {
        QMessageBox::critical(getMainWindow(), tr("File not found"), tr("The file '%1' cannot be opened.").arg(filename));
        files.removeAll(filename);
        setFiles(files);
    }
    else {
        // invokes appendFile()
        SelectModule::Dict dict = SelectModule::importHandler(filename);
        for (SelectModule::Dict::iterator it = dict.begin(); it != dict.end(); ++it) {
            Application::Instance->open(it.key().toUtf8(), it.value().toLatin1());
            break;
        }
    }
}

void RecentFilesAction::resizeList(int size)
{
    this->visibleItems = size;
    int diff = this->visibleItems - this->maximumItems;
    // create new items if needed
    for (int i=0; i<diff; i++)
        _group->addAction(QLatin1String(""))->setVisible(false);
    setFiles(files());
}

/** Loads all recent files from the preferences. */
void RecentFilesAction::restore()
{
    ParameterGrp::handle hGrp = _pimpl->handle;
    // we want at least 20 items but we do only show the number of files
    // that is defined in user parameters
    this->visibleItems = hGrp->GetInt("RecentFiles", this->visibleItems);

    int count = std::max<int>(this->maximumItems, this->visibleItems);
    for (int i=_group->actions().size(); i<count; i++)
        _group->addAction(QLatin1String(""))->setVisible(false);
    std::vector<std::string> MRU = hGrp->GetASCIIs("MRU");
    QStringList files;
    for (std::vector<std::string>::iterator it = MRU.begin(); it!=MRU.end();++it)
        files.append(QString::fromUtf8(it->c_str()));
    setFiles(files);
}

/** Saves all recent files to the preferences. */
void RecentFilesAction::save()
{
    ParameterGrp::handle hGrp = _pimpl->handle;
    int count = hGrp->GetInt("RecentFiles", this->visibleItems); // save number of files
    hGrp->Clear();

    // count all set items
    QList<QAction*> recentFiles = _group->actions();
    int num = std::min<int>(count, recentFiles.count());
    for (int index = 0; index < num; index++) {
        QString key = QString::fromLatin1("MRU%1").arg(index);
        QString value = recentFiles[index]->toolTip();
        if (value.isEmpty())
            break;
        hGrp->SetASCII(key.toLatin1(), value.toUtf8());
    }

    Base::StateLocker guard(_pimpl->updating);
    hGrp->SetInt("RecentFiles", count); // restore
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::RecentMacrosAction */

RecentMacrosAction::RecentMacrosAction ( Command* pcCmd, QObject * parent )
  : ActionGroup( pcCmd, parent ), visibleItems(4), maximumItems(20)
{
    restore();
}

RecentMacrosAction::~RecentMacrosAction()
{
}

/** Adds the new item to the recent files. */
void RecentMacrosAction::appendFile(const QString& filename)
{
    // restore the list of recent files
    QStringList files = this->files();

    // if already inside remove and prepend it
    files.removeAll(filename);
    files.prepend(filename);
    setFiles(files);
    save();

    // update the XML structure and save the user parameter to disk (#0001989)
    bool saveParameter = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/General")->GetBool("SaveUserParameter", true);
    if (saveParameter) {
        ParameterManager* parmgr = App::GetApplication().GetParameterSet("User parameter");
        parmgr->SaveDocument(App::Application::Config()["UserParameter"].c_str());
    }
}

/**
 * Set the list of recent macro files. For each item an action object is
 * created and added to this action group.
 */
void RecentMacrosAction::setFiles(const QStringList& files)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                                ->GetGroup("Preferences")->GetGroup("RecentMacros");
    this->shortcut_modifiers = hGrp->GetASCII("ShortcutModifiers","Ctrl+Shift+");
    this->shortcut_count = std::min<int>(hGrp->GetInt("ShortcutCount",3),9);//max = 9, e.g. Ctrl+Shift+9
    this->visibleItems = hGrp->GetInt("RecentMacros",12);
    QList<QAction*> recentFiles = _group->actions();

    int numRecentFiles = std::min<int>(recentFiles.count(), files.count());
    for (int index = 0; index < numRecentFiles; index++) {
        QFileInfo fi(files[index]);
        QString accel = tr(QString::fromLatin1(shortcut_modifiers.c_str())\
                           .append(QString::number(index+1,10)).toStdString().c_str());
        recentFiles[index]->setText(QString::fromLatin1("%1 %2").arg(index+1).arg(fi.baseName()));
        recentFiles[index]->setStatusTip(tr("Run macro %1 (Shift+click to edit) shortcut: %2").arg(files[index]).arg(accel));
        recentFiles[index]->setToolTip(files[index]); // set the full name that we need later for saving
        recentFiles[index]->setData(QVariant(index));
        if (index < shortcut_count){
            recentFiles[index]->setShortcut(accel);
        }
        recentFiles[index]->setVisible(true);
    }

    // if less file names than actions
    numRecentFiles = std::min<int>(numRecentFiles, this->visibleItems);
    for (int index = numRecentFiles; index < recentFiles.count(); index++) {
        recentFiles[index]->setVisible(false);
        recentFiles[index]->setText(QString());
        recentFiles[index]->setToolTip(QString());
    }
}

/**
 * Returns the list of defined recent files.
 */
QStringList RecentMacrosAction::files() const
{
    QStringList files;
    QList<QAction*> recentFiles = _group->actions();
    for (int index = 0; index < recentFiles.count(); index++) {
        QString file = recentFiles[index]->toolTip();
        if (file.isEmpty())
            break;
        files.append(file);
    }

    return files;
}

void RecentMacrosAction::activateFile(int id)
{
    // restore the list of recent files
    QStringList files = this->files();
    if (id < 0 || id >= files.count())
        return; // no valid item

    QString filename = files[id];
    QFileInfo fi(filename);
    if (!fi.exists() || !fi.isFile()) {
        QMessageBox::critical(getMainWindow(), tr("File not found"), tr("The file '%1' cannot be opened.").arg(filename));
        files.removeAll(filename);
        setFiles(files);
    }
    else {
        if (QApplication::keyboardModifiers() == Qt::ShiftModifier){ //open for editing on Shift+click
            PythonEditor* editor = new PythonEditor();
            editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
            PythonEditorView* edit = new PythonEditorView(editor, getMainWindow());
            edit->setDisplayName(PythonEditorView::FileName);
            edit->open(filename);
            edit->resize(400, 300);
            getMainWindow()->addWindow(edit);
            getMainWindow()->appendRecentMacro(filename);
            edit->setWindowTitle(fi.fileName());
        } else { //execute macro on normal (non-shifted) click
            try {
                getMainWindow()->appendRecentMacro(fi.filePath());
                Application::Instance->macroManager()->run(Gui::MacroManager::File, fi.filePath().toUtf8());
                // after macro run recalculate the document
                if (Application::Instance->activeDocument())
                    Application::Instance->activeDocument()->getDocument()->recompute();
            }
            catch (const Base::SystemExitException&) {
                // handle SystemExit exceptions
                Base::PyGILStateLocker locker;
                Base::PyException e;
                e.ReportException();
            }
        }
    }
}

void RecentMacrosAction::resizeList(int size)
{
    this->visibleItems = size;
    int diff = this->visibleItems - this->maximumItems;
    // create new items if needed
    for (int i=0; i<diff; i++)
        _group->addAction(QLatin1String(""))->setVisible(false);
    setFiles(files());
}

/** Loads all recent files from the preferences. */
void RecentMacrosAction::restore()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences");
    if (hGrp->HasGroup("RecentMacros")) {
        hGrp = hGrp->GetGroup("RecentMacros");
        // we want at least 20 items but we do only show the number of files
        // that is defined in user parameters
        this->visibleItems = hGrp->GetInt("RecentMacros", this->visibleItems);
        this->shortcut_count = hGrp->GetInt("ShortcutCount", 3); // number of shortcuts
        this->shortcut_modifiers = hGrp->GetASCII("ShortcutModifiers","Ctrl+Shift+");
    }

    int count = std::max<int>(this->maximumItems, this->visibleItems);
    for (int i=_group->actions().size(); i<count; i++)
        _group->addAction(QLatin1String(""))->setVisible(false);
    std::vector<std::string> MRU = hGrp->GetASCIIs("MRU");
    QStringList files;
    for (std::vector<std::string>::iterator it = MRU.begin(); it!=MRU.end();++it)
        files.append(QString::fromUtf8(it->c_str()));
    setFiles(files);
}

/** Saves all recent files to the preferences. */
void RecentMacrosAction::save()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                                ->GetGroup("Preferences")->GetGroup("RecentMacros");
    int count = hGrp->GetInt("RecentMacros", this->visibleItems); // save number of files
    hGrp->Clear();

    // count all set items
    QList<QAction*> recentFiles = _group->actions();
    int num = std::min<int>(count, recentFiles.count());
    for (int index = 0; index < num; index++) {
        QString key = QString::fromLatin1("MRU%1").arg(index);
        QString value = recentFiles[index]->toolTip();
        if (value.isEmpty())
            break;
        hGrp->SetASCII(key.toLatin1(), value.toUtf8());
    }

    hGrp->SetInt("RecentMacros", count); // restore
    hGrp->SetInt("ShortcutCount", this->shortcut_count);
    hGrp->SetASCII("ShortcutModifiers",this->shortcut_modifiers.c_str());
}

// --------------------------------------------------------------------

UndoAction::UndoAction (Command* pcCmd,QObject * parent)
  : Action(pcCmd, parent)
{
    _toolAction = new QAction(this);
    _toolAction->setMenu(new UndoDialog());
    connect(_toolAction, SIGNAL(triggered(bool)), this, SLOT(onActivated()));
}

UndoAction::~UndoAction()
{
    QMenu* menu = _toolAction->menu();
    delete menu;
    delete _toolAction;
}

void UndoAction::addTo (QWidget * w)
{
    if (w->inherits("QToolBar")) {
        actionChanged();
        connect(_action, SIGNAL(changed()), this, SLOT(actionChanged()));
        w->addAction(_toolAction);
    }
    else {
        w->addAction(_action);
    }
}

void UndoAction::actionChanged()
{
    // Do NOT set the shortcut again for _toolAction since this is already
    // reserved for _action. Otherwise we get an ambiguity of it with the
    // result that it doesn't work anymore.
    _toolAction->setText(_action->text());
    _toolAction->setToolTip(_action->toolTip());
    _toolAction->setStatusTip(_action->statusTip());
    _toolAction->setWhatsThis(_action->whatsThis());
    _toolAction->setIcon(_action->icon());
}

void UndoAction::setEnabled(bool b)
{
    Action::setEnabled(b);
    _toolAction->setEnabled(b);
}

void UndoAction::setVisible(bool b)
{
    Action::setVisible(b);
    _toolAction->setVisible(b);
}

// --------------------------------------------------------------------

RedoAction::RedoAction ( Command* pcCmd,QObject * parent )
  : Action(pcCmd, parent)
{
    _toolAction = new QAction(this);
    _toolAction->setMenu(new RedoDialog());
    connect(_toolAction, SIGNAL(triggered(bool)), this, SLOT(onActivated()));
}

RedoAction::~RedoAction()
{
    QMenu* menu = _toolAction->menu();
    delete menu;
    delete _toolAction;
}

void RedoAction::addTo ( QWidget * w )
{
    if (w->inherits("QToolBar")) {
        actionChanged();
        connect(_action, SIGNAL(changed()), this, SLOT(actionChanged()));
        w->addAction(_toolAction);
    }
    else {
        w->addAction(_action);
    }
}

void RedoAction::actionChanged()
{
    // Do NOT set the shortcut again for _toolAction since this is already
    // reserved for _action. Otherwise we get an ambiguity of it with the
    // result that it doesn't work anymore.
    _toolAction->setText(_action->text());
    _toolAction->setToolTip(_action->toolTip());
    _toolAction->setStatusTip(_action->statusTip());
    _toolAction->setWhatsThis(_action->whatsThis());
    _toolAction->setIcon(_action->icon());
}

void RedoAction::setEnabled  ( bool b )
{
    Action::setEnabled(b);
    _toolAction->setEnabled(b);
}

void RedoAction::setVisible ( bool b )
{
    Action::setVisible(b);
    _toolAction->setVisible(b);
}

// --------------------------------------------------------------------

DockWidgetAction::DockWidgetAction ( Command* pcCmd, QObject * parent )
  : Action(pcCmd, parent), _menu(0)
{
}

DockWidgetAction::~DockWidgetAction()
{
    delete _menu;
}

void DockWidgetAction::addTo ( QWidget * w )
{
    if (!_menu) {
      _menu = new QMenu();
      _action->setMenu(_menu);
      connect(_menu, SIGNAL(aboutToShow()), getMainWindow(), SLOT(onDockWindowMenuAboutToShow()));
    }

    w->addAction(_action);
}

// --------------------------------------------------------------------

ToolBarAction::ToolBarAction ( Command* pcCmd, QObject * parent )
  : Action(pcCmd, parent), _menu(0)
{
}

ToolBarAction::~ToolBarAction()
{
    delete _menu;
}

void ToolBarAction::addTo ( QWidget * w )
{
    if (!_menu) {
      _menu = new QMenu();
      _action->setMenu(_menu);
      connect(_menu, SIGNAL(aboutToShow()), getMainWindow(), SLOT(onToolBarMenuAboutToShow()));
    }

    w->addAction(_action);
}

// --------------------------------------------------------------------

WindowAction::WindowAction ( Command* pcCmd, QObject * parent )
  : ActionGroup(pcCmd, parent), _menu(0)
{
}

WindowAction::~WindowAction()
{
}

void WindowAction::addTo ( QWidget * w )
{
    QMenu* menu = qobject_cast<QMenu*>(w);
    if (!menu) {
        if (!_menu) {
            _menu = new QMenu();
            _action->setMenu(_menu);
            _menu->addActions(_group->actions());
            connect(_menu, SIGNAL(aboutToShow()),
                    getMainWindow(), SLOT(onWindowsMenuAboutToShow()));
        }

        w->addAction(_action);
    }
    else {
        menu->addActions(_group->actions());
        connect(menu, SIGNAL(aboutToShow()),
                getMainWindow(), SLOT(onWindowsMenuAboutToShow()));
    }
}

// --------------------------------------------------------------------

ViewCameraBindingAction::ViewCameraBindingAction ( Command* pcCmd, QObject * parent )
  : Action(pcCmd, parent), _menu(0)
{
}

ViewCameraBindingAction::~ViewCameraBindingAction()
{
}

void ViewCameraBindingAction::addTo ( QWidget * w )
{
    if (!_menu) {
        _menu = new QMenu();
        setupMenuStyle(_menu);
        _action->setMenu(_menu);
        connect(_menu, SIGNAL(aboutToShow()), this, SLOT(onShowMenu()));
        connect(_menu, SIGNAL(triggered(QAction*)), this, SLOT(onTriggered(QAction*)));
    }
    w->addAction(_action);
}

void ViewCameraBindingAction::onShowMenu()
{
    _menu->clear();
    setupMenuStyle(_menu);

    auto activeView = Base::freecad_dynamic_cast<View3DInventor>(
            Application::Instance->activeView());
    if(!activeView)
        return;

    auto boundViews = activeView->boundViews();
    if(boundViews.size()) {
        if(boundViews.size() == 1) {
            auto action = _menu->addAction(tr("Sync camera"));
            action->setData(1);
        }
        auto action = _menu->addAction(tr("Unbind"));
        action->setData(2);
        _menu->addSeparator();
    }
    for(auto doc : App::GetApplication().getDocuments()) {
        auto gdoc = Application::Instance->getDocument(doc);
        if(!gdoc)
            continue;
        auto views = gdoc->getMDIViewsOfType(View3DInventor::getClassTypeId());
        for(auto it=views.begin();it!=views.end();) {
            auto view = static_cast<View3DInventor*>(*it);
            if(view == activeView ||
                    (!boundViews.count(view) && view->boundViews(true).count(activeView)))
                it = views.erase(it);
            else
                ++it;
        }
        if(views.empty())
            continue;
        if(views.size() == 1) {
            auto view = static_cast<View3DInventor*>(views.front());
            auto action = _menu->addAction(view->windowTitle());
            action->setCheckable(true);
            if(boundViews.count(view))
                action->setChecked(true);
            continue;
        }
        auto menu = _menu->addMenu(QString::fromUtf8(doc->Label.getValue()));
        for(auto view : views) {
            auto action = menu->addAction(view->windowTitle());
            action->setCheckable(true);
            if(boundViews.count(static_cast<View3DInventor*>(view)))
                action->setChecked(true);
        }
    }
}

void ViewCameraBindingAction::onTriggered(QAction *action)
{
    auto activeView = Base::freecad_dynamic_cast<View3DInventor>(
            Application::Instance->activeView());
    if(!activeView)
        return;

    switch(action->data().toInt()) {
    case 1: {
        auto views = activeView->boundViews();
        if(views.size())
            activeView->syncCamera(*views.begin());
        break;
    }
    case 2:
        activeView->unbindView();
        break;
    default:
        if (action->isChecked())
            activeView->bindView(action->text(), true);
        else
            activeView->unbindView(action->text());
        break;
    }
}

// --------------------------------------------------------------------

SelUpAction::SelUpAction ( Command* pcCmd, QObject * parent )
  : Action(pcCmd, parent), _menu(nullptr), _emptyAction(nullptr)
{
}

SelUpAction::~SelUpAction()
{
    delete _menu;
}

void SelUpAction::addTo ( QWidget * w )
{
    if (!_menu) {
        _menu = new SelUpMenu(nullptr);
        _action->setMenu(_menu);
        connect(_menu, SIGNAL(aboutToShow()), this, SLOT(onShowMenu()));
    }
    w->addAction(_action);
}

void SelUpAction::onShowMenu()
{
    _menu->clear();
    setupMenuStyle(_menu);
    TreeWidget::populateSelUpMenu(_menu);
}

void SelUpAction::popup(const QPoint &pt)
{
    if(_menu->actions().isEmpty()) {
        if (!_emptyAction) {
            _emptyAction = new QAction(tr("<None>"), this);
            _emptyAction->setDisabled(true);
        }
        _menu->addAction(_emptyAction);
    }
    TreeWidget::execSelUpMenu(qobject_cast<SelUpMenu*>(_menu), pt);
}

// --------------------------------------------------------------------

struct CmdInfo {
    Command *cmd = nullptr;
    QString text;
    QString tooltip;
    QIcon icon;
    bool iconChecked = false;
};
static std::vector<CmdInfo> _Commands;
static int _CommandRevision;

class CommandModel : public QAbstractItemModel
{
public:

public:
    CommandModel(QObject* parent)
        : QAbstractItemModel(parent)
    {
        update();
    }

    void update()
    {
        auto &manager = Application::Instance->commandManager();
        if (_CommandRevision == manager.getRevision())
            return;
        beginResetModel();
        _CommandRevision = manager.getRevision();
        _Commands.clear();
        for (auto &v : manager.getCommands()) {
            _Commands.emplace_back();
            auto &info = _Commands.back();
            info.cmd = v.second;
        }
        endResetModel();
    }

    virtual QModelIndex parent(const QModelIndex &) const
    {
        return QModelIndex();
    }

    virtual QVariant data(const QModelIndex & index, int role) const
    {
        if (index.row() < 0 || index.row() >= (int)_Commands.size())
            return QVariant();

        auto &info = _Commands[index.row()];

        switch(role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            if (info.text.isEmpty()) {
#if QT_VERSION>=QT_VERSION_CHECK(5,2,0)
                info.text = QString::fromLatin1("%2 (%1)").arg(
                        QString::fromLatin1(info.cmd->getName()),
                        qApp->translate(info.cmd->className(), info.cmd->getMenuText()));
#else
                info.text = qApp->translate(info.cmd->className(), info.cmd->getMenuText());
#endif
                info.text.replace(QLatin1Char('&'), QString());
                if (info.text.isEmpty())
                    info.text = QString::fromLatin1(info.cmd->getName());
            }
            return info.text;

        case Qt::DecorationRole:
            if (!info.iconChecked) {
                info.iconChecked = true;
                if(info.cmd->getPixmap())
                    info.icon = BitmapFactory().iconFromTheme(info.cmd->getPixmap());
            }
            return info.icon;

        case Qt::ToolTipRole:
            if (info.tooltip.isEmpty()) {
                info.tooltip = QString::fromLatin1("%1: %2").arg(
                        QString::fromLatin1(info.cmd->getName()),
                        qApp->translate(info.cmd->className(), info.cmd->getMenuText()));
                QString tooltip = qApp->translate(info.cmd->className(), info.cmd->getToolTipText());
                if (tooltip.size())
                    info.tooltip += QString::fromLatin1("\n\n") + tooltip;
            }
            return info.tooltip;

        case Qt::UserRole:
            return QByteArray(info.cmd->getName());

        default:
            return QVariant();
        }
    }

    virtual QModelIndex index(int row, int, const QModelIndex &) const
    {
        return this->createIndex(row, 0);
    }

    virtual int rowCount(const QModelIndex &) const
    {
        return (int)(_Commands.size());
    }

    virtual int columnCount(const QModelIndex &) const
    {
        return 1;
    }
};


// --------------------------------------------------------------------

CommandCompleter::CommandCompleter(QLineEdit *lineedit, QObject *parent)
    : QCompleter(parent)
{
    this->setModel(new CommandModel(this));
#if QT_VERSION>=QT_VERSION_CHECK(5,2,0)
    this->setFilterMode(Qt::MatchContains);
#endif
    this->setCaseSensitivity(Qt::CaseInsensitive);
    this->setCompletionMode(QCompleter::PopupCompletion);
    this->setWidget(lineedit);
    connect(lineedit, SIGNAL(textEdited(QString)), this, SLOT(onTextChanged(QString)));
    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(onCommandActivated(QModelIndex)));
    connect(this, SIGNAL(highlighted(QString)), lineedit, SLOT(setText(QString)));
}

bool CommandCompleter::eventFilter(QObject *o, QEvent *ev)
{
    if (ev->type() == QEvent::KeyPress
            && (o == this->widget() || o == this->popup()))
    {
        QKeyEvent * ke = static_cast<QKeyEvent*>(ev);
        switch(ke->key()) {
        case Qt::Key_Escape: {
            auto edit = qobject_cast<QLineEdit*>(this->widget());
            if (edit && edit->text().size()) {
                edit->setText(QString());
                popup()->hide();
                return true;
            } else if (popup()->isVisible()) {
                popup()->hide();
                return true;
            }
            break;
        }
        case Qt::Key_Tab: {
            if (this->popup()->isVisible()) {
                QKeyEvent kevent(ke->type(),Qt::Key_Down,0);
                qApp->sendEvent(this->popup(), &kevent);
                return true;
            }
            break;
        }
        case Qt::Key_Backtab: {
            if (this->popup()->isVisible()) {
                QKeyEvent kevent(ke->type(),Qt::Key_Up,0);
                qApp->sendEvent(this->popup(), &kevent);
                return true;
            }
            break;
        }
        default:
            break;
        }
    }
    return QCompleter::eventFilter(o, ev);
}

void CommandCompleter::onCommandActivated(const QModelIndex &index)
{
    QByteArray name = completionModel()->data(index, Qt::UserRole).toByteArray();
    Q_EMIT commandActivated(name);
}

void CommandCompleter::onTextChanged(const QString &txt)
{
    if (txt.size() < 3 || !widget())
        return;

    static_cast<CommandModel*>(this->model())->update();

    this->setCompletionPrefix(txt);
    QRect rect = widget()->rect();
    if (rect.width() < 300)
        rect.setWidth(300);
    this->complete(rect);
}

// --------------------------------------------------------------------

class CmdHistoryMenu: public QMenu
{
public:
    CmdHistoryMenu(QWidget *focus)
        :focusWidget(focus)
    {}
protected:
    void keyPressEvent(QKeyEvent *e)
    {
        if (e->key() == Qt::Key_Space) {
            if (!isVisible()) {
                keyFocus = true;
                exec(QCursor::pos());
            } else {
                focusWidget->setFocus();
                e->accept();
            }
            return;
        }
        QMenu::keyPressEvent(e);
        return;
    }

    bool event(QEvent *e)
    {
        if (e->type() == QEvent::ShortcutOverride) {
            auto ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Space
                    && ke->modifiers() == Qt::NoModifier)
            {
                e->accept();
                return true;
            }
        }
        return QMenu::event(e);
    }

    void showEvent(QShowEvent *ev)
    {
        QMenu::showEvent(ev);
        if (keyFocus) {
            keyFocus = false;
            focusWidget->setFocus();
        }
    }

public:
    QWidget *focusWidget;
    bool keyFocus = false;
};

// --------------------------------------------------------------------
CmdHistoryAction::CmdHistoryAction ( Command* pcCmd, QObject * parent )
  : Action(pcCmd, parent)
{
    qApp->installEventFilter(this);
}

CmdHistoryAction::~CmdHistoryAction()
{
    delete _menu;
}

void CmdHistoryAction::addTo ( QWidget * w )
{
    if (!_menu) {
        _lineedit = new QLineEdit;
        _lineedit->setPlaceholderText(tr("Press SPACE to search"));
        _widgetAction = new QWidgetAction(this);
        _widgetAction->setDefaultWidget(_lineedit);
        _completer = new CommandCompleter(_lineedit, this);
        connect(_completer, SIGNAL(commandActivated(QByteArray)), this, SLOT(onCommandActivated(QByteArray)));

        _newAction = new QAction(tr("Add toolbar or menu"), this);
        _newAction->setToolTip(tr("Create a Global customized toolbar or menu.\n"
                                  "Or, hold SHIFT key to create a toolbar/menu\n"
                                  "for the current workbench."));
        connect(_newAction, SIGNAL(triggered(bool)), this, SLOT(onNewAction()));

        _menu = new CmdHistoryMenu(_lineedit);
        setupMenuStyle(_menu);
        _menu->addAction(_widgetAction);
        _menu->addAction(_newAction);
        _action->setMenu(_menu);
        connect(_menu, SIGNAL(aboutToShow()), this, SLOT(onShowMenu()));
    }

    w->addAction(_action);
}

static long _RecentCommandID;
static std::map<long, const char *, std::greater<long> > _RecentCommands;
static std::unordered_map<std::string, long> _RecentCommandMap;
static long _RecentCommandPopulated;
static QElapsedTimer _ButtonTime;

std::vector<Command*> CmdHistoryAction::recentCommands()
{
    auto &manager = Application::Instance->commandManager();
    std::vector<Command*> cmds;
    cmds.reserve(_RecentCommands.size());
    for (auto &v : _RecentCommands) {
        auto cmd = manager.getCommandByName(v.second);
        if (cmd)
            cmds.push_back(cmd);
    }
    return cmds;
}

void CmdHistoryAction::onNewAction()
{
    Application::Instance->commandManager().runCommandByName("Std_DlgCustomize", 1);
}

bool CmdHistoryAction::eventFilter(QObject *, QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::MouseButtonPress: {
        auto e = static_cast<QMouseEvent*>(ev);
        if (e->button() == Qt::LeftButton)
            _ButtonTime.start();
        break;
    }
    default:
        break;
    }
    return false;
}

void CmdHistoryAction::onInvokeCommand(const char *name, bool force)
{
    if(!force && (!_ButtonTime.isValid() || _ButtonTime.elapsed() > 1000))
        return;

    _ButtonTime.invalidate();

    auto &manager = Application::Instance->commandManager();
    Command *cmd = manager.getCommandByName(name);
    if (!cmd || qobject_cast<CmdHistoryAction*>(cmd->getAction())
             || qobject_cast<ToolbarMenuAction*>(cmd->getAction()))
        return;
    
    if (!force && (cmd->getType() & Command::NoHistory) && !_RecentCommandMap.count(name))
        return;

    auto res = _RecentCommandMap.insert(std::make_pair(name, 0));
    if (!res.second)
        _RecentCommands.erase(res.first->second);
    res.first->second = ++_RecentCommandID;
    _RecentCommands[_RecentCommandID] = res.first->first.c_str();
    if (ViewParams::getCommandHistorySize() < (int)_RecentCommandMap.size()) {
        auto it = _RecentCommands.end();
        --it;
        _RecentCommandMap.erase(it->second);
        _RecentCommands.erase(it);
    }
}

void CmdHistoryAction::onShowMenu()
{
    setupMenuStyle(_menu);

    _menu->setFocus(Qt::PopupFocusReason);
    _lineedit->setText(QString());

    if (_RecentCommandPopulated == _RecentCommandID)
        return;

    _RecentCommandPopulated = _RecentCommandID;
    _menu->clear();
    _menu->addAction(_widgetAction);
    _menu->addAction(_newAction);
    _menu->addSeparator();
    auto &manager = Application::Instance->commandManager();
    for (auto &v : _RecentCommands)
        manager.addTo(v.second, _menu);
}

void CmdHistoryAction::onCommandActivated(const QByteArray &name)
{
    _menu->hide();

    auto &manager = Application::Instance->commandManager();
    if (name.size()) {
        manager.runCommandByName(name.constData());
        onInvokeCommand(name.constData(), true);
    }
}

void CmdHistoryAction::popup(const QPoint &pt)
{
    PieMenu::exec(_menu, pt, _pcCmd->getName(), true);
}

// --------------------------------------------------------------------

class ToolbarMenuAction::Private: public ParameterGrp::ObserverType
{
public:
    Private(ToolbarMenuAction *master, const char *path):master(master)
    {
        handle = App::GetApplication().GetParameterGroupByPath(path);
    }

    void OnChange(Base::Subject<const char*> &, const char *)
    {
        if (!updating) {
            Base::StateLocker guard(updating);
            master->update();
        }
    }

public:
    ToolbarMenuAction *master;
    ParameterGrp::handle handle;
    ParameterGrp::handle hShortcut;
    std::set<std::string> cmds;
    bool updating = false;
};

// --------------------------------------------------------------------

class GuiExport ToolbarMenuSubAction : public ToolbarMenuAction
{
public:
    ToolbarMenuSubAction (Command* pcCmd, ParameterGrp::handle hGrp, QObject * parent = 0)
        : ToolbarMenuAction(pcCmd, parent)
    {
        _pimpl->handle = hGrp;
        _pimpl->handle->Attach(_pimpl.get());
    }

    ~ToolbarMenuSubAction() {
        _pimpl->handle->Detach(_pimpl.get());
    }

protected:
    virtual void onShowMenu() {
        setupMenuStyle(_menu);

        auto &manager = Application::Instance->commandManager();
        if (revision == manager.getRevision())
            return;
        _menu->clear();
        for (auto &v : _pimpl->handle->GetASCIIMap()) {
            if (v.first == "Name")
                setText(QString::fromUtf8(v.second.c_str()));
            else if (boost::starts_with(v.first, "Separator"))
                _menu->addSeparator();
            else
                manager.addTo(v.first.c_str(), _menu);
        }
        revision = manager.getRevision();
    }

    virtual void update()
    {
        revision = 0;
        std::string text = _pimpl->handle->GetASCII("Name", "");
        this->setText(QString::fromUtf8(text.c_str()));
    }

private:
    int revision = 0;
};

// --------------------------------------------------------------------

class StdCmdToolbarSubMenu : public Gui::Command
{
public:
    StdCmdToolbarSubMenu(const char *name, ParameterGrp::handle hGrp)
        :Command(name)
    {
        menuText      = hGrp->GetASCII("Name", "Custom");
        sGroup        = QT_TR_NOOP("Tools");
        sMenuText     = menuText.c_str();
        sWhatsThis    = "Std_CmdToolbarSubMenu";
        eType         = NoTransaction | NoHistory;

        _pcAction = new ToolbarMenuSubAction(this, hGrp, getMainWindow());
        applyCommandData(this->className(), _pcAction);
    }

    virtual ~StdCmdToolbarSubMenu() {}

    virtual const char* className() const
    { return "StdCmdToolbarSubMenu"; }

protected:
    virtual void activated(int) {
        if (_pcAction)
            static_cast<ToolbarMenuSubAction*>(_pcAction)->popup(QCursor::pos());
    }
    virtual bool isActive(void) { return true;}

    virtual Gui::Action * createAction(void) {
        assert(false);
        return nullptr;
    }

private:
    std::string menuText;
};

// --------------------------------------------------------------------

ToolbarMenuAction::ToolbarMenuAction ( Command* pcCmd, QObject * parent )
  : Action(pcCmd, parent), _menu(0)
  , _pimpl(new Private(this, "User parameter:BaseApp/Workbench/Global/Toolbar"))
{
    _pimpl->hShortcut = WindowParameter::getDefaultParameter()->GetGroup("Shortcut");
    _pimpl->hShortcut->Attach(_pimpl.get());

    Application::Instance->signalActivateWorkbench.connect(
        [](const char*) {
            ToolbarMenuAction::populate();
        });
}

ToolbarMenuAction::~ToolbarMenuAction()
{
    _pimpl->hShortcut->Detach(_pimpl.get());
    delete _menu;
}

void ToolbarMenuAction::addTo ( QWidget * w )
{
    if (!_menu) {
        _menu = new QMenu;
        setupMenuStyle(_menu);
        _action->setMenu(_menu);
        update();
        connect(_menu, SIGNAL(aboutToShow()), this, SLOT(onShowMenu()));
    }
    w->addAction(_action);
}

void ToolbarMenuAction::onShowMenu()
{
    setupMenuStyle(_menu);
}

void ToolbarMenuAction::populate()
{
    auto &manager = Application::Instance->commandManager();
    auto cmd = manager.getCommandByName("Std_CmdToolbarMenus");
    if (!cmd)
        return;
    auto action = qobject_cast<ToolbarMenuAction*>(cmd->getAction());
    if (action)
        action->update();
}

std::string ToolbarMenuAction::paramName(const char *name, const char *workbench)
{
    if (!name)
        name = "";

    if (!workbench || strcmp(workbench, "Global")==0)
        return std::string("Std_ToolbarMenu_") + name;

    std::string res("Std_WBMenu_");
    res += workbench;
    if (name[0])
        res += "_";
    return res + name;
}

void ToolbarMenuAction::update()
{
    if (_pimpl->updating)
        return;
    Base::StateLocker guard(_pimpl->updating);

    auto &manager = Application::Instance->commandManager();
    _menu->clear();
    std::set<std::string> cmds;

    auto addCommand = [&](ParameterGrp::handle hGrp, const char *workbench) {
        std::string name = paramName(hGrp->GetGroupName(), workbench);
        QString shortcut = QString::fromLatin1(_pimpl->hShortcut->GetASCII(name.c_str()).c_str());
        if (shortcut.isEmpty())
            return;

        if (!cmds.insert(name).second)
            return;

        auto res = _pimpl->cmds.insert(name);
        Command *cmd = manager.getCommandByName(name.c_str());
        if (!cmd) {
            cmd = new StdCmdToolbarSubMenu(res.first->c_str(), hGrp);
            manager.addCommand(cmd);
        }
        if (cmd->getAction() && cmd->getAction()->shortcut() != shortcut)
            cmd->getAction()->setShortcut(shortcut);
        cmd->addTo(_menu);
    };

    for (auto &hGrp : _pimpl->handle->GetGroups()) {
        if(hGrp->GetASCII("Name","").empty())
            continue;
        addCommand(hGrp, nullptr);
    }

    auto wb = WorkbenchManager::instance()->active();
    if (wb) {
        auto hGrp = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Workbench");
        if (hGrp->HasGroup(wb->name().c_str())) {
            hGrp = hGrp->GetGroup(wb->name().c_str());
            if (hGrp->HasGroup("Toolbar")) {
                for (auto h : hGrp->GetGroup("Toolbar")->GetGroups()) {
                    if(h->GetASCII("Name","").empty())
                        continue;
                    addCommand(h, wb->name().c_str());
                }
            }
        }
    }

    for (auto it=_pimpl->cmds.begin(); it!=_pimpl->cmds.end();) {
        if (cmds.count(*it)) {
            ++it;
            continue;
        }
        Command *cmd = manager.getCommandByName(it->c_str());
        if (cmd)
            manager.removeCommand(cmd);
        it = _pimpl->cmds.erase(it);
    }
}

void ToolbarMenuAction::popup(const QPoint &pt)
{
    PieMenu::exec(_menu, pt, _pcCmd->getName());
    _menu->setActiveAction(0);
}

////////////////////////////////////////////////////////////////////

class ExpressionAction::Private {
public:
    void init(QMenu *menu)
    {
        pcActionCopySel = menu->addAction(QObject::tr("Copy selected"));
        pcActionCopyActive = menu->addAction(QObject::tr("Copy active document"));
        pcActionCopyAll = menu->addAction(QObject::tr("Copy all documents"));
        pcActionPaste = menu->addAction(QObject::tr("Paste"));
    }

    void onAction(QAction *action) {
        if (action == pcActionPaste) {
            pasteExpressions();
            return;
        }

        std::map<App::Document*, std::set<App::DocumentObject*> > objs;
        if (action == pcActionCopySel) {
            for(auto &sel : Selection().getCompleteSelection())
                objs[sel.pObject->getDocument()].insert(sel.pObject);
        }
        else if (action == pcActionCopyActive) {
            if(App::GetApplication().getActiveDocument()) {
                auto doc = App::GetApplication().getActiveDocument();
                auto array = doc->getObjects();
                auto &set = objs[doc];
                set.insert(array.begin(),array.end());
            }
        }
        else if (action == pcActionCopyAll) {
            for(auto doc : App::GetApplication().getDocuments()) {
                auto &set = objs[doc];
                auto array = doc->getObjects();
                set.insert(array.begin(),array.end());
            }
        }
        copyExpressions(objs);
    }

    void copyExpressions(const std::map<App::Document*, std::set<App::DocumentObject*> > &objs)
    {
        std::ostringstream ss;
        std::vector<App::Property*> props;
        for(auto &v : objs) {
            for(auto obj : v.second) {
                props.clear();
                obj->getPropertyList(props);
                for(auto prop : props) {
                    auto p = dynamic_cast<App::PropertyExpressionContainer*>(prop);
                    if(!p) continue;
                    for(auto &v : p->getExpressions()) {
                        ss << "##@@ " << v.first.toString() << ' '
                            << obj->getFullName() << '.' << p->getName()
                            << " (" << obj->Label.getValue() << ')' << std::endl;
                        ss << "##@@";
                        if(v.second->comment.size()) {
                            if(v.second->comment[0] == '&' 
                                    || v.second->comment.find('\n') != std::string::npos
                                    || v.second->comment.find('\r') != std::string::npos)
                            {
                                std::string comment = v.second->comment;
                                boost::replace_all(comment,"&","&amp;");
                                boost::replace_all(comment,"\n","&#10;");
                                boost::replace_all(comment,"\r","&#13;");
                                ss << '&' << comment;
                            }else
                                ss << v.second->comment;
                        }
                        ss << std::endl << v.second->toStr(true,true) << std::endl << std::endl;
                    }
                }
            }
        }
        QApplication::clipboard()->setText(QString::fromUtf8(ss.str().c_str()));
    }

    void pasteExpressions() {
        std::map<App::Document*, std::map<App::PropertyExpressionContainer*, 
            std::map<App::ObjectIdentifier, App::ExpressionPtr> > > exprs;

        bool failed = false;
        std::string txt = QApplication::clipboard()->text().toUtf8().constData();
        const char *tstart = txt.c_str();
        const char *tend = tstart + txt.size();

        static boost::regex rule("^##@@ ([^ ]+) (\\w+)#(\\w+)\\.(\\w+) [^\n]+\n##@@([^\n]*)\n");
        boost::cmatch m;
        if(!boost::regex_search(tstart,m,rule)) {
            FC_WARN("No expression header found");
            return;
        }
        boost::cmatch m2;
        bool found = true;
        for(;found;m=m2) {
            found = boost::regex_search(m[0].second,tend,m2,rule);

            auto pathName = m.str(1);
            auto docName = m.str(2);
            auto objName = m.str(3);
            auto propName = m.str(4);
            auto comment = m.str(5);

            App::Document *doc = App::GetApplication().getDocument(docName.c_str());
            if(!doc) {
                FC_WARN("Cannot find document '" << docName << "'");
                continue;
            }

            auto obj = doc->getObject(objName.c_str());
            if(!obj) {
                FC_WARN("Cannot find object '" << docName << '#' << objName << "'");
                continue;
            }

            auto prop = dynamic_cast<App::PropertyExpressionContainer*>(
                    obj->getPropertyByName(propName.c_str()));
            if(!prop) {
                FC_WARN("Invalid property '" << docName << '#' << objName << '.' << propName << "'");
                continue;
            }

            size_t len = (found?m2[0].first:tend) - m[0].second;
            try {
                // Check if the expression body contains a single
                // non-whitespace character '#'.  This is used to signal the
                // user's intention of unbinding the property.
                bool empty = false;
                const char *t = m[0].second;
                for(;*t && std::isspace((int)*t);++t);
                if(*t == '#') {
                    for(++t;*t && std::isspace((int)*t);++t);
                    empty = !*t;
                }
                App::ExpressionPtr expr;
                if(!empty)
                    expr = App::Expression::parse(obj,std::string(m[0].second,len));
                if(expr && comment.size()) {
                    if(comment[0] == '&') {
                        expr->comment = comment.c_str()+1;
                        boost::replace_all(expr->comment,"&amp;","&");
                        boost::replace_all(expr->comment,"&#10;","\n");
                        boost::replace_all(expr->comment,"&#13;","\r");
                    } else
                        expr->comment = comment;
                }
                exprs[doc][prop][App::ObjectIdentifier::parse(obj,pathName)] = std::move(expr);
            } catch(Base::Exception &e) {
                FC_ERR(e.what() << std::endl << m[0].str());
                failed = true;
            }
        }
        if(failed) {
            QMessageBox::critical(getMainWindow(), QObject::tr("Expression error"),
                QObject::tr("Failed to parse some of the expressions.\n"
                            "Please check the Report View for more details."));
            return;
        }

        App::AutoTransaction guard("Paste expressions");
        try {
            for(auto &v : exprs) {
                for(auto &v2 : v.second) {
                    auto &expressions = v2.second;
                    auto old = v2.first->getExpressions();
                    for(auto it=expressions.begin(),itNext=it;it!=expressions.end();it=itNext) {
                        ++itNext;
                        if(!it->second)
                            continue;
                        auto iter = old.find(it->first);
                        if(iter != old.end() && it->second->isSame(*iter->second))
                            expressions.erase(it);
                    }
                    if(expressions.size())
                        v2.first->setExpressions(std::move(expressions));
                }
            }
        } catch (const Base::Exception& e) {
            e.ReportException();
            App::GetApplication().closeActiveTransaction(true);
            QMessageBox::critical(getMainWindow(), QObject::tr("Failed to paste expressions"),
                QString::fromLatin1(e.what()));
        }
    }

    void update() {
        if(!App::GetApplication().getActiveDocument()) {
            pcActionCopyAll->setEnabled(false);
            pcActionCopySel->setEnabled(false);
            pcActionCopyActive->setEnabled(false);
            pcActionPaste->setEnabled(false);
            return;
        }
        pcActionCopyActive->setEnabled(true);
        pcActionCopyAll->setEnabled(true);
        pcActionCopySel->setEnabled(Selection().hasSelection());

        QString txt = QApplication::clipboard()->text();
        pcActionPaste->setEnabled(txt.startsWith(QLatin1String("##@@ ")));
    }

    QAction *pcActionCopyAll;
    QAction *pcActionCopySel;
    QAction *pcActionCopyActive;
    QAction *pcActionPaste;
};

ExpressionAction::ExpressionAction (Command* pcCmd, QObject * parent)
    : Action(pcCmd, parent), _menu(nullptr), _pimpl(new Private)
{
}

ExpressionAction::~ExpressionAction ()
{
}

void ExpressionAction::addTo ( QWidget * w )
{
    if (!_menu) {
        _menu = new QMenu();
        _action->setMenu(_menu);
        connect(_menu, SIGNAL(aboutToShow()), this, SLOT(onShowMenu()));
        connect(_menu, SIGNAL(triggered(QAction*)), this, SLOT(onAction(QAction*)));

        _pimpl->init(_menu);
    }

    w->addAction(_action);
}

void ExpressionAction::onAction(QAction *action) {
    _pimpl->onAction(action);
}

void ExpressionAction::onShowMenu()
{
    _pimpl->update();
}

void ExpressionAction::popup(const QPoint &pt)
{
    if (_menu)
        _menu->exec(pt);
}

// --------------------------------------------------------------------

PresetsAction::PresetsAction ( Command* pcCmd, QObject * parent )
  : Action(pcCmd, parent), _menu(0)
{
}

PresetsAction::~PresetsAction()
{
    delete _menu;
}

void PresetsAction::addTo ( QWidget * w )
{
    if (!_menu) {
      _menu = new QMenu();
      _menu->setToolTipsVisible(true);
      _action->setMenu(_menu);
      connect(_menu, SIGNAL(aboutToShow()), this, SLOT(onShowMenu()));
      connect(_menu, SIGNAL(triggered(QAction*)), this, SLOT(onAction(QAction*)));
    }

    w->addAction(_action);
}

void PresetsAction::onAction(QAction *action) {
    auto param = App::GetApplication().GetParameterSet(
            action->data().toByteArray().constData());
    if (param) {
        if (QApplication::queryKeyboardModifiers() == Qt::ControlModifier)
            App::GetApplication().GetUserParameter().revert(param);
        else
            param->insertTo(&App::GetApplication().GetUserParameter());
    }
}

void PresetsAction::onShowMenu()
{
    _menu->clear();
    QString tooltip = tr("Click to apply the setting.\nCtrl + Click to revert to default.");
    for (auto &v : App::GetApplication().GetParameterSetList()) {
        if (v.second == &App::GetApplication().GetUserParameter()
                || v.second == &App::GetApplication().GetSystemParameter()
                || !v.second->GetBool("Preset", true))
            continue;
        auto action = _menu->addAction(QString::fromUtf8(
                    v.second->GetASCII("Name", v.first.c_str()).c_str()));
        QString t = QString::fromUtf8(v.second->GetASCII("ToolTip").c_str());
        if (t.size())
            t += QStringLiteral("\n\n");
        t += tooltip;
        action->setToolTip(t);
        action->setData(QByteArray(v.first.c_str()));
    }
}

void PresetsAction::popup(const QPoint &pt)
{
    PieMenu::exec(_menu, pt, _pcCmd->getName(), true);
}

#include "moc_Action.cpp"
