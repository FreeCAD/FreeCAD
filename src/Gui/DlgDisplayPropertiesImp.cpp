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
# include <boost_bind_bind.hpp>
# include <boost_signals2.hpp>
# include <QDockWidget>
#endif

#include "DlgDisplayPropertiesImp.h"
#include "ui_DlgDisplayProperties.h"
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
namespace bp = boost::placeholders;


/* TRANSLATOR Gui::Dialog::DlgDisplayPropertiesImp */

#if 0 // needed for Qt's lupdate utility
    qApp->translate("QDockWidget", "Display properties");
#endif

class DlgDisplayPropertiesImp::Private
{
    typedef boost::signals2::connection DlgDisplayPropertiesImp_Connection;
public:
    Ui::DlgDisplayProperties ui;
    bool floating;
    DlgDisplayPropertiesImp_Connection connectChangedObject;
};

/**
 *  Constructs a DlgDisplayPropertiesImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgDisplayPropertiesImp::DlgDisplayPropertiesImp(bool floating, QWidget* parent, Qt::WindowFlags fl)
  : QDialog( parent, fl )
  , d(new Private)
{
    d->ui.setupUi(this);
    d->ui.textLabel1_3->hide();
    d->ui.changePlot->hide();
    d->ui.buttonLineColor->setModal(false);
    d->ui.buttonColor->setModal(false);
    d->floating = floating;

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
    if (floating) {
        Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
        QDockWidget* dw = pDockMgr->addDockWindow("Display properties", this, Qt::AllDockWidgetAreas);
        dw->setFeatures(QDockWidget::DockWidgetMovable|QDockWidget::DockWidgetFloatable);
        dw->setFloating(true);
        dw->show();
    }

    Gui::Selection().Attach(this);

    d->connectChangedObject =
    Gui::Application::Instance->signalChangedObject.connect(boost::bind
        (&DlgDisplayPropertiesImp::slotChangedObject, this, bp::_1, bp::_2));
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgDisplayPropertiesImp::~DlgDisplayPropertiesImp()
{
    // no need to delete child widgets, Qt does it all for us
    d->connectChangedObject.disconnect();
    Gui::Selection().Detach(this);
}

void DlgDisplayPropertiesImp::showDefaultButtons(bool ok)
{
    d->ui.buttonBox->setVisible(ok);
}

void DlgDisplayPropertiesImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
    }
    QDialog::changeEvent(e);
}

/// @cond DOXERR
void DlgDisplayPropertiesImp::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                                       Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
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
        (Provider.begin(), Provider.end(), [&obj](Gui::ViewProvider* v) { return v == &obj; });

    if (vp != Provider.end()) {
        const char* name = obj.getPropertyName(&prop);
        // this is not a property of the view provider but of the document object
        if (!name)
            return;
        std::string prop_name = name;
        if (prop.getTypeId() == App::PropertyColor::getClassTypeId()) {
            App::Color value = static_cast<const App::PropertyColor&>(prop).getValue();
            if (prop_name == "ShapeColor") {
                bool blocked = d->ui.buttonColor->blockSignals(true);
                d->ui.buttonColor->setColor(QColor((int)(255.0f*value.r),
                                                   (int)(255.0f*value.g),
                                                   (int)(255.0f*value.b)));
                d->ui.buttonColor->blockSignals(blocked);
            }
            else if (prop_name == "LineColor") {
                bool blocked = d->ui.buttonLineColor->blockSignals(true);
                d->ui.buttonLineColor->setColor(QColor((int)(255.0f*value.r),
                                                       (int)(255.0f*value.g),
                                                       (int)(255.0f*value.b)));
                d->ui.buttonLineColor->blockSignals(blocked);
            }
        }
        else if (prop.getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            long value = static_cast<const App::PropertyInteger&>(prop).getValue();
            if (prop_name == "Transparency") {
                bool blocked = d->ui.spinTransparency->blockSignals(true);
                d->ui.spinTransparency->setValue(value);
                d->ui.spinTransparency->blockSignals(blocked);
                blocked = d->ui.horizontalSlider->blockSignals(true);
                d->ui.horizontalSlider->setValue(value);
                d->ui.horizontalSlider->blockSignals(blocked);
            }
            else if (prop_name == "LineTransparency") {
                bool blocked = d->ui.spinLineTransparency->blockSignals(true);
                d->ui.spinLineTransparency->setValue(value);
                d->ui.spinLineTransparency->blockSignals(blocked);
                blocked = d->ui.sliderLineTransparency->blockSignals(true);
                d->ui.sliderLineTransparency->setValue(value);
                d->ui.sliderLineTransparency->blockSignals(blocked);
            }
        }
        else if (prop.getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            double value = static_cast<const App::PropertyFloat&>(prop).getValue();
            if (prop_name == "PointSize") {
                bool blocked = d->ui.spinPointSize->blockSignals(true);
                d->ui.spinPointSize->setValue((int)value);
                d->ui.spinPointSize->blockSignals(blocked);
            }
            else if (prop_name == "LineWidth") {
                bool blocked = d->ui.spinLineWidth->blockSignals(true);
                d->ui.spinLineWidth->setValue((int)value);
                d->ui.spinLineWidth->blockSignals(blocked);
            }
        }
    }
}

/**
 * Destroys the dock window this object is embedded into without destroying itself.
 */
void DlgDisplayPropertiesImp::reject()
{
    if (d->floating) {
        // closes the dock window
        Gui::DockWindowManager* pDockMgr = Gui::DockWindowManager::instance();
        pDockMgr->removeDockWindow(this);
    }
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

    d->ui.buttonColor->setColor(dlg.diffuseColor());
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
void DlgDisplayPropertiesImp::on_changeMaterial_activated(int index)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    App::Material::MaterialType matType = static_cast<App::Material::MaterialType>(d->ui.changeMaterial->itemData(index).toInt());
    App::Material mat(matType);
    App::Color diffuseColor = mat.diffuseColor;
    d->ui.buttonColor->setColor(QColor((int)(diffuseColor.r*255.0f),
                                       (int)(diffuseColor.g*255.0f),
                                       (int)(diffuseColor.b*255.0f)));

    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();++It) {
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
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();++It) {
        App::Property* prop = (*It)->getPropertyByName("DisplayMode");
        if (prop && prop->getTypeId() == App::PropertyEnumeration::getClassTypeId()) {
            App::PropertyEnumeration* Display = (App::PropertyEnumeration*)prop;
            Display->setValue((const char*)s.toLatin1());
        }
    }
}

void DlgDisplayPropertiesImp::on_changePlot_activated(const QString&s)
{
    Base::Console().Log("Plot = %s\n",(const char*)s.toLatin1());
}

/**
 * Sets the 'ShapeColor' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::on_buttonColor_changed()
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    QColor s = d->ui.buttonColor->color();
    App::Color c(s.red()/255.0,s.green()/255.0,s.blue()/255.0);
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();++It) {
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
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();++It) {
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
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();++It) {
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
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();++It) {
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
    QColor s = d->ui.buttonLineColor->color();
    App::Color c(s.red()/255.0,s.green()/255.0,s.blue()/255.0);
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();++It) {
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
    for (std::vector<Gui::ViewProvider*>::iterator It= Provider.begin();It!=Provider.end();++It) {
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

    d->ui.changeMode->clear();
    d->ui.changeMode->addItems(commonModes);
    d->ui.changeMode->setDisabled(commonModes.isEmpty());

    // find the display mode to activate
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("DisplayMode");
        if (prop && prop->getTypeId() == App::PropertyEnumeration::getClassTypeId()) {
            App::PropertyEnumeration* display = static_cast<App::PropertyEnumeration*>(prop);
            QString activeMode = QString::fromLatin1(display->getValueAsString());
            int index = d->ui.changeMode->findText(activeMode);
            if (index != -1) {
                d->ui.changeMode->setCurrentIndex(index);
                break;
            }
        }
    }
}

void DlgDisplayPropertiesImp::setMaterial(const std::vector<Gui::ViewProvider*>& views)
{
    bool material = false;
    App::Material::MaterialType matType = App::Material::DEFAULT;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("ShapeMaterial");
        if (prop && prop->getTypeId() == App::PropertyMaterial::getClassTypeId()) {
            material = true;
            matType = static_cast<App::PropertyMaterial*>(prop)->getValue().getType();
            break;
        }
    }

    int index = d->ui.changeMaterial->findData(matType);
    if (index >= 0) {
        d->ui.changeMaterial->setCurrentIndex(index);
    }
    d->ui.changeMaterial->setEnabled(material);
    d->ui.buttonUserDefinedMaterial->setEnabled(material);
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

    d->ui.buttonColorPlot->setEnabled(material);
}

void DlgDisplayPropertiesImp::fillupMaterials()
{
    d->ui.changeMaterial->addItem(tr("Default"), App::Material::DEFAULT);
    d->ui.changeMaterial->addItem(tr("Aluminium"), App::Material::ALUMINIUM);
    d->ui.changeMaterial->addItem(tr("Brass"), App::Material::BRASS);
    d->ui.changeMaterial->addItem(tr("Bronze"), App::Material::BRONZE);
    d->ui.changeMaterial->addItem(tr("Copper"), App::Material::COPPER);
    d->ui.changeMaterial->addItem(tr("Chrome"), App::Material::CHROME);
    d->ui.changeMaterial->addItem(tr("Emerald"), App::Material::EMERALD);
    d->ui.changeMaterial->addItem(tr("Gold"), App::Material::GOLD);
    d->ui.changeMaterial->addItem(tr("Jade"), App::Material::JADE);
    d->ui.changeMaterial->addItem(tr("Metalized"), App::Material::METALIZED);
    d->ui.changeMaterial->addItem(tr("Neon GNC"), App::Material::NEON_GNC);
    d->ui.changeMaterial->addItem(tr("Neon PHC"), App::Material::NEON_PHC);
    d->ui.changeMaterial->addItem(tr("Obsidian"), App::Material::OBSIDIAN);
    d->ui.changeMaterial->addItem(tr("Pewter"), App::Material::PEWTER);
    d->ui.changeMaterial->addItem(tr("Plaster"), App::Material::PLASTER);
    d->ui.changeMaterial->addItem(tr("Plastic"), App::Material::PLASTIC);
    d->ui.changeMaterial->addItem(tr("Ruby"), App::Material::RUBY);
    d->ui.changeMaterial->addItem(tr("Satin"), App::Material::SATIN);
    d->ui.changeMaterial->addItem(tr("Shiny plastic"), App::Material::SHINY_PLASTIC);
    d->ui.changeMaterial->addItem(tr("Silver"), App::Material::SILVER);
    d->ui.changeMaterial->addItem(tr("Steel"), App::Material::STEEL);
    d->ui.changeMaterial->addItem(tr("Stone"), App::Material::STONE);
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
            bool blocked = d->ui.buttonColor->blockSignals(true);
            d->ui.buttonColor->setColor(shape);
            d->ui.buttonColor->blockSignals(blocked);
            shapeColor = true;
            break;
        }
    }

    d->ui.buttonColor->setEnabled(shapeColor);
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
            bool blocked = d->ui.buttonLineColor->blockSignals(true);
            d->ui.buttonLineColor->setColor(shape);
            d->ui.buttonLineColor->blockSignals(blocked);
            shapeColor = true;
            break;
        }
    }

    d->ui.buttonLineColor->setEnabled(shapeColor);
}

void DlgDisplayPropertiesImp::setPointSize(const std::vector<Gui::ViewProvider*>& views)
{
    bool pointSize = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("PointSize");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            bool blocked = d->ui.spinPointSize->blockSignals(true);
            d->ui.spinPointSize->setValue((int)static_cast<App::PropertyFloat*>(prop)->getValue());
            d->ui.spinPointSize->blockSignals(blocked);
            pointSize = true;
            break;
        }
    }

    d->ui.spinPointSize->setEnabled(pointSize);
}

void DlgDisplayPropertiesImp::setLineWidth(const std::vector<Gui::ViewProvider*>& views)
{
    bool lineWidth = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("LineWidth");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            bool blocked = d->ui.spinLineWidth->blockSignals(true);
            d->ui.spinLineWidth->setValue((int)static_cast<App::PropertyFloat*>(prop)->getValue());
            d->ui.spinLineWidth->blockSignals(blocked);
            lineWidth = true;
            break;
        }
    }

    d->ui.spinLineWidth->setEnabled(lineWidth);
}

void DlgDisplayPropertiesImp::setTransparency(const std::vector<Gui::ViewProvider*>& views)
{
    bool transparency = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("Transparency");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            bool blocked = d->ui.spinTransparency->blockSignals(true);
            d->ui.spinTransparency->setValue(static_cast<App::PropertyInteger*>(prop)->getValue());
            d->ui.spinTransparency->blockSignals(blocked);

            blocked = d->ui.horizontalSlider->blockSignals(true);
            d->ui.horizontalSlider->setValue(static_cast<App::PropertyInteger*>(prop)->getValue());
            d->ui.horizontalSlider->blockSignals(blocked);
            transparency = true;
            break;
        }
    }

    d->ui.spinTransparency->setEnabled(transparency);
    d->ui.horizontalSlider->setEnabled(transparency);
}

void DlgDisplayPropertiesImp::setLineTransparency(const std::vector<Gui::ViewProvider*>& views)
{
    bool transparency = false;
    for (std::vector<Gui::ViewProvider*>::const_iterator it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("LineTransparency");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            bool blocked = d->ui.spinLineTransparency->blockSignals(true);
            d->ui.spinLineTransparency->setValue(static_cast<App::PropertyInteger*>(prop)->getValue());
            d->ui.spinLineTransparency->blockSignals(blocked);

            blocked = d->ui.sliderLineTransparency->blockSignals(true);
            d->ui.sliderLineTransparency->setValue(static_cast<App::PropertyInteger*>(prop)->getValue());
            d->ui.sliderLineTransparency->blockSignals(blocked);
            transparency = true;
            break;
        }
    }

    d->ui.spinLineTransparency->setEnabled(transparency);
    d->ui.sliderLineTransparency->setEnabled(transparency);
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

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::TaskDisplayProperties */

TaskDisplayProperties::TaskDisplayProperties()
{
    this->setButtonPosition(TaskDisplayProperties::South);
    widget = new DlgDisplayPropertiesImp(false);
    widget->showDefaultButtons(false);
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(),true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDisplayProperties::~TaskDisplayProperties()
{
    // automatically deleted in the sub-class
}

QDialogButtonBox::StandardButtons TaskDisplayProperties::getStandardButtons() const
{
    return QDialogButtonBox::Close;
}

bool TaskDisplayProperties::reject()
{
    widget->reject();
    return (widget->result() == QDialog::Rejected);
}

#include "moc_DlgDisplayPropertiesImp.cpp"

