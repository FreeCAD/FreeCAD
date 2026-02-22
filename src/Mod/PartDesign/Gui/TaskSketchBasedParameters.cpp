// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTextStream>


#include <App/Document.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskSketchBasedParameters */

TaskSketchBasedParameters::TaskSketchBasedParameters(
    PartDesignGui::ViewProvider* vp,
    QWidget* parent,
    const std::string& pixmapname,
    const QString& parname
)
    : TaskFeatureParameters(vp, parent, pixmapname, parname)
{
    // disable selection
    this->blockSelection(true);
}

const QString TaskSketchBasedParameters::onAddSelection(
    const Gui::SelectionChanges& msg,
    App::PropertyLinkSub& prop
)
{
    // Note: The validity checking has already been done in ReferenceSelection.cpp
    auto sketchBased = getObject<PartDesign::ProfileBased>();
    App::DocumentObject* selObj = sketchBased->getDocument()->getObject(msg.pObjectName);
    if (selObj == sketchBased) {
        return QString();
    }

    std::string subname = msg.pSubName;
    QString refStr;

    if (PartDesign::Feature::isDatum(selObj)) {
        // Check if it's a plane within a LCS
        auto datum = freecad_cast<App::DatumElement*>(selObj);
        if (datum && datum->getLCS()) {
            selObj = datum->getLCS();
            subname = datum->getNameInDocument();

            refStr = QString::fromUtf8(selObj->getNameInDocument()) + QStringLiteral(":")
                + QString::fromUtf8(subname);
        }
        else {
            // Remove subname for planes and datum features
            subname = "";
            refStr = QString::fromUtf8(selObj->getNameInDocument());
        }
    }
    else if (subname.size() > 4) {
        int faceId = std::atoi(&subname[4]);
        refStr = QString::fromUtf8(selObj->getNameInDocument()) + QStringLiteral(":")
            + QObject::tr("Face") + QString::number(faceId);
    }

    std::vector<std::string> upToFaces(1, subname);
    prop.setValue(selObj, upToFaces);
    recomputeFeature();

    return refStr;
}

void TaskSketchBasedParameters::startReferenceSelection(App::DocumentObject*, App::DocumentObject* base)
{
    const auto* bodyViewProvider = getViewObject<ViewProvider>()->getBodyViewProvider();

    previouslyVisibleViewProvider = bodyViewProvider->getShownViewProvider();

    if (!base) {
        return;
    }

    if (Document* doc = getGuiDocument()) {
        if (previouslyVisibleViewProvider) {
            previouslyVisibleViewProvider->hide();
        }

        doc->setShow(base->getNameInDocument());
    }
}

void TaskSketchBasedParameters::finishReferenceSelection(App::DocumentObject*, App::DocumentObject* base)
{
    if (!previouslyVisibleViewProvider) {
        return;
    }

    if (Document* doc = getGuiDocument()) {
        if (base) {
            doc->setHide(base->getNameInDocument());
        }

        previouslyVisibleViewProvider->show();
        previouslyVisibleViewProvider = nullptr;
    }
}

void TaskSketchBasedParameters::onSelectReference(AllowSelectionFlags allow)
{
    // Note: Even if there is no solid, App::Plane and Part::Datum can still be selected
    if (auto sketchBased = getObject<PartDesign::ProfileBased>()) {
        // The solid this feature will be fused to
        App::DocumentObject* prevSolid = sketchBased->getBaseObject(/* silent =*/true);

        if (AllowSelectionFlags::Int(allow) != int(AllowSelection::NONE)) {
            startReferenceSelection(sketchBased, prevSolid);
            this->blockSelection(false);
            Gui::Selection().clearSelection();
            Gui::Selection().addSelectionGate(new ReferenceSelection(prevSolid, allow));
        }
        else {
            Gui::Selection().rmvSelectionGate();
            finishReferenceSelection(sketchBased, prevSolid);
            this->blockSelection(true);
        }
    }
}


void TaskSketchBasedParameters::exitSelectionMode()
{
    onSelectReference(AllowSelection::NONE);
}

QVariant TaskSketchBasedParameters::setUpToFace(const QString& text)
{
    if (text.isEmpty()) {
        return {};
    }

    QStringList parts = text.split(QChar::fromLatin1(':'));
    if (parts.length() < 2) {
        parts.push_back(QString());
    }

    // Check whether this is the name of an App::Plane or Part::Datum feature
    App::Document* doc = getAppDocument();
    if (!doc) {
        return {};
    }

    App::DocumentObject* obj = doc->getObject(parts[0].toLatin1());
    if (!obj) {
        return {};
    }

    if (obj->isDerivedFrom<App::Plane>()) {
        // everything is OK (we assume a Part can only have exactly 3 App::Plane objects
        // located at the base of the feature tree)
        return {};
    }

    if (obj->isDerivedFrom<Part::Datum>()) {
        // it's up to the document to check that the datum plane is in the same body
        return {};
    }

    // We must expect that "parts[1]" is the translation of "Face" followed by an ID.
    QString name;
    QTextStream str(&name);
    str << "^" << tr("Face") << "(\\d+)$";
    QRegularExpression rx(name);
    QRegularExpressionMatch match;
    if (parts[1].indexOf(rx, 0, &match) < 0) {
        return {};
    }

    int faceId = match.captured(1).toInt();
    std::stringstream ss;
    ss << "Face" << faceId;

    std::vector<std::string> upToFaces(1, ss.str());
    auto sketchBased = getObject<PartDesign::ProfileBased>();
    sketchBased->UpToFace.setValue(obj, upToFaces);
    recomputeFeature();

    return QByteArray(ss.str().c_str());
}

QVariant TaskSketchBasedParameters::objectNameByLabel(const QString& label, const QVariant& suggest) const
{
    // search for an object with the given label
    App::Document* doc = getAppDocument();
    if (!doc) {
        return {};
    }

    // for faster access try the suggestion
    if (suggest.isValid()) {
        App::DocumentObject* obj = doc->getObject(suggest.toByteArray());
        if (obj && QString::fromUtf8(obj->Label.getValue()) == label) {
            return QVariant(QByteArray(obj->getNameInDocument()));
        }
    }

    // go through all objects and check the labels
    std::string name = label.toUtf8().data();
    std::vector<App::DocumentObject*> objs = doc->getObjects();
    for (auto obj : objs) {
        if (name == obj->Label.getValue()) {
            return QVariant(QByteArray(obj->getNameInDocument()));
        }
    }

    return {};  // no such feature found
}

QString TaskSketchBasedParameters::getFaceReference(const QString& obj, const QString& sub) const
{
    App::Document* doc = getAppDocument();
    if (!doc) {
        return {};
    }

    QString o = obj.left(obj.indexOf(QStringLiteral(":")));
    if (o.isEmpty()) {
        return {};
    }

    return QString::fromUtf8(R"((App.getDocument("%1").%2, ["%3"]))")
        .arg(QString::fromUtf8(doc->getName()), o, sub);
}

QString TaskSketchBasedParameters::make2DLabel(
    const App::DocumentObject* section,
    const std::vector<std::string>& subValues
)
{
    if (section->isDerivedFrom<Part::Part2DObject>()) {
        return QString::fromUtf8(section->Label.getValue());
    }
    else if (subValues.empty()) {
        Base::Console().error("No valid subelement linked in %s\n", section->Label.getValue());
        return {};
    }
    else {
        return QString::fromStdString((std::string(section->getNameInDocument()) + ":" + subValues[0]));
    }
}

TaskSketchBasedParameters::~TaskSketchBasedParameters()
{
    Gui::Selection().rmvSelectionGate();
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgSketchBasedParameters::TaskDlgSketchBasedParameters(PartDesignGui::ViewProvider* vp)
    : TaskDlgFeatureParameters(vp)
{}

TaskDlgSketchBasedParameters::~TaskDlgSketchBasedParameters() = default;

//==== calls from the TaskView ===============================================================


bool TaskDlgSketchBasedParameters::accept()
{
    auto feature = getObject<PartDesign::ProfileBased>();

    // Make sure the feature is what we are expecting
    // Should be fine but you never know...
    if (!feature) {
        throw Base::TypeError("Bad object processed in the sketch based dialog.");
    }

    // First verify that the feature can be built and then hide the profile as otherwise
    // it will remain hidden if the feature's recompute fails
    if (TaskDlgFeatureParameters::accept()) {
        App::DocumentObject* sketch = feature->Profile.getValue();
        Gui::cmdAppObjectHide(sketch);
        return true;
    }

    return false;
}

bool TaskDlgSketchBasedParameters::reject()
{
    auto feature = getObject<PartDesign::ProfileBased>();

    // Make sure the feature is what we are expecting
    // Should be fine but you never know...
    if (!feature) {
        throw Base::TypeError("Bad object processed in the sketch based dialog.");
    }

    App::DocumentObjectWeakPtrT weakptr(feature);
    auto sketch = dynamic_cast<Sketcher::SketchObject*>(feature->Profile.getValue());

    bool value = TaskDlgFeatureParameters::reject();

    // if abort command deleted the object the sketch is visible again.
    // The previous one feature already should be made visible
    if (weakptr.expired()) {
        // Make the sketch visible
        if (sketch && Gui::Application::Instance->getViewProvider(sketch)) {
            Gui::Application::Instance->getViewProvider(sketch)->show();
        }
    }

    return value;
}

#include "moc_TaskSketchBasedParameters.cpp"
