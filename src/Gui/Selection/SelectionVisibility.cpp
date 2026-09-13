// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Juergen Riegel <juergen.riegel@web.de>
// SPDX-FileCopyrightText: 2011 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 FreeCAD Project Association
// SPDX-FileNotice: Part of the FreeCAD project.

#include <set>
#include <string>
#include <utility>
#include <vector>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>

#include "Application.h"
#include "Selection.h"
#include "ViewProvider.h"

using namespace Gui;

namespace
{

struct SelInfo
{
    std::string DocName;
    std::string FeatName;
    std::string SubName;
    SelInfo(const std::string& docName, const std::string& featName, const std::string& subName)
        : DocName(docName)
        , FeatName(featName)
        , SubName(subName)
    {}
};

}  // namespace

void SelectionSingleton::setVisible(VisibleState vis)
{
    std::set<std::pair<App::DocumentObject*, App::DocumentObject*>> filter;
    int visible;
    switch (vis) {
        case VisShow:
            visible = 1;
            break;
        case VisToggle:
            visible = -1;
            break;
        default:
            visible = 0;
    }

    // Copy the selection in case it changes during this function
    std::vector<SelInfo> sels;
    sels.reserve(_SelList.size());
    for (auto& sel : _SelList) {
        if (sel.DocName.empty() || sel.FeatName.empty() || !sel.pObject) {
            continue;
        }
        sels.emplace_back(sel.DocName, sel.FeatName, sel.SubName);
    }

    for (auto& sel : sels) {
        App::Document* doc = App::GetApplication().getDocument(sel.DocName.c_str());
        if (!doc) {
            continue;
        }
        App::DocumentObject* obj = doc->getObject(sel.FeatName.c_str());
        if (!obj) {
            continue;
        }

        // get parent object
        App::DocumentObject* parent = nullptr;
        std::string elementName;
        obj = obj->resolve(sel.SubName.c_str(), &parent, &elementName);
        if (!obj || !obj->isAttachedToDocument() || (parent && !parent->isAttachedToDocument())) {
            continue;
        }
        // try call parent object's setElementVisible
        if (parent) {
            // prevent setting the same object visibility more than once
            if (!filter.insert(std::make_pair(obj, parent)).second) {
                continue;
            }
            int visElement = parent->isElementVisible(elementName.c_str());
            if (visElement >= 0) {
                if (visElement > 0) {
                    visElement = 1;
                }
                if (visible >= 0) {
                    if (visElement == visible) {
                        continue;
                    }
                    visElement = visible;
                }
                else {
                    visElement = !visElement;
                }

                if (!visElement) {
                    updateSelection(
                        false,
                        sel.DocName.c_str(),
                        sel.FeatName.c_str(),
                        sel.SubName.c_str()
                    );
                }
                parent->setElementVisible(elementName.c_str(), visElement ? true : false);
                if (visElement) {
                    updateSelection(true, sel.DocName.c_str(), sel.FeatName.c_str(), sel.SubName.c_str());
                }
                continue;
            }

            // Fall back to direct object visibility setting
        }
        if (!filter.insert(std::make_pair(obj, static_cast<App::DocumentObject*>(nullptr))).second) {
            continue;
        }

        auto vp = Application::Instance->getViewProvider(obj);

        if (vp) {
            if (visible < 0) {
                // Toggle link instead of the original object
                ViewProvider* toggleVp = vp;
                if (parent
                    && parent->hasExtension(App::LinkBaseExtension::getExtensionClassTypeId(), true)) {
                    if (auto* parentVp = Application::Instance->getViewProvider(parent)) {
                        toggleVp = parentVp;
                    }
                }
                toggleVp->toggleVisibility();
                updateSelection(
                    toggleVp->isShow(),
                    sel.DocName.c_str(),
                    sel.FeatName.c_str(),
                    sel.SubName.c_str()
                );
            }
            else {
                if (visible) {
                    vp->show();
                    updateSelection(
                        visible,
                        sel.DocName.c_str(),
                        sel.FeatName.c_str(),
                        sel.SubName.c_str()
                    );
                }
                else {
                    updateSelection(
                        visible,
                        sel.DocName.c_str(),
                        sel.FeatName.c_str(),
                        sel.SubName.c_str()
                    );
                    vp->hide();
                }
            }
        }
    }
}
