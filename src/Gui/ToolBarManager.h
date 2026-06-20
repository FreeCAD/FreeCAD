/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#pragma once

#include <string>
#include <utility>
#include <fastsignals/signal.h>

#include <QStringList>
#include <QMetaType>
#include <QPointer>
#include <QTimer>
#include <QToolBar>
#include <QPointer>

#include <FCGlobal.h>
#include <Base/Parameter.h>

#include "ParamHandler.h"

class QAction;
class QLayout;
class QMenu;
class QMouseEvent;

namespace Gui
{

class ToolBarAreaWidget;
enum class ToolBarArea;

class GuiExport ToolBarItem
{
public:
    /** Manages the default visibility status of a toolbar item, as well as the default status
     * of the toggleViewAction usable by the contextual menu to enable and disable its visibility
     */
    enum class DefaultVisibility
    {
        Visible,      // toolbar is hidden by default, visibility toggle action is enabled
        Hidden,       // toolbar hidden by default, visibility toggle action is enabled
        Unavailable,  // toolbar visibility is managed independently by client code and defaults to
                      // hidden, visibility toggle action is disabled by default (it is unavailable
                      // to the UI). Upon being forced to be available, these toolbars default to
                      // visible.
    };

    enum class Tier
    {
        /** Primary toolbar users should normally see in the default layout. */
        Recommended,
        /** Useful toolbar that is hidden from the recommended default layout. */
        Secondary,
        /** Specialized toolbar intended for explicit opt-in. */
        Advanced,
        /** Toolbar whose availability is controlled by a contextual mode. */
        Contextual,
    };

    ToolBarItem();
    explicit ToolBarItem(
        ToolBarItem* item,
        DefaultVisibility visibilityPolicy = DefaultVisibility::Visible
    );
    ~ToolBarItem();

    void setCommand(const std::string&);
    const std::string& command() const;
    bool hasPersistenceKey() const;
    void setPersistenceKey(const std::string&);
    const std::string& persistenceKey() const;
    void setTier(Tier tier);
    Tier tier() const;

    bool hasItems() const;
    ToolBarItem* findItem(const std::string&);
    ToolBarItem* copy() const;
    uint count() const;

    void appendItem(ToolBarItem* item);
    bool insertItem(ToolBarItem*, ToolBarItem* item);
    void removeItem(ToolBarItem* item);
    void clear();

    ToolBarItem& operator<<(ToolBarItem* item);
    ToolBarItem& operator<<(const std::string& command);
    QList<ToolBarItem*> getItems() const;

    DefaultVisibility visibilityPolicy;

private:
    std::string _name;
    std::string _persistenceKey;
    Tier _tier = Tier::Recommended;
    QList<ToolBarItem*> _items;
};

class ToolBarGrip: public QWidget
{
    Q_OBJECT

public:
    explicit ToolBarGrip(QToolBar*);

    void attach();
    void detach();

    bool isAttached() const;

protected:
    void paintEvent(QPaintEvent*);
    void mouseMoveEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);

    void updateSize();

private:
    QPointer<QAction> _action = nullptr;
};

/**
 * QToolBar from Qt lacks few abilities like ability to float toolbar from code.
 * This class allows us to provide custom behaviors for toolbars if needed.
 */
class GuiExport ToolBar: public QToolBar
{
    Q_OBJECT

    friend class ToolBarGrip;

public:
    ToolBar();
    explicit ToolBar(QWidget* parent);

    virtual ~ToolBar() = default;

    void undock();
    void updateCustomGripVisibility();

protected:
    void setupConnections();
};

/**
 * The ToolBarManager class is responsible for the creation of toolbars and appending them
 * to the main window.
 * @see ToolBoxManager
 * @see MenuManager
 * @author Werner Mayer
 */
class GuiExport ToolBarManager: public QObject
{
    Q_OBJECT
public:
    /** Toolbar persistence scope used to separate global, workbench and contextual layouts. */
    enum class Scope
    {
        /** Historical name-based toolbar identity without scoped persistence metadata. */
        Legacy,
        /** Toolbar shared outside a specific workbench or contextual mode. */
        Shared,
        /** Toolbar owned by a specific workbench layout. */
        Workbench,
        /** Toolbar owned by a contextual mode inside a specific workbench. */
        Contextual,
    };

    /** Identifies the toolbar layout scope currently being saved or restored. */
    struct ToolbarScopeId
    {
        /** Scope kind for this layout id. */
        Scope scope = Scope::Legacy;
        /** Workbench name for workbench and contextual scopes. */
        QString workbench;
        /** Context name for contextual scopes, such as edit mode. */
        QString context;

        /** Return a workbench layout scope. */
        static ToolbarScopeId forWorkbench(QString workbench)
        {
            return {Scope::Workbench, std::move(workbench), {}};
        }

        /** Return a contextual layout scope inside a workbench. */
        static ToolbarScopeId forContextual(QString workbench, QString context)
        {
            return {Scope::Contextual, std::move(workbench), std::move(context)};
        }

        bool operator==(const ToolbarScopeId& other) const
        {
            return scope == other.scope && workbench == other.workbench && context == other.context;
        }
        bool operator!=(const ToolbarScopeId& other) const
        {
            return !(*this == other);
        }

        /** Return true when this is the default, unset scope id. */
        bool isEmpty() const
        {
            return scope == Scope::Legacy && workbench.isEmpty() && context.isEmpty();
        }
    };

    /** Stable toolbar identity used for layout persistence and migration. */
    struct PersistenceId
    {
        /** Serialized prefix used by shared toolbar identities. */
        enum class SharedPrefix
        {
            /** Standard shared toolbar prefix. */
            Shared,
            /** Legacy global toolbar prefix accepted for compatibility. */
            Global,
        };

        PersistenceId() = default;
        PersistenceId(
            ToolbarScopeId scopeId,
            QString toolbar,
            SharedPrefix sharedPrefix = SharedPrefix::Shared
        )
            : scopeId(std::move(scopeId))
            , toolbar(std::move(toolbar))
            , sharedPrefix(sharedPrefix)
        {}
        PersistenceId(
            Scope scope,
            QString toolbar,
            QString workbench = {},
            QString context = {},
            SharedPrefix sharedPrefix = SharedPrefix::Shared
        )
            : PersistenceId(
                  ToolbarScopeId {scope, std::move(workbench), std::move(context)},
                  std::move(toolbar),
                  sharedPrefix
              )
        {}

        /** Scope that owns this toolbar identity. */
        ToolbarScopeId scopeId;
        /** Stable toolbar id inside the owning scope. */
        QString toolbar;
        /** Prefix used when serializing shared toolbar identities. */
        SharedPrefix sharedPrefix = SharedPrefix::Shared;

        /** Return true when no toolbar id is set. */
        bool isEmpty() const
        {
            return toolbar.isEmpty();
        }

        /** Return the layout scope for this toolbar identity. */
        ToolbarScopeId toolbarScopeId() const
        {
            return scopeId;
        }
    };

    /** Toolbar state transition requested during workbench or context changes. */
    enum class State
    {
        ForceHidden,     // Forces a toolbar to hide and hides the toggle action
        ForceAvailable,  // Forces a toolbar toggle action to show, visibility depends on user config
        RestoreDefault,  // Restores a toolbar toggle action default, visibility as user config
        SaveState,       // Saves the state of the toolbars
    };

    /// The one and only instance.
    static ToolBarManager* getInstance();
    static void destruct();
    static QString toolBarPersistenceKey(const ToolBarItem*);
    static QString toolBarPersistenceKey(const QToolBar*);
    static PersistenceId toolBarPersistenceId(const QString& persistenceKey);
    static PersistenceId toolBarPersistenceId(const ToolBarItem*);
    static PersistenceId toolBarPersistenceId(const QToolBar*);
    static ToolbarScopeId layoutContextId(const QString& context);
    static QString makeToolBarLayoutContext(const ToolbarScopeId& scopeId);
    static QString makeToolBarPersistenceKey(const PersistenceId&);
    static ToolbarScopeId toolBarScopeId(const QString& persistenceKey);
    static ToolbarScopeId toolBarScopeId(const ToolBarItem*);
    static ToolbarScopeId toolBarScopeId(const QToolBar*);
    static QString toolBarScopeLabel(const QString& persistenceKey);
    static QString toolBarScopeLabel(const ToolBarItem*);
    static QString toolBarScopeLabel(const QToolBar*);
    static ToolBarItem::Tier toolBarTier(const ToolBarItem*);
    static ToolBarItem::Tier toolBarTier(const QToolBar*);
    static ToolBarItem::Tier normalizeCustomToolBarTier(ToolBarItem::Tier);
    static ToolBarItem::Tier customToolBarTierFromName(const QString&);
    static ToolBarItem::Tier toolBarTierFromName(const QString&);
    static QString toolBarTierName(ToolBarItem::Tier);
    static QString toolBarTierLabel(ToolBarItem::Tier);
    static QString toolBarTierLabel(const ToolBarItem*);
    static QString toolBarTierLabel(const QToolBar*);
    static void setToolBarPersistenceKey(QToolBar*, const QString&);
    static void setToolBarTier(QToolBar*, ToolBarItem::Tier);

    /** Sets up the toolbars of a given workbench. */
    void setup(ToolBarItem*);
    void saveState() const;
    void restoreState();
    void retranslate() const;
    void populateToolBarMenu(QMenu* menu);
    void setToolbarLayoutContextOverride(const QString& workbench, const QString& context);
    void setToolbarLayoutContextOverride(const QString& workbench, const ToolbarScopeId& context);
    void clearToolbarLayoutContextOverride(const QString& workbench);
    QString currentToolbarLayoutScopeLabel() const;
    QString currentToolbarLayoutResetLabel() const;
    QString currentRecommendedToolbarLayoutResetLabel() const;
    QString currentShowRecommendedOnlyLabel() const;
    void resetCurrentToolbarLayout();
    void resetCurrentToolbarLayoutToRecommended();
    void showRecommendedToolBarsOnly();

    bool areToolBarsLocked() const;
    void setToolBarsLocked(bool locked) const;

    void setState(const QList<QString>& names, State state);
    void setState(const QString& name, State state);
    void setState(const QList<PersistenceId>& ids, State state);
    void setState(const PersistenceId& id, State state);

    int toolBarIconSize(QWidget* widget = nullptr) const;
    void setupToolBarIconSize();

    ToolBarArea toolBarArea(QWidget* toolBar) const;
    ToolBarAreaWidget* toolBarAreaWidget(QWidget* toolBar) const;

Q_SIGNALS:
    void toolbarLayoutContextChanged();
    void toolbarLayoutScopeRestored(const ToolbarScopeId& context);
    void toolbarLayoutRestored(const QString& context);

protected:
    void setup(ToolBarItem*, QToolBar*) const;

    void setMovable(bool movable) const;

    ToolBarItem::DefaultVisibility getToolbarPolicy(const QToolBar*) const;

    bool addToolBarToArea(QObject* source, QMouseEvent* ev);
    bool showContextMenu(QObject* source);
    void onToggleStatusBarWidget(QWidget* widget, bool visible);
    void setToolBarIconSize(QToolBar* toolbar);
    void onTimer();

    bool eventFilter(QObject* source, QEvent* ev) override;

    /** Returns a list of all currently existing toolbars. */
    QList<ToolBar*> toolBars() const;
    ToolBar* findToolBar(const QList<ToolBar*>&, const QString&) const;
    QAction* findAction(const QList<QAction*>&, const QString&) const;
    ToolBarManager();
    ~ToolBarManager() override;

private:
    enum class CurrentLayoutScope
    {
        None,
        Workbench,
        Contextual,
    };

    void setupParameters();
    void setupStatusBar();
    void setupMenuBar();
    void setupConnection();
    void setupTimer();
    void setupSizeTimer();
    void setupResizeTimer();
    void setupMenuBarTimer();
    void setupWidgetProducers();
    void onToolbarParametersChanged(const ParamKey*);
    void addToolBarActionsByScope(QMenu* menu, const QList<QToolBar*>& toolbars) const;
    void addCurrentToolbarLayoutActions(QMenu* menu);
    ToolbarScopeId activeToolbarLayoutContext() const;
    ToolbarScopeId effectiveToolbarLayoutContext() const;
    CurrentLayoutScope currentToolbarLayoutScope(
        ToolbarScopeId* layoutContext = nullptr,
        ToolbarScopeId* activeContext = nullptr
    ) const;
    bool rememberToolbarLayoutByWorkbench() const;
    bool hasSavedWorkbenchToolBarLayout(const ToolbarScopeId& context) const;
    bool toolbarBelongsToLayoutContext(const QToolBar* toolbar, const ToolbarScopeId& context) const;
    void activateToolbarLayoutContext(const ToolbarScopeId& context);
    void moveToolBarToMainWindow(QToolBar* toolbar) const;
    void deactivateToolBarForScope(QToolBar* toolbar) const;
    void initializeUnsavedToolbarLayoutContext(const ToolbarScopeId& context);
    void updateLayoutParameters(const ToolbarScopeId& context);
    ParameterGrp::handle workbenchLayoutGroup(const ToolbarScopeId& context) const;
    void saveWorkbenchToolBarLayout(const ToolbarScopeId& context) const;
    void resetMainWindowToolBarLayout() const;
    bool recommendedToolBarVisibility(const QToolBar* toolbar) const;
    void applyRecommendedToolBarPreferences();
    void applyRecommendedToolBarVisibility();

    void addToMenu(QLayout* layout, QWidget* area, QMenu* menu);
    QLayout* findLayoutOfObject(QObject* source, QWidget* area) const;
    ToolBarAreaWidget* findToolBarAreaWidget() const;

private:
    QStringList toolbarKeys;
    ToolbarScopeId toolbarLayoutContext;
    QString toolbarLayoutContextOverrideWorkbench;
    ToolbarScopeId toolbarLayoutContextOverride;
    static ToolBarManager* _instance;

    QTimer timer;
    QTimer menuBarTimer;
    QTimer sizeTimer;
    QTimer resizeTimer;
    ParamHandlers paramHandlers;
    ToolBarAreaWidget* statusBarAreaWidget = nullptr;
    ToolBarAreaWidget* menuBarLeftAreaWidget = nullptr;
    ToolBarAreaWidget* menuBarRightAreaWidget = nullptr;
    ParameterGrp::handle hGeneral;
    ParameterGrp::handle hMainWindow;
    ParameterGrp::handle hPref;
    ParameterGrp::handle hWorkbenchLayouts;
    ParameterGrp::handle hGlobalStatusBar;
    ParameterGrp::handle hGlobalMenuBarLeft;
    ParameterGrp::handle hGlobalMenuBarRight;
    ParameterGrp::handle hStatusBar;
    ParameterGrp::handle hMenuBarLeft;
    ParameterGrp::handle hMenuBarRight;
    std::map<QToolBar*, QPointer<QToolBar>> resizingToolbars;
    int _toolBarIconSize = 0;
    int _statusBarIconSize = 0;
    int _menuBarIconSize = 0;
    bool blockRestore = false;
};

}  // namespace Gui

Q_DECLARE_METATYPE(Gui::ToolBarManager::ToolbarScopeId)
