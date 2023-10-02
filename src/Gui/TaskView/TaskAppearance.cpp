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


#include "PreCompiled.h"

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include "ui_TaskAppearance.h"
#include "TaskAppearance.h"


using namespace Gui::TaskView;
namespace sp = std::placeholders;

/* TRANSLATOR Gui::TaskView::TaskAppearance */

TaskAppearance::TaskAppearance(QWidget *parent)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Appearance"),true, parent)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui = new Ui_TaskAppearance();
    ui->setupUi(proxy);
    setupConnections();

    ui->textLabel1_3->hide();
    ui->changePlot->hide();
    QMetaObject::connectSlotsByName(this);

    this->groupLayout()->addWidget(proxy);
    Gui::Selection().Attach(this);

    //NOLINTBEGIN
    this->connectChangedObject =
    Gui::Application::Instance->signalChangedObject.connect(std::bind
        (&TaskAppearance::slotChangedObject, this, sp::_1, sp::_2));
    //NOLINTEND
}

TaskAppearance::~TaskAppearance()
{
    delete ui;
    this->connectChangedObject.disconnect();
    Gui::Selection().Detach(this);
}

void TaskAppearance::setupConnections()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    connect(ui->changeMode, qOverload<const QString&>(&QComboBox::activated),
            this, &TaskAppearance::onChangeModeActivated);
    connect(ui->changePlot, qOverload<const QString&>(&QComboBox::activated),
            this, &TaskAppearance::onChangePlotActivated);
#else
    connect(ui->changeMode, &QComboBox::textActivated,
            this, &TaskAppearance::onChangeModeActivated);
    connect(ui->changePlot, &QComboBox::textActivated,
            this, &TaskAppearance::onChangePlotActivated);
#endif
    connect(ui->spinTransparency, qOverload<int>(&QSpinBox::valueChanged),
            this, &TaskAppearance::onTransparencyValueChanged);
    connect(ui->spinPointSize, qOverload<int>(&QSpinBox::valueChanged),
            this, &TaskAppearance::onPointSizeValueChanged);
    connect(ui->spinLineWidth, qOverload<int>(&QSpinBox::valueChanged),
            this, &TaskAppearance::onLineWidthValueChanged);
}

void TaskAppearance::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

/// @cond DOXERR
void TaskAppearance::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                              Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller); 
    if (Reason.Type == SelectionChanges::AddSelection ||
        Reason.Type == SelectionChanges::RmvSelection ||
        Reason.Type == SelectionChanges::SetSelection ||
        Reason.Type == SelectionChanges::ClrSelection) {
        std::vector<Gui::ViewProvider*> views = getSelection();
        setDisplayModes(views);
        setPointSize(views);
        setLineWidth(views);
        setTransparency(views);
    }
}
/// @endcond

void TaskAppearance::slotChangedObject(const Gui::ViewProvider& obj,
                                       const App::Property& prop)
{
    // This method gets called if a property of any view provider is changed.
    // We pick out all the properties for which we need to update this dialog.
    const std::vector<Gui::ViewProvider*> Provider = getSelection();
    auto vp = std::find_if
        (Provider.begin(), Provider.end(), [&obj](Gui::ViewProvider* v) { return v == &obj; });

    if (vp != Provider.end()) {
        std::string prop_name = obj.getPropertyName(&prop);
        if (prop.getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            long value = static_cast<const App::PropertyInteger&>(prop).getValue();
            if (prop_name == "Transparency") {
                bool blocked = ui->spinTransparency->blockSignals(true);
                ui->spinTransparency->setValue(value);
                ui->spinTransparency->blockSignals(blocked);
                blocked = ui->horizontalSlider->blockSignals(true);
                ui->horizontalSlider->setValue(value);
                ui->horizontalSlider->blockSignals(blocked);
            }
        }
        else if (prop.getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            float value = static_cast<const App::PropertyFloat&>(prop).getValue();
            if (prop_name == "PointSize") {
                bool blocked = ui->spinPointSize->blockSignals(true);
                ui->spinPointSize->setValue((int)value);
                ui->spinPointSize->blockSignals(blocked);
            }
            else if (prop_name == "LineWidth") {
                bool blocked = ui->spinLineWidth->blockSignals(true);
                ui->spinLineWidth->setValue((int)value);
                ui->spinLineWidth->blockSignals(blocked);
            }
        }
    }
}

/**
 * Sets the 'Display' property of all selected view providers.
 */
void TaskAppearance::onChangeModeActivated(const QString& s)
{
    Gui::WaitCursor wc;
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (const auto & It : Provider) {
        App::Property* prop = It->getPropertyByName("DisplayMode");
        if (prop && prop->getTypeId() == App::PropertyEnumeration::getClassTypeId()) {
            auto Display = static_cast<App::PropertyEnumeration*>(prop);
            Display->setValue((const char*)s.toLatin1());
        }
    }
}

void TaskAppearance::onChangePlotActivated(const QString&s)
{
    Base::Console().Log("Plot = %s\n",(const char*)s.toLatin1());
}

/**
 * Sets the 'Transparency' property of all selected view providers.
 */
void TaskAppearance::onTransparencyValueChanged(int transparency)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (const auto & It : Provider) {
        App::Property* prop = It->getPropertyByName("Transparency");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            auto Transparency = static_cast<App::PropertyInteger*>(prop);
            Transparency->setValue(transparency);
        }
    }
}

/**
 * Sets the 'PointSize' property of all selected view providers.
 */
void TaskAppearance::onPointSizeValueChanged(int pointsize)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (const auto & It : Provider) {
        App::Property* prop = It->getPropertyByName("PointSize");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            auto PointSize = static_cast<App::PropertyFloat*>(prop);
            PointSize->setValue(static_cast<float>(pointsize));
        }
    }
}

/**
 * Sets the 'LineWidth' property of all selected view providers.
 */
void TaskAppearance::onLineWidthValueChanged(int linewidth)
{
    std::vector<Gui::ViewProvider*> Provider = getSelection();
    for (const auto & It : Provider) {
        App::Property* prop = It->getPropertyByName("LineWidth");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            auto LineWidth = static_cast<App::PropertyFloat*>(prop);
            LineWidth->setValue(static_cast<float>(linewidth));
        }
    }
}

void TaskAppearance::setDisplayModes(const std::vector<Gui::ViewProvider*>& views)
{
    QStringList commonModes, modes;
    for (auto it = views.begin(); it != views.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName("DisplayMode");
        if (prop && prop->getTypeId() == App::PropertyEnumeration::getClassTypeId()) {
            auto display = static_cast<App::PropertyEnumeration*>(prop);
            if (!display->hasEnums())
                return;
            std::vector<std::string> value = display->getEnumVector();
            if (it == views.begin()) {
                for (const auto & jt : value)
                    commonModes << QLatin1String(jt.c_str());
            }
            else {
                for (const auto & jt : value) {
                    if (commonModes.contains(QLatin1String(jt.c_str())))
                        modes << QLatin1String(jt.c_str());
                }

                commonModes = modes;
                modes.clear();
            }
        }
    }

    ui->changeMode->clear();
    ui->changeMode->addItems(commonModes);
    ui->changeMode->setDisabled(commonModes.isEmpty());

    // find the display mode to activate
    for (const auto view : views) {
        App::Property* prop = view->getPropertyByName("DisplayMode");
        if (prop && prop->getTypeId() == App::PropertyEnumeration::getClassTypeId()) {
            auto display = static_cast<App::PropertyEnumeration*>(prop);
            QString activeMode = QString::fromLatin1(display->getValueAsString());
            int index = ui->changeMode->findText(activeMode);
            if (index != -1) {
                ui->changeMode->setCurrentIndex(index);
                break;
            }
        }
    }
}

void TaskAppearance::setPointSize(const std::vector<Gui::ViewProvider*>& views)
{
    bool pointSize = false;
    for (const auto & view : views) {
        App::Property* prop = view->getPropertyByName("PointSize");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            bool blocked = ui->spinPointSize->blockSignals(true);
            ui->spinPointSize->setValue((int)static_cast<App::PropertyFloat*>(prop)->getValue());
            ui->spinPointSize->blockSignals(blocked);
            pointSize = true;
            break;
        }
    }

    ui->spinPointSize->setEnabled(pointSize);
}

void TaskAppearance::setLineWidth(const std::vector<Gui::ViewProvider*>& views)
{
    bool lineWidth = false;
    for (const auto & view : views) {
        App::Property* prop = view->getPropertyByName("LineWidth");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
            bool blocked = ui->spinLineWidth->blockSignals(true);
            ui->spinLineWidth->setValue((int)static_cast<App::PropertyFloat*>(prop)->getValue());
            ui->spinLineWidth->blockSignals(blocked);
            lineWidth = true;
            break;
        }
    }

    ui->spinLineWidth->setEnabled(lineWidth);
}

void TaskAppearance::setTransparency(const std::vector<Gui::ViewProvider*>& views)
{
    bool transparency = false;
    for (const auto & view : views) {
        App::Property* prop = view->getPropertyByName("Transparency");
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyInteger::getClassTypeId())) {
            bool blocked = ui->spinTransparency->blockSignals(true);
            ui->spinTransparency->setValue(static_cast<App::PropertyInteger*>(prop)->getValue());
            ui->spinTransparency->blockSignals(blocked);
            transparency = true;
            break;
        }
    }

    ui->spinTransparency->setEnabled(transparency);
    ui->horizontalSlider->setEnabled(transparency);
}

std::vector<Gui::ViewProvider*> TaskAppearance::getSelection() const
{
    std::vector<Gui::ViewProvider*> views;

    // get the complete selection
    std::vector<SelectionSingleton::SelObj> sel = Selection().getCompleteSelection();
    for (const auto & it : sel) {
        Gui::ViewProvider* view = Application::Instance->getDocument(it.pDoc)->getViewProvider(it.pObject);
        if (view) views.push_back(view);
    }

    return views;
}


#include "moc_TaskAppearance.cpp"
