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
# include <QApplication>
# include <QRegExp>
# include <QMessageBox>
# include <memory>
#endif

#include "DlgSettings3DViewImp.h"
#include "NavigationStyle.h"
#include "PrefWidgets.h"
#include "View3DInventorViewer.h"
#include "ui_MouseButtons.h"
#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Gui/Command.h>

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettings3DViewImp */

bool DlgSettings3DViewImp::showMsg = true;

/**
 *  Constructs a DlgSettings3DViewImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettings3DViewImp::DlgSettings3DViewImp(QWidget* parent)
    : PreferencePage( parent )
{
    this->setupUi(this);
    retranslate();
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettings3DViewImp::~DlgSettings3DViewImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettings3DViewImp::saveSettings()
{
    // must be done as very first because we create a new instance of NavigatorStyle
    // where we set some attributes afterwards
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    QVariant data = comboNavigationStyle->itemData(comboNavigationStyle->currentIndex(), Qt::UserRole);
    hGrp->SetASCII("NavigationStyle", (const char*)data.toByteArray());

    int index = comboOrbitStyle->currentIndex();
    hGrp->SetInt("OrbitStyle", index);
    
    index = this->comboAliasing->currentIndex();
    hGrp->SetInt("AntiAliasing", index);

    index = this->comboNewDocView->currentIndex();
    hGrp->SetASCII("NewDocumentCameraOrientation",comboNewDocView->currentText().toStdString().c_str());

    index = this->naviCubeCorner->currentIndex();
    hGrp->SetInt("CornerNaviCube", index);

    QVariant const &vBoxMarkerSize = this->boxMarkerSize->itemData(this->boxMarkerSize->currentIndex());
    hGrp->SetInt("MarkerSize", vBoxMarkerSize.toInt());



    checkBoxZoomAtCursor->onSave();
    checkBoxInvertZoom->onSave();
    spinBoxZoomStep->onSave();
    checkBoxDragAtCursor->onSave();
    CheckBox_CornerCoordSystem->onSave();
    CheckBox_ShowFPS->onSave();
    CheckBox_useVBO->onSave();
    CheckBox_NaviCube->onSave();
    CheckBox_UseAutoRotation->onSave();
    FloatSpinBox_EyeDistance->onSave();
    checkBoxBacklight->onSave();
    backlightColor->onSave();
    sliderIntensity->onSave();
    radioPerspective->onSave();
    radioOrthographic->onSave();
}

void DlgSettings3DViewImp::loadSettings()
{
    checkBoxZoomAtCursor->onRestore();
    checkBoxInvertZoom->onRestore();
    spinBoxZoomStep->onRestore();
    checkBoxDragAtCursor->onRestore();
    CheckBox_CornerCoordSystem->onRestore();
    CheckBox_ShowFPS->onRestore();
    CheckBox_useVBO->onRestore();
    CheckBox_NaviCube->onRestore();
    CheckBox_UseAutoRotation->onRestore();
    FloatSpinBox_EyeDistance->onRestore();
    checkBoxBacklight->onRestore();
    backlightColor->onRestore();
    sliderIntensity->onRestore();
    radioPerspective->onRestore();
    radioOrthographic->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    std::string model = hGrp->GetASCII("NavigationStyle",CADNavigationStyle::getClassTypeId().getName());
    int index = comboNavigationStyle->findData(QByteArray(model.c_str()));
    if (index > -1) comboNavigationStyle->setCurrentIndex(index);

    index = hGrp->GetInt("OrbitStyle", int(NavigationStyle::Trackball));
    index = Base::clamp(index, 0, comboOrbitStyle->count()-1);
    comboOrbitStyle->setCurrentIndex(index);
    
    index = hGrp->GetInt("AntiAliasing", int(Gui::View3DInventorViewer::None));
    index = Base::clamp(index, 0, comboAliasing->count()-1);
    comboAliasing->setCurrentIndex(index);
    // connect after setting current item of the combo box
    connect(comboAliasing, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onAliasingChanged(int)));

    index = hGrp->GetInt("CornerNaviCube", 1);
    naviCubeCorner->setCurrentIndex(index);

    int const current = hGrp->GetInt("MarkerSize", 9L);
    this->boxMarkerSize->addItem(tr("5px"), QVariant(5));
    this->boxMarkerSize->addItem(tr("7px"), QVariant(7));
    this->boxMarkerSize->addItem(tr("9px"), QVariant(9));
    this->boxMarkerSize->addItem(tr("11px"), QVariant(11));
    this->boxMarkerSize->addItem(tr("13px"), QVariant(13));
    this->boxMarkerSize->addItem(tr("15px"), QVariant(15));
    index = this->boxMarkerSize->findData(QVariant(current));
    if (index < 0) index = 2;
    this->boxMarkerSize->setCurrentIndex(index);

    this->comboNewDocView->addItem(tr("Axonometric"));
    this->comboNewDocView->addItem(tr("Isometric"));
    this->comboNewDocView->addItem(tr("Dimetric"));
    this->comboNewDocView->addItem(tr("Trimetric"));
    this->comboNewDocView->addItem(tr("Top"));
    this->comboNewDocView->addItem(tr("Front"));
    this->comboNewDocView->addItem(tr("Left"));
    this->comboNewDocView->addItem(tr("Right"));
    this->comboNewDocView->addItem(tr("Rear"));
    this->comboNewDocView->addItem(tr("Bottom"));
    this->comboNewDocView->addItem(tr("Use current"));
    this->comboNewDocView->addItem(tr("Custom"));
    //parameter setting will always be in English even if the items in the combobox have been translated
    std::string newDocView = hGrp->GetASCII("NewDocumentCameraOrientation","Axonometric");
    if (newDocView.find("Rotation")==0 || newDocView.find("V")==0){
        this->comboNewDocView->addItem(QString::fromStdString(newDocView));
    }
    index = this->comboNewDocView->findText(QString::fromStdString(newDocView));
    if (index != -1){
        this->comboNewDocView->setCurrentIndex(comboNewDocView->findText(QString::fromStdString(newDocView)));
    }
    connect(comboNewDocView, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onNewDocViewChanged(int)));
}

void DlgSettings3DViewImp::onNewDocViewChanged(int index){
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");

    //convert current selection back to English, if necessary for processing
    std::vector<std::string> items = std::vector<std::string>{"Axonometric", "Isometric", "Dimetric",
                                                              "Trimetric", "Top", "Front", "Left",
                                                              "Right","Rear","Bottom","Use current",
                                                              "Custom"};
    std::string newDocView;
    if ((std::string::size_type) index < items.size()){
        newDocView = items[index];
    } else { //must be a custom rotation added dynamically to the combobox
        newDocView = this->comboNewDocView->currentText().toStdString();
    }
    if (newDocView == "Use current"){ //get current orientation and set parameter with it from python
        QString cmdString = QString::fromLatin1("if App.ActiveDocument:\n");
        cmdString += QString::fromLatin1("    FreeCAD.ParamGet(\"User parameter:BaseApp/Preferences/View\")");
        cmdString += QString::fromLatin1(".SetString(\"NewDocumentCameraOrientation\",str(Gui.activeView().getCameraOrientation()))\n");
        Gui::Command::doCommand(Gui::Command::Doc,cmdString.toStdString().c_str());
    } else if (newDocView == "Custom"){ //get custom orientation from user and set the parameter via python
        QString cmdString = QString::fromLatin1("from PySide import QtGui\n");
        cmdString += QString::fromLatin1("_s,_ok = QtGui.QInputDialog.getText(Gui.getMainWindow(),");
        cmdString += tr("\"Custom New Document Camera Orientation\","); //dlg title
        cmdString += QString::fromLatin1("\"Enter V() values in the form V(x,y,z) where V() is a camera aiming vector, but reversed\\n\\n");//dlg msg
        cmdString += QString::fromLatin1("Alternatively, use form Rotation(q0,q1,q2,q3) or R(q0,q1,q2,q3) using quaternions\\n");
        cmdString += QString::fromLatin1("where q0 = x, q1 = y, q2 = z and q3 = w, the quaternion is specified by q=w+xi+yj+zk.\\n\\n");
        cmdString += QString::fromLatin1("Examples:\\n\\nV(0,0,1) = Top, V(1,0,0) = Right, V(-1,0,0) = Left\\n");
        cmdString += QString::fromLatin1("V(1,-1,.5) = Diametric, V(.5,-1,.5) = Trimetric\\n");
        cmdString += QString::fromLatin1("Rotation(44,18,34,82) ~= Isometric, R(53,13,20,82) ~= Trimetric\\n\\n");
        cmdString += QString::fromLatin1("\""); //end dlg msg
        cmdString += QString::fromLatin1(",text=\'V(1,-2,1)\')\n"); //default text
        cmdString += QString::fromLatin1("if _ok:\n");
        cmdString += QString::fromLatin1("    if not \'Right\' in _s and not \'Rear\' in _s and not \'Rotation\' in _s and _s.startswith(\'R\'):\n");
        cmdString += QString::fromLatin1("        _s = _s.replace(\'R\',\'Rotation\')\n"); //allow R() alias for Rotation()
        cmdString += QString::fromLatin1("    FreeCAD.ParamGet(\"User parameter:BaseApp/Preferences/View\")");
        cmdString += QString::fromLatin1(".SetString(\"NewDocumentCameraOrientation\",_s)\n");
        Gui::Command::doCommand(Gui::Command::Doc,cmdString.toStdString().c_str());
        newDocView = hGrp->GetASCII("NewDocumentCameraOrientation","Axonometric");
        if (newDocView == "Custom"){ //user canceled, so make it default Axonometric
            newDocView = "Axonometric";
        }
        //update combobox in case user clicked apply instead of ok
        this->comboNewDocView->addItem(QString::fromStdString(newDocView));
        index = this->comboNewDocView->findText(QString::fromStdString(newDocView));
        if (index != -1){
            this->comboNewDocView->setCurrentIndex(comboNewDocView->findText(QString::fromStdString(newDocView)));
        }
    } else { //not Custom or Use current
        hGrp->SetASCII("NewDocumentCameraOrientation",newDocView.c_str());
    }
        // make this the current camera orientation if we have an active document unless it was Use current
    if (App::GetApplication().getDocuments().size() > 0 && newDocView != "Use current"){
        if (newDocView.find("Rotation")==0) { //user wants a custom rotation
            Gui::Command::doCommand(Gui::Command::Doc,
                                    std::string("Gui.activeView().setCameraOrientation(App."+newDocView+")").c_str());
        } else if (newDocView.find("V")==0) { //user wants a custom rotation using for example: V(3,2,1)
            Gui::Command::doCommand(Gui::Command::Doc,
                                    std::string("V=App.Vector\nGui.activeView().setCameraOrientation(App.Rotation(V(),V(0,0,1),"+newDocView+",\"ZYX\"))").c_str());
        } else if (newDocView == "Isometric"){
            Gui::Command::doCommand(Gui::Command::Doc,
                                    std::string("Gui.activeView().setCameraOrientation(App.Rotation (0.4247081321999479, 0.1759200437218226, 0.339851090706265, 0.8204732639190053))").c_str());
        } else if (newDocView == "Dimetric"){
            Gui::Command::doCommand(Gui::Command::Doc,
                                    std::string("Gui.activeView().setCameraOrientation(App.Rotation (0.5334020967542208, 0.22094238278685738, 0.31245971396736344, 0.7543444795410782))").c_str());
        } else if (newDocView == "Trimetric"){
            Gui::Command::doCommand(Gui::Command::Doc,
                                    std::string("Gui.activeView().setCameraOrientation(App.Rotation (0.5293936757472739, 0.12497288799917983, 0.19279050929146058, 0.8166737003669666))").c_str());
        } else if (newDocView == "Top" || newDocView == "Bottom" || newDocView == "Right" || newDocView == "Left"
                   || newDocView == "Rear" || newDocView == "Axonometric" || newDocView == "Front"){
            Gui::Command::doCommand(Gui::Command::Doc,std::string("Gui.activeView().view"+newDocView+"()").c_str());
        } else {
            Base::Console().Warning(std::string("Invalid initial view: "+newDocView+"\nUsing Axonometric instead\n").c_str());
            Gui::Command::doCommand(Gui::Command::Doc,std::string("Gui.activeView().viewAxonometric()").c_str());
        }
    }

}

void DlgSettings3DViewImp::on_mouseButton_clicked()
{
    QDialog dlg(this);
    Ui_MouseButtons ui;
    ui.setupUi(&dlg);

    QVariant data = comboNavigationStyle->itemData(comboNavigationStyle->currentIndex(), Qt::UserRole);
    void* instance = Base::Type::createInstanceByName((const char*)data.toByteArray());
    std::unique_ptr<UserNavigationStyle> ns(static_cast<UserNavigationStyle*>(instance));
    ui.groupBox->setTitle(ui.groupBox->title()+QString::fromLatin1(" ")+comboNavigationStyle->currentText());
    QString descr;
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::SELECTION));
    descr.replace(QLatin1String("\n"), QLatin1String("<p>"));
    ui.selectionLabel->setText(QString::fromLatin1("<b>%1</b>").arg(descr));
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::PANNING));
    descr.replace(QLatin1String("\n"), QLatin1String("<p>"));
    ui.panningLabel->setText(QString::fromLatin1("<b>%1</b>").arg(descr));
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::DRAGGING));
    descr.replace(QLatin1String("\n"), QLatin1String("<p>"));
    ui.rotationLabel->setText(QString::fromLatin1("<b>%1</b>").arg(descr));
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::ZOOMING));
    descr.replace(QLatin1String("\n"), QLatin1String("<p>"));
    ui.zoomingLabel->setText(QString::fromLatin1("<b>%1</b>").arg(descr));
    dlg.exec();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettings3DViewImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        comboAliasing->blockSignals(true);
        int navigation = comboNavigationStyle->currentIndex();
        int orbit = comboOrbitStyle->currentIndex();
        int aliasing = comboAliasing->currentIndex();
        int corner = naviCubeCorner->currentIndex();
        retranslateUi(this);
        retranslate();
        comboNavigationStyle->setCurrentIndex(navigation);
        comboOrbitStyle->setCurrentIndex(orbit);
        comboAliasing->setCurrentIndex(aliasing);
        comboAliasing->blockSignals(false);
        naviCubeCorner->setCurrentIndex(corner);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettings3DViewImp::retranslate()
{
    comboNavigationStyle->clear();

    // add submenu at the end to select navigation style
    std::map<Base::Type, std::string> styles = UserNavigationStyle::getUserFriendlyNames();
    for (std::map<Base::Type, std::string>::iterator it = styles.begin(); it != styles.end(); ++it) {
        QByteArray data(it->first.getName());
        QString name = QApplication::translate(it->first.getName(), it->second.c_str());

        comboNavigationStyle->addItem(name, data);
    }
}

void DlgSettings3DViewImp::onAliasingChanged(int index)
{
    if (index < 0 || !isVisible())
        return;
    // Show this message only once per application session to reduce
    // annoyance when showing it too often.
    if (showMsg) {
        showMsg = false;
        QMessageBox::information(this, tr("Anti-aliasing"),
            tr("Open a new viewer or restart %1 to apply anti-aliasing changes.").arg(qApp->applicationName()));
    }
}

#include "moc_DlgSettings3DViewImp.cpp"

