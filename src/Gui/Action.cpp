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
# include <QFileInfo>
# include <QMenu>
# include <QRegularExpression>
# include <QScreen>
# include <QTimer>
# include <QToolBar>
# include <QToolButton>
# include <QToolTip>
#endif

#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <App/Document.h>

#include "Action.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Dialogs/DlgUndoRedo.h"
#include "PreferencePages/DlgSettingsWorkbenchesImp.h"
#include "Document.h"
#include "EditorView.h"
#include "Macro.h"
#include "ModuleIO.h"
#include "MainWindow.h"
#include "PythonEditor.h"
#include "WhatsThis.h"
#include "Widgets.h"
#include "Workbench.h"
#include "WorkbenchManager.h"
#include "WorkbenchSelector.h"
#include "ShortcutManager.h"
#include "Tools.h"


using namespace Gui;
using namespace Gui::Dialog;
namespace sp = std::placeholders;

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

void Action::setChecked(bool check)
{
    _action->setChecked(check);
}

/*!
 * \brief Action::setBlockedChecked
 * \param check
 * Does the same as \ref setChecked but additionally blocks
 * any signals.
 */
void Action::setBlockedChecked(bool check)
{
    QSignalBlocker block(_action);
    _action->setChecked(check);
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
        return {};
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
        return {};
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
    // wrapping using <p style='white-space:pre'>.

    QString shortcut = shortCut;
    if (!shortcut.isEmpty() && helpText.endsWith(shortcut)) {
        helpText.resize(helpText.size() - shortcut.size());
    }
    if (!shortcut.isEmpty()) {
        shortcut = QStringLiteral(" (%1)").arg(shortcut);
    }

    QString tooltip = QStringLiteral(
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

    if (!shortcut.isEmpty() && helpText.endsWith(shortcut)) {
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

    tooltip += QStringLiteral("<p style='white-space:pre; margin:0;'>");

    // If the user supplied tooltip contains line break, we shall honour it.
    if (helpText.indexOf(QLatin1Char('\n')) >= 0) {
        tooltip += helpText.toHtmlEscaped() + QStringLiteral("</p>") ;
    }
    else {
        // If not, try to end the non wrapping paragraph at some pre defined
        // width, so that the following text can wrap at that width.
        float tipWidth = 400;
        QFontMetrics fm(font);
        int width = QtTools::horizontalAdvance(fm, helpText);
        if (width <= tipWidth) {
            tooltip += helpText.toHtmlEscaped() + QStringLiteral("</p>") ;
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
                + QStringLiteral("</p>")
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
ActionGroup::ActionGroup(Command* pcCmd, QObject* parent)
    : Action(pcCmd, parent)
    , _group(nullptr)
    , _dropDown(false)
    , _isMode(false)
    , _rememberLast(true)
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

            QObject::connect(menu, &QMenu::aboutToShow, [this, menu]() {
                Q_EMIT aboutToShow(menu);
            });

            QObject::connect(menu, &QMenu::aboutToHide, [this, menu]() {
                Q_EMIT aboutToHide(menu);
            });
        }
        else if (widget->inherits("QToolBar")) {
            widget->addAction(action());
            QToolButton* tb = widget->findChildren<QToolButton*>().constLast();
            tb->setPopupMode(QToolButton::MenuButtonPopup);
            tb->setObjectName(QStringLiteral("qt_toolbutton_menubutton"));
            QList<QAction*> acts = groupAction()->actions();
            auto menu = new QMenu(tb);
            menu->addActions(acts);
            tb->setMenu(menu);

            QObject::connect(menu, &QMenu::aboutToShow, [this, menu]() {
                Q_EMIT aboutToShow(menu);
            });

            QObject::connect(menu, &QMenu::aboutToHide, [this, menu]() {
                Q_EMIT aboutToHide(menu);
            });
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

void ActionGroup::setRememberLast(bool remember)
{
    _rememberLast = remember;
}

bool ActionGroup::doesRememberLast() const
{
    return _rememberLast;
}

QAction* ActionGroup::addAction(QAction* action)
{
    return groupAction()->addAction(action);
}

QAction* ActionGroup::addAction(const QString& text)
{
    return groupAction()->addAction(text);
}

QList<QAction*> ActionGroup::actions() const
{
    return groupAction()->actions();
}

int ActionGroup::checkedAction() const
{
    auto checked = groupAction()->checkedAction();

    return actions().indexOf(checked);
}

void ActionGroup::setCheckedAction(int index)
{
    auto acts = groupAction()->actions();
    QAction* act = acts.at(index);
    act->setChecked(true);
    this->setIcon(act->icon());

    if (!this->_isMode) {
        this->setToolTip(act->toolTip(), act->text());
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

void ActionGroup::onToggled(bool check)
{
    Q_UNUSED(check)
    onActivated();
}

/**
 * Activates the command.
 */
void ActionGroup::onActivated (QAction* act)
{
    if (_rememberLast) {
        int index = groupAction()->actions().indexOf(act);

        this->setIcon(act->icon());
        if (!this->_isMode) {
            this->setToolTip(act->toolTip(), act->text());
        }
        this->setProperty("defaultAction", QVariant(index));
        command()->invoke(index, Command::TriggerChildAction);
    }
}

/**
 * Shows tooltip at the right side when hovered.
 */
void ActionGroup::onHovered(QAction *act)
{
    const auto topLevelWidgets = QApplication::topLevelWidgets();
    QMenu* foundMenu = nullptr;

    for (QWidget* widget : topLevelWidgets) {
        QList<QMenu*> menus = widget->findChildren<QMenu*>();

        for (QMenu* menu : menus) {
            if (menu->isVisible() && menu->actions().contains(act)) {
                foundMenu = menu;
                break;
            }
        }

        if (foundMenu) {
            break;
        }

    }

    if (foundMenu) {
        QRect actionRect = foundMenu->actionGeometry(act);
        QPoint globalPos = foundMenu->mapToGlobal(actionRect.topRight());
        QToolTip::showText(globalPos, act->toolTip(), foundMenu, actionRect);
    } else {
        QToolTip::showText(QCursor::pos(), act->toolTip());
    }
}


/* TRANSLATOR Gui::WorkbenchGroup */
WorkbenchGroup::WorkbenchGroup (  Command* pcCmd, QObject * parent )
  : ActionGroup( pcCmd, parent )
{
    refreshWorkbenchList();

    //NOLINTBEGIN
    Application::Instance->signalRefreshWorkbenches.connect(std::bind(&WorkbenchGroup::refreshWorkbenchList, this));
    //NOLINTEND

    connect(getMainWindow(), &MainWindow::workbenchActivated,
        this, &WorkbenchGroup::onWorkbenchActivated);
}

QAction* WorkbenchGroup::getOrCreateAction(const QString& wbName)
{
    if (!actionByWorkbenchName.contains(wbName)) {
        actionByWorkbenchName[wbName] = new QAction(QApplication::instance());
    }

    return actionByWorkbenchName[wbName];
}

void WorkbenchGroup::addTo(QWidget *widget)
{
    if (widget->inherits("QToolBar")) {
        ParameterGrp::handle hGrp;
        hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Workbenches");

        QWidget* workbenchSelectorWidget;
        if (hGrp->GetInt("WorkbenchSelectorType", 0) == 0) {
            workbenchSelectorWidget = new WorkbenchComboBox(this, widget);
        }
        else {
            workbenchSelectorWidget = new WorkbenchTabWidget(this, widget);
        }

        static_cast<QToolBar*>(widget)->addWidget(workbenchSelectorWidget);
    }
    else if (widget->inherits("QMenu")) {
        auto menu = qobject_cast<QMenu*>(widget);
        menu = menu->addMenu(action()->text());
        menu->addActions(getEnabledWbActions());

        connect(this, &WorkbenchGroup::workbenchListRefreshed, this, [menu](QList<QAction*> actions) {
            menu->clear();
            menu->addActions(actions);
        });
    }
}

void WorkbenchGroup::refreshWorkbenchList()
{
    QStringList enabledWbNames = DlgSettingsWorkbenchesImp::getEnabledWorkbenches();

    // Clear the actions.
    for (QAction* action : actions()) {
        groupAction()->removeAction(action);
    }

    enabledWbsActions.clear();
    disabledWbsActions.clear();

    std::string activeWbName = WorkbenchManager::instance()->activeName();

    // Create action list of enabled wb
    int index = 0;
    for (const auto& wbName : enabledWbNames) {
        QString name = Application::Instance->workbenchMenuText(wbName);
        QPixmap px = Application::Instance->workbenchIcon(wbName);
        QString tip = Application::Instance->workbenchToolTip(wbName);

        QAction* action = getOrCreateAction(wbName);

        groupAction()->addAction(action);

        action->setText(name);
        action->setCheckable(true);
        action->setData(QVariant(index)); // set the index
        action->setObjectName(wbName);
        action->setIcon(px);
        action->setToolTip(tip);
        action->setStatusTip(tr("Selects the '%1' workbench").arg(name));
        if (index < 9) {
            action->setShortcut(QKeySequence(QStringLiteral("W,%1").arg(index + 1)));
        }
        if (wbName.toStdString() == activeWbName) {
            action->setChecked(true);
        }
        enabledWbsActions.push_back(action);
        index++;
    }

    // Also create action list of disabled wbs
    QStringList disabledWbNames = DlgSettingsWorkbenchesImp::getDisabledWorkbenches();
    for (const auto& wbName : disabledWbNames) {
        QString name = Application::Instance->workbenchMenuText(wbName);
        QPixmap px = Application::Instance->workbenchIcon(wbName);
        QString tip = Application::Instance->workbenchToolTip(wbName);

        QAction* action = getOrCreateAction(wbName);

        groupAction()->addAction(action);

        action->setText(name);
        action->setCheckable(true);
        action->setData(QVariant(index)); // set the index
        action->setObjectName(wbName);
        action->setIcon(px);
        action->setToolTip(tip);
        action->setStatusTip(tr("Select the '%1' workbench").arg(name));
        if (wbName.toStdString() == activeWbName) {
            action->setChecked(true);
        }
        disabledWbsActions.push_back(action);
        index++;
    }

    // Signal to the widgets (WorkbenchComboBox & menu) to update the wb list
    workbenchListRefreshed(enabledWbsActions);
}

void WorkbenchGroup::onWorkbenchActivated(const QString& name)
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

    for (QAction* action : actions()) {
        if (action->objectName() == name) {
            if (!action->isChecked()) {
                action->trigger();
            }
            break;
        }
    }
}

QList<QAction*> WorkbenchGroup::getEnabledWbActions() const
{
    return enabledWbsActions;
}

QList<QAction*> WorkbenchGroup::getDisabledWbActions() const
{
    return disabledWbsActions;
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

    void OnChange(Base::Subject<const char*> & sub, const char *reason) override
    {
        Q_UNUSED(sub)
        if (!updating && reason && strcmp(reason, "RecentFiles")==0) {
            Base::StateLocker guard(updating);
            master->restore();
        }
    }

    void trySaveUserParameter()
    {
        // update the XML structure and save the user parameter to disk (#0001989)
        bool saveParameter = App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/General")->GetBool("SaveUserParameter", true);
        if (saveParameter) {
            saveUserParameter();
        }
    }

    void saveUserParameter()
    {
        ParameterManager* parmgr = App::GetApplication().GetParameterSet("User parameter");
        parmgr->SaveDocument(App::Application::Config()["UserParameter"].c_str());
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
    _pimpl = std::make_unique<Private>(this, "User parameter:BaseApp/Preferences/RecentFiles");
    restore();

    sep.setSeparator(true);
    sep.setToolTip({});
    this->groupAction()->addAction(&sep);

    //: Empties the list of recent files
    clearRecentFilesListAction.setText(tr("Clear Recent Files"));
    clearRecentFilesListAction.setToolTip({});
    this->groupAction()->addAction(&clearRecentFilesListAction);

    auto clearFun = [this, hGrp = _pimpl->handle](){
        const size_t recentFilesListSize = hGrp->GetASCIIs("MRU").size();
        for (size_t i = 0; i < recentFilesListSize; i++)
        {
            const QByteArray key = QStringLiteral("MRU%1").arg(i).toLocal8Bit();
            hGrp->SetASCII(key.data(), "");
        }
        restore();
        clearRecentFilesListAction.setEnabled(false);
    };

    connect(&clearRecentFilesListAction, &QAction::triggered,
            this, clearFun);

    connect(&clearRecentFilesListAction, &QAction::triggered,
            this, &RecentFilesAction::recentFilesListModified);
}

RecentFilesAction::~RecentFilesAction()
{
    _pimpl.reset(nullptr);
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

    _pimpl->trySaveUserParameter();

    clearRecentFilesListAction.setEnabled(true);

    Q_EMIT recentFilesListModified();
}

static QString numberToLabel(int number) {
    if (number > 0 && number < 10) { // NOLINT: *-magic-numbers
        return QStringLiteral("&%1").arg(number);
    }
    if (number == 10) { // NOLINT: *-magic-numbers
        return QStringLiteral("1&0");
    }
    // If we have a number greater than 10, we start using the alphabet.
    // So 11 becomes 'A' and so on.
    constexpr char lettersStart = 11;
    constexpr char lettersEnd = lettersStart + ('Z' - 'A');
    if (number >= lettersStart && number < lettersEnd) {
        QChar letter = QChar::fromLatin1('A' + (number - lettersStart));
        return QStringLiteral("%1 (&%2)").arg(number).arg(letter);
    }
    // Not enough accelerators to cover this number.
    return QStringLiteral("%1").arg(number);
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
        QString numberLabel = numberToLabel(index + 1);
        QFileInfo fi(files[index]);
        recentFiles[index]->setText(QStringLiteral("%1 %2").arg(numberLabel).arg(fi.fileName()));
        recentFiles[index]->setStatusTip(tr("Open file %1").arg(files[index]));
        recentFiles[index]->setToolTip(files[index]); // set the full name that we need later for saving
        recentFiles[index]->setData(QVariant(index));
        recentFiles[index]->setVisible(true);
    }

    // if less file names than actions
    numRecentFiles = std::min<int>(numRecentFiles, this->visibleItems);
    for (int index = numRecentFiles; index < recentFiles.count(); index++) {
        if (recentFiles[index] == &sep || recentFiles[index] == &clearRecentFilesListAction) {
            continue;
        }
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
    if (!ModuleIO::verifyFile(filename)) {
        files.removeAll(filename);
        setFiles(files);
        save();
    }
    else {
        ModuleIO::openFile(filename);
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
        auto filePath = QString::fromUtf8(it.c_str());
        if (QFileInfo::exists(filePath)) {
            files.append(filePath);
        }
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
        QString key = QStringLiteral("MRU%1").arg(index);
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
    QStringList existingCommands;
    auto accel_col = QString::fromStdString(shortcut_modifiers);
    for (int index = 0; index < numRecentFiles; index++) {
        QFileInfo fi(files[index]);
        QString numberLabel = numberToLabel(index + 1);
        recentFiles[index]->setText(QStringLiteral("%1 %2").arg(numberLabel).arg(fi.completeBaseName()));
        recentFiles[index]->setToolTip(files[index]); // set the full name that we need later for saving
        recentFiles[index]->setData(QVariant(index));
        QString accel(tr("none"));
        if (index < shortcut_count){
            auto accel_tmp = QString::fromStdString(shortcut_modifiers);
            accel_tmp.append(QString::number(index+1,10)).toStdString();
            auto check = Application::Instance->commandManager().checkAcceleratorForConflicts(qPrintable(accel_tmp));
            if (check) {
                recentFiles[index]->setShortcut(QKeySequence());
                accel_col.append(accel_tmp);
                existingCommands.append(QLatin1String(check->getName()));
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
    // Raise a single warning no matter how many conflicts
    if (!existingCommands.isEmpty()) {
        auto msgMain = QStringLiteral("Recent macros : keyboard shortcuts");
        for (int index = 0; index < accel_col.size(); index++) {
            msgMain += QStringLiteral(" %1").arg(accel_col[index]);
        }
        msgMain += QStringLiteral(" disabled because of conflicts with");
        for (int index = 0; index < existingCommands.count(); index++) {
            msgMain += QStringLiteral(" %1").arg(existingCommands[index]);
        }
        msgMain += QStringLiteral(" respectively.\nHint: In Preferences -> Python -> Macro ->"
                             " Recent Macros menu -> Keyboard Modifiers this should be Ctrl+Shift+"
                             " by default, if this is now blank then you should revert it back to"
                             " Ctrl+Shift+ by pressing both keys at the same time.");
        Base::Console().warning("%s\n", qPrintable(msgMain));
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
    if (!ModuleIO::verifyFile(filename)) {
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
                exc.reportException();
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
        QString key = QStringLiteral("MRU%1").arg(index);
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
