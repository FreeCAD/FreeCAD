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
#include <QSignalBlocker>
#include <algorithm>
#include <boost/signals2.hpp>
#endif

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Dialogs/DlgMaterialPropertiesImp.h>
#include <Gui/DockWindowManager.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/WaitCursor.h>

#include <Mod/Material/App/ModelUuids.h>

#include "DlgDisplayPropertiesImp.h"
#include "ui_DlgDisplayProperties.h"


using namespace MatGui;
using namespace std;
namespace sp = std::placeholders;


/* TRANSLATOR Gui::Dialog::DlgDisplayPropertiesImp */

class DlgDisplayPropertiesImp::Private
{
    using DlgDisplayPropertiesImp_Connection = boost::signals2::connection;

public:
    Ui::DlgDisplayProperties ui;
    DlgDisplayPropertiesImp_Connection connectChangedObject;

    static void setElementColor(const std::vector<Gui::ViewProvider*>& views,
                                const char* property,
                                Gui::ColorButton* buttonColor)
    {
        bool hasElementColor = false;
        for (const auto& view : views) {
            if (auto* prop = dynamic_cast<App::PropertyColor*>(view->getPropertyByName(property))) {
                Base::Color color = prop->getValue();
                QSignalBlocker block(buttonColor);
                buttonColor->setColor(color.asValue<QColor>());
                hasElementColor = true;
                break;
            }
        }

        buttonColor->setEnabled(hasElementColor);
    }

    static void setElementAppearance(const std::vector<Gui::ViewProvider*>& views,
                                     const char* property,
                                     Gui::ColorButton* buttonColor)
    {
        bool hasElementColor = false;
        for (const auto& view : views) {
            if (auto* prop =
                    dynamic_cast<App::PropertyMaterial*>(view->getPropertyByName(property))) {
                Base::Color color = prop->getDiffuseColor();
                QSignalBlocker block(buttonColor);
                buttonColor->setColor(color.asValue<QColor>());
                hasElementColor = true;
                break;
            }
        }

        buttonColor->setEnabled(hasElementColor);
    }

    static void setDrawStyle(const std::vector<Gui::ViewProvider*>& views,
                             const char* property,
                             QSpinBox* spinbox)
    {
        bool hasDrawStyle = false;
        for (const auto& view : views) {
            if (auto* prop = dynamic_cast<App::PropertyFloat*>(view->getPropertyByName(property))) {
                QSignalBlocker block(spinbox);
                spinbox->setValue(int(prop->getValue()));
                hasDrawStyle = true;
                break;
            }
        }

        spinbox->setEnabled(hasDrawStyle);
    }

    static void setTransparency(const std::vector<Gui::ViewProvider*>& views,
                                const char* property,
                                QSpinBox* spinbox,
                                QSlider* slider)
    {
        bool hasTransparency = false;
        for (const auto& view : views) {
            if (auto* prop =
                    dynamic_cast<App::PropertyInteger*>(view->getPropertyByName(property))) {
                QSignalBlocker blockSpinBox(spinbox);
                spinbox->setValue(prop->getValue());

                QSignalBlocker blockSlider(slider);
                slider->setValue(prop->getValue());
                hasTransparency = true;
                break;
            }
        }

        spinbox->setEnabled(hasTransparency);
        slider->setEnabled(hasTransparency);
    }
};

DlgDisplayPropertiesImp::DlgDisplayPropertiesImp(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , d(new Private)
{
    d->ui.setupUi(this);
    setupConnections();

    d->ui.textLabel1_3->hide();
    d->ui.changePlot->hide();
    d->ui.buttonLineColor->setModal(false);
    d->ui.buttonPointColor->setModal(false);

    // Create a filter to only include current format materials
    // that contain the basic render model.
    setupFilters();

    {
        QSignalBlocker block(d->ui.widgetMaterial);
        setPropertiesFromSelection();
    }

    Gui::Selection().Attach(this);

    // NOLINTBEGIN
    d->connectChangedObject = Gui::Application::Instance->signalChangedObject.connect(
        std::bind(&DlgDisplayPropertiesImp::slotChangedObject, this, sp::_1, sp::_2));
    // NOLINTEND
}

DlgDisplayPropertiesImp::~DlgDisplayPropertiesImp()
{
    // no need to delete child widgets, Qt does it all for us
    d->connectChangedObject.disconnect();
    Gui::Selection().Detach(this);
}

void DlgDisplayPropertiesImp::setupFilters()
{
    // Create a filter to only include current format materials
    // that contain the basic render model.
    auto filterList = std::make_shared<std::list<std::shared_ptr<Materials::MaterialFilter>>>();

    auto filter = std::make_shared<Materials::MaterialFilter>();
    filter->setName(tr("Basic appearance"));
    filter->addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    filterList->push_back(filter);

    filter = std::make_shared<Materials::MaterialFilter>();
    filter->setName(tr("Texture appearance"));
    filter->addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Rendering_Texture);
    filterList->push_back(filter);

    filter = std::make_shared<Materials::MaterialFilter>();
    filter->setName(tr("All materials"));
    filterList->push_back(filter);

    d->ui.widgetMaterial->setIncludeEmptyFolders(false);
    d->ui.widgetMaterial->setIncludeLegacy(false);

    d->ui.widgetMaterial->setFilter(filterList);
}

void DlgDisplayPropertiesImp::setupConnections()
{
    connect(d->ui.changeMode,
            &QComboBox::textActivated,
            this,
            &DlgDisplayPropertiesImp::onChangeModeActivated);
    connect(d->ui.changePlot,
            &QComboBox::textActivated,
            this,
            &DlgDisplayPropertiesImp::onChangePlotActivated);
    connect(d->ui.spinTransparency,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &DlgDisplayPropertiesImp::onSpinTransparencyValueChanged);
    connect(d->ui.spinPointSize,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &DlgDisplayPropertiesImp::onSpinPointSizeValueChanged);
    connect(d->ui.buttonLineColor,
            &Gui::ColorButton::changed,
            this,
            &DlgDisplayPropertiesImp::onButtonLineColorChanged);
    connect(d->ui.buttonPointColor,
            &Gui::ColorButton::changed,
            this,
            &DlgDisplayPropertiesImp::onButtonPointColorChanged);
    connect(d->ui.spinLineWidth,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &DlgDisplayPropertiesImp::onSpinLineWidthValueChanged);
    connect(d->ui.spinLineTransparency,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &DlgDisplayPropertiesImp::onSpinLineTransparencyValueChanged);
    connect(d->ui.buttonCustomAppearance,
            &Gui::ColorButton::clicked,
            this,
            &DlgDisplayPropertiesImp::onButtonCustomAppearanceClicked);
    connect(d->ui.buttonColorPlot,
            &Gui::ColorButton::clicked,
            this,
            &DlgDisplayPropertiesImp::onButtonColorPlotClicked);
    connect(d->ui.widgetMaterial,
            &MaterialTreeWidget::materialSelected,
            this,
            &DlgDisplayPropertiesImp::onMaterialSelected);
}

void DlgDisplayPropertiesImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
    }
    QDialog::changeEvent(e);
}

void DlgDisplayPropertiesImp::setPropertiesFromSelection()
{
    std::vector<Gui::ViewProvider*> views = getSelection();
    setDisplayModes(views);
    setColorPlot(views);
    setShapeAppearance(views);
    setLineColor(views);
    setPointColor(views);
    setPointSize(views);
    setLineWidth(views);
    setTransparency(views);
    setLineTransparency(views);
}

/// @cond DOXERR
void DlgDisplayPropertiesImp::OnChange(Gui::SelectionSingleton::SubjectType& rCaller,
                                       Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
    if (Reason.Type == Gui::SelectionChanges::AddSelection
        || Reason.Type == Gui::SelectionChanges::RmvSelection
        || Reason.Type == Gui::SelectionChanges::SetSelection
        || Reason.Type == Gui::SelectionChanges::ClrSelection) {
        setPropertiesFromSelection();
    }
}
/// @endcond

void DlgDisplayPropertiesImp::slotChangedObject(const Gui::ViewProvider& obj,
                                                const App::Property& prop)
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
        if (prop.is<App::PropertyColor>()) {
            Base::Color value = static_cast<const App::PropertyColor&>(prop).getValue();
            if (prop_name == "LineColor") {
                bool blocked = d->ui.buttonLineColor->blockSignals(true);
                d->ui.buttonLineColor->setColor(value.asValue<QColor>());
                d->ui.buttonLineColor->blockSignals(blocked);
            }
            else if (prop_name == "PointColor") {
                bool blocked = d->ui.buttonPointColor->blockSignals(true);
                d->ui.buttonPointColor->setColor(value.asValue<QColor>());
                d->ui.buttonPointColor->blockSignals(blocked);
            }
        }
        else if (prop.isDerivedFrom<App::PropertyMaterialList>()) {
            if (prop_name == "ShapeAppearance") {
                auto& values = static_cast<const App::PropertyMaterialList&>(prop).getValues();
                auto& material = values[0];
                d->ui.widgetMaterial->setMaterial(QString::fromStdString(material.uuid));
            }
        }
        else if (prop.isDerivedFrom<App::PropertyInteger>()) {
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
        else if (prop.isDerivedFrom<App::PropertyFloat>()) {
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

void DlgDisplayPropertiesImp::reject()
{
    QDialog::reject();
}

/**
 * Opens a dialog that allows one to modify the 'ShapeMaterial' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::onButtonCustomAppearanceClicked()
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    Gui::Dialog::DlgMaterialPropertiesImp dlg(this);
    if (!Provider.empty()) {
        if (auto vp = dynamic_cast<Gui::ViewProviderGeometryObject*>(Provider.front())) {
            App::Material mat = vp->ShapeAppearance[0];
            dlg.setCustomMaterial(mat);
            dlg.setDefaultMaterial(mat);
        }
    }
    dlg.exec();
    App::Material mat = dlg.getCustomMaterial();
    for (auto vp : Provider) {
        if (auto vpg = dynamic_cast<Gui::ViewProviderGeometryObject*>(vp)) {
            vpg->ShapeAppearance.setValue(mat);
        }
    }
}

/**
 * Opens a dialog that allows one to modify the 'ShapeMaterial' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::onButtonColorPlotClicked()
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    static QPointer<Gui::Dialog::DlgMaterialPropertiesImp> dlg = nullptr;
    if (!dlg) {
        dlg = new Gui::Dialog::DlgMaterialPropertiesImp(this);
    }
    dlg->setModal(false);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    if (!Provider.empty()) {
        App::Property* prop = Provider.front()->getPropertyByName("TextureMaterial");
        if (auto matProp = dynamic_cast<App::PropertyMaterialList*>(prop)) {
            App::Material mat = (*matProp)[0];
            dlg->setCustomMaterial(mat);
            dlg->setDefaultMaterial(mat);
        }
    }
    dlg->show();
}

/**
 * Sets the 'Display' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::onChangeModeActivated(const QString& s)
{
    Gui::WaitCursor wc;
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (auto it : Provider) {
        if (auto* prop =
                dynamic_cast<App::PropertyEnumeration*>(it->getPropertyByName("DisplayMode"))) {
            prop->setValue(static_cast<const char*>(s.toLatin1()));
        }
    }
}

void DlgDisplayPropertiesImp::onChangePlotActivated(const QString& s)
{
    Base::Console().log("Plot = %s\n", (const char*)s.toLatin1());
}

/**
 * Sets the 'Transparency' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::onSpinTransparencyValueChanged(int transparency)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (auto it : Provider) {
        if (auto* prop =
                dynamic_cast<App::PropertyInteger*>(it->getPropertyByName("Transparency"))) {
            prop->setValue(transparency);
        }
    }
}

/**
 * Sets the 'PointSize' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::onSpinPointSizeValueChanged(int pointsize)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (auto it : Provider) {
        if (auto* prop = dynamic_cast<App::PropertyFloat*>(it->getPropertyByName("PointSize"))) {
            prop->setValue(static_cast<double>(pointsize));
        }
    }
}

/**
 * Sets the 'LineWidth' property of all selected view providers.
 */
void DlgDisplayPropertiesImp::onSpinLineWidthValueChanged(int linewidth)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (auto it : Provider) {
        if (auto* prop = dynamic_cast<App::PropertyFloat*>(it->getPropertyByName("LineWidth"))) {
            prop->setValue(static_cast<double>(linewidth));
        }
    }
}

void DlgDisplayPropertiesImp::onButtonLineColorChanged()
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    QColor s = d->ui.buttonLineColor->color();
    Base::Color c {};
    c.setValue<QColor>(s);
    for (auto it : Provider) {
        if (auto* prop = dynamic_cast<App::PropertyColor*>(it->getPropertyByName("LineColor"))) {
            prop->setValue(c);
        }
    }
}

void DlgDisplayPropertiesImp::onButtonPointColorChanged()
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    QColor s = d->ui.buttonPointColor->color();
    Base::Color c {};
    c.setValue<QColor>(s);
    for (auto it : Provider) {
        if (auto* prop = dynamic_cast<App::PropertyColor*>(it->getPropertyByName("PointColor"))) {
            prop->setValue(c);
        }
    }
}

void DlgDisplayPropertiesImp::onSpinLineTransparencyValueChanged(int transparency)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (auto it : Provider) {
        if (auto* prop =
                dynamic_cast<App::PropertyInteger*>(it->getPropertyByName("LineTransparency"))) {
            prop->setValue(transparency);
        }
    }
}

void DlgDisplayPropertiesImp::setDisplayModes(const std::vector<Gui::ViewProvider*>& views)
{
    QStringList commonModes;
    QStringList modes;
    for (auto it = views.begin(); it != views.end(); ++it) {
        if (auto* prop =
                dynamic_cast<App::PropertyEnumeration*>((*it)->getPropertyByName("DisplayMode"))) {
            if (!prop->hasEnums()) {
                return;
            }
            std::vector<std::string> value = prop->getEnumVector();
            if (it == views.begin()) {
                for (const auto& jt : value) {
                    commonModes << QLatin1String(jt.c_str());
                }
            }
            else {
                for (const auto& jt : value) {
                    if (commonModes.contains(QLatin1String(jt.c_str()))) {
                        modes << QLatin1String(jt.c_str());
                    }
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
    for (const auto& view : views) {
        if (auto* prop =
                dynamic_cast<App::PropertyEnumeration*>(view->getPropertyByName("DisplayMode"))) {
            QString activeMode = QString::fromLatin1(prop->getValueAsString());
            int index = d->ui.changeMode->findText(activeMode);
            if (index != -1) {
                d->ui.changeMode->setCurrentIndex(index);
                break;
            }
        }
    }
}

void DlgDisplayPropertiesImp::setColorPlot(const std::vector<Gui::ViewProvider*>& views)
{
    bool material = false;
    for (auto view : views) {
        auto* prop =
            dynamic_cast<App::PropertyMaterial*>(view->getPropertyByName("TextureMaterial"));
        if (prop) {
            material = true;
            break;
        }
    }

    d->ui.buttonColorPlot->setEnabled(material);
}

void DlgDisplayPropertiesImp::setShapeAppearance(const std::vector<Gui::ViewProvider*>& views)
{
    bool material = false;
    App::Material mat = App::Material(App::Material::DEFAULT);
    for (auto view : views) {
        if (auto* prop =
                dynamic_cast<App::PropertyMaterialList*>(view->getPropertyByName("ShapeAppearance"))) {
            material = true;
            mat = prop->getValues()[0];
            d->ui.widgetMaterial->setMaterial(QString::fromStdString(mat.uuid));
            break;
        }
    }
    d->ui.buttonCustomAppearance->setEnabled(material);
}

void DlgDisplayPropertiesImp::setLineColor(const std::vector<Gui::ViewProvider*>& views)
{
    Private::setElementColor(views, "LineColor", d->ui.buttonLineColor);
}

void DlgDisplayPropertiesImp::setPointColor(const std::vector<Gui::ViewProvider*>& views)
{
    Private::setElementColor(views, "PointColor", d->ui.buttonPointColor);
}

void DlgDisplayPropertiesImp::setPointSize(const std::vector<Gui::ViewProvider*>& views)
{
    Private::setDrawStyle(views, "PointSize", d->ui.spinPointSize);
}

void DlgDisplayPropertiesImp::setLineWidth(const std::vector<Gui::ViewProvider*>& views)
{
    Private::setDrawStyle(views, "LineWidth", d->ui.spinLineWidth);
}

void DlgDisplayPropertiesImp::setTransparency(const std::vector<Gui::ViewProvider*>& views)
{
    Private::setTransparency(views, "Transparency", d->ui.spinTransparency, d->ui.horizontalSlider);
}

void DlgDisplayPropertiesImp::setLineTransparency(const std::vector<Gui::ViewProvider*>& views)
{
    Private::setTransparency(views,
                             "LineTransparency",
                             d->ui.spinLineTransparency,
                             d->ui.sliderLineTransparency);
}

std::vector<Gui::ViewProvider*> DlgDisplayPropertiesImp::getSelection() const
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

void DlgDisplayPropertiesImp::onMaterialSelected(
    const std::shared_ptr<Materials::Material>& material)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (auto it : Provider) {
        if (auto* prop = dynamic_cast<App::PropertyMaterialList*>(
                it->getPropertyByName("ShapeAppearance"))) {
            prop->setValue(material->getMaterialAppearance());
        }
    }
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::TaskDisplayProperties */

TaskDisplayProperties::TaskDisplayProperties()
{
    this->setButtonPosition(TaskDisplayProperties::North);
    widget = new DlgDisplayPropertiesImp();
    addTaskBox(widget);
}

TaskDisplayProperties::~TaskDisplayProperties() = default;

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
