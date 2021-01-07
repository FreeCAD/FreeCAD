/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *                 2020 David Österberg                                    *
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
#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QFontDatabase>
#include <map>
#endif

#include <Base/UnitsApi.h>
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
#include <Gui/ViewProviderOrigin.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/FeatureText.h>
#include <Mod/PartDesign/App/FeatureGroove.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <QString>
#include <QLineEdit>
#include <QFont>
#include <QStandardPaths>

#include "ReferenceSelection.h"
#include "Utils.h"

#include "ui_TaskTextParameters.h"
#include "TaskTextParameters.h"

using namespace PartDesignGui;
using namespace Gui;


/* TRANSLATOR PartDesignGui::TaskTextParameters */

TaskTextParameters::TaskTextParameters(PartDesignGui::ViewProviderText *TextView, QWidget *parent)
    : TaskSketchBasedParameters(TextView, parent, "PartDesign_Text",tr("Text parameters")),
    ui (new Ui_TaskTextParameters)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
    QMetaObject::connectSlotsByName(this);

    connect(ui->axis, SIGNAL(activated(int)),
            this, SLOT(onAxisChanged(int)));
    connect(ui->text, SIGNAL(textChanged(const QString &)),
            this, SLOT(onTextChanged(QString)));
    connect(ui->font, SIGNAL(currentFontChanged(const QFont &)),
            this, SLOT(onFontChanged(QFont)));
    connect(ui->size, SIGNAL(valueChanged(double)),
            this, SLOT(onSizeChanged(double)));
    connect(ui->height, SIGNAL(valueChanged(double)),
            this, SLOT(onHeightChanged(double)));



    this->groupLayout()->addWidget(proxy);

    getFontPaths();

    ui->font->setWritingSystem(QFontDatabase::Latin);
    ui->font->setFontFilters(QFontComboBox::ScalableFonts);


    // Temporarily prevent unnecessary feature recomputes
    ui->axis->blockSignals(true);
    ui->text->blockSignals(true);
    ui->font->blockSignals(true);
    ui->size->blockSignals(true);
    ui->height->blockSignals(true);

    //bind property mirrors
    PartDesign::ProfileBased* pcFeat = static_cast<PartDesign::ProfileBased*>(vp->getObject());

    PartDesign::Text* rev = static_cast<PartDesign::Text*>(vp->getObject());

    this->propReferenceAxis = &(rev->ReferenceAxis);
    this->propText = &(rev->TextString);
    this->propFont = &(rev->Font);
    this->propSize = &(rev->Size);
    this->propHeight = &(rev->Height);

    QString text(QString::fromUtf8(propText->getValue()));
    QString fontFam(QString::fromUtf8(propFont->getValue()));
    int pointSize = -1;
    int weight = -1;
    bool italic = false;
    QFont font(fontFam, pointSize, weight, italic);
    double size = propSize->getValue();
    double height = propHeight->getValue();

    ui->text->setText(text);
    //ui->font->setCurrentFont(font);
    ui->size->setValue(size);
    ui->height->setValue(height);

    blockUpdate = false;
    updateUI();

    // make it possible to use expressions for the height and size
    ui->height->bind(static_cast<PartDesign::Text *>(pcFeat)->Height);
    ui->size->bind(static_cast<PartDesign::Text *>(pcFeat)->Size);

    ui->axis->blockSignals(false);
    ui->text->blockSignals(false);
    ui->font->blockSignals(false);
    ui->size->blockSignals(false);
    ui->height->blockSignals(false);

    setFocus ();

    //show the parts coordinate system axis for selection
    PartDesign::Body * body = PartDesign::Body::findBodyOf ( vp->getObject () );
    if(body) {
        try {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->setTemporaryVisibility(true, false);
        } catch (const Base::Exception &ex) {
            ex.ReportException();
        }
     }
}

void TaskTextParameters::fillAxisCombo(bool forceRefill)
{
    bool oldVal_blockUpdate = blockUpdate;
    blockUpdate = true;

    if (axesInList.empty())
        forceRefill = true;//not filled yet, full refill

    if (forceRefill){
        ui->axis->clear();

        this->axesInList.clear();

        //add sketch axes
        PartDesign::ProfileBased* pcFeat = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        Part::Part2DObject* pcSketch = dynamic_cast<Part::Part2DObject*>(pcFeat->Profile.getValue());
        if (pcSketch){
            addAxisToCombo(pcSketch,"V_Axis",QObject::tr("Vertical sketch axis"));
            addAxisToCombo(pcSketch,"H_Axis",QObject::tr("Horizontal sketch axis"));
            for (int i=0; i < pcSketch->getAxisCount(); i++) {
                QString itemText = QObject::tr("Construction line %1").arg(i+1);
                std::stringstream sub;
                sub << "Axis" << i;
                addAxisToCombo(pcSketch,sub.str(),itemText);
            }
        }

        //add part axes
        PartDesign::Body * body = PartDesign::Body::findBodyOf ( pcFeat );
        if (body) {
            try {
                App::Origin* orig = body->getOrigin();
                addAxisToCombo(orig->getX(),"",tr("Base X axis"));
                addAxisToCombo(orig->getY(),"",tr("Base Y axis"));
                addAxisToCombo(orig->getZ(),"",tr("Base Z axis"));
            } catch (const Base::Exception &ex) {
                ex.ReportException();
            }
        }

        //add "Select reference"
        addAxisToCombo(0,std::string(),tr("Select reference..."));
    }//endif forceRefill

    //add current link, if not in list
    //first, figure out the item number for current axis
    int indexOfCurrent = -1;
    App::DocumentObject* ax = propReferenceAxis->getValue();
    const std::vector<std::string> &subList = propReferenceAxis->getSubValues();
    for (size_t i = 0; i < axesInList.size(); i++) {
        if (ax == axesInList[i]->getValue() && subList == axesInList[i]->getSubValues())
            indexOfCurrent = i;
    }
    if (indexOfCurrent == -1  &&  ax) {
        assert(subList.size() <= 1);
        std::string sub;
        if (!subList.empty())
            sub = subList[0];
        addAxisToCombo(ax, sub, getRefStr(ax, subList));
        indexOfCurrent = axesInList.size()-1;
    }

    //highlight current.
    if (indexOfCurrent != -1)
        ui->axis->setCurrentIndex(indexOfCurrent);

    blockUpdate = oldVal_blockUpdate;
}

void TaskTextParameters::addAxisToCombo(App::DocumentObject* linkObj,
                                              std::string linkSubname,
                                              QString itemText)
{
    this->ui->axis->addItem(itemText);
    this->axesInList.emplace_back(new App::PropertyLinkSub);
    App::PropertyLinkSub &lnk = *(axesInList[axesInList.size()-1]);
    lnk.setValue(linkObj,std::vector<std::string>(1,linkSubname));
}

void TaskTextParameters::updateUI()
{
    fillAxisCombo();

    PartDesign::ProfileBased* pcText = static_cast<PartDesign::ProfileBased*>(vp->getObject());
    ui->labelMessage->setText(QString::fromUtf8(pcText->getStatusString()));

}

void TaskTextParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type == Gui::SelectionChanges::AddSelection) {

        exitSelectionMode();
        std::vector<std::string> axis;
        App::DocumentObject* selObj;
        if (getReferencedSelection(vp->getObject(), msg, selObj, axis) && selObj) {
            propReferenceAxis->setValue(selObj, axis);

            if (blockUpdate)
                return;

            recomputeFeature();
            updateUI();
        }
    }
}


void TaskTextParameters::onTextChanged(QString s)
{
    propText->setValue(s.toStdString());
    recomputeFeature();
    updateUI();
}


void TaskTextParameters::onFontChanged(QFont font)
{

    Base::Console().Warning("%s\n", font.family().toStdString().c_str());

    auto itr = fontPath.find(font.family());
    if (itr == fontPath.end())
        throw Base::ValueError("Could not find selected font");

    propFont->setValue((itr->second).toStdString());
    recomputeFeature();
    updateUI();
}

void TaskTextParameters::onSizeChanged(double size)
{
    propSize->setValue(size);
    recomputeFeature();
    updateUI();
}

void TaskTextParameters::onHeightChanged(double h)
{
    propHeight->setValue(h);
    recomputeFeature();
    updateUI();
}

void TaskTextParameters::onAxisChanged(int num)
{
    PartDesign::ProfileBased* pcText = static_cast<PartDesign::ProfileBased*>(vp->getObject());

    if (axesInList.empty())
        return;

    App::PropertyLinkSub &lnk = *(axesInList[num]);
    if (lnk.getValue() == 0) {
        // enter reference selection mode
        TaskSketchBasedParameters::onSelectReference(true, true, false, true);
    } else {
        if (!pcText->getDocument()->isIn(lnk.getValue())){
            Base::Console().Error("Object was deleted\n");
            return;
        }
        propReferenceAxis->Paste(lnk);
        exitSelectionMode();
    }

    try {
        recomputeFeature();
        updateUI();
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}


TaskTextParameters::~TaskTextParameters()
{
    try {
        //hide the parts coordinate system axis for selection
        PartDesign::Body * body = vp ? PartDesign::Body::findBodyOf(vp->getObject()) : 0;
        if (body) {
            App::Origin *origin = body->getOrigin();
            ViewProviderOrigin* vpOrigin;
            vpOrigin = static_cast<ViewProviderOrigin*>(Gui::Application::Instance->getViewProvider(origin));
            vpOrigin->resetTemporaryVisibility();
        }
    } catch (const Base::Exception &ex) {
        ex.ReportException();
    }

}

void TaskTextParameters::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}

void TaskTextParameters::getReferenceAxis(App::DocumentObject*& obj, std::vector<std::string>& sub) const
{
    if (axesInList.empty())
        throw Base::RuntimeError("Not initialized!");

    int num = ui->axis->currentIndex();
    const App::PropertyLinkSub &lnk = *(axesInList[num]);
    if (lnk.getValue() == 0) {
        throw Base::RuntimeError("Still in reference selection mode; reference wasn't selected yet");
    } else {
        PartDesign::ProfileBased* pcRevolution = static_cast<PartDesign::ProfileBased*>(vp->getObject());
        if (!pcRevolution->getDocument()->isIn(lnk.getValue())){
            throw Base::RuntimeError("Object was deleted");
        }

        obj = lnk.getValue();
        sub = lnk.getSubValues();
    }
}

// this is used for logging the command fully when recording macros
void TaskTextParameters::apply()
{
    std::vector<std::string> sub;
    App::DocumentObject* obj;
    getReferenceAxis(obj, sub);
    std::string axis = buildLinkSingleSubPythonStr(obj, sub);
    auto tobj = vp->getObject();
    FCMD_OBJ_CMD(tobj,"ReferenceAxis = " << axis);
    FCMD_OBJ_CMD(tobj,"TextString = \"" << propText->getValue() << "\"");
    FCMD_OBJ_CMD(tobj,"Font = \"" << propFont->getValue() << "\"");
    FCMD_OBJ_CMD(tobj,"Size = " << propSize->getValue());
    FCMD_OBJ_CMD(tobj,"Height = " << propHeight->getValue());
}


// Based on https://stackoverflow.com/a/64729615
// Ported to c++
// Modified to ignore Non-TrueType fonts
void TaskTextParameters::getFontPaths(void)
{
    QStringList font_paths = QStandardPaths::standardLocations(QStandardPaths::FontsLocation);
    QFontDatabase db;
    QStringListIterator fpathIterator(font_paths);
    while (fpathIterator.hasNext()) {
        QString fpath = fpathIterator.next();
        QDirIterator filenameIt(fpath, QDirIterator::Subdirectories);
        while (filenameIt.hasNext()) {
            QString filename = filenameIt.next();
            //if (!filename.endsWith(QString::fromUtf8(".ttf"))  && !filename.endsWith(QString::fromUtf8(".ttc")))
            if (!filename.endsWith(QString::fromUtf8(".ttf")))
                continue;
            QString path = QDir(fpath).filePath(filename);
            int idx = db.addApplicationFont(path);  // add font path
            if (idx >= 0) { // success
                QStringList names = db.applicationFontFamilies(idx);  // load back font family name
                QStringListIterator namesIterator(names);
                while (namesIterator.hasNext()) {
                    QString name = namesIterator.next();
                    if(fontPath.count(name) == 0) {
                        fontPath.insert({name, path});
                    }
                }
            }
        }
    }
}

//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
TaskDlgTextParameters::TaskDlgTextParameters(ViewProviderText *TextView)
    : TaskDlgSketchBasedParameters(TextView)
{
    assert(TextView);
    Content.push_back(new TaskTextParameters(TextView));
}


#include "moc_TaskTextParameters.cpp"
