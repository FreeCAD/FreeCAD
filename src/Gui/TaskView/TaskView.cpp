// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <QAbstractSpinBox>
#include <QActionEvent>
#include <QApplication>
#include <QCursor>
#include <QDockWidget>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <FCConfig.h>

#include <App/Document.h>
#include <Gui/ActionFunction.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "TaskView.h"
#include "TaskDialog.h"
#include "TaskEditControl.h"
#include <Gui/Control.h>

#include <Gui/QSint/actionpanel/taskgroup_p.h>
#include <Gui/QSint/actionpanel/taskheader_p.h>
#include <Gui/QSint/actionpanel/actionpanelscheme.h>


using namespace Gui::TaskView;
namespace sp = std::placeholders;


//**************************************************************************
//**************************************************************************
// TaskWidget
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskWidget::TaskWidget(QWidget* parent)
    : QWidget(parent)
{}

TaskWidget::~TaskWidget() = default;

//**************************************************************************
//**************************************************************************
// TaskGroup
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskGroup::TaskGroup(QWidget* parent)
    : QSint::ActionBox(parent)
{}

TaskGroup::TaskGroup(const QString& headerText, QWidget* parent)
    : QSint::ActionBox(headerText, parent)
{}

TaskGroup::TaskGroup(const QPixmap& icon, const QString& headerText, QWidget* parent)
    : QSint::ActionBox(icon, headerText, parent)
{}

TaskGroup::~TaskGroup() = default;

void TaskGroup::actionEvent(QActionEvent* e)
{
    QAction* action = e->action();
    switch (e->type()) {
        case QEvent::ActionAdded: {
            this->createItem(action);
            break;
        }
        case QEvent::ActionChanged: {
            break;
        }
        case QEvent::ActionRemoved: {
            // cannot change anything
            break;
        }
        default:
            break;
    }
}

//**************************************************************************
//**************************************************************************
// TaskBox
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskBox::TaskBox(QWidget* parent)
    : QSint::ActionGroup(parent)
    , wasShown(false)
{
    // override vertical size policy because otherwise task dialogs
    // whose needsFullSpace() returns true won't take full space.
    myGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

TaskBox::TaskBox(const QString& title, bool expandable, QWidget* parent)
    : QSint::ActionGroup(title, expandable, parent)
    , wasShown(false)
{
    // override vertical size policy because otherwise task dialogs
    // whose needsFullSpace() returns true won't take full space.
    myGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

TaskBox::TaskBox(const QPixmap& icon, const QString& title, bool expandable, QWidget* parent)
    : QSint::ActionGroup(icon, title, expandable, parent)
    , wasShown(false)
{
    // override vertical size policy because otherwise task dialogs
    // whose needsFullSpace() returns true won't take full space.
    myGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

QSize TaskBox::minimumSizeHint() const
{
    // ActionGroup returns a size of 200x100 which leads to problems
    // when there are several task groups in a panel and the first
    // one is collapsed. In this case the task panel doesn't expand to
    // the actually required size and all the remaining groups are
    // squeezed into the available space and thus the widgets in there
    // often can't be used any more.
    // To fix this problem minimumSizeHint() is implemented to again
    // respect the layout's minimum size.
    QSize s1 = QSint::ActionGroup::minimumSizeHint();
    QSize s2 = QWidget::minimumSizeHint();
    return {qMax(s1.width(), s2.width()), qMax(s1.height(), s2.height())};
}

TaskBox::~TaskBox() = default;

void TaskBox::showEvent(QShowEvent*)
{
    wasShown = true;
}

void TaskBox::hideGroupBox()
{
    if (!wasShown) {
        // get approximate height
        int h = 0;
        int ct = groupLayout()->count();
        for (int i = 0; i < ct; i++) {
            QLayoutItem* item = groupLayout()->itemAt(i);
            if (item && item->widget()) {
                QWidget* w = item->widget();
                h += w->height();
            }
        }

        m_tempHeight = m_fullHeight = h;
        // For the very first time the group gets shown
        // we cannot do the animation because the layouting
        // is not yet fully done
        m_foldDelta = 0;
    }
    else {
        m_tempHeight = m_fullHeight = myGroup->height();
        m_foldDelta = m_fullHeight / myScheme->groupFoldSteps;
    }

    m_foldStep = 0.0;
    m_foldDirection = -1;

    // make sure to have the correct icon
    bool block = myHeader->blockSignals(true);
    myHeader->fold();
    myHeader->blockSignals(block);

    myDummy->setFixedHeight(0);
    myDummy->hide();
    myGroup->hide();

    m_foldPixmap = QPixmap();
    setFixedHeight(myHeader->height());
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

bool TaskBox::isGroupVisible() const
{
    return myGroup->isVisible();
}

void TaskBox::actionEvent(QActionEvent* e)
{
    QAction* action = e->action();
    switch (e->type()) {
        case QEvent::ActionAdded: {
            auto label = new QSint::ActionLabel(action, this);
            this->addActionLabel(label, true, false);
            break;
        }
        case QEvent::ActionChanged: {
            break;
        }
        case QEvent::ActionRemoved: {
            // cannot change anything
            break;
        }
        default:
            break;
    }
}

//**************************************************************************
//**************************************************************************
// TaskPanel
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskPanel::TaskPanel(QWidget* parent)
    : QSint::ActionPanel(parent)
{}

TaskPanel::~TaskPanel() = default;

QSize TaskPanel::minimumSizeHint() const
{
    // ActionPanel returns a size of 200x150 which leads to problems
    // when there are several task groups in the panel and the first
    // one is collapsed. In this case the task panel doesn't expand to
    // the actually required size and all the remaining groups are
    // squeezed into the available space and thus the widgets in there
    // often can't be used any more.
    // To fix this problem minimumSizeHint() is implemented to again
    // respect the layout's minimum size.
    QSize s1 = QSint::ActionPanel::minimumSizeHint();
    QSize s2 = QWidget::minimumSizeHint();
    return {qMax(s1.width(), s2.width()), qMax(s1.height(), s2.height())};
}


//**************************************************************************
//**************************************************************************
// TaskView
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskView::TaskView(QWidget* parent)
    : QWidget(parent)
    , ActiveDialog(nullptr)
    , ActiveCtrl(nullptr)
    , hGrp(Gui::WindowParameter::getDefaultParameter()->GetGroup("General"))
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(mainLayout);
    scrollArea = new QScrollArea(this);

    contextualPanelsLayout = new QVBoxLayout();
    contextualPanelsLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(contextualPanelsLayout);

    dialogLayout = new QVBoxLayout();
    dialogLayout->setContentsMargins(0, 0, 0, 0);
    dialogLayout->setSpacing(0);
    mainLayout->addLayout(dialogLayout, 1);

    taskPanel = new TaskPanel(scrollArea);
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(taskPanel->sizePolicy().hasHeightForWidth());
    taskPanel->setSizePolicy(sizePolicy);
    taskPanel->setScheme(QSint::ActionPanelScheme::defaultScheme());

    scrollArea->setWidget(taskPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setMinimumWidth(200);
    dialogLayout->addWidget(scrollArea, 1);

    Gui::Selection().Attach(this);

    // NOLINTBEGIN
    connectApplicationActiveDocument = App::GetApplication().signalActiveDocument.connect(
        std::bind(&Gui::TaskView::TaskView::slotActiveDocument, this, sp::_1)
    );
    connectApplicationDeleteDocument = App::GetApplication().signalDeleteDocument.connect(
        std::bind(&Gui::TaskView::TaskView::slotDeletedDocument, this, sp::_1)
    );
    connectApplicationClosedView = Gui::Application::Instance->signalCloseView.connect(
        std::bind(&Gui::TaskView::TaskView::slotViewClosed, this, sp::_1)
    );
    connectApplicationUndoDocument = App::GetApplication().signalUndoDocument.connect(
        std::bind(&Gui::TaskView::TaskView::slotUndoDocument, this, sp::_1)
    );
    connectApplicationRedoDocument = App::GetApplication().signalRedoDocument.connect(
        std::bind(&Gui::TaskView::TaskView::slotRedoDocument, this, sp::_1)
    );
    connectApplicationInEdit = Gui::Application::Instance->signalInEdit.connect(
        std::bind(&Gui::TaskView::TaskView::slotInEdit, this, sp::_1)
    );
    // NOLINTEND

    setShowTaskWatcher(hGrp->GetBool("ShowTaskWatcher", true));
    connectShowTaskWatcherSetting = hGrp->Manager()->signalParamChanged.connect(
        [this](ParameterGrp* Param, ParameterGrp::ParamType Type, const char* name, const char* value) {
            if (Param == hGrp && Type == ParameterGrp::ParamType::FCBool && name
                && strcmp(name, "ShowTaskWatcher") == 0) {
                setShowTaskWatcher(value && *value == '1');
            }
        }
    );

    updateWatcher();
}

TaskView::~TaskView()
{
    connectApplicationActiveDocument.disconnect();
    connectApplicationDeleteDocument.disconnect();
    connectApplicationClosedView.disconnect();
    connectApplicationUndoDocument.disconnect();
    connectApplicationRedoDocument.disconnect();
    connectApplicationInEdit.disconnect();
    connectShowTaskWatcherSetting.disconnect();
    Gui::Selection().Detach(this);

    for (QWidget* panel : contextualPanels) {
        delete panel;
    }
}

bool TaskView::isEmpty(bool includeWatcher) const
{
    if (ActiveCtrl || ActiveDialog) {
        return false;
    }

    if (includeWatcher) {
        for (auto* watcher : ActiveWatcher) {
            if (watcher->shouldShow()) {
                return false;
            }
        }
    }
    return true;
}

bool TaskView::event(QEvent* event)
{
    // Workaround for a limitation in Qt (#0003794)
    // Line edits and spin boxes don't handle the key combination
    // Shift+Keypad button (if NumLock is activated)
    if (event->type() == QEvent::ShortcutOverride) {
        QWidget* focusWidget = qApp->focusWidget();
        bool isLineEdit = qobject_cast<QLineEdit*>(focusWidget);
        bool isSpinBox = qobject_cast<QAbstractSpinBox*>(focusWidget);

        if (isLineEdit || isSpinBox) {
            auto kevent = static_cast<QKeyEvent*>(event);
            Qt::KeyboardModifiers ShiftKeypadModifier = Qt::ShiftModifier | Qt::KeypadModifier;
            if (kevent->modifiers() == Qt::NoModifier || kevent->modifiers() == Qt::ShiftModifier
                || kevent->modifiers() == Qt::KeypadModifier
                || kevent->modifiers() == ShiftKeypadModifier) {
                switch (kevent->key()) {
                    case Qt::Key_Delete:
                    case Qt::Key_Home:
                    case Qt::Key_End:
                    case Qt::Key_Backspace:
                    case Qt::Key_Left:
                    case Qt::Key_Right:
                        kevent->accept();
                    default:
                        break;
                }
            }
        }
    }
    return QWidget::event(event);
}

void TaskView::keyPressEvent(QKeyEvent* ke)
{
    if (ActiveCtrl && ActiveDialog) {
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            // get all buttons of the complete task dialog
            QList<QPushButton*> list = this->findChildren<QPushButton*>();
            for (auto pb : list) {
                if (pb->isDefault() && pb->isVisible()) {
                    if (pb->isEnabled()) {
#if defined(FC_OS_MACOSX)
                        // #0001354: Crash on using Enter-Key for confirmation of chamfer or fillet
                        // entries
                        QPoint pos = QCursor::pos();
                        QCursor::setPos(pb->parentWidget()->mapToGlobal(pb->pos()));
#endif
                        pb->click();
#if defined(FC_OS_MACOSX)
                        QCursor::setPos(pos);
#endif
                    }
                    return;
                }
            }
        }
        else if (ke->key() == Qt::Key_Escape && ActiveDialog->isEscapeButtonEnabled()) {
            // get only the buttons of the button box
            QDialogButtonBox* box = ActiveCtrl->standardButtons();
            QList<QAbstractButton*> list = box->buttons();
            for (auto pb : list) {
                if (box->buttonRole(pb) == QDialogButtonBox::RejectRole) {
                    if (pb->isEnabled()) {
#if defined(FC_OS_MACOSX)
                        // #0001354: Crash on using Enter-Key for confirmation of chamfer or fillet
                        // entries
                        QPoint pos = QCursor::pos();
                        QCursor::setPos(pb->parentWidget()->mapToGlobal(pb->pos()));
#endif
                        pb->click();
#if defined(FC_OS_MACOSX)
                        QCursor::setPos(pos);
#endif
                    }
                    return;
                }
            }

            // In case a task panel has no Close or Cancel button
            // then invoke resetEdit() directly
            // See also ViewProvider::eventCallback
            auto func = new Gui::TimerFunction();
            func->setAutoDelete(true);
            Gui::Document* doc = Gui::Application::Instance->getDocument(
                ActiveDialog->getDocumentName().c_str()
            );
            if (doc) {
                func->setFunction([doc]() { doc->resetEdit(); });
                func->singleShot(0);
            }
        }
    }
    else {
        QWidget::keyPressEvent(ke);
    }
}

void TaskView::triggerMinimumSizeHint()
{
    // NOLINTNEXTLINE
    QTimer::singleShot(100, this, &TaskView::adjustMinimumSizeHint);
}

void TaskView::adjustMinimumSizeHint()
{
    QSize ms = minimumSizeHint();
    setMinimumWidth(ms.width());
}

QSize TaskView::minimumSizeHint() const
{
    QSize ms = QWidget::minimumSizeHint();
    int spacing = 0;
    if (QLayout* layout = taskPanel->layout()) {
        spacing = 2 * layout->spacing();
    }

    ms.setWidth(taskPanel->minimumSizeHint().width() + spacing);
    return ms;
}

void TaskView::slotActiveDocument(const App::Document& doc)
{
    Q_UNUSED(doc);
    if (!ActiveDialog) {
        // at this point, active object of the active view returns None.
        // which is a problem if shouldShow of a watcher rely on the presence
        // of an active object (example Assembly).
        QTimer::singleShot(100, this, &TaskView::updateWatcher);
    }
}

void TaskView::slotInEdit(const Gui::ViewProviderDocumentObject& vp)
{
    Q_UNUSED(vp);
    if (!ActiveDialog) {
        updateWatcher();
    }
}

void TaskView::slotDeletedDocument(const App::Document& doc)
{
    if (ActiveDialog) {
        if (ActiveDialog->isAutoCloseOnDeletedDocument()) {
            std::string name = ActiveDialog->getDocumentName();
            if (name.empty()) {
                Base::Console().warning(
                    std::string("TaskView::slotDeletedDocument"),
                    "No document name set\n"
                );
            }

            if (name == doc.getName()) {
                ActiveDialog->autoClosedOnDeletedDocument();
                removeDialog();
            }
        }
    }

    if (!ActiveDialog) {
        updateWatcher();
    }
}

void TaskView::slotViewClosed(const Gui::MDIView* view)
{
    // It can happen that only a view is closed an not the document
    if (ActiveDialog) {
        if (ActiveDialog->isAutoCloseOnClosedView()) {
            const Gui::MDIView* associatedView = ActiveDialog->getAssociatedView();
            if (!associatedView) {
                Base::Console().warning(std::string("TaskView::slotViewClosed"), "No view associated\n");
            }

            if (associatedView == view) {
                ActiveDialog->autoClosedOnClosedView();
                removeDialog();
            }
        }
    }

    if (!ActiveDialog) {
        updateWatcher();
    }
}

void TaskView::transactionChangeOnDocument(const App::Document& doc, bool undo)
{
    if (ActiveDialog) {
        std::string name = ActiveDialog->getDocumentName();
        if (name == doc.getName()) {
            undo ? ActiveDialog->onUndo() : ActiveDialog->onRedo();
        }

        if (ActiveDialog->isAutoCloseOnTransactionChange()) {
            if (name.empty()) {
                Base::Console().warning(
                    std::string("TaskView::transactionChangeOnDocument"),
                    "No document name set\n"
                );
            }

            if (name == doc.getName()) {
                ActiveDialog->autoClosedOnTransactionChange();
                removeDialog();
            }
        }
    }

    if (!ActiveDialog) {
        updateWatcher();
    }
}

void TaskView::slotUndoDocument(const App::Document& doc)
{
    transactionChangeOnDocument(doc, true);
}

void TaskView::slotRedoDocument(const App::Document& doc)
{
    transactionChangeOnDocument(doc, false);
}

/// @cond DOXERR
void TaskView::OnChange(
    Gui::SelectionSingleton::SubjectType& rCaller,
    Gui::SelectionSingleton::MessageType Reason
)
{
    Q_UNUSED(rCaller);
    std::string temp;

    if (Reason.Type == SelectionChanges::AddSelection || Reason.Type == SelectionChanges::ClrSelection
        || Reason.Type == SelectionChanges::SetSelection
        || Reason.Type == SelectionChanges::RmvSelection) {

        if (!ActiveDialog) {
            updateWatcher();
        }
    }
}
/// @endcond

void TaskView::showDialog(TaskDialog* dlg)
{
    // if trying to open the same dialog twice nothing needs to be done
    if (ActiveDialog == dlg) {
        return;
    }
    assert(!ActiveDialog);
    assert(!ActiveCtrl);

    // remove the TaskWatcher as long as the Dialog is up
    removeTaskWatcher();

    // first create the control element, set it up and wire it:
    ActiveCtrl = new TaskEditControl(this);
    ActiveCtrl->buttonBox->setStandardButtons(dlg->getStandardButtons());
    TaskDialogAttorney::setButtonBox(dlg, ActiveCtrl->buttonBox);

    // clang-format off
    // make connection to the needed signals
    connect(ActiveCtrl->buttonBox, &QDialogButtonBox::accepted,
            this, &TaskView::accept);
    connect(ActiveCtrl->buttonBox, &QDialogButtonBox::rejected,
            this, &TaskView::reject);
    connect(ActiveCtrl->buttonBox, &QDialogButtonBox::helpRequested,
            this, &TaskView::helpRequested);
    connect(ActiveCtrl->buttonBox, &QDialogButtonBox::clicked,
            this, &TaskView::clicked);
    // clang-format on

    const std::vector<QWidget*>& cont = dlg->getDialogContent();

    // give to task dialog to customize the button box
    dlg->modifyStandardButtons(ActiveCtrl->buttonBox);

    if (dlg->buttonPosition() == TaskDialog::North) {
        // Add button box to the top of the main layout
        dialogLayout->insertWidget(0, ActiveCtrl);
        for (const auto& it : cont) {
            taskPanel->addWidget(it);
        }
    }
    else {
        for (const auto& it : cont) {
            taskPanel->addWidget(it);
        }
        // Add button box to the bottom of the main layout
        dialogLayout->addWidget(ActiveCtrl);
    }

    taskPanel->setScheme(QSint::ActionPanelScheme::defaultScheme());

    if (!dlg->needsFullSpace()) {
        taskPanel->addStretch();
    }

    // set as active Dialog
    ActiveDialog = dlg;

    ActiveDialog->open();

    saveCurrentWidth();
    getMainWindow()->updateActions();

    triggerMinimumSizeHint();

    Q_EMIT taskUpdate();
}

void TaskView::removeDialog()
{
    getMainWindow()->updateActions();

    if (ActiveCtrl) {
        dialogLayout->removeWidget(ActiveCtrl);
        delete ActiveCtrl;
        ActiveCtrl = nullptr;
    }

    TaskDialog* remove = nullptr;
    if (ActiveDialog) {
        // See 'accept' and 'reject'
        if (ActiveDialog->property("taskview_accept_or_reject").isNull()) {
            const std::vector<QWidget*>& cont = ActiveDialog->getDialogContent();
            for (const auto& it : cont) {
                taskPanel->removeWidget(it);
            }
            remove = ActiveDialog;
            ActiveDialog = nullptr;
        }
        else {
            ActiveDialog->setProperty("taskview_remove_dialog", true);
        }
    }

    taskPanel->removeStretch();

    // put the watcher back in control
    addTaskWatcher();

    if (remove) {
        remove->closed();
        remove->emitDestructionSignal();
        delete remove;
    }

    tryRestoreWidth();
    triggerMinimumSizeHint();
}
void TaskView::setShowTaskWatcher(bool show)
{
    if (showTaskWatcher == show) {
        return;
    }

    showTaskWatcher = show;
    if (show) {
        addTaskWatcher();
    }
    else {
        clearTaskWatcher();
    }
}
void TaskView::updateWatcher()
{
    if (!showTaskWatcher) {
        return;
    }

    if (ActiveWatcher.empty()) {
        auto panel = Gui::Control().taskPanel();
        if (panel && panel->ActiveWatcher.size()) {
            takeTaskWatcher(panel);
        }
    }

    // In case a child of the TaskView has the focus and get hidden we have
    // to make sure to set the focus on a widget that won't be hidden or
    // deleted because otherwise Qt may forward the focus via focusNextPrevChild()
    // to the mdi area which may switch to another mdi view which is not an
    // acceptable behaviour.
    QWidget* fw = QApplication::focusWidget();
    if (!fw) {
        this->setFocus();
    }
    QPointer<QWidget> fwp = fw;
    while (fw && !fw->isWindow()) {
        if (fw == this) {
            this->setFocus();
            break;
        }
        fw = fw->parentWidget();
    }

    // add all widgets for all watcher to the task view
    for (const auto& it : ActiveWatcher) {
        bool match = it->shouldShow();
        std::vector<QWidget*>& cont = it->getWatcherContent();
        for (auto& it2 : cont) {
            if (match) {
                it2->show();
            }
            else {
                it2->hide();
            }
        }
    }

    // In case the previous widget that had the focus is still visible
    // give it the focus back.
    if (fwp && fwp->isVisible()) {
        fwp->setFocus();
    }

    triggerMinimumSizeHint();

    Q_EMIT taskUpdate();
}

void TaskView::addTaskWatcher(const std::vector<TaskWatcher*>& Watcher)
{
    // remove and delete the old set of TaskWatcher
    for (TaskWatcher* tw : ActiveWatcher) {
        delete tw;
    }

    ActiveWatcher = Watcher;
    if (!ActiveCtrl && !ActiveDialog) {
        addTaskWatcher();
    }
}

void TaskView::takeTaskWatcher(TaskView* other)
{
    clearTaskWatcher();
    ActiveWatcher.swap(other->ActiveWatcher);
    other->clearTaskWatcher();
    if (isEmpty(false)) {
        addTaskWatcher();
    }
}

void TaskView::clearTaskWatcher()
{
    std::vector<TaskWatcher*> watcher;
    removeTaskWatcher();
    // make sure to delete the old watchers
    addTaskWatcher(watcher);
}

void TaskView::addTaskWatcher()
{
    if (!showTaskWatcher) {
        return;
    }
    // add all widgets for all watcher to the task view
    for (TaskWatcher* tw : ActiveWatcher) {
        std::vector<QWidget*>& cont = tw->getWatcherContent();
        for (QWidget* w : cont) {
            taskPanel->addWidget(w);
        }
    }

    if (!ActiveWatcher.empty()) {
        taskPanel->addStretch();
    }
    updateWatcher();

    // Workaround to avoid a crash in Qt. See also
    // https://forum.freecad.org/viewtopic.php?f=8&t=39187
    //
    // Notify the button box about a style change so that it can
    // safely delete the style animation of its push buttons.
    auto box = taskPanel->findChild<QDialogButtonBox*>();
    if (box) {
        QEvent event(QEvent::StyleChange);
        QApplication::sendEvent(box, &event);
    }

    taskPanel->setScheme(QSint::ActionPanelScheme::defaultScheme());
}

void TaskView::saveCurrentWidth()
{
    if (shouldRestoreWidth()) {
        if (auto parent = qobject_cast<QDockWidget*>(parentWidget())) {
            currentWidth = parent->width();
        }
    }
}

void TaskView::tryRestoreWidth()
{
    if (shouldRestoreWidth()) {
        if (auto parent = qobject_cast<QDockWidget*>(parentWidget())) {
            Gui::getMainWindow()->resizeDocks({parent}, {currentWidth}, Qt::Horizontal);
        }
    }
}

void TaskView::setRestoreWidth(bool on)
{
    restoreWidth = on;
}

bool TaskView::shouldRestoreWidth() const
{
    return restoreWidth;
}

void TaskView::removeTaskWatcher()
{
    // In case a child of the TaskView has the focus and get hidden we have
    // to make sure that set the focus on a widget that won't be hidden or
    // deleted because otherwise Qt may forward the focus via focusNextPrevChild()
    // to the mdi area which may switch to another mdi view which is not an
    // acceptable behaviour.
    QWidget* fw = QApplication::focusWidget();
    if (!fw) {
        this->setFocus();
    }
    while (fw && !fw->isWindow()) {
        if (fw == this) {
            this->setFocus();
            break;
        }
        fw = fw->parentWidget();
    }

    // remove all widgets
    for (TaskWatcher* tw : ActiveWatcher) {
        std::vector<QWidget*>& cont = tw->getWatcherContent();
        for (QWidget* w : cont) {
            w->hide();
            taskPanel->removeWidget(w);
        }
    }

    taskPanel->removeStretch();
}

void TaskView::accept()
{
    if (!ActiveDialog) {  // Protect against segfaults due to out-of-order deletions
        Base::Console().warning("ActiveDialog was null in call to TaskView::accept()\n");
        return;
    }

    // Make sure that if 'accept' calls 'closeDialog' the deletion is postponed until
    // the dialog leaves the 'accept' method
    ActiveDialog->setProperty("taskview_accept_or_reject", true);
    bool success = ActiveDialog->accept();
    ActiveDialog->setProperty("taskview_accept_or_reject", QVariant());
    if (success || ActiveDialog->property("taskview_remove_dialog").isValid()) {
        removeDialog();
    }
}

void TaskView::reject()
{
    if (!ActiveDialog) {  // Protect against segfaults due to out-of-order deletions
        Base::Console().warning("ActiveDialog was null in call to TaskView::reject()\n");
        return;
    }

    // Make sure that if 'reject' calls 'closeDialog' the deletion is postponed until
    // the dialog leaves the 'reject' method
    ActiveDialog->setProperty("taskview_accept_or_reject", true);
    bool success = ActiveDialog->reject();
    ActiveDialog->setProperty("taskview_accept_or_reject", QVariant());
    if (success || ActiveDialog->property("taskview_remove_dialog").isValid()) {
        removeDialog();
    }
}

void TaskView::helpRequested()
{
    ActiveDialog->helpRequested();
}

void TaskView::clicked(QAbstractButton* button)
{
    int id = ActiveCtrl->buttonBox->standardButton(button);
    ActiveDialog->clicked(id);
}

void TaskView::clearActionStyle()
{
    static_cast<QSint::ActionPanelScheme*>(QSint::ActionPanelScheme::defaultScheme())->clearActionStyle();
    taskPanel->setScheme(QSint::ActionPanelScheme::defaultScheme());
}

void TaskView::restoreActionStyle()
{
    static_cast<QSint::ActionPanelScheme*>(QSint::ActionPanelScheme::defaultScheme())
        ->restoreActionStyle();
    taskPanel->setScheme(QSint::ActionPanelScheme::defaultScheme());
}

void TaskView::addContextualPanel(QWidget* panel)
{
    if (!panel || contextualPanels.contains(panel)) {
        return;
    }

    contextualPanelsLayout->addWidget(panel);
    contextualPanels.append(panel);
    panel->show();
    triggerMinimumSizeHint();
    Q_EMIT taskUpdate();
}

void TaskView::removeContextualPanel(QWidget* panel)
{
    if (!panel || !contextualPanels.contains(panel)) {
        return;
    }

    contextualPanelsLayout->removeWidget(panel);
    contextualPanels.removeOne(panel);
    panel->deleteLater();
    triggerMinimumSizeHint();
    Q_EMIT taskUpdate();
}

#include "moc_TaskView.cpp"
