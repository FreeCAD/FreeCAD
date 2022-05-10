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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QRegExp>
# include <QTextStream>
#endif

#include <App/Document.h>
#include <App/Origin.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "TaskSketchBasedParameters.h"
#include "ReferenceSelection.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskSketchBasedParameters */

TaskSketchBasedParameters::TaskSketchBasedParameters(PartDesignGui::ViewProvider *vp, QWidget *parent,
                                                     const std::string& pixmapname, const QString& parname)
    : TaskFeatureParameters(vp, parent, pixmapname, parname)
{
    // disable selection
    this->blockSelection(true);
}

const QString TaskSketchBasedParameters::onAddSelection(const Gui::SelectionChanges& msg)
{
    // Note: The validity checking has already been done in ReferenceSelection.cpp
    PartDesign::ProfileBased* pcSketchBased = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    App::DocumentObject* selObj = pcSketchBased->getDocument()->getObject(msg.pObjectName);
    if (selObj == pcSketchBased)
        return QString();
    std::string subname = msg.pSubName;
    QString refStr;

    // Remove subname for planes and datum features
    if (PartDesign::Feature::isDatum(selObj)) {
        subname = "";
        refStr = QString::fromLatin1(selObj->getNameInDocument());
    } else if (subname.size() > 4) {
        int faceId = std::atoi(&subname[4]);
        refStr = QString::fromLatin1(selObj->getNameInDocument()) + QString::fromLatin1(":") + QObject::tr("Face") + QString::number(faceId);
    }

    std::vector<std::string> upToFaces(1,subname);
    pcSketchBased->UpToFace.setValue(selObj, upToFaces);
    recomputeFeature();

    return refStr;
}

void TaskSketchBasedParameters::startReferenceSelection(App::DocumentObject* profile, App::DocumentObject* base)
{
    Gui::Document* doc = vp->getDocument();
    if (doc) {
        doc->setHide(profile->getNameInDocument());
        if (base)
            doc->setShow(base->getNameInDocument());
    }
}

void TaskSketchBasedParameters::finishReferenceSelection(App::DocumentObject* profile, App::DocumentObject* base)
{
    Gui::Document* doc = vp->getDocument();
    if (doc) {
        doc->setShow(profile->getNameInDocument());
        if (base)
            doc->setHide(base->getNameInDocument());
    }
}

void TaskSketchBasedParameters::onSelectReference(AllowSelectionFlags allow) {
    // Note: Even if there is no solid, App::Plane and Part::Datum can still be selected

    PartDesign::ProfileBased* pcSketchBased = dynamic_cast<PartDesign::ProfileBased*>(vp->getObject());
    if (pcSketchBased) {
        // The solid this feature will be fused to
        App::DocumentObject* prevSolid = pcSketchBased->getBaseObject( /* silent =*/ true );

        if (AllowSelectionFlags::Int(allow) != int(AllowSelection::NONE)) {
            startReferenceSelection(pcSketchBased, prevSolid);
            this->blockSelection(false);
            Gui::Selection().clearSelection();
            Gui::Selection().addSelectionGate(new ReferenceSelection(prevSolid, allow));
        }
        else {
            Gui::Selection().rmvSelectionGate();
            finishReferenceSelection(pcSketchBased, prevSolid);
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
    if (text.isEmpty())
        return QVariant();

    QStringList parts = text.split(QChar::fromLatin1(':'));
    if (parts.length() < 2)
        parts.push_back(QString::fromLatin1(""));

    // Check whether this is the name of an App::Plane or Part::Datum feature
    App::DocumentObject* obj = vp->getObject()->getDocument()->getObject(parts[0].toLatin1());
    if (obj == nullptr)
        return QVariant();

    if (obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
        // everything is OK (we assume a Part can only have exactly 3 App::Plane objects located at the base of the feature tree)
        return QVariant();
    }
    else if (obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
        // it's up to the document to check that the datum plane is in the same body
        return QVariant();
    }
    else {
        // We must expect that "parts[1]" is the translation of "Face" followed by an ID.
        QString name;
        QTextStream str(&name);
        str << "^" << tr("Face") << "(\\d+)$";
        QRegExp rx(name);
        if (parts[1].indexOf(rx) < 0) {
            return QVariant();
        }

        int faceId = rx.cap(1).toInt();
        std::stringstream ss;
        ss << "Face" << faceId;

        std::vector<std::string> upToFaces(1,ss.str());
        PartDesign::ProfileBased* pcSketchBased = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        pcSketchBased->UpToFace.setValue(obj, upToFaces);
        recomputeFeature();

        return QByteArray(ss.str().c_str());
    }
}

QVariant TaskSketchBasedParameters::objectNameByLabel(const QString& label,
                                                      const QVariant& suggest) const
{
    // search for an object with the given label
    App::Document* doc = this->vp->getObject()->getDocument();
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
    for (std::vector<App::DocumentObject*>::iterator it = objs.begin(); it != objs.end(); ++it) {
        if (name == (*it)->Label.getValue()) {
            return QVariant(QByteArray((*it)->getNameInDocument()));
        }
    }

    return QVariant(); // no such feature found
}

QString TaskSketchBasedParameters::getFaceReference(const QString& obj, const QString& sub) const
{
    App::Document* doc = this->vp->getObject()->getDocument();
    QString o = obj.left(obj.indexOf(QString::fromLatin1(":")));

    if (o.isEmpty())
        return QString();

    return QString::fromLatin1("(App.getDocument(\"%1\").%2, [\"%3\"])")
            .arg(QString::fromLatin1(doc->getName()), o, sub);
}

QString TaskSketchBasedParameters::make2DLabel(const App::DocumentObject* section,
                                               const std::vector<std::string>& subValues)
{
    if (section->isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        return QString::fromUtf8(section->Label.getValue());
    }
    else if (subValues.empty()) {
        Base::Console().Error("No valid subelement linked in %s\n", section->Label.getValue());
        return QString();
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

TaskDlgSketchBasedParameters::TaskDlgSketchBasedParameters(PartDesignGui::ViewProvider *vp)
    : TaskDlgFeatureParameters(vp)
{
}

TaskDlgSketchBasedParameters::~TaskDlgSketchBasedParameters()
{

}

//==== calls from the TaskView ===============================================================


bool TaskDlgSketchBasedParameters::accept() {
    App::DocumentObject* feature = vp->getObject();

    // Make sure the feature is what we are expecting
    // Should be fine but you never know...
    if (!feature->getTypeId().isDerivedFrom(PartDesign::ProfileBased::getClassTypeId())) {
        throw Base::TypeError("Bad object processed in the sketch based dialog.");
    }

    // First verify that the feature can be built and then hide the profile as otherwise
    // it will remain hidden if the feature's recompute fails
    if (TaskDlgFeatureParameters::accept()) {
        App::DocumentObject* sketch = static_cast<PartDesign::ProfileBased*>(feature)->Profile.getValue();
        Gui::cmdAppObjectHide(sketch);
        return true;
    }

    return false;
}

bool TaskDlgSketchBasedParameters::reject()
{
    PartDesign::ProfileBased* pcSketchBased = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    App::DocumentObjectWeakPtrT weakptr(pcSketchBased);
    // get the Sketch
    Sketcher::SketchObject *pcSketch = static_cast<Sketcher::SketchObject*>(pcSketchBased->Profile.getValue());
    bool rv;

    // rv should be true anyway but to be on the safe side due to further changes better respect it.
    rv = TaskDlgFeatureParameters::reject();

    // if abort command deleted the object the sketch is visible again.
    // The previous one feature already should be made visible
    if (weakptr.expired()) {
        // Make the sketch visible
        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
            Gui::Application::Instance->getViewProvider(pcSketch)->show();
    }

    return rv;
}

#include "moc_TaskSketchBasedParameters.cpp"
