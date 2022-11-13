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
# include <QActionEvent>
# include <QActionGroup>
# include <QApplication>
# include <QEvent>
# include <QMenu>
# include <QMessageBox>
# include <QRegularExpression>
# include <QScreen>
# include <QTimer>
# include <QToolBar>
# include <QToolButton>
# include <QToolTip>
# include <QMenuBar>
#endif

#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <App/Document.h>

#include "Action.h"
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
#include "UserSettings.h"
#include "WhatsThis.h"
#include "Widgets.h"
#include "Workbench.h"
#include "ShortcutManager.h"
#include "Tools.h"


using namespace Gui;
using namespace Gui::Dialog;
namespace bp = boost::placeholders;

/**
 * Constructs an action called \a name with parent \a parent. It also stores a pointer
 * to the command object.
 */
Action::Action (Command* pcCmd, QObject * parent)
  : QObject(parent)
  , _action(new QAction( this ))
  , _pcCmd(pcCmd)
{
    _action->setObjectName(QString::fromLatin1(_pcCmd->getName()));
    _connection = connect(_action, &QAction::triggered, this, &Action::onActivated);
}

Action::Action (Command* pcCmd, QAction* action, QObject * parent)
  : QObject(parent)
  , _action(action)
  , _pcCmd(pcCmd)
{
    _action->setParent(this);
    _action->setObjectName(QString::fromLatin1(_pcCmd->getName()));
    _connection = connect(_action, &QAction::triggered, this, &Action::onActivated);
}

Action::~Action()
{
    delete _action;
}

/**
 * Adds this action to widget \a widget.
 */
void Action::addTo(QWidget *widget)
{
    widget->addAction(_action);
}

/**
 * Activates the command.
 */
void Action::onActivated ()
{
    command()->invoke(0, Command::TriggerAction);
}

/**
 * Sets whether the command is toggled.
 */
void Action::onToggled(bool toggle)
{
    command()->invoke(toggle ? 1 : 0, Command::TriggerAction);
}

void Action::setCheckable(bool check)
{
    if (check == _action->isCheckable()) {
        return;
    }

    _action->setCheckable(check);

    if (check) {
        disconnect(_connection);
        _connection = connect(_action, &QAction::toggled, this, &Action::onToggled);
    }
    else {
        disconnect(_connection);
        _connection = connect(_action, &QAction::triggered, this, &Action::onActivated);
    }
}

void Action::setChecked(bool check, bool no_signal)
{
    bool blocked = false;
    if (no_signal) {
        blocked = _action->blockSignals(true);
    }
    _action->setChecked(check);
    if (no_signal) {
        _action->blockSignals(blocked);
    }
}

bool Action::isChecked() const
{
    return _action->isChecked();
}

/**
 * Sets whether the action is enabled.
 */
void Action::setEnabled(bool enable)
{
    _action->setEnabled(enable);
}

bool Action::isEnabled() const
{
    return _action->isEnabled();
}

void Action::setVisible(bool visible)
{
    _action->setVisible(visible);
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

void Action::setStatusTip(const QString & text)
{
    _action->setStatusTip(text);
}

QString Action::statusTip() const
{
    return _action->statusTip();
}

void Action::setText(const QString & text)
{
    _action->setText(text);
    if (_title.isEmpty()) {
        setToolTip(_tooltip);
    }
}

QString Action::text() const
{
    return _action->text();
}

void Action::setToolTip(const QString & text, const QString & title)
{
    _tooltip = text;
    _title = title;
    _action->setToolTip(createToolTip(text,
                title.isEmpty() ? _action->text() : title, 
                _action->font(),
                _action->shortcut().toString(QKeySequence::NativeText),
                command()));
}

QString Action::cleanTitle(const QString & title)
{
    QString text(title);
    // Deal with QAction title mnemonic
    static QRegularExpression re(QStringLiteral("&(.)"));
    text.replace(re, QStringLiteral("\\1"));

    // Probably not a good idea to trim ending punctuation
#if 0
    // Trim line ending punctuation
    static QRegularExpression rePunct(QStringLiteral("[[:punct:]]+$"));
    text.replace(rePunct, QString());
#endif
    return text;
}

QString Action::commandToolTip(const Command *cmd, bool richFormat)
{
    if (!cmd) {
        return QString();
    }

    if (richFormat) {
        if (auto action = cmd->getAction()) {
            return action->_action->toolTip();
        }
    }

    QString title, tooltip;
    if (dynamic_cast<const MacroCommand*>(cmd)) {
        if (auto txt = cmd->getMenuText()) {
            title = QString::fromUtf8(txt);
        }
        if (auto txt = cmd->getToolTipText()) {
            tooltip = QString::fromUtf8(txt);
        }
    } else {
        if (auto txt = cmd->getMenuText()) {
            title = qApp->translate(cmd->className(), txt);
        }
        if (auto txt = cmd->getToolTipText()) {
            tooltip = qApp->translate(cmd->className(), txt);
        }
    }

    if (!richFormat) {
        return tooltip;
    }
    return createToolTip(tooltip, title, QFont(), cmd->getShortcut(), cmd);
}

QString Action::commandMenuText(const Command *cmd)
{
    if (!cmd) {
        return QString();
    }

    QString title;
    if (auto action = cmd->getAction()) {
        title = action->text();
    }
    else if (dynamic_cast<const MacroCommand*>(cmd)) {
        if (auto txt = cmd->getMenuText()) {
            title = QString::fromUtf8(txt);
        }
    } else {
        if (auto txt = cmd->getMenuText()) {
            title = qApp->translate(cmd->className(), txt);
        }
    }
    if (title.isEmpty()) {
        title = QString::fromUtf8(cmd->getName());
    }
    else {
        title = cleanTitle(title);
    }
    return title;
}

QString Action::createToolTip(QString helpText,
                              const QString & title,
                              const QFont &font,
                              const QString &shortCut,
                              const Command *command)
{
    QString text = cleanTitle(title);

    if (text.isEmpty()) {
        return helpText;
    }

    // The following code tries to make a more useful tooltip by inserting at
    // the beginning of the tooltip the action title in bold followed by the
    // shortcut.
    //
    // The long winding code is to deal with the fact that Qt will auto wrap
    // a rich text tooltip but the width is too short. We can escape the auto
    // wrappin using <p style='white-space:pre'>.

    QString shortcut = shortCut;
    if (shortcut.size() && helpText.endsWith(shortcut)) {
        helpText.resize(helpText.size() - shortcut.size());
    }
    if (shortcut.size()) {
        shortcut = QString::fromLatin1(" (%1)").arg(shortcut);
    }

    QString tooltip = QString::fromLatin1(
            "<p style='white-space:pre; margin-bottom:0.5em;'><b>%1</b>%2</p>").arg(
            text.toHtmlEscaped(), shortcut.toHtmlEscaped());

    QString cmdName;
    if (command && command->getName()) {
        cmdName = QString::fromLatin1(command->getName());
        if (auto groupcmd = dynamic_cast<const GroupCommand*>(command)) {
            if (auto act = command->getAction()) {
                int idx = act->property("defaultAction").toInt();
                auto cmd = groupcmd->getCommand(idx);
                if (cmd && cmd->getName()) {
                    cmdName = QStringLiteral("%1 (%2:%3)")
                        .arg(QString::fromLatin1(cmd->getName()), cmdName)
                        .arg(idx);
                }
            }
        }
        cmdName = QStringLiteral("<p style='white-space:pre; margin-top:0.5em;'><i>%1</i></p>")
            .arg(cmdName.toHtmlEscaped());
    }

    if (shortcut.size() && helpText.endsWith(shortcut)) {
        helpText.resize(helpText.size() - shortcut.size());
    }

    if (helpText.isEmpty()
            || helpText == text
            || helpText == title)
    {
        return tooltip + cmdName;
    }
    if (Qt::mightBeRichText(helpText)) {
        // already rich text, so let it be to avoid duplicated unwrapping
        return tooltip + helpText + cmdName;
    }

    tooltip += QString::fromLatin1(
            "<p style='white-space:pre; margin:0;'>");

    // If the user supplied tooltip contains line break, we shall honour it.
    if (helpText.indexOf(QLatin1Char('\n')) >= 0) {
        tooltip += helpText.toHtmlEscaped() + QString::fromLatin1("</p>") ;
    }
    else {
        // If not, try to end the non wrapping paragraph at some pre defined
        // width, so that the following text can wrap at that width.
        float tipWidth = 400;
        QFontMetrics fm(font);
        int width = QtTools::horizontalAdvance(fm, helpText);
        if (width <= tipWidth) {
            tooltip += helpText.toHtmlEscaped() + QString::fromLatin1("</p>") ;
        }
        else {
            int index = tipWidth / width * helpText.size();
            // Try to only break at white space
            for(int i=0; i<50 && index<helpText.size(); ++i, ++index) {
                if (helpText[index] == QLatin1Char(' ')) {
                    break;
                }
            }
            tooltip += helpText.left(index).toHtmlEscaped()
                + QString::fromLatin1("</p>")
                + helpText.right(helpText.size()-index).trimmed().toHtmlEscaped();
        }
    }
    return tooltip + cmdName;
}

QString Action::toolTip() const
{
    return _tooltip;
}

void Action::setWhatsThis(const QString & text)
{
    _action->setWhatsThis(text);
}

QString Action::whatsThis() const
{
    return _action->whatsThis();
}

void Action::setMenuRole(QAction::MenuRole menuRole)
{
    _action->setMenuRole(menuRole);
}

// --------------------------------------------------------------------

/**
 * Constructs an action called \a name with parent \a parent. It also stores a pointer
 * to the command object.
 */
ActionGroup::ActionGroup ( Command* pcCmd,QObject * parent)
  : Action(pcCmd, parent)
  , _group(nullptr)
  , _dropDown(false)
  , _isMode(false)
{
    _group = new QActionGroup(this);
    connect(_group, &QActionGroup::triggered, this, qOverload<QAction*>(&ActionGroup::onActivated));
    connect(_group, &QActionGroup::hovered, this, &ActionGroup::onHovered);
}

ActionGroup::~ActionGroup()
{
    delete _group;
}

/**
 * Adds this action to widget \a w.
 */
void ActionGroup::addTo(QWidget *widget)
{
    // When adding an action that has defined a menu then shortcuts
    // of the menu actions don't work. To make this working we must
    // set the menu explicitly. This means calling QAction::setMenu()
    // and adding this action to the widget doesn't work.
    if (_dropDown) {
        if (widget->inherits("QMenu")) {
            auto menu = new QMenu(widget);
            QAction* item = qobject_cast<QMenu*>(widget)->addMenu(menu);
            item->setMenuRole(action()->menuRole());
            menu->setTitle(action()->text());
            menu->addActions(groupAction()->actions());
        }
        else if (widget->inherits("QToolBar")) {
            widget->addAction(action());
            QToolButton* tb = widget->findChildren<QToolButton*>().constLast();
            tb->setPopupMode(QToolButton::MenuButtonPopup);
            tb->setObjectName(QString::fromLatin1("qt_toolbutton_menubutton"));
            QList<QAction*> acts = groupAction()->actions();
            auto menu = new QMenu(tb);
            menu->addActions(acts);
            tb->setMenu(menu);
        }
        else {
            widget->addActions(groupAction()->actions()); // no drop-down
        }
    }
    else {
        widget->addActions(groupAction()->actions());
    }
}

void ActionGroup::setEnabled( bool check )
{
    Action::setEnabled(check);
    groupAction()->setEnabled(check);
}

void ActionGroup::setDisabled (bool check)
{
    Action::setEnabled(!check);
    groupAction()->setDisabled(check);
}

void ActionGroup::setExclusive (bool check)
{
    groupAction()->setExclusive(check);
}

bool ActionGroup::isExclusive() const
{
    return groupAction()->isExclusive();
}

void ActionGroup::setVisible( bool check )
{
    Action::setVisible(check);
    groupAction()->setVisible(check);
}

QAction* ActionGroup::addAction(QAction* action)
{
    int index = groupAction()->actions().size();
    action = groupAction()->addAction(action);
    action->setData(QVariant(index));
    return action;
}

QAction* ActionGroup::addAction(const QString& text)
{
    int index = groupAction()->actions().size();
    QAction* action = groupAction()->addAction(text);
    action->setData(QVariant(index));
    return action;
}

QList<QAction*> ActionGroup::actions() const
{
    return groupAction()->actions();
}

int ActionGroup::checkedAction() const
{
    QAction* checked = groupAction()->checkedAction();
    return checked ? checked->data().toInt() : -1;
}

void ActionGroup::setCheckedAction(int index)
{
    auto acts = groupAction()->actions();
    QAction* act = acts.at(index);
    act->setChecked(true);
    this->setIcon(act->icon());

    if (!this->_isMode) {
        this->setToolTip(act->toolTip());
    }
    this->setProperty("defaultAction", QVariant(index));
}

/**
 * Activates the command.
 */
void ActionGroup::onActivated ()
{
    command()->invoke(this->property("defaultAction").toInt(), Command::TriggerAction);
}

void ActionGroup::onToggled(bool)
{
    onActivated();
}

/**
 * Activates the command.
 */
void ActionGroup::onActivated (QAction* act)
{
    int index = groupAction()->actions().indexOf(act);

    this->setIcon(act->icon());
    if (!this->_isMode) {
        this->setToolTip(act->toolTip());
    }
    this->setProperty("defaultAction", QVariant(index));
    command()->invoke(index, Command::TriggerChildAction);
}

void ActionGroup::onHovered (QAction *act)
{
    QToolTip::showText(QCursor::pos(), act->toolTip());
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
    explicit WorkbenchActionEvent(QAction* act)
      : QEvent(QEvent::User), act(act)
    { }
    ~WorkbenchActionEvent() override
    { }
    QAction* action() const
    { return act; }
private:
    QAction* act;

    Q_DISABLE_COPY(WorkbenchActionEvent)
};
}

WorkbenchComboBox::WorkbenchComboBox(WorkbenchGroup* wb, QWidget* parent) : QComboBox(parent), group(wb)
{
    connect(this, qOverload<int>(&WorkbenchComboBox::activated),
            this, qOverload<int>(&WorkbenchComboBox::onActivated));
    connect(getMainWindow(), &MainWindow::workbenchActivated,
            this, &WorkbenchComboBox::onWorkbenchActivated);
}

WorkbenchComboBox::~WorkbenchComboBox()
{
}

void WorkbenchComboBox::showPopup()
{
    int rows = count();
    if (rows > 0) {
        int height = view()->sizeHintForRow(0);
        int maxHeight = QApplication::primaryScreen()->size().height();
        view()->setMinimumHeight(qMin(height * rows, maxHeight/2));
    }

    QComboBox::showPopup();
}

void WorkbenchComboBox::actionEvent ( QActionEvent* qae )
{
    QAction *action = qae->action();
    switch (qae->type()) {
    case QEvent::ActionAdded:
        {
            if (action->isVisible()) {
                QIcon icon = action->icon();
                if (icon.isNull()) {
                    this->addItem(action->text(), action->data());
                }
                else {
                    this->addItem(icon, action->text(), action->data());
                }
                if (action->isChecked()) {
                    this->setCurrentIndex(action->data().toInt());
                }
            }
            break;
        }
    case QEvent::ActionChanged:
        {
            QVariant data = action->data();
            int index = this->findData(data);
            // added a workbench
            if (index < 0 && action->isVisible()) {
                QIcon icon = action->icon();
                if (icon.isNull())
                    this->addItem(action->text(), data);
                else
                    this->addItem(icon, action->text(), data);
            }
            // removed a workbench
            else if (index >=0 && !action->isVisible()) {
                this->removeItem(index);
            }
            break;
        }
    case QEvent::ActionRemoved:
        {
            //Nothing needs to be done
            break;
        }
    default:
        break;
    }
}

void WorkbenchComboBox::onActivated(int item)
{
    // Send the event to the workbench group to delay the destruction of the emitting widget.
    int index = itemData(item).toInt();
    auto ev = new WorkbenchActionEvent(this->actions().at(index));
    QApplication::postEvent(this->group, ev);
}

void WorkbenchComboBox::onActivated(QAction* action)
{
    // set the according item to the action
    QVariant data = action->data();
    int index = this->findData(data);
    setCurrentIndex(index);
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
    QList<QAction*> act = actions();
    for (QList<QAction*>::Iterator it = act.begin(); it != act.end(); ++it) {
        if ((*it)->objectName() == name) {
            if (/*(*it)->data() != item*/!(*it)->isChecked()) {
                (*it)->trigger();
            }
            break;
        }
    }
}

/* TRANSLATOR Gui::WorkbenchGroup */
WorkbenchGroup::WorkbenchGroup (  Command* pcCmd, QObject * parent )
  : ActionGroup( pcCmd, parent )
{
    // Start a list with 50 elements but extend it when requested
    for (int i=0; i<50; i++) {
        QAction* action = groupAction()->addAction(QLatin1String(""));
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
}

void WorkbenchGroup::addTo(QWidget *widget)
{
    refreshWorkbenchList();

    auto setupBox = [&](WorkbenchComboBox* box) {
        box->setIconSize(QSize(16, 16));
        box->setToolTip(toolTip());
        box->setStatusTip(action()->statusTip());
        box->setWhatsThis(action()->whatsThis());
        box->addActions(groupAction()->actions());
        connect(groupAction(), &QActionGroup::triggered, box, qOverload<QAction*>(&WorkbenchComboBox::onActivated));
    };
    if (widget->inherits("QToolBar")) {
        auto* box = new WorkbenchComboBox(this, widget);
        setupBox(box);

        qobject_cast<QToolBar*>(widget)->addWidget(box);
    }
    else if (widget->inherits("QMenuBar")) {
        auto* box = new WorkbenchComboBox(this, widget);
        setupBox(box);

        bool left = WorkbenchSwitcher::isLeftCorner(WorkbenchSwitcher::getValue());
        qobject_cast<QMenuBar*>(widget)->setCornerWidget(box, left ? Qt::TopLeftCorner : Qt::TopRightCorner);
    }
    else if (widget->inherits("QMenu")) {
        auto menu = qobject_cast<QMenu*>(widget);
        menu = menu->addMenu(action()->text());
        menu->addActions(groupAction()->actions());
    }
}

void WorkbenchGroup::setWorkbenchData(int index, const QString& wb)
{
    QList<QAction*> workbenches = groupAction()->actions();
    QString name = Application::Instance->workbenchMenuText(wb);
    QPixmap px = Application::Instance->workbenchIcon(wb);
    QString tip = Application::Instance->workbenchToolTip(wb);

    workbenches[index]->setObjectName(wb);
    workbenches[index]->setIcon(px);
    workbenches[index]->setText(name);
    workbenches[index]->setToolTip(tip);
    workbenches[index]->setStatusTip(tr("Select the '%1' workbench").arg(name));
    workbenches[index]->setVisible(true);
    if (index < 9) {
        workbenches[index]->setShortcut(QKeySequence(QString::fromUtf8("W,%1").arg(index+1)));
    }
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
    QList<QAction*> workbenches = groupAction()->actions();
    int numActions = workbenches.size();
    int extend = enable_wbs.size() - numActions;
    if (extend > 0) {
        for (int i=0; i<extend; i++) {
            QAction* action = groupAction()->addAction(QLatin1String(""));
            action->setCheckable(true);
            action->setData(QVariant(numActions++)); // set the index
        }
    }

    // Show all enabled wb
    int index = 0;
    for (QStringList::Iterator it = enable_wbs.begin(); it != enable_wbs.end(); ++it) {
        setWorkbenchData(index++, *it);
    }
}

void WorkbenchGroup::customEvent( QEvent* event )
{
    if (event->type() == QEvent::User) {
        auto ce = static_cast<Gui::WorkbenchActionEvent*>(event);
        ce->action()->trigger();
    }
}

void WorkbenchGroup::slotActivateWorkbench(const char* /*name*/)
{
}

void WorkbenchGroup::slotAddWorkbench(const char* name)
{
    QList<QAction*> workbenches = groupAction()->actions();
    QAction* action = nullptr;
    for (QList<QAction*>::Iterator it = workbenches.begin(); it != workbenches.end(); ++it) {
        if (!(*it)->isVisible()) {
            action = *it;
            break;
        }
    }

    if (!action) {
        int index = workbenches.size();
        action = groupAction()->addAction(QLatin1String(""));
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
    QList<QAction*> workbenches = groupAction()->actions();
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
    Private(const Private&) = delete;
    Private(Private&&) = delete;
    void operator= (const Private&) = delete;
    void operator= (Private&&) = delete;

    Private(RecentFilesAction *master, const char *path):master(master)
    {
        handle = App::GetApplication().GetParameterGroupByPath(path);
        handle->Attach(this);
    }

    ~Private() override
    {
        handle->Detach(this);
    }

    void OnChange(Base::Subject<const char*> &, const char *reason) override
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
  : ActionGroup( pcCmd, parent )
  , visibleItems(4)
  , maximumItems(20)
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
    QList<QAction*> recentFiles = groupAction()->actions();

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
    QList<QAction*> recentFiles = groupAction()->actions();
    for (int index = 0; index < recentFiles.count(); index++) {
        QString file = recentFiles[index]->toolTip();
        if (file.isEmpty()) {
            break;
        }
        files.append(file);
    }

    return files;
}

void RecentFilesAction::activateFile(int id)
{
    // restore the list of recent files
    QStringList files = this->files();
    if (id < 0 || id >= files.count()) {
        return; // no valid item
    }

    QString filename = files[id];
    QFileInfo fi(filename);
    if (!fi.exists() || !fi.isFile()) {
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
    for (int i=0; i<diff; i++) {
        groupAction()->addAction(QLatin1String(""))->setVisible(false);
    }
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
    for (int i=0; i<count; i++) {
        groupAction()->addAction(QLatin1String(""))->setVisible(false);
    }
    std::vector<std::string> MRU = hGrp->GetASCIIs("MRU");
    QStringList files;
    for(const auto& it : MRU) {
        files.append(QString::fromUtf8(it.c_str()));
    }
    setFiles(files);
}

/** Saves all recent files to the preferences. */
void RecentFilesAction::save()
{
    ParameterGrp::handle hGrp = _pimpl->handle;
    int count = hGrp->GetInt("RecentFiles", this->visibleItems); // save number of files
    hGrp->Clear();

    // count all set items
    QList<QAction*> recentFiles = groupAction()->actions();
    int num = std::min<int>(count, recentFiles.count());
    for (int index = 0; index < num; index++) {
        QString key = QString::fromLatin1("MRU%1").arg(index);
        QString value = recentFiles[index]->toolTip();
        if (value.isEmpty()) {
            break;
        }
        hGrp->SetASCII(key.toLatin1(), value.toUtf8());
    }

    Base::StateLocker guard(_pimpl->updating);
    hGrp->SetInt("RecentFiles", count); // restore
}

// --------------------------------------------------------------------

/* TRANSLATOR Gui::RecentMacrosAction */

RecentMacrosAction::RecentMacrosAction ( Command* pcCmd, QObject * parent )
  : ActionGroup( pcCmd, parent )
  , visibleItems(4)
  , maximumItems(20)
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
    QList<QAction*> recentFiles = groupAction()->actions();

    int numRecentFiles = std::min<int>(recentFiles.count(), files.count());
    for (int index = 0; index < numRecentFiles; index++) {
        QFileInfo fi(files[index]);
        recentFiles[index]->setText(QString::fromLatin1("%1 %2").arg(index+1).arg(fi.baseName()));
        recentFiles[index]->setToolTip(files[index]); // set the full name that we need later for saving
        recentFiles[index]->setData(QVariant(index));
        QString accel(tr("none"));
        if (index < shortcut_count){
            auto accel_tmp = QString::fromStdString(shortcut_modifiers);
            accel_tmp.append(QString::number(index+1,10)).toStdString();
            auto check = Application::Instance->commandManager().checkAcceleratorForConflicts(qPrintable(accel_tmp));
            if (check) {
                recentFiles[index]->setShortcut(QKeySequence());
                auto msg = QStringLiteral("Recent macros : keyboard shortcut %1 disabled because conflicting with %2")
                                                            .arg(accel_tmp, QLatin1String(check->getName()));
                Base::Console().Warning("%s\n", qPrintable(msg));
            }
            else {
                accel = accel_tmp;
                recentFiles[index]->setShortcut(accel);
            }
        }
        recentFiles[index]->setStatusTip(tr("Run macro %1 (Shift+click to edit) keyboard shortcut: %2").arg(files[index], accel));
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
    QList<QAction*> recentFiles = groupAction()->actions();
    for (int index = 0; index < recentFiles.count(); index++) {
        QString file = recentFiles[index]->toolTip();
        if (file.isEmpty()) {
            break;
        }
        files.append(file);
    }

    return files;
}

void RecentMacrosAction::activateFile(int id)
{
    // restore the list of recent files
    QStringList files = this->files();
    if (id < 0 || id >= files.count()) {
        return; // no valid item
    }

    QString filename = files[id];
    QFileInfo fi(filename);
    if (!fi.exists() || !fi.isFile()) {
        QMessageBox::critical(getMainWindow(), tr("File not found"), tr("The file '%1' cannot be opened.").arg(filename));
        files.removeAll(filename);
        setFiles(files);
    }
    else {
        if (QApplication::keyboardModifiers() == Qt::ShiftModifier){ //open for editing on Shift+click
            auto editor = new PythonEditor();
            editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
            auto edit = new PythonEditorView(editor, getMainWindow());
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
                if (Application::Instance->activeDocument()) {
                    Application::Instance->activeDocument()->getDocument()->recompute();
                }
            }
            catch (const Base::SystemExitException&) {
                // handle SystemExit exceptions
                Base::PyGILStateLocker locker;
                Base::PyException exc;
                exc.ReportException();
            }
        }
    }
}

void RecentMacrosAction::resizeList(int size)
{
    this->visibleItems = size;
    int diff = this->visibleItems - this->maximumItems;
    // create new items if needed
    for (int i=0; i<diff; i++) {
        groupAction()->addAction(QLatin1String(""))->setVisible(false);
    }
    setFiles(files());
}

/** Loads all recent files from the preferences. */
void RecentMacrosAction::restore()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                                ->GetGroup("Preferences")->GetGroup("RecentMacros");

    for (int i=groupAction()->actions().size(); i<this->maximumItems; i++) {
        groupAction()->addAction(QLatin1String(""))->setVisible(false);
    }
    resizeList(hGrp->GetInt("RecentMacros"));

    std::vector<std::string> MRU = hGrp->GetASCIIs("MRU");
    QStringList files;
    for (auto& filename : MRU) {
        files.append(QString::fromUtf8(filename.c_str()));
    }
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
    QList<QAction*> recentFiles = groupAction()->actions();
    int num = std::min<int>(count, recentFiles.count());
    for (int index = 0; index < num; index++) {
        QString key = QString::fromLatin1("MRU%1").arg(index);
        QString value = recentFiles[index]->toolTip();
        if (value.isEmpty()) {
            break;
        }
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
    connect(_toolAction, &QAction::triggered, this, &UndoAction::onActivated);
}

UndoAction::~UndoAction()
{
    QMenu* menu = _toolAction->menu();
    delete menu;
    delete _toolAction;
}

void UndoAction::addTo (QWidget * widget)
{
    if (widget->inherits("QToolBar")) {
        actionChanged();
        connect(action(), &QAction::changed, this, &UndoAction::actionChanged);
        widget->addAction(_toolAction);
    }
    else {
        widget->addAction(action());
    }
}

void UndoAction::actionChanged()
{
    // Do NOT set the shortcut again for _toolAction since this is already
    // reserved for _action. Otherwise we get an ambiguity of it with the
    // result that it doesn't work anymore.
    _toolAction->setText(action()->text());
    _toolAction->setToolTip(action()->toolTip());
    _toolAction->setStatusTip(action()->statusTip());
    _toolAction->setWhatsThis(action()->whatsThis());
    _toolAction->setIcon(action()->icon());
}

void UndoAction::setEnabled(bool check)
{
    Action::setEnabled(check);
    _toolAction->setEnabled(check);
}

void UndoAction::setVisible(bool check)
{
    Action::setVisible(check);
    _toolAction->setVisible(check);
}

// --------------------------------------------------------------------

RedoAction::RedoAction ( Command* pcCmd,QObject * parent )
  : Action(pcCmd, parent)
{
    _toolAction = new QAction(this);
    _toolAction->setMenu(new RedoDialog());
    connect(_toolAction, &QAction::triggered, this, &RedoAction::onActivated);
}

RedoAction::~RedoAction()
{
    QMenu* menu = _toolAction->menu();
    delete menu;
    delete _toolAction;
}

void RedoAction::addTo ( QWidget * widget )
{
    if (widget->inherits("QToolBar")) {
        actionChanged();
        connect(action(), &QAction::changed, this, &RedoAction::actionChanged);
        widget->addAction(_toolAction);
    }
    else {
        widget->addAction(action());
    }
}

void RedoAction::actionChanged()
{
    // Do NOT set the shortcut again for _toolAction since this is already
    // reserved for _action. Otherwise we get an ambiguity of it with the
    // result that it doesn't work anymore.
    _toolAction->setText(action()->text());
    _toolAction->setToolTip(action()->toolTip());
    _toolAction->setStatusTip(action()->statusTip());
    _toolAction->setWhatsThis(action()->whatsThis());
    _toolAction->setIcon(action()->icon());
}

void RedoAction::setEnabled  ( bool check )
{
    Action::setEnabled(check);
    _toolAction->setEnabled(check);
}

void RedoAction::setVisible ( bool check )
{
    Action::setVisible(check);
    _toolAction->setVisible(check);
}

// --------------------------------------------------------------------

DockWidgetAction::DockWidgetAction ( Command* pcCmd, QObject * parent )
  : Action(pcCmd, parent)
  , _menu(nullptr)
{
}

DockWidgetAction::~DockWidgetAction()
{
    delete _menu;
}

void DockWidgetAction::addTo ( QWidget * widget )
{
    if (!_menu) {
        _menu = new QMenu();
        action()->setMenu(_menu);
        getMainWindow()->setDockWindowMenu(_menu);
    }

    widget->addAction(action());
}

// --------------------------------------------------------------------

ToolBarAction::ToolBarAction ( Command* pcCmd, QObject * parent )
  : Action(pcCmd, parent)
  , _menu(nullptr)
{
}

ToolBarAction::~ToolBarAction()
{
    delete _menu;
}

void ToolBarAction::addTo ( QWidget * widget )
{
    if (!_menu) {
      _menu = new QMenu();
      action()->setMenu(_menu);
      getMainWindow()->setToolBarMenu(_menu);
    }

    widget->addAction(action());
}

// --------------------------------------------------------------------

WindowAction::WindowAction ( Command* pcCmd, QObject * parent )
  : ActionGroup(pcCmd, parent)
  , _menu(nullptr)
{
}

WindowAction::~WindowAction()
{
}

void WindowAction::addTo ( QWidget * widget )
{
    auto menu = qobject_cast<QMenu*>(widget);
    if (!menu) {
        if (!_menu) {
            _menu = new QMenu();
            action()->setMenu(_menu);
            _menu->addActions(groupAction()->actions());
            getMainWindow()->setWindowsMenu(_menu);
        }

        widget->addAction(action());
    }
    else {
        menu->addActions(groupAction()->actions());
        getMainWindow()->setWindowsMenu(menu);
    }
}

#include "moc_Action.cpp"
