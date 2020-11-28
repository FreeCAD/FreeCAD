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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <Precision.hxx>
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <Base/Tools.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/PrefWidgets.h>

#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/FeatureOffset.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>
#include <Mod/PartDesign/App/FeatureExtrusion.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>

#include "Utils.h"
#include "ReferenceSelection.h"

#include "TaskSketchBasedParameters.h"

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::TaskSketchBasedParameters */

TaskSketchBasedParameters::TaskSketchBasedParameters(PartDesignGui::ViewProvider *vp, QWidget *parent,
                                                     const std::string& pixmapname, const QString& parname)
    : TaskFeatureParameters(vp, parent, pixmapname, parname)
{

}

void TaskSketchBasedParameters::initUI(QWidget *widget) {
    if(!vp)
        return;

    QBoxLayout * boxLayout = qobject_cast<QBoxLayout*>(widget->layout());
    if (!boxLayout)
        return;

    addNewSolidCheckBox(widget);

    PartDesign::ProfileBased* pcSketchBased = static_cast<PartDesign::ProfileBased*>(vp->getObject());

    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget(new QLabel(tr("Fit tolerance"), this));
    fitEdit = new Gui::PrefQuantitySpinBox(this);
    fitEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/ProfileFit"));
    fitEdit->bind(pcSketchBased->Fit);
    fitEdit->setUnit(Base::Unit::Length);
    fitEdit->setKeyboardTracking(false);
    fitEdit->setToolTip(QApplication::translate("Property", pcSketchBased->Fit.getDocumentation()));
    layout->addWidget(fitEdit);
    connect(fitEdit, SIGNAL(valueChanged(double)), this, SLOT(onFitChanged(double)));
    boxLayout->addLayout(layout);

    layout = new QHBoxLayout();
    layout->addWidget(new QLabel(tr("Fit join type")));
    fitJoinType = new QComboBox(this);
    for (int i=0;;++i) {
        const char * type = Part::Offset::JoinEnums[i];
        if (!type)
            break;
        fitJoinType->addItem(tr(type));
    }
    connect(fitJoinType, SIGNAL(currentIndexChanged(int)), this, SLOT(onFitJoinChanged(int)));
    layout->addWidget(fitJoinType);
    boxLayout->addLayout(layout);

    layout = new QHBoxLayout();
    layout->addWidget(new QLabel(tr("Inner fit"), this));
    innerFitEdit = new Gui::PrefQuantitySpinBox(this);
    innerFitEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/ProfileInnerFit"));
    innerFitEdit->bind(pcSketchBased->InnerFit);
    innerFitEdit->setUnit(Base::Unit::Length);
    innerFitEdit->setKeyboardTracking(false);
    innerFitEdit->setToolTip(QApplication::translate(
                "Property", pcSketchBased->InnerFit.getDocumentation()));
    layout->addWidget(innerFitEdit);
    connect(innerFitEdit, SIGNAL(valueChanged(double)), this, SLOT(onInnerFitChanged(double)));
    boxLayout->addLayout(layout);

    layout = new QHBoxLayout();
    layout->addWidget(new QLabel(tr("Inner fit join type")));
    innerFitJoinType = new QComboBox(this);
    for (int i=0;;++i) {
        const char * type = Part::Offset::JoinEnums[i];
        if (!type)
            break;
        innerFitJoinType->addItem(tr(type));
    }
    connect(innerFitJoinType, SIGNAL(currentIndexChanged(int)), this, SLOT(onInnerFitJoinChanged(int)));
    layout->addWidget(innerFitJoinType);
    boxLayout->addLayout(layout);

    addUpdateViewCheckBox(widget);
}

void TaskSketchBasedParameters::refresh()
{
    if (!vp || !vp->getObject())
        return;

    PartDesign::ProfileBased* pcSketchBased = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    if (fitEdit) {
        QSignalBlocker guard(fitEdit);
        fitEdit->setValue(pcSketchBased->Fit.getValue());
    }

    if (fitJoinType) {
        QSignalBlocker guard(fitJoinType);
        fitJoinType->setCurrentIndex(pcSketchBased->FitJoin.getValue());
    }

    if (innerFitEdit) {
        QSignalBlocker guard(innerFitEdit);
        innerFitEdit->setValue(pcSketchBased->InnerFit.getValue());
    }

    if (innerFitJoinType) {
        QSignalBlocker guard(innerFitJoinType);
        innerFitJoinType->setCurrentIndex(pcSketchBased->InnerFitJoin.getValue());
    }
    TaskFeatureParameters::refresh();
}

void TaskSketchBasedParameters::saveHistory(void)
{
    if (fitEdit)
        fitEdit->pushToHistory();
    if (innerFitEdit)
        innerFitEdit->pushToHistory();
    TaskFeatureParameters::saveHistory();
}

void TaskSketchBasedParameters::onFitChanged(double v)
{
    PartDesign::ProfileBased* pcSketchBased = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    pcSketchBased->Fit.setValue(v);
    recomputeFeature();
}

void TaskSketchBasedParameters::onFitJoinChanged(int v)
{
    PartDesign::ProfileBased* pcSketchBased = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    pcSketchBased->FitJoin.setValue((long)v);
    recomputeFeature();
}

void TaskSketchBasedParameters::onInnerFitChanged(double v)
{
    PartDesign::ProfileBased* pcSketchBased = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    pcSketchBased->InnerFit.setValue(v);
    recomputeFeature();
}

void TaskSketchBasedParameters::onInnerFitJoinChanged(int v)
{
    PartDesign::ProfileBased* pcSketchBased = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    pcSketchBased->InnerFitJoin.setValue((long)v);
    recomputeFeature();
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

    std::vector<std::string> upToFaces;

    // Remove subname for planes and datum features
    if (PartDesign::Feature::isDatum(selObj)) {
        subname = "";
        refStr = QString::fromLatin1(selObj->getNameInDocument());
    } else if (boost::starts_with(subname, "Face")) {
        int faceId = std::atoi(&subname[4]);
        if (faceId > 0) {
            refStr = QString::fromLatin1("%1:%2%3").arg(
                    QString::fromLatin1(selObj->getNameInDocument()),
                    QObject::tr("Face"),
                    QString::number(faceId));
            upToFaces.push_back(subname);
        }
    }

    if (refStr.isEmpty()) {
        if (subname.size()) {
            refStr = QString::fromLatin1("%1:%2").arg(
                    QString::fromLatin1(selObj->getNameInDocument()),
                    QString::fromLatin1(subname.c_str()));
            upToFaces.push_back(subname);
        } else
            refStr = QString::fromLatin1(selObj->getNameInDocument());
    }

    pcSketchBased->UpToFace.setValue(selObj, upToFaces);
    recomputeFeature();

    return refStr;
}

void TaskSketchBasedParameters::onSelectReference(const bool pressed, const bool edge, const bool face, const bool planar) {
    // Note: Even if there is no solid, App::Plane and Part::Datum can still be selected

    PartDesign::ProfileBased* pcSketchBased = dynamic_cast<PartDesign::ProfileBased*>(vp->getObject());
    if (pcSketchBased) {
        // The solid this feature will be fused to
        App::DocumentObject* prevSolid = pcSketchBased->getBaseObject( /* silent =*/ true );

        if (pressed) {
            Gui::Selection().clearSelection();
            Gui::Selection().addSelectionGate
                (new ReferenceSelection(prevSolid, edge, face, planar));
        } else {
            Gui::Selection().rmvSelectionGate();
        }
    }
}

void TaskSketchBasedParameters::exitSelectionMode()
{
    onSelectReference(false, false, false, false);
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
    if (obj == NULL)
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

        std::string upToFace;
        QRegExp rx(name);
        if (parts[1].indexOf(rx) > 0) {
            int faceId = rx.cap(1).toInt();
            std::stringstream ss;
            ss << "Face" << faceId;
            upToFace = ss.str();
        }
        else
            upToFace = parts[1].toLatin1().constData();

        PartDesign::ProfileBased* pcSketchBased = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        pcSketchBased->UpToFace.setValue(obj, {upToFace});
        recomputeFeature();

        return QByteArray(upToFace.c_str());
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
    if ( !feature->getTypeId().isDerivedFrom(PartDesign::ProfileBased::getClassTypeId()) ) {
        throw Base::TypeError("Bad object processed in the sketch based dialog.");
    }

    return TaskDlgFeatureParameters::accept();
}

bool TaskDlgSketchBasedParameters::reject()
{
    return TaskDlgFeatureParameters::reject();
}

#include "moc_TaskSketchBasedParameters.cpp"
