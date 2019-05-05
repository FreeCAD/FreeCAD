/***************************************************************************
 *   Copyright (c) 2013 Thomas Anderson <blobfish[at]gmx.com>              *
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
#ifndef _PreCpmp_
# include <QButtonGroup>
# include <QPushButton>
# include <sstream>
# include <Python.h>

# include <TopoDS_Shape.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRep_Tool.hxx>
# include <TopExp.hxx>
# include <Geom_ElementarySurface.hxx>
# include <Geom_CylindricalSurface.hxx>
# include <Geom_SphericalSurface.hxx>
# include <Geom_Line.hxx>
# include <GeomAPI_ProjectPointOnCurve.hxx>
# include <GeomAPI_ExtremaCurveCurve.hxx>

# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoMatrixTransform.h>
# include <Inventor/nodes/SoVertexProperty.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodekits/SoShapeKit.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoCone.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoNurbsCurve.h>
# include <Inventor/engines/SoComposeVec3f.h>
# include <Inventor/engines/SoCalculator.h>
# include <Inventor/nodes/SoResetTransform.h>
# include <Inventor/engines/SoConcatenate.h>
# include <Inventor/engines/SoComposeRotationFromTo.h>
# include <Inventor/engines/SoComposeRotation.h>
# include <Inventor/nodes/SoMaterial.h>
#endif

#include <Base/Console.h>
#include <Base/UnitsApi.h>
#include "../App/PartFeature.h"
#include <Gui/Application.h>
#include <Gui/Selection.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>

#include "TaskDimension.h"

static bool _MeasureInfoInited;

static void slotDeleteDocument(const App::Document &doc);

struct MeasureInfo {
    PartGui::DimSelections sel1;
    PartGui::DimSelections sel2;
    bool linear;
    MeasureInfo(const PartGui::DimSelections &sel1, const PartGui::DimSelections &sel2, bool linear)
        :sel1(sel1),sel2(sel2),linear(linear)
    {
        if(!_MeasureInfoInited) {
            _MeasureInfoInited = true;
            App::GetApplication().signalDeleteDocument.connect(boost::bind(slotDeleteDocument, _1));
        }
    }
};
static std::map<std::string, std::list<MeasureInfo> > _Measures;

static void slotDeleteDocument(const App::Document &doc) {
    _Measures.erase(doc.getName());
}

bool PartGui::getShapeFromStrings(TopoDS_Shape &shapeOut, const std::string &doc, const std::string &object, const std::string &sub, Base::Matrix4D *mat)
{
  App::Document *docPointer = App::GetApplication().getDocument(doc.c_str());
  if (!docPointer)
    return false;
  App::DocumentObject *objectPointer = docPointer->getObject(object.c_str());
  if (!objectPointer)
    return false;
  shapeOut = Part::Feature::getShape(objectPointer,sub.c_str(),true,mat);
  if (shapeOut.IsNull())
    return false;
  return true;
}

bool PartGui::evaluateLinearPreSelection(TopoDS_Shape &shape1, TopoDS_Shape &shape2)
{
  std::vector<Gui::SelectionSingleton::SelObj> selections = Gui::Selection().getSelection(0,false);
  if (selections.size() != 2)
    return false;
  std::vector<Gui::SelectionSingleton::SelObj>::iterator it;
  std::vector<TopoDS_Shape> shapes;
  DimSelections sels[2];
  
  int i=0;
  for (it = selections.begin(); it != selections.end(); ++it)
  {
    TopoDS_Shape shape = Part::Feature::getShape(it->pObject,it->SubName,true);
    if (shape.IsNull())
      break;
    shapes.push_back(shape);
    sels[i].selections.push_back(DimSelections::DimSelection());
    auto &sel = sels[i].selections[0];
    ++i;
    sel.documentName = it->DocName;
    sel.objectName = it->FeatName;
    sel.subObjectName = it->SubName;
  }

  if (shapes.size() != 2)
    return false;

  shape1 = shapes.front();
  shape2 = shapes.back();
  
  auto doc = App::GetApplication().getActiveDocument();
  if(doc) 
    _Measures[doc->getName()].emplace_back(sels[0],sels[1],true);
  return true;
}

void PartGui::goDimensionLinearRoot()
{
  PartGui::ensureSomeDimensionVisible();

  TopoDS_Shape shape1, shape2;
  if(evaluateLinearPreSelection(shape1, shape2))
  {
    goDimensionLinearNoTask(shape1, shape2);
    Gui::Selection().clearSelection();
  }
  else
  {
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg)
    {
      Gui::Selection().clearSelection();
      dlg = new PartGui::TaskMeasureLinear();
    }
    Gui::Control().showDialog(dlg);
  }
}

void PartGui::goDimensionLinearNoTask(const TopoDS_Shape &shape1, const TopoDS_Shape &shape2)
{
    //Warning: BRepExtrema_DistShapeShape solution array is NOT 0 based.
  BRepExtrema_DistShapeShape measure(shape1, shape2);
  if (!measure.IsDone() || measure.NbSolution() < 1)
    return;

  dumpLinearResults(measure);
  addLinearDimensions(measure);

  //if we ever make this a class add viewer to member.
  Gui::View3DInventorViewer *viewer = getViewer();
  if (!viewer)
    return;
}

void PartGui::dumpLinearResults(const BRepExtrema_DistShapeShape &measure)
{
  std::ostringstream out;
  //switch to initializer list when switch to c++11
  std::vector<std::string> typeNames;
  typeNames.resize(3);
  typeNames[0] = "Vertex";
  typeNames[1] = "Edge";
  typeNames[2] = "Face";

  Base::Quantity quantity(measure.Value(), Base::Unit::Length);
  out << std::endl<< std::setprecision(std::numeric_limits<double>::digits10 + 1) << "distance = " <<
    measure.Value() << "mm    unit distance = " << quantity.getUserString().toUtf8().constData() << std::endl <<
    "solution count: " << measure.NbSolution() << std::endl;

  for (int index = 1; index < measure.NbSolution() + 1; ++index) //not zero based.
  {
    gp_Pnt point1 = measure.PointOnShape1(index);
    gp_Pnt point2 = measure.PointOnShape2(index);
    out << "   solution " << index << ":" << std::endl << std::setprecision(std::numeric_limits<double>::digits10 + 1) <<
           "      point1 " << point1.X() << "   " << point1.Y() << "   " << point1.Z() << std::endl <<
           "      point2 " << point2.X() << "   " << point2.Y() << "   " << point2.Z() << std::endl <<
           "      DeltaX " << fabs(point2.X() - point1.X()) << std::endl <<
           "      DeltaY " << fabs(point2.Y() - point1.Y()) << std::endl <<
           "      DeltaZ " << fabs(point2.Z() - point1.Z()) << std::endl <<
           "      shape type on object1 is: " << typeNames.at(measure.SupportTypeShape1(index)) << std::endl <<
           "      shape type on object2 is: " << typeNames.at(measure.SupportTypeShape2(index)) << std::endl;
  }
  out << std::endl;
  Base::Console().Message(out.str().c_str());
}

Gui::View3DInventorViewer * PartGui::getViewer()
{
  Gui::Document *doc = Gui::Application::Instance->activeDocument();
  if (!doc)
    return 0;
  Gui::View3DInventor *view = dynamic_cast<Gui::View3DInventor*>(doc->getActiveView());
  if (!view)
    return 0;
  Gui::View3DInventorViewer *viewer = view->getViewer();
  if (!viewer)
    return 0;
  return viewer;
}

void PartGui::addLinearDimensions(const BRepExtrema_DistShapeShape &measure)
{
  ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
  App::Color c(1.0,0.0,0.0);
  c.fromHexString(group->GetASCII("Dimensions3dColor", c.asHexString().c_str()));
  App::Color d(0.0,1.0,0.0);
  d.fromHexString(group->GetASCII("DimensionsDeltaColor", d.asHexString().c_str()));

  Gui::View3DInventorViewer *viewer = getViewer();
  if (!viewer)
    return;
  gp_Pnt point1 = measure.PointOnShape1(1);
  gp_Pnt point2 = measure.PointOnShape2(1);
  viewer->addDimension3d(createLinearDimension(point1, point2, SbColor(c.r, c.g, c.b)));

  //create deltas. point1 will always be the same.
  gp_Pnt temp = point1;
  gp_Pnt lastTemp = temp;
  temp.SetX(point2.X());
  viewer->addDimensionDelta(createLinearDimension(lastTemp, temp, SbColor(d.r, d.g, d.b)));
  lastTemp = temp;
  temp.SetY(point2.Y());
  viewer->addDimensionDelta(createLinearDimension(lastTemp, temp, SbColor(d.r, d.g, d.b)));
  lastTemp = temp;
  temp.SetZ(point2.Z());
  viewer->addDimensionDelta(createLinearDimension(lastTemp, temp, SbColor(d.r, d.g, d.b)));
}

SoNode* PartGui::createLinearDimension(const gp_Pnt &point1, const gp_Pnt &point2, const SbColor &color)
{
  SbVec3f vec1(point1.X(), point1.Y(), point1.Z());
  SbVec3f vec2(point2.X(), point2.Y(), point2.Z());
  if ((vec2-vec1).length() < FLT_EPSILON)
    return new SoSeparator(); //empty object.
  PartGui::DimensionLinear *dimension = new PartGui::DimensionLinear();
  dimension->point1.setValue(vec1);
  dimension->point2.setValue(vec2);
  dimension->setupDimension();

  Base::Quantity quantity(static_cast<double>((vec2-vec1).length()), Base::Unit::Length);
  dimension->text.setValue(quantity.getUserString().toUtf8().constData());

  dimension->dColor.setValue(color);
  return dimension;
}

void PartGui::eraseAllDimensions()
{
  Gui::Document *doc = Gui::Application::Instance->activeDocument();
  if (!doc)
    return;
  _Measures.erase(doc->getDocument()->getName());
  Gui::View3DInventor *view = dynamic_cast<Gui::View3DInventor*>(doc->getActiveView());
  if (!view)
    return;
  Gui::View3DInventorViewer *viewer = view->getViewer();
  if (!viewer)
    return;
  viewer->eraseAllDimensions();
}

void PartGui::refreshDimensions() {
  auto doc = App::GetApplication().getActiveDocument();
  if(!doc) 
      return;
  auto it = _Measures.find(doc->getName());
  if(it == _Measures.end())
      return;
  std::list<MeasureInfo> measures;
  measures.swap(it->second);
  eraseAllDimensions();
  for(auto &info : measures) {
      if(info.linear)
          PartGui::TaskMeasureLinear::buildDimension(info.sel1,info.sel2);
      else
          PartGui::TaskMeasureAngular::buildDimension(info.sel1,info.sel2);
  }
}
    
void PartGui::toggle3d()
{
  ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
  bool visibility = group->GetBool("Dimensions3dVisible", true);
  if (visibility)
    group->SetBool("Dimensions3dVisible", false);
  else
    group->SetBool("Dimensions3dVisible", true);
}

void PartGui::toggleDelta()
{
  ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
  bool visibility = group->GetBool("DimensionsDeltaVisible", true);
  if (visibility)
    group->SetBool("DimensionsDeltaVisible", false);
  else
    group->SetBool("DimensionsDeltaVisible", true);
}

void PartGui::ensureSomeDimensionVisible()
{
  ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
  bool visibilityAll = group->GetBool("DimensionsVisible", true);
  if (!visibilityAll)
    group->SetBool("DimensionsVisible", true);
  bool visibility3d = group->GetBool("Dimensions3dVisible", true);
  bool visibilityDelta = group->GetBool("DimensionsDeltaVisible", true);

  if (!visibility3d && !visibilityDelta) //both turned off.
    group->SetBool("Dimensions3dVisible", true); //turn on 3d, so something is visible.
}

void PartGui::ensure3dDimensionVisible()
{
  ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
  bool visibilityAll = group->GetBool("DimensionsVisible", true);
  if (!visibilityAll)
    group->SetBool("DimensionsVisible", true);
  bool visibility3d = group->GetBool("Dimensions3dVisible", true);

  if (!visibility3d) //both turned off.
    group->SetBool("Dimensions3dVisible", true); //turn on 3d, so something is visible.
}


SO_KIT_SOURCE(PartGui::DimensionLinear);

void PartGui::DimensionLinear::initClass()
{
    SO_KIT_INIT_CLASS(DimensionLinear, SoSeparatorKit, "SeparatorKit");
}

PartGui::DimensionLinear::DimensionLinear()
{
    SO_KIT_CONSTRUCTOR(PartGui::DimensionLinear);

    SO_KIT_ADD_CATALOG_ENTRY(transformation, SoTransform, true, topSeparator,"" , true);
    SO_KIT_ADD_CATALOG_ENTRY(annotate, SoAnnotation, true, topSeparator,"" , true);
    SO_KIT_ADD_CATALOG_ENTRY(leftArrow, SoShapeKit, true, topSeparator,"" ,true);
    SO_KIT_ADD_CATALOG_ENTRY(rightArrow, SoShapeKit, true, topSeparator,"" ,true);
    SO_KIT_ADD_CATALOG_ENTRY(line, SoShapeKit, true, annotate,"" ,true);
    SO_KIT_ADD_CATALOG_ENTRY(textSep, SoSeparator, true, annotate,"" ,true);

    SO_KIT_INIT_INSTANCE();

    SO_NODE_ADD_FIELD(rotate, (1.0, 0.0, 0.0, 0.0));//position orientation of the dimension.
    SO_NODE_ADD_FIELD(length, (1.0));//turns into dimension length
    SO_NODE_ADD_FIELD(origin, (0.0, 0.0, 0.0));//static
    SO_NODE_ADD_FIELD(text, ("test"));//dimension text
    SO_NODE_ADD_FIELD(dColor, (1.0, 0.0, 0.0));//dimension color.
}

PartGui::DimensionLinear::~DimensionLinear()
{

}

SbBool PartGui::DimensionLinear::affectsState() const
{
    return false;
}

void PartGui::DimensionLinear::setupDimension()
{
  //transformation
  SoTransform *trans = static_cast<SoTransform *>(getPart("transformation", true));
  trans->translation.connectFrom(&point1);
  //build engine for vector subtraction and length.
  SoCalculator *hyp = new SoCalculator();
  hyp->A.connectFrom(&point1);
  hyp->B.connectFrom(&point2);
  hyp->expression.set1Value(0, "oA = B-A");
  hyp->expression.set1Value(1, "oB = normalize(oA)");
  hyp->expression.set1Value(2, "oa = length(oA)");
  length.connectFrom(&hyp->oa);

  //build engine for rotation.
  SoComposeRotationFromTo *rotationEngine = new SoComposeRotationFromTo();
  rotationEngine->from.setValue(SbVec3f(1.0, 0.0, 0.0));
  rotationEngine->to.connectFrom(&hyp->oB);
  trans->rotation.connectFrom(&rotationEngine->rotation);

  //color
  SoMaterial *material = new SoMaterial;
  material->diffuseColor.connectFrom(&dColor);

  //dimension arrows
  float dimLength = (point2.getValue()-point1.getValue()).length();
  float coneHeight = dimLength * 0.05;
  float coneRadius = coneHeight * 0.5;

  SoCone *cone = new SoCone();
  cone->bottomRadius.setValue(coneRadius);
  cone->height.setValue(coneHeight);

  char lStr[100];
  char rStr[100];
  snprintf(lStr, sizeof(lStr), "translation %.6f 0.0 0.0", coneHeight * 0.5);
  snprintf(rStr, sizeof(rStr), "translation 0.0 -%.6f 0.0", coneHeight * 0.5);

  setPart("leftArrow.shape", cone);
  set("leftArrow.transform", "rotation 0.0 0.0 1.0 1.5707963");
  set("leftArrow.transform", lStr);
  setPart("rightArrow.shape", cone);
  set("rightArrow.transform", "rotation 0.0 0.0 -1.0 1.5707963"); //no constant for PI.
  //have use local here to do the offset because the main is wired up to length of dimension.
  set("rightArrow.localTransform", rStr);

  SoTransform *transform = static_cast<SoTransform *>(getPart("rightArrow.transform", false));
  if (!transform)
      return;//what to do here?
  SoComposeVec3f *vec = new SoComposeVec3f;
  vec->x.connectFrom(&length);
  vec->y.setValue(0.0);
  vec->z.setValue(0.0);
  transform->translation.connectFrom(&vec->vector);

  setPart("leftArrow.material", material);
  setPart("rightArrow.material", material);

  //line
  SoConcatenate *catEngine = new SoConcatenate(SoMFVec3f::getClassTypeId());
  //don't know how to get around having this dummy origin. cat engine wants to connectfrom?
  catEngine->input[0]->connectFrom(&origin);
  catEngine->input[1]->connectFrom(&vec->vector);

  SoVertexProperty *lineVerts = new SoVertexProperty;
  lineVerts->vertex.connectFrom(catEngine->output);

  int lineVertexMap[] = {0, 1};
  int lineVertexMapSize(sizeof(lineVertexMap)/sizeof(int));
  SoIndexedLineSet *line = new SoIndexedLineSet;
  line->vertexProperty = lineVerts;
  line->coordIndex.setValues(0, lineVertexMapSize, lineVertexMap);

  setPart("line.shape", line);
  setPart("line.material", material);

  //text
  SoSeparator *textSep = static_cast<SoSeparator *>(getPart("textSep", true));
  if (!textSep)
      return;

  textSep->addChild(material);

  SoCalculator *textVecCalc = new SoCalculator();
  textVecCalc->A.connectFrom(&vec->vector);
  textVecCalc->B.set1Value(0, 0.0, 0.250, 0.0);
  textVecCalc->expression.set1Value(0, "oA = (A / 2) + B");

  SoTransform *textTransform =  new SoTransform();
  textTransform->translation.connectFrom(&textVecCalc->oA);
  textSep->addChild(textTransform);

  SoFont *fontNode = new SoFont();
  fontNode->name.setValue("defaultFont");
  fontNode->size.setValue(30);
  textSep->addChild(fontNode);

  SoText2 *textNode = new SoText2();
  textNode->justification = SoText2::CENTER;
  textNode->string.connectFrom(&text);
  textSep->addChild(textNode);

  //this prevents the 2d text from screwing up the bounding box for a viewall
  SoResetTransform *rTrans = new SoResetTransform;
  rTrans->whatToReset = SoResetTransform::BBOX;
  textSep->addChild(rTrans);
}

PartGui::TaskMeasureLinear::TaskMeasureLinear()
    : Gui::SelectionObserver(true,false)
    , selections1(), selections2(), buttonSelectedIndex(0)
{
  setUpGui();
}

PartGui::TaskMeasureLinear::~TaskMeasureLinear()
{
  Gui::Selection().clearSelection();
}

void PartGui::TaskMeasureLinear::onSelectionChanged(const Gui::SelectionChanges& msg)
{
  if (buttonSelectedIndex == 0)
  {
    if (msg.Type == Gui::SelectionChanges::AddSelection)
    {
      DimSelections::DimSelection newSelection;
      newSelection.documentName = msg.pDocName;
      newSelection.objectName = msg.pObjectName;
      newSelection.subObjectName = msg.pSubName;
      newSelection.x = msg.x;
      newSelection.y = msg.y;
      newSelection.z = msg.z;
      selections1.selections.clear();//we only want one item.
      selections1.selections.push_back(newSelection);
      QTimer::singleShot(0, this, SLOT(selectionClearDelayedSlot()));
      stepped->getButton(1)->setEnabled(true);
      stepped->getButton(1)->setChecked(true);
      return;
    }
  }
  if (buttonSelectedIndex == 1)
  {
    if (msg.Type == Gui::SelectionChanges::AddSelection)
    {
      DimSelections::DimSelection newSelection;
      newSelection.documentName = msg.pDocName;
      newSelection.objectName = msg.pObjectName;
      newSelection.subObjectName = msg.pSubName;
      newSelection.x = msg.x;
      newSelection.y = msg.y;
      newSelection.z = msg.z;
      selections2.selections.clear();//we only want one item.
      selections2.selections.push_back(newSelection);
      buildDimension();
      clearSelectionStrings();
      QTimer::singleShot(0, this, SLOT(selectionClearDelayedSlot()));
      stepped->getButton(0)->setChecked(true);
      stepped->getButton(1)->setEnabled(false);
      return;
    }
  }
}

void PartGui::TaskMeasureLinear::selectionClearDelayedSlot()
{
  //hack.
  //clearing selections are not working as I hoped. Apparently the observer callback gets called
  //before the actual selection takes place. Resulting in selections being left. this addresses this
  //by being called from the event loop.
  this->blockConnection(true);
  Gui::Selection().clearSelection();
  this->blockConnection(false);
}

void PartGui::TaskMeasureLinear::buildDimension() {
    buildDimension(selections1,selections2);
}

void PartGui::TaskMeasureLinear::buildDimension(const DimSelections &sel1, const DimSelections &sel2)
{
  if(sel1.selections.size() != 1 || sel2.selections.size() != 1)
    return;
  
  DimSelections::DimSelection current1 = sel1.selections.at(0);
  DimSelections::DimSelection current2 = sel2.selections.at(0);
  
  TopoDS_Shape shape1, shape2;
  if (!getShapeFromStrings(shape1, current1.documentName, current1.objectName, current1.subObjectName))
  {
    Base::Console().Message("\nFailed to get shape\n\n");
    return;
  }
  if (!getShapeFromStrings(shape2, current2.documentName, current2.objectName, current2.subObjectName))
  {
    Base::Console().Message("\nFailed to get shape\n\n");
    return;
  }
  auto doc = App::GetApplication().getActiveDocument();
  if(doc) 
    _Measures[doc->getName()].emplace_back(sel1,sel2,true);
  goDimensionLinearNoTask(shape1, shape2);
}

void PartGui::TaskMeasureLinear::clearSelectionStrings()
{
  selections1.selections.clear();
  selections2.selections.clear();
}

void PartGui::TaskMeasureLinear::setUpGui()
{
  QPixmap mainIcon = Gui::BitmapFactory().pixmap("Part_Measure_Linear");

  Gui::TaskView::TaskBox* selectionTaskBox = new Gui::TaskView::TaskBox
    (mainIcon, QObject::tr("Selections"), false, 0);
  QVBoxLayout *selectionLayout = new QVBoxLayout();
  stepped = new SteppedSelection(2, selectionTaskBox);
  selectionLayout->addWidget(stepped);
  selectionTaskBox->groupLayout()->addLayout(selectionLayout);

  Gui::TaskView::TaskBox* controlTaskBox = new Gui::TaskView::TaskBox
    (mainIcon, QObject::tr("Control"), false, 0);
  QVBoxLayout *controlLayout = new QVBoxLayout();

  DimensionControl *control = new DimensionControl(controlTaskBox);
  controlLayout->addWidget(control);
  controlTaskBox->groupLayout()->addLayout(controlLayout);
  QObject::connect(control->resetButton, SIGNAL(clicked(bool)), this, SLOT(resetDialogSlot(bool)));

  this->setButtonPosition(TaskDialog::South);
  Content.push_back(selectionTaskBox);
  Content.push_back(controlTaskBox);

  stepped->getButton(0)->setChecked(true);//before wired up.
  stepped->getButton(0)->setEnabled(true);
  QObject::connect(stepped->getButton(0), SIGNAL(toggled(bool)), this, SLOT(selection1Slot(bool)));
  QObject::connect(stepped->getButton(1), SIGNAL(toggled(bool)), this, SLOT(selection2Slot(bool)));
}

void PartGui::TaskMeasureLinear::selection1Slot(bool checked)
{
  if (!checked)
  {
    if (selections1.selections.size() > 0)
      stepped->setIconDone(0);
    return;
  }
  buttonSelectedIndex = 0;

  this->blockConnection(true);
  Gui::Selection().clearSelection();
  //we should only be working with 1 entity, but oh well do the loop anyway.
  std::vector<DimSelections::DimSelection>::const_iterator it;
  for (it = selections1.selections.begin(); it != selections1.selections.end(); ++it)
    Gui::Selection().addSelection(it->documentName.c_str(), it->objectName.c_str(), it->subObjectName.c_str());
  this->blockConnection(false);
}

void PartGui::TaskMeasureLinear::selection2Slot(bool checked)
{
  if (!checked)
    return;
  buttonSelectedIndex = 1;
  this->blockConnection(true);
  Gui::Selection().clearSelection();
  std::vector<DimSelections::DimSelection>::const_iterator it;
  for (it = selections2.selections.begin(); it != selections2.selections.end(); ++it)
    Gui::Selection().addSelection(it->documentName.c_str(), it->objectName.c_str(), it->subObjectName.c_str());
  this->blockConnection(false);
}

void PartGui::TaskMeasureLinear::resetDialogSlot(bool)
{
  clearSelectionStrings();
  this->blockConnection(true);
  Gui::Selection().clearSelection();
  stepped->getButton(0)->setChecked(true);
  stepped->getButton(1)->setEnabled(false);
  this->blockConnection(false);
}

void PartGui::TaskMeasureLinear::toggle3dSlot(bool)
{
  PartGui::toggle3d();
}

void PartGui::TaskMeasureLinear::toggleDeltaSlot(bool)
{
  PartGui::toggleDelta();
}

void PartGui::TaskMeasureLinear::clearAllSlot(bool)
{
  PartGui::eraseAllDimensions();
}

PartGui::VectorAdapter::VectorAdapter() : status(false), vector()
{
}

PartGui::VectorAdapter::VectorAdapter(const TopoDS_Face &faceIn, const gp_Vec &pickedPointIn) :
  status(false), vector(), origin(pickedPointIn)
{
  Handle(Geom_Surface) surface = BRep_Tool::Surface(faceIn);
  if (surface->IsKind(STANDARD_TYPE(Geom_ElementarySurface)))
  {
    Handle(Geom_ElementarySurface) eSurface = Handle(Geom_ElementarySurface)::DownCast(surface);
    gp_Dir direction = eSurface->Axis().Direction();
    vector = direction;
    vector.Normalize();
    if (faceIn.Orientation() == TopAbs_REVERSED)
      vector.Reverse();
    if (surface->IsKind(STANDARD_TYPE(Geom_CylindricalSurface)) ||
      surface->IsKind(STANDARD_TYPE(Geom_SphericalSurface))
    )
    {
      origin = eSurface->Axis().Location().XYZ();
      projectOriginOntoVector(pickedPointIn);
    }
    else
      origin = pickedPointIn + vector;
    status = true;
  }
}

PartGui::VectorAdapter::VectorAdapter(const TopoDS_Edge &edgeIn, const gp_Vec &pickedPointIn) :
  status(false), vector(), origin(pickedPointIn)
{
  TopoDS_Vertex firstVertex = TopExp::FirstVertex(edgeIn, Standard_True);
  TopoDS_Vertex lastVertex = TopExp::LastVertex(edgeIn, Standard_True);
  vector = PartGui::convert(lastVertex) - PartGui::convert(firstVertex);
  if (vector.Magnitude() < Precision::Confusion())
    return;
  vector.Normalize();

  status = true;
  projectOriginOntoVector(pickedPointIn);
}

PartGui::VectorAdapter::VectorAdapter(const TopoDS_Vertex &vertex1In, const TopoDS_Vertex &vertex2In) :
  status(false), vector(), origin()
{
  vector = PartGui::convert(vertex2In) - PartGui::convert(vertex1In);
  vector.Normalize();

  //build origin half way.
  gp_Vec tempVector = (PartGui::convert(vertex2In) - PartGui::convert(vertex1In));
  double mag = tempVector.Magnitude();
  tempVector.Normalize();
  tempVector *= (mag / 2.0);
  origin = tempVector + PartGui::convert(vertex1In);

  status = true;
}

PartGui::VectorAdapter::VectorAdapter(const gp_Vec &vector1, const gp_Vec &vector2) :
  status(false), vector(), origin()
{
  vector = vector2- vector1;
  vector.Normalize();

  //build origin half way.
  gp_Vec tempVector = vector2 - vector1;
  double mag = tempVector.Magnitude();
  tempVector.Normalize();
  tempVector *= (mag / 2.0);
  origin = tempVector + vector1;

  status = true;
}

void PartGui::VectorAdapter::projectOriginOntoVector(const gp_Vec &pickedPointIn)
{
  Handle(Geom_Curve) heapLine = new Geom_Line(origin.XYZ(), vector.XYZ());
  gp_Pnt tempPoint(pickedPointIn.XYZ());
  GeomAPI_ProjectPointOnCurve projection(tempPoint, heapLine);
  if (projection.NbPoints() < 1)
    return;
  origin.SetXYZ(projection.Point(1).XYZ());
}

PartGui::VectorAdapter::operator gp_Lin() const
{
  gp_Pnt tempOrigin;
  tempOrigin.SetXYZ(origin.XYZ());
  return gp_Lin(tempOrigin, gp_Dir(vector));
}

gp_Vec PartGui::convert(const TopoDS_Vertex &vertex)
{
  gp_Pnt point = BRep_Tool::Pnt(vertex);
  gp_Vec out(point.X(), point.Y(), point.Z());
  return out;
}

void PartGui::goDimensionAngularRoot()
{
  PartGui::ensure3dDimensionVisible();

  VectorAdapter adapter1, adapter2;
  if(PartGui::evaluateAngularPreSelection(adapter1, adapter2))
    goDimensionAngularNoTask(adapter1, adapter2);
  else
  {
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (!dlg)
    {
      Gui::Selection().clearSelection();
      dlg = new PartGui::TaskMeasureAngular();
    }
    Gui::Control().showDialog(dlg);
  }
  Gui::Selection().clearSelection();
}

bool PartGui::evaluateAngularPreSelection(VectorAdapter &vector1Out, VectorAdapter &vector2Out)
{
  std::vector<Gui::SelectionSingleton::SelObj> selections = Gui::Selection().getSelection(0,false);
  if (selections.size() > 4 || selections.size() < 2)
    return false;
  std::vector<Gui::SelectionSingleton::SelObj>::iterator it;
  std::vector<VectorAdapter> adapters;
  std::vector<DimSelections> sels;
  TopoDS_Vertex lastVertex;
  for (it = selections.begin(); it != selections.end(); ++it)
  {
    Base::Matrix4D mat;
    TopoDS_Shape shape = Part::Feature::getShape(it->pObject,it->SubName,true,&mat);
    if (shape.IsNull())
      break;
    mat.inverse();
    
    if (shape.ShapeType() == TopAbs_VERTEX)
    {
        if(sels.empty() || 
           sels.back().selections.back().shapeType!=DimSelections::Vertex ||
           sels.back().selections.size()==1) 
        {
            sels.push_back(PartGui::DimSelections());
        }
        sels.back().selections.push_back(DimSelections::DimSelection());
        auto &sel = sels.back().selections.back();
        sel.documentName = it->DocName;
        sel.objectName = it->FeatName;
        sel.subObjectName = it->SubName;
        sel.shapeType = DimSelections::Vertex;
        Base::Vector3d v(it->x,it->y,it->z);
        v = mat*v;
        sel.x = v.x;
        sel.y = v.y;
        sel.z = v.z;

      TopoDS_Vertex currentVertex = TopoDS::Vertex(shape);
      if (!lastVertex.IsNull())
      {
	//need something here for 0 length vector.
	//create a point half way between to vertices.
	adapters.push_back(VectorAdapter(currentVertex, lastVertex));
	lastVertex = TopoDS_Vertex();
      }
      else
      {
	lastVertex = currentVertex;
      }
      continue;
    }
    //vertices have to be selected in succession. so if we make it here clear the last vertex.
    lastVertex = TopoDS_Vertex();

    gp_Vec pickPoint(it->x, it->y, it->z);
    //can't use selections without a pick point.
    if (pickPoint.IsEqual(gp_Vec(0.0, 0.0, 0.0), Precision::Confusion(), Precision::Angular()))
    {
      Base::Console().Message("Can't use selections without a pick point.\n");
      continue;
    }

    sels.push_back(PartGui::DimSelections());
    sels.back().selections.push_back(DimSelections::DimSelection());
    auto &sel = sels.back().selections.back();
    sel.documentName = it->DocName;
    sel.objectName = it->FeatName;
    sel.subObjectName = it->SubName;
    Base::Vector3d v(it->x,it->y,it->z);
    v = mat*v;
    sel.x = v.x;
    sel.y = v.y;
    sel.z = v.z;
    
    if (shape.ShapeType() == TopAbs_EDGE)
    {
      sel.shapeType = DimSelections::Edge;
      TopoDS_Edge edge = TopoDS::Edge(shape);
      // make edge orientation so that end of edge closest to pick is head of vector.
      gp_Vec firstPoint = PartGui::convert(TopExp::FirstVertex(edge, Standard_True));
      gp_Vec lastPoint = PartGui::convert(TopExp::LastVertex(edge, Standard_True));
      double firstDistance = (firstPoint - pickPoint).Magnitude();
      double lastDistance = (lastPoint - pickPoint).Magnitude();
      if (lastDistance > firstDistance)
      {
	if (edge.Orientation() == TopAbs_FORWARD)
	  edge.Orientation(TopAbs_REVERSED);
	else
	  edge.Orientation(TopAbs_FORWARD);
      }
      adapters.push_back(VectorAdapter(edge, pickPoint));
      continue;
    }

    if (shape.ShapeType() == TopAbs_FACE)
    {
      sel.shapeType = DimSelections::Face;
      TopoDS_Face face = TopoDS::Face(shape);
      adapters.push_back(VectorAdapter(face, pickPoint));
      continue;
    }
  }

  if (adapters.size() != 2)
    return false;
  if (!adapters.front().isValid() || !adapters.back().isValid())
    return false;

  vector1Out = adapters.front();
  vector2Out = adapters.back();

  //making sure pick points are not equal
  if ((vector1Out.getPickPoint() - vector2Out.getPickPoint()).Magnitude() < std::numeric_limits<float>::epsilon())
  {
    Base::Console().Message("pick points are equal\n");
    return false;
  }

  auto doc = App::GetApplication().getActiveDocument();
  if(doc) 
    _Measures[doc->getName()].emplace_back(sels[0],sels[1],false);
  return true;
}

void PartGui::goDimensionAngularNoTask(const VectorAdapter &vector1Adapter, const VectorAdapter &vector2Adapter)
{
  gp_Vec vector1 = vector1Adapter;
  gp_Vec vector2 = vector2Adapter;
  double angle = vector1.Angle(vector2);

  std::ostringstream stream;
  stream << std::setprecision(std::numeric_limits<double>::digits10 + 1) << std::fixed << std::endl <<
    "angle in radians is: " << angle << std::endl <<
    "angle in degrees is: " << 180 * angle / M_PI << std::endl;
  if (angle < M_PI / 2.0)
    stream << std::setprecision(std::numeric_limits<double>::digits10 + 1) <<
    "complement in radians is: " << M_PI / 2.0 - angle << std::endl <<
    "complement in degrees is: " << 90 - 180 * angle / M_PI << std::endl;
  //I don't think we get anything over 180, but just in case.
  if (angle > M_PI / 2.0 && angle < M_PI)
    stream << std::setprecision(std::numeric_limits<double>::digits10 + 1) <<
    "supplement in radians is: " << M_PI - angle << std::endl <<
    "supplement in degrees is: " << 180 - 180 * angle / M_PI << std::endl;
  Base::Console().Message(stream.str().c_str());

  SbMatrix dimSys;
  double radius;
  double displayAngle;//have to fake the angle in the 3d.

  if (vector1.IsParallel(vector2, Precision::Angular()))
  {
    //take first point project it onto second vector.
    Handle(Geom_Curve) heapLine2 = new Geom_Line(vector2Adapter);
    gp_Pnt tempPoint(vector1Adapter.getPickPoint().XYZ());

    GeomAPI_ProjectPointOnCurve projection(tempPoint, heapLine2);
    if (projection.NbPoints() < 1)
    {
      Base::Console().Message("parallel vectors: couldn't project onto line\n");
      return;
    }
    gp_Vec newPoint2;
    newPoint2.SetXYZ(projection.Point(1).XYZ());

    //if points are colinear, projection doesn't work and returns the same point.
    //In this case we just use the original point.
    if ((newPoint2 - vector1Adapter.getPickPoint()).Magnitude() < Precision::Confusion())
      newPoint2 = vector2Adapter.getPickPoint();

    //now get midpoint between for dim origin.
    gp_Vec point1 = vector1Adapter.getPickPoint();
    gp_Vec midPointProjection = newPoint2 - point1;
    double distance = midPointProjection.Magnitude();
    midPointProjection.Normalize();
    midPointProjection *= distance / 2.0;

    gp_Vec origin = point1 + midPointProjection;

    //yaxis should be the same as vector1, but doing this to eliminate any potential slop from
    //using precision::angular. If lines are colinear and we have no plane, we can't establish zAxis from crossing.
    //we just the absolute axis.
    gp_Vec xAxis = (point1 - origin).Normalized();
    gp_Vec zAxis;
    if (xAxis.IsParallel(vector1, Precision::Angular()))
    {
      if (!xAxis.IsParallel(gp_Vec(0.0, 0.0, 1.0), Precision::Angular()))
	zAxis = gp_Vec(0.0, 0.0, 1.0);
      else
	zAxis = gp_Vec(0.0, 1.0, 0.0);
    }
    else
      zAxis = xAxis.Crossed(vector1).Normalized();
    gp_Vec yAxis = zAxis.Crossed(xAxis).Normalized();
    zAxis = xAxis.Crossed(yAxis).Normalized();

    dimSys = SbMatrix
    (
      xAxis.X(), yAxis.X(), zAxis.X(), origin.X(),
      xAxis.Y(), yAxis.Y(), zAxis.Y(), origin.Y(),
      xAxis.Z(), yAxis.Z(), zAxis.Z(), origin.Z(),
      0.0, 0.0, 0.0, 1.0
    );
    dimSys = dimSys.transpose();

    radius = midPointProjection.Magnitude();
    displayAngle = M_PI;
  }
  else
  {
    Handle(Geom_Curve) heapLine1 = new Geom_Line(vector1Adapter);
    Handle(Geom_Curve) heapLine2 = new Geom_Line(vector2Adapter);

    GeomAPI_ExtremaCurveCurve extrema(heapLine1, heapLine2);

    if (extrema.NbExtrema() < 1)
    {
      Base::Console().Message("couldn't get extrema\n");
      return;
    }

    gp_Pnt extremaPoint1, extremaPoint2, dimensionOriginPoint;
    extrema.Points(1, extremaPoint1, extremaPoint2);
    if (extremaPoint1.Distance(extremaPoint2) < Precision::Confusion())
      dimensionOriginPoint = extremaPoint1;
    else
    {
      //find halfway point in between extrema points for dimension origin.
      gp_Vec vec1(extremaPoint1.XYZ());
      gp_Vec vec2(extremaPoint2.XYZ());
      gp_Vec connection(vec2-vec1);
      Standard_Real distance = connection.Magnitude();
      connection.Normalize();
      connection *= (distance / 2.0);
      dimensionOriginPoint.SetXYZ((vec1 + connection).XYZ());
    }

    gp_Vec thirdPoint(vector2Adapter.getPickPoint());
    gp_Vec originVector(dimensionOriginPoint.XYZ());
    gp_Vec extrema2Vector(extremaPoint2.XYZ());
    radius = (vector1Adapter.getPickPoint() - originVector).Magnitude();
    double legOne = (extrema2Vector - originVector).Magnitude();
    displayAngle = angle;
    if (legOne > Precision::Confusion())
    {
      double legTwo = sqrt(pow(radius, 2) - pow(legOne, 2));
      gp_Vec projectionVector(vector2);
      projectionVector.Normalize();
      projectionVector *= legTwo;
      thirdPoint = extrema2Vector + projectionVector;
      gp_Vec hyp(thirdPoint - originVector);
      hyp.Normalize();
      gp_Vec otherSide(vector1Adapter.getPickPoint() - originVector);
      otherSide.Normalize();
      displayAngle = hyp.Angle(otherSide);
    }

    gp_Vec xAxis = (vector1Adapter.getPickPoint() - originVector).Normalized();
    gp_Vec fakeYAxis = (thirdPoint - originVector).Normalized();
    gp_Vec zAxis = (xAxis.Crossed(fakeYAxis)).Normalized();
    gp_Vec yAxis = zAxis.Crossed(xAxis).Normalized();

    dimSys = SbMatrix
    (
      xAxis.X(), yAxis.X(), zAxis.X(), dimensionOriginPoint.X(),
      xAxis.Y(), yAxis.Y(), zAxis.Y(), dimensionOriginPoint.Y(),
      xAxis.Z(), yAxis.Z(), zAxis.Z(), dimensionOriginPoint.Z(),
      0.0, 0.0, 0.0, 1.0
    );

    dimSys = dimSys.transpose();
  }

  ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
  App::Color c(0.0,0.0,1.0);
  c.fromHexString(group->GetASCII("DimensionsAngularColor", c.asHexString().c_str()));

  DimensionAngular *dimension = new DimensionAngular();
  dimension->ref();
  dimension->matrix.setValue(dimSys);
  dimension->radius.setValue(radius);
  dimension->angle.setValue(static_cast<float>(displayAngle));
  dimension->text.setValue((Base::Quantity(180 * angle / M_PI, Base::Unit::Angle)).getUserString().toUtf8().constData());
  dimension->dColor.setValue(SbColor(c.r, c.g, c.b));
  dimension->setupDimension();

  Gui::View3DInventorViewer *viewer = getViewer();
  if (viewer)
    viewer->addDimension3d(dimension);
  dimension->unref();
}

SO_KIT_SOURCE(PartGui::DimensionAngular);

void PartGui::DimensionAngular::initClass()
{
    SO_KIT_INIT_CLASS(DimensionAngular, SoSeparatorKit, "SeparatorKit");
}

PartGui::DimensionAngular::DimensionAngular()
{
    SO_KIT_CONSTRUCTOR(PartGui::DimensionAngular);

    SO_KIT_ADD_CATALOG_ENTRY(transformation, SoMatrixTransform, true, topSeparator,"" , true);
    SO_KIT_ADD_CATALOG_ENTRY(annotate, SoAnnotation, true, topSeparator,"" , true);
    SO_KIT_ADD_CATALOG_ENTRY(arrow1, SoShapeKit, true, topSeparator,"" ,true);
    SO_KIT_ADD_CATALOG_ENTRY(arrow2, SoShapeKit, true, topSeparator,"" ,true);
    SO_KIT_ADD_CATALOG_ENTRY(arcSep, SoSeparator, true, annotate,"" ,true);
    SO_KIT_ADD_CATALOG_ENTRY(textSep, SoSeparator, true, annotate,"" ,true);

    SO_KIT_INIT_INSTANCE();

    SO_NODE_ADD_FIELD(radius, (10.0));
    SO_NODE_ADD_FIELD(angle, (1.0));
    SO_NODE_ADD_FIELD(text, ("test"));//dimension text
    SO_NODE_ADD_FIELD(dColor, (1.0, 0.0, 0.0));//dimension color.
    SO_NODE_ADD_FIELD(matrix, (1.0, 0.0, 0.0, 0.0,
                               0.0, 1.0, 0.0, 0.0,
			       0.0, 0.0, 1.0, 0.0,
			       0.0, 0.0, 0.0, 1.0));
}

PartGui::DimensionAngular::~DimensionAngular()
{

}

SbBool PartGui::DimensionAngular::affectsState() const
{
    return false;
}


void PartGui::DimensionAngular::setupDimension()
{
  //transformation
  SoMatrixTransform *trans = static_cast<SoMatrixTransform *>(getPart("transformation", true));
  trans->matrix.connectFrom(&matrix);

  //color
  SoMaterial *material = new SoMaterial;
  material->ref();
  material->diffuseColor.connectFrom(&dColor);

  //dimension arrows
  float coneHeight = radius.getValue() * 0.1;
  float coneRadius = coneHeight * 0.5;

  SoCone *cone = new SoCone();
  cone->bottomRadius.setValue(coneRadius);
  cone->height.setValue(coneHeight);

  char str1[100];
  char str2[100];
  snprintf(str1, sizeof(str1), "translation 0.0 %.6f 0.0", coneHeight * 0.5);
  snprintf(str2, sizeof(str2), "translation 0.0 -%.6f 0.0", coneHeight * 0.5);

  setPart("arrow1.shape", cone);
  set("arrow1.localTransform", "rotation 0.0 0.0 1.0 3.1415927");
  set("arrow1.localTransform", str1);
  setPart("arrow2.shape", cone);
  set("arrow2.transform", "rotation 0.0 0.0 1.0 0.0");
  set("arrow2.localTransform", str2);

  //I was getting errors if I didn't manually allocate for these transforms. Not sure why.
  SoTransform *arrow1Transform = new SoTransform();
  SoComposeVec3f *arrow1Compose = new SoComposeVec3f();
  arrow1Compose->x.connectFrom(&radius);
  arrow1Compose->y.setValue(0.0);
  arrow1Compose->y.setValue(0.0);
  arrow1Transform->translation.connectFrom(&arrow1Compose->vector);
  setPart("arrow1.transform", arrow1Transform);

  SoComposeRotation *arrow2Rotation = new SoComposeRotation();
  arrow2Rotation->angle.connectFrom(&angle);
  arrow2Rotation->axis.setValue(0.0, 0.0, 1.0);
  SoTransform *arrow2Transform = new SoTransform();
  arrow2Transform->rotation.connectFrom(&arrow2Rotation->rotation);
  SoCalculator *arrow2LocationCalc = new SoCalculator();
  arrow2LocationCalc->a.connectFrom(&angle);
  arrow2LocationCalc->b.connectFrom(&radius);
  arrow2LocationCalc->expression.set1Value(0, "oa = cos(a) * b"); //xLocation
  arrow2LocationCalc->expression.set1Value(1, "ob = sin(a) * b"); //yLocation
  SoComposeVec3f *arrow2Compose = new SoComposeVec3f();
  arrow2Compose->x.connectFrom(&arrow2LocationCalc->oa);
  arrow2Compose->y.connectFrom(&arrow2LocationCalc->ob);
  arrow2Compose->z.setValue(0.0f);
  arrow2Transform->translation.connectFrom(&arrow2Compose->vector);
  setPart("arrow2.transform", arrow2Transform);

  setPart("arrow1.material", material);
  setPart("arrow2.material", material);

  ArcEngine *arcEngine = new ArcEngine();
  arcEngine->angle.connectFrom(&angle);
  arcEngine->radius.connectFrom(&radius);
  arcEngine->deviation.setValue(0.1f);

  SoCoordinate3 *coordinates = new SoCoordinate3();
  coordinates->point.connectFrom(&arcEngine->points);

  SoLineSet *lineSet = new SoLineSet();
  lineSet->ref();
  lineSet->vertexProperty.setValue(coordinates);
  lineSet->numVertices.connectFrom(&arcEngine->pointCount);
  lineSet->startIndex.setValue(0);

  SoSeparator *arcSep = static_cast<SoSeparator *>(getPart("arcSep", true));
  if (arcSep) {
    arcSep->addChild(material);
    arcSep->addChild(lineSet);
  }

  //text
  SoSeparator *textSep = static_cast<SoSeparator *>(getPart("textSep", true));
  if (textSep)
    textSep->addChild(material);

  SoCalculator *textVecCalc = new SoCalculator();
  textVecCalc->a.connectFrom(&angle);
  textVecCalc->b.connectFrom(&radius);
  textVecCalc->expression.set1Value(0, "oa = a / 2.0");
  textVecCalc->expression.set1Value(1, "ob = cos(oa) * b"); //x
  textVecCalc->expression.set1Value(2, "oc = sin(oa) * b"); //y

  SoComposeVec3f *textLocation = new SoComposeVec3f();
  textLocation->x.connectFrom(&textVecCalc->ob);
  textLocation->y.connectFrom(&textVecCalc->oc);
  textLocation->z.setValue(0.0);


  SoTransform *textTransform =  new SoTransform();
  textTransform->translation.connectFrom(&textLocation->vector);
  textSep->addChild(textTransform);

  SoFont *fontNode = new SoFont();
  fontNode->name.setValue("defaultFont");
  fontNode->size.setValue(30);
  textSep->addChild(fontNode);

  SoText2 *textNode = new SoText2();
  textNode->justification = SoText2::CENTER;
  textNode->string.connectFrom(&text);
  textSep->addChild(textNode);

  //this prevents the 2d text from screwing up the bounding box for a viewall
  SoResetTransform *rTrans = new SoResetTransform;
  rTrans->whatToReset = SoResetTransform::BBOX;
  textSep->addChild(rTrans);

  lineSet->unref();
  material->unref();
}

SO_ENGINE_SOURCE(PartGui::ArcEngine);

PartGui::ArcEngine::ArcEngine()
{
  SO_ENGINE_CONSTRUCTOR(ArcEngine);

  SO_ENGINE_ADD_INPUT(radius, (10.0));
  SO_ENGINE_ADD_INPUT(angle, (1.0));
  SO_ENGINE_ADD_INPUT(deviation, (0.25));

  SO_ENGINE_ADD_OUTPUT(points, SoMFVec3f);
  SO_ENGINE_ADD_OUTPUT(pointCount, SoSFInt32);
}

void PartGui::ArcEngine::initClass()
{
  SO_ENGINE_INIT_CLASS(ArcEngine, SoEngine, "Engine");
}

void PartGui::ArcEngine::evaluate()
{
  if (radius.getValue() < std::numeric_limits<float>::epsilon() ||
    angle.getValue() < std::numeric_limits<float>::epsilon() ||
    deviation.getValue() < std::numeric_limits<float>::epsilon())
  {
    defaultValues();
    return;
  }

  float deviationAngle(acos((radius.getValue() - deviation.getValue()) / radius.getValue()));
  std::vector<SbVec3f> tempPoints;
  int segmentCount;
  if (deviationAngle >= angle.getValue())
    segmentCount = 1;
  else
  {
    segmentCount = static_cast<int>(angle.getValue() / deviationAngle) + 1;
    if (segmentCount < 2)
    {
      defaultValues();
      return;
    }
  }
  float angleIncrement = angle.getValue() / static_cast<float>(segmentCount);
  for (int index = 0; index < segmentCount + 1; ++index)
  {
    SbVec3f currentNormal(1.0, 0.0, 0.0);
    float currentAngle = index * angleIncrement;
    SbRotation rotation(SbVec3f(0.0, 0.0, 1.0), currentAngle);
    rotation.multVec(currentNormal, currentNormal);
    tempPoints.push_back(currentNormal * radius.getValue());
  }
  int tempCount = tempPoints.size(); //for macro.
  SO_ENGINE_OUTPUT(points, SoMFVec3f, setNum(tempCount));
  SO_ENGINE_OUTPUT(pointCount, SoSFInt32, setValue(tempCount));
  std::vector<SbVec3f>::const_iterator it;
  for (it = tempPoints.begin(); it != tempPoints.end(); ++it)
  {
    int currentIndex = it-tempPoints.begin(); //for macro.
    SbVec3f temp(*it); //for macro
    SO_ENGINE_OUTPUT(points, SoMFVec3f, set1Value(currentIndex, temp));
  }

}

void PartGui::ArcEngine::defaultValues()
{
  //just some non-failing info.
  SO_ENGINE_OUTPUT(points, SoMFVec3f, setNum(2));
  SbVec3f point1(10.0, 0.0, 0.0);
  SO_ENGINE_OUTPUT(points, SoMFVec3f, set1Value(0, point1));
  SbVec3f point2(7.07f, 7.07f, 0.0);
  SO_ENGINE_OUTPUT(points, SoMFVec3f, set1Value(1, point2));
  SO_ENGINE_OUTPUT(pointCount, SoSFInt32, setValue(2));
}

PartGui::SteppedSelection::SteppedSelection(const uint& buttonCountIn, QWidget* parent)
  : QWidget(parent)
  , stepActive(0)
  , stepDone(0)
{
  if (buttonCountIn < 1)
    return;

  QVBoxLayout *mainLayout = new QVBoxLayout();
  this->setLayout(mainLayout);

  QButtonGroup *buttonGroup = new QButtonGroup();
  buttonGroup->setExclusive(true);

  for (uint index = 0; index < buttonCountIn; ++index)
  {
    ButtonIconPairType tempPair;

    std::ostringstream stream;
    stream << "Selection " << ((index < 10) ? "0" : "") <<  index + 1;
    QString buttonText = QObject::tr(stream.str().c_str());
    QPushButton *button = new QPushButton(buttonText, this);
    button->setCheckable(true);
    button->setEnabled(false);
    buttonGroup->addButton(button);
    connect(button, SIGNAL(toggled(bool)), this, SLOT(selectionSlot(bool)));

    QLabel *label = new QLabel;

    tempPair.first = button;
    tempPair.second = label;
    buttons.push_back(tempPair);

    QHBoxLayout *layout = new QHBoxLayout();
    mainLayout->addLayout(layout);
    layout->addWidget(button);
    layout->addSpacing(10);
    layout->addWidget(label);
    layout->addStretch();
  }
  mainLayout->addStretch();

  buildPixmaps(); //uses button size
}

PartGui::SteppedSelection::~SteppedSelection()
{
  if(stepActive)
  {
    delete stepActive;
    stepActive = 0;
  }
  if (stepDone)
  {
    delete stepDone;
    stepDone = 0;
  }
}

void PartGui::SteppedSelection::buildPixmaps()
{
  assert(buttons.size() > 0);
  int iconHeight(buttons.at(0).first->height()-6);
  stepActive = new QPixmap(Gui::BitmapFactory().pixmap("Part_Measure_Step_Active").scaled
    (iconHeight, iconHeight, Qt::KeepAspectRatio));
  stepDone = new QPixmap(Gui::BitmapFactory().pixmap("Part_Measure_Step_Done").scaled
    (iconHeight, iconHeight, Qt::KeepAspectRatio));
}

void PartGui::SteppedSelection::selectionSlot(bool checked)
{
  QPushButton *sender = qobject_cast<QPushButton*>(QObject::sender());
  assert(sender != 0);
  std::vector<ButtonIconPairType>::iterator it;
  for (it = buttons.begin(); it != buttons.end(); ++it)
    if (it->first == sender)
      break;
  assert(it != buttons.end());

  if (checked)
    it->second->setPixmap(*stepActive);
  else
    it->second->setPixmap(QPixmap());
}

QPushButton* PartGui::SteppedSelection::getButton(const uint& index)
{
  return buttons.at(index).first;
}

void PartGui::SteppedSelection::setIconDone(const uint& index)
{
  buttons.at(index).second->setPixmap(*stepDone);
}

PartGui::DimensionControl::DimensionControl(QWidget* parent): QWidget(parent)
{
  QVBoxLayout *commandLayout = new QVBoxLayout();
  this->setLayout(commandLayout);

  resetButton = new QPushButton(Gui::BitmapFactory().pixmap("Part_Measure_Linear"),
				QObject::tr("Reset Dialog"), this);
  commandLayout->addWidget(resetButton);

  QPushButton *toggle3dButton = new QPushButton(Gui::BitmapFactory().pixmap("Part_Measure_Toggle_3d"),
						QObject::tr("Toggle 3d"), this);
  QObject::connect(toggle3dButton, SIGNAL(clicked(bool)), this, SLOT(toggle3dSlot(bool)));
  commandLayout->addWidget(toggle3dButton);

  QPushButton *toggleDeltaButton = new QPushButton(Gui::BitmapFactory().pixmap("Part_Measure_Toggle_Delta"),
						QObject::tr("Toggle Delta"), this);
  QObject::connect(toggleDeltaButton, SIGNAL(clicked(bool)), this, SLOT(toggleDeltaSlot(bool)));
  commandLayout->addWidget(toggleDeltaButton);

  QPushButton *clearAllButton = new QPushButton(Gui::BitmapFactory().pixmap("Part_Measure_Clear_All"),
						QObject::tr("Clear All"), this);
  QObject::connect(clearAllButton, SIGNAL(clicked(bool)), this, SLOT(clearAllSlot(bool)));
  commandLayout->addWidget(clearAllButton);
}

void PartGui::DimensionControl::toggle3dSlot(bool)
{
  PartGui::toggle3d();
}

void PartGui::DimensionControl::toggleDeltaSlot(bool)
{
  PartGui::toggleDelta();
}

void PartGui::DimensionControl::clearAllSlot(bool)
{
  PartGui::eraseAllDimensions();
}

PartGui::TaskMeasureAngular::TaskMeasureAngular()
    : Gui::SelectionObserver(true,false)
    , selections1(), selections2(), buttonSelectedIndex(0)
{
  setUpGui();
}

PartGui::TaskMeasureAngular::~TaskMeasureAngular()
{
  Gui::Selection().clearSelection();
}

void PartGui::TaskMeasureAngular::onSelectionChanged(const Gui::SelectionChanges& msg)
{
  TopoDS_Shape shape;
  Base::Matrix4D mat;
  if (!getShapeFromStrings(shape, std::string(msg.pDocName), 
              std::string(msg.pObjectName), std::string(msg.pSubName),&mat))
    return;
  mat.inverse();
  DimSelections::DimSelection newSelection;
  newSelection.documentName = msg.pDocName;
  newSelection.objectName = msg.pObjectName;
  newSelection.subObjectName = msg.pSubName;
  gp_Vec pickPoint(msg.x, msg.y, msg.z);
  Base::Vector3d v(msg.x,msg.y,msg.z);
  v = mat*v;
  newSelection.x = v.x;
  newSelection.y = v.y;
  newSelection.z = v.z;
  if (buttonSelectedIndex == 0)
  {
    if (msg.Type == Gui::SelectionChanges::AddSelection)
    {
      if (shape.ShapeType() == TopAbs_VERTEX)
      {
	//if we have previous selection it should be only one vertex.
	if (selections1.selections.size() > 1)
	  selections1.selections.clear();
	else if(selections1.selections.size() == 1)
	{
	  //make sure it is a vertex.
	  if (selections1.selections.at(0).shapeType != DimSelections::Vertex)
	    selections1.selections.clear();
	}

	newSelection.shapeType = DimSelections::Vertex;
	selections1.selections.push_back(newSelection);
	if (selections1.selections.size() == 1)
	  return;
	//here we should have 2 vertices, but will check to make sure.
	assert(selections1.selections.size() == 2);
	assert(selections1.selections.at(0).shapeType == DimSelections::Vertex);
	assert(selections1.selections.at(1).shapeType == DimSelections::Vertex);

	QTimer::singleShot(0, this, SLOT(selectionClearDelayedSlot()));
	stepped->getButton(1)->setEnabled(true);
	stepped->getButton(1)->setChecked(true);
	return;
      }

      //here there should only be one in the selections container. so just clear it.
      selections1.selections.clear();

      if (shape.ShapeType() == TopAbs_EDGE)
      {
	newSelection.shapeType = DimSelections::Edge;
	selections1.selections. push_back(newSelection);
      }

      if (shape.ShapeType() == TopAbs_FACE)
      {
	newSelection.shapeType = DimSelections::Face;
	selections1.selections.push_back(newSelection);
      }

      QTimer::singleShot(0, this, SLOT(selectionClearDelayedSlot()));
      stepped->getButton(1)->setEnabled(true);
      stepped->getButton(1)->setChecked(true);
      return;
    }
  }
  if (buttonSelectedIndex == 1)
  {
    if (msg.Type == Gui::SelectionChanges::AddSelection)
    {
      if (shape.ShapeType() == TopAbs_VERTEX)
      {
	//if we have previous selection it should be only one vertex.
	if (selections2.selections.size() > 1)
	  selections2.selections.clear();
	else if(selections2.selections.size() == 1)
	{
	  //make sure it is a vertex.
	  if (selections2.selections.at(0).shapeType != DimSelections::Vertex)
	    selections2.selections.clear();
	}

	newSelection.shapeType = DimSelections::Vertex;
	selections2.selections.push_back(newSelection);
	if (selections2.selections.size() == 1)
	  return;
	//here we should have 2 vertices, but will check to make sure.
	assert(selections2.selections.size() == 2);
	assert(selections2.selections.at(0).shapeType == DimSelections::Vertex);
	assert(selections2.selections.at(1).shapeType == DimSelections::Vertex);

	buildDimension();
	clearSelection();
	QTimer::singleShot(0, this, SLOT(selectionClearDelayedSlot()));
	stepped->getButton(0)->setChecked(true);
	stepped->getButton(1)->setEnabled(false);
	return;
      }
      //vertices have to be selected in succession. if we get here,clear temp selection.
      selections2.selections.clear();

      if (shape.ShapeType() == TopAbs_EDGE)
      {
	newSelection.shapeType = DimSelections::Edge;
	selections2.selections. push_back(newSelection);
      }

      if (shape.ShapeType() == TopAbs_FACE)
      {
	newSelection.shapeType = DimSelections::Face;
	selections2.selections.push_back(newSelection);
      }

      buildDimension();
      clearSelection();
      QTimer::singleShot(0, this, SLOT(selectionClearDelayedSlot()));
      stepped->getButton(0)->setChecked(true);
      stepped->getButton(1)->setEnabled(false);
      return;
    }
  }
}

void PartGui::TaskMeasureAngular::selectionClearDelayedSlot()
{
  //hack.
  //clearing selections are not working as I hoped. Apparently the observer callback gets called
  //before the actual selection takes place. Resulting in selections being left. this addresses this
  //by being called from the event loop.
  this->blockConnection(true);
  Gui::Selection().clearSelection();
  this->blockConnection(false);
}

PartGui::VectorAdapter PartGui::TaskMeasureAngular::buildAdapter(const PartGui::DimSelections& selection)
{
  Base::Matrix4D mat;
  assert(selection.selections.size() > 0 && selection.selections.size() < 3);
  if (selection.selections.size() == 1)
  {
    DimSelections::DimSelection current = selection.selections.at(0);
    if (current.shapeType == DimSelections::Edge)
    {
      TopoDS_Shape edgeShape;
      if (!getShapeFromStrings(edgeShape, current.documentName, current.objectName, current.subObjectName,&mat))
        return VectorAdapter();
      TopoDS_Edge edge = TopoDS::Edge(edgeShape);
      // make edge orientation so that end of edge closest to pick is head of vector.
      TopoDS_Vertex firstVertex = TopExp::FirstVertex(edge, Standard_True);
      TopoDS_Vertex lastVertex = TopExp::LastVertex(edge, Standard_True);
      if (firstVertex.IsNull() || lastVertex.IsNull())
        return VectorAdapter();
      gp_Vec firstPoint = PartGui::convert(firstVertex);
      gp_Vec lastPoint = PartGui::convert(lastVertex);
      Base::Vector3d v(current.x,current.y,current.z);
      v = mat*v;
      gp_Vec pickPoint(v.x, v.y, v.z);
      double firstDistance = (firstPoint - pickPoint).Magnitude();
      double lastDistance = (lastPoint - pickPoint).Magnitude();
      if (lastDistance > firstDistance)
      {
        if (edge.Orientation() == TopAbs_FORWARD)
          edge.Orientation(TopAbs_REVERSED);
        else
          edge.Orientation(TopAbs_FORWARD);
      }
      return VectorAdapter(edge, pickPoint);
    }
    if (current.shapeType == DimSelections::Face)
    {
      TopoDS_Shape faceShape;
      if (!getShapeFromStrings(faceShape, current.documentName, current.objectName, current.subObjectName,&mat))
	return VectorAdapter();

      TopoDS_Face face = TopoDS::Face(faceShape);
      Base::Vector3d v(current.x,current.y,current.z);
      v = mat*v;
      gp_Vec pickPoint(v.x, v.y, v.z);
      return VectorAdapter(face, pickPoint);
    }
  }
  //selection size == 2.
  DimSelections::DimSelection current1 = selection.selections.at(0);
  DimSelections::DimSelection current2 = selection.selections.at(1);
  assert(current1.shapeType == DimSelections::Vertex);
  assert(current2.shapeType == DimSelections::Vertex);
  TopoDS_Shape vertexShape1, vertexShape2;
  if(!getShapeFromStrings(vertexShape1, current1.documentName, current1.objectName, current1.subObjectName))
    return VectorAdapter();
  if(!getShapeFromStrings(vertexShape2, current2.documentName, current2.objectName, current2.subObjectName))
    return VectorAdapter();

  TopoDS_Vertex vertex1 = TopoDS::Vertex(vertexShape1);
  TopoDS_Vertex vertex2 = TopoDS::Vertex(vertexShape2);

  //build a temp adapter to make sure it is valid.
  return VectorAdapter(PartGui::convert(vertex2), PartGui::convert(vertex1));
}

void PartGui::TaskMeasureAngular::buildDimension() {
    buildDimension(selections1,selections2);
}

void PartGui::TaskMeasureAngular::buildDimension(const DimSelections &sel1, const DimSelections &sel2)
{
  //build adapters.
  VectorAdapter adapt1 = buildAdapter(sel1);
  VectorAdapter adapt2 = buildAdapter(sel2);
  
  if (!adapt1.isValid() || !adapt2.isValid())
  {
    Base::Console().Message("\ncouldn't build adapter\n\n");
    return;
  }
  auto doc = App::GetApplication().getActiveDocument();
  if(doc) 
    _Measures[doc->getName()].emplace_back(sel1,sel2,false);
  goDimensionAngularNoTask(adapt1, adapt2);
}

void PartGui::TaskMeasureAngular::clearSelection()
{
  selections1.selections.clear();
  selections2.selections.clear();
}

void PartGui::TaskMeasureAngular::setUpGui()
{
  QPixmap mainIcon = Gui::BitmapFactory().pixmap("Part_Measure_Angular");

  Gui::TaskView::TaskBox* selectionTaskBox = new Gui::TaskView::TaskBox
    (mainIcon, QObject::tr("Selections"), false, 0);
  QVBoxLayout *selectionLayout = new QVBoxLayout();
  stepped = new SteppedSelection(2, selectionTaskBox);
  selectionLayout->addWidget(stepped);
  selectionTaskBox->groupLayout()->addLayout(selectionLayout);

  Gui::TaskView::TaskBox* controlTaskBox = new Gui::TaskView::TaskBox
    (mainIcon, QObject::tr("Control"), false, 0);
  QVBoxLayout *controlLayout = new QVBoxLayout();

  DimensionControl *control = new DimensionControl(controlTaskBox);
  controlLayout->addWidget(control);
  controlTaskBox->groupLayout()->addLayout(controlLayout);
  QObject::connect(control->resetButton, SIGNAL(clicked(bool)), this, SLOT(resetDialogSlot(bool)));

  this->setButtonPosition(TaskDialog::South);
  Content.push_back(selectionTaskBox);
  Content.push_back(controlTaskBox);

  stepped->getButton(0)->setChecked(true);//before wired up.
  stepped->getButton(0)->setEnabled(true);
  QObject::connect(stepped->getButton(0), SIGNAL(toggled(bool)), this, SLOT(selection1Slot(bool)));
  QObject::connect(stepped->getButton(1), SIGNAL(toggled(bool)), this, SLOT(selection2Slot(bool)));
}

void PartGui::TaskMeasureAngular::selection1Slot(bool checked)
{
  if (checked)
  {
    buttonSelectedIndex = 0;
    this->blockConnection(true);
    Gui::Selection().clearSelection();
    std::vector<DimSelections::DimSelection>::const_iterator it;
    for (it = selections1.selections.begin(); it != selections1.selections.end(); ++it)
      Gui::Selection().addSelection(it->documentName.c_str(), it->objectName.c_str(), it->subObjectName.c_str());
    this->blockConnection(false);
  }
  else
  {
    if (selections1.selections.size() > 0)
      stepped->setIconDone(0);
  }
}

void PartGui::TaskMeasureAngular::selection2Slot(bool checked)
{
  if (checked)
    buttonSelectedIndex = 1;
  this->blockConnection(true);
  Gui::Selection().clearSelection();
  std::vector<DimSelections::DimSelection>::const_iterator it;
  for (it = selections2.selections.begin(); it != selections2.selections.end(); ++it)
    Gui::Selection().addSelection(it->documentName.c_str(), it->objectName.c_str(), it->subObjectName.c_str());
  this->blockConnection(false);
}

void PartGui::TaskMeasureAngular::resetDialogSlot(bool)
{
  clearSelection();
  this->blockConnection(true);
  Gui::Selection().clearSelection();
  stepped->getButton(0)->setChecked(true);
  stepped->getButton(1)->setEnabled(false);
  this->blockConnection(false);
}

void PartGui::TaskMeasureAngular::toggle3dSlot(bool)
{
  PartGui::toggle3d();
}

void PartGui::TaskMeasureAngular::toggleDeltaSlot(bool)
{
  PartGui::toggleDelta();
}

void PartGui::TaskMeasureAngular::clearAllSlot(bool)
{
  PartGui::eraseAllDimensions();
}

#include "moc_TaskDimension.cpp"
