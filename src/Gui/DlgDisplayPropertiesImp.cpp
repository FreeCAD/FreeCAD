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
# include <algorithm>
# include <boost/bind.hpp>
# include <QDockWidget>
#endif

#include "DlgDisplayPropertiesImp.h"
#include "DlgMaterialPropertiesImp.h"
#include "DockWindowManager.h"
#include "View3DInventorViewer.h"
#include "View3DInventor.h"
#include "Command.h"
#include "Application.h"
#include "Widgets.h"
#include "Selection.h"
#include "Document.h"
#include "ViewProvider.h"
#include "WaitCursor.h"
#include "SpinBox.h"

#include <Base/Console.h>
#include <App/Application.h>
#include <App/DocumentObject.h>
#include <App/Material.h>

using namespace Gui::Dialog;
using namespace std;


/* TRANSLATOR Gui::Dialog::DlgDisplayPropertiesImp */

#if 0 // needed for Qt's lupdate utility
    qApp->translate("QDockWidget", "Display properties");
#endif

/**
 *  Constructs a DlgDisplayPropertiesImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgDisplayPropertiesImp::DlgDisplayPropertiesImp( QWidget* parent, Qt::WFlags fl )
  : QDialog( parent, fl )
{
    this->setupUi(this);
    textLabel1_3->hide();
    changePlot->hide();
    buttonLineColor->setModal(false);
    buttonColor->setModal(false);

    std::vector<Gui::ViewProvider*> views = getSelection();
    setDisplayModes(views);
    fillupMaterials();
    setMaterial(views);
    setColorPlot(views);
    setShapeColor(views);
    setLineColor(views);
    setPointSize(views);
    setLineWidth(views);
    setTransparency(views);
    setLineTransparency(views);

    // embed this dialog into a dockable widget container
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    QDockWidget* dw = pDockMgr->addDockWindow("Display properties", this, Qt::AllDockWidgetAreas);
    dw->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
    dw->setAllowedAreas(Qt::DockWidgetAreas());
    dw->setFloating(true);
    dw->show();

    Gui::Selection().Attach(this);

    this->connectChangedObject =
    Gui::Application::Instance->signalChangedObject.connect(boost::bind
        (&DlgDisplayPropertiesImp::slotChangedObject, this, _1, _2));
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgDisplayPropertiesImp::~DlgDisplayPropertiesImp()
{
    // no need to delete child widgets, Qt does it all for us
    this->connectChangedObject.disconnect();
    Gui::Selection().Detach(this);
}

void DlgDisplayPropertiesImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        this->retranslateUi(this);
    }
    QDialog::changeEvent(e);
}

/// @cond DOXERR
void DlgDisplayPropertiesImp::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                                       Gui::SelectionSingleton::MessageType Reason)
{
    if (Reason.Type == SelectionChanges::AddSelection ||
        Reason.Type == SelectionChanges::RmvSelection ||
        Reason.Type == SelectionChanges::SetSelection ||
        Reason.Type == SelectionChanges::ClrSelection) {
        std::vector<Gui::ViewProvider*> views = getSelection();
        setDisplayModes(views);
        setMaterial(views);
        setColorPlot(views);
        setShapeColor(views);
        setLineColor(views);
        setPointSize(views);
        setLineWidth(views);
        setTransparency(views);
        setLineTransparency(views);
    }
}
/// @endcond

void DlgDisplayPropertiesImp::slotChangedObject(const Gui::ViewProvider& obj,
                                                const App::Property& prop)
{
    // This method gets called if a property of any view provider is changed.
    // We pick out all the properties for which we need to update this dialog.
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    std::vector<Gui::ViewProvider*>::const_iterator vp = std::find_if
        (Provider.begin(), Provider.end(), 
        std::bind2nd(std::equal_to<Gui::ViewProvider*>(),
        const_cast<Gui::ViewProvider*>(&obj)));
    if (vp != Provider.end()) {
        std::string prop_name = obj.getName(&prop);
        if (prop.getTypeId() == App::PropertyColor::getClassTypeId()) {
            App::Color value = static_cast<const App::PropertyColor&>(prop).getValue();
            if (prop_name == "ShapeColor") {
                bool blocked = buttonColor->blockSignals(true);
                buttonColor->setColor(QColor((int)(255.0f*value.r),
                                             (int)(255.0f*value.g),
                                             (int)(255.0f*value.b)));
                buttonColor->blockSignals(blocked);
            }
            else if (prop_name == "LineColor") {
                bool blocked = buttonLineColor->blockSignals(true);
                buttonLineColor->setColor(QColor((int)(255.0f*value.r),
                                                 (int)(255.0f*value.g),
                                                 (int)(255.0f*value.b)));
                buttonLineColor->blockSignals(blocked);
            }
        }
        else if (prop.getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            long value = static_cast<const App::PropertyInteger&>(prop).getValue();
            if (prop_name == "Transparency") {
                bool blocked = spinTransparency->blockSignals(true);
                spinTransparency->setValue(value);
                spinTransparency->blockSignals(blocked);
                blocked = horizontalSlider->blockSignals(true);
                horizontalSlider->setValue(value);
                horizontalSlider->blockSignals(blocked);
            }
            else if (prop_name == "LineTransparency") {
                bool blocked = spinLineTransparency->blockSignals(true);
                spinLineTransparency->setValue(value);
                spinLineTransparency->blockSignals(blocked);
                blocked = sliderLineTransparency->blockSignals(true);
                sliderLineTransparency->setValue(value);
                sliderLineTransparency->blockSignals(blocked);
            }
        }
        else if (prop.getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            double value = static_cast<const App::PropertyFloat&>(prop).getValue();
            if (prop_name == "PointSize") {
                bool blocked = spinPointSize->blockSignals(true);
                spinPointSize->setValue((int)value);
                spinPointSize->blockSignals(blocked);
            }
            else if (prop_name == "LineWidth") {
                bool blocked = spinLineWidth->blockSignals(true);
                spinLineWidth->setValue((int)value);
                spinLineWidth->blockSignals(blocked);
            }
        }
    }
}

/**
 * Destroys the dock window this object is embedded into without destroying itself.
 */
void DlgDisplayPropertiesImp::reject()
{
    // closes the dock window
    Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
    pDockMgr->removeDockWindow(this);
    QDialog::reject();
}

/**
 * Opens a dialog that allows to modify the 'ShapeMaterial' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::on_buttonUserDefinedMaterial_clicked()
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    DlgMaterialPropertiesImp dlg("ShapeMaterial", this);
    dlg.setViewProviders(Provider);
    dlg.exec();

    buttonColor->setColor(dlg.diffuseColor->color());
}

/**
 * Opens a dialog that allows to modify the 'ShapeMaterial' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::on_buttonColorPlot_clicked()
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    static QPointer<DlgMaterialPropertiesImp> dlg = 0;
    if (!dlg)
        dlg = new DlgMaterialPropertiesImp("TextureMaterial", this);
    dlg->setModal(false);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setViewProviders(Provider);
    dlg->show();
}

/**
 * Sets the 'ShapeMaterial' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::on_changeMaterial_activated(const QString& material)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    App::Material mat(Materials[material]);
    App::Color diffuseColor = mat.diffuseColor;
    buttonColor->setColor(QColor((int)(diffuseColor.r*255.0f), (int)(diffuseColor.g*255.0f), (int)(diffuseColor.b*255.0f)));

    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();It++) {
        App::Property* prop = (*It)->getPropertyByName("ShapeMaterial");
        if (prop && prop->getTypeId() == App::PropertyMaterial::getClassTypeId()) {
            App::PropertyMaterial* ShapeMaterial = (App::PropertyMaterial*)prop;
            ShapeMaterial->setValue(mat);
        }
    }
}

/**
 * Sets the 'Display' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::on_changeMode_activated(const QString& s)
{
    Gui::WaitCursor wc;
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();It++) {
        App::Property* prop = (*It)->getPropertyByName("DisplayMode");
        if (prop && prop->getTypeId() == App::PropertyEnumeration::getClassTypeId()) {
            App::PropertyEnumeration* Display = (App::PropertyEnumeration*)prop;
            Display->setValue((const char*)s.toAscii());
        }
    }
}

void DlgDisplayPropertiesImp::on_changePlot_activated(const QString&s)
{
    Base::Console().Log("Plot = %s\n",(const char*)s.toAscii());
}

/**
 * Sets the 'ShapeColor' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::on_buttonColor_changed()
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    QColor s = buttonColor->color();
    App::Color c(s.red()/255.0,s.green()/255.0,s.blue()/255.0);
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();It++) {
        App::Property* prop = (*It)->getPropertyByName("ShapeColor");
        if (prop && prop->getTypeId() == App::PropertyColor::getClassTypeId()) {
            App::PropertyColor* ShapeColor = (App::PropertyColor*)prop;
            ShapeColor->setValue(c);
        }
    }
}

/**
 * Sets the 'Transparency' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::on_spinTransparency_valueChanged(int transparency)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();It++) {
        App::Property* prop = (*It)->getPropertyByName("Transparency");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            App::PropertyInteger* Transparency = (App::PropertyInteger*)prop;
            Transparency->setValue(transparency);
        }
    }
}

/**
 * Sets the 'PointSize' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::on_spinPointSize_valueChanged(int pointsize)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();It++) {
        App::Property* prop = (*It)->getPropertyByName("PointSize");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            App::PropertyFloat* PointSize = (App::PropertyFloat*)prop;
            PointSize->setValue((double)pointsize);
        }
    }
}

/**
 * Sets the 'LineWidth' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::on_spinLineWidth_valueChanged(int linewidth)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();It++) {
        App::Property* prop = (*It)->getPropertyByName("LineWidth");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            App::PropertyFloat* LineWidth = (App::PropertyFloat*)prop;
            LineWidth->setValue((double)linewidth);
        }
    }
}

void DlgDisplayPropertiesImp::on_buttonLineColor_changed()
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    QColor s = buttonLineColor->color();
    App::Color c(s.red()/255.0,s.green()/255.0,s.blue()/255.0);
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();It++) {
        App::Property* prop = (*It)->getPropertyByName("LineColor");
        if (prop && prop->getTypeId() == App::PropertyColor::getClassTypeId()) {
            App::PropertyColor* ShapeColor = (App::PropertyColor*)prop;
            ShapeColor->setValue(c);
        }
    }
}

void DlgDisplayPropertiesImp::on_spinLineTransparency_valueChanged(int transparency)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();It++) {
        App::Property* prop = (*It)->getPropertyByName("LineTransparency");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            App::PropertyInteger* Transparency = (App::PropertyInteger*)prop;
            Transparency->setValue(transparency);
        }
    }
}

void DlgDisplayPropertiesImp::setDisplayModes(const std::vector<Gui::ViewProvider*>& views)
{
    QStringList commonModes, modes;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("DisplayMode");
        if (prop && prop->getTypeId() == App::PropertyEnumeration::getClassTypeId()) {
            App::PropertyEnumeration* display = static_cast<App::PropertyEnumeration*>(prop);
            if (!display->getEnums()) return;
            const std::vector<std::string>& value = display->getEnumVector();
            if (it == views.begin()) {
                for (std::vector<std::string>::const_iterator jt = value.begin(); jt != value.end(); ++jt)
                    commonModes << QLatin1String(jt->c_str());
            }
            else {
                for (std::vector<std::string>::const_iterator jt = value.begin(); jt != value.end(); ++jt) {
                    if (commonModes.contains(QLatin1String(jt->c_str())))
                        modes << QLatin1String(jt->c_str());
                }

                commonModes = modes;
                modes.clear();
            }
        }
    }

    changeMode->clear();
    changeMode->addItems(commonModes);
    changeMode->setDisabled(commonModes.isEmpty());

    // find the display mode to activate
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("DisplayMode");
        if (prop && prop->getTypeId() == App::PropertyEnumeration::getClassTypeId()) {
            App::PropertyEnumeration* display = static_cast<App::PropertyEnumeration*>(prop);
            QString activeMode = QString::fromAscii(display->getValueAsString());
            int index = changeMode->findText(activeMode);
            if (index != -1) {
                changeMode->setCurrentIndex(index);
                break;
            }
        }
    }
}

void DlgDisplayPropertiesImp::setMaterial(const std::vector<Gui::ViewProvider*>& views)
{
    bool material = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("ShapeMaterial");
        if (prop && prop->getTypeId() == App::PropertyMaterial::getClassTypeId()) {
            material = true;
            break;
        }
    }

    changeMaterial->setEnabled(material);
    buttonUserDefinedMaterial->setEnabled(material);
}

void DlgDisplayPropertiesImp::setColorPlot(const std::vector<Gui::ViewProvider*>& views)
{
    bool material = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("TextureMaterial");
        if (prop && prop->getTypeId() == App::PropertyMaterial::getClassTypeId()) {
            material = true;
            break;
        }
    }

    buttonColorPlot->setEnabled(material);
}

void DlgDisplayPropertiesImp::fillupMaterials()
{
    Materials[QLatin1String("Brass")]         = App::Material::BRASS;
    Materials[QLatin1String("Bronze")]        = App::Material::BRONZE;
    Materials[QLatin1String("Copper")]        = App::Material::COPPER;
    Materials[QLatin1String("Gold")]          = App::Material::GOLD;
    Materials[QLatin1String("Pewter")]        = App::Material::PEWTER;
    Materials[QLatin1String("Plaster")]       = App::Material::PLASTER;
    Materials[QLatin1String("Plastic")]       = App::Material::PLASTIC;
    Materials[QLatin1String("Silver")]        = App::Material::SILVER;
    Materials[QLatin1String("Steel")]         = App::Material::STEEL;
    Materials[QLatin1String("Stone")]         = App::Material::STONE;
    Materials[QLatin1String("Shiny plastic")] = App::Material::SHINY_PLASTIC;
    Materials[QLatin1String("Satin")]         = App::Material::SATIN;
    Materials[QLatin1String("Metalized")]     = App::Material::METALIZED;
    Materials[QLatin1String("Neon GNC")]      = App::Material::NEON_GNC;
    Materials[QLatin1String("Chrome")]        = App::Material::CHROME;
    Materials[QLatin1String("Aluminium")]     = App::Material::ALUMINIUM;
    Materials[QLatin1String("Obsidian")]      = App::Material::OBSIDIAN;
    Materials[QLatin1String("Neon PHC")]      = App::Material::NEON_PHC;
    Materials[QLatin1String("Jade")]          = App::Material::JADE;
    Materials[QLatin1String("Ruby")]          = App::Material::RUBY;
    Materials[QLatin1String("Emerald")]       = App::Material::EMERALD;

    QStringList material = Materials.keys();
    material.sort();
    changeMaterial->addItem(QLatin1String("Default"));
    changeMaterial->addItems(material);
    Materials[QLatin1String("Default")]       = App::Material::DEFAULT;
}

void DlgDisplayPropertiesImp::setShapeColor(const std::vector<Gui::ViewProvider*>& views)
{
    bool shapeColor = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("ShapeColor");
        if (prop && prop->getTypeId() == App::PropertyColor::getClassTypeId()) {
            App::Color c = static_cast<App::PropertyColor*>(prop)->getValue();
            QColor shape;
            shape.setRgb((int)(c.r*255.0f), (int)(c.g*255.0f),(int)(c.b*255.0f));
            bool blocked = buttonColor->blockSignals(true);
            buttonColor->setColor(shape);
            buttonColor->blockSignals(blocked);
            shapeColor = true;
            break;
        }
    }

    buttonColor->setEnabled(shapeColor);
}

void DlgDisplayPropertiesImp::setLineColor(const std::vector<Gui::ViewProvider*>& views)
{
    bool shapeColor = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("LineColor");
        if (prop && prop->getTypeId() == App::PropertyColor::getClassTypeId()) {
            App::Color c = static_cast<App::PropertyColor*>(prop)->getValue();
            QColor shape;
            shape.setRgb((int)(c.r*255.0f), (int)(c.g*255.0f),(int)(c.b*255.0f));
            bool blocked = buttonLineColor->blockSignals(true);
            buttonLineColor->setColor(shape);
            buttonLineColor->blockSignals(blocked);
            shapeColor = true;
            break;
        }
    }

    buttonLineColor->setEnabled(shapeColor);
}

void DlgDisplayPropertiesImp::setPointSize(const std::vector<Gui::ViewProvider*>& views)
{
    bool pointSize = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("PointSize");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            bool blocked = spinPointSize->blockSignals(true);
            spinPointSize->setValue((int)static_cast<App::PropertyFloat*>(prop)->getValue());
            spinPointSize->blockSignals(blocked);
            pointSize = true;
            break;
        }
    }

    spinPointSize->setEnabled(pointSize);
}

void DlgDisplayPropertiesImp::setLineWidth(const std::vector<Gui::ViewProvider*>& views)
{
    bool lineWidth = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("LineWidth");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            bool blocked = spinLineWidth->blockSignals(true);
            spinLineWidth->setValue((int)static_cast<App::PropertyFloat*>(prop)->getValue());
            spinLineWidth->blockSignals(blocked);
            lineWidth = true;
            break;
        }
    }

    spinLineWidth->setEnabled(lineWidth);
}

void DlgDisplayPropertiesImp::setTransparency(const std::vector<Gui::ViewProvider*>& views)
{
    bool transparency = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("Transparency");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            bool blocked = spinTransparency->blockSignals(true);
            spinTransparency->setValue(static_cast<App::PropertyInteger*>(prop)->getValue());
            spinTransparency->blockSignals(blocked);

            blocked = horizontalSlider->blockSignals(true);
            horizontalSlider->setValue(static_cast<App::PropertyInteger*>(prop)->getValue());
            horizontalSlider->blockSignals(blocked);
            transparency = true;
            break;
        }
    }

    spinTransparency->setEnabled(transparency);
    horizontalSlider->setEnabled(transparency);
}

void DlgDisplayPropertiesImp::setLineTransparency(const std::vector<Gui::ViewProvider*>& views)
{
    bool transparency = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("LineTransparency");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            bool blocked = spinLineTransparency->blockSignals(true);
            spinLineTransparency->setValue(static_cast<App::PropertyInteger*>(prop)->getValue());
            spinLineTransparency->blockSignals(blocked);

            blocked = sliderLineTransparency->blockSignals(true);
            sliderLineTransparency->setValue(static_cast<App::PropertyInteger*>(prop)->getValue());
            sliderLineTransparency->blockSignals(blocked);
            transparency = true;
            break;
        }
    }

    spinLineTransparency->setEnabled(transparency);
    sliderLineTransparency->setEnabled(transparency);
}

std::vector<Gui::ViewProvider*> DlgDisplayPropertiesImp::getSelection() const
{
    std::vector<Gui::ViewProvider*> views;

    // get the complete selection
    std::vector<SelectionSingleton::SelObj> sel = Selection().getCompleteSelection();
    for (std::vector<SelectionSingleton::SelObj>::iterator it = sel.begin(); it != sel.end(); ++it) {
        Gui::ViewProvider* view = Application::Instance->getDocument(it->pDoc)->getViewProvider(it->pObject);
        views.push_back(view);
    }

    return views;
}

#include "moc_DlgDisplayPropertiesImp.cpp"

