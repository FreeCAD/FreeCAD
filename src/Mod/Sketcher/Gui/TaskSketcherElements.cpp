/***************************************************************************
 *   Copyright (c) 2014 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
# include <QContextMenuEvent>
# include <QMenu>
# include <QRegExp>
# include <QShortcut>
# include <QString>
#endif

#include "TaskSketcherElements.h"
#include "ui_TaskSketcherElements.h"
#include "EditDatumDialog.h"
#include "ViewProviderSketch.h"

#include <Mod/Sketcher/App/SketchObject.h>

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/BitmapFactory.h>
#include <Gui/ViewProvider.h>
#include <Gui/BitmapFactory.h>
#include <boost/bind.hpp>
#include <Gui/Command.h>

using namespace SketcherGui;
using namespace Gui::TaskView;

/// Inserts a QAction into an existing menu
/// ICONSTR is the string of the icon in the resource file
/// NAMESTR is the text appearing in the contextual menuAction
/// CMDSTR is the string registered in the commandManager
/// FUNC is the name of the member function to be executed on selection of the menu item
/// ACTSONSELECTION is a true/false value to activate the command only if a selection is made
#define CONTEXT_ITEM(ICONSTR,NAMESTR,CMDSTR,FUNC,ACTSONSELECTION) \
QIcon icon_ ## FUNC( Gui::BitmapFactory().pixmap(ICONSTR) ); \
    QAction* constr_ ## FUNC = menu.addAction(icon_ ## FUNC,tr(NAMESTR), this, SLOT(FUNC()), \
        QKeySequence(QString::fromUtf8(Gui::Application::Instance->commandManager().getCommandByName(CMDSTR)->getAccel()))); \
    if(ACTSONSELECTION) constr_ ## FUNC->setEnabled(!items.isEmpty()); else constr_ ## FUNC->setEnabled(true);

/// Defines the member function corresponding to the CONTEXT_ITEM macro
#define CONTEXT_MEMBER_DEF(CMDSTR,FUNC) \
void ElementView::FUNC(){ \
   Gui::Application::Instance->commandManager().runCommandByName(CMDSTR);}

// helper class to store additional information about the listWidget entry.
class ElementItem : public QListWidgetItem
{
public:
    ElementItem(const QIcon & icon, const QString & text, int elementnr,
                int startingVertex, int midVertex, int endVertex,
                Base::Type geometryType)
        : QListWidgetItem(icon,text)
        , ElementNbr(elementnr)
        , StartingVertex(startingVertex)
        , MidVertex(midVertex)
        , EndVertex(endVertex)
        , isLineSelected(false)
        , isStartingPointSelected(false)
        , isEndPointSelected(false)
        , isMidPointSelected(false)
        , GeometryType(geometryType)
    {

    }
    ElementItem(const QString & text,int elementnr,
                int startingVertex, int midVertex, int endVertex,
                Base::Type geometryType)
        : QListWidgetItem(text)
        , ElementNbr(elementnr)
        , StartingVertex(startingVertex)
        , MidVertex(midVertex)
        , EndVertex(endVertex)
        , isLineSelected(false)
        , isStartingPointSelected(false)
        , isEndPointSelected(false)
        , isMidPointSelected(false)
        , GeometryType(geometryType)
    {

    }
    ~ElementItem()
    {
    }

    int ElementNbr;
    int StartingVertex;
    int MidVertex;
    int EndVertex;
    bool isLineSelected;
    bool isStartingPointSelected;
    bool isEndPointSelected;
    bool isMidPointSelected;
    Base::Type GeometryType;
};

ElementView::ElementView(QWidget *parent)
    : QListWidget(parent)
{
}

ElementView::~ElementView()
{
}

void ElementView::contextMenuEvent (QContextMenuEvent* event)
{
    QMenu menu;
    QList<QListWidgetItem *> items = selectedItems();

    // CONTEXT_ITEM(ICONSTR,NAMESTR,FUNC,KEY)
    CONTEXT_ITEM("Constraint_PointOnPoint","Point Coincidence","Sketcher_ConstrainCoincident",doPointCoincidence,true)
    CONTEXT_ITEM("Constraint_PointOnObject","Point on Object","Sketcher_ConstrainPointOnObject",doPointOnObjectConstraint,true)
    CONTEXT_ITEM("Constraint_Vertical","Vertical Constraint","Sketcher_ConstrainVertical", doVerticalConstraint,true)
    CONTEXT_ITEM("Constraint_Horizontal","Horizontal Constraint","Sketcher_ConstrainHorizontal",doHorizontalConstraint,true)
    CONTEXT_ITEM("Constraint_Parallel","Parallel Constraint","Sketcher_ConstrainParallel",doParallelConstraint,true)
    CONTEXT_ITEM("Constraint_Perpendicular","Perpendicular Constraint","Sketcher_ConstrainPerpendicular",doPerpendicularConstraint,true)
    CONTEXT_ITEM("Constraint_Tangent","Tangent Constraint","Sketcher_ConstrainTangent",doTangentConstraint,true)
    CONTEXT_ITEM("Constraint_EqualLength","Equal Length","Sketcher_ConstrainEqual",doEqualConstraint,true)
    CONTEXT_ITEM("Constraint_Symmetric","Symmetric","Sketcher_ConstrainSymmetric",doSymmetricConstraint,true)
    CONTEXT_ITEM("Sketcher_ConstrainLock","Lock Constraint","Sketcher_ConstrainLock",doLockConstraint,true)
    CONTEXT_ITEM("Constraint_HorizontalDistance","Horizontal Distance","Sketcher_ConstrainDistanceX",doHorizontalDistance,true)
    CONTEXT_ITEM("Constraint_VerticalDistance","Vertical Distance","Sketcher_ConstrainDistanceY",doVerticalDistance,true)
    CONTEXT_ITEM("Constraint_Length","Length Constraint","Sketcher_ConstrainDistance",doLengthConstraint,true)
    CONTEXT_ITEM("Constraint_Radius","Radius Constraint","Sketcher_ConstrainRadius",doRadiusConstraint,true)
    CONTEXT_ITEM("Constraint_Diameter","Diameter Constraint","Sketcher_ConstrainDiameter",doDiameterConstraint,true)
    CONTEXT_ITEM("Constraint_InternalAngle","Angle Constraint","Sketcher_ConstrainAngle",doAngleConstraint,true)

    menu.addSeparator();

    CONTEXT_ITEM("Sketcher_AlterConstruction","Toggle construction line","Sketcher_ToggleConstruction",doToggleConstruction,true)

    menu.addSeparator();

    CONTEXT_ITEM("Sketcher_CloseShape","Close Shape","Sketcher_CloseShape",doCloseShape,true)
    CONTEXT_ITEM("Sketcher_ConnectLines","Connect","Sketcher_ConnectLines",doConnect,true)
    CONTEXT_ITEM("Sketcher_SelectConstraints","Select Constraints","Sketcher_SelectConstraints",doSelectConstraints,true)
    CONTEXT_ITEM("Sketcher_SelectOrigin","Select Origin","Sketcher_SelectOrigin",doSelectOrigin,false)
    CONTEXT_ITEM("Sketcher_SelectHorizontalAxis","Select Horizontal Axis","Sketcher_SelectHorizontalAxis",doSelectHAxis,false)
    CONTEXT_ITEM("Sketcher_SelectVerticalAxis","Select Vertical Axis","Sketcher_SelectVerticalAxis",doSelectVAxis,false)

    menu.addSeparator();

    QAction* remove = menu.addAction(tr("Delete"), this, SLOT(deleteSelectedItems()),
        QKeySequence(QKeySequence::Delete));
    remove->setEnabled(!items.isEmpty());

    menu.menuAction()->setIconVisibleInMenu(true);

    menu.exec(event->globalPos());
}

CONTEXT_MEMBER_DEF("Sketcher_ConstrainDistanceX",doHorizontalDistance)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainDistanceY",doVerticalDistance)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainHorizontal",doHorizontalConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainVertical",doVerticalConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainLock",doLockConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainCoincident",doPointCoincidence)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainParallel",doParallelConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainPerpendicular",doPerpendicularConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainDistance",doLengthConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainRadius",doRadiusConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainDiameter",doDiameterConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainAngle",doAngleConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainEqual",doEqualConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainPointOnObject",doPointOnObjectConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainSymmetric",doSymmetricConstraint)
CONTEXT_MEMBER_DEF("Sketcher_ConstrainTangent",doTangentConstraint)
CONTEXT_MEMBER_DEF("Sketcher_CloseShape",doCloseShape)
CONTEXT_MEMBER_DEF("Sketcher_ConnectLines",doConnect)
CONTEXT_MEMBER_DEF("Sketcher_ToggleConstruction",doToggleConstruction)
CONTEXT_MEMBER_DEF("Sketcher_SelectConstraints",doSelectConstraints)
CONTEXT_MEMBER_DEF("Sketcher_SelectOrigin",doSelectOrigin)
CONTEXT_MEMBER_DEF("Sketcher_SelectHorizontalAxis",doSelectHAxis)
CONTEXT_MEMBER_DEF("Sketcher_SelectVerticalAxis",doSelectVAxis)

void ElementView::deleteSelectedItems()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) return;

    doc->openTransaction("Delete");
    std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx(doc->getName());
    for (std::vector<Gui::SelectionObject>::iterator ft = sel.begin(); ft != sel.end(); ++ft) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(ft->getObject());
        if (vp) {
            vp->onDelete(ft->getSubNames());
        }
    }
    doc->commitTransaction();
}


void ElementView::keyPressEvent(QKeyEvent * event)
{
    switch (event->key())
    {
      case Qt::Key_Z:
        // signal
        onFilterShortcutPressed();
        break;
      default:
        QListWidget::keyPressEvent( event );
        break;
    }
}

// ----------------------------------------------------------------------------

/* TRANSLATOR SketcherGui::TaskSketcherElements */

TaskSketcherElements::TaskSketcherElements(ViewProviderSketch *sketchView)
    : TaskBox(Gui::BitmapFactory().pixmap("document-new"),tr("Elements"),true, 0)
    , sketchView(sketchView)
    , ui(new Ui_TaskSketcherElements())
    , focusItemIndex(-1)
    , previouslySelectedItemIndex(-1)
    , isNamingBoxChecked(false)
    , isautoSwitchBoxChecked(false)
    , inhibitSelectionUpdate(false)
{
    // we need a separate container widget to add all controls to
    proxy = new QWidget(this);
    ui->setupUi(proxy);
#ifdef Q_OS_MAC
    QString cmdKey = QString::fromUtf8("\xe2\x8c\x98"); // U+2318
#else
    // translate the text (it's offered by Qt's translation files)
    // but avoid being picked up by lupdate
    const char* ctrlKey = "Ctrl";
    QString cmdKey = QShortcut::tr(ctrlKey);
#endif
    QString zKey = QString::fromLatin1("Z");
    ui->Explanation->setText(tr("<html><head/><body><p>&quot;%1&quot;: multiple selection</p>"
                                "<p>&quot;%2&quot;: switch to next valid type</p></body></html>")
                             .arg(cmdKey).arg(zKey));
    ui->listWidgetElements->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->listWidgetElements->setEditTriggers(QListWidget::NoEditTriggers);
    ui->listWidgetElements->setMouseTracking(true);

    // connecting the needed signals
    QObject::connect(
        ui->listWidgetElements, SIGNAL(itemSelectionChanged()),
        this                     , SLOT  (on_listWidgetElements_itemSelectionChanged())
       );
    QObject::connect(
        ui->listWidgetElements, SIGNAL(itemEntered(QListWidgetItem *)),
        this                     , SLOT  (on_listWidgetElements_itemEntered(QListWidgetItem *))
       );
    QObject::connect(
        ui->listWidgetElements, SIGNAL(onFilterShortcutPressed()),
        this                     , SLOT  (on_listWidgetElements_filterShortcutPressed())
       );
    QObject::connect(
        ui->comboBoxElementFilter, SIGNAL(currentIndexChanged(int)),
        this                     , SLOT  (on_listWidgetElements_currentFilterChanged(int))
       );
    QObject::connect(
        ui->namingBox, SIGNAL(stateChanged(int)),
        this                     , SLOT  (on_namingBox_stateChanged(int))
       );
    QObject::connect(
        ui->autoSwitchBox, SIGNAL(stateChanged(int)),
        this                     , SLOT  (on_autoSwitchBox_stateChanged(int))
       );

    connectionElementsChanged = sketchView->signalElementsChanged.connect(
        boost::bind(&SketcherGui::TaskSketcherElements::slotElementsChanged, this));

    this->groupLayout()->addWidget(proxy);

    ui->comboBoxElementFilter->setCurrentIndex(0);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/Elements");

    ui->autoSwitchBox->setChecked(hGrp->GetBool("Auto-switch to edge", true));
    ui->namingBox->setChecked(hGrp->GetBool("Extended Naming", false));

    ui->comboBoxElementFilter->setEnabled(!isautoSwitchBoxChecked);

    slotElementsChanged();
}

TaskSketcherElements::~TaskSketcherElements()
{
    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/Elements");
        hGrp->SetBool("Auto-switch to edge", ui->autoSwitchBox->isChecked());
        hGrp->SetBool("Extended Naming", ui->namingBox->isChecked());
    }
    catch (const Base::Exception&) {
    }

    connectionElementsChanged.disconnect();
    delete ui;
}

void TaskSketcherElements::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    std::string temp;
    if (msg.Type == Gui::SelectionChanges::ClrSelection) {
        clearWidget();
    }
    else if (msg.Type == Gui::SelectionChanges::AddSelection ||
             msg.Type == Gui::SelectionChanges::RmvSelection) {
        bool select = (msg.Type == Gui::SelectionChanges::AddSelection);
        // is it this object??
        if (strcmp(msg.pDocName,sketchView->getSketchObject()->getDocument()->getName())==0 &&
            strcmp(msg.pObjectName,sketchView->getSketchObject()->getNameInDocument())== 0) {
            if (msg.pSubName) {
                QString expr = QString::fromLatin1(msg.pSubName);
                std::string shapetype(msg.pSubName);
                // if-else edge vertex
                if (shapetype.size() > 4 && shapetype.substr(0,4) == "Edge") {
                    QRegExp rx(QString::fromLatin1("^Edge(\\d+)$"));
                    int pos = expr.indexOf(rx);
                    if (pos > -1) {
                        bool ok;
                        int ElementId = rx.cap(1).toInt(&ok) - 1;
                        if (ok) {
                            int countItems = ui->listWidgetElements->count();
                            for (int i=0; i < countItems; i++) {
                                ElementItem* item = static_cast<ElementItem*>
                                  (ui->listWidgetElements->item(i));
                                if (item->ElementNbr == ElementId) {
                                    item->isLineSelected=select;
                                    break;
                                }
                            }
                        }
                    }
                }
                else if (shapetype.size() > 6 && shapetype.substr(0,6) == "Vertex"){
                    QRegExp rx(QString::fromLatin1("^Vertex(\\d+)$"));
                    int pos = expr.indexOf(rx);
                    if (pos > -1) {
                        bool ok;
                        int ElementId = rx.cap(1).toInt(&ok) - 1;
                        if (ok) {
                            // Get the GeoID&Pos
                            int GeoId;
                            Sketcher::PointPos PosId;
                            sketchView->getSketchObject()->getGeoVertexIndex(ElementId,GeoId, PosId);

                            int countItems = ui->listWidgetElements->count();
                            for (int i=0; i < countItems; i++) {
                                ElementItem* item = static_cast<ElementItem*>
                                  (ui->listWidgetElements->item(i));
                                if (item->ElementNbr == GeoId) {
                                    switch(PosId)
                                    {
                                    case Sketcher::start:
                                        item->isStartingPointSelected=select;
                                        break;
                                    case Sketcher::end:
                                        item->isEndPointSelected=select;
                                        break;
                                    case Sketcher::mid:
                                        item->isMidPointSelected=select;
                                        break;
                                    default:
                                        break;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                // update the listwidget
                int element=ui->comboBoxElementFilter->currentIndex();
                ui->listWidgetElements->blockSignals(true);


                for (int i=0;i<ui->listWidgetElements->count(); i++) {
                    ElementItem * ite=static_cast<ElementItem*>(ui->listWidgetElements->item(i));

                    switch(element){
                      case 0:
                          ite->setSelected(ite->isLineSelected);
                          break;
                      case 1:
                          ite->setSelected(ite->isStartingPointSelected);
                          break;
                      case 2:
                          ite->setSelected(ite->isEndPointSelected);
                          break;
                      case 3:
                          ite->setSelected(ite->isMidPointSelected);
                          break;
                    }
                }

                ui->listWidgetElements->blockSignals(false);

            }
        }
    }
    else if (msg.Type == Gui::SelectionChanges::SetSelection) {
        // do nothing here
    }
}


void TaskSketcherElements::on_listWidgetElements_itemSelectionChanged(void)
{
    ui->listWidgetElements->blockSignals(true);


    // selection changed because we acted on the current entered item
    // we can not do this with ItemPressed because that signal is triggered after this one
    int element=ui->comboBoxElementFilter->currentIndex();

    ElementItem * itf;

    if(focusItemIndex>-1 && focusItemIndex<ui->listWidgetElements->count())
      itf=static_cast<ElementItem*>(ui->listWidgetElements->item(focusItemIndex));
    else
      itf=NULL;

    bool multipleselection=true; // ctrl type of selection in listWidget
    bool multipleconsecutiveselection=false; // shift type of selection in listWidget

    if (!inhibitSelectionUpdate) {
        if(itf!=NULL) {
            switch(element){
            case 0:
                itf->isLineSelected=!itf->isLineSelected;
                break;
            case 1:
                itf->isStartingPointSelected=!itf->isStartingPointSelected;
                break;
            case 2:
                itf->isEndPointSelected=!itf->isEndPointSelected;
                break;
            case 3:
                itf->isMidPointSelected=!itf->isMidPointSelected;
                break;
            }
        }

        if (QApplication::keyboardModifiers()==Qt::ControlModifier)// multiple ctrl selection?
            multipleselection=true;
        else
            multipleselection=false;

        if (QApplication::keyboardModifiers()==Qt::ShiftModifier)// multiple shift selection?
            multipleconsecutiveselection=true;
        else
            multipleconsecutiveselection=false;

        if (multipleselection && multipleconsecutiveselection) { // ctrl takes priority over shift functionality
            multipleselection=true;
            multipleconsecutiveselection=false;
        }
    }

    std::string doc_name = sketchView->getSketchObject()->getDocument()->getName();
    std::string obj_name = sketchView->getSketchObject()->getNameInDocument();

    bool block = this->blockConnection(true); // avoid to be notified by itself
    Gui::Selection().clearSelection();

    for (int i=0;i<ui->listWidgetElements->count(); i++) {
        ElementItem * ite=static_cast<ElementItem*>(ui->listWidgetElements->item(i));

        if(multipleselection==false && multipleconsecutiveselection==false && ite!=itf) {
            ite->isLineSelected=false;
            ite->isStartingPointSelected=false;
            ite->isEndPointSelected=false;
            ite->isMidPointSelected=false;
        }

        if( multipleconsecutiveselection) {
            if ((( i>focusItemIndex && i<previouslySelectedItemIndex ) ||
                 ( i<focusItemIndex && i>previouslySelectedItemIndex )) &&
                previouslySelectedItemIndex>=0){
              // select the element of the Item
                      switch(element){
                  case 0:
                      ite->isLineSelected=true;
                      break;
                  case 1:
                      ite->isStartingPointSelected=true;
                      break;
                  case 2:
                      ite->isEndPointSelected=true;
                      break;
                  case 3:
                      ite->isMidPointSelected=true;
                      break;
                }
            }
        }

        // first update the listwidget
        switch(element){
          case 0:
              ite->setSelected(ite->isLineSelected);
              break;
          case 1:
              ite->setSelected(ite->isStartingPointSelected);
              break;
          case 2:
              ite->setSelected(ite->isEndPointSelected);
              break;
          case 3:
              ite->setSelected(ite->isMidPointSelected);
              break;
        }

        // now the scene
        std::stringstream ss;
        int vertex;

        if (ite->isLineSelected) {
            ss << "Edge" << ite->ElementNbr + 1;
            Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
        }

        if (ite->isStartingPointSelected) {
            ss.str(std::string());
            vertex= ite->StartingVertex;
            if (vertex!=-1) {
                ss << "Vertex" << vertex + 1;
                Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
            }
        }

        if (ite->isEndPointSelected) {
            ss.str(std::string());
            vertex= ite->EndVertex;
            if (vertex!=-1) {
                ss << "Vertex" << vertex + 1;
                Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
            }
        }

        if (ite->isMidPointSelected) {
            ss.str(std::string());
            vertex= ite->MidVertex;
            if (vertex!=-1) {
                ss << "Vertex" << vertex + 1;
                Gui::Selection().addSelection(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
            }
        }
    }

    this->blockConnection(block);
    ui->listWidgetElements->blockSignals(false);

    if (focusItemIndex>-1 && focusItemIndex<ui->listWidgetElements->count())
        previouslySelectedItemIndex=focusItemIndex;
}

void TaskSketcherElements::on_listWidgetElements_itemEntered(QListWidgetItem *item)
{
    ElementItem *it = dynamic_cast<ElementItem*>(item);
    if (!it) return;

    Gui::Selection().rmvPreselect();

    ui->listWidgetElements->setFocus();

    int tempitemindex=ui->listWidgetElements->row(item);

    std::string doc_name = sketchView->getSketchObject()->getDocument()->getName();
    std::string obj_name = sketchView->getSketchObject()->getNameInDocument();

    /* 0 - Lines
     * 1 - Starting Points
     * 2 - End Points
     * 3 - Middle Points
     */
    std::stringstream ss;


    // Edge Auto-Switch functionality
    if (isautoSwitchBoxChecked && tempitemindex!=focusItemIndex){
        ui->listWidgetElements->blockSignals(true);
        if (it->GeometryType==Part::GeomPoint::getClassTypeId()) {
            ui->comboBoxElementFilter->setCurrentIndex(1);
        }
        else {
            ui->comboBoxElementFilter->setCurrentIndex(0);
        }
        ui->listWidgetElements->blockSignals(false);
    }

    int element=ui->comboBoxElementFilter->currentIndex();

    focusItemIndex=tempitemindex;

    int vertex;

    switch(element)
    {
    case 0:
        {
            ss << "Edge" << it->ElementNbr + 1;
            Gui::Selection().setPreselect(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
        }
        break;
    case 1:
    case 2:
    case 3:
        vertex= sketchView->getSketchObject()->getVertexIndexGeoPos(it->ElementNbr,static_cast<Sketcher::PointPos>(element));
        if (vertex!=-1) {
            ss << "Vertex" << vertex + 1;
            Gui::Selection().setPreselect(doc_name.c_str(), obj_name.c_str(), ss.str().c_str());
        }
        break;
    }
}

void TaskSketcherElements::leaveEvent (QEvent * event)
{
    Q_UNUSED(event);
    Gui::Selection().rmvPreselect();
    ui->listWidgetElements->clearFocus();
}

void TaskSketcherElements::slotElementsChanged(void)
{
    QIcon Sketcher_Element_Arc_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Arc_Edge") );
    QIcon Sketcher_Element_Arc_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Arc_EndPoint") );
    QIcon Sketcher_Element_Arc_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Arc_MidPoint") );
    QIcon Sketcher_Element_Arc_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Arc_StartingPoint") );
    QIcon Sketcher_Element_Circle_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Circle_Edge") );
    QIcon Sketcher_Element_Circle_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Circle_MidPoint") );
    QIcon Sketcher_Element_Line_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Line_Edge") );
    QIcon Sketcher_Element_Line_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Line_EndPoint") );
    QIcon Sketcher_Element_Line_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Line_StartingPoint") );
    QIcon Sketcher_Element_Point_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Point_StartingPoint") );
    QIcon Sketcher_Element_Ellipse_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Ellipse_Edge_2") );
    QIcon Sketcher_Element_Ellipse_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Ellipse_CentrePoint") );
    QIcon Sketcher_Element_ArcOfEllipse_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Elliptical_Arc_Edge") );
    QIcon Sketcher_Element_ArcOfEllipse_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Elliptical_Arc_Centre_Point") );
    QIcon Sketcher_Element_ArcOfEllipse_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Elliptical_Arc_Start_Point") );
    QIcon Sketcher_Element_ArcOfEllipse_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Elliptical_Arc_End_Point") );
    QIcon Sketcher_Element_ArcOfHyperbola_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Hyperbolic_Arc_Edge") );
    QIcon Sketcher_Element_ArcOfHyperbola_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Hyperbolic_Arc_Centre_Point") );
    QIcon Sketcher_Element_ArcOfHyperbola_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Hyperbolic_Arc_Start_Point") );
    QIcon Sketcher_Element_ArcOfHyperbola_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Hyperbolic_Arc_End_Point") );
    QIcon Sketcher_Element_ArcOfParabola_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Parabolic_Arc_Edge") );
    QIcon Sketcher_Element_ArcOfParabola_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Parabolic_Arc_Centre_Point") );
    QIcon Sketcher_Element_ArcOfParabola_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Parabolic_Arc_Start_Point") );
    QIcon Sketcher_Element_ArcOfParabola_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Parabolic_Arc_End_Point") );
    QIcon Sketcher_Element_BSpline_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_BSpline_Edge") );
    QIcon Sketcher_Element_BSpline_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_BSpline_StartPoint") );
    QIcon Sketcher_Element_BSpline_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_BSpline_EndPoint") );
    QIcon none( Gui::BitmapFactory().pixmap("Sketcher_Element_SelectionTypeInvalid") );

    assert(sketchView);
    // Build up ListView with the elements
    const std::vector< Part::Geometry * > &vals = sketchView->getSketchObject()->Geometry.getValues();

    ui->listWidgetElements->clear();

    int element = ui->comboBoxElementFilter->currentIndex();

    int i=1;
    for(std::vector< Part::Geometry * >::const_iterator it= vals.begin();it!=vals.end();++it,++i){
      Base::Type type = (*it)->getTypeId();
      bool construction = (*it)->Construction;

      ui->listWidgetElements->addItem(new ElementItem(
        (type == Part::GeomPoint::getClassTypeId()          && element==1) ? Sketcher_Element_Point_StartingPoint :
        (type == Part::GeomLineSegment::getClassTypeId()    && element==0) ? Sketcher_Element_Line_Edge :
        (type == Part::GeomLineSegment::getClassTypeId()    && element==1) ? Sketcher_Element_Line_StartingPoint :
        (type == Part::GeomLineSegment::getClassTypeId()    && element==2) ? Sketcher_Element_Line_EndPoint :
        (type == Part::GeomArcOfCircle::getClassTypeId()    && element==0) ? Sketcher_Element_Arc_Edge :
        (type == Part::GeomArcOfCircle::getClassTypeId()    && element==1) ? Sketcher_Element_Arc_StartingPoint :
        (type == Part::GeomArcOfCircle::getClassTypeId()    && element==2) ? Sketcher_Element_Arc_EndPoint :
        (type == Part::GeomArcOfCircle::getClassTypeId()    && element==3) ? Sketcher_Element_Arc_MidPoint :
        (type == Part::GeomCircle::getClassTypeId()         && element==0) ? Sketcher_Element_Circle_Edge :
        (type == Part::GeomCircle::getClassTypeId()         && element==3) ? Sketcher_Element_Circle_MidPoint :
        (type == Part::GeomEllipse::getClassTypeId()        && element==0) ? Sketcher_Element_Ellipse_Edge :
        (type == Part::GeomEllipse::getClassTypeId()        && element==3) ? Sketcher_Element_Ellipse_MidPoint :
        (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==0) ? Sketcher_Element_ArcOfEllipse_Edge :
        (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==1) ? Sketcher_Element_ArcOfEllipse_StartingPoint :
        (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==2) ? Sketcher_Element_ArcOfEllipse_EndPoint :
        (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==3) ? Sketcher_Element_ArcOfEllipse_MidPoint :
        (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==0) ? Sketcher_Element_ArcOfHyperbola_Edge :
        (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==1) ? Sketcher_Element_ArcOfHyperbola_StartingPoint :
        (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==2) ? Sketcher_Element_ArcOfHyperbola_EndPoint :
        (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==3) ? Sketcher_Element_ArcOfHyperbola_MidPoint :
        (type == Part::GeomArcOfParabola::getClassTypeId()    && element==0) ? Sketcher_Element_ArcOfParabola_Edge :
        (type == Part::GeomArcOfParabola::getClassTypeId()    && element==1) ? Sketcher_Element_ArcOfParabola_StartingPoint :
        (type == Part::GeomArcOfParabola::getClassTypeId()    && element==2) ? Sketcher_Element_ArcOfParabola_EndPoint :
        (type == Part::GeomArcOfParabola::getClassTypeId()    && element==3) ? Sketcher_Element_ArcOfParabola_MidPoint :
        (type == Part::GeomBSplineCurve::getClassTypeId()    && element==0) ? Sketcher_Element_BSpline_Edge :
        (type == Part::GeomBSplineCurve::getClassTypeId()    && element==1) ? Sketcher_Element_BSpline_StartingPoint :
        (type == Part::GeomBSplineCurve::getClassTypeId()    && element==2) ? Sketcher_Element_BSpline_EndPoint :
        none,
        type == Part::GeomPoint::getClassTypeId()           ? ( isNamingBoxChecked ?
                                                                (tr("Point") + QString::fromLatin1("(Edge%1)").arg(i)):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Point")))         :
        type == Part::GeomLineSegment::getClassTypeId()     ? ( isNamingBoxChecked ?
                                                                (tr("Line") + QString::fromLatin1("(Edge%1)").arg(i)) +
                                                                (construction?(QString::fromLatin1("-")+tr("Construction")):QString::fromLatin1("")):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Line")))         :
        type == Part::GeomArcOfCircle::getClassTypeId()     ? ( isNamingBoxChecked ?
                                                                (tr("Arc") + QString::fromLatin1("(Edge%1)").arg(i)) +
                                                                (construction?(QString::fromLatin1("-")+tr("Construction")):QString::fromLatin1("")):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Arc")))         :
        type == Part::GeomCircle::getClassTypeId()          ? ( isNamingBoxChecked ?
                                                                (tr("Circle") + QString::fromLatin1("(Edge%1)").arg(i)) +
                                                                (construction?(QString::fromLatin1("-")+tr("Construction")):QString::fromLatin1("")):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Circle")))         :
        type == Part::GeomEllipse::getClassTypeId()         ? ( isNamingBoxChecked ?
                                                                (tr("Ellipse") + QString::fromLatin1("(Edge%1)").arg(i)) +
                                                                (construction?(QString::fromLatin1("-")+tr("Construction")):QString::fromLatin1("")):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Ellipse")))   :
        type == Part::GeomArcOfEllipse::getClassTypeId()    ? ( isNamingBoxChecked ?
                                                                (tr("Elliptical Arc") + QString::fromLatin1("(Edge%1)").arg(i)) +
                                                                (construction?(QString::fromLatin1("-")+tr("Construction")):QString::fromLatin1("")):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Elliptical Arc")))   :
        type == Part::GeomArcOfHyperbola::getClassTypeId()    ? ( isNamingBoxChecked ?
                                                                (tr("Hyperbolic Arc") + QString::fromLatin1("(Edge%1)").arg(i)) +
                                                                (construction?(QString::fromLatin1("-")+tr("Construction")):QString::fromLatin1("")):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Hyperbolic Arc")))   :
        type == Part::GeomArcOfParabola::getClassTypeId()    ? ( isNamingBoxChecked ?
                                                                (tr("Parabolic Arc") + QString::fromLatin1("(Edge%1)").arg(i)) +
                                                                (construction?(QString::fromLatin1("-")+tr("Construction")):QString::fromLatin1("")):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Parabolic Arc")))   :
        type == Part::GeomBSplineCurve::getClassTypeId()    ? ( isNamingBoxChecked ?
                                                                (tr("BSpline") + QString::fromLatin1("(Edge%1)").arg(i)) +
                                                                (construction?(QString::fromLatin1("-")+tr("Construction")):QString::fromLatin1("")):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("BSpline")))   :
        ( isNamingBoxChecked ?
          (tr("Other") + QString::fromLatin1("(Edge%1)").arg(i)) +
          (construction?(QString::fromLatin1("-")+tr("Construction")):QString::fromLatin1("")):
          (QString::fromLatin1("%1-").arg(i)+tr("Other"))),
        i-1,
        sketchView->getSketchObject()->getVertexIndexGeoPos(i-1,Sketcher::start),
        sketchView->getSketchObject()->getVertexIndexGeoPos(i-1,Sketcher::mid),
        sketchView->getSketchObject()->getVertexIndexGeoPos(i-1,Sketcher::end),
        type));
    }

    const std::vector< Part::Geometry * > &ext_vals = sketchView->getSketchObject()->getExternalGeometry();

    const std::vector<App::DocumentObject*> linkobjs = sketchView->getSketchObject()->ExternalGeometry.getValues();
    const std::vector<std::string> linksubs = sketchView->getSketchObject()->ExternalGeometry.getSubValues();

    int j=1;
    for(std::vector< Part::Geometry * >::const_iterator it= ext_vals.begin();it!=ext_vals.end();++it,++i,++j){
      Base::Type type = (*it)->getTypeId();

      if(j>2) { // we do not want the H and V axes

        QString linkname;

        if(isNamingBoxChecked) {
            if(size_t(j-3) < linkobjs.size() && size_t(j-3) < linksubs.size()) {
                linkname =  QString::fromLatin1("(ExternalEdge%1, ").arg(j-2) +
                            QString::fromUtf8(linkobjs[j-3]->getNameInDocument()) +
                            QString::fromLatin1(".") +
                            QString::fromUtf8(linksubs[j-3].c_str()) +
                            QString::fromLatin1(")");
            }
            else {
                linkname = QString::fromLatin1("(ExternalEdge%1)").arg(j-2);
            }
        }


        ui->listWidgetElements->addItem(new ElementItem(
            (type == Part::GeomPoint::getClassTypeId()         && element==1) ? Sketcher_Element_Point_StartingPoint :
            (type == Part::GeomLineSegment::getClassTypeId()  && element==0) ? Sketcher_Element_Line_Edge :
            (type == Part::GeomLineSegment::getClassTypeId()  && element==1) ? Sketcher_Element_Line_StartingPoint :
            (type == Part::GeomLineSegment::getClassTypeId()  && element==2) ? Sketcher_Element_Line_EndPoint :
            (type == Part::GeomArcOfCircle::getClassTypeId()         && element==0) ? Sketcher_Element_Arc_Edge :
            (type == Part::GeomArcOfCircle::getClassTypeId()         && element==1) ? Sketcher_Element_Arc_StartingPoint :
            (type == Part::GeomArcOfCircle::getClassTypeId()         && element==2) ? Sketcher_Element_Arc_EndPoint :
            (type == Part::GeomArcOfCircle::getClassTypeId()         && element==3) ? Sketcher_Element_Arc_MidPoint :
            (type == Part::GeomCircle::getClassTypeId()        && element==0) ? Sketcher_Element_Circle_Edge :
            (type == Part::GeomCircle::getClassTypeId()        && element==3) ? Sketcher_Element_Circle_MidPoint :
            (type == Part::GeomEllipse::getClassTypeId()        && element==0) ? Sketcher_Element_Ellipse_Edge :
            (type == Part::GeomEllipse::getClassTypeId()        && element==3) ? Sketcher_Element_Ellipse_MidPoint :
            (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==0) ? Sketcher_Element_ArcOfEllipse_Edge :
            (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==1) ? Sketcher_Element_ArcOfEllipse_StartingPoint :
            (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==2) ? Sketcher_Element_ArcOfEllipse_EndPoint :
            (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==3) ? Sketcher_Element_ArcOfEllipse_MidPoint :
            (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==0) ? Sketcher_Element_ArcOfHyperbola_Edge :
            (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==1) ? Sketcher_Element_ArcOfHyperbola_StartingPoint :
            (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==2) ? Sketcher_Element_ArcOfHyperbola_EndPoint :
            (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==3) ? Sketcher_Element_ArcOfHyperbola_MidPoint :
            (type == Part::GeomArcOfParabola::getClassTypeId()    && element==0) ? Sketcher_Element_ArcOfParabola_Edge :
            (type == Part::GeomArcOfParabola::getClassTypeId()    && element==1) ? Sketcher_Element_ArcOfParabola_StartingPoint :
            (type == Part::GeomArcOfParabola::getClassTypeId()    && element==2) ? Sketcher_Element_ArcOfParabola_EndPoint :
            (type == Part::GeomArcOfParabola::getClassTypeId()    && element==3) ? Sketcher_Element_ArcOfParabola_MidPoint :
            (type == Part::GeomBSplineCurve::getClassTypeId()    && element==0) ? Sketcher_Element_BSpline_Edge :
            (type == Part::GeomBSplineCurve::getClassTypeId()    && element==1) ? Sketcher_Element_BSpline_StartingPoint :
            (type == Part::GeomBSplineCurve::getClassTypeId()    && element==2) ? Sketcher_Element_BSpline_EndPoint :
            none,
            type == Part::GeomPoint::getClassTypeId()         ? ( isNamingBoxChecked ?
                                                                (tr("Point") + linkname):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Point")))         :
            type == Part::GeomLineSegment::getClassTypeId()        ? ( isNamingBoxChecked ?
                                                                (tr("Line") + linkname):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Line")))         :
            type == Part::GeomArcOfCircle::getClassTypeId()        ? ( isNamingBoxChecked ?
                                                                (tr("Arc") + linkname):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Arc")))         :
            type == Part::GeomCircle::getClassTypeId()        ? ( isNamingBoxChecked ?
                                                                (tr("Circle") + linkname):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Circle")))         :
            type == Part::GeomEllipse::getClassTypeId()         ? ( isNamingBoxChecked ?
                                                                (tr("Ellipse") + linkname):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Ellipse")))   :
            type == Part::GeomArcOfEllipse::getClassTypeId()    ? ( isNamingBoxChecked ?
                                                                (tr("Elliptical Arc") + linkname):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Elliptical Arc")))   :
            type == Part::GeomArcOfHyperbola::getClassTypeId()    ? ( isNamingBoxChecked ?
                                                                (tr("Hyperbolic Arc") + linkname):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Hyperbolic Arc")))   :
            type == Part::GeomArcOfParabola::getClassTypeId()    ? ( isNamingBoxChecked ?
                                                                (tr("Parabolic Arc") + linkname):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("Parabolic Arc")))   :
            type == Part::GeomBSplineCurve::getClassTypeId()    ? ( isNamingBoxChecked ?
                                                                (tr("BSpline") + linkname):
                                                                (QString::fromLatin1("%1-").arg(i)+tr("BSpline")))   :
            ( isNamingBoxChecked ?
            (tr("Other") + linkname):
            (QString::fromLatin1("%1-").arg(i)+tr("Other"))),
            -j,
            sketchView->getSketchObject()->getVertexIndexGeoPos(-j,Sketcher::start),
            sketchView->getSketchObject()->getVertexIndexGeoPos(-j,Sketcher::mid),
            sketchView->getSketchObject()->getVertexIndexGeoPos(-j,Sketcher::end),
            type));
      }
    }
}


void TaskSketcherElements::on_listWidgetElements_filterShortcutPressed()
{
    int element;

    previouslySelectedItemIndex=-1; // Shift selection on list widget implementation

    // calculate next element type on shift press according to entered/preselected element
    // This is the aka fast-forward functionality
    if(focusItemIndex>-1 && focusItemIndex<ui->listWidgetElements->count()){

      ElementItem * itf=static_cast<ElementItem*>(ui->listWidgetElements->item(focusItemIndex));

      Base::Type type = itf->GeometryType;

      element = ui->comboBoxElementFilter->currentIndex(); // currently selected type index

      switch(element)
      {

        case 0: // Edge
          element =        ( type == Part::GeomCircle::getClassTypeId() || type == Part::GeomEllipse::getClassTypeId() ) ? 3 : 1;
          break;
        case 1: // StartingPoint
          element =        ( type == Part::GeomCircle::getClassTypeId() || type == Part::GeomEllipse::getClassTypeId() ) ? 3 :
                            ( type == Part::GeomPoint::getClassTypeId()  ) ? 1 : 2;
          break;
        case 2: // EndPoint
          element =        ( type == Part::GeomLineSegment::getClassTypeId() ) ? 0 :
                            ( type == Part::GeomPoint::getClassTypeId()  ) ? 1 : 3;
          break;
        case 3: // MidPoint
          element =        ( type == Part::GeomPoint::getClassTypeId()  ) ? 1 : 0;
          break;
        default:
          element = 0;
      }

      ui->comboBoxElementFilter->setCurrentIndex(element);

      Gui::Selection().rmvPreselect();

      on_listWidgetElements_itemEntered(itf);
    }
    else{
      element = (ui->comboBoxElementFilter->currentIndex()+1) %
                ui->comboBoxElementFilter->count();

      ui->comboBoxElementFilter->setCurrentIndex(element);

      Gui::Selection().rmvPreselect();
    }

    //update the icon
    updateIcons(element);

    updatePreselection();
}


void TaskSketcherElements::on_namingBox_stateChanged(int state)
{
      isNamingBoxChecked=(state==Qt::Checked);
      slotElementsChanged();
}

void TaskSketcherElements::on_autoSwitchBox_stateChanged(int state)
{
      isautoSwitchBoxChecked=(state==Qt::Checked);
      ui->comboBoxElementFilter->setCurrentIndex(0);
      ui->comboBoxElementFilter->setEnabled(!isautoSwitchBoxChecked);
}

void TaskSketcherElements::on_listWidgetElements_currentFilterChanged ( int index )
{
    previouslySelectedItemIndex=-1; // Shift selection on list widget implementation

    Gui::Selection().rmvPreselect();

    updateIcons(index);

    updatePreselection();

}

void TaskSketcherElements::updatePreselection()
{
    inhibitSelectionUpdate=true;
    on_listWidgetElements_itemSelectionChanged();
    inhibitSelectionUpdate=false;
}

void TaskSketcherElements::clearWidget()
{
    ui->listWidgetElements->blockSignals(true);
    ui->listWidgetElements->clearSelection ();
    ui->listWidgetElements->blockSignals(false);

    // update widget
    int countItems = ui->listWidgetElements->count();
    for (int i=0; i < countItems; i++) {
      ElementItem* item = static_cast<ElementItem*> (ui->listWidgetElements->item(i));
      item->isLineSelected=false;
      item->isStartingPointSelected=false;
      item->isEndPointSelected=false;
      item->isMidPointSelected=false;
    }
}

void TaskSketcherElements::updateIcons(int element)
{
    QIcon Sketcher_Element_Arc_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Arc_Edge") );
    QIcon Sketcher_Element_Arc_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Arc_EndPoint") );
    QIcon Sketcher_Element_Arc_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Arc_MidPoint") );
    QIcon Sketcher_Element_Arc_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Arc_StartingPoint") );
    QIcon Sketcher_Element_Circle_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Circle_Edge") );
    QIcon Sketcher_Element_Circle_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Circle_MidPoint") );
    QIcon Sketcher_Element_Line_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Line_Edge") );
    QIcon Sketcher_Element_Line_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Line_EndPoint") );
    QIcon Sketcher_Element_Line_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Line_StartingPoint") );
    QIcon Sketcher_Element_Point_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Point_StartingPoint") );
    QIcon Sketcher_Element_Ellipse_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Ellipse_Edge_2") );
    QIcon Sketcher_Element_Ellipse_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Ellipse_CentrePoint") );
    QIcon Sketcher_Element_ArcOfEllipse_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Elliptical_Arc_Edge") );
    QIcon Sketcher_Element_ArcOfEllipse_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Elliptical_Arc_Centre_Point") );
    QIcon Sketcher_Element_ArcOfEllipse_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Elliptical_Arc_Start_Point") );
    QIcon Sketcher_Element_ArcOfEllipse_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Elliptical_Arc_End_Point") );
    QIcon Sketcher_Element_ArcOfHyperbola_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Hyperbolic_Arc_Edge") );
    QIcon Sketcher_Element_ArcOfHyperbola_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Hyperbolic_Arc_Centre_Point") );
    QIcon Sketcher_Element_ArcOfHyperbola_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Hyperbolic_Arc_Start_Point") );
    QIcon Sketcher_Element_ArcOfHyperbola_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Hyperbolic_Arc_End_Point") );
    QIcon Sketcher_Element_ArcOfParabola_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_Parabolic_Arc_Edge") );
    QIcon Sketcher_Element_ArcOfParabola_MidPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Parabolic_Arc_Centre_Point") );
    QIcon Sketcher_Element_ArcOfParabola_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Parabolic_Arc_Start_Point") );
    QIcon Sketcher_Element_ArcOfParabola_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_Parabolic_Arc_End_Point") );
    QIcon Sketcher_Element_BSpline_Edge( Gui::BitmapFactory().pixmap("Sketcher_Element_BSpline_Edge") );
    QIcon Sketcher_Element_BSpline_StartingPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_BSpline_StartPoint") );
    QIcon Sketcher_Element_BSpline_EndPoint( Gui::BitmapFactory().pixmap("Sketcher_Element_BSpline_EndPoint") );
    QIcon none( Gui::BitmapFactory().pixmap("Sketcher_Element_SelectionTypeInvalid") );

    for (int i=0;i<ui->listWidgetElements->count(); i++) {
      Base::Type type = static_cast<ElementItem *>(ui->listWidgetElements->item(i))->GeometryType;

      ui->listWidgetElements->item(i)->setIcon(
        (type == Part::GeomPoint::getClassTypeId()          && element==1) ? Sketcher_Element_Point_StartingPoint :
        (type == Part::GeomLineSegment::getClassTypeId()    && element==0) ? Sketcher_Element_Line_Edge :
        (type == Part::GeomLineSegment::getClassTypeId()    && element==1) ? Sketcher_Element_Line_StartingPoint :
        (type == Part::GeomLineSegment::getClassTypeId()    && element==2) ? Sketcher_Element_Line_EndPoint :
        (type == Part::GeomArcOfCircle::getClassTypeId()    && element==0) ? Sketcher_Element_Arc_Edge :
        (type == Part::GeomArcOfCircle::getClassTypeId()    && element==1) ? Sketcher_Element_Arc_StartingPoint :
        (type == Part::GeomArcOfCircle::getClassTypeId()    && element==2) ? Sketcher_Element_Arc_EndPoint :
        (type == Part::GeomArcOfCircle::getClassTypeId()    && element==3) ? Sketcher_Element_Arc_MidPoint :
        (type == Part::GeomCircle::getClassTypeId()         && element==0) ? Sketcher_Element_Circle_Edge :
        (type == Part::GeomCircle::getClassTypeId()         && element==3) ? Sketcher_Element_Circle_MidPoint :
        (type == Part::GeomEllipse::getClassTypeId()        && element==0) ? Sketcher_Element_Ellipse_Edge :
        (type == Part::GeomEllipse::getClassTypeId()        && element==3) ? Sketcher_Element_Ellipse_MidPoint :
        (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==0) ? Sketcher_Element_ArcOfEllipse_Edge :
        (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==1) ? Sketcher_Element_ArcOfEllipse_StartingPoint :
        (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==2) ? Sketcher_Element_ArcOfEllipse_EndPoint :
        (type == Part::GeomArcOfEllipse::getClassTypeId()    && element==3) ? Sketcher_Element_ArcOfEllipse_MidPoint :
        (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==0) ? Sketcher_Element_ArcOfHyperbola_Edge :
        (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==1) ? Sketcher_Element_ArcOfHyperbola_StartingPoint :
        (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==2) ? Sketcher_Element_ArcOfHyperbola_EndPoint :
        (type == Part::GeomArcOfHyperbola::getClassTypeId()    && element==3) ? Sketcher_Element_ArcOfHyperbola_MidPoint :
        (type == Part::GeomArcOfParabola::getClassTypeId()    && element==0) ? Sketcher_Element_ArcOfParabola_Edge :
        (type == Part::GeomArcOfParabola::getClassTypeId()    && element==1) ? Sketcher_Element_ArcOfParabola_StartingPoint :
        (type == Part::GeomArcOfParabola::getClassTypeId()    && element==2) ? Sketcher_Element_ArcOfParabola_EndPoint :
        (type == Part::GeomArcOfParabola::getClassTypeId()    && element==3) ? Sketcher_Element_ArcOfParabola_MidPoint :
        (type == Part::GeomBSplineCurve::getClassTypeId()    && element==0) ? Sketcher_Element_BSpline_Edge :
        (type == Part::GeomBSplineCurve::getClassTypeId()    && element==1) ? Sketcher_Element_BSpline_StartingPoint :
        (type == Part::GeomBSplineCurve::getClassTypeId()    && element==2) ? Sketcher_Element_BSpline_EndPoint :
        none);
    }
}

void TaskSketcherElements::changeEvent(QEvent *e)
{
    TaskBox::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(proxy);
    }
}



#include "moc_TaskSketcherElements.cpp"
