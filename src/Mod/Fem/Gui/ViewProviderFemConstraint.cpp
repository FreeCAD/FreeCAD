/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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
# include <QApplication>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoFontStyle.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <TopoDS.hxx>
# include <TopoDS_Vertex.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
# include <gp_Pln.hxx>
# include <gp_Cylinder.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <GProp_GProps.hxx>
# include <BRepGProp.hxx>
# include <BRepGProp_Face.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <ShapeAnalysis.hxx>
# include <BRepClass_FaceClassifier.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <GeomAPI_IntCS.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Line.hxx>
# include <Precision.hxx>
#endif

#include "ViewProviderFemConstraint.h"
#include "TaskFemConstraint.h"
#include "Gui/SoFCSelection.h"
#include "Gui/Application.h"
#include "Gui/Control.h"
#include "Gui/Command.h"
#include "Gui/Document.h"
#include "Gui/View3DInventorViewer.h"
#include "App/Document.h"


#include <App/PropertyGeo.h>
#include <App/PropertyStandard.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Fem/App/FemConstraint.h>
#include <Base/Console.h>

using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraint, Gui::ViewProviderDocumentObject)


ViewProviderFemConstraint::ViewProviderFemConstraint()
{
    ADD_PROPERTY(TextColor,(0.0f,0.0f,0.0f));
    ADD_PROPERTY(FaceColor,(1.0f,0.0f,0.2f));
    ADD_PROPERTY(FontSize,(18));
    ADD_PROPERTY(DistFactor,(1.0));
    ADD_PROPERTY(Mirror,(false));

    pFont = new SoFontStyle();
    pFont->ref();
    pLabel = new SoText2();
    pLabel->ref();
    pColor = new SoBaseColor();
    pColor->ref();
    pTextColor = new SoBaseColor();
    pTextColor->ref();
    pTranslation = new SoTranslation();
    pTranslation->ref();

    TextColor.touch();
    FontSize.touch();
    FaceColor.touch();

    pCoords = new SoCoordinate3();
    pCoords->ref();
    pCoords->point.setNum(0);

    pFaces = new SoIndexedFaceSet();
    pFaces->ref();
    pFaces->coordIndex.setNum(0);

    sPixmap = "view-femconstraint";

    normalDirection = new SbVec3f(0, 0, 1);
    arrowDirection = NULL;
}

ViewProviderFemConstraint::~ViewProviderFemConstraint()
{
    pFont->unref();
    pLabel->unref();
    pColor->unref();
    pTextColor->unref();
    pTranslation->unref();
    pCoords->unref();
    pFaces->unref();
    delete arrowDirection;
    delete normalDirection;
}

std::vector<App::DocumentObject*> ViewProviderFemConstraint::claimChildren(void)const
{
    return std::vector<App::DocumentObject*>();
}

void ViewProviderFemConstraint::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit constraint"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    ViewProviderDocumentObject::setupContextMenu(menu, receiver, member);
}

void ViewProviderFemConstraint::onChanged(const App::Property* prop)
{
    if (this->getObject() != NULL)
        Base::Console().Error("%s: onChanged: %s\n", this->getObject()->getNameInDocument(), prop->getName());
    else
        Base::Console().Error("Anonymous: onChanged: %s\n", prop->getName());

    if (prop == &Mirror || prop == &DistFactor) {
        updateData(prop);
    }
    else if (prop == &TextColor) {
        const App::Color& c = TextColor.getValue();
        pTextColor->rgb.setValue(c.r,c.g,c.b);
    }
    else if (prop == &FaceColor) {
        const App::Color& c = FaceColor.getValue();
        pColor->rgb.setValue(c.r,c.g,c.b);
    }
    else if (prop == &FontSize) {
        pFont->size = FontSize.getValue();
    }
    else {
        ViewProviderDocumentObject::onChanged(prop);
    }
}

bool ViewProviderFemConstraint::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this constraint the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgFemConstraint *constrDlg = qobject_cast<TaskDlgFemConstraint *>(dlg);
        if (constrDlg && constrDlg->getConstraintView() != this)
            constrDlg = 0; // another constraint left open its task panel
        if (dlg && !constrDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().closeDialog();
            else
                return false;
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (constrDlg)
            Gui::Control().showDialog(constrDlg);
        else
            Gui::Control().showDialog(new TaskDlgFemConstraint(this));

        return true;
    }
    else {
        return ViewProviderDocumentObject::setEdit(ModNum);
    }
}

void ViewProviderFemConstraint::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDocumentObject::unsetEdit(ModNum);
    }
}

std::vector<std::string> ViewProviderFemConstraint::getDisplayModes(void) const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.push_back("Base");
    return StrList;
}

void ViewProviderFemConstraint::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

void ViewProviderFemConstraint::attach(App::DocumentObject* pcObject)
{
    ViewProviderDocumentObject::attach(pcObject);

    SoPickStyle* ps = new SoPickStyle();
    ps->style = SoPickStyle::UNPICKABLE;

    SoSeparator *faceSep = new SoSeparator();
    faceSep->addChild(ps);
    faceSep->addChild(pColor);
    faceSep->addChild(pCoords);
    faceSep->addChild(pFaces);

    SoSeparator* sep = new SoSeparator();
    sep->addChild(faceSep);
    addDisplayMaskMode(sep, "Base");
}

// Create a local coordinate system with the z-axis given in dir
void getLocalCoordinateSystem(const SbVec3f& z, SbVec3f& y, SbVec3f& x)
{
    // Find the y axis in an arbitrary direction, normal to z
    // Conditions:
    // y1 * z1 + y2 * z2 + y3 * z3 = |y| |z| cos(90°) = 0
    // |y| = sqrt(y1^2 + y2^2 + y3^2) = 1
    float z1, z2, z3;
    z.getValue(z1, z2, z3);
    float y1, y2, y3;
    if (fabs(z1) > Precision::Confusion()) {
        // Choose: y3 = 0
        // Solution:
        // y1 * z1 + y2 * z2 = 0
        // y1 = - z2/z1 y2
        // sqrt(z2^2/z1^2 y2^2 + y2^2) = 1
        // y2^2 ( 1 + z2^2/z1^2)) = +-1 -> choose +1 otherwise no solution
        // y2 = +- sqrt(1 / (1 + z2^2/z1^2))
        y3 = 0;
        y2 = sqrt(1 / (1 + z2*z2 / (z1*z1)));
        y1 = -z2/z1 * y2;
        // Note: result might be (0, 1, 0)
    } else if (fabs(z2) > Precision::Confusion()) {
        // Given: z1 = 0
        // Choose: y1 = 0
        // Solution:
        // y2 * z2 + y3 * z3 = 0
        // y2 = - z3/z2 y3
        // sqrt(z3^2/z2^2 y3^3 + y3^2) = 1
        // y3^2 (1 + z3^2/z2^2)) = +1
        // y3 = +- sqrt(1 / (1 + z3^2/z2^2))
        y1 = 0;
        y3 = sqrt(1 / (1 + z3*z3 / (z2*z2)));
        y2 = -z3/z2 * y3;
        // Note: result might be (0, 0, 1)
    } else if  (fabs(z3) > Precision::Confusion()) {
        // Given: z1 = z2 = 0
        // Choose the remaining possible axis
        y1 = 1;
        y2 = 0;
        y3 = 0;
    }

    y = SbVec3f(y1, y2, y3);
    x = y.cross(z);
}

#define FACETS 12
#define CONEPOINTS (FACETS + 1)
#define CONEFACETPOINTS (FACETS * 4 + FACETS + 1)

void createCone(SoMFVec3f& point, SoMFInt32& refs, const int ipoints, const int ifaces, const SbVec3f& base, const SbVec3f& dir,
                const double height, const double radius, const bool update = false)
{
    SbVec3f x, y;
    getLocalCoordinateSystem(dir, y, x);

    point.set1Value(ipoints, base); // tip

    SbVec3f midpoint(base + dir * height); // centre of the circle
    for (int i = 0; i < FACETS; i++) {
        float angle = 2 * M_PI / FACETS * i;
        point.set1Value(ipoints + i + 1, midpoint + cos(angle) * x * radius + sin(angle) * y * radius);
    }

    if (update)
        return;

    int32_t faces[CONEFACETPOINTS];
    int start_index = 1;
    for (int f = 0; f < FACETS; f++) {
        faces[f * 4]     = ipoints; // tip of arrow
        int idx = start_index;
        faces[f * 4 + 1] = ipoints + idx;
        idx++;
        if (idx > FACETS) idx = 1; // Happens in the last iteration
        faces[f * 4 + 2] = ipoints + idx;
        faces[f * 4 + 3] = -1;
        start_index++;
    }
    for (int f = 0; f < FACETS; f++)
        faces[FACETS * 4 + f]     = ipoints + f + 1;
    faces[CONEFACETPOINTS - 1] = -1;
    refs.setValues(ifaces, CONEFACETPOINTS, faces);
}

#define CYLPOINTS (FACETS * 2)
#define CYLFACETPOINTS (FACETS * 5 + 2 * FACETS + 2)

void createCylinder(SoMFVec3f& point, SoMFInt32& refs, const int ipoints, const int ifaces, const SbVec3f& base, const SbVec3f& dir,
                    const double height, const double radius, const bool update = false)
{
    SbVec3f x, y;
    getLocalCoordinateSystem(dir, y, x);

    for (int i = 0; i < CYLPOINTS; i+=2) {
        float angle = 2 * M_PI / FACETS * i/2;
        point.set1Value(ipoints + i,     base                + cos(angle) * x * radius + sin(angle) * y * radius);
        point.set1Value(ipoints + i + 1, base + dir * height + cos(angle) * x * radius + sin(angle) * y * radius);
    }

    if (update)
        return;

    int32_t faces[CYLFACETPOINTS];
    int start_index = 0;
    for (int f = 0; f < FACETS; f++) {
        int idx = start_index;
        faces[f * 5]     = ipoints + idx;
        idx++;
        faces[f * 5 + 1] = ipoints + idx;
        idx++;
        if (idx >= CYLPOINTS) idx = 0; // Happens in the last iteration
        faces[f * 5 + 3] = ipoints + idx;
        idx++;
        faces[f * 5 + 2] = ipoints + idx;
        faces[f * 5 + 4] = -1;
        start_index += 2;
    }
    for (int f = 0; f < FACETS; f++) {
        faces[FACETS * 5 + f] = ipoints + 2 * f;
        faces[FACETS * 5 + FACETS + f + 1] = ipoints + 1 + 2 * f;
    }
    faces[FACETS * 5 + FACETS] = -1;
    faces[CYLFACETPOINTS - 1] = -1;
    refs.setValues(ifaces, CYLFACETPOINTS, faces);
}

#define ARROWPOINTS (CONEPOINTS + CYLPOINTS)
#define ARROWFACETPOINTS (CONEFACETPOINTS + CYLFACETPOINTS)

void createArrow(SoMFVec3f& point, SoMFInt32& refs, const int ipoints, const int ifaces, const SbVec3f& base, const SbVec3f& dir,
                 const double length, const double radius, const bool update = false)
{
    createCone(point, refs, ipoints, ifaces, base, dir, radius, radius, update);
    createCylinder(point, refs, ipoints + CONEPOINTS, ifaces + CONEFACETPOINTS, base + dir * radius, dir, length-radius, radius/3, update);
}

#define BOXPOINTS 8
#define BOXFACEPOINTS 30

void createBox(SoMFVec3f& point, SoMFInt32& refs, const int ipoints, const int ifaces, const SbVec3f& base, const SbVec3f& dir,
               const double width, const double length, const double height, const bool update = false)
{
    SbVec3f x, y;
    getLocalCoordinateSystem(dir, y, x);

    point.set1Value(ipoints,   base + width/2 * y + length/2 * x);
    point.set1Value(ipoints+1, base + width/2 * y - length/2 * x);
    point.set1Value(ipoints+2, base - width/2 * y - length/2 * x);
    point.set1Value(ipoints+3, base - width/2 * y + length/2 * x);
    point.set1Value(ipoints+4, base + dir * height + width/2 * y + length/2 * x);
    point.set1Value(ipoints+5, base + dir * height + width/2 * y - length/2 * x);
    point.set1Value(ipoints+6, base + dir * height - width/2 * y - length/2 * x);
    point.set1Value(ipoints+7, base + dir * height - width/2 * y + length/2 * x);

    if (update)
        return;

    int32_t faces[BOXFACEPOINTS] = {
        ipoints, ipoints+1, ipoints+2, ipoints+3, -1,
        ipoints, ipoints+1, ipoints+5, ipoints+4, -1,
        ipoints+1, ipoints+2, ipoints+6, ipoints+5, -1,
        ipoints+2, ipoints+3, ipoints+7, ipoints+6, -1,
        ipoints+3, ipoints, ipoints+4, ipoints+7, -1,
        ipoints+4, ipoints+5, ipoints+6, ipoints+7, -1};
    refs.setValues(ifaces, BOXFACEPOINTS, faces);
}

void ViewProviderFemConstraint::findCylinderData(SbVec3f& z, SbVec3f& y, SbVec3f& x, SbVec3f& p, double& radius, double& height) {
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(this->getObject());
    std::vector<App::DocumentObject*> Objects = pcConstraint->References.getValues();
    std::vector<std::string> SubElements = pcConstraint->References.getSubValues();
    if (Objects.empty())
        return;
    App::DocumentObject* obj = Objects[0];
    Part::Feature* feat = static_cast<Part::Feature*>(obj);
    TopoDS_Shape sh = feat->Shape.getShape().getSubShape(SubElements[0].c_str());

    TopoDS_Face face = TopoDS::Face(sh);
    BRepAdaptor_Surface surface(face);
    gp_Cylinder cyl = surface.Cylinder();
    gp_Pnt start = surface.Value(surface.FirstUParameter(), surface.FirstVParameter());
    gp_Pnt end   = surface.Value(surface.FirstUParameter(), surface.LastVParameter());
    height = start.Distance(end);
    radius = cyl.Radius();
    gp_Dir dirz = cyl.Axis().Direction();
    z = SbVec3f(dirz.X(), dirz.Y(), dirz.Z());
    gp_Dir diry = cyl.YAxis().Direction();
    y = SbVec3f(diry.X(), diry.Y(), diry.Z());
    gp_Dir dirx = cyl.XAxis().Direction();
    x = SbVec3f(dirx.X(), dirx.Y(), dirx.Z());

    if (pcConstraint->Location.getValue() == NULL) {
        // Get a point in the middle of the cylindrical face.
        gp_Pnt centre = cyl.Location();
        SbVec3f base(centre.X(), centre.Y(), centre.Z());
        p = base + z * height/2;
    } else {
        // Get the point specified by Location and Distance
        App::DocumentObject* objLoc = pcConstraint->Location.getValue();
        std::string subName = pcConstraint->Location.getSubValues().front();
        Part::Feature* featLoc = static_cast<Part::Feature*>(objLoc);
        TopoDS_Shape shloc = featLoc->Shape.getShape().getSubShape(subName.c_str());
        // Get a plane from the Location reference
        gp_Pln plane;
        if (shloc.ShapeType() == TopAbs_FACE) {
            BRepAdaptor_Surface surface(TopoDS::Face(shloc));
            plane = surface.Plane();
        } else {
            BRepAdaptor_Curve curve(TopoDS::Edge(shloc));
            gp_Lin line = curve.Line();
            gp_Dir tang = line.Direction().Crossed(dirz);
            gp_Dir norm = line.Direction().Crossed(tang);
            plane = gp_Pln(line.Location(), norm);
        }
        // Translate the plane in direction of the cylinder (for positive values of Distance)
        Handle_Geom_Plane pln = new Geom_Plane(plane);
        GeomAPI_ProjectPointOnSurf proj(cyl.Location(), pln);
        if (!proj.IsDone())
            return;
        gp_Pnt projPnt = proj.NearestPoint();
        plane.Translate(gp_Vec(projPnt, cyl.Location()).Normalized().Multiplied(pcConstraint->Distance.getValue()));
        Handle_Geom_Plane plnt = new Geom_Plane(plane);
        // Intersect translated plane with cylinder axis
        Handle_Geom_Curve crv = new Geom_Line(cyl.Axis());
        GeomAPI_IntCS intersector(crv, plnt);
        if (!intersector.IsDone())
            return;
        gp_Pnt inter = intersector.Point(1);
        p.setValue(inter.X(), inter.Y(), inter.Z());
    }
}

void ViewProviderFemConstraint::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    if (this->getObject() != NULL)
        Base::Console().Error("%s: updateData: %s\n", this->getObject()->getNameInDocument(), prop->getName());
    else
        Base::Console().Error("Anonymous: updateData: %s\n", prop->getName());
    Fem::Constraint* pcConstraint = static_cast<Fem::Constraint*>(this->getObject());

    if (strcmp(prop->getName(),"References") == 0) {
        const App::PropertyLinkSubList* pr = static_cast<const App::PropertyLinkSubList*>(prop);
        std::vector<App::DocumentObject*> Objects = pr->getValues();
        std::vector<std::string> SubElements = pr->getSubValues();

        // Remove all arrows
        pCoords->point.deleteValues(0, pCoords->point.getNum());
        pFaces->coordIndex.deleteValues(0, pFaces->coordIndex.getNum());
        if (Objects.empty()) {
            Base::Console().Error("   updateData: No references\n");
            Objects = pcConstraint->References.getValues();
            SubElements = pcConstraint->References.getSubValues();
            if (Objects.empty())
                return;
        }

        Base::Console().Error("   updateData: Found %u references\n", Objects.size());

        // Re-create all arrows
        int type = pcConstraint->Type.getValue();

        if ((type == 0) || (type == 1)) {
            // Force on geometry
            std::vector<gp_Pnt> points;
            TopoDS_Shape sh;

            for (int i = 0; i < Objects.size(); i++) {
                App::DocumentObject* obj = Objects[i];
                Part::Feature* feat = static_cast<Part::Feature*>(obj);
                const Part::TopoShape& toposhape = feat->Shape.getShape();
                if (toposhape.isNull()) {
                    Base::Console().Error("   updateData: Empty toposhape\n");
                    return;
                }
                sh = toposhape.getSubShape(SubElements[i].c_str());

                if (sh.ShapeType() == TopAbs_VERTEX) {
                    const TopoDS_Vertex& vertex = TopoDS::Vertex(sh);
                    gp_Pnt p = BRep_Tool::Pnt(vertex);
                    points.push_back(p);
                } else if (sh.ShapeType() == TopAbs_EDGE) {
                    BRepAdaptor_Curve curve(TopoDS::Edge(sh));
                    double fp = curve.FirstParameter();
                    double lp = curve.LastParameter();
                    GProp_GProps props;
                    BRepGProp::LinearProperties(sh, props);
                    double l = props.Mass();
                    int steps = round(l / 3); // TODO: Make number of steps depend on actual screen size of element!
                    double step = (lp - fp) / steps;
                    if (steps < 1) {
                        points.push_back(curve.Value(fp));
                        points.push_back(curve.Value(lp));
                    } else {
                        for (int i = 0; i < steps + 1; i++)
                            points.push_back(curve.Value(i * step));
                    }
                } else if (sh.ShapeType() == TopAbs_FACE) {
                    TopoDS_Face face = TopoDS::Face(sh);
                    BRepAdaptor_Surface surface(face);
                    double ufp = surface.FirstUParameter();
                    double ulp = surface.LastUParameter();
                    double vfp = surface.FirstVParameter();
                    double vlp = surface.LastVParameter();
                    double ustep = (ulp - ufp) / 6.0;
                    double vstep = (vlp - vfp) / 6.0;
                    // TODO: How to find the distance between ufp and ulp to get the number of steps?
                    for (int i = 0; i < 7; i++) {
                        for (int j = 0; j < 7; j++) {
                            gp_Pnt p = surface.Value(ufp + i * ustep, vfp + j * vstep);
                            BRepClass_FaceClassifier classifier(face, p, Precision::Confusion());
                            if (classifier.State() != TopAbs_OUT)
                                points.push_back(p);
                        }
                    }
                }
            }

            // Get default direction (on first call to method)
            if (arrowDirection == NULL) {
                if (sh.ShapeType() == TopAbs_FACE) {
                    // Get face normal in center point
                    TopoDS_Face face = TopoDS::Face(sh);
                    BRepGProp_Face prop(face);
                    gp_Vec normal;
                    gp_Pnt center;
                    double u1,u2,v1,v2;
                    prop.Bounds(u1,u2,v1,v2);
                    prop.Normal((u1+u2)/2.0,(v1+v2)/2.0,center,normal);
                    normal.Normalize();
                    normalDirection->setValue(normal.X(), normal.Y(), normal.Z());
                } // else use z axis

                arrowDirection = new SbVec3f(*normalDirection);
            }

            if (type == 0) {
                // Force on geometry
                pCoords->point.setNum(ARROWPOINTS * points.size());
                pFaces->coordIndex.setNum(ARROWFACETPOINTS * points.size());
                int index = 0;

                for (std::vector<gp_Pnt>::const_iterator p = points.begin(); p != points.end(); p++) {
                    SbVec3f v(p->X(), p->Y(), p->Z());
                    if (*arrowDirection != *normalDirection) // Turn arrow around
                        v = v + *normalDirection * 5.0;
                    createArrow(pCoords->point, pFaces->coordIndex,
                                index * ARROWPOINTS, index * ARROWFACETPOINTS,
                                v, *arrowDirection, 5.0, 1.0);
                    index++;
                }
            } else if (type == 1) {
                // Fixed
                pCoords->point.setNum((CONEPOINTS + BOXPOINTS) * points.size());
                pFaces->coordIndex.setNum((CONEFACETPOINTS + BOXFACEPOINTS) * points.size());
                int index = 0;

                for (std::vector<gp_Pnt>::const_iterator p = points.begin(); p != points.end(); p++) {
                    SbVec3f v(p->X(), p->Y(), p->Z());
                    createCone(pCoords->point, pFaces->coordIndex,
                               index * (CONEPOINTS + BOXPOINTS), index * (CONEFACETPOINTS + BOXFACEPOINTS),
                               v, *normalDirection, 2.0, 1.0);
                    createBox(pCoords->point, pFaces->coordIndex,
                              index * (CONEPOINTS + BOXPOINTS) + CONEPOINTS, index * (CONEFACETPOINTS + BOXFACEPOINTS) + CONEFACETPOINTS,
                              v + *normalDirection * 2.0, *normalDirection, 2.0, 2.0, 0.5);
                    index++;
                }
             }
        } else if ((type == 2) || (type == 3)) {
            // Bearing. Note that only one face is allowed for this constraint
            SbVec3f z, y, x, p;
            double radius, height;
            findCylinderData(z, y, x, p, radius, height);
            p = p + y * radius;

            pCoords->point.setNum(CONEPOINTS + BOXPOINTS);
            pFaces->coordIndex.setNum(CONEFACETPOINTS + BOXFACEPOINTS);
            if (type == 2)
                // axial free
                createCone(pCoords->point, pFaces->coordIndex, 0, 0, p, y, radius/2.5, radius/4);
            else
                // axial fixed
                createCone(pCoords->point, pFaces->coordIndex, 0, 0, p, y, radius/2, radius/4);

            createBox(pCoords->point, pFaces->coordIndex, CONEPOINTS, CONEFACETPOINTS, p + y * radius/2, y, radius, radius, radius/10);
        } else if ((type == 4) || (type == 5)) {
            // Pulley, Gear
            SbVec3f z, y, x, p;
            double radius, height;
            findCylinderData(z, y, x, p, radius, height);

            double dia = pcConstraint->Diameter.getValue();
            if (dia < Precision::Confusion())
                dia = radius * 4;
            double otherdia = pcConstraint->OtherDiameter.getValue();
            if (otherdia < Precision::Confusion())
                otherdia = radius * 2;
            double centerdist = pcConstraint->CenterDistance.getValue();
            if (fabs(centerdist) < Precision::Confusion())
                centerdist = 500;

            if (type == 4) {
                // Pulley
                pCoords->point.setNum(CYLPOINTS + 2 * ARROWPOINTS);
                pFaces->coordIndex.setNum(CYLFACETPOINTS + 2 * ARROWFACETPOINTS);
                createCylinder(pCoords->point, pFaces->coordIndex, 0, 0, p - z * height * 0.4, z, height * 0.8, dia);

                double angle = asin((dia - otherdia)/2/centerdist);
                SbVec3f p1 = p + y * dia * cos(angle) + x * dia * sin(angle);
                SbVec3f dir1 = x - y * sin(angle);
                dir1.normalize();
                p1 = p1 + dir1 * 2 * radius;
                dir1.negate();
                SbVec3f p2 = p - y * dia * cos(angle) + x * dia * sin(angle);
                SbVec3f dir2 = x + y * sin(angle);
                dir2.normalize();
                p2 = p2 + dir2 * 2 * radius;
                dir2.negate();
                createArrow(pCoords->point, pFaces->coordIndex, CYLPOINTS, CYLFACETPOINTS, p1, dir1, 2 * radius, radius/5);
                createArrow(pCoords->point, pFaces->coordIndex, CYLPOINTS+ARROWPOINTS, CYLFACETPOINTS+ARROWFACETPOINTS, p2, dir2, 2 * radius, radius/5);
            } else if (type == 5) {
                // Gear
                pCoords->point.setNum(CYLPOINTS + ARROWPOINTS);
                pFaces->coordIndex.setNum(CYLFACETPOINTS + ARROWFACETPOINTS);
                createCylinder(pCoords->point, pFaces->coordIndex, 0, 0, p - z * height * 0.4, z, height * 0.8, dia);
                SbVec3f p1 = p + y * dia;
                createArrow(pCoords->point, pFaces->coordIndex, CYLPOINTS, CYLFACETPOINTS, p1, x, radius, radius/5);
            }
        }
    } else if (strcmp(prop->getName(),"Direction") == 0) {
        if (arrowDirection == NULL)
            return;
        const App::PropertyLinkSub* pr = static_cast<const App::PropertyLinkSub*>(prop);
        App::DocumentObject* obj = pr->getValue();
        std::vector<std::string> names = pr->getSubValues();
        if (names.size() == 0)
            return;
        std::string subName = names.front();
        Part::Feature* feat = static_cast<Part::Feature*>(obj);
        TopoDS_Shape sh = feat->Shape.getShape().getSubShape(subName.c_str());

        if (sh.ShapeType() == TopAbs_FACE) {
            BRepAdaptor_Surface surface(TopoDS::Face(sh));
            if (surface.GetType() == GeomAbs_Plane) {
                gp_Dir dir = surface.Plane().Axis().Direction();
                arrowDirection->setValue(dir.X(), dir.Y(), dir.Z());
            } else {
                return; // Not a planar face
            }
        } else if (sh.ShapeType() == TopAbs_EDGE) {
            BRepAdaptor_Curve line(TopoDS::Edge(sh));
            if (line.GetType() == GeomAbs_Line) {
                gp_Dir dir = line.Line().Direction();
                arrowDirection->setValue(dir.X(), dir.Y(), dir.Z());
            } else {
                return; // Not a linear edge
            }
        }

        // TODO: Check whether direction points inside or outside of solid? But for which reference?

        arrowDirection->normalize();

        *normalDirection = *arrowDirection;
        bool reversed = pcConstraint->Reversed.getValue();
        if (reversed)
            arrowDirection->negate();

        // Re-orient all arrows
        int numArrows = pCoords->point.getNum()/ARROWPOINTS;

        for (int i = 0; i < numArrows; i++) {
            // Note: for update=true the pFaces->coordIndex is not touched
            SbVec3f p = pCoords->point[i * ARROWPOINTS];
            if (reversed)
                p = p + *normalDirection * 5.0;
            createArrow(pCoords->point, pFaces->coordIndex,
                        i * ARROWPOINTS, 0,
                        p, *arrowDirection, 5.0, 1.0, true);
        }
    } else if (strcmp(prop->getName(),"Reversed") == 0) {
        if (arrowDirection == NULL)
            return;
        bool reversed = static_cast<const App::PropertyBool*>(prop)->getValue();
        bool isReversed = (*arrowDirection != *normalDirection);
        if (reversed == isReversed)
            return;

        *arrowDirection = *normalDirection;
        if (reversed)
            arrowDirection->negate();

        // Reverse all arrows
        int numArrows = pCoords->point.getNum()/ARROWPOINTS;

        for (int i = 0; i < numArrows; i++) {
            createArrow(pCoords->point, pFaces->coordIndex,
                       i * ARROWPOINTS, 0,
                       pCoords->point[i * ARROWPOINTS], *arrowDirection, 5.0, 1.0, true);
        }
    } else if ((strcmp(prop->getName(),"Location") == 0) || (strcmp(prop->getName(),"Distance") == 0)) {
        // Move bearing constraint
        SbVec3f z, y, x, p;
        double radius, height;
        findCylinderData(z, y, x, p, radius, height);

        int type = pcConstraint->Type.getValue();
        if (type == 2) {
            // axial free
            createCone(pCoords->point, pFaces->coordIndex, 0, 0, p, y, radius/2.5, radius/4, true);
            createBox(pCoords->point, pFaces->coordIndex, CONEPOINTS, CONEFACETPOINTS, p + y * radius/2, y, radius, radius, radius/10, true);
        } else if (type == 3) {
            // axial fixed
            createCone(pCoords->point, pFaces->coordIndex, 0, 0, p, y, radius/2, radius/4, true);
            createBox(pCoords->point, pFaces->coordIndex, CONEPOINTS, CONEFACETPOINTS, p + y * radius/2, y, radius, radius, radius/10, true);
        } else if ((type == 4) || (type == 5)) {
            createCylinder(pCoords->point, pFaces->coordIndex, 0, 0, p - z * height * 0.4, z, height * 0.8, pcConstraint->Diameter.getValue(), true);
        }
    } else if ((strcmp(prop->getName(),"Diameter") == 0) || (strcmp(prop->getName(),"OtherDiameter") == 0) ||
               (strcmp(prop->getName(),"CenterDistance") == 0)) {
        // Update pulley/gear constraint
        SbVec3f z, y, x, p;
        double radius, height;
        findCylinderData(z, y, x, p, radius, height);

        double dia = pcConstraint->Diameter.getValue();
        if (dia < Precision::Confusion())
            dia = radius * 4;
        double otherdia = pcConstraint->OtherDiameter.getValue();
        if (otherdia < Precision::Confusion())
            otherdia = radius * 2;
        double centerdist = pcConstraint->CenterDistance.getValue();
        if (fabs(centerdist) < Precision::Confusion())
            centerdist = 500;
        int type = pcConstraint->Type.getValue();

        if (type == 4) {
            // Pulley
            createCylinder(pCoords->point, pFaces->coordIndex, 0, 0, p - z * height * 0.4, z, height * 0.8, dia, true);

            double angle = asin((dia - otherdia)/2/centerdist);
            SbVec3f p1 = p + y * dia * cos(angle) + x * dia * sin(angle);
            SbVec3f dir1 = x - y * sin(angle);
            dir1.normalize();
            p1 = p1 + dir1 * 2 * radius;
            dir1.negate();
            SbVec3f p2 = p - y * dia * cos(angle) + x * dia * sin(angle);
            SbVec3f dir2 = x + y * sin(angle);
            dir2.normalize();
            p2 = p2 + dir2 * 2 * radius;
            dir2.negate();
            createArrow(pCoords->point, pFaces->coordIndex, CYLPOINTS, CYLFACETPOINTS, p1, dir1, 2 * radius, radius/5, true);
            createArrow(pCoords->point, pFaces->coordIndex, CYLPOINTS+ARROWPOINTS, CYLFACETPOINTS+ARROWFACETPOINTS, p2, dir2, 2 * radius, radius/5, true);
        } else if (type == 5) {
            // Gear
            createCylinder(pCoords->point, pFaces->coordIndex, 0, 0, p - z * height * 0.4, z, height * 0.8, dia, true);
            SbVec3f p1 = p + y * dia;
            createArrow(pCoords->point, pFaces->coordIndex, CYLPOINTS, CYLFACETPOINTS, p1, x, 2 * radius, radius/5, true);
        }
    }
    ViewProviderDocumentObject::updateData(prop);
}
