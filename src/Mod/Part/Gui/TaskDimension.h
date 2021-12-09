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

#ifndef TASKDIMENSION_H
#define TASKDIMENSION_H

#include <gp_Vec.hxx>
#include <gp_Lin.hxx>

#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFMatrix.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/nodekits/SoSeparatorKit.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFRotation.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/engines/SoSubEngine.h>
#include <Inventor/engines/SoEngine.h>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Base/Matrix.h>

class TopoDS_Shape;
class TopoDS_Face;
class TopoDS_Edge;
class TopoDS_Vertex;
class gp_Pnt;
class BRepExtrema_DistShapeShape;

class QPushButton;
class QPixmap;
class QLabel;

namespace Gui{class View3dInventorViewer;}

namespace PartGui
{
  /*!find shape from selection strings
   * @param shapeOut search results.
   * @param doc document name to search.
   * @param object object name to search.
   * @param sub sub-object name to search.
   * @return signal if the search was successful.
   */
  bool getShapeFromStrings(TopoDS_Shape &shapeOut, const std::string &doc, const std::string &object, const std::string &sub, Base::Matrix4D *mat=0);
  /*!examine pre selection
   * @param shape1 first shape in current selection
   * @param shape2 second shape in current selection
   * @return signal if preselection is valid. false means shape1 and shape2 are invalid.
   */
  bool evaluateLinearPreSelection(TopoDS_Shape &shape1, TopoDS_Shape &shape2);
  /*!start of the measure linear command*/
  void goDimensionLinearRoot();
  /*!does the measure and create dimensions without a dialog
   * @param shape1 first shape.
   * @param shape2 second shape.
   * @todo incorporate some form of "adapt to topods_shape". so we can expand to other types outside OCC.
   */
  void goDimensionLinearNoTask(const TopoDS_Shape &shape1, const TopoDS_Shape &shape2);
  /*!prints results of measuring to console.
   * @param measure object containing the measure information
   */
  void dumpLinearResults(const BRepExtrema_DistShapeShape &measure);
  /*!convenience function to get the viewer*/
  Gui::View3DInventorViewer* getViewer();
  /*!adds 3d and delta dimensions to the viewer
   * @param measure object containing the measure information.
   */
  void addLinearDimensions(const BRepExtrema_DistShapeShape &measure);
  /*!creates one dimension from points with color
   * @param point1 first point
   * @param point2 second point
   * @param color color of dimension
   * @return an inventor node to add to a scenegraph
   */
  SoNode* createLinearDimension(const gp_Pnt &point1, const gp_Pnt &point2, const SbColor &color);
  /*!erases all the dimensions in the viewer.*/
  void eraseAllDimensions();
  /*!refresh all the dimensions in the viewer.*/
  void refreshDimensions();
  /*!toggles the display status of the 3d dimensions*/
  void toggle3d();
  /*!toggles the display status of the delta dimensions*/
  void toggleDelta();
  /*!make sure measure command isn't working with everything invisible. Confusing the user*/
  void ensureSomeDimensionVisible();
  /*!make sure angle measure command isn't working with 3d off. Confusing the user*/
  void ensure3dDimensionVisible();
  /*convert a vertex to vector*/
  gp_Vec convert(const TopoDS_Vertex &vertex);
  
class DimensionLinear : public SoSeparatorKit
{
  SO_KIT_HEADER(DimensionLinear);

  SO_KIT_CATALOG_ENTRY_HEADER(transformation);
  SO_KIT_CATALOG_ENTRY_HEADER(annotate);
  SO_KIT_CATALOG_ENTRY_HEADER(leftArrow);
  SO_KIT_CATALOG_ENTRY_HEADER(rightArrow);
  SO_KIT_CATALOG_ENTRY_HEADER(line);
  SO_KIT_CATALOG_ENTRY_HEADER(textSep);
public:
  DimensionLinear();
  static void initClass();
  virtual SbBool affectsState() const;
  void setupDimension();

  SoSFVec3f point1;
  SoSFVec3f point2;
  SoSFString text;
  SoSFColor dColor;
protected:
  SoSFRotation rotate;
  SoSFFloat length;
  SoSFVec3f origin;

private:
  virtual ~DimensionLinear();
};

/*kit for anglular dimensions*/
class DimensionAngular : public SoSeparatorKit
{
  SO_KIT_HEADER(DimensionAngular);

  SO_KIT_CATALOG_ENTRY_HEADER(transformation);
  SO_KIT_CATALOG_ENTRY_HEADER(annotate);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow1);
  SO_KIT_CATALOG_ENTRY_HEADER(arrow2);
  SO_KIT_CATALOG_ENTRY_HEADER(arcSep);
  SO_KIT_CATALOG_ENTRY_HEADER(textSep);
public:
  DimensionAngular();
  static void initClass();
  virtual SbBool affectsState() const;

  SoSFFloat radius;//radians.
  SoSFFloat angle;//radians.
  SoSFString text;
  SoSFColor dColor;
  SoSFMatrix matrix;
  void setupDimension();
private:
  virtual ~DimensionAngular();
};

/*used for generating points for arc display*/
class ArcEngine : public SoEngine
{
    SO_ENGINE_HEADER(ArcEngine);
public:
    ArcEngine();
    static void initClass();

    SoSFFloat radius;
    SoSFFloat angle;
    SoSFFloat deviation;

    SoEngineOutput points;
    SoEngineOutput pointCount;
protected:
    virtual void evaluate();
private:
    virtual ~ArcEngine(){}
    void defaultValues(); //some non error values if something goes wrong.
};

/*! a widget with buttons and icons for a controlled selection process*/
class SteppedSelection : public QWidget
{
  Q_OBJECT
public:
  SteppedSelection(const uint &buttonCountIn, QWidget *parent = 0);
  ~SteppedSelection();
  QPushButton* getButton(const uint &index);
  void setIconDone(const uint &index);
  
protected:
  typedef std::pair<QPushButton *, QLabel *> ButtonIconPairType;
  std::vector<ButtonIconPairType> buttons;
  QPixmap *stepActive;
  QPixmap *stepDone;
  
private Q_SLOTS:
  void selectionSlot(bool checked);
  void buildPixmaps();
  
};

/*! just convenience container*/
class DimSelections
{
public:
  enum ShapeType{None, Vertex, Edge, Face};
  struct DimSelection
  {
    std::string documentName;
    std::string objectName;
    std::string subObjectName;
    float x;
    float y;
    float z;
    ShapeType shapeType;
  };
  std::vector<DimSelection> selections;
};

/*!widget for buttons controlling the display of dimensions*/
class DimensionControl : public QWidget
{
  Q_OBJECT
public:
    explicit DimensionControl(QWidget* parent);
    QPushButton *resetButton;
public Q_SLOTS:
  void toggle3dSlot(bool);
  void toggleDeltaSlot(bool);
  void clearAllSlot(bool);
};

/*!linear dialog*/
class TaskMeasureLinear : public Gui::TaskView::TaskDialog, public Gui::SelectionObserver
{
    Q_OBJECT
public:
  TaskMeasureLinear();
  ~TaskMeasureLinear();

  virtual QDialogButtonBox::StandardButtons getStandardButtons() const
      {return QDialogButtonBox::Close;}
  virtual bool isAllowedAlterDocument(void) const {return false;}
  virtual bool needsFullSpace() const {return false;}
protected:
  virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    
protected Q_SLOTS:
  void selection1Slot(bool checked);
  void selection2Slot(bool checked);
  void resetDialogSlot(bool);
  void toggle3dSlot(bool);
  void toggleDeltaSlot(bool);
  void clearAllSlot(bool);
  void selectionClearDelayedSlot();

public:
  static void buildDimension(const DimSelections &sel1, const DimSelections &sel2);

private:
  void setUpGui();
  void buildDimension();
  void clearSelectionStrings();
  DimSelections selections1;
  DimSelections selections2;
  uint buttonSelectedIndex;
  SteppedSelection *stepped;

};

/*! @brief Convert to vector
 * 
 * Used to construct a vector from various input types
 */
class VectorAdapter
{
public:
  /*!default construction isValid is set to false*/
  VectorAdapter();
  /*!Build a vector from a faceIn
   * @param faceIn vector will be normal to plane and equal to cylindrical axis.
   * @param pickedPointIn location of pick. straight conversion from sbvec. not accurate.*/
  VectorAdapter(const TopoDS_Face &faceIn, const gp_Vec &pickedPointIn);
  /*!Build a vector from an edgeIn
   * @param edgeIn vector will be lastPoint - firstPoint.
   * @param pickedPointIn location of pick. straight conversion from sbvec. not accurate.*/
  VectorAdapter(const TopoDS_Edge &edgeIn, const gp_Vec &pickedPointIn);
  /*!Build a vector From 2 vertices.
   *vector will be equal to @param vertex2In - @param vertex1In.*/
  VectorAdapter(const TopoDS_Vertex &vertex1In, const TopoDS_Vertex &vertex2In);
  /*!Build a vector From 2 vectors.
   *vector will be equal to @param vector2 - @param vector1.*/
  VectorAdapter(const gp_Vec &vector1, const gp_Vec &vector2);
  
  /*!make sure no errors in vector construction.
   * @return true = vector is good. false = vector is NOT good.*/
  bool isValid() const {return status;}
  /*!get the calculated vector.
   * @return the vector. use isValid to ensure correct results.*/
  operator gp_Vec() const {return vector;}
  /*!build occ line used for extrema calculation*/
  operator gp_Lin() const;
  gp_Vec getPickPoint() const {return origin;}
  
private:
  void projectOriginOntoVector(const gp_Vec &pickedPointIn);
  bool status;
  gp_Vec vector;
  gp_Vec origin;
};

/*!angular dialog class*/
class TaskMeasureAngular : public Gui::TaskView::TaskDialog, public Gui::SelectionObserver
{
    Q_OBJECT
public:
  TaskMeasureAngular();
  ~TaskMeasureAngular();

  virtual QDialogButtonBox::StandardButtons getStandardButtons() const
      {return QDialogButtonBox::Close;}
  virtual bool isAllowedAlterDocument(void) const {return false;}
  virtual bool needsFullSpace() const {return false;}
protected:
  virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    
protected Q_SLOTS:
  void selection1Slot(bool checked);
  void selection2Slot(bool checked);
  void resetDialogSlot(bool);
  void toggle3dSlot(bool);
  void toggleDeltaSlot(bool);
  void clearAllSlot(bool);
  void selectionClearDelayedSlot();

public:
  static void buildDimension(const DimSelections &sel1, const DimSelections &sel2);

private:
  void buildDimension();
  void setUpGui();
  void clearSelection();
  DimSelections selections1;
  DimSelections selections2;
  uint buttonSelectedIndex;
  SteppedSelection *stepped;
  static VectorAdapter buildAdapter(const DimSelections &selection);
};

/*!start of the measure angular command*/
void goDimensionAngularRoot();
/*!examine angular pre selection
  * @param vector1Out first shape in current selection
  * @param vector2Out second shape in current selection
  * @return signal if preselection is valid. false means vector1Out and vector2Out are invalid.
  */
bool evaluateAngularPreSelection(VectorAdapter &vector1Out, VectorAdapter &vector2Out);
/*!build angular dimension*/
void goDimensionAngularNoTask(const VectorAdapter &vector1Adapter, const VectorAdapter &vector2Adapter);
}

#endif // TASKDIMENSION_H
