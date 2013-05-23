/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender@users.sourceforge.net>*
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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include "TaskSketchBasedParameters.h"
#include <App/Application.h>
#include <App/Document.h>
#include <App/Plane.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Base/Console.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include "ReferenceSelection.h"
#include "Workbench.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskSketchBasedParameters */

TaskSketchBasedParameters::TaskSketchBasedParameters(ViewProvider *vp, QWidget *parent,
                                                     const std::string& pixmapname, const QString& parname)
    : TaskBox(Gui::BitmapFactory().pixmap(pixmapname.c_str()),parname,true, parent),
      vp(vp), blockUpdate(false)
{

}

const QString TaskSketchBasedParameters::onAddSelection(const Gui::SelectionChanges& msg)
{
    // Note: The validity checking has already been done in ReferenceSelection.cpp
    PartDesign::SketchBased* pcSketchBased = static_cast<PartDesign::SketchBased*>(vp->getObject());
    App::DocumentObject* selObj = pcSketchBased->getDocument()->getObject(msg.pObjectName);
    if (selObj == pcSketchBased)
        return QString::fromAscii("");
    std::string subname = msg.pSubName;
    QString refStr;

    // Remove subname for planes and datum features
    if (PartDesign::Feature::isDatum(selObj)) {
        subname = "";
        refStr = QString::fromAscii(selObj->getNameInDocument());
    } else {
        int faceId = std::atoi(&subname[4]);
        refStr = QString::fromAscii(selObj->getNameInDocument()) + QObject::tr(":Face") + QString::number(faceId);
    }

    std::vector<std::string> upToFaces(1,subname);
    pcSketchBased->UpToFace.setValue(selObj, upToFaces);
    recomputeFeature();

    return refStr;
}

void TaskSketchBasedParameters::onSelectReference(const bool pressed, const bool edge, const bool face, const bool planar) {
    // Note: Even if there is no solid, App::Plane and Part::Datum can still be selected
    App::DocumentObject* solid = PartDesignGui::ActivePartObject->getPrevSolidFeature(NULL, false);
    PartDesign::SketchBased* pcSketchBased = static_cast<PartDesign::SketchBased*>(vp->getObject());

    if (pressed) {
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc && solid) {
            doc->setHide(pcSketchBased->getNameInDocument());
            doc->setShow(solid->getNameInDocument());
        }
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate
            (new ReferenceSelection(solid, edge, face, planar));
    } else {
        Gui::Selection().rmvSelectionGate();
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        if (doc && solid) {
            doc->setShow(pcSketchBased->getNameInDocument());
            doc->setHide(solid->getNameInDocument());
        }
    }
}

const QByteArray TaskSketchBasedParameters::onFaceName(const QString& text)
{
    if (text.length() == 0)
        return QByteArray();

    QStringList parts = text.split(QChar::fromAscii(':'));
    if (parts.length() < 2)
        parts.push_back(QString::fromAscii(""));
    // Check whether this is the name of an App::Plane or Part::Datum feature
    App::DocumentObject* obj = vp->getObject()->getDocument()->getObject(parts[0].toAscii());
    if (obj == NULL)
        return QByteArray();

    if (obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
        // everything is OK (we assume a Part can only have exactly 3 App::Plane objects located at the base of the feature tree)
        return QByteArray();
    } else if (obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {
        if (!PartDesignGui::ActivePartObject->hasFeature(obj))
            return QByteArray();
        return QByteArray();
    } else {
        // We must expect that "text" is the translation of "Face" followed by an ID.
        QString name;
        QTextStream str(&name);
        str << "^" << tr("Face") << "(\\d+)$";
        QRegExp rx(name);
        if (text.indexOf(rx) < 0) {
            return QByteArray();
        }

        int faceId = rx.cap(1).toInt();
        std::stringstream ss;
        ss << "Face" << faceId;

        std::vector<std::string> upToFaces(1,ss.str());
        PartDesign::SketchBased* pcSketchBased = static_cast<PartDesign::SketchBased*>(vp->getObject());
        pcSketchBased->UpToFace.setValue(obj, upToFaces);
        recomputeFeature();

        return QByteArray(ss.str().c_str());
    }
}

QString TaskSketchBasedParameters::getFaceReference(const QString& obj, const QString& sub) const
{
    QString o = obj.left(obj.indexOf(QString::fromAscii(":")));

    if (o == tr("No face selected"))
        return QString::fromAscii("");
    else
        return QString::fromAscii("(App.activeDocument().") + o +
                QString::fromAscii(", [\"") + sub + QString::fromAscii("\"])");
}

void TaskSketchBasedParameters::onUpdateView(bool on)
{
    blockUpdate = !on;
    recomputeFeature();
}

void TaskSketchBasedParameters::recomputeFeature()
{
    if (!blockUpdate) {
        PartDesign::SketchBased* pcSketchBased = static_cast<PartDesign::SketchBased*>(vp->getObject());
        pcSketchBased->getDocument()->recomputeFeature(pcSketchBased);
    }
}

TaskSketchBasedParameters::~TaskSketchBasedParameters()
{
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgSketchBasedParameters::TaskDlgSketchBasedParameters(ViewProvider *vp)
    : TaskDialog(),vp(vp)
{
}

TaskDlgSketchBasedParameters::~TaskDlgSketchBasedParameters()
{

}

//==== calls from the TaskView ===============================================================


void TaskDlgSketchBasedParameters::open()
{
    
}

void TaskDlgSketchBasedParameters::clicked(int)
{
    
}

bool TaskDlgSketchBasedParameters::reject()
{
    // get the support and Sketch
    PartDesign::SketchBased* pcSketchBased = static_cast<PartDesign::SketchBased*>(vp->getObject());
    Sketcher::SketchObject *pcSketch;
    if (pcSketchBased->Sketch.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcSketchBased->Sketch.getValue());
    }    

    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    
    // if abort command deleted the object the sketch is visible again
    if (!Gui::Application::Instance->getViewProvider(pcSketchBased)) {
        if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
            Gui::Application::Instance->getViewProvider(pcSketch)->show();
    }

    // Body housekeeping
    if (ActivePartObject != NULL) {
        // Make the new Tip and the previous solid feature visible again
        App::DocumentObject* tip = ActivePartObject->Tip.getValue();
        App::DocumentObject* prev = ActivePartObject->getPrevSolidFeature();
        if (tip != NULL) {
            Gui::Application::Instance->getViewProvider(tip)->show();
            if ((tip != prev) && (prev != NULL))
                Gui::Application::Instance->getViewProvider(prev)->show();
        }
    }

    return true;
}


#include "moc_TaskSketchBasedParameters.cpp"
