/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <QDockWidget>
#include <QSignalBlocker>
#include <QString>
#include <algorithm>
#include <boost/signals2.hpp>
#endif

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/ModelUuids.h>
#include <Mod/Material/App/PropertyMaterial.h>

#include "DlgMaterialImp.h"
#include "ui_DlgMaterial.h"


using namespace MatGui;
using namespace std;
namespace sp = std::placeholders;


/* TRANSLATOR Gui::Dialog::DlgMaterialImp */

#if 0  // needed for Qt's lupdate utility
    qApp->translate("QDockWidget", "Material");
#endif

class DlgMaterialImp::Private
{
    using DlgMaterialImp_Connection = boost::signals2::connection;

public:
    Ui::DlgMaterial ui;
    bool floating;
    DlgMaterialImp_Connection connectChangedObject;
};

/**
 *  Constructs a DlgMaterialImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgMaterialImp::DlgMaterialImp(bool floating, QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , d(new Private)
{
    d->ui.setupUi(this);
    setupConnections();

    d->floating = floating;

    // Create a filter to only include current format materials
    // that contain physical properties.
    Materials::MaterialFilter filter;
    filter.requirePhysical(true);
    d->ui.widgetMaterial->setFilter(filter);

    std::vector<App::DocumentObject*> objects = getSelectionObjects();
    setMaterial(objects);

    // embed this dialog into a dockable widget container
    if (floating) {
        Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
        QDockWidget* dw =
            pDockMgr->addDockWindow("Display Properties", this, Qt::AllDockWidgetAreas);
        dw->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
        dw->setFloating(true);
        dw->show();
    }

    Gui::Selection().Attach(this);

    // NOLINTBEGIN
    d->connectChangedObject = Gui::Application::Instance->signalChangedObject.connect(
        std::bind(&DlgMaterialImp::slotChangedObject, this, sp::_1, sp::_2));
    // NOLINTEND
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgMaterialImp::~DlgMaterialImp()
{
    // no need to delete child widgets, Qt does it all for us
    d->connectChangedObject.disconnect();
    Gui::Selection().Detach(this);
}

void DlgMaterialImp::setupConnections()
{
    connect(d->ui.widgetMaterial,
            &MaterialTreeWidget::materialSelected,
            this,
            &DlgMaterialImp::onMaterialSelected);
}

void DlgMaterialImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
    }
    QDialog::changeEvent(e);
}

/// @cond DOXERR
void DlgMaterialImp::OnChange(Gui::SelectionSingleton::SubjectType& rCaller,
                              Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
    if (Reason.Type == Gui::SelectionChanges::AddSelection
        || Reason.Type == Gui::SelectionChanges::RmvSelection
        || Reason.Type == Gui::SelectionChanges::SetSelection
        || Reason.Type == Gui::SelectionChanges::ClrSelection) {
        std::vector<App::DocumentObject*> objects = getSelectionObjects();
        setMaterial(objects);
    }
}
/// @endcond

void DlgMaterialImp::slotChangedObject(const Gui::ViewProvider& obj, const App::Property& prop)
{
    // This method gets called if a property of any view provider is changed.
    // We pick out all the properties for which we need to update this dialog.
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    auto vp = std::find_if(Provider.begin(), Provider.end(), [&obj](Gui::ViewProvider* v) {
        return v == &obj;
    });

    if (vp != Provider.end()) {
        const char* name = obj.getPropertyName(&prop);
        // this is not a property of the view provider but of the document object
        if (!name) {
            return;
        }
        std::string prop_name = name;
        if (prop.isDerivedFrom<App::PropertyMaterial>()) {
            //auto& value = static_cast<const App::PropertyMaterial&>(prop).getValue();
            if (prop_name == "ShapeMaterial") {
                // bool blocked = d->ui.buttonColor->blockSignals(true);
                // auto color = value.diffuseColor;
                // d->ui.buttonColor->setColor(QColor((int)(255.0f * color.r),
                //                                    (int)(255.0f * color.g),
                //                                    (int)(255.0f * color.b)));
                // d->ui.buttonColor->blockSignals(blocked);
            }
        }
    }
}

/**
 * Destroys the dock window this object is embedded into without destroying itself.
 */
void DlgMaterialImp::reject()
{
    if (d->floating) {
        // closes the dock window
        Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
        pDockMgr->removeDockWindow(this);
    }
    QDialog::reject();
}

void DlgMaterialImp::setMaterial(const std::vector<App::DocumentObject*>& objects)
{
    for (auto it : objects) {
        if (auto prop = dynamic_cast<Materials::PropertyMaterial*>(it->getPropertyByName("ShapeMaterial"))) {
            try {
                const auto& material = prop->getValue();
                d->ui.widgetMaterial->setMaterial(material.getUUID());
                return;
            }
            catch (const Materials::MaterialNotFound&) {
            }
        }
    }
    d->ui.widgetMaterial->setMaterial(Materials::MaterialManager::defaultMaterialUUID());
}

std::vector<Gui::ViewProvider*> DlgMaterialImp::getSelection() const
{
    std::vector<Gui::ViewProvider*> views;

    // get the complete selection
    std::vector<Gui::SelectionSingleton::SelObj> sel = Gui::Selection().getCompleteSelection();
    for (const auto& it : sel) {
        Gui::ViewProvider* view =
            Gui::Application::Instance->getDocument(it.pDoc)->getViewProvider(it.pObject);
        views.push_back(view);
    }

    return views;
}

std::vector<App::DocumentObject*> DlgMaterialImp::getSelectionObjects() const
{
    std::vector<App::DocumentObject*> objects;

    // get the complete selection
    std::vector<Gui::SelectionSingleton::SelObj> sel = Gui::Selection().getCompleteSelection();
    for (const auto& it : sel) {
        objects.push_back(it.pObject);
    }

    return objects;
}

void DlgMaterialImp::onMaterialSelected(const std::shared_ptr<Materials::Material>& material)
{
    std::vector<App::DocumentObject*> objects = getSelectionObjects();
    for (auto it : objects) {
        if (auto prop = dynamic_cast<Materials::PropertyMaterial*>(it->getPropertyByName("ShapeMaterial"))) {
            prop->setValue(*material);
        }
    }
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::TaskMaterial */

TaskMaterial::TaskMaterial()
{
    this->setButtonPosition(TaskMaterial::North);
    widget = new DlgMaterialImp(false);
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskMaterial::~TaskMaterial() = default;

QDialogButtonBox::StandardButtons TaskMaterial::getStandardButtons() const
{
    return QDialogButtonBox::Close;
}

bool TaskMaterial::reject()
{
    widget->reject();
    return (widget->result() == QDialog::Rejected);
}

#include "moc_DlgMaterialImp.cpp"
