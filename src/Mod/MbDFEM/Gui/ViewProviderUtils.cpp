// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderUtils.h"

#include <cstring>
#include <string>

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QMenu>
#include <QObject>

#include <Inventor/SoFullPath.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSeparator.h>

#include <App/Document.h>
#include <App/OriginGroupExtension.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>

namespace
{

constexpr const char* mbdfemOtherMenuProperty = "MbDFEMOtherMenu";

bool addCommandToMenu(QMenu* menu, const char* commandName)
{
    auto* command = commandName
        ? Gui::Application::Instance->commandManager().getCommandByName(commandName)
        : nullptr;
    if (!command) {
        return false;
    }

    command->addTo(menu);
    command->testActive();
    return true;
}

bool isMbDFEMOtherMenuAction(QAction* action)
{
    return action && action->property(mbdfemOtherMenuProperty).toBool();
}

void moveTrailingActionsToOther(QMenu* menu, QMenu* otherMenu)
{
    if (!menu || !otherMenu) {
        return;
    }

    const auto actions = menu->actions();
    auto* otherAction = otherMenu->menuAction();
    bool collect = false;
    for (auto* action : actions) {
        if (action == otherAction) {
            collect = true;
            continue;
        }
        if (!collect || isMbDFEMOtherMenuAction(action)) {
            continue;
        }

        auto* submenu = action->menu();
        if (submenu && submenu == otherMenu) {
            continue;
        }

        menu->removeAction(action);
        otherMenu->addAction(action);
    }
}

void finalizeMbDFEMOtherMenu(QMenu* menu)
{
    if (!menu) {
        return;
    }

    for (auto* action : menu->actions()) {
        if (isMbDFEMOtherMenuAction(action)) {
            moveTrailingActionsToOther(menu, action->menu());
            return;
        }
    }
}

class MbDFEMContextMenuFilter: public QObject
{
public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event && event->type() == QEvent::Show) {
            finalizeMbDFEMOtherMenu(qobject_cast<QMenu*>(watched));
        }
        return QObject::eventFilter(watched, event);
    }
};

void ensureContextMenuFilterInstalled()
{
    static auto* filter = new MbDFEMContextMenuFilter(qApp);
    static bool installed = false;
    if (!installed && qApp) {
        qApp->installEventFilter(filter);
        installed = true;
    }
}

}  // namespace

void MbDFEMGui::setOriginInTreeVisible(App::DocumentObject* object, bool visible)
{
    if (!object) {
        return;
    }

    auto* originGroup = object->getExtensionByType<App::OriginGroupExtension>();
    auto* origin = originGroup ? originGroup->Origin.getValue() : nullptr;
    auto* document = object->getDocument();
    auto* guiDocument = document ? Gui::Application::Instance->getDocument(document) : nullptr;
    auto* viewProvider = guiDocument && origin ? guiDocument->getViewProvider(origin) : nullptr;
    if (auto* documentViewProvider = freecad_cast<Gui::ViewProviderDocumentObject*>(viewProvider)) {
        documentViewProvider->ShowInTree.setValue(visible);
    }
}

void MbDFEMGui::hideOriginInTree(App::DocumentObject* object)
{
    setOriginInTreeVisible(object, false);
}

App::DocumentObject* MbDFEMGui::getOriginObject(App::DocumentObject* object)
{
    auto* originGroup = object ? object->getExtensionByType<App::OriginGroupExtension>() : nullptr;
    return originGroup ? originGroup->Origin.getValue() : nullptr;
}

bool MbDFEMGui::addMbDFEMContextMenuCommands(QMenu* menu,
                                             std::initializer_list<const char*> commandNames)
{
    if (!menu) {
        return false;
    }

    bool addedCommand = false;
    for (const char* commandName : commandNames) {
        addedCommand = addCommandToMenu(menu, commandName) || addedCommand;
    }

    return addedCommand;
}

QMenu* MbDFEMGui::addOtherContextMenu(QMenu* menu)
{
    if (!menu) {
        return nullptr;
    }

    addCommandToMenu(menu, "Std_TransformManip");
    addCommandToMenu(menu, "Std_Delete");

    auto* otherMenu = menu->addMenu(QObject::tr("Other"));
    auto* otherAction = otherMenu->menuAction();
    otherAction->setProperty(mbdfemOtherMenuProperty, true);

    ensureContextMenuFilterInstalled();
    QObject::connect(menu, &QMenu::aboutToShow, otherMenu, [menu, otherMenu]() {
        moveTrailingActionsToOther(menu, otherMenu);
    });

    return otherMenu;
}

bool MbDFEMGui::delegateSubobjectDetailPath(const Gui::ViewProviderDocumentObject* parent,
                                            const char* subname,
                                            SoFullPath* path,
                                            bool append,
                                            SoDetail*& det)
{
    if (!parent || !subname || !*subname || !path) {
        return false;
    }

    const int originalLength = path->getLength();
    const char* dot = std::strchr(subname, '.');
    if (!dot) {
        return false;
    }

    auto* object = parent->getObject();
    auto* child = object ? object->getSubObject(std::string(subname, dot - subname + 1).c_str()) : nullptr;
    auto* childViewProvider = child ? Gui::Application::Instance->getViewProvider(child) : nullptr;
    if (!childViewProvider) {
        return false;
    }

    if (auto* childRoot = parent->getChildRoot()) {
        if (append) {
            path->append(parent->getRoot());
        }
        path->append(childRoot);
    }
    else if (append) {
        path->append(parent->getRoot());
    }

    if (path->getLength()) {
        auto* tail = path->getTail();
        const SoChildList* children = tail ? tail->getChildren() : nullptr;
        if (children && children->find(childViewProvider->getRoot()) >= 0
            && childViewProvider->getDetailPath(dot + 1, path, true, det)) {
            return true;
        }
    }

    path->truncate(originalLength);
    return false;
}
