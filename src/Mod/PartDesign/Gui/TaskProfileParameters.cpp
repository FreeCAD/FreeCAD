// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Jesse Trutna <jesse@trutna.com>                    *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                      *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include <algorithm>

#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolButton>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <Base/Tools.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>

#include "TaskProfileParameters.h"
#include "ViewProvider.h"

using namespace PartDesignGui;

namespace
{
bool startsWith(const std::string& s, const char* prefix)
{
    return s.rfind(prefix, 0) == 0;
}

/// Closed sketch regions ("InternalFaceN") and faces of other shapes can serve as profiles.
bool isProfileElement(App::DocumentObject* obj, const std::string& sub)
{
    if (obj->isDerivedFrom<Part::Part2DObject>()) {
        return startsWith(sub, "InternalFace");
    }
    return startsWith(sub, "Face");
}

class ProfileGate: public Gui::SelectionGate
{
public:
    explicit ProfileGate(App::DocumentObject* feature)
        : feature(feature)
    {}

    bool allow(App::Document*, App::DocumentObject* obj, const char* sub) override
    {
        return obj && obj != feature && sub && isProfileElement(obj, sub);
    }

private:
    App::DocumentObject* feature;
};
}  // namespace

TaskProfileParameters::TaskProfileParameters(ViewProvider* vp, QWidget* parent)
    : TaskBox(Gui::BitmapFactory().pixmap("Sketcher_Sketch"), tr("Profiles"), true, parent)
    , SelectionObserver(true)
    , vp(vp)
{
    auto proxy = new QWidget(this);
    auto layout = new QHBoxLayout(proxy);
    layout->setContentsMargins(0, 0, 0, 0);

    summary = new QLabel(proxy);
    summary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    layout->addWidget(summary);

    selectButton = new QToolButton(proxy);
    selectButton->setText(tr("Select"));
    selectButton->setToolTip(
        tr("Click sketch regions or faces in the 3D view to add or remove them from the profile")
    );
    selectButton->setCheckable(true);
    layout->addWidget(selectButton);

    allButton = new QToolButton(proxy);
    allButton->setText(tr("Whole Sketch"));
    allButton->setToolTip(tr("Use every closed region of the sketch"));
    layout->addWidget(allButton);

    groupLayout()->addWidget(proxy);

    connect(selectButton, &QToolButton::toggled, this, &TaskProfileParameters::setSelecting);
    connect(allButton, &QToolButton::clicked, this, &TaskProfileParameters::useWholeProfile);
    updateSummary();
}

TaskProfileParameters::~TaskProfileParameters()
{
    setSelecting(false);
}

PartDesign::ProfileBased* TaskProfileParameters::getFeature() const
{
    return vp ? vp->getObject<PartDesign::ProfileBased>() : nullptr;
}

void TaskProfileParameters::updateSummary()
{
    auto feature = getFeature();
    App::DocumentObject* profile = feature ? feature->Profile.getValue() : nullptr;
    if (!profile) {
        summary->setText(tr("No profile"));
        allButton->setEnabled(false);
        return;
    }

    const auto& subs = feature->Profile.getSubValues();
    const auto regions = std::count_if(subs.begin(), subs.end(), [](const std::string& s) {
        return startsWith(s, "InternalFace");
    });
    const auto faces = std::count_if(subs.begin(), subs.end(), [](const std::string& s) {
        return startsWith(s, "Face");
    });

    QString label = QString::fromUtf8(profile->Label.getValue());
    if (regions > 0) {
        label += QStringLiteral(": ")
            + (regions == 1 ? tr("1 region") : tr("%1 regions").arg(regions));
    }
    else if (faces > 0) {
        label += QStringLiteral(": ") + (faces == 1 ? tr("1 face") : tr("%1 faces").arg(faces));
    }
    else if (profile->isDerivedFrom<Part::Part2DObject>()) {
        label += QStringLiteral(": ") + tr("whole sketch");
    }
    summary->setText(label);
    allButton->setEnabled(profile->isDerivedFrom<Part::Part2DObject>() && !subs.empty());
}

void TaskProfileParameters::setSelecting(bool on)
{
    if (on == selecting) {
        return;
    }
    selecting = on;
    if (selectButton->isChecked() != on) {
        QSignalBlocker block(selectButton);
        selectButton->setChecked(on);
    }

    auto feature = getFeature();
    Gui::Document* guiDoc = vp ? vp->getDocument() : nullptr;
    if (!feature || !guiDoc) {
        return;
    }

    if (on) {
        // Show the profile and the solid beneath the feature, so regions and faces can be
        // clicked; hide the feature itself so it does not cover them.
        shownProfile = feature->Profile.getValue();
        shownBase = feature->getBaseObject(/*silent =*/true);
        featureWasVisible = vp->isShow();
        vp->hide();
        if (shownBase) {
            guiDoc->setShow(shownBase->getNameInDocument());
        }
        if (shownProfile) {
            // Closed regions are only pickable when the sketch builds them.
            auto internals = freecad_cast<App::PropertyBool*>(
                shownProfile->getPropertyByName("MakeInternals")
            );
            if (internals && !internals->getValue()) {
                internals->setValue(true);
                shownProfile->recomputeFeature();
            }
            guiDoc->setShow(shownProfile->getNameInDocument());
        }

        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new ProfileGate(feature));
    }
    else {
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().clearSelection();
        if (shownProfile) {
            guiDoc->setHide(shownProfile->getNameInDocument());
        }
        if (shownBase) {
            guiDoc->setHide(shownBase->getNameInDocument());
        }
        if (featureWasVisible) {
            vp->show();
        }
        shownProfile = nullptr;
        shownBase = nullptr;
    }
}

void TaskProfileParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (!selecting || msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }

    auto feature = getFeature();
    App::DocumentObject* obj = feature ? feature->getDocument()->getObject(msg.pObjectName) : nullptr;
    const std::string sub = msg.pSubName ? msg.pSubName : "";
    if (!obj || !isProfileElement(obj, sub)) {
        return;
    }
    pick(obj, sub);

    // Clear after the selection change has been handled, so the same element can be
    // clicked again to remove it.
    QTimer::singleShot(0, this, [] { Gui::Selection().clearSelection(); });
}

void TaskProfileParameters::pick(App::DocumentObject* obj, const std::string& sub)
{
    auto feature = getFeature();
    std::vector<std::string> subs;
    if (feature->Profile.getValue() == obj) {
        subs = feature->Profile.getSubValues();
    }

    // Clicking an element toggles it in or out of the profile.
    auto it = std::find(subs.begin(), subs.end(), sub);
    if (it != subs.end()) {
        subs.erase(it);
    }
    else {
        subs.push_back(sub);
    }

    // Removing the last region falls back to the whole sketch; a solid needs a face.
    if (subs.empty() && !obj->isDerivedFrom<Part::Part2DObject>()) {
        return;
    }

    // Sketch sub-elements are ignored unless the feature may use several faces.
    feature->AllowMultiFace.setValue(true);
    feature->Profile.setValue(obj, subs);
    feature->recomputeFeature();
    updateSummary();
}

void TaskProfileParameters::useWholeProfile()
{
    auto feature = getFeature();
    if (!feature || !feature->Profile.getValue()) {
        return;
    }
    feature->Profile.setValue(feature->Profile.getValue(), std::vector<std::string>());
    feature->recomputeFeature();
    updateSummary();
}

#include "moc_TaskProfileParameters.cpp"
