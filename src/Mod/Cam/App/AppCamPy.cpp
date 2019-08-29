/***************************************************************************
 *   Copyright (c) 2007                                                    *
 *   Joachim Zettler <Joachim.Zettler@gmx.de>                              *
 *   Jürgen Riegel <Juergen.Riegel@web.de                                  *
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

# if defined (_POSIX_C_SOURCE)
#   undef  _POSIX_C_SOURCE
# endif // (re-)defined in pyconfig.h
#include <cmath>

#ifndef PI
#define PI M_PI
#endif

//Basic Stuff
#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Handle.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Builder3D.h>
#include <App/Application.h>
#include <App/Document.h>


//Part Stuff
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapePy.h>

//Mesh Stuff
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/TopoAlgorithm.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/Builder.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/Core/Elements.h>
#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/Triangulation.h>

//OCC Stuff
#include <Poly_Triangulation.hxx>
#include <BRepOffsetAPI_MakeOffsetShape.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepBndLib.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepMesh.hxx>
#include <Geom_OffsetSurface.hxx>
#include <GeomAPI_PointsToBSplineSurface.hxx>
#include <TColgp_HArray2OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <BRepGProp.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <GProp_PrincipalProps.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Handle_Geom_Plane.hxx>
#include <Geom_Plane.hxx>
#include <Geom2d_Curve.hxx>
#include <gp_Lin.hxx>
#include <gp_Dir.hxx>

//Own Stuff
#include "Approx.h"
#include "ConvertDyna.h"
#include "cutting_tools.h"
#include "best_fit.h"
#include "SpringbackCorrection.h"


using namespace Part;
using namespace Mesh;
using namespace std;
using MeshCore::MeshKernel;


/* module functions */
static PyObject *
open(PyObject *self, PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    PY_TRY
    {


    } PY_CATCH;

    Py_Return;
}


/* module functions */
static PyObject * insert(PyObject *self, PyObject *args)
{
    char* Name;
    const char* DocName;
    if (!PyArg_ParseTuple(args, "ets","utf-8",&Name,&DocName))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    PY_TRY
    {

    } PY_CATCH;

    Py_Return;
}

/* module functions */
static PyObject * read(PyObject *self, PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);
    PY_TRY
    {

    } PY_CATCH;
    Py_Return;
}


static PyObject * tesselateShape(PyObject *self, PyObject *args)
{

    PyObject *pcObj;
    float aDeflection;
    //PyObject *pcObj2;
    if (!PyArg_ParseTuple(args, "O!f", &(TopoShapePy::Type), &pcObj, &aDeflection))    // convert args: Python->C
        return NULL;                             // NULL triggers exception

    TopoShapePy *pcShape = static_cast<TopoShapePy*>(pcObj); //Surface oder Step-File wird übergeben


    Base::Builder3D aBuild;

    MeshCore::MeshKernel mesh;
    MeshCore::MeshBuilder builder(mesh);
    builder.Initialize(1000);
    Base::Vector3f Points[3];

    PY_TRY
    {
        // removes all the triangulations of the faces ,
        //and all the polygons on the triangulations of the edges:
        TopoDS_Shape aShape = pcShape->getTopoShapePtr()->_Shape;
        BRepTools::Clean(aShape);

        // adds a triangulation of the shape aShape with the deflection aDeflection:
        //BRepMesh_IncrementalMesh Mesh(pcShape->getShape(),aDeflection);

        BRepMesh::Mesh(aShape,aDeflection);
        TopExp_Explorer aExpFace;
        for (aExpFace.Init(aShape,TopAbs_FACE);aExpFace.More();aExpFace.Next())
        {
            TopoDS_Face aFace = TopoDS::Face(aExpFace.Current());
            TopLoc_Location aLocation;
            // takes the triangulation of the face aFace:
            Handle_Poly_Triangulation aTr = BRep_Tool::Triangulation(aFace,aLocation);
            if (!aTr.IsNull()) // if this triangulation is not NULL
            {
                // takes the array of nodes for this triangulation:
                const TColgp_Array1OfPnt& aNodes = aTr->Nodes();
                // takes the array of triangles for this triangulation:
                const Poly_Array1OfTriangle& triangles = aTr->Triangles();
                // create array of node points in absolute coordinate system
                TColgp_Array1OfPnt aPoints(1, aNodes.Length());
                for ( Standard_Integer i = 1; i < aNodes.Length()+1; i++)
                    aPoints(i) = aNodes(i).Transformed(aLocation);
                // Takes the node points of each triangle of this triangulation.
                // takes a number of triangles:
                Standard_Integer nnn = aTr->NbTriangles();
                Standard_Integer nt,n1,n2,n3;
                for ( nt = 1 ; nt < nnn+1 ; nt++)
                {
                    // takes the node indices of each triangle in n1,n2,n3:
                    triangles(nt).Get(n1,n2,n3);
                    // takes the node points:
                    gp_Pnt aPnt1 = aPoints(n1);
                    Points[0].Set(float(aPnt1.X()),float(aPnt1.Y()),float(aPnt1.Z()));
                    gp_Pnt aPnt2 = aPoints(n2);
                    Points[1].Set(float(aPnt2.X()),float(aPnt2.Y()),float(aPnt2.Z()));
                    gp_Pnt aPnt3 = aPoints(n3);
                    Points[2].Set(float(aPnt3.X()),float(aPnt3.Y()),float(aPnt3.Z()));
                    // give the occ faces to the internal mesh structure of freecad
                    MeshCore::MeshGeomFacet Face(Points[0],Points[1],Points[2]);
                    builder.AddFacet(Face);

                }

            }
            // if the triangulation of only one face is not possible to get
            else
            {
                throw Base::RuntimeError("Empty face triangulation\n");
            }
        }
        // finish FreeCAD Mesh Builder and exit with new mesh
        builder.Finish();
        /*return new MeshPy(&mesh);*/
    } PY_CATCH;

    Py_Return;
}

static PyObject * best_fit_coarse(PyObject *self, PyObject *args)
{
    PyObject *pcObj2;

    if (!PyArg_ParseTuple(args, "O!; Need one Mesh objects and one toposhape", &(TopoShapePy::Type), &pcObj2))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY
    {

        TopoShapePy *pcShape = static_cast<TopoShapePy*>(pcObj2); //Shape wird übergeben
        TopoDS_Shape cad           = pcShape->getTopoShapePtr()->_Shape;  // Input CAD


        //best_fit befi(cad);
        //best_fit::Tesselate_Shape(pcShape->getShape(),mesh,0.1);

        //return new MeshPy(&befi.m_CadMesh);

        //return new TopoShapePyOld(befi.m_Cad);

        /*
        befi.MeshFit_Coarse();
        */

    }PY_CATCH;

    Py_Return;
}


#include <GeomAdaptor_Curve.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>

//static PyObject * makeToolPath(PyObject *self, PyObject *args)
//{
//// Py::List AllCuts = Py::List();
//// double offset=0.0;
//    ofstream anoutput,anoutput2;
//    anoutput.open("c:/bspline_output.txt");
//    anoutput2.open("c:/bspline2_output.txt");
//    PyObject *pcObj;
//    //PyObject *pcObj2;
//    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePyOld::Type), &pcObj))    // convert args: Python->C
//        return NULL;                             // NULL triggers exception
//
//    TopoShapePyOld *pcShape = static_cast<TopoShapePyOld*>(pcObj); //Surface wird übergeben
//// TopoShapePyOld *pcShape2 = static_cast<TopoShapePyOld*>(pcObj2); //Cut-Curve
//    PY_TRY
//    {
//
//        cutting_tools anewCuttingEnv(pcShape->getShape(),10.0);
//        anewCuttingEnv.arrangecuts_ZLEVEL();
//        std::vector<std::pair<float,TopoDS_Shape> > aTestOutput = anewCuttingEnv.getCutShape();
//        BRep_Builder BB;
//
//        TopoDS_Compound aCompound;
//
//        BB.MakeCompound(aCompound);
//        for (unsigned int i=0;i<aTestOutput.size();++i)
//        {
//
//            BB.Add(aCompound,TopoDS::Compound(aTestOutput[i].second));
//        }
//
//        anewCuttingEnv.OffsetWires_Standard(10.0);
//
//        std::vector<Handle_Geom_BSplineCurve> topCurves;
//        std::vector<Handle_Geom_BSplineCurve> botCurves;
//        std::vector<Handle_Geom_BSplineCurve>::iterator an_it;
//        topCurves = *(anewCuttingEnv.getOutputhigh());
//        botCurves = *(anewCuttingEnv.getOutputlow());
//        for (unsigned int i=0;i<topCurves.size();++i)
//        {
//            GeomAdaptor_Curve aCurveAdaptor(topCurves[i]);
//            GCPnts_QuasiUniformDeflection aPointGenerator(aCurveAdaptor,0.1);
//            for (int t=1;t<=aPointGenerator.NbPoints();++t)
//            {
//                anoutput << (aPointGenerator.Value(t)).X() <<","<< (aPointGenerator.Value(t)).Y() <<","<<(aPointGenerator.Value(t)).Z()<<std::endl;
//            }
//        }
//        for (unsigned int i=0;i<botCurves.size();++i)
//        {
//            GeomAdaptor_Curve aCurveAdaptor(botCurves[i]);
//            GCPnts_QuasiUniformDeflection aPointGenerator(aCurveAdaptor,0.1);
//            for (int t=1;t<=aPointGenerator.NbPoints();++t)
//            {
//                anoutput2 << (aPointGenerator.Value(t)).X() <<","<< (aPointGenerator.Value(t)).Y() <<","<<(aPointGenerator.Value(t)).Z()<<std::endl;
//            }
//        }
//        anoutput.close();
//        anoutput2.close();
//
//        //botCurves.push_back(*(topCurves.begin()));
//
//        //path_simulate path(topCurves , botCurves);
//        ////path.MakePathSimulate();
//        //path.MakePathRobot();
//
//        //GeomAdaptor_Curve anAdaptorCurve;
//        //for(an_it=topCurves.begin();an_it<topCurves.end();++an_it)
//        //{
//        // anAdaptorCurve.Load(*an_it);
//        // double length = GCPnts_AbscissaPoint::Length(anAdaptorCurve);
//        // std::cout << "Length: " << length << std::endl;
//        //}
//
//        return new TopoShapePyOld(aCompound);
//
//
//    } PY_CATCH;
//
//    Py_Return;
//}
//
//

static PyObject * offset(PyObject *self,PyObject *args)
{
    double offset;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!d",&(TopoShapePy::Type), &pcObj,&offset ))
        return NULL;

    TopoShapePy *pcShape = static_cast<TopoShapePy*>(pcObj); //Original-Shape wird hier übergeben

    PY_TRY
    {

        BRepOffsetAPI_MakeOffsetShape MakeOffsetShape (pcShape->getTopoShapePtr()->_Shape,
            offset,0.001,BRepOffset_Skin);
        return new TopoShapePy(new TopoShape(MakeOffsetShape.Shape()));


    } PY_CATCH;

}


/*
static PyObject * cut(PyObject *self, PyObject *args)
{
 PyObject *pcObj;
 double z_pitch;
 //double rGap = 1000.0; //Rand um die Bounding Box für ein sauberes Ergebnis
 if (!PyArg_ParseTuple(args, "O!d", &(TopoShapePyOld::Type), &pcObj,&z_pitch))     // convert args: Python->C
  return NULL;                             // NULL triggers exception

 TopoShapePyOld *pcShape =  static_cast<TopoShapePyOld*>(pcObj); //Surface to cut
 //TopoShapePyOld *pcShape2 = static_cast<TopoShapePyOld*>(pcObj2); //Cutting Plane


 ofstream outputfile;
 outputfile.open("c:/allpoints.out");

 PY_TRY
 {


   Base::Builder3D logit;


   Jetzt die eigentlichen Schnitte erzeugen:
   1. Wenn die oberste Ebene ein flacher Bereich ist, werden von dort die Bounding Wires genommen
    Ermittlung über die Bounding Box
   2. Anschließend über die Differenz von zwei Flat-Bereichen die Anzahl von Schnitten ermitteln mit gegebenem Abstand
   3. Die Edges bzw. Wires in B-Spline Kurven wandeln und anschließend evaluieren
   4. Abfahrreihenfolge festlegen und Output für die Simulation bzw. Versuch vorbereiten




     //builder.Add(totalwire,mkWire.Wire());






     BRep_Builder buildface;
     TopoDS_Face topoface;
     buildface.MakeFace(topoface,asecondPlane,0.001);
     //TopoDS_Wire wire = TopoDS::Wire(explore_cut.Current());

     //Exp_Wire.Init(wire);
     explore_cut.ReInit();
     for(; explore_cut.More(); explore_cut.Next())
     {
      TopoDS_Edge edge = TopoDS::Edge(explore_cut.Current());
      BRepAdaptor_Curve cutedge(edge);

      Handle(BRepAdaptor_HCurve) hadapt_cutedge = new BRepAdaptor_HCurve(cutedge);
      BRepAdaptor_Surface cutplane(topoface);
      Handle(BRepAdaptor_HSurface) hadapt_cutplane = new BRepAdaptor_HSurface(cutplane);
      intersect.Perform(hadapt_cutedge,hadapt_cutplane);
      int numberofpoints = 0;
      numberofpoints = intersect.NbPoints();
      if (numberofpoints > 0)
      {
       IntCurveSurface_IntersectionPoint pointofintersect = intersect.Point(1);
       gp_Pnt actualIntersectPoint = pointofintersect.Pnt();
       logit.addSinglePoint(actualIntersectPoint.X(),actualIntersectPoint.Y(),actualIntersectPoint.Z(),10);
      }
     }

    }
   }

  }
  for(int i =0;i<allcutPoints.size();++i)
  {
   outputfile <<  allcutPoints[i].X() << "," << allcutPoints[i].Y() << "," << allcutPoints[i].Z() <<endl;
  }
  outputfile.close();
  logit.saveToFile("c:/test.iv");
  return new TopoShapePyOld( totalwire);

}PY_CATCH;

}
*/

/* Approximate test function */

static PyObject * createTestApproximate(PyObject *self, PyObject *args)
{
    if (! PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY
    {

/////////////////////////////////////////////////////////////////
// Approximation of surface.
// Building a BSpline surface which approximates a set of points.
/////////////////////////////////////////////////////////////////
        /*
        // creating a set of points:
        Standard_Real Step = 175;
        Standard_Integer Upper = 4;

          //a set of X and Y coordinates:
            Standard_Real aXStep = Step , aYStep = Step ;
            Standard_Real aX0 = -300, aY0 = -200;
          //Z coordinates:
            TColStd_Array2OfReal aZPoints( 1, Upper , 1, Upper );

        // initializing array of Z coordinates:
        // aZPoints(1,1) = -2;
        // aZPoints(1,2) = 3;
        // ...

        //creating a approximate BSpline surface:
        Parameters of surface:
        DegMin = 3;
        DegMax = 9;
        Continuity = GeomAbs_C1;
        Tolerance = 0.7;

        GeomAPI_PointsToBSplineSurface aPTBS;
        aPTBS.Init(aZPoints,aX0,aXStep,aY0,aYStep,
                   DegMin,DegMax,Continuity,Tolerance);
        Handle_Geom_BSplineSurface aSurface = aPTBS.Surface();






        */



        TColgp_Array2OfPnt Input(1,2,1,3);
        Input.SetValue(1,1,gp_Pnt(20,20,0));
        Input.SetValue(1,2,gp_Pnt(25,25,0));
        Input.SetValue(1,3,gp_Pnt(30,30,0));
        Input.SetValue(2,1,gp_Pnt(18,35,0));
        Input.SetValue(2,2,gp_Pnt(23,37,-3));
        Input.SetValue(2,3,gp_Pnt(40,40,0));





        GeomAPI_PointsToBSplineSurface *Approx_Surface = new GeomAPI_PointsToBSplineSurface(Input, 3, 8, GeomAbs_C2,0.001);
        Handle(Geom_BSplineSurface) Final_Approx = Approx_Surface->Surface () ;

        BRepBuilderAPI_MakeFace  Face(Final_Approx);

        return new TopoShapePy(new TopoShape(Face.Face()));
    } PY_CATCH;
}
/* BREP test function */
static PyObject *
createTestBSPLINE(PyObject *self, PyObject *args)
{
    //const char* Name;
    if (! PyArg_ParseTuple(args, ""))
        return NULL;

    PY_TRY
    {
        TColgp_Array2OfPnt Poles(1,48,1,48);


        Poles.SetValue(1,1,gp_Pnt(-150.004,-150.032,0.000561847));
        Poles.SetValue(1,2,gp_Pnt(-150.002,-142.327,-0.00168953));
        Poles.SetValue(1,3,gp_Pnt(-150,-126.92,-0.00028435));
        Poles.SetValue(1,4,gp_Pnt(-150,-103.831,-0.000519289));
        Poles.SetValue(1,5,gp_Pnt(-150,-84.6001,-0.000265669));
        Poles.SetValue(1,6,gp_Pnt(-150,-71.1405,-0.00162853));
        Poles.SetValue(1,7,gp_Pnt(-150,-63.4497,0.00623225));
        Poles.SetValue(1,8,gp_Pnt(-150,-57.6819,-0.00552693));
        Poles.SetValue(1,9,gp_Pnt(-150,-51.9139,-0.000608209));
        Poles.SetValue(1,10,gp_Pnt(-150.001,-47.107,-0.000761491));
        Poles.SetValue(1,11,gp_Pnt(-150.001,-43.2611,-0.000606976));
        Poles.SetValue(1,12,gp_Pnt(-150.001,-40.3767,0.000301703));
        Poles.SetValue(1,13,gp_Pnt(-150.001,-37.492,0.00136356));
        Poles.SetValue(1,14,gp_Pnt(-150.001,-34.6077,-0.000772864));
        Poles.SetValue(1,15,gp_Pnt(-150.001,-32.2036,-0.00110298));
        Poles.SetValue(1,16,gp_Pnt(-150.001,-30.2806,-0.000821554));
        Poles.SetValue(1,17,gp_Pnt(-150.001,-27.3959,-0.000341639));
        Poles.SetValue(1,18,gp_Pnt(-150.002,-24.0305,0.00107516));
        Poles.SetValue(1,19,gp_Pnt(-150.002,-20.1843,0.000545201));
        Poles.SetValue(1,20,gp_Pnt(-150.003,-16.3382,-0.0036344));
        Poles.SetValue(1,21,gp_Pnt(-150.003,-12.4921,-0.00133336));
        Poles.SetValue(1,22,gp_Pnt(-150.003,-8.64594,0.000602981));
        Poles.SetValue(1,23,gp_Pnt(-150.004,-5.76127,0.000740304));
        Poles.SetValue(1,24,gp_Pnt(-150.004,-1.91501,-0.000669011));
        Poles.SetValue(1,25,gp_Pnt(-150.004,1.93125,0.00331739));
        Poles.SetValue(1,26,gp_Pnt(-150.003,5.77745,0.00628266));
        Poles.SetValue(1,27,gp_Pnt(-150.003,8.6621,0.00185797));
        Poles.SetValue(1,28,gp_Pnt(-150.002,12.5083,0.00117201));
        Poles.SetValue(1,29,gp_Pnt(-150.002,16.3543,0.00359695));
        Poles.SetValue(1,30,gp_Pnt(-150.001,20.2004,-0.000866218));
        Poles.SetValue(1,31,gp_Pnt(-150.001,24.0465,0.0024382));
        Poles.SetValue(1,32,gp_Pnt(-150.001,27.4119,0.000782921));
        Poles.SetValue(1,33,gp_Pnt(-150.001,30.2965,0.00019153));
        Poles.SetValue(1,34,gp_Pnt(-150.001,32.2196,0.000181438));
        Poles.SetValue(1,35,gp_Pnt(-150.001,34.6234,-0.00175203));
        Poles.SetValue(1,36,gp_Pnt(-150,37.5078,0.00248825));
        Poles.SetValue(1,37,gp_Pnt(-150.001,40.3923,0.000162624));
        Poles.SetValue(1,38,gp_Pnt(-150.001,43.2767,-0.00043742));
        Poles.SetValue(1,39,gp_Pnt(-150.001,47.1224,-0.00243046));
        Poles.SetValue(1,40,gp_Pnt(-150,51.9291,0.00271265));
        Poles.SetValue(1,41,gp_Pnt(-150,57.6971,-0.00206532));
        Poles.SetValue(1,42,gp_Pnt(-150,63.4651,-0.000689487));
        Poles.SetValue(1,43,gp_Pnt(-150,71.1557,0.000696594));
        Poles.SetValue(1,44,gp_Pnt(-150,84.6148,-0.00159957));
        Poles.SetValue(1,45,gp_Pnt(-150,103.843,0.00079398));
        Poles.SetValue(1,46,gp_Pnt(-150,126.921,0.00325858));
        Poles.SetValue(1,47,gp_Pnt(-150,142.308,-0.00205645));
        Poles.SetValue(1,48,gp_Pnt(-150.001,150.002,0.000266785));
        Poles.SetValue(2,1,gp_Pnt(-142.31,-150.021,-0.00181926));
        Poles.SetValue(2,2,gp_Pnt(-142.309,-142.316,-0.126204));
        Poles.SetValue(2,3,gp_Pnt(-142.305,-126.903,-0.0907513));
        Poles.SetValue(2,4,gp_Pnt(-142.304,-103.808,-0.172784));
        Poles.SetValue(2,5,gp_Pnt(-142.302,-84.571,-0.256387));
        Poles.SetValue(2,6,gp_Pnt(-142.298,-71.1085,-0.215109));
        Poles.SetValue(2,7,gp_Pnt(-142.301,-63.4178,-0.321454));
        Poles.SetValue(2,8,gp_Pnt(-142.302,-57.6491,-0.362883));
        Poles.SetValue(2,9,gp_Pnt(-142.304,-51.8812,-0.411528));
        Poles.SetValue(2,10,gp_Pnt(-142.305,-47.0752,-0.435599));
        Poles.SetValue(2,11,gp_Pnt(-142.305,-43.2302,-0.438131));
        Poles.SetValue(2,12,gp_Pnt(-142.305,-40.3466,-0.445607));
        Poles.SetValue(2,13,gp_Pnt(-142.305,-37.4617,-0.436859));
        Poles.SetValue(2,14,gp_Pnt(-142.305,-34.5771,-0.442155));
        Poles.SetValue(2,15,gp_Pnt(-142.304,-32.1727,-0.43872));
        Poles.SetValue(2,16,gp_Pnt(-142.305,-30.2502,-0.452521));
        Poles.SetValue(2,17,gp_Pnt(-142.307,-27.3666,-0.474689));
        Poles.SetValue(2,18,gp_Pnt(-142.311,-24.0038,-0.521193));
        Poles.SetValue(2,19,gp_Pnt(-142.314,-20.1589,-0.56475));
        Poles.SetValue(2,20,gp_Pnt(-142.319,-16.3146,-0.619306));
        Poles.SetValue(2,21,gp_Pnt(-142.321,-12.4685,-0.646119));
        Poles.SetValue(2,22,gp_Pnt(-142.321,-8.62227,-0.646118));
        Poles.SetValue(2,23,gp_Pnt(-142.321,-5.73782,-0.634708));
        Poles.SetValue(2,24,gp_Pnt(-142.317,-1.89155,-0.588682));
        Poles.SetValue(2,25,gp_Pnt(-142.316,1.95492,-0.560158));
        Poles.SetValue(2,26,gp_Pnt(-142.316,5.80123,-0.547648));
        Poles.SetValue(2,27,gp_Pnt(-142.313,8.68585,-0.518341));
        Poles.SetValue(2,28,gp_Pnt(-142.312,12.532,-0.500341));
        Poles.SetValue(2,29,gp_Pnt(-142.311,16.3777,-0.499537));
        Poles.SetValue(2,30,gp_Pnt(-142.307,20.2226,-0.465716));
        Poles.SetValue(2,31,gp_Pnt(-142.307,24.0679,-0.460146));
        Poles.SetValue(2,32,gp_Pnt(-142.304,27.4314,-0.438242));
        Poles.SetValue(2,33,gp_Pnt(-142.304,30.315,-0.433771));
        Poles.SetValue(2,34,gp_Pnt(-142.304,32.2374,-0.432231));
        Poles.SetValue(2,35,gp_Pnt(-142.306,34.6411,-0.446475));
        Poles.SetValue(2,36,gp_Pnt(-142.306,37.525,-0.445412));
        Poles.SetValue(2,37,gp_Pnt(-142.307,40.4085,-0.44998));
        Poles.SetValue(2,38,gp_Pnt(-142.306,43.2916,-0.431981));
        Poles.SetValue(2,39,gp_Pnt(-142.305,47.1355,-0.408878));
        Poles.SetValue(2,40,gp_Pnt(-142.303,51.94,-0.366882));
        Poles.SetValue(2,41,gp_Pnt(-142.301,57.7052,-0.307906));
        Poles.SetValue(2,42,gp_Pnt(-142.301,63.4721,-0.30131));
        Poles.SetValue(2,43,gp_Pnt(-142.302,71.1622,-0.256453));
        Poles.SetValue(2,44,gp_Pnt(-142.302,84.6149,-0.220375));
        Poles.SetValue(2,45,gp_Pnt(-142.31,103.846,-0.23077));
        Poles.SetValue(2,46,gp_Pnt(-142.304,126.92,0.0181769));
        Poles.SetValue(2,47,gp_Pnt(-142.309,142.309,-0.0738464));
        Poles.SetValue(2,48,gp_Pnt(-142.307,150.001,0.000863329));
        Poles.SetValue(3,1,gp_Pnt(-126.921,-150,0.00165292));
        Poles.SetValue(3,2,gp_Pnt(-126.921,-142.291,-0.108971));
        Poles.SetValue(3,3,gp_Pnt(-126.916,-126.875,-0.280815));
        Poles.SetValue(3,4,gp_Pnt(-126.912,-103.76,-0.548913));
        Poles.SetValue(3,5,gp_Pnt(-126.904,-84.5133,-0.647195));
        Poles.SetValue(3,6,gp_Pnt(-126.909,-71.0494,-0.979561));
        Poles.SetValue(3,7,gp_Pnt(-126.905,-63.3539,-0.997419));
        Poles.SetValue(3,8,gp_Pnt(-126.904,-57.5844,-1.07424));
        Poles.SetValue(3,9,gp_Pnt(-126.902,-51.8141,-1.11764));
        Poles.SetValue(3,10,gp_Pnt(-126.901,-47.0063,-1.16545));
        Poles.SetValue(3,11,gp_Pnt(-126.904,-43.162,-1.23177));
        Poles.SetValue(3,12,gp_Pnt(-126.906,-40.2796,-1.27218));
        Poles.SetValue(3,13,gp_Pnt(-126.91,-37.3996,-1.3403));
        Poles.SetValue(3,14,gp_Pnt(-126.913,-34.5199,-1.37958));
        Poles.SetValue(3,15,gp_Pnt(-126.918,-32.1209,-1.42493));
        Poles.SetValue(3,16,gp_Pnt(-126.919,-30.1996,-1.43471));
        Poles.SetValue(3,17,gp_Pnt(-126.92,-27.3171,-1.44871));
        Poles.SetValue(3,18,gp_Pnt(-126.916,-23.9503,-1.43521));
        Poles.SetValue(3,19,gp_Pnt(-126.915,-20.1051,-1.43151));
        Poles.SetValue(3,20,gp_Pnt(-126.906,-16.2565,-1.38855));
        Poles.SetValue(3,21,gp_Pnt(-126.903,-12.4114,-1.40876));
        Poles.SetValue(3,22,gp_Pnt(-126.9,-8.56614,-1.41947));
        Poles.SetValue(3,23,gp_Pnt(-126.897,-5.68165,-1.41744));
        Poles.SetValue(3,24,gp_Pnt(-126.899,-1.83662,-1.46074));
        Poles.SetValue(3,25,gp_Pnt(-126.896,2.00777,-1.45305));
        Poles.SetValue(3,26,gp_Pnt(-126.895,5.85236,-1.4119));
        Poles.SetValue(3,27,gp_Pnt(-126.9,8.73579,-1.42133));
        Poles.SetValue(3,28,gp_Pnt(-126.903,12.58,-1.39958));
        Poles.SetValue(3,29,gp_Pnt(-126.905,16.4245,-1.35132));
        Poles.SetValue(3,30,gp_Pnt(-126.914,20.2695,-1.36455));
        Poles.SetValue(3,31,gp_Pnt(-126.911,24.1114,-1.3157));
        Poles.SetValue(3,32,gp_Pnt(-126.913,27.4746,-1.29886));
        Poles.SetValue(3,33,gp_Pnt(-126.911,30.3549,-1.2689));
        Poles.SetValue(3,34,gp_Pnt(-126.909,32.275,-1.24747));
        Poles.SetValue(3,35,gp_Pnt(-126.904,34.6731,-1.20445));
        Poles.SetValue(3,36,gp_Pnt(-126.901,37.5522,-1.17054));
        Poles.SetValue(3,37,gp_Pnt(-126.898,40.4322,-1.14411));
        Poles.SetValue(3,38,gp_Pnt(-126.898,43.3131,-1.12165));
        Poles.SetValue(3,39,gp_Pnt(-126.899,47.1552,-1.1049));
        Poles.SetValue(3,40,gp_Pnt(-126.903,51.9593,-1.08453));
        Poles.SetValue(3,41,gp_Pnt(-126.906,57.7251,-1.02902));
        Poles.SetValue(3,42,gp_Pnt(-126.909,63.4889,-0.984029));
        Poles.SetValue(3,43,gp_Pnt(-126.912,71.1726,-0.939984));
        Poles.SetValue(3,44,gp_Pnt(-126.916,84.6287,-0.75009));
        Poles.SetValue(3,45,gp_Pnt(-126.914,103.84,-0.446948));
        Poles.SetValue(3,46,gp_Pnt(-126.922,126.917,-0.208932));
        Poles.SetValue(3,47,gp_Pnt(-126.919,142.304,-0.0369009));
        Poles.SetValue(3,48,gp_Pnt(-126.921,150,-0.00113251));
        Poles.SetValue(4,1,gp_Pnt(-103.843,-150,-0.0010153));
        Poles.SetValue(4,2,gp_Pnt(-103.841,-142.278,-0.149614));
        Poles.SetValue(4,3,gp_Pnt(-103.841,-126.835,-0.557018));
        Poles.SetValue(4,4,gp_Pnt(-103.832,-103.697,-1.04331));
        Poles.SetValue(4,5,gp_Pnt(-103.825,-84.4296,-1.51248));
        Poles.SetValue(4,6,gp_Pnt(-103.814,-70.9483,-1.71721));
        Poles.SetValue(4,7,gp_Pnt(-103.81,-63.2492,-1.91447));
        Poles.SetValue(4,8,gp_Pnt(-103.805,-57.475,-2.0133));
        Poles.SetValue(4,9,gp_Pnt(-103.802,-51.7032,-2.17279));
        Poles.SetValue(4,10,gp_Pnt(-103.796,-46.8927,-2.30177));
        Poles.SetValue(4,11,gp_Pnt(-103.786,-43.0402,-2.38814));
        Poles.SetValue(4,12,gp_Pnt(-103.778,-40.152,-2.44243));
        Poles.SetValue(4,13,gp_Pnt(-103.765,-37.2574,-2.50059));
        Poles.SetValue(4,14,gp_Pnt(-103.753,-34.3676,-2.54694));
        Poles.SetValue(4,15,gp_Pnt(-103.741,-31.9551,-2.60633));
        Poles.SetValue(4,16,gp_Pnt(-103.734,-30.0306,-2.65752));
        Poles.SetValue(4,17,gp_Pnt(-103.725,-27.1449,-2.73151));
        Poles.SetValue(4,18,gp_Pnt(-103.722,-23.7866,-2.82704));
        Poles.SetValue(4,19,gp_Pnt(-103.716,-19.9474,-2.91344));
        Poles.SetValue(4,20,gp_Pnt(-103.726,-16.1157,-3.04938));
        Poles.SetValue(4,21,gp_Pnt(-103.727,-12.2811,-3.06216));
        Poles.SetValue(4,22,gp_Pnt(-103.73,-8.44811,-3.0722));
        Poles.SetValue(4,23,gp_Pnt(-103.734,-5.57452,-3.08626));
        Poles.SetValue(4,24,gp_Pnt(-103.733,-1.74218,-3.02862));
        Poles.SetValue(4,25,gp_Pnt(-103.739,2.09027,-3.04208));
        Poles.SetValue(4,26,gp_Pnt(-103.743,5.92168,-3.09969));
        Poles.SetValue(4,27,gp_Pnt(-103.739,8.795,-3.08108));
        Poles.SetValue(4,28,gp_Pnt(-103.737,12.6264,-3.09153));
        Poles.SetValue(4,29,gp_Pnt(-103.739,16.4571,-3.11293));
        Poles.SetValue(4,30,gp_Pnt(-103.733,20.2881,-3.01659));
        Poles.SetValue(4,31,gp_Pnt(-103.744,24.1243,-2.98515));
        Poles.SetValue(4,32,gp_Pnt(-103.749,27.4798,-2.90047));
        Poles.SetValue(4,33,gp_Pnt(-103.759,30.3609,-2.8434));
        Poles.SetValue(4,34,gp_Pnt(-103.766,32.282,-2.80723));
        Poles.SetValue(4,35,gp_Pnt(-103.781,34.6883,-2.77627));
        Poles.SetValue(4,36,gp_Pnt(-103.792,37.5714,-2.73126));
        Poles.SetValue(4,37,gp_Pnt(-103.803,40.4561,-2.66247));
        Poles.SetValue(4,38,gp_Pnt(-103.811,43.3376,-2.62498));
        Poles.SetValue(4,39,gp_Pnt(-103.819,47.1799,-2.53849));
        Poles.SetValue(4,40,gp_Pnt(-103.824,51.9789,-2.42499));
        Poles.SetValue(4,41,gp_Pnt(-103.828,57.7369,-2.30022));
        Poles.SetValue(4,42,gp_Pnt(-103.83,63.4966,-2.12922));
        Poles.SetValue(4,43,gp_Pnt(-103.832,71.1786,-1.8623));
        Poles.SetValue(4,44,gp_Pnt(-103.837,84.6178,-1.53092));
        Poles.SetValue(4,45,gp_Pnt(-103.85,103.844,-1.13325));
        Poles.SetValue(4,46,gp_Pnt(-103.848,126.916,-0.481826));
        Poles.SetValue(4,47,gp_Pnt(-103.846,142.306,-0.137169));
        Poles.SetValue(4,48,gp_Pnt(-103.844,150,0.000586788));
        Poles.SetValue(5,1,gp_Pnt(-84.614,-150,0.000270488));
        Poles.SetValue(5,2,gp_Pnt(-84.6141,-142.269,-0.159149));
        Poles.SetValue(5,3,gp_Pnt(-84.6159,-126.815,-0.660259));
        Poles.SetValue(5,4,gp_Pnt(-84.6086,-103.651,-1.4185));
        Poles.SetValue(5,5,gp_Pnt(-84.5935,-84.3582,-2.06391));
        Poles.SetValue(5,6,gp_Pnt(-84.5714,-70.8586,-2.4464));
        Poles.SetValue(5,7,gp_Pnt(-84.5628,-63.1503,-2.67335));
        Poles.SetValue(5,8,gp_Pnt(-84.5613,-57.3748,-2.91676));
        Poles.SetValue(5,9,gp_Pnt(-84.5613,-51.6011,-3.07405));
        Poles.SetValue(5,10,gp_Pnt(-84.5691,-46.7952,-3.20612));
        Poles.SetValue(5,11,gp_Pnt(-84.5837,-42.9554,-3.33601));
        Poles.SetValue(5,12,gp_Pnt(-84.5994,-40.0793,-3.44324));
        Poles.SetValue(5,13,gp_Pnt(-84.6194,-37.2057,-3.54474));
        Poles.SetValue(5,14,gp_Pnt(-84.6441,-34.3358,-3.66032));
        Poles.SetValue(5,15,gp_Pnt(-84.6655,-31.9433,-3.73538));
        Poles.SetValue(5,16,gp_Pnt(-84.6811,-30.0273,-3.78877));
        Poles.SetValue(5,17,gp_Pnt(-84.704,-27.1526,-3.87363));
        Poles.SetValue(5,18,gp_Pnt(-84.7235,-23.7921,-3.94992));
        Poles.SetValue(5,19,gp_Pnt(-84.7453,-19.953,-4.04994));
        Poles.SetValue(5,20,gp_Pnt(-84.7487,-16.1062,-4.06117));
        Poles.SetValue(5,21,gp_Pnt(-84.761,-12.2635,-4.21439));
        Poles.SetValue(5,22,gp_Pnt(-84.7646,-8.4186,-4.30051));
        Poles.SetValue(5,23,gp_Pnt(-84.7638,-5.53345,-4.32728));
        Poles.SetValue(5,24,gp_Pnt(-84.7694,-1.6881,-4.45068));
        Poles.SetValue(5,25,gp_Pnt(-84.762,2.15614,-4.45867));
        Poles.SetValue(5,26,gp_Pnt(-84.7563,6.00003,-4.40673));
        Poles.SetValue(5,27,gp_Pnt(-84.7609,8.88229,-4.44703));
        Poles.SetValue(5,28,gp_Pnt(-84.7604,12.7235,-4.42816));
        Poles.SetValue(5,29,gp_Pnt(-84.7534,16.563,-4.35447));
        Poles.SetValue(5,30,gp_Pnt(-84.754,20.3999,-4.39459));
        Poles.SetValue(5,31,gp_Pnt(-84.7327,24.2293,-4.30122));
        Poles.SetValue(5,32,gp_Pnt(-84.7173,27.5803,-4.27802));
        Poles.SetValue(5,33,gp_Pnt(-84.6977,30.4479,-4.21241));
        Poles.SetValue(5,34,gp_Pnt(-84.6843,32.3595,-4.16497));
        Poles.SetValue(5,35,gp_Pnt(-84.6617,34.7453,-4.05325));
        Poles.SetValue(5,36,gp_Pnt(-84.642,37.6106,-3.97243));
        Poles.SetValue(5,37,gp_Pnt(-84.626,40.4793,-3.88558));
        Poles.SetValue(5,38,gp_Pnt(-84.613,43.3488,-3.76512));
        Poles.SetValue(5,39,gp_Pnt(-84.602,47.1786,-3.6382));
        Poles.SetValue(5,40,gp_Pnt(-84.5979,51.9706,-3.48883));
        Poles.SetValue(5,41,gp_Pnt(-84.5955,57.7241,-3.24621));
        Poles.SetValue(5,42,gp_Pnt(-84.5981,63.4805,-3.0356));
        Poles.SetValue(5,43,gp_Pnt(-84.6055,71.1595,-2.79784));
        Poles.SetValue(5,44,gp_Pnt(-84.622,84.6125,-2.27334));
        Poles.SetValue(5,45,gp_Pnt(-84.632,103.829,-1.5763));
        Poles.SetValue(5,46,gp_Pnt(-84.6319,126.911,-0.827433));
        Poles.SetValue(5,47,gp_Pnt(-84.6202,142.302,-0.277168));
        Poles.SetValue(5,48,gp_Pnt(-84.616,150,-0.00084759));
        Poles.SetValue(6,1,gp_Pnt(-71.1545,-150,0.000818217));
        Poles.SetValue(6,2,gp_Pnt(-71.1559,-142.265,-0.171988));
        Poles.SetValue(6,3,gp_Pnt(-71.1636,-126.808,-0.877511));
        Poles.SetValue(6,4,gp_Pnt(-71.156,-103.621,-1.6021));
        Poles.SetValue(6,5,gp_Pnt(-71.1356,-84.3088,-2.3871));
        Poles.SetValue(6,6,gp_Pnt(-71.1082,-70.796,-2.8955));
        Poles.SetValue(6,7,gp_Pnt(-71.0937,-63.0781,-3.24057));
        Poles.SetValue(6,8,gp_Pnt(-71.0799,-57.2886,-3.46587));
        Poles.SetValue(6,9,gp_Pnt(-71.0655,-51.499,-3.70299));
        Poles.SetValue(6,10,gp_Pnt(-71.0546,-46.6771,-3.94985));
        Poles.SetValue(6,11,gp_Pnt(-71.0462,-42.8205,-4.15299));
        Poles.SetValue(6,12,gp_Pnt(-71.0381,-39.9275,-4.28235));
        Poles.SetValue(6,13,gp_Pnt(-71.0329,-37.0353,-4.43768));
        Poles.SetValue(6,14,gp_Pnt(-71.026,-34.1434,-4.56256));
        Poles.SetValue(6,15,gp_Pnt(-71.0223,-31.7352,-4.68719));
        Poles.SetValue(6,16,gp_Pnt(-71.0206,-29.8116,-4.79297));
        Poles.SetValue(6,17,gp_Pnt(-71.0187,-26.9274,-4.95523));
        Poles.SetValue(6,18,gp_Pnt(-71.0182,-23.5685,-5.15222));
        Poles.SetValue(6,19,gp_Pnt(-71.0127,-19.7304,-5.35227));
        Poles.SetValue(6,20,gp_Pnt(-71.0136,-15.8989,-5.56506));
        Poles.SetValue(6,21,gp_Pnt(-71.0096,-12.0712,-5.68795));
        Poles.SetValue(6,22,gp_Pnt(-71.0107,-8.25242,-5.79463));
        Poles.SetValue(6,23,gp_Pnt(-71.0139,-5.39299,-5.88817));
        Poles.SetValue(6,24,gp_Pnt(-71.0145,-1.5833,-5.93167));
        Poles.SetValue(6,25,gp_Pnt(-71.0292,2.22315,-5.99844));
        Poles.SetValue(6,26,gp_Pnt(-71.0411,6.02965,-6.03414));
        Poles.SetValue(6,27,gp_Pnt(-71.0445,8.88553,-5.98762));
        Poles.SetValue(6,28,gp_Pnt(-71.0538,12.6963,-5.9483));
        Poles.SetValue(6,29,gp_Pnt(-71.0673,16.5105,-5.90522));
        Poles.SetValue(6,30,gp_Pnt(-71.0725,20.328,-5.76435));
        Poles.SetValue(6,31,gp_Pnt(-71.0851,24.1504,-5.66147));
        Poles.SetValue(6,32,gp_Pnt(-71.0875,27.4934,-5.51298));
        Poles.SetValue(6,33,gp_Pnt(-71.0908,30.364,-5.39372));
        Poles.SetValue(6,34,gp_Pnt(-71.0932,32.2778,-5.31749));
        Poles.SetValue(6,35,gp_Pnt(-71.0997,34.6747,-5.24058));
        Poles.SetValue(6,36,gp_Pnt(-71.1085,37.552,-5.1317));
        Poles.SetValue(6,37,gp_Pnt(-71.1096,40.4266,-4.95936));
        Poles.SetValue(6,38,gp_Pnt(-71.1188,43.3052,-4.8684));
        Poles.SetValue(6,39,gp_Pnt(-71.1247,47.1418,-4.66742));
        Poles.SetValue(6,40,gp_Pnt(-71.1323,51.9393,-4.41857));
        Poles.SetValue(6,41,gp_Pnt(-71.1417,57.6996,-4.12519));
        Poles.SetValue(6,42,gp_Pnt(-71.1508,63.4613,-3.8404));
        Poles.SetValue(6,43,gp_Pnt(-71.162,71.1459,-3.46491));
        Poles.SetValue(6,44,gp_Pnt(-71.1789,84.5949,-2.84945));
        Poles.SetValue(6,45,gp_Pnt(-71.191,103.827,-1.9693));
        Poles.SetValue(6,46,gp_Pnt(-71.1797,126.907,-0.936212));
        Poles.SetValue(6,47,gp_Pnt(-71.1648,142.304,-0.336438));
        Poles.SetValue(6,48,gp_Pnt(-71.1568,150,-0.000475897));
        Poles.SetValue(7,1,gp_Pnt(-63.4635,-150,0.000362352));
        Poles.SetValue(7,2,gp_Pnt(-63.4667,-142.266,-0.270865));
        Poles.SetValue(7,3,gp_Pnt(-63.4754,-126.801,-0.886325));
        Poles.SetValue(7,4,gp_Pnt(-63.4707,-103.604,-1.70578));
        Poles.SetValue(7,5,gp_Pnt(-63.4505,-84.2857,-2.5768));
        Poles.SetValue(7,6,gp_Pnt(-63.4181,-70.7632,-3.20272));
        Poles.SetValue(7,7,gp_Pnt(-63.3948,-63.0341,-3.53045));
        Poles.SetValue(7,8,gp_Pnt(-63.3767,-57.2389,-3.80851));
        Poles.SetValue(7,9,gp_Pnt(-63.3583,-51.4454,-4.13513));
        Poles.SetValue(7,10,gp_Pnt(-63.338,-46.6134,-4.37957));
        Poles.SetValue(7,11,gp_Pnt(-63.319,-42.7455,-4.60555));
        Poles.SetValue(7,12,gp_Pnt(-63.3044,-39.8447,-4.78324));
        Poles.SetValue(7,13,gp_Pnt(-63.2827,-36.9396,-4.96843));
        Poles.SetValue(7,14,gp_Pnt(-63.2624,-34.0396,-5.18158));
        Poles.SetValue(7,15,gp_Pnt(-63.2386,-31.6189,-5.33987));
        Poles.SetValue(7,16,gp_Pnt(-63.221,-29.6882,-5.45269));
        Poles.SetValue(7,17,gp_Pnt(-63.1927,-26.7909,-5.60579));
        Poles.SetValue(7,18,gp_Pnt(-63.1645,-23.4252,-5.7359));
        Poles.SetValue(7,19,gp_Pnt(-63.1509,-19.5935,-5.838));
        Poles.SetValue(7,20,gp_Pnt(-63.1439,-15.77,-5.92716));
        Poles.SetValue(7,21,gp_Pnt(-63.1534,-11.9628,-6.05663));
        Poles.SetValue(7,22,gp_Pnt(-63.1624,-8.16097,-6.16538));
        Poles.SetValue(7,23,gp_Pnt(-63.1713,-5.31206,-6.20947));
        Poles.SetValue(7,24,gp_Pnt(-63.193,-1.51882,-6.33325));
        Poles.SetValue(7,25,gp_Pnt(-63.1982,2.26931,-6.42672));
        Poles.SetValue(7,26,gp_Pnt(-63.2064,6.05752,-6.45684));
        Poles.SetValue(7,27,gp_Pnt(-63.2158,8.89967,-6.49216));
        Poles.SetValue(7,28,gp_Pnt(-63.2266,12.6896,-6.5197));
        Poles.SetValue(7,29,gp_Pnt(-63.2272,16.4827,-6.46308));
        Poles.SetValue(7,30,gp_Pnt(-63.2514,20.2851,-6.51582));
        Poles.SetValue(7,31,gp_Pnt(-63.2665,24.0915,-6.41687));
        Poles.SetValue(7,32,gp_Pnt(-63.3025,27.4349,-6.36479));
        Poles.SetValue(7,33,gp_Pnt(-63.3303,30.3117,-6.22694));
        Poles.SetValue(7,34,gp_Pnt(-63.3488,32.2305,-6.13272));
        Poles.SetValue(7,35,gp_Pnt(-63.366,34.6316,-5.97585));
        Poles.SetValue(7,36,gp_Pnt(-63.3837,37.5115,-5.82491));
        Poles.SetValue(7,37,gp_Pnt(-63.4034,40.3963,-5.69285));
        Poles.SetValue(7,38,gp_Pnt(-63.4132,43.2758,-5.49842));
        Poles.SetValue(7,39,gp_Pnt(-63.4263,47.1169,-5.25128));
        Poles.SetValue(7,40,gp_Pnt(-63.4394,51.9183,-4.95511));
        Poles.SetValue(7,41,gp_Pnt(-63.4539,57.6817,-4.64148));
        Poles.SetValue(7,42,gp_Pnt(-63.4684,63.4475,-4.3285));
        Poles.SetValue(7,43,gp_Pnt(-63.4802,71.1311,-3.82751));
        Poles.SetValue(7,44,gp_Pnt(-63.499,84.589,-3.10785));
        Poles.SetValue(7,45,gp_Pnt(-63.5115,103.821,-2.17125));
        Poles.SetValue(7,46,gp_Pnt(-63.4944,126.908,-1.06963));
        Poles.SetValue(7,47,gp_Pnt(-63.4761,142.304,-0.392153));
        Poles.SetValue(7,48,gp_Pnt(-63.4659,150,0.000947448));
        Poles.SetValue(8,1,gp_Pnt(-57.6952,-150,-0.00314282));
        Poles.SetValue(8,2,gp_Pnt(-57.6995,-142.266,-0.311295));
        Poles.SetValue(8,3,gp_Pnt(-57.7097,-126.797,-0.921112));
        Poles.SetValue(8,4,gp_Pnt(-57.7066,-103.591,-1.73366));
        Poles.SetValue(8,5,gp_Pnt(-57.6899,-84.2722,-2.7131));
        Poles.SetValue(8,6,gp_Pnt(-57.6482,-70.7348,-3.36076));
        Poles.SetValue(8,7,gp_Pnt(-57.6215,-63.0007,-3.76313));
        Poles.SetValue(8,8,gp_Pnt(-57.5991,-57.2001,-4.08499));
        Poles.SetValue(8,9,gp_Pnt(-57.5756,-51.4007,-4.39273));
        Poles.SetValue(8,10,gp_Pnt(-57.554,-46.5664,-4.69516));
        Poles.SetValue(8,11,gp_Pnt(-57.5322,-42.6946,-4.96155));
        Poles.SetValue(8,12,gp_Pnt(-57.5132,-39.7891,-5.21994));
        Poles.SetValue(8,13,gp_Pnt(-57.4836,-36.8743,-5.38283));
        Poles.SetValue(8,14,gp_Pnt(-57.4423,-33.9556,-5.4932));
        Poles.SetValue(8,15,gp_Pnt(-57.402,-31.5272,-5.53467));
        Poles.SetValue(8,16,gp_Pnt(-57.3861,-29.5989,-5.60063));
        Poles.SetValue(8,17,gp_Pnt(-57.3688,-26.7132,-5.72085));
        Poles.SetValue(8,18,gp_Pnt(-57.398,-23.3843,-5.9876));
        Poles.SetValue(8,19,gp_Pnt(-57.4294,-19.5722,-6.26213));
        Poles.SetValue(8,20,gp_Pnt(-57.5035,-15.7794,-6.68012));
        Poles.SetValue(8,21,gp_Pnt(-57.5719,-11.9809,-7.06297));
        Poles.SetValue(8,22,gp_Pnt(-57.6221,-8.1767,-7.32819));
        Poles.SetValue(8,23,gp_Pnt(-57.6505,-5.32072,-7.50978));
        Poles.SetValue(8,24,gp_Pnt(-57.6777,-1.51151,-7.67969));
        Poles.SetValue(8,25,gp_Pnt(-57.7106,2.29043,-7.79385));
        Poles.SetValue(8,26,gp_Pnt(-57.6789,6.09212,-7.72725));
        Poles.SetValue(8,27,gp_Pnt(-57.6575,8.94134,-7.67509));
        Poles.SetValue(8,28,gp_Pnt(-57.6107,12.7342,-7.50788));
        Poles.SetValue(8,29,gp_Pnt(-57.5593,16.5215,-7.2957));
        Poles.SetValue(8,30,gp_Pnt(-57.5083,20.3002,-7.009));
        Poles.SetValue(8,31,gp_Pnt(-57.4814,24.0877,-6.79325));
        Poles.SetValue(8,32,gp_Pnt(-57.4846,27.4081,-6.67069));
        Poles.SetValue(8,33,gp_Pnt(-57.5135,30.2753,-6.61361));
        Poles.SetValue(8,34,gp_Pnt(-57.5328,32.1869,-6.57093));
        Poles.SetValue(8,35,gp_Pnt(-57.5743,34.5945,-6.55824));
        Poles.SetValue(8,36,gp_Pnt(-57.6121,37.4882,-6.44634));
        Poles.SetValue(8,37,gp_Pnt(-57.628,40.3737,-6.21298));
        Poles.SetValue(8,38,gp_Pnt(-57.6442,43.2575,-6.00935));
        Poles.SetValue(8,39,gp_Pnt(-57.6633,47.1029,-5.76432));
        Poles.SetValue(8,40,gp_Pnt(-57.6718,51.9,-5.37064));
        Poles.SetValue(8,41,gp_Pnt(-57.6883,57.6662,-5.02319));
        Poles.SetValue(8,42,gp_Pnt(-57.7048,63.4336,-4.65762));
        Poles.SetValue(8,43,gp_Pnt(-57.7185,71.1203,-4.08762));
        Poles.SetValue(8,44,gp_Pnt(-57.7448,84.5871,-3.3533));
        Poles.SetValue(8,45,gp_Pnt(-57.7511,103.816,-2.29786));
        Poles.SetValue(8,46,gp_Pnt(-57.7309,126.907,-1.1419));
        Poles.SetValue(8,47,gp_Pnt(-57.7091,142.303,-0.384611));
        Poles.SetValue(8,48,gp_Pnt(-57.6978,150,-0.00163315));
        Poles.SetValue(9,1,gp_Pnt(-51.9266,-150,-0.000170689));
        Poles.SetValue(9,2,gp_Pnt(-51.9325,-142.265,-0.308476));
        Poles.SetValue(9,3,gp_Pnt(-51.9437,-126.792,-0.954312));
        Poles.SetValue(9,4,gp_Pnt(-51.9434,-103.579,-1.79846));
        Poles.SetValue(9,5,gp_Pnt(-51.9339,-84.2654,-2.84979));
        Poles.SetValue(9,6,gp_Pnt(-51.8762,-70.7033,-3.47842));
        Poles.SetValue(9,7,gp_Pnt(-51.8492,-62.9676,-3.99915));
        Poles.SetValue(9,8,gp_Pnt(-51.8239,-57.1623,-4.34605));
        Poles.SetValue(9,9,gp_Pnt(-51.7903,-51.3504,-4.69053));
        Poles.SetValue(9,10,gp_Pnt(-51.7729,-46.5189,-5.09883));
        Poles.SetValue(9,11,gp_Pnt(-51.757,-42.6481,-5.27347));
        Poles.SetValue(9,12,gp_Pnt(-51.7513,-39.7469,-5.30536));
        Poles.SetValue(9,13,gp_Pnt(-51.7401,-36.8465,-5.38604));
        Poles.SetValue(9,14,gp_Pnt(-51.7694,-33.9846,-5.66624));
        Poles.SetValue(9,15,gp_Pnt(-51.7798,-31.5914,-5.96847));
        Poles.SetValue(9,16,gp_Pnt(-51.8152,-29.6949,-6.3184));
        Poles.SetValue(9,17,gp_Pnt(-51.8773,-26.8541,-6.90413));
        Poles.SetValue(9,18,gp_Pnt(-51.9335,-23.5242,-7.53269));
        Poles.SetValue(9,19,gp_Pnt(-52.0359,-19.7223,-8.2157));
        Poles.SetValue(9,20,gp_Pnt(-52.1308,-15.9092,-8.84603));
        Poles.SetValue(9,21,gp_Pnt(-52.2346,-12.094,-9.39748));
        Poles.SetValue(9,22,gp_Pnt(-52.3068,-8.26388,-9.81203));
        Poles.SetValue(9,23,gp_Pnt(-52.3488,-5.38726,-10.0427));
        Poles.SetValue(9,24,gp_Pnt(-52.391,-1.54826,-10.2726));
        Poles.SetValue(9,25,gp_Pnt(-52.4088,2.29184,-10.382));
        Poles.SetValue(9,26,gp_Pnt(-52.3622,6.12771,-10.2589));
        Poles.SetValue(9,27,gp_Pnt(-52.3201,8.99991,-10.0938));
        Poles.SetValue(9,28,gp_Pnt(-52.2783,12.8262,-9.86285));
        Poles.SetValue(9,29,gp_Pnt(-52.1925,16.6338,-9.442));
        Poles.SetValue(9,30,gp_Pnt(-52.1187,20.4371,-8.97308));
        Poles.SetValue(9,31,gp_Pnt(-52.0376,24.2289,-8.44813));
        Poles.SetValue(9,32,gp_Pnt(-51.9688,27.5366,-7.90192));
        Poles.SetValue(9,33,gp_Pnt(-51.9242,30.3704,-7.47434));
        Poles.SetValue(9,34,gp_Pnt(-51.9019,32.2644,-7.22041));
        Poles.SetValue(9,35,gp_Pnt(-51.8684,34.6208,-6.88143));
        Poles.SetValue(9,36,gp_Pnt(-51.8535,37.4738,-6.68843));
        Poles.SetValue(9,37,gp_Pnt(-51.8792,40.3596,-6.69292));
        Poles.SetValue(9,38,gp_Pnt(-51.8924,43.2452,-6.61057));
        Poles.SetValue(9,39,gp_Pnt(-51.8939,47.0764,-6.26774));
        Poles.SetValue(9,40,gp_Pnt(-51.9082,51.8812,-5.88782));
        Poles.SetValue(9,41,gp_Pnt(-51.9297,57.653,-5.46697));
        Poles.SetValue(9,42,gp_Pnt(-51.9428,63.4193,-5.00425));
        Poles.SetValue(9,43,gp_Pnt(-51.9613,71.113,-4.45556));
        Poles.SetValue(9,44,gp_Pnt(-51.9912,84.5843,-3.48722));
        Poles.SetValue(9,45,gp_Pnt(-51.9934,103.813,-2.50034));
        Poles.SetValue(9,46,gp_Pnt(-51.9654,126.903,-1.154));
        Poles.SetValue(9,47,gp_Pnt(-51.9437,142.306,-0.456106));
        Poles.SetValue(9,48,gp_Pnt(-51.9293,150,0.00113058));
        Poles.SetValue(10,1,gp_Pnt(-46.1581,-150.001,-0.000638257));
        Poles.SetValue(10,2,gp_Pnt(-46.1658,-142.266,-0.338908));
        Poles.SetValue(10,3,gp_Pnt(-46.1771,-126.787,-0.982666));
        Poles.SetValue(10,4,gp_Pnt(-46.1819,-103.569,-1.90958));
        Poles.SetValue(10,5,gp_Pnt(-46.1784,-84.261,-2.93134));
        Poles.SetValue(10,6,gp_Pnt(-46.1093,-70.6785,-3.69831));
        Poles.SetValue(10,7,gp_Pnt(-46.0739,-62.928,-4.17581));
        Poles.SetValue(10,8,gp_Pnt(-46.047,-57.1203,-4.57934));
        Poles.SetValue(10,9,gp_Pnt(-46.0154,-51.3116,-5.03379));
        Poles.SetValue(10,10,gp_Pnt(-45.9925,-46.4699,-5.18556));
        Poles.SetValue(10,11,gp_Pnt(-46.012,-42.6314,-5.32418));
        Poles.SetValue(10,12,gp_Pnt(-46.0662,-39.7925,-5.76494));
        Poles.SetValue(10,13,gp_Pnt(-46.1808,-37.0074,-6.4582));
        Poles.SetValue(10,14,gp_Pnt(-46.2627,-34.1775,-7.11737));
        Poles.SetValue(10,15,gp_Pnt(-46.3521,-31.8361,-7.6892));
        Poles.SetValue(10,16,gp_Pnt(-46.4119,-29.9436,-8.15917));
        Poles.SetValue(10,17,gp_Pnt(-46.4974,-27.1051,-8.83871));
        Poles.SetValue(10,18,gp_Pnt(-46.6385,-23.7928,-9.76221));
        Poles.SetValue(10,19,gp_Pnt(-46.7404,-19.9549,-10.6246));
        Poles.SetValue(10,20,gp_Pnt(-46.8688,-16.1204,-11.4653));
        Poles.SetValue(10,21,gp_Pnt(-46.9478,-12.2561,-12.113));
        Poles.SetValue(10,22,gp_Pnt(-47.0023,-8.38515,-12.5828));
        Poles.SetValue(10,23,gp_Pnt(-47.0269,-5.4785,-12.8456));
        Poles.SetValue(10,24,gp_Pnt(-47.0386,-1.60018,-13.0705));
        Poles.SetValue(10,25,gp_Pnt(-47.0287,2.2868,-13.1044));
        Poles.SetValue(10,26,gp_Pnt(-47.034,6.16793,-13.0276));
        Poles.SetValue(10,27,gp_Pnt(-47.0114,9.0735,-12.8621));
        Poles.SetValue(10,28,gp_Pnt(-46.9836,12.9397,-12.5471));
        Poles.SetValue(10,29,gp_Pnt(-46.9389,16.7984,-12.1024));
        Poles.SetValue(10,30,gp_Pnt(-46.8056,20.6199,-11.386));
        Poles.SetValue(10,31,gp_Pnt(-46.729,24.4527,-10.7291));
        Poles.SetValue(10,32,gp_Pnt(-46.5805,27.7429,-9.92361));
        Poles.SetValue(10,33,gp_Pnt(-46.5017,30.5752,-9.34857));
        Poles.SetValue(10,34,gp_Pnt(-46.4461,32.4597,-8.95609));
        Poles.SetValue(10,35,gp_Pnt(-46.3462,34.7849,-8.34287));
        Poles.SetValue(10,36,gp_Pnt(-46.2837,37.6128,-7.82045));
        Poles.SetValue(10,37,gp_Pnt(-46.162,40.3806,-7.10571));
        Poles.SetValue(10,38,gp_Pnt(-46.1171,43.2098,-6.73439));
        Poles.SetValue(10,39,gp_Pnt(-46.1366,47.0603,-6.75792));
        Poles.SetValue(10,40,gp_Pnt(-46.1512,51.8648,-6.41774));
        Poles.SetValue(10,41,gp_Pnt(-46.1664,57.632,-5.86914));
        Poles.SetValue(10,42,gp_Pnt(-46.1871,63.4077,-5.41307));
        Poles.SetValue(10,43,gp_Pnt(-46.2021,71.1,-4.72497));
        Poles.SetValue(10,44,gp_Pnt(-46.2443,84.5899,-3.70014));
        Poles.SetValue(10,45,gp_Pnt(-46.2336,103.804,-2.61154));
        Poles.SetValue(10,46,gp_Pnt(-46.2013,126.902,-1.24637));
        Poles.SetValue(10,47,gp_Pnt(-46.1774,142.306,-0.449946));
        Poles.SetValue(10,48,gp_Pnt(-46.1611,150.001,-0.00144769));
        Poles.SetValue(11,1,gp_Pnt(-41.3506,-150.001,0.000129401));
        Poles.SetValue(11,2,gp_Pnt(-41.3604,-142.268,-0.369782));
        Poles.SetValue(11,3,gp_Pnt(-41.3715,-126.784,-1.01965));
        Poles.SetValue(11,4,gp_Pnt(-41.381,-103.559,-1.98585));
        Poles.SetValue(11,5,gp_Pnt(-41.3846,-84.2649,-3.02478));
        Poles.SetValue(11,6,gp_Pnt(-41.3076,-70.6584,-3.82604));
        Poles.SetValue(11,7,gp_Pnt(-41.2583,-62.8898,-4.34596));
        Poles.SetValue(11,8,gp_Pnt(-41.2301,-57.0807,-4.77554));
        Poles.SetValue(11,9,gp_Pnt(-41.208,-51.2838,-5.08544));
        Poles.SetValue(11,10,gp_Pnt(-41.2328,-46.4935,-5.34778));
        Poles.SetValue(11,11,gp_Pnt(-41.3268,-42.7488,-6.24075));
        Poles.SetValue(11,12,gp_Pnt(-41.4109,-39.9421,-6.96341));
        Poles.SetValue(11,13,gp_Pnt(-41.4934,-37.1246,-7.70505));
        Poles.SetValue(11,14,gp_Pnt(-41.6366,-34.3514,-8.60427));
        Poles.SetValue(11,15,gp_Pnt(-41.7592,-32.0314,-9.39254));
        Poles.SetValue(11,16,gp_Pnt(-41.866,-30.171,-10.027));
        Poles.SetValue(11,17,gp_Pnt(-42.007,-27.3579,-10.9448));
        Poles.SetValue(11,18,gp_Pnt(-42.1295,-24.0111,-11.8554));
        Poles.SetValue(11,19,gp_Pnt(-42.2841,-20.1843,-12.84));
        Poles.SetValue(11,20,gp_Pnt(-42.4397,-16.3348,-13.7597));
        Poles.SetValue(11,21,gp_Pnt(-42.6184,-12.4706,-14.5978));
        Poles.SetValue(11,22,gp_Pnt(-42.7575,-8.5647,-15.2417));
        Poles.SetValue(11,23,gp_Pnt(-42.8491,-5.62091,-15.6293));
        Poles.SetValue(11,24,gp_Pnt(-42.8914,-1.67076,-15.914));
        Poles.SetValue(11,25,gp_Pnt(-42.8577,2.29453,-15.9201));
        Poles.SetValue(11,26,gp_Pnt(-42.8099,6.24375,-15.7308));
        Poles.SetValue(11,27,gp_Pnt(-42.7438,9.19033,-15.4664));
        Poles.SetValue(11,28,gp_Pnt(-42.6546,13.1006,-15.0329));
        Poles.SetValue(11,29,gp_Pnt(-42.4397,16.9381,-14.2592));
        Poles.SetValue(11,30,gp_Pnt(-42.3321,20.793,-13.586));
        Poles.SetValue(11,31,gp_Pnt(-42.202,24.6193,-12.7928));
        Poles.SetValue(11,32,gp_Pnt(-42.0702,27.9455,-11.976));
        Poles.SetValue(11,33,gp_Pnt(-41.926,30.743,-11.1473));
        Poles.SetValue(11,34,gp_Pnt(-41.8424,32.617,-10.6229));
        Poles.SetValue(11,35,gp_Pnt(-41.7499,34.9475,-9.97304));
        Poles.SetValue(11,36,gp_Pnt(-41.6445,37.7479,-9.23094));
        Poles.SetValue(11,37,gp_Pnt(-41.557,40.5517,-8.47969));
        Poles.SetValue(11,38,gp_Pnt(-41.4664,43.3478,-7.79497));
        Poles.SetValue(11,39,gp_Pnt(-41.3421,47.0459,-6.80636));
        Poles.SetValue(11,40,gp_Pnt(-41.3394,51.8371,-6.68123));
        Poles.SetValue(11,41,gp_Pnt(-41.3706,57.6167,-6.22193));
        Poles.SetValue(11,42,gp_Pnt(-41.3831,63.3849,-5.68374));
        Poles.SetValue(11,43,gp_Pnt(-41.4131,71.0964,-4.98988));
        Poles.SetValue(11,44,gp_Pnt(-41.4553,84.5966,-3.8597));
        Poles.SetValue(11,45,gp_Pnt(-41.4358,103.797,-2.69226));
        Poles.SetValue(11,46,gp_Pnt(-41.396,126.901,-1.33321));
        Poles.SetValue(11,47,gp_Pnt(-41.3733,142.306,-0.423026));
        Poles.SetValue(11,48,gp_Pnt(-41.3535,150.001,0.000856916));
        Poles.SetValue(12,1,gp_Pnt(-37.5046,-150.002,-0.000990159));
        Poles.SetValue(12,2,gp_Pnt(-37.5158,-142.268,-0.362104));
        Poles.SetValue(12,3,gp_Pnt(-37.5275,-126.781,-1.04216));
        Poles.SetValue(12,4,gp_Pnt(-37.54,-103.548,-2.02727));
        Poles.SetValue(12,5,gp_Pnt(-37.5516,-84.2718,-3.08919));
        Poles.SetValue(12,6,gp_Pnt(-37.4692,-70.645,-3.92736));
        Poles.SetValue(12,7,gp_Pnt(-37.4034,-62.8516,-4.46926));
        Poles.SetValue(12,8,gp_Pnt(-37.3656,-57.0305,-4.90814));
        Poles.SetValue(12,9,gp_Pnt(-37.38,-51.2739,-5.03776));
        Poles.SetValue(12,10,gp_Pnt(-37.4847,-46.6009,-5.98182));
        Poles.SetValue(12,11,gp_Pnt(-37.5865,-42.8757,-7.03982));
        Poles.SetValue(12,12,gp_Pnt(-37.6857,-40.0925,-7.9519));
        Poles.SetValue(12,13,gp_Pnt(-37.8317,-37.3448,-9.00586));
        Poles.SetValue(12,14,gp_Pnt(-38.0147,-34.6055,-10.1344));
        Poles.SetValue(12,15,gp_Pnt(-38.1289,-32.2759,-10.9655));
        Poles.SetValue(12,16,gp_Pnt(-38.2014,-30.3789,-11.5644));
        Poles.SetValue(12,17,gp_Pnt(-38.3098,-27.5372,-12.4577));
        Poles.SetValue(12,18,gp_Pnt(-38.5591,-24.2661,-13.6428));
        Poles.SetValue(12,19,gp_Pnt(-38.8481,-20.4819,-14.8886));
        Poles.SetValue(12,20,gp_Pnt(-39.176,-16.67,-16.0722));
        Poles.SetValue(12,21,gp_Pnt(-39.431,-12.7582,-16.9903));
        Poles.SetValue(12,22,gp_Pnt(-39.6338,-8.78775,-17.6581));
        Poles.SetValue(12,23,gp_Pnt(-39.7083,-5.77365,-17.9709));
        Poles.SetValue(12,24,gp_Pnt(-39.7945,-1.73776,-18.2789));
        Poles.SetValue(12,25,gp_Pnt(-39.7792,2.31582,-18.3108));
        Poles.SetValue(12,26,gp_Pnt(-39.7111,6.3468,-18.1589));
        Poles.SetValue(12,27,gp_Pnt(-39.6071,9.34647,-17.8966));
        Poles.SetValue(12,28,gp_Pnt(-39.4225,13.3136,-17.3697));
        Poles.SetValue(12,29,gp_Pnt(-39.1565,17.2006,-16.5688));
        Poles.SetValue(12,30,gp_Pnt(-38.8412,21.0139,-15.5401));
        Poles.SetValue(12,31,gp_Pnt(-38.5614,24.7878,-14.4514));
        Poles.SetValue(12,32,gp_Pnt(-38.3865,28.1004,-13.5309));
        Poles.SetValue(12,33,gp_Pnt(-38.2837,30.935,-12.7238));
        Poles.SetValue(12,34,gp_Pnt(-38.1998,32.81,-12.1537));
        Poles.SetValue(12,35,gp_Pnt(-38.0919,35.1348,-11.3864));
        Poles.SetValue(12,36,gp_Pnt(-37.9482,37.9015,-10.4242));
        Poles.SetValue(12,37,gp_Pnt(-37.8069,40.6462,-9.43281));
        Poles.SetValue(12,38,gp_Pnt(-37.7142,43.4352,-8.61957));
        Poles.SetValue(12,39,gp_Pnt(-37.5907,47.134,-7.52689));
        Poles.SetValue(12,40,gp_Pnt(-37.5157,51.8377,-6.80127));
        Poles.SetValue(12,41,gp_Pnt(-37.5183,57.5848,-6.49516));
        Poles.SetValue(12,42,gp_Pnt(-37.5412,63.3635,-5.90827));
        Poles.SetValue(12,43,gp_Pnt(-37.5834,71.0941,-5.20371));
        Poles.SetValue(12,44,gp_Pnt(-37.6269,84.6055,-3.9813));
        Poles.SetValue(12,45,gp_Pnt(-37.5971,103.791,-2.77693));
        Poles.SetValue(12,46,gp_Pnt(-37.5523,126.898,-1.36182));
        Poles.SetValue(12,47,gp_Pnt(-37.5291,142.307,-0.42845));
        Poles.SetValue(12,48,gp_Pnt(-37.5076,150.002,0.00471985));
        Poles.SetValue(13,1,gp_Pnt(-34.62,-150.003,-0.0023316));
        Poles.SetValue(13,2,gp_Pnt(-34.632,-142.268,-0.349958));
        Poles.SetValue(13,3,gp_Pnt(-34.645,-126.78,-1.05185));
        Poles.SetValue(13,4,gp_Pnt(-34.6586,-103.539,-2.05983));
        Poles.SetValue(13,5,gp_Pnt(-34.6791,-84.2792,-3.11805));
        Poles.SetValue(13,6,gp_Pnt(-34.591,-70.6351,-4.00171));
        Poles.SetValue(13,7,gp_Pnt(-34.5131,-62.8198,-4.56575));
        Poles.SetValue(13,8,gp_Pnt(-34.4669,-56.9824,-4.92811));
        Poles.SetValue(13,9,gp_Pnt(-34.5266,-51.29,-5.24163));
        Poles.SetValue(13,10,gp_Pnt(-34.6876,-46.7104,-6.52793));
        Poles.SetValue(13,11,gp_Pnt(-34.8026,-43.0114,-7.78986));
        Poles.SetValue(13,12,gp_Pnt(-34.9344,-40.2726,-8.89694));
        Poles.SetValue(13,13,gp_Pnt(-35.0891,-37.5373,-10.0435));
        Poles.SetValue(13,14,gp_Pnt(-35.2348,-34.7597,-11.1199));
        Poles.SetValue(13,15,gp_Pnt(-35.3211,-32.4067,-11.9429));
        Poles.SetValue(13,16,gp_Pnt(-35.4497,-30.5606,-12.6961));
        Poles.SetValue(13,17,gp_Pnt(-35.614,-27.7584,-13.7623));
        Poles.SetValue(13,18,gp_Pnt(-35.9962,-24.5678,-15.2273));
        Poles.SetValue(13,19,gp_Pnt(-36.363,-20.7882,-16.5677));
        Poles.SetValue(13,20,gp_Pnt(-36.6904,-16.9282,-17.7055));
        Poles.SetValue(13,21,gp_Pnt(-36.8578,-12.9306,-18.3233));
        Poles.SetValue(13,22,gp_Pnt(-36.9527,-8.88561,-18.7197));
        Poles.SetValue(13,23,gp_Pnt(-36.9967,-5.83477,-18.8948));
        Poles.SetValue(13,24,gp_Pnt(-37.0318,-1.75551,-19.0735));
        Poles.SetValue(13,25,gp_Pnt(-37.0313,2.32873,-19.1232));
        Poles.SetValue(13,26,gp_Pnt(-37.0175,6.39695,-19.1615));
        Poles.SetValue(13,27,gp_Pnt(-36.9594,9.43212,-19.0345));
        Poles.SetValue(13,28,gp_Pnt(-36.8675,13.4561,-18.7867));
        Poles.SetValue(13,29,gp_Pnt(-36.6616,17.4137,-18.1833));
        Poles.SetValue(13,30,gp_Pnt(-36.3331,21.2595,-17.2008));
        Poles.SetValue(13,31,gp_Pnt(-35.995,25.041,-16.0204));
        Poles.SetValue(13,32,gp_Pnt(-35.6555,28.2725,-14.7687));
        Poles.SetValue(13,33,gp_Pnt(-35.5108,31.0696,-13.8117));
        Poles.SetValue(13,34,gp_Pnt(-35.4456,32.9642,-13.2402));
        Poles.SetValue(13,35,gp_Pnt(-35.3492,35.2949,-12.4616));
        Poles.SetValue(13,36,gp_Pnt(-35.189,38.0465,-11.4098));
        Poles.SetValue(13,37,gp_Pnt(-35.0275,40.7699,-10.3096));
        Poles.SetValue(13,38,gp_Pnt(-34.9073,43.5243,-9.30737));
        Poles.SetValue(13,39,gp_Pnt(-34.7987,47.2339,-8.14392));
        Poles.SetValue(13,40,gp_Pnt(-34.651,51.8373,-6.91992));
        Poles.SetValue(13,41,gp_Pnt(-34.6284,57.5527,-6.63443));
        Poles.SetValue(13,42,gp_Pnt(-34.659,63.3436,-6.09258));
        Poles.SetValue(13,43,gp_Pnt(-34.7102,71.0895,-5.31871));
        Poles.SetValue(13,44,gp_Pnt(-34.7584,84.6162,-4.09984));
        Poles.SetValue(13,45,gp_Pnt(-34.7172,103.784,-2.82096));
        Poles.SetValue(13,46,gp_Pnt(-34.6703,126.897,-1.3891));
        Poles.SetValue(13,47,gp_Pnt(-34.6455,142.308,-0.431219));
        Poles.SetValue(13,48,gp_Pnt(-34.6232,150.002,0.00496173));
        Poles.SetValue(14,1,gp_Pnt(-31.7352,-150.003,-0.00154848));
        Poles.SetValue(14,2,gp_Pnt(-31.7482,-142.267,-0.331617));
        Poles.SetValue(14,3,gp_Pnt(-31.763,-126.779,-1.063));
        Poles.SetValue(14,4,gp_Pnt(-31.7766,-103.527,-2.07596));
        Poles.SetValue(14,5,gp_Pnt(-31.8089,-84.2907,-3.17444));
        Poles.SetValue(14,6,gp_Pnt(-31.7122,-70.6219,-4.03525));
        Poles.SetValue(14,7,gp_Pnt(-31.6249,-62.7848,-4.66387));
        Poles.SetValue(14,8,gp_Pnt(-31.5769,-56.9282,-4.89998));
        Poles.SetValue(14,9,gp_Pnt(-31.6802,-51.3146,-5.53692));
        Poles.SetValue(14,10,gp_Pnt(-31.8745,-46.8081,-7.10223));
        Poles.SetValue(14,11,gp_Pnt(-32.018,-43.1597,-8.56607));
        Poles.SetValue(14,12,gp_Pnt(-32.1638,-40.4524,-9.81769));
        Poles.SetValue(14,13,gp_Pnt(-32.3073,-37.7124,-11.0081));
        Poles.SetValue(14,14,gp_Pnt(-32.4358,-34.9155,-12.0817));
        Poles.SetValue(14,15,gp_Pnt(-32.5926,-32.6407,-13.1205));
        Poles.SetValue(14,16,gp_Pnt(-32.7344,-30.7964,-13.9191));
        Poles.SetValue(14,17,gp_Pnt(-33.0143,-28.0979,-15.252));
        Poles.SetValue(14,18,gp_Pnt(-33.4113,-24.8923,-16.747));
        Poles.SetValue(14,19,gp_Pnt(-33.7395,-21.0593,-17.9381));
        Poles.SetValue(14,20,gp_Pnt(-33.9529,-17.0987,-18.7264));
        Poles.SetValue(14,21,gp_Pnt(-34.0517,-13.0389,-19.0923));
        Poles.SetValue(14,22,gp_Pnt(-34.0927,-8.94344,-19.1565));
        Poles.SetValue(14,23,gp_Pnt(-34.1148,-5.86615,-19.2346));
        Poles.SetValue(14,24,gp_Pnt(-34.1342,-1.75859,-19.3087));
        Poles.SetValue(14,25,gp_Pnt(-34.1496,2.34806,-19.3815));
        Poles.SetValue(14,26,gp_Pnt(-34.1479,6.43942,-19.5269));
        Poles.SetValue(14,27,gp_Pnt(-34.1348,9.49773,-19.5973));
        Poles.SetValue(14,28,gp_Pnt(-34.086,13.5579,-19.6271));
        Poles.SetValue(14,29,gp_Pnt(-33.9618,17.5747,-19.3248));
        Poles.SetValue(14,30,gp_Pnt(-33.7384,21.5045,-18.634));
        Poles.SetValue(14,31,gp_Pnt(-33.4066,25.3178,-17.5646));
        Poles.SetValue(14,32,gp_Pnt(-33.0544,28.5653,-16.2703));
        Poles.SetValue(14,33,gp_Pnt(-32.8588,31.3256,-15.1565));
        Poles.SetValue(14,34,gp_Pnt(-32.6532,33.099,-14.2631));
        Poles.SetValue(14,35,gp_Pnt(-32.5381,35.4039,-13.3772));
        Poles.SetValue(14,36,gp_Pnt(-32.393,38.1751,-12.3245));
        Poles.SetValue(14,37,gp_Pnt(-32.231,40.8861,-11.1505));
        Poles.SetValue(14,38,gp_Pnt(-32.1223,43.6538,-10.1046));
        Poles.SetValue(14,39,gp_Pnt(-31.9975,47.3317,-8.76792));
        Poles.SetValue(14,40,gp_Pnt(-31.8214,51.8667,-7.32186));
        Poles.SetValue(14,41,gp_Pnt(-31.7377,57.5022,-6.67478));
        Poles.SetValue(14,42,gp_Pnt(-31.7779,63.3209,-6.2713));
        Poles.SetValue(14,43,gp_Pnt(-31.838,71.0853,-5.43635));
        Poles.SetValue(14,44,gp_Pnt(-31.8903,84.6268,-4.19797));
        Poles.SetValue(14,45,gp_Pnt(-31.8372,103.777,-2.86658));
        Poles.SetValue(14,46,gp_Pnt(-31.7884,126.895,-1.40853));
        Poles.SetValue(14,47,gp_Pnt(-31.7622,142.309,-0.438532));
        Poles.SetValue(14,48,gp_Pnt(-31.7384,150.003,0.00199423));
        Poles.SetValue(15,1,gp_Pnt(-28.8504,-150.003,-0.000296043));
        Poles.SetValue(15,2,gp_Pnt(-28.8644,-142.267,-0.326339));
        Poles.SetValue(15,3,gp_Pnt(-28.8809,-126.778,-1.04463));
        Poles.SetValue(15,4,gp_Pnt(-28.8954,-103.518,-2.13061));
        Poles.SetValue(15,5,gp_Pnt(-28.9393,-84.3,-3.15907));
        Poles.SetValue(15,6,gp_Pnt(-28.837,-70.6117,-4.10819));
        Poles.SetValue(15,7,gp_Pnt(-28.7434,-62.7489,-4.72495));
        Poles.SetValue(15,8,gp_Pnt(-28.708,-56.8994,-4.89063));
        Poles.SetValue(15,9,gp_Pnt(-28.8485,-51.3762,-6.00024));
        Poles.SetValue(15,10,gp_Pnt(-29.054,-46.9121,-7.70424));
        Poles.SetValue(15,11,gp_Pnt(-29.2298,-43.3343,-9.40211));
        Poles.SetValue(15,12,gp_Pnt(-29.3671,-40.6219,-10.6912));
        Poles.SetValue(15,13,gp_Pnt(-29.4868,-37.8621,-11.8917));
        Poles.SetValue(15,14,gp_Pnt(-29.653,-35.1193,-13.1207));
        Poles.SetValue(15,15,gp_Pnt(-29.8374,-32.8838,-14.2764));
        Poles.SetValue(15,16,gp_Pnt(-30.0554,-31.1218,-15.2551));
        Poles.SetValue(15,17,gp_Pnt(-30.3616,-28.4511,-16.6604));
        Poles.SetValue(15,18,gp_Pnt(-30.7128,-25.1891,-18.0319));
        Poles.SetValue(15,19,gp_Pnt(-30.8848,-21.2184,-18.7039));
        Poles.SetValue(15,20,gp_Pnt(-31.0171,-17.1843,-19.1198));
        Poles.SetValue(15,21,gp_Pnt(-31.0697,-13.0808,-19.0549));
        Poles.SetValue(15,22,gp_Pnt(-31.0917,-8.96259,-19.0617));
        Poles.SetValue(15,23,gp_Pnt(-31.1082,-5.8717,-19.0769));
        Poles.SetValue(15,24,gp_Pnt(-31.1282,-1.74971,-19.1218));
        Poles.SetValue(15,25,gp_Pnt(-31.1451,2.36805,-19.2344));
        Poles.SetValue(15,26,gp_Pnt(-31.1463,6.47208,-19.4017));
        Poles.SetValue(15,27,gp_Pnt(-31.1369,9.5398,-19.563));
        Poles.SetValue(15,28,gp_Pnt(-31.125,13.624,-19.676));
        Poles.SetValue(15,29,gp_Pnt(-31.0629,17.6731,-19.8221));
        Poles.SetValue(15,30,gp_Pnt(-30.9394,21.6753,-19.5784));
        Poles.SetValue(15,31,gp_Pnt(-30.7208,25.5796,-18.8941));
        Poles.SetValue(15,32,gp_Pnt(-30.429,28.8939,-17.7437));
        Poles.SetValue(15,33,gp_Pnt(-30.1355,31.5716,-16.4029));
        Poles.SetValue(15,34,gp_Pnt(-30.004,33.4123,-15.6337));
        Poles.SetValue(15,35,gp_Pnt(-29.7454,35.5561,-14.3595));
        Poles.SetValue(15,36,gp_Pnt(-29.5827,38.3004,-13.2055));
        Poles.SetValue(15,37,gp_Pnt(-29.4071,40.9896,-11.9148));
        Poles.SetValue(15,38,gp_Pnt(-29.3087,43.7647,-10.8315));
        Poles.SetValue(15,39,gp_Pnt(-29.1671,47.4036,-9.29662));
        Poles.SetValue(15,40,gp_Pnt(-28.9805,51.9045,-7.68659));
        Poles.SetValue(15,41,gp_Pnt(-28.8656,57.4635,-6.70615));
        Poles.SetValue(15,42,gp_Pnt(-28.899,63.2886,-6.3862));
        Poles.SetValue(15,43,gp_Pnt(-28.9679,71.0817,-5.5474));
        Poles.SetValue(15,44,gp_Pnt(-29.0236,84.6395,-4.29974));
        Poles.SetValue(15,45,gp_Pnt(-28.9571,103.767,-2.89536));
        Poles.SetValue(15,46,gp_Pnt(-28.9071,126.895,-1.43175));
        Poles.SetValue(15,47,gp_Pnt(-28.8786,142.31,-0.451004));
        Poles.SetValue(15,48,gp_Pnt(-28.8536,150.003,-0.000328798));
        Poles.SetValue(16,1,gp_Pnt(-25.9656,-150.004,0.00137713));
        Poles.SetValue(16,2,gp_Pnt(-25.9803,-142.267,-0.325925));
        Poles.SetValue(16,3,gp_Pnt(-25.9991,-126.777,-1.02412));
        Poles.SetValue(16,4,gp_Pnt(-26.0145,-103.507,-2.17811));
        Poles.SetValue(16,5,gp_Pnt(-26.071,-84.312,-3.15724));
        Poles.SetValue(16,6,gp_Pnt(-25.9635,-70.5999,-4.17961));
        Poles.SetValue(16,7,gp_Pnt(-25.8703,-62.7171,-4.72997));
        Poles.SetValue(16,8,gp_Pnt(-25.8574,-56.9015,-4.98549));
        Poles.SetValue(16,9,gp_Pnt(-26.0153,-51.4433,-6.4689));
        Poles.SetValue(16,10,gp_Pnt(-26.2167,-47.0041,-8.31462));
        Poles.SetValue(16,11,gp_Pnt(-26.414,-43.4866,-10.1773));
        Poles.SetValue(16,12,gp_Pnt(-26.5319,-40.7572,-11.4557));
        Poles.SetValue(16,13,gp_Pnt(-26.6786,-38.0475,-12.8017));
        Poles.SetValue(16,14,gp_Pnt(-26.9109,-35.41,-14.2563));
        Poles.SetValue(16,15,gp_Pnt(-27.1481,-33.2466,-15.577));
        Poles.SetValue(16,16,gp_Pnt(-27.3362,-31.4586,-16.4821));
        Poles.SetValue(16,17,gp_Pnt(-27.6323,-28.7816,-17.8543));
        Poles.SetValue(16,18,gp_Pnt(-27.8191,-25.3474,-18.6862));
        Poles.SetValue(16,19,gp_Pnt(-27.9266,-21.3202,-19.0118));
        Poles.SetValue(16,20,gp_Pnt(-27.9944,-17.2306,-19.0069));
        Poles.SetValue(16,21,gp_Pnt(-28.0229,-13.1072,-18.9149));
        Poles.SetValue(16,22,gp_Pnt(-28.0429,-8.9775,-18.9805));
        Poles.SetValue(16,23,gp_Pnt(-28.0561,-5.87853,-19.0437));
        Poles.SetValue(16,24,gp_Pnt(-28.0792,-1.74655,-19.1248));
        Poles.SetValue(16,25,gp_Pnt(-28.0969,2.38173,-19.2133));
        Poles.SetValue(16,26,gp_Pnt(-28.121,6.50036,-19.3792));
        Poles.SetValue(16,27,gp_Pnt(-28.1339,9.58285,-19.5254));
        Poles.SetValue(16,28,gp_Pnt(-28.1071,13.6792,-19.5675));
        Poles.SetValue(16,29,gp_Pnt(-28.0705,17.7504,-19.8531));
        Poles.SetValue(16,30,gp_Pnt(-28.0003,21.791,-19.912));
        Poles.SetValue(16,31,gp_Pnt(-27.911,25.7842,-19.8198));
        Poles.SetValue(16,32,gp_Pnt(-27.6949,29.1933,-18.9987));
        Poles.SetValue(16,33,gp_Pnt(-27.4332,31.8849,-17.6989));
        Poles.SetValue(16,34,gp_Pnt(-27.2196,33.6394,-16.7355));
        Poles.SetValue(16,35,gp_Pnt(-26.9796,35.7881,-15.4254));
        Poles.SetValue(16,36,gp_Pnt(-26.7348,38.4053,-13.9907));
        Poles.SetValue(16,37,gp_Pnt(-26.6103,41.1531,-12.7843));
        Poles.SetValue(16,38,gp_Pnt(-26.4815,43.872,-11.5302));
        Poles.SetValue(16,39,gp_Pnt(-26.3471,47.5059,-9.95418));
        Poles.SetValue(16,40,gp_Pnt(-26.1542,51.9566,-8.16439));
        Poles.SetValue(16,41,gp_Pnt(-26.0013,57.4301,-6.7748));
        Poles.SetValue(16,42,gp_Pnt(-26.0309,63.2656,-6.51026));
        Poles.SetValue(16,43,gp_Pnt(-26.0987,71.0753,-5.64577));
        Poles.SetValue(16,44,gp_Pnt(-26.1566,84.652,-4.37635));
        Poles.SetValue(16,45,gp_Pnt(-26.0777,103.757,-2.92071));
        Poles.SetValue(16,46,gp_Pnt(-26.0262,126.895,-1.44846));
        Poles.SetValue(16,47,gp_Pnt(-25.9946,142.312,-0.475798));
        Poles.SetValue(16,48,gp_Pnt(-25.9691,150.004,0.000603631));
        Poles.SetValue(17,1,gp_Pnt(-23.0806,-150.004,0.0019641));
        Poles.SetValue(17,2,gp_Pnt(-23.0964,-142.267,-0.330405));
        Poles.SetValue(17,3,gp_Pnt(-23.1173,-126.777,-0.994297));
        Poles.SetValue(17,4,gp_Pnt(-23.1339,-103.497,-2.23784));
        Poles.SetValue(17,5,gp_Pnt(-23.203,-84.3232,-3.12651));
        Poles.SetValue(17,6,gp_Pnt(-23.0914,-70.5857,-4.25051));
        Poles.SetValue(17,7,gp_Pnt(-23.0079,-62.6967,-4.71921));
        Poles.SetValue(17,8,gp_Pnt(-23.0207,-56.9337,-5.11304));
        Poles.SetValue(17,9,gp_Pnt(-23.1812,-51.5274,-6.94389));
        Poles.SetValue(17,10,gp_Pnt(-23.3792,-47.1212,-8.95957));
        Poles.SetValue(17,11,gp_Pnt(-23.5592,-43.5991,-10.8437));
        Poles.SetValue(17,12,gp_Pnt(-23.6823,-40.8919,-12.1796));
        Poles.SetValue(17,13,gp_Pnt(-23.8673,-38.2715,-13.717));
        Poles.SetValue(17,14,gp_Pnt(-24.1096,-35.6834,-15.2627));
        Poles.SetValue(17,15,gp_Pnt(-24.3579,-33.557,-16.646));
        Poles.SetValue(17,16,gp_Pnt(-24.5176,-31.7432,-17.4483));
        Poles.SetValue(17,17,gp_Pnt(-24.7308,-28.9759,-18.5377));
        Poles.SetValue(17,18,gp_Pnt(-24.8179,-25.4266,-18.8272));
        Poles.SetValue(17,19,gp_Pnt(-24.8974,-21.3738,-18.951));
        Poles.SetValue(17,20,gp_Pnt(-24.9391,-17.2569,-18.7823));
        Poles.SetValue(17,21,gp_Pnt(-24.9579,-13.1274,-18.8788));
        Poles.SetValue(17,22,gp_Pnt(-24.9753,-8.98927,-18.9409));
        Poles.SetValue(17,23,gp_Pnt(-24.9913,-5.88506,-19.0143));
        Poles.SetValue(17,24,gp_Pnt(-25.0155,-1.74473,-19.0767));
        Poles.SetValue(17,25,gp_Pnt(-25.0387,2.39217,-19.1733));
        Poles.SetValue(17,26,gp_Pnt(-25.0554,6.52043,-19.304));
        Poles.SetValue(17,27,gp_Pnt(-25.062,9.61116,-19.3704));
        Poles.SetValue(17,28,gp_Pnt(-25.0579,13.7179,-19.5885));
        Poles.SetValue(17,29,gp_Pnt(-25.0328,17.8051,-19.6948));
        Poles.SetValue(17,30,gp_Pnt(-24.9947,21.8711,-19.9465));
        Poles.SetValue(17,31,gp_Pnt(-24.9153,25.8755,-20.1336));
        Poles.SetValue(17,32,gp_Pnt(-24.8254,29.3978,-19.7884));
        Poles.SetValue(17,33,gp_Pnt(-24.5662,32.0898,-18.5604));
        Poles.SetValue(17,34,gp_Pnt(-24.4055,33.8853,-17.7436));
        Poles.SetValue(17,35,gp_Pnt(-24.178,36.0383,-16.4151));
        Poles.SetValue(17,36,gp_Pnt(-23.9261,38.6092,-14.8899));
        Poles.SetValue(17,37,gp_Pnt(-23.7426,41.2345,-13.4103));
        Poles.SetValue(17,38,gp_Pnt(-23.6547,44.0121,-12.2559));
        Poles.SetValue(17,39,gp_Pnt(-23.4902,47.5631,-10.4569));
        Poles.SetValue(17,40,gp_Pnt(-23.3099,52.0088,-8.56085));
        Poles.SetValue(17,41,gp_Pnt(-23.1561,57.4374,-6.92122));
        Poles.SetValue(17,42,gp_Pnt(-23.1683,63.2457,-6.58119));
        Poles.SetValue(17,43,gp_Pnt(-23.2319,71.0684,-5.75082));
        Poles.SetValue(17,44,gp_Pnt(-23.2881,84.6629,-4.4223));
        Poles.SetValue(17,45,gp_Pnt(-23.199,103.747,-2.94621));
        Poles.SetValue(17,46,gp_Pnt(-23.1451,126.896,-1.46385));
        Poles.SetValue(17,47,gp_Pnt(-23.1109,142.313,-0.498328));
        Poles.SetValue(17,48,gp_Pnt(-23.0842,150.004,0.000641323));
        Poles.SetValue(18,1,gp_Pnt(-20.1956,-150.004,-0.000939892));
        Poles.SetValue(18,2,gp_Pnt(-20.2119,-142.265,-0.324814));
        Poles.SetValue(18,3,gp_Pnt(-20.2367,-126.781,-0.99002));
        Poles.SetValue(18,4,gp_Pnt(-20.2522,-103.48,-2.25874));
        Poles.SetValue(18,5,gp_Pnt(-20.3366,-84.3417,-3.14413));
        Poles.SetValue(18,6,gp_Pnt(-20.2198,-70.5658,-4.30028));
        Poles.SetValue(18,7,gp_Pnt(-20.1558,-62.6899,-4.68856));
        Poles.SetValue(18,8,gp_Pnt(-20.1905,-56.9911,-5.31764));
        Poles.SetValue(18,9,gp_Pnt(-20.3412,-51.6127,-7.37483));
        Poles.SetValue(18,10,gp_Pnt(-20.5364,-47.2541,-9.61034));
        Poles.SetValue(18,11,gp_Pnt(-20.6972,-43.7208,-11.486));
        Poles.SetValue(18,12,gp_Pnt(-20.8365,-41.0734,-12.9256));
        Poles.SetValue(18,13,gp_Pnt(-21.037,-38.5159,-14.5838));
        Poles.SetValue(18,14,gp_Pnt(-21.299,-36.0127,-16.2371));
        Poles.SetValue(18,15,gp_Pnt(-21.5046,-33.834,-17.4931));
        Poles.SetValue(18,16,gp_Pnt(-21.6031,-31.945,-18.0503));
        Poles.SetValue(18,17,gp_Pnt(-21.7517,-29.0964,-18.8401));
        Poles.SetValue(18,18,gp_Pnt(-21.7895,-25.4868,-18.7856));
        Poles.SetValue(18,19,gp_Pnt(-21.8447,-21.407,-18.7832));
        Poles.SetValue(18,20,gp_Pnt(-21.8724,-17.2831,-18.7158));
        Poles.SetValue(18,21,gp_Pnt(-21.8885,-13.1476,-18.8247));
        Poles.SetValue(18,22,gp_Pnt(-21.9053,-9.00401,-18.9335));
        Poles.SetValue(18,23,gp_Pnt(-21.9203,-5.89434,-19.0031));
        Poles.SetValue(18,24,gp_Pnt(-21.9444,-1.74696,-19.0678));
        Poles.SetValue(18,25,gp_Pnt(-21.9704,2.39832,-19.1774));
        Poles.SetValue(18,26,gp_Pnt(-21.9895,6.53686,-19.2539));
        Poles.SetValue(18,27,gp_Pnt(-21.9976,9.63356,-19.3732));
        Poles.SetValue(18,28,gp_Pnt(-21.9987,13.7521,-19.5004));
        Poles.SetValue(18,29,gp_Pnt(-21.9832,17.8493,-19.6642));
        Poles.SetValue(18,30,gp_Pnt(-21.9481,21.9195,-19.8425));
        Poles.SetValue(18,31,gp_Pnt(-21.8998,25.958,-20.0751));
        Poles.SetValue(18,32,gp_Pnt(-21.8165,29.4779,-20.0877));
        Poles.SetValue(18,33,gp_Pnt(-21.6537,32.2814,-19.218));
        Poles.SetValue(18,34,gp_Pnt(-21.5332,34.1277,-18.592));
        Poles.SetValue(18,35,gp_Pnt(-21.3059,36.244,-17.2119));
        Poles.SetValue(18,36,gp_Pnt(-21.0772,38.8004,-15.6898));
        Poles.SetValue(18,37,gp_Pnt(-20.8763,41.346,-14.041));
        Poles.SetValue(18,38,gp_Pnt(-20.7928,44.1142,-12.8629));
        Poles.SetValue(18,39,gp_Pnt(-20.6422,47.6606,-11.0393));
        Poles.SetValue(18,40,gp_Pnt(-20.4703,52.0726,-9.0358));
        Poles.SetValue(18,41,gp_Pnt(-20.3288,57.4895,-7.14688));
        Poles.SetValue(18,42,gp_Pnt(-20.3107,63.2274,-6.59652));
        Poles.SetValue(18,43,gp_Pnt(-20.366,71.0607,-5.86066));
        Poles.SetValue(18,44,gp_Pnt(-20.4189,84.6755,-4.47339));
        Poles.SetValue(18,45,gp_Pnt(-20.32,103.734,-2.95074));
        Poles.SetValue(18,46,gp_Pnt(-20.2644,126.899,-1.49374));
        Poles.SetValue(18,47,gp_Pnt(-20.227,142.314,-0.508887));
        Poles.SetValue(18,48,gp_Pnt(-20.1991,150.004,-0.00112432));
        Poles.SetValue(19,1,gp_Pnt(-16.3491,-150.003,0.00439318));
        Poles.SetValue(19,2,gp_Pnt(-16.3656,-142.264,-0.34061));
        Poles.SetValue(19,3,gp_Pnt(-16.3951,-126.785,-0.955435));
        Poles.SetValue(19,4,gp_Pnt(-16.4123,-103.462,-2.32584));
        Poles.SetValue(19,5,gp_Pnt(-16.5104,-84.3597,-3.09639));
        Poles.SetValue(19,6,gp_Pnt(-16.3944,-70.5334,-4.33957));
        Poles.SetValue(19,7,gp_Pnt(-16.3623,-62.6963,-4.6472));
        Poles.SetValue(19,8,gp_Pnt(-16.4203,-57.084,-5.58419));
        Poles.SetValue(19,9,gp_Pnt(-16.5511,-51.7391,-7.92073));
        Poles.SetValue(19,10,gp_Pnt(-16.7137,-47.3952,-10.3398));
        Poles.SetValue(19,11,gp_Pnt(-16.8584,-43.8683,-12.2379));
        Poles.SetValue(19,12,gp_Pnt(-17.0069,-41.2968,-13.8148));
        Poles.SetValue(19,13,gp_Pnt(-17.234,-38.8784,-15.6644));
        Poles.SetValue(19,14,gp_Pnt(-17.4559,-36.3846,-17.2802));
        Poles.SetValue(19,15,gp_Pnt(-17.5711,-34.0769,-18.2311));
        Poles.SetValue(19,16,gp_Pnt(-17.6306,-32.1332,-18.5268));
        Poles.SetValue(19,17,gp_Pnt(-17.7058,-29.1815,-18.8298));
        Poles.SetValue(19,18,gp_Pnt(-17.7222,-25.5444,-18.565));
        Poles.SetValue(19,19,gp_Pnt(-17.755,-21.4437,-18.6143));
        Poles.SetValue(19,20,gp_Pnt(-17.7721,-17.3146,-18.6936));
        Poles.SetValue(19,21,gp_Pnt(-17.7833,-13.1725,-18.7973));
        Poles.SetValue(19,22,gp_Pnt(-17.7956,-9.02146,-18.8982));
        Poles.SetValue(19,23,gp_Pnt(-17.8084,-5.90664,-18.9585));
        Poles.SetValue(19,24,gp_Pnt(-17.8297,-1.75266,-19.0369));
        Poles.SetValue(19,25,gp_Pnt(-17.8562,2.40144,-19.105));
        Poles.SetValue(19,26,gp_Pnt(-17.8776,6.54951,-19.2157));
        Poles.SetValue(19,27,gp_Pnt(-17.8892,9.65525,-19.2919));
        Poles.SetValue(19,28,gp_Pnt(-17.8977,13.7848,-19.4492));
        Poles.SetValue(19,29,gp_Pnt(-17.8891,17.8872,-19.6002));
        Poles.SetValue(19,30,gp_Pnt(-17.8651,21.9666,-19.7805));
        Poles.SetValue(19,31,gp_Pnt(-17.8168,26.0004,-19.778));
        Poles.SetValue(19,32,gp_Pnt(-17.7893,29.5844,-20.0714));
        Poles.SetValue(19,33,gp_Pnt(-17.6844,32.4681,-19.7048));
        Poles.SetValue(19,34,gp_Pnt(-17.6156,34.381,-19.3921));
        Poles.SetValue(19,35,gp_Pnt(-17.468,36.5995,-18.2429));
        Poles.SetValue(19,36,gp_Pnt(-17.273,39.1443,-16.764));
        Poles.SetValue(19,37,gp_Pnt(-17.0603,41.5783,-14.9142));
        Poles.SetValue(19,38,gp_Pnt(-16.942,44.2046,-13.5092));
        Poles.SetValue(19,39,gp_Pnt(-16.8113,47.7336,-11.6467));
        Poles.SetValue(19,40,gp_Pnt(-16.6707,52.1542,-9.5335));
        Poles.SetValue(19,41,gp_Pnt(-16.5537,57.549,-7.39217));
        Poles.SetValue(19,42,gp_Pnt(-16.5136,63.2221,-6.59968));
        Poles.SetValue(19,43,gp_Pnt(-16.5433,71.0421,-5.99115));
        Poles.SetValue(19,44,gp_Pnt(-16.5912,84.6886,-4.45254));
        Poles.SetValue(19,45,gp_Pnt(-16.4831,103.728,-3.08142));
        Poles.SetValue(19,46,gp_Pnt(-16.423,126.896,-1.4405));
        Poles.SetValue(19,47,gp_Pnt(-16.3811,142.316,-0.552973));
        Poles.SetValue(19,48,gp_Pnt(-16.3527,150.004,0.00512247));
        Poles.SetValue(20,1,gp_Pnt(-11.5409,-150.004,0.00117643));
        Poles.SetValue(20,2,gp_Pnt(-11.5567,-142.255,-0.323629));
        Poles.SetValue(20,3,gp_Pnt(-11.5945,-126.808,-1.00415));
        Poles.SetValue(20,4,gp_Pnt(-11.6123,-103.418,-2.2741));
        Poles.SetValue(20,5,gp_Pnt(-11.7238,-84.4038,-3.17774));
        Poles.SetValue(20,6,gp_Pnt(-11.6216,-70.4805,-4.34939));
        Poles.SetValue(20,7,gp_Pnt(-11.6306,-62.7158,-4.61848));
        Poles.SetValue(20,8,gp_Pnt(-11.6985,-57.2166,-5.86008));
        Poles.SetValue(20,9,gp_Pnt(-11.804,-51.9451,-8.59692));
        Poles.SetValue(20,10,gp_Pnt(-11.9017,-47.511,-11.0341));
        Poles.SetValue(20,11,gp_Pnt(-12.0348,-44.054,-13.0527));
        Poles.SetValue(20,12,gp_Pnt(-12.1892,-41.6494,-14.8698));
        Poles.SetValue(20,13,gp_Pnt(-12.3712,-39.2779,-16.7157));
        Poles.SetValue(20,14,gp_Pnt(-12.4805,-36.6354,-17.9375));
        Poles.SetValue(20,15,gp_Pnt(-12.5363,-34.226,-18.5069));
        Poles.SetValue(20,16,gp_Pnt(-12.561,-32.2298,-18.5446));
        Poles.SetValue(20,17,gp_Pnt(-12.6007,-29.2245,-18.4861));
        Poles.SetValue(20,18,gp_Pnt(-12.6061,-25.5943,-18.4035));
        Poles.SetValue(20,19,gp_Pnt(-12.6185,-21.4799,-18.541));
        Poles.SetValue(20,20,gp_Pnt(-12.6259,-17.3464,-18.6219));
        Poles.SetValue(20,21,gp_Pnt(-12.6294,-13.198,-18.7456));
        Poles.SetValue(20,22,gp_Pnt(-12.6383,-9.04337,-18.8541));
        Poles.SetValue(20,23,gp_Pnt(-12.6485,-5.92505,-18.9104));
        Poles.SetValue(20,24,gp_Pnt(-12.6659,-1.76525,-18.964));
        Poles.SetValue(20,25,gp_Pnt(-12.6869,2.39485,-19.067));
        Poles.SetValue(20,26,gp_Pnt(-12.7086,6.55182,-19.1737));
        Poles.SetValue(20,27,gp_Pnt(-12.7237,9.66537,-19.2438));
        Poles.SetValue(20,28,gp_Pnt(-12.7365,13.803,-19.406));
        Poles.SetValue(20,29,gp_Pnt(-12.7387,17.9165,-19.5004));
        Poles.SetValue(20,30,gp_Pnt(-12.725,21.9984,-19.5481));
        Poles.SetValue(20,31,gp_Pnt(-12.7006,26.0496,-19.5971));
        Poles.SetValue(20,32,gp_Pnt(-12.6724,29.61,-19.7481));
        Poles.SetValue(20,33,gp_Pnt(-12.6172,32.5589,-19.7855));
        Poles.SetValue(20,34,gp_Pnt(-12.577,34.5045,-19.6765));
        Poles.SetValue(20,35,gp_Pnt(-12.5091,36.8627,-18.9431));
        Poles.SetValue(20,36,gp_Pnt(-12.3992,39.4504,-17.717));
        Poles.SetValue(20,37,gp_Pnt(-12.2417,41.8732,-15.8993));
        Poles.SetValue(20,38,gp_Pnt(-12.1136,44.3374,-14.2229));
        Poles.SetValue(20,39,gp_Pnt(-12.0124,47.8838,-12.4053));
        Poles.SetValue(20,40,gp_Pnt(-11.9096,52.2497,-10.0193));
        Poles.SetValue(20,41,gp_Pnt(-11.8312,57.645,-7.74397));
        Poles.SetValue(20,42,gp_Pnt(-11.7795,63.2306,-6.59009));
        Poles.SetValue(20,43,gp_Pnt(-11.7676,71.0031,-6.05246));
        Poles.SetValue(20,44,gp_Pnt(-11.803,84.7251,-4.53343));
        Poles.SetValue(20,45,gp_Pnt(-11.6834,103.69,-3.04429));
        Poles.SetValue(20,46,gp_Pnt(-11.6246,126.918,-1.52563));
        Poles.SetValue(20,47,gp_Pnt(-11.5716,142.307,-0.515972));
        Poles.SetValue(20,48,gp_Pnt(-11.5446,150.004,0.000245205));
        Poles.SetValue(21,1,gp_Pnt(-7.21355,-150.004,-0.0025218));
        Poles.SetValue(21,2,gp_Pnt(-7.23063,-142.252,-0.331648));
        Poles.SetValue(21,3,gp_Pnt(-7.2677,-126.82,-1.02226));
        Poles.SetValue(21,4,gp_Pnt(-7.30323,-103.391,-2.23259));
        Poles.SetValue(21,5,gp_Pnt(-7.39757,-84.4224,-3.1889));
        Poles.SetValue(21,6,gp_Pnt(-7.34772,-70.4538,-4.37072));
        Poles.SetValue(21,7,gp_Pnt(-7.3712,-62.7345,-4.5951));
        Poles.SetValue(21,8,gp_Pnt(-7.42816,-57.2934,-6.04634));
        Poles.SetValue(21,9,gp_Pnt(-7.49535,-52.026,-8.86275));
        Poles.SetValue(21,10,gp_Pnt(-7.55838,-47.6033,-11.3943));
        Poles.SetValue(21,11,gp_Pnt(-7.65,-44.2056,-13.5286));
        Poles.SetValue(21,12,gp_Pnt(-7.75695,-41.8805,-15.4454));
        Poles.SetValue(21,13,gp_Pnt(-7.85282,-39.4135,-17.0864));
        Poles.SetValue(21,14,gp_Pnt(-7.91823,-36.7399,-18.1659));
        Poles.SetValue(21,15,gp_Pnt(-7.94146,-34.2627,-18.4245));
        Poles.SetValue(21,16,gp_Pnt(-7.95506,-32.2581,-18.4508));
        Poles.SetValue(21,17,gp_Pnt(-7.97185,-29.2355,-18.438));
        Poles.SetValue(21,18,gp_Pnt(-7.9724,-25.6227,-18.4383));
        Poles.SetValue(21,19,gp_Pnt(-7.97498,-21.5055,-18.5136));
        Poles.SetValue(21,20,gp_Pnt(-7.97332,-17.3696,-18.5895));
        Poles.SetValue(21,21,gp_Pnt(-7.9724,-13.2197,-18.6677));
        Poles.SetValue(21,22,gp_Pnt(-7.97728,-9.06333,-18.7605));
        Poles.SetValue(21,23,gp_Pnt(-7.98383,-5.94346,-18.8376));
        Poles.SetValue(21,24,gp_Pnt(-7.99613,-1.78048,-18.9102));
        Poles.SetValue(21,25,gp_Pnt(-8.0124,2.38413,-18.9749));
        Poles.SetValue(21,26,gp_Pnt(-8.03056,6.5435,-19.1002));
        Poles.SetValue(21,27,gp_Pnt(-8.04412,9.658,-19.2087));
        Poles.SetValue(21,28,gp_Pnt(-8.06164,13.8027,-19.2687));
        Poles.SetValue(21,29,gp_Pnt(-8.0717,17.9217,-19.335));
        Poles.SetValue(21,30,gp_Pnt(-8.07055,22.0086,-19.4167));
        Poles.SetValue(21,31,gp_Pnt(-8.05978,26.0647,-19.4458));
        Poles.SetValue(21,32,gp_Pnt(-8.05131,29.6191,-19.5118));
        Poles.SetValue(21,33,gp_Pnt(-8.01648,32.5759,-19.5302));
        Poles.SetValue(21,34,gp_Pnt(-7.99358,34.5369,-19.5134));
        Poles.SetValue(21,35,gp_Pnt(-7.96305,36.995,-19.2659));
        Poles.SetValue(21,36,gp_Pnt(-7.89264,39.5831,-18.0731));
        Poles.SetValue(21,37,gp_Pnt(-7.80775,42.0631,-16.4261));
        Poles.SetValue(21,38,gp_Pnt(-7.72798,44.4322,-14.6014));
        Poles.SetValue(21,39,gp_Pnt(-7.66064,47.9074,-12.6609));
        Poles.SetValue(21,40,gp_Pnt(-7.60827,52.3383,-10.3218));
        Poles.SetValue(21,41,gp_Pnt(-7.5563,57.6762,-7.76823));
        Poles.SetValue(21,42,gp_Pnt(-7.51475,63.2251,-6.53953));
        Poles.SetValue(21,43,gp_Pnt(-7.4842,70.9764,-6.08044));
        Poles.SetValue(21,44,gp_Pnt(-7.47797,84.7397,-4.49862));
        Poles.SetValue(21,45,gp_Pnt(-7.3721,103.678,-3.1159));
        Poles.SetValue(21,46,gp_Pnt(-7.30045,126.923,-1.49726));
        Poles.SetValue(21,47,gp_Pnt(-7.24466,142.305,-0.525039));
        Poles.SetValue(21,48,gp_Pnt(-7.21746,150.004,0.00128023));
        Poles.SetValue(22,1,gp_Pnt(-4.32857,-150.003,-0.00540373));
        Poles.SetValue(22,2,gp_Pnt(-4.34699,-142.25,-0.340193));
        Poles.SetValue(22,3,gp_Pnt(-4.38209,-126.827,-1.02934));
        Poles.SetValue(22,4,gp_Pnt(-4.43222,-103.375,-2.20818));
        Poles.SetValue(22,5,gp_Pnt(-4.51083,-84.4311,-3.19069));
        Poles.SetValue(22,6,gp_Pnt(-4.50022,-70.4378,-4.36422));
        Poles.SetValue(22,7,gp_Pnt(-4.53329,-62.7487,-4.58414));
        Poles.SetValue(22,8,gp_Pnt(-4.57686,-57.3432,-6.16997));
        Poles.SetValue(22,9,gp_Pnt(-4.61953,-52.0531,-8.97091));
        Poles.SetValue(22,10,gp_Pnt(-4.66076,-47.6551,-11.5891));
        Poles.SetValue(22,11,gp_Pnt(-4.71605,-44.2575,-13.7456));
        Poles.SetValue(22,12,gp_Pnt(-4.77767,-41.9525,-15.6786));
        Poles.SetValue(22,13,gp_Pnt(-4.83539,-39.4972,-17.3021));
        Poles.SetValue(22,14,gp_Pnt(-4.8662,-36.7917,-18.2384));
        Poles.SetValue(22,15,gp_Pnt(-4.87772,-34.2883,-18.3918));
        Poles.SetValue(22,16,gp_Pnt(-4.88127,-32.275,-18.4126));
        Poles.SetValue(22,17,gp_Pnt(-4.88815,-29.2471,-18.3654));
        Poles.SetValue(22,18,gp_Pnt(-4.88172,-25.6402,-18.4073));
        Poles.SetValue(22,19,gp_Pnt(-4.87676,-21.522,-18.4843));
        Poles.SetValue(22,20,gp_Pnt(-4.86998,-17.3855,-18.5721));
        Poles.SetValue(22,21,gp_Pnt(-4.86604,-13.2345,-18.6261));
        Poles.SetValue(22,22,gp_Pnt(-4.868,-9.07675,-18.7038));
        Poles.SetValue(22,23,gp_Pnt(-4.87197,-5.95561,-18.7836));
        Poles.SetValue(22,24,gp_Pnt(-4.88122,-1.79016,-18.8508));
        Poles.SetValue(22,25,gp_Pnt(-4.89416,2.37596,-18.9454));
        Poles.SetValue(22,26,gp_Pnt(-4.91129,6.53746,-19.0539));
        Poles.SetValue(22,27,gp_Pnt(-4.92518,9.65454,-19.1026));
        Poles.SetValue(22,28,gp_Pnt(-4.944,13.8012,-19.1613));
        Poles.SetValue(22,29,gp_Pnt(-4.95795,17.9235,-19.2456));
        Poles.SetValue(22,30,gp_Pnt(-4.96475,22.0131,-19.3102));
        Poles.SetValue(22,31,gp_Pnt(-4.96533,26.0736,-19.3498));
        Poles.SetValue(22,32,gp_Pnt(-4.96079,29.6176,-19.4284));
        Poles.SetValue(22,33,gp_Pnt(-4.94348,32.5873,-19.466));
        Poles.SetValue(22,34,gp_Pnt(-4.93172,34.561,-19.4669));
        Poles.SetValue(22,35,gp_Pnt(-4.91254,37.0161,-19.3038));
        Poles.SetValue(22,36,gp_Pnt(-4.88212,39.6621,-18.2512));
        Poles.SetValue(22,37,gp_Pnt(-4.83796,42.1306,-16.6572));
        Poles.SetValue(22,38,gp_Pnt(-4.79279,44.4479,-14.7487));
        Poles.SetValue(22,39,gp_Pnt(-4.75792,47.9195,-12.7903));
        Poles.SetValue(22,40,gp_Pnt(-4.73201,52.3475,-10.3957));
        Poles.SetValue(22,41,gp_Pnt(-4.70638,57.7231,-7.87102));
        Poles.SetValue(22,42,gp_Pnt(-4.67107,63.2171,-6.47477));
        Poles.SetValue(22,43,gp_Pnt(-4.63073,70.9601,-6.08756));
        Poles.SetValue(22,44,gp_Pnt(-4.59233,84.7477,-4.47556));
        Poles.SetValue(22,45,gp_Pnt(-4.49985,103.673,-3.18097));
        Poles.SetValue(22,46,gp_Pnt(-4.41639,126.923,-1.46262));
        Poles.SetValue(22,47,gp_Pnt(-4.36049,142.305,-0.536542));
        Poles.SetValue(22,48,gp_Pnt(-4.3327,150.004,0.000952771));
        Poles.SetValue(23,1,gp_Pnt(-2.88635,-150.004,-0.00575623));
        Poles.SetValue(23,2,gp_Pnt(-2.90517,-142.249,-0.341549));
        Poles.SetValue(23,3,gp_Pnt(-2.93974,-126.829,-1.02278));
        Poles.SetValue(23,4,gp_Pnt(-2.99795,-103.372,-2.19554));
        Poles.SetValue(23,5,gp_Pnt(-3.06615,-84.432,-3.18891));
        Poles.SetValue(23,6,gp_Pnt(-3.07817,-70.436,-4.35294));
        Poles.SetValue(23,7,gp_Pnt(-3.11358,-62.751,-4.58803));
        Poles.SetValue(23,8,gp_Pnt(-3.149,-57.3489,-6.16224));
        Poles.SetValue(23,9,gp_Pnt(-3.18087,-52.0832,-9.02443));
        Poles.SetValue(23,10,gp_Pnt(-3.20794,-47.6671,-11.6136));
        Poles.SetValue(23,11,gp_Pnt(-3.24262,-44.2844,-13.7946));
        Poles.SetValue(23,12,gp_Pnt(-3.28123,-41.9804,-15.721));
        Poles.SetValue(23,13,gp_Pnt(-3.31573,-39.5144,-17.3255));
        Poles.SetValue(23,14,gp_Pnt(-3.33437,-36.7949,-18.2234));
        Poles.SetValue(23,15,gp_Pnt(-3.34011,-34.2951,-18.3746));
        Poles.SetValue(23,16,gp_Pnt(-3.34154,-32.2808,-18.382));
        Poles.SetValue(23,17,gp_Pnt(-3.34327,-29.2512,-18.3471));
        Poles.SetValue(23,18,gp_Pnt(-3.33398,-25.6485,-18.3903));
        Poles.SetValue(23,19,gp_Pnt(-3.32526,-21.5295,-18.4702));
        Poles.SetValue(23,20,gp_Pnt(-3.31575,-17.3931,-18.5594));
        Poles.SetValue(23,21,gp_Pnt(-3.30973,-13.2415,-18.6106));
        Poles.SetValue(23,22,gp_Pnt(-3.3096,-9.08307,-18.6879));
        Poles.SetValue(23,23,gp_Pnt(-3.31238,-5.96107,-18.7525));
        Poles.SetValue(23,24,gp_Pnt(-3.31981,-1.7953,-18.8291));
        Poles.SetValue(23,25,gp_Pnt(-3.33173,2.3711,-18.9327));
        Poles.SetValue(23,26,gp_Pnt(-3.34803,6.53266,-19.0255));
        Poles.SetValue(23,27,gp_Pnt(-3.36177,9.6497,-19.0722));
        Poles.SetValue(23,28,gp_Pnt(-3.38073,13.796,-19.1329));
        Poles.SetValue(23,29,gp_Pnt(-3.39705,17.9191,-19.2008));
        Poles.SetValue(23,30,gp_Pnt(-3.40752,22.0091,-19.2675));
        Poles.SetValue(23,31,gp_Pnt(-3.41262,26.0715,-19.318));
        Poles.SetValue(23,32,gp_Pnt(-3.4138,29.6137,-19.3688));
        Poles.SetValue(23,33,gp_Pnt(-3.40248,32.5858,-19.4134));
        Poles.SetValue(23,34,gp_Pnt(-3.39479,34.5605,-19.4192));
        Poles.SetValue(23,35,gp_Pnt(-3.38139,37.0117,-19.2935));
        Poles.SetValue(23,36,gp_Pnt(-3.36593,39.6724,-18.2438));
        Poles.SetValue(23,37,gp_Pnt(-3.34268,42.1365,-16.6785));
        Poles.SetValue(23,38,gp_Pnt(-3.32014,44.4714,-14.7844));
        Poles.SetValue(23,39,gp_Pnt(-3.30336,47.9303,-12.8108));
        Poles.SetValue(23,40,gp_Pnt(-3.29238,52.3621,-10.4158));
        Poles.SetValue(23,41,gp_Pnt(-3.27781,57.7309,-7.88219));
        Poles.SetValue(23,42,gp_Pnt(-3.24892,63.2201,-6.4727));
        Poles.SetValue(23,43,gp_Pnt(-3.20448,70.9551,-6.07441));
        Poles.SetValue(23,44,gp_Pnt(-3.14808,84.7489,-4.47297));
        Poles.SetValue(23,45,gp_Pnt(-3.06369,103.669,-3.15564));
        Poles.SetValue(23,46,gp_Pnt(-2.97399,126.928,-1.47488));
        Poles.SetValue(23,47,gp_Pnt(-2.9181,142.304,-0.533731));
        Poles.SetValue(23,48,gp_Pnt(-2.8902,150.005,-0.00415237));
        Poles.SetValue(24,1,gp_Pnt(-0.963346,-150.005,-0.00635749));
        Poles.SetValue(24,2,gp_Pnt(-0.982765,-142.248,-0.343803));
        Poles.SetValue(24,3,gp_Pnt(-1.01653,-126.833,-1.01474));
        Poles.SetValue(24,4,gp_Pnt(-1.0855,-103.368,-2.17966));
        Poles.SetValue(24,5,gp_Pnt(-1.14,-84.4332,-3.18535));
        Poles.SetValue(24,6,gp_Pnt(-1.18193,-70.4337,-4.33879));
        Poles.SetValue(24,7,gp_Pnt(-1.22089,-62.7532,-4.58618));
        Poles.SetValue(24,8,gp_Pnt(-1.24557,-57.3605,-6.16284));
        Poles.SetValue(24,9,gp_Pnt(-1.26254,-52.1152,-9.08105));
        Poles.SetValue(24,10,gp_Pnt(-1.27132,-47.6865,-11.6584));
        Poles.SetValue(24,11,gp_Pnt(-1.27992,-44.3077,-13.8385));
        Poles.SetValue(24,12,gp_Pnt(-1.28505,-42.028,-15.8032));
        Poles.SetValue(24,13,gp_Pnt(-1.29202,-39.5278,-17.3361));
        Poles.SetValue(24,14,gp_Pnt(-1.29044,-36.8146,-18.2432));
        Poles.SetValue(24,15,gp_Pnt(-1.2914,-34.2976,-18.337));
        Poles.SetValue(24,16,gp_Pnt(-1.28862,-32.2873,-18.3422));
        Poles.SetValue(24,17,gp_Pnt(-1.28371,-29.2589,-18.3188));
        Poles.SetValue(24,18,gp_Pnt(-1.27051,-25.6583,-18.3674));
        Poles.SetValue(24,19,gp_Pnt(-1.25674,-21.5399,-18.4519));
        Poles.SetValue(24,20,gp_Pnt(-1.24362,-17.4031,-18.5423));
        Poles.SetValue(24,21,gp_Pnt(-1.23485,-13.2508,-18.5894));
        Poles.SetValue(24,22,gp_Pnt(-1.23205,-9.09139,-18.6638));
        Poles.SetValue(24,23,gp_Pnt(-1.23315,-5.96846,-18.7159));
        Poles.SetValue(24,24,gp_Pnt(-1.23832,-1.80206,-18.7988));
        Poles.SetValue(24,25,gp_Pnt(-1.24872,2.36463,-18.9162));
        Poles.SetValue(24,26,gp_Pnt(-1.26403,6.5264,-18.9901));
        Poles.SetValue(24,27,gp_Pnt(-1.27763,9.64349,-19.0285));
        Poles.SetValue(24,28,gp_Pnt(-1.29677,13.7898,-19.0962));
        Poles.SetValue(24,29,gp_Pnt(-1.31634,17.9135,-19.1409));
        Poles.SetValue(24,30,gp_Pnt(-1.33161,22.005,-19.2088));
        Poles.SetValue(24,31,gp_Pnt(-1.34301,26.0674,-19.2798));
        Poles.SetValue(24,32,gp_Pnt(-1.35106,29.6117,-19.2876));
        Poles.SetValue(24,33,gp_Pnt(-1.34846,32.5823,-19.3398));
        Poles.SetValue(24,34,gp_Pnt(-1.34527,34.5559,-19.385));
        Poles.SetValue(24,35,gp_Pnt(-1.34071,37.0193,-19.2612));
        Poles.SetValue(24,36,gp_Pnt(-1.34509,39.6789,-18.263));
        Poles.SetValue(24,37,gp_Pnt(-1.35008,42.1558,-16.6984));
        Poles.SetValue(24,38,gp_Pnt(-1.35773,44.4897,-14.8274));
        Poles.SetValue(24,39,gp_Pnt(-1.36444,47.9479,-12.8384));
        Poles.SetValue(24,40,gp_Pnt(-1.37307,52.376,-10.4394));
        Poles.SetValue(24,41,gp_Pnt(-1.37346,57.7433,-7.89619));
        Poles.SetValue(24,42,gp_Pnt(-1.35291,63.2224,-6.46618));
        Poles.SetValue(24,43,gp_Pnt(-1.30269,70.9487,-6.0561));
        Poles.SetValue(24,44,gp_Pnt(-1.2225,84.7507,-4.47221));
        Poles.SetValue(24,45,gp_Pnt(-1.1488,103.665,-3.1278));
        Poles.SetValue(24,46,gp_Pnt(-1.05086,126.933,-1.48899));
        Poles.SetValue(24,47,gp_Pnt(-0.994938,142.302,-0.530106));
        Poles.SetValue(24,48,gp_Pnt(-0.966871,150.006,-0.0110631));
        Poles.SetValue(25,1,gp_Pnt(0.959819,-150.003,0.00568631));
        Poles.SetValue(25,2,gp_Pnt(0.940055,-142.247,-0.331366));
        Poles.SetValue(25,3,gp_Pnt(0.906483,-126.833,-0.995719));
        Poles.SetValue(25,4,gp_Pnt(0.827614,-103.361,-2.15145));
        Poles.SetValue(25,5,gp_Pnt(0.785273,-84.4342,-3.16894));
        Poles.SetValue(25,6,gp_Pnt(0.714392,-70.4325,-4.31753));
        Poles.SetValue(25,7,gp_Pnt(0.672237,-62.7537,-4.559));
        Poles.SetValue(25,8,gp_Pnt(0.659329,-57.3653,-6.14528));
        Poles.SetValue(25,9,gp_Pnt(0.661645,-52.1004,-9.03387));
        Poles.SetValue(25,10,gp_Pnt(0.669516,-47.6625,-11.5964));
        Poles.SetValue(25,11,gp_Pnt(0.689569,-44.3049,-13.8062));
        Poles.SetValue(25,12,gp_Pnt(0.71644,-42.0351,-15.7831));
        Poles.SetValue(25,13,gp_Pnt(0.738005,-39.535,-17.3193));
        Poles.SetValue(25,14,gp_Pnt(0.754916,-36.82,-18.2146));
        Poles.SetValue(25,15,gp_Pnt(0.759797,-34.3048,-18.3011));
        Poles.SetValue(25,16,gp_Pnt(0.766913,-32.293,-18.3232));
        Poles.SetValue(25,17,gp_Pnt(0.776889,-29.2662,-18.2948));
        Poles.SetValue(25,18,gp_Pnt(0.794825,-25.6669,-18.3503));
        Poles.SetValue(25,19,gp_Pnt(0.812837,-21.5492,-18.4233));
        Poles.SetValue(25,20,gp_Pnt(0.829745,-17.4125,-18.5129));
        Poles.SetValue(25,21,gp_Pnt(0.841595,-13.2595,-18.5688));
        Poles.SetValue(25,22,gp_Pnt(0.847267,-9.09893,-18.6407));
        Poles.SetValue(25,23,gp_Pnt(0.848219,-5.97561,-18.6984));
        Poles.SetValue(25,24,gp_Pnt(0.844946,-1.80848,-18.7707));
        Poles.SetValue(25,25,gp_Pnt(0.835788,2.35803,-18.8937));
        Poles.SetValue(25,26,gp_Pnt(0.820538,6.52081,-18.96));
        Poles.SetValue(25,27,gp_Pnt(0.806453,9.63874,-18.9827));
        Poles.SetValue(25,28,gp_Pnt(0.786356,13.7858,-19.059));
        Poles.SetValue(25,29,gp_Pnt(0.763708,17.9106,-19.0832));
        Poles.SetValue(25,30,gp_Pnt(0.743879,22.0023,-19.1557));
        Poles.SetValue(25,31,gp_Pnt(0.727329,26.0648,-19.2442));
        Poles.SetValue(25,32,gp_Pnt(0.712384,29.6076,-19.2121));
        Poles.SetValue(25,33,gp_Pnt(0.709962,32.5804,-19.3472));
        Poles.SetValue(25,34,gp_Pnt(0.705581,34.5476,-19.3214));
        Poles.SetValue(25,35,gp_Pnt(0.704066,37.0253,-19.2554));
        Poles.SetValue(25,36,gp_Pnt(0.68005,39.6706,-18.2118));
        Poles.SetValue(25,37,gp_Pnt(0.648205,42.1617,-16.677));
        Poles.SetValue(25,38,gp_Pnt(0.61021,44.4849,-14.7789));
        Poles.SetValue(25,39,gp_Pnt(0.578672,47.947,-12.7969));
        Poles.SetValue(25,40,gp_Pnt(0.549361,52.3609,-10.3659));
        Poles.SetValue(25,41,gp_Pnt(0.531102,57.7221,-7.80854));
        Poles.SetValue(25,42,gp_Pnt(0.542483,63.2121,-6.41916));
        Poles.SetValue(25,43,gp_Pnt(0.600457,70.9445,-6.01981));
        Poles.SetValue(25,44,gp_Pnt(0.701564,84.7526,-4.48639));
        Poles.SetValue(25,45,gp_Pnt(0.766341,103.657,-3.09192));
        Poles.SetValue(25,46,gp_Pnt(0.870496,126.929,-1.44947));
        Poles.SetValue(25,47,gp_Pnt(0.927725,142.298,-0.506458));
        Poles.SetValue(25,48,gp_Pnt(0.955638,150.002,0.013664));
        Poles.SetValue(26,1,gp_Pnt(2.88276,-150.003,0.00510712));
        Poles.SetValue(26,2,gp_Pnt(2.86273,-142.249,-0.334571));
        Poles.SetValue(26,3,gp_Pnt(2.82941,-126.831,-0.981236));
        Poles.SetValue(26,4,gp_Pnt(2.74035,-103.363,-2.14876));
        Poles.SetValue(26,5,gp_Pnt(2.71066,-84.4321,-3.14812));
        Poles.SetValue(26,6,gp_Pnt(2.61085,-70.4333,-4.28563));
        Poles.SetValue(26,7,gp_Pnt(2.56523,-62.7558,-4.53254));
        Poles.SetValue(26,8,gp_Pnt(2.56347,-57.3598,-6.10287));
        Poles.SetValue(26,9,gp_Pnt(2.58158,-52.1003,-8.99526));
        Poles.SetValue(26,10,gp_Pnt(2.60803,-47.6567,-11.5441));
        Poles.SetValue(26,11,gp_Pnt(2.65382,-44.2876,-13.7304));
        Poles.SetValue(26,12,gp_Pnt(2.71408,-42.0076,-15.6963));
        Poles.SetValue(26,13,gp_Pnt(2.7632,-39.5234,-17.2513));
        Poles.SetValue(26,14,gp_Pnt(2.80034,-36.8171,-18.1742));
        Poles.SetValue(26,15,gp_Pnt(2.81048,-34.3087,-18.2745));
        Poles.SetValue(26,16,gp_Pnt(2.82183,-32.2984,-18.2966));
        Poles.SetValue(26,17,gp_Pnt(2.83832,-29.2732,-18.2842));
        Poles.SetValue(26,18,gp_Pnt(2.8597,-25.6748,-18.3055));
        Poles.SetValue(26,19,gp_Pnt(2.88306,-21.5575,-18.3968));
        Poles.SetValue(26,20,gp_Pnt(2.9041,-17.4213,-18.4977));
        Poles.SetValue(26,21,gp_Pnt(2.91912,-13.2677,-18.5504));
        Poles.SetValue(26,22,gp_Pnt(2.92733,-9.10621,-18.6181));
        Poles.SetValue(26,23,gp_Pnt(2.9299,-5.98236,-18.6736));
        Poles.SetValue(26,24,gp_Pnt(2.92848,-1.81523,-18.7537));
        Poles.SetValue(26,25,gp_Pnt(2.92031,2.35123,-18.8647));
        Poles.SetValue(26,26,gp_Pnt(2.90582,6.51345,-18.9446));
        Poles.SetValue(26,27,gp_Pnt(2.89198,9.63105,-18.9812));
        Poles.SetValue(26,28,gp_Pnt(2.87035,13.7782,-19.0068));
        Poles.SetValue(26,29,gp_Pnt(2.84585,17.9026,-19.0689));
        Poles.SetValue(26,30,gp_Pnt(2.82094,21.9938,-19.1239));
        Poles.SetValue(26,31,gp_Pnt(2.79658,26.0594,-19.1547));
        Poles.SetValue(26,32,gp_Pnt(2.77809,29.5996,-19.2041));
        Poles.SetValue(26,33,gp_Pnt(2.76651,32.5773,-19.2914));
        Poles.SetValue(26,34,gp_Pnt(2.75864,34.5628,-19.3306));
        Poles.SetValue(26,35,gp_Pnt(2.74443,36.979,-19.1345));
        Poles.SetValue(26,36,gp_Pnt(2.70387,39.6961,-18.1971));
        Poles.SetValue(26,37,gp_Pnt(2.63982,42.114,-16.5479));
        Poles.SetValue(26,38,gp_Pnt(2.5746,44.4902,-14.7277));
        Poles.SetValue(26,39,gp_Pnt(2.51798,47.9308,-12.7131));
        Poles.SetValue(26,40,gp_Pnt(2.46965,52.3578,-10.3138));
        Poles.SetValue(26,41,gp_Pnt(2.43642,57.712,-7.75158));
        Poles.SetValue(26,42,gp_Pnt(2.43995,63.2076,-6.38184));
        Poles.SetValue(26,43,gp_Pnt(2.50293,70.9438,-5.9876));
        Poles.SetValue(26,44,gp_Pnt(2.62733,84.7471,-4.46429));
        Poles.SetValue(26,45,gp_Pnt(2.68225,103.665,-3.11603));
        Poles.SetValue(26,46,gp_Pnt(2.79269,126.925,-1.43235));
        Poles.SetValue(26,47,gp_Pnt(2.85123,142.301,-0.519032));
        Poles.SetValue(26,48,gp_Pnt(2.87892,150.003,0.00754219));
        Poles.SetValue(27,1,gp_Pnt(4.32494,-150.003,0.00465984));
        Poles.SetValue(27,2,gp_Pnt(4.30474,-142.25,-0.336806));
        Poles.SetValue(27,3,gp_Pnt(4.2716,-126.829,-0.969986));
        Poles.SetValue(27,4,gp_Pnt(4.17478,-103.363,-2.1468));
        Poles.SetValue(27,5,gp_Pnt(4.15476,-84.4306,-3.13459));
        Poles.SetValue(27,6,gp_Pnt(4.0329,-70.4336,-4.25922));
        Poles.SetValue(27,7,gp_Pnt(3.98517,-62.757,-4.50984));
        Poles.SetValue(27,8,gp_Pnt(3.9913,-57.3574,-6.07543));
        Poles.SetValue(27,9,gp_Pnt(4.02248,-52.0977,-8.96381));
        Poles.SetValue(27,10,gp_Pnt(4.06134,-47.6524,-11.5091));
        Poles.SetValue(27,11,gp_Pnt(4.12878,-44.273,-13.6756));
        Poles.SetValue(27,12,gp_Pnt(4.21141,-41.9866,-15.6295));
        Poles.SetValue(27,13,gp_Pnt(4.28353,-39.5132,-17.2059));
        Poles.SetValue(27,14,gp_Pnt(4.33348,-36.8179,-18.1432));
        Poles.SetValue(27,15,gp_Pnt(4.34938,-34.3123,-18.2685));
        Poles.SetValue(27,16,gp_Pnt(4.36331,-32.3022,-18.2773));
        Poles.SetValue(27,17,gp_Pnt(4.38418,-29.2798,-18.2738));
        Poles.SetValue(27,18,gp_Pnt(4.40847,-25.6802,-18.2617));
        Poles.SetValue(27,19,gp_Pnt(4.43585,-21.5644,-18.385));
        Poles.SetValue(27,20,gp_Pnt(4.45998,-17.428,-18.4847));
        Poles.SetValue(27,21,gp_Pnt(4.47734,-13.274,-18.5362));
        Poles.SetValue(27,22,gp_Pnt(4.48745,-9.11172,-18.5999));
        Poles.SetValue(27,23,gp_Pnt(4.49128,-5.98756,-18.6581));
        Poles.SetValue(27,24,gp_Pnt(4.49121,-1.8203,-18.7405));
        Poles.SetValue(27,25,gp_Pnt(4.48385,2.34609,-18.8459));
        Poles.SetValue(27,26,gp_Pnt(4.46986,6.50806,-18.9314));
        Poles.SetValue(27,27,gp_Pnt(4.45634,9.62519,-18.9867));
        Poles.SetValue(27,28,gp_Pnt(4.43334,13.773,-18.962));
        Poles.SetValue(27,29,gp_Pnt(4.40783,17.8965,-19.0659));
        Poles.SetValue(27,30,gp_Pnt(4.37876,21.9885,-19.0986));
        Poles.SetValue(27,31,gp_Pnt(4.34905,26.0539,-19.0891));
        Poles.SetValue(27,32,gp_Pnt(4.32668,29.5961,-19.1878));
        Poles.SetValue(27,33,gp_Pnt(4.30965,32.5756,-19.2608));
        Poles.SetValue(27,34,gp_Pnt(4.29649,34.547,-19.2801));
        Poles.SetValue(27,35,gp_Pnt(4.27784,36.9972,-19.1272));
        Poles.SetValue(27,36,gp_Pnt(4.21947,39.6638,-18.1139));
        Poles.SetValue(27,37,gp_Pnt(4.13554,42.1168,-16.5088));
        Poles.SetValue(27,38,gp_Pnt(4.04783,44.4726,-14.6594));
        Poles.SetValue(27,39,gp_Pnt(3.97295,47.9284,-12.6694));
        Poles.SetValue(27,40,gp_Pnt(3.90975,52.3484,-10.2657));
        Poles.SetValue(27,41,gp_Pnt(3.86598,57.7104,-7.72202));
        Poles.SetValue(27,42,gp_Pnt(3.86291,63.2023,-6.3517));
        Poles.SetValue(27,43,gp_Pnt(3.92956,70.9439,-5.96309));
        Poles.SetValue(27,44,gp_Pnt(4.07168,84.743,-4.45222));
        Poles.SetValue(27,45,gp_Pnt(4.11915,103.671,-3.13599));
        Poles.SetValue(27,46,gp_Pnt(4.23427,126.922,-1.42098));
        Poles.SetValue(27,47,gp_Pnt(4.2939,142.303,-0.528462));
        Poles.SetValue(27,48,gp_Pnt(4.3214,150.003,0.00181057));
        Poles.SetValue(28,1,gp_Pnt(7.20975,-150.003,0.00199379));
        Poles.SetValue(28,2,gp_Pnt(7.18882,-142.253,-0.339409));
        Poles.SetValue(28,3,gp_Pnt(7.15603,-126.823,-0.960991));
        Poles.SetValue(28,4,gp_Pnt(7.04705,-103.378,-2.14121));
        Poles.SetValue(28,5,gp_Pnt(7.04011,-84.4152,-3.0599));
        Poles.SetValue(28,6,gp_Pnt(6.8827,-70.4519,-4.22491));
        Poles.SetValue(28,7,gp_Pnt(6.82633,-62.7416,-4.45356));
        Poles.SetValue(28,8,gp_Pnt(6.84632,-57.3274,-5.92928));
        Poles.SetValue(28,9,gp_Pnt(6.90313,-52.0603,-8.76894));
        Poles.SetValue(28,10,gp_Pnt(6.96541,-47.6191,-11.2651));
        Poles.SetValue(28,11,gp_Pnt(7.06713,-44.2153,-13.3835));
        Poles.SetValue(28,12,gp_Pnt(7.19512,-41.9163,-15.3266));
        Poles.SetValue(28,13,gp_Pnt(7.31552,-39.4707,-16.9917));
        Poles.SetValue(28,14,gp_Pnt(7.39349,-36.7863,-18.0229));
        Poles.SetValue(28,15,gp_Pnt(7.42239,-34.3003,-18.2325));
        Poles.SetValue(28,16,gp_Pnt(7.44451,-32.3001,-18.2769));
        Poles.SetValue(28,17,gp_Pnt(7.47675,-29.2865,-18.2652));
        Poles.SetValue(28,18,gp_Pnt(7.50629,-25.6849,-18.2576));
        Poles.SetValue(28,19,gp_Pnt(7.5408,-21.5692,-18.3235));
        Poles.SetValue(28,20,gp_Pnt(7.57163,-17.4339,-18.426));
        Poles.SetValue(28,21,gp_Pnt(7.59489,-13.282,-18.5025));
        Poles.SetValue(28,22,gp_Pnt(7.60855,-9.11981,-18.5619));
        Poles.SetValue(28,23,gp_Pnt(7.61471,-5.99647,-18.6513));
        Poles.SetValue(28,24,gp_Pnt(7.6166,-1.8301,-18.7256));
        Poles.SetValue(28,25,gp_Pnt(7.61019,2.33523,-18.8123));
        Poles.SetValue(28,26,gp_Pnt(7.59637,6.49519,-18.8964));
        Poles.SetValue(28,27,gp_Pnt(7.5818,9.61113,-18.9175));
        Poles.SetValue(28,28,gp_Pnt(7.55779,13.7559,-18.9587));
        Poles.SetValue(28,29,gp_Pnt(7.52587,17.8776,-18.9626));
        Poles.SetValue(28,30,gp_Pnt(7.4901,21.9674,-19.0247));
        Poles.SetValue(28,31,gp_Pnt(7.45312,26.0294,-19.1257));
        Poles.SetValue(28,32,gp_Pnt(7.42514,29.5856,-19.167));
        Poles.SetValue(28,33,gp_Pnt(7.39395,32.555,-19.2183));
        Poles.SetValue(28,34,gp_Pnt(7.37239,34.5295,-19.2131));
        Poles.SetValue(28,35,gp_Pnt(7.33162,36.946,-18.9418));
        Poles.SetValue(28,36,gp_Pnt(7.24313,39.6032,-17.8455));
        Poles.SetValue(28,37,gp_Pnt(7.11296,42.0514,-16.191));
        Poles.SetValue(28,38,gp_Pnt(6.98647,44.4381,-14.3835));
        Poles.SetValue(28,39,gp_Pnt(6.88127,47.9054,-12.4221));
        Poles.SetValue(28,40,gp_Pnt(6.79154,52.3186,-10.0449));
        Poles.SetValue(28,41,gp_Pnt(6.72185,57.6668,-7.53612));
        Poles.SetValue(28,42,gp_Pnt(6.71345,63.1976,-6.30207));
        Poles.SetValue(28,43,gp_Pnt(6.78803,70.9427,-5.8544));
        Poles.SetValue(28,44,gp_Pnt(6.9604,84.7347,-4.44573));
        Poles.SetValue(28,45,gp_Pnt(6.99436,103.676,-3.08077));
        Poles.SetValue(28,46,gp_Pnt(7.11838,126.92,-1.41297));
        Poles.SetValue(28,47,gp_Pnt(7.17835,142.303,-0.517264));
        Poles.SetValue(28,48,gp_Pnt(7.20607,150.004,0.000972213));
        Poles.SetValue(29,1,gp_Pnt(11.537,-150.004,-0.0019364));
        Poles.SetValue(29,2,gp_Pnt(11.5153,-142.257,-0.344646));
        Poles.SetValue(29,3,gp_Pnt(11.4814,-126.814,-0.945148));
        Poles.SetValue(29,4,gp_Pnt(11.3573,-103.399,-2.13514));
        Poles.SetValue(29,5,gp_Pnt(11.367,-84.3943,-2.97324));
        Poles.SetValue(29,6,gp_Pnt(11.1587,-70.4767,-4.1361));
        Poles.SetValue(29,7,gp_Pnt(11.0926,-62.7343,-4.37369));
        Poles.SetValue(29,8,gp_Pnt(11.1242,-57.2568,-5.69537));
        Poles.SetValue(29,9,gp_Pnt(11.2126,-51.9572,-8.35841));
        Poles.SetValue(29,10,gp_Pnt(11.3271,-47.5822,-10.8942));
        Poles.SetValue(29,11,gp_Pnt(11.4644,-44.1049,-12.8681));
        Poles.SetValue(29,12,gp_Pnt(11.6405,-41.7204,-14.7153));
        Poles.SetValue(29,13,gp_Pnt(11.826,-39.3053,-16.4686));
        Poles.SetValue(29,14,gp_Pnt(11.9708,-36.7063,-17.7552));
        Poles.SetValue(29,15,gp_Pnt(12.0323,-34.2848,-18.2164));
        Poles.SetValue(29,16,gp_Pnt(12.0665,-32.2944,-18.2456));
        Poles.SetValue(29,17,gp_Pnt(12.1134,-29.2928,-18.2183));
        Poles.SetValue(29,18,gp_Pnt(12.1502,-25.6859,-18.2702));
        Poles.SetValue(29,19,gp_Pnt(12.1962,-21.5727,-18.2883));
        Poles.SetValue(29,20,gp_Pnt(12.2377,-17.4404,-18.3531));
        Poles.SetValue(29,21,gp_Pnt(12.2704,-13.2922,-18.4587));
        Poles.SetValue(29,22,gp_Pnt(12.2909,-9.1316,-18.5394));
        Poles.SetValue(29,23,gp_Pnt(12.3004,-6.00847,-18.5853));
        Poles.SetValue(29,24,gp_Pnt(12.3062,-1.84393,-18.668));
        Poles.SetValue(29,25,gp_Pnt(12.3017,2.31872,-18.7656));
        Poles.SetValue(29,26,gp_Pnt(12.2867,6.47589,-18.8211));
        Poles.SetValue(29,27,gp_Pnt(12.27,9.58913,-18.8432));
        Poles.SetValue(29,28,gp_Pnt(12.242,13.7302,-18.8857));
        Poles.SetValue(29,29,gp_Pnt(12.2013,17.8471,-18.8567));
        Poles.SetValue(29,30,gp_Pnt(12.1551,21.9335,-18.9411));
        Poles.SetValue(29,31,gp_Pnt(12.1027,25.9912,-19.0065));
        Poles.SetValue(29,32,gp_Pnt(12.0689,29.5626,-19.1079));
        Poles.SetValue(29,33,gp_Pnt(12.0248,32.536,-19.2146));
        Poles.SetValue(29,34,gp_Pnt(11.9908,34.506,-19.2085));
        Poles.SetValue(29,35,gp_Pnt(11.9133,36.8769,-18.6385));
        Poles.SetValue(29,36,gp_Pnt(11.7541,39.4633,-17.3418));
        Poles.SetValue(29,37,gp_Pnt(11.5496,41.8804,-15.5601));
        Poles.SetValue(29,38,gp_Pnt(11.3811,44.3392,-13.8642));
        Poles.SetValue(29,39,gp_Pnt(11.2454,47.87,-12.0305));
        Poles.SetValue(29,40,gp_Pnt(11.1037,52.2433,-9.63697));
        Poles.SetValue(29,41,gp_Pnt(11.0077,57.6157,-7.26995));
        Poles.SetValue(29,42,gp_Pnt(10.9873,63.1748,-6.14324));
        Poles.SetValue(29,43,gp_Pnt(11.0809,70.9505,-5.75361));
        Poles.SetValue(29,44,gp_Pnt(11.2905,84.7164,-4.38829));
        Poles.SetValue(29,45,gp_Pnt(11.3097,103.688,-3.03385));
        Poles.SetValue(29,46,gp_Pnt(11.443,126.913,-1.36961));
        Poles.SetValue(29,47,gp_Pnt(11.5055,142.304,-0.513898));
        Poles.SetValue(29,48,gp_Pnt(11.5331,150.004,0.00143569));
        Poles.SetValue(30,1,gp_Pnt(16.3445,-150.003,0.00578743));
        Poles.SetValue(30,2,gp_Pnt(16.3249,-142.267,-0.370801));
        Poles.SetValue(30,3,gp_Pnt(16.2799,-126.795,-0.898163));
        Poles.SetValue(30,4,gp_Pnt(16.1574,-103.437,-2.15905));
        Poles.SetValue(30,5,gp_Pnt(16.1621,-84.359,-2.81595));
        Poles.SetValue(30,6,gp_Pnt(15.9355,-70.522,-4.02929));
        Poles.SetValue(30,7,gp_Pnt(15.8353,-62.7113,-4.30109));
        Poles.SetValue(30,8,gp_Pnt(15.8592,-57.1301,-5.29219));
        Poles.SetValue(30,9,gp_Pnt(15.9885,-51.8279,-7.70639));
        Poles.SetValue(30,10,gp_Pnt(16.1458,-47.454,-10.0875));
        Poles.SetValue(30,11,gp_Pnt(16.3056,-43.9394,-11.9786));
        Poles.SetValue(30,12,gp_Pnt(16.4697,-41.3722,-13.5536));
        Poles.SetValue(30,13,gp_Pnt(16.7346,-38.9838,-15.419));
        Poles.SetValue(30,14,gp_Pnt(16.9692,-36.4718,-16.9881));
        Poles.SetValue(30,15,gp_Pnt(17.0978,-34.1539,-17.8546));
        Poles.SetValue(30,16,gp_Pnt(17.1618,-32.2064,-18.1241));
        Poles.SetValue(30,17,gp_Pnt(17.2498,-29.2572,-18.3817));
        Poles.SetValue(30,18,gp_Pnt(17.2843,-25.6335,-18.1023));
        Poles.SetValue(30,19,gp_Pnt(17.349,-21.5427,-18.2505));
        Poles.SetValue(30,20,gp_Pnt(17.4018,-17.4207,-18.275));
        Poles.SetValue(30,21,gp_Pnt(17.4446,-13.282,-18.3905));
        Poles.SetValue(30,22,gp_Pnt(17.4712,-9.12898,-18.4481));
        Poles.SetValue(30,23,gp_Pnt(17.484,-6.01145,-18.4793));
        Poles.SetValue(30,24,gp_Pnt(17.4931,-1.85495,-18.5611));
        Poles.SetValue(30,25,gp_Pnt(17.489,2.29988,-18.6221));
        Poles.SetValue(30,26,gp_Pnt(17.4726,6.44817,-18.6907));
        Poles.SetValue(30,27,gp_Pnt(17.4549,9.55413,-18.7566));
        Poles.SetValue(30,28,gp_Pnt(17.4205,13.6861,-18.7666));
        Poles.SetValue(30,29,gp_Pnt(17.3732,17.7936,-18.8446));
        Poles.SetValue(30,30,gp_Pnt(17.3181,21.8766,-18.906));
        Poles.SetValue(30,31,gp_Pnt(17.2501,25.9253,-18.909));
        Poles.SetValue(30,32,gp_Pnt(17.2175,29.502,-19.3082));
        Poles.SetValue(30,33,gp_Pnt(17.1165,32.4131,-19.0021));
        Poles.SetValue(30,34,gp_Pnt(17.0447,34.3389,-18.7229));
        Poles.SetValue(30,35,gp_Pnt(16.8753,36.5819,-17.6749));
        Poles.SetValue(30,36,gp_Pnt(16.6444,39.1331,-16.2131));
        Poles.SetValue(30,37,gp_Pnt(16.3774,41.5535,-14.3732));
        Poles.SetValue(30,38,gp_Pnt(16.2266,44.1917,-12.9771));
        Poles.SetValue(30,39,gp_Pnt(16.0512,47.7031,-11.1156));
        Poles.SetValue(30,40,gp_Pnt(15.8965,52.1551,-9.02732));
        Poles.SetValue(30,41,gp_Pnt(15.7531,57.497,-6.73597));
        Poles.SetValue(30,42,gp_Pnt(15.7515,63.1508,-5.99587));
        Poles.SetValue(30,43,gp_Pnt(15.8708,70.9704,-5.59492));
        Poles.SetValue(30,44,gp_Pnt(16.0904,84.6832,-4.25753));
        Poles.SetValue(30,45,gp_Pnt(16.1141,103.717,-3.04271));
        Poles.SetValue(30,46,gp_Pnt(16.2429,126.893,-1.24525));
        Poles.SetValue(30,47,gp_Pnt(16.3147,142.31,-0.535292));
        Poles.SetValue(30,48,gp_Pnt(16.341,150.003,0.00253219));
        Poles.SetValue(31,1,gp_Pnt(20.1907,-150.004,-0.000526058));
        Poles.SetValue(31,2,gp_Pnt(20.1704,-142.268,-0.331239));
        Poles.SetValue(31,3,gp_Pnt(20.122,-126.794,-0.970591));
        Poles.SetValue(31,4,gp_Pnt(19.9951,-103.451,-1.9936));
        Poles.SetValue(31,5,gp_Pnt(19.9957,-84.3412,-2.79149));
        Poles.SetValue(31,6,gp_Pnt(19.7663,-70.5532,-3.8958));
        Poles.SetValue(31,7,gp_Pnt(19.6372,-62.6961,-4.24537));
        Poles.SetValue(31,8,gp_Pnt(19.6312,-57.0153,-4.8376));
        Poles.SetValue(31,9,gp_Pnt(19.8029,-51.7257,-7.12882));
        Poles.SetValue(31,10,gp_Pnt(19.9903,-47.3456,-9.33312));
        Poles.SetValue(31,11,gp_Pnt(20.1544,-43.7936,-11.1495));
        Poles.SetValue(31,12,gp_Pnt(20.3067,-41.1475,-12.5808));
        Poles.SetValue(31,13,gp_Pnt(20.5363,-38.6079,-14.2468));
        Poles.SetValue(31,14,gp_Pnt(20.84,-36.134,-15.9275));
        Poles.SetValue(31,15,gp_Pnt(21.0623,-33.9474,-17.1599));
        Poles.SetValue(31,16,gp_Pnt(21.1716,-32.0518,-17.667));
        Poles.SetValue(31,17,gp_Pnt(21.3214,-29.179,-18.3673));
        Poles.SetValue(31,18,gp_Pnt(21.3793,-25.5812,-18.3124));
        Poles.SetValue(31,19,gp_Pnt(21.4548,-21.5049,-18.1853));
        Poles.SetValue(31,20,gp_Pnt(21.5142,-17.3922,-18.2366));
        Poles.SetValue(31,21,gp_Pnt(21.558,-13.2603,-18.3063));
        Poles.SetValue(31,22,gp_Pnt(21.5878,-9.11659,-18.3336));
        Poles.SetValue(31,23,gp_Pnt(21.6056,-6.00831,-18.4512));
        Poles.SetValue(31,24,gp_Pnt(21.6167,-1.86141,-18.4833));
        Poles.SetValue(31,25,gp_Pnt(21.6127,2.2836,-18.5266));
        Poles.SetValue(31,26,gp_Pnt(21.5944,6.42098,-18.6074));
        Poles.SetValue(31,27,gp_Pnt(21.5727,9.51941,-18.6404));
        Poles.SetValue(31,28,gp_Pnt(21.5378,13.6411,-18.7487));
        Poles.SetValue(31,29,gp_Pnt(21.4866,17.743,-18.7215));
        Poles.SetValue(31,30,gp_Pnt(21.4258,21.8165,-18.8984));
        Poles.SetValue(31,31,gp_Pnt(21.3369,25.8371,-19.0188));
        Poles.SetValue(31,32,gp_Pnt(21.2862,29.4092,-19.1007));
        Poles.SetValue(31,33,gp_Pnt(21.1135,32.2368,-18.3775));
        Poles.SetValue(31,34,gp_Pnt(20.9861,34.1014,-17.8358));
        Poles.SetValue(31,35,gp_Pnt(20.7501,36.2734,-16.587));
        Poles.SetValue(31,36,gp_Pnt(20.4474,38.7816,-15.0031));
        Poles.SetValue(31,37,gp_Pnt(20.2219,41.3577,-13.4124));
        Poles.SetValue(31,38,gp_Pnt(20.0752,44.046,-12.1119));
        Poles.SetValue(31,39,gp_Pnt(19.9,47.6135,-10.3667));
        Poles.SetValue(31,40,gp_Pnt(19.7027,52.0328,-8.33553));
        Poles.SetValue(31,41,gp_Pnt(19.5529,57.4314,-6.41226));
        Poles.SetValue(31,42,gp_Pnt(19.5744,63.1466,-5.88305));
        Poles.SetValue(31,43,gp_Pnt(19.7084,70.9809,-5.38238));
        Poles.SetValue(31,44,gp_Pnt(19.9269,84.6646,-4.23097));
        Poles.SetValue(31,45,gp_Pnt(19.9547,103.723,-2.89651));
        Poles.SetValue(31,46,gp_Pnt(20.0869,126.892,-1.26097));
        Poles.SetValue(31,47,gp_Pnt(20.16,142.308,-0.498836));
        Poles.SetValue(31,48,gp_Pnt(20.1872,150.004,-0.000635283));
        Poles.SetValue(32,1,gp_Pnt(23.0753,-150.004,0.00144521));
        Poles.SetValue(32,2,gp_Pnt(23.0548,-142.27,-0.337801));
        Poles.SetValue(32,3,gp_Pnt(23.0016,-126.788,-0.942781));
        Poles.SetValue(32,4,gp_Pnt(22.8773,-103.469,-1.9433));
        Poles.SetValue(32,5,gp_Pnt(22.8673,-84.3255,-2.74248));
        Poles.SetValue(32,6,gp_Pnt(22.6434,-70.5754,-3.79108));
        Poles.SetValue(32,7,gp_Pnt(22.4949,-62.6931,-4.17157));
        Poles.SetValue(32,8,gp_Pnt(22.4768,-56.9735,-4.67076));
        Poles.SetValue(32,9,gp_Pnt(22.6398,-51.6068,-6.54067));
        Poles.SetValue(32,10,gp_Pnt(22.85,-47.2263,-8.64328));
        Poles.SetValue(32,11,gp_Pnt(23.0318,-43.6813,-10.4638));
        Poles.SetValue(32,12,gp_Pnt(23.1562,-40.9615,-11.764));
        Poles.SetValue(32,13,gp_Pnt(23.368,-38.3504,-13.3038));
        Poles.SetValue(32,14,gp_Pnt(23.6652,-35.817,-14.919));
        Poles.SetValue(32,15,gp_Pnt(23.9354,-33.6785,-16.2623));
        Poles.SetValue(32,16,gp_Pnt(24.0927,-31.8451,-17.0162));
        Poles.SetValue(32,17,gp_Pnt(24.3261,-29.0715,-18.0603));
        Poles.SetValue(32,18,gp_Pnt(24.4211,-25.5163,-18.3534));
        Poles.SetValue(32,19,gp_Pnt(24.5253,-21.4717,-18.3276));
        Poles.SetValue(32,20,gp_Pnt(24.5868,-17.3628,-18.1811));
        Poles.SetValue(32,21,gp_Pnt(24.6337,-13.2388,-18.2382));
        Poles.SetValue(32,22,gp_Pnt(24.6633,-9.10443,-18.3301));
        Poles.SetValue(32,23,gp_Pnt(24.6763,-6.00141,-18.3329));
        Poles.SetValue(32,24,gp_Pnt(24.6883,-1.86447,-18.4));
        Poles.SetValue(32,25,gp_Pnt(24.685,2.27094,-18.4513));
        Poles.SetValue(32,26,gp_Pnt(24.6679,6.39927,-18.5271));
        Poles.SetValue(32,27,gp_Pnt(24.6503,9.49024,-18.6028));
        Poles.SetValue(32,28,gp_Pnt(24.6143,13.6057,-18.6066));
        Poles.SetValue(32,29,gp_Pnt(24.5631,17.7003,-18.6756));
        Poles.SetValue(32,30,gp_Pnt(24.4992,21.766,-18.9712));
        Poles.SetValue(32,31,gp_Pnt(24.3992,25.7809,-18.8799));
        Poles.SetValue(32,32,gp_Pnt(24.3004,29.3057,-18.7554));
        Poles.SetValue(32,33,gp_Pnt(24.037,32.0419,-17.6838));
        Poles.SetValue(32,34,gp_Pnt(23.8657,33.8604,-16.9256));
        Poles.SetValue(32,35,gp_Pnt(23.5774,35.9803,-15.5713));
        Poles.SetValue(32,36,gp_Pnt(23.2968,38.5683,-14.0879));
        Poles.SetValue(32,37,gp_Pnt(23.093,41.2165,-12.6479));
        Poles.SetValue(32,38,gp_Pnt(22.9531,43.9435,-11.4158));
        Poles.SetValue(32,39,gp_Pnt(22.7622,47.5105,-9.68785));
        Poles.SetValue(32,40,gp_Pnt(22.5568,51.9527,-7.8039));
        Poles.SetValue(32,41,gp_Pnt(22.4004,57.3744,-6.1033));
        Poles.SetValue(32,42,gp_Pnt(22.4489,63.1511,-5.80662));
        Poles.SetValue(32,43,gp_Pnt(22.5902,70.9891,-5.2305));
        Poles.SetValue(32,44,gp_Pnt(22.799,84.6457,-4.15543));
        Poles.SetValue(32,45,gp_Pnt(22.8398,103.734,-2.83232));
        Poles.SetValue(32,46,gp_Pnt(22.9682,126.889,-1.24771));
        Poles.SetValue(32,47,gp_Pnt(23.0442,142.308,-0.479822));
        Poles.SetValue(32,48,gp_Pnt(23.072,150.003,-0.00023355));
        Poles.SetValue(33,1,gp_Pnt(25.96,-150.004,-0.00107395));
        Poles.SetValue(33,2,gp_Pnt(25.9383,-142.271,-0.323802));
        Poles.SetValue(33,3,gp_Pnt(25.883,-126.787,-0.940847));
        Poles.SetValue(33,4,gp_Pnt(25.7581,-103.481,-1.84472));
        Poles.SetValue(33,5,gp_Pnt(25.7408,-84.3172,-2.76066));
        Poles.SetValue(33,6,gp_Pnt(25.52,-70.5894,-3.62915));
        Poles.SetValue(33,7,gp_Pnt(25.366,-62.7115,-4.17081));
        Poles.SetValue(33,8,gp_Pnt(25.3057,-56.9037,-4.30025));
        Poles.SetValue(33,9,gp_Pnt(25.4874,-51.5283,-6.03263));
        Poles.SetValue(33,10,gp_Pnt(25.6972,-47.1048,-7.91375));
        Poles.SetValue(33,11,gp_Pnt(25.8911,-43.5608,-9.71674));
        Poles.SetValue(33,12,gp_Pnt(26.0083,-40.8113,-10.9474));
        Poles.SetValue(33,13,gp_Pnt(26.1812,-38.1233,-12.3183));
        Poles.SetValue(33,14,gp_Pnt(26.4261,-35.4747,-13.7471));
        Poles.SetValue(33,15,gp_Pnt(26.7228,-33.3637,-15.1404));
        Poles.SetValue(33,16,gp_Pnt(26.9425,-31.5869,-16.0448));
        Poles.SetValue(33,17,gp_Pnt(27.2466,-28.8898,-17.3418));
        Poles.SetValue(33,18,gp_Pnt(27.4392,-25.4377,-18.079));
        Poles.SetValue(33,19,gp_Pnt(27.5703,-21.4197,-18.3425));
        Poles.SetValue(33,20,gp_Pnt(27.6524,-17.3331,-18.2754));
        Poles.SetValue(33,21,gp_Pnt(27.691,-13.2136,-18.2387));
        Poles.SetValue(33,22,gp_Pnt(27.7352,-9.09145,-18.3076));
        Poles.SetValue(33,23,gp_Pnt(27.7708,-5.99784,-18.3543));
        Poles.SetValue(33,24,gp_Pnt(27.7901,-1.8693,-18.4133));
        Poles.SetValue(33,25,gp_Pnt(27.7867,2.25931,-18.461));
        Poles.SetValue(33,26,gp_Pnt(27.7597,6.37821,-18.527));
        Poles.SetValue(33,27,gp_Pnt(27.7183,9.46104,-18.5728));
        Poles.SetValue(33,28,gp_Pnt(27.6728,13.5629,-18.5809));
        Poles.SetValue(33,29,gp_Pnt(27.6341,17.6553,-18.7896));
        Poles.SetValue(33,30,gp_Pnt(27.5313,21.69,-18.856));
        Poles.SetValue(33,31,gp_Pnt(27.4221,25.7022,-18.6912));
        Poles.SetValue(33,32,gp_Pnt(27.1858,29.0914,-17.9663));
        Poles.SetValue(33,33,gp_Pnt(26.8754,31.7892,-16.6502));
        Poles.SetValue(33,34,gp_Pnt(26.6629,33.5795,-15.8008));
        Poles.SetValue(33,35,gp_Pnt(26.392,35.7345,-14.5141));
        Poles.SetValue(33,36,gp_Pnt(26.1302,38.3859,-13.1359));
        Poles.SetValue(33,37,gp_Pnt(25.9577,41.0989,-11.8672));
        Poles.SetValue(33,38,gp_Pnt(25.8174,43.84,-10.6733));
        Poles.SetValue(33,39,gp_Pnt(25.6131,47.4059,-8.97942));
        Poles.SetValue(33,40,gp_Pnt(25.4162,51.8971,-7.31061));
        Poles.SetValue(33,41,gp_Pnt(25.2618,57.3518,-5.8554));
        Poles.SetValue(33,42,gp_Pnt(25.3309,63.1621,-5.70901));
        Poles.SetValue(33,43,gp_Pnt(25.4737,70.9955,-5.07709));
        Poles.SetValue(33,44,gp_Pnt(25.6714,84.6291,-4.0924));
        Poles.SetValue(33,45,gp_Pnt(25.7247,103.742,-2.75106));
        Poles.SetValue(33,46,gp_Pnt(25.8505,126.888,-1.25015));
        Poles.SetValue(33,47,gp_Pnt(25.9279,142.307,-0.4543));
        Poles.SetValue(33,48,gp_Pnt(25.9567,150.003,1.78031e-006));
        Poles.SetValue(34,1,gp_Pnt(28.8443,-150.003,-0.000814414));
        Poles.SetValue(34,2,gp_Pnt(28.822,-142.272,-0.314965));
        Poles.SetValue(34,3,gp_Pnt(28.7639,-126.784,-0.915393));
        Poles.SetValue(34,4,gp_Pnt(28.6408,-103.496,-1.77607));
        Poles.SetValue(34,5,gp_Pnt(28.6108,-84.3033,-2.7051));
        Poles.SetValue(34,6,gp_Pnt(28.4002,-70.6057,-3.51828));
        Poles.SetValue(34,7,gp_Pnt(28.2459,-62.7375,-4.07858));
        Poles.SetValue(34,8,gp_Pnt(28.1744,-56.9104,-4.20906));
        Poles.SetValue(34,9,gp_Pnt(28.3177,-51.4347,-5.41731));
        Poles.SetValue(34,10,gp_Pnt(28.5405,-47,-7.20566));
        Poles.SetValue(34,11,gp_Pnt(28.7156,-43.4043,-8.86385));
        Poles.SetValue(34,12,gp_Pnt(28.8516,-40.6751,-10.1127));
        Poles.SetValue(34,13,gp_Pnt(28.9811,-37.9111,-11.2996));
        Poles.SetValue(34,14,gp_Pnt(29.1942,-35.2049,-12.5938));
        Poles.SetValue(34,15,gp_Pnt(29.4218,-32.9982,-13.7949));
        Poles.SetValue(34,16,gp_Pnt(29.6546,-31.2373,-14.7599));
        Poles.SetValue(34,17,gp_Pnt(30.0209,-28.5986,-16.2161));
        Poles.SetValue(34,18,gp_Pnt(30.3511,-25.2896,-17.4304));
        Poles.SetValue(34,19,gp_Pnt(30.5644,-21.3363,-18.1086));
        Poles.SetValue(34,20,gp_Pnt(30.6971,-17.2907,-18.3429));
        Poles.SetValue(34,21,gp_Pnt(30.7624,-13.191,-18.3042));
        Poles.SetValue(34,22,gp_Pnt(30.7888,-9.07614,-18.2619));
        Poles.SetValue(34,23,gp_Pnt(30.7984,-5.98767,-18.2612));
        Poles.SetValue(34,24,gp_Pnt(30.8028,-1.86922,-18.3218));
        Poles.SetValue(34,25,gp_Pnt(30.8038,2.24578,-18.378));
        Poles.SetValue(34,26,gp_Pnt(30.7952,6.35643,-18.4258));
        Poles.SetValue(34,27,gp_Pnt(30.783,9.43271,-18.5099));
        Poles.SetValue(34,28,gp_Pnt(30.7508,13.5293,-18.5862));
        Poles.SetValue(34,29,gp_Pnt(30.6756,17.5933,-18.7948));
        Poles.SetValue(34,30,gp_Pnt(30.5377,21.6111,-18.5821));
        Poles.SetValue(34,31,gp_Pnt(30.3253,25.5499,-17.9497));
        Poles.SetValue(34,32,gp_Pnt(29.9809,28.8453,-16.7757));
        Poles.SetValue(34,33,gp_Pnt(29.6116,31.4959,-15.4039));
        Poles.SetValue(34,34,gp_Pnt(29.3795,33.2673,-14.4675));
        Poles.SetValue(34,35,gp_Pnt(29.1738,35.5112,-13.3894));
        Poles.SetValue(34,36,gp_Pnt(28.9461,38.2178,-12.1512));
        Poles.SetValue(34,37,gp_Pnt(28.8226,41.0067,-11.0898));
        Poles.SetValue(34,38,gp_Pnt(28.6485,43.7085,-9.8326));
        Poles.SetValue(34,39,gp_Pnt(28.4613,47.3204,-8.29397));
        Poles.SetValue(34,40,gp_Pnt(28.2597,51.8236,-6.71601));
        Poles.SetValue(34,41,gp_Pnt(28.1363,57.35,-5.69854));
        Poles.SetValue(34,42,gp_Pnt(28.2228,63.1847,-5.58611));
        Poles.SetValue(34,43,gp_Pnt(28.3583,70.9987,-4.92408));
        Poles.SetValue(34,44,gp_Pnt(28.5449,84.6147,-4.02298));
        Poles.SetValue(34,45,gp_Pnt(28.6102,103.75,-2.67549));
        Poles.SetValue(34,46,gp_Pnt(28.733,126.887,-1.24976));
        Poles.SetValue(34,47,gp_Pnt(28.8119,142.306,-0.431209));
        Poles.SetValue(34,48,gp_Pnt(28.8411,150.003,0.000394087));
        Poles.SetValue(35,1,gp_Pnt(31.7287,-150.003,-0.00159563));
        Poles.SetValue(35,2,gp_Pnt(31.7054,-142.271,-0.292958));
        Poles.SetValue(35,3,gp_Pnt(31.6455,-126.784,-0.897779));
        Poles.SetValue(35,4,gp_Pnt(31.5244,-103.509,-1.70454));
        Poles.SetValue(35,5,gp_Pnt(31.4818,-84.2933,-2.66393));
        Poles.SetValue(35,6,gp_Pnt(31.2818,-70.6177,-3.40872));
        Poles.SetValue(35,7,gp_Pnt(31.1379,-62.7777,-3.96987));
        Poles.SetValue(35,8,gp_Pnt(31.0487,-56.922,-4.10969));
        Poles.SetValue(35,9,gp_Pnt(31.1572,-51.3677,-4.89488));
        Poles.SetValue(35,10,gp_Pnt(31.3712,-46.8953,-6.51595));
        Poles.SetValue(35,11,gp_Pnt(31.5139,-43.2378,-7.96936));
        Poles.SetValue(35,12,gp_Pnt(31.6758,-40.5316,-9.22108));
        Poles.SetValue(35,13,gp_Pnt(31.8199,-37.7807,-10.3811));
        Poles.SetValue(35,14,gp_Pnt(31.9893,-35.0147,-11.5145));
        Poles.SetValue(35,15,gp_Pnt(32.1247,-32.7053,-12.464));
        Poles.SetValue(35,16,gp_Pnt(32.3218,-30.8986,-13.34));
        Poles.SetValue(35,17,gp_Pnt(32.6432,-28.2165,-14.6979));
        Poles.SetValue(35,18,gp_Pnt(33.097,-25.0373,-16.1855));
        Poles.SetValue(35,19,gp_Pnt(33.4343,-21.1834,-17.345));
        Poles.SetValue(35,20,gp_Pnt(33.6567,-17.2197,-18.0405));
        Poles.SetValue(35,21,gp_Pnt(33.7429,-13.1456,-18.2543));
        Poles.SetValue(35,22,gp_Pnt(33.7947,-9.05496,-18.3306));
        Poles.SetValue(35,23,gp_Pnt(33.814,-5.97761,-18.3713));
        Poles.SetValue(35,24,gp_Pnt(33.8326,-1.87322,-18.3889));
        Poles.SetValue(35,25,gp_Pnt(33.8307,2.23248,-18.5068));
        Poles.SetValue(35,26,gp_Pnt(33.8103,6.3285,-18.5284));
        Poles.SetValue(35,27,gp_Pnt(33.7859,9.39575,-18.5442));
        Poles.SetValue(35,28,gp_Pnt(33.7311,13.4669,-18.5787));
        Poles.SetValue(35,29,gp_Pnt(33.6309,17.5149,-18.4035));
        Poles.SetValue(35,30,gp_Pnt(33.3631,21.4402,-17.6914));
        Poles.SetValue(35,31,gp_Pnt(33.0314,25.2897,-16.6389));
        Poles.SetValue(35,32,gp_Pnt(32.5814,28.4766,-15.2398));
        Poles.SetValue(35,33,gp_Pnt(32.2879,31.1937,-13.9832));
        Poles.SetValue(35,34,gp_Pnt(32.101,33.016,-13.1896));
        Poles.SetValue(35,35,gp_Pnt(31.9711,35.3393,-12.3157));
        Poles.SetValue(35,36,gp_Pnt(31.762,38.0751,-11.166));
        Poles.SetValue(35,37,gp_Pnt(31.6205,40.8468,-10.1215));
        Poles.SetValue(35,38,gp_Pnt(31.4569,43.5672,-8.96315));
        Poles.SetValue(35,39,gp_Pnt(31.304,47.2348,-7.63002));
        Poles.SetValue(35,40,gp_Pnt(31.1056,51.7662,-6.18385));
        Poles.SetValue(35,41,gp_Pnt(31.032,57.3778,-5.59198));
        Poles.SetValue(35,42,gp_Pnt(31.1183,63.208,-5.41945));
        Poles.SetValue(35,43,gp_Pnt(31.2462,71.0036,-4.78942));
        Poles.SetValue(35,44,gp_Pnt(31.4185,84.5987,-3.92699));
        Poles.SetValue(35,45,gp_Pnt(31.4967,103.758,-2.61174));
        Poles.SetValue(35,46,gp_Pnt(31.6157,126.886,-1.24044));
        Poles.SetValue(35,47,gp_Pnt(31.6958,142.305,-0.417113));
        Poles.SetValue(35,48,gp_Pnt(31.7255,150.002,0.00194988));
        Poles.SetValue(36,1,gp_Pnt(34.613,-150.003,-8.84142e-006));
        Poles.SetValue(36,2,gp_Pnt(34.5892,-142.271,-0.281898));
        Poles.SetValue(36,3,gp_Pnt(34.5259,-126.782,-0.842126));
        Poles.SetValue(36,4,gp_Pnt(34.4114,-103.526,-1.69502));
        Poles.SetValue(36,5,gp_Pnt(34.3501,-84.279,-2.54815));
        Poles.SetValue(36,6,gp_Pnt(34.1678,-70.6326,-3.32282));
        Poles.SetValue(36,7,gp_Pnt(34.0367,-62.8161,-3.82536));
        Poles.SetValue(36,8,gp_Pnt(33.9563,-56.9857,-4.13682));
        Poles.SetValue(36,9,gp_Pnt(34.0087,-51.3335,-4.46806));
        Poles.SetValue(36,10,gp_Pnt(34.1924,-46.7917,-5.85857));
        Poles.SetValue(36,11,gp_Pnt(34.3026,-43.0822,-7.10693));
        Poles.SetValue(36,12,gp_Pnt(34.4437,-40.3441,-8.2133));
        Poles.SetValue(36,13,gp_Pnt(34.6249,-37.6272,-9.39894));
        Poles.SetValue(36,14,gp_Pnt(34.7778,-34.844,-10.4473));
        Poles.SetValue(36,15,gp_Pnt(34.8915,-32.5077,-11.3048));
        Poles.SetValue(36,16,gp_Pnt(35.0303,-30.6596,-12.0421));
        Poles.SetValue(36,17,gp_Pnt(35.2632,-27.9023,-13.1946));
        Poles.SetValue(36,18,gp_Pnt(35.6871,-24.7181,-14.6532));
        Poles.SetValue(36,19,gp_Pnt(36.0775,-20.9311,-15.9594));
        Poles.SetValue(36,20,gp_Pnt(36.3883,-17.0442,-16.9645));
        Poles.SetValue(36,21,gp_Pnt(36.6346,-13.0717,-17.6564));
        Poles.SetValue(36,22,gp_Pnt(36.703,-9.00817,-17.8864));
        Poles.SetValue(36,23,gp_Pnt(36.753,-5.9561,-18.0651));
        Poles.SetValue(36,24,gp_Pnt(36.7617,-1.86876,-18.1671));
        Poles.SetValue(36,25,gp_Pnt(36.7651,2.21985,-18.2173));
        Poles.SetValue(36,26,gp_Pnt(36.7472,6.29944,-18.1998));
        Poles.SetValue(36,27,gp_Pnt(36.7037,9.34658,-18.1186));
        Poles.SetValue(36,28,gp_Pnt(36.6083,13.3906,-17.8978));
        Poles.SetValue(36,29,gp_Pnt(36.3678,17.3552,-17.2999));
        Poles.SetValue(36,30,gp_Pnt(36.0217,21.2169,-16.3136));
        Poles.SetValue(36,31,gp_Pnt(35.6137,24.997,-15.0642));
        Poles.SetValue(36,32,gp_Pnt(35.226,28.2063,-13.7406));
        Poles.SetValue(36,33,gp_Pnt(34.9984,30.9793,-12.6852));
        Poles.SetValue(36,34,gp_Pnt(34.8482,32.8226,-11.9616));
        Poles.SetValue(36,35,gp_Pnt(34.7496,35.1802,-11.2179));
        Poles.SetValue(36,36,gp_Pnt(34.5774,37.9518,-10.2031));
        Poles.SetValue(36,37,gp_Pnt(34.4083,40.6991,-9.15684));
        Poles.SetValue(36,38,gp_Pnt(34.2536,43.4345,-8.10986));
        Poles.SetValue(36,39,gp_Pnt(34.1265,47.141,-6.95245));
        Poles.SetValue(36,40,gp_Pnt(33.9694,51.7379,-5.79187));
        Poles.SetValue(36,41,gp_Pnt(33.9407,57.4177,-5.51793));
        Poles.SetValue(36,42,gp_Pnt(34.0186,63.2307,-5.20171));
        Poles.SetValue(36,43,gp_Pnt(34.1362,71.0089,-4.67244));
        Poles.SetValue(36,44,gp_Pnt(34.2957,84.5851,-3.80167));
        Poles.SetValue(36,45,gp_Pnt(34.383,103.766,-2.5642));
        Poles.SetValue(36,46,gp_Pnt(34.4993,126.886,-1.22044));
        Poles.SetValue(36,47,gp_Pnt(34.5794,142.304,-0.409319));
        Poles.SetValue(36,48,gp_Pnt(34.61,150.002,0.00244459));
        Poles.SetValue(37,1,gp_Pnt(37.4972,-150.002,0.000555526));
        Poles.SetValue(37,2,gp_Pnt(37.4728,-142.27,-0.265447));
        Poles.SetValue(37,3,gp_Pnt(37.4079,-126.783,-0.802161));
        Poles.SetValue(37,4,gp_Pnt(37.2967,-103.539,-1.66555));
        Poles.SetValue(37,5,gp_Pnt(37.2229,-84.2708,-2.46547));
        Poles.SetValue(37,6,gp_Pnt(37.0545,-70.6462,-3.22563));
        Poles.SetValue(37,7,gp_Pnt(36.9382,-62.8514,-3.69803));
        Poles.SetValue(37,8,gp_Pnt(36.8695,-57.0355,-4.05169));
        Poles.SetValue(37,9,gp_Pnt(36.8664,-51.3068,-4.20071));
        Poles.SetValue(37,10,gp_Pnt(36.9963,-46.6801,-5.21492));
        Poles.SetValue(37,11,gp_Pnt(37.1028,-42.9622,-6.32347));
        Poles.SetValue(37,12,gp_Pnt(37.2217,-40.1924,-7.25707));
        Poles.SetValue(37,13,gp_Pnt(37.3806,-37.45,-8.32532));
        Poles.SetValue(37,14,gp_Pnt(37.5666,-34.6987,-9.40192));
        Poles.SetValue(37,15,gp_Pnt(37.6875,-32.3677,-10.2225));
        Poles.SetValue(37,16,gp_Pnt(37.7796,-30.4815,-10.8337));
        Poles.SetValue(37,17,gp_Pnt(37.8949,-27.6361,-11.7039));
        Poles.SetValue(37,18,gp_Pnt(38.1939,-24.3881,-12.9412));
        Poles.SetValue(37,19,gp_Pnt(38.5477,-20.6236,-14.2289));
        Poles.SetValue(37,20,gp_Pnt(38.9367,-16.8248,-15.4489));
        Poles.SetValue(37,21,gp_Pnt(39.2113,-12.9073,-16.3298));
        Poles.SetValue(37,22,gp_Pnt(39.4234,-8.92763,-16.959));
        Poles.SetValue(37,23,gp_Pnt(39.4867,-5.90171,-17.2202));
        Poles.SetValue(37,24,gp_Pnt(39.5456,-1.85585,-17.4489));
        Poles.SetValue(37,25,gp_Pnt(39.5315,2.20624,-17.47));
        Poles.SetValue(37,26,gp_Pnt(39.4789,6.249,-17.3411));
        Poles.SetValue(37,27,gp_Pnt(39.3801,9.26259,-17.0826));
        Poles.SetValue(37,28,gp_Pnt(39.1782,13.2421,-16.5416));
        Poles.SetValue(37,29,gp_Pnt(38.9025,17.148,-15.7336));
        Poles.SetValue(37,30,gp_Pnt(38.4895,20.9378,-14.5637));
        Poles.SetValue(37,31,gp_Pnt(38.1479,24.7114,-13.3818));
        Poles.SetValue(37,32,gp_Pnt(37.8908,27.991,-12.3161));
        Poles.SetValue(37,33,gp_Pnt(37.7444,30.8157,-11.4556));
        Poles.SetValue(37,34,gp_Pnt(37.6502,32.7032,-10.8985));
        Poles.SetValue(37,35,gp_Pnt(37.5088,35.0224,-10.1));
        Poles.SetValue(37,36,gp_Pnt(37.3138,37.7726,-9.07484));
        Poles.SetValue(37,37,gp_Pnt(37.1382,40.5101,-8.05559));
        Poles.SetValue(37,38,gp_Pnt(37.054,43.3215,-7.31185));
        Poles.SetValue(37,39,gp_Pnt(36.9421,47.0468,-6.33184));
        Poles.SetValue(37,40,gp_Pnt(36.8379,51.7166,-5.49796));
        Poles.SetValue(37,41,gp_Pnt(36.8607,57.4567,-5.443));
        Poles.SetValue(37,42,gp_Pnt(36.9222,63.2529,-5.029));
        Poles.SetValue(37,43,gp_Pnt(37.0259,71.0129,-4.53609));
        Poles.SetValue(37,44,gp_Pnt(37.1745,84.5734,-3.68955));
        Poles.SetValue(37,45,gp_Pnt(37.2692,103.772,-2.50735));
        Poles.SetValue(37,46,gp_Pnt(37.3831,126.887,-1.20321));
        Poles.SetValue(37,47,gp_Pnt(37.4633,142.303,-0.401612));
        Poles.SetValue(37,48,gp_Pnt(37.4942,150.002,0.00165959));
        Poles.SetValue(38,1,gp_Pnt(41.3427,-150.002,0.000910129));
        Poles.SetValue(38,2,gp_Pnt(41.3172,-142.269,-0.250994));
        Poles.SetValue(38,3,gp_Pnt(41.252,-126.785,-0.743205));
        Poles.SetValue(38,4,gp_Pnt(41.1432,-103.556,-1.64006));
        Poles.SetValue(38,5,gp_Pnt(41.0572,-84.2638,-2.33977));
        Poles.SetValue(38,6,gp_Pnt(40.9025,-70.662,-3.09061));
        Poles.SetValue(38,7,gp_Pnt(40.8094,-62.8944,-3.52967));
        Poles.SetValue(38,8,gp_Pnt(40.7531,-57.091,-3.91119));
        Poles.SetValue(38,9,gp_Pnt(40.7096,-51.3094,-4.15327));
        Poles.SetValue(38,10,gp_Pnt(40.7225,-46.5336,-4.39732));
        Poles.SetValue(38,11,gp_Pnt(40.8349,-42.8149,-5.35507));
        Poles.SetValue(38,12,gp_Pnt(40.9198,-40.01,-6.07186));
        Poles.SetValue(38,13,gp_Pnt(41.039,-37.2275,-6.89946));
        Poles.SetValue(38,14,gp_Pnt(41.1807,-34.446,-7.76251));
        Poles.SetValue(38,15,gp_Pnt(41.3421,-32.1529,-8.6033));
        Poles.SetValue(38,16,gp_Pnt(41.4528,-30.2866,-9.22754));
        Poles.SetValue(38,17,gp_Pnt(41.6048,-27.4727,-10.1338));
        Poles.SetValue(38,18,gp_Pnt(41.7346,-24.1213,-11.015));
        Poles.SetValue(38,19,gp_Pnt(41.93,-20.3071,-12.0361));
        Poles.SetValue(38,20,gp_Pnt(42.1256,-16.4709,-12.9787));
        Poles.SetValue(38,21,gp_Pnt(42.3664,-12.6243,-13.8833));
        Poles.SetValue(38,22,gp_Pnt(42.5253,-8.71352,-14.5112));
        Poles.SetValue(38,23,gp_Pnt(42.6203,-5.76323,-14.8691));
        Poles.SetValue(38,24,gp_Pnt(42.6798,-1.79912,-15.1424));
        Poles.SetValue(38,25,gp_Pnt(42.6737,2.17503,-15.1747));
        Poles.SetValue(38,26,gp_Pnt(42.5953,6.13666,-14.9438));
        Poles.SetValue(38,27,gp_Pnt(42.5039,9.08523,-14.6485));
        Poles.SetValue(38,28,gp_Pnt(42.3376,12.9911,-14.0849));
        Poles.SetValue(38,29,gp_Pnt(42.1163,16.8397,-13.292));
        Poles.SetValue(38,30,gp_Pnt(41.8966,20.6667,-12.3969));
        Poles.SetValue(38,31,gp_Pnt(41.7151,24.4907,-11.5063));
        Poles.SetValue(38,32,gp_Pnt(41.5833,27.8377,-10.6892));
        Poles.SetValue(38,33,gp_Pnt(41.412,30.6412,-9.8422));
        Poles.SetValue(38,34,gp_Pnt(41.2857,32.4975,-9.24392));
        Poles.SetValue(38,35,gp_Pnt(41.1281,34.7988,-8.48532));
        Poles.SetValue(38,36,gp_Pnt(40.9982,37.5923,-7.7491));
        Poles.SetValue(38,37,gp_Pnt(40.9022,40.4018,-7.05478));
        Poles.SetValue(38,38,gp_Pnt(40.8045,43.2027,-6.38438));
        Poles.SetValue(38,39,gp_Pnt(40.6947,46.9351,-5.51775));
        Poles.SetValue(38,40,gp_Pnt(40.6936,51.7133,-5.40542));
        Poles.SetValue(38,41,gp_Pnt(40.7465,57.4932,-5.2213));
        Poles.SetValue(38,42,gp_Pnt(40.7949,63.2798,-4.80832));
        Poles.SetValue(38,43,gp_Pnt(40.8787,71.0181,-4.35466));
        Poles.SetValue(38,44,gp_Pnt(41.0184,84.563,-3.55086));
        Poles.SetValue(38,45,gp_Pnt(41.1158,103.778,-2.42645));
        Poles.SetValue(38,46,gp_Pnt(41.2295,126.889,-1.17679));
        Poles.SetValue(38,47,gp_Pnt(41.308,142.302,-0.393553));
        Poles.SetValue(38,48,gp_Pnt(41.3397,150.001,-0.000267641));
        Poles.SetValue(39,1,gp_Pnt(46.1496,-150.001,0.00111063));
        Poles.SetValue(39,2,gp_Pnt(46.1222,-142.268,-0.24987));
        Poles.SetValue(39,3,gp_Pnt(46.0591,-126.79,-0.697635));
        Poles.SetValue(39,4,gp_Pnt(45.9503,-103.571,-1.56599));
        Poles.SetValue(39,5,gp_Pnt(45.856,-84.2638,-2.23446));
        Poles.SetValue(39,6,gp_Pnt(45.7158,-70.683,-2.88403));
        Poles.SetValue(39,7,gp_Pnt(45.6442,-62.9379,-3.34584));
        Poles.SetValue(39,8,gp_Pnt(45.5919,-57.1341,-3.69072));
        Poles.SetValue(39,9,gp_Pnt(45.5333,-51.33,-4.08565));
        Poles.SetValue(39,10,gp_Pnt(45.4961,-46.5017,-4.13549));
        Poles.SetValue(39,11,gp_Pnt(45.5149,-42.6839,-4.33983));
        Poles.SetValue(39,12,gp_Pnt(45.5827,-39.8663,-4.80837));
        Poles.SetValue(39,13,gp_Pnt(45.6781,-37.0642,-5.3692));
        Poles.SetValue(39,14,gp_Pnt(45.8102,-34.2784,-6.16223));
        Poles.SetValue(39,15,gp_Pnt(45.8709,-31.9134,-6.65192));
        Poles.SetValue(39,16,gp_Pnt(45.9562,-30.039,-7.1593));
        Poles.SetValue(39,17,gp_Pnt(46.0717,-27.2191,-7.89044));
        Poles.SetValue(39,18,gp_Pnt(46.2519,-23.9199,-8.84437));
        Poles.SetValue(39,19,gp_Pnt(46.4143,-20.1034,-9.76693));
        Poles.SetValue(39,20,gp_Pnt(46.5429,-16.2588,-10.5643));
        Poles.SetValue(39,21,gp_Pnt(46.6217,-12.3944,-11.1733));
        Poles.SetValue(39,22,gp_Pnt(46.681,-8.52657,-11.622));
        Poles.SetValue(39,23,gp_Pnt(46.7153,-5.61927,-11.8682));
        Poles.SetValue(39,24,gp_Pnt(46.7497,-1.73975,-12.0911));
        Poles.SetValue(39,25,gp_Pnt(46.7278,2.15162,-12.0955));
        Poles.SetValue(39,26,gp_Pnt(46.7066,6.03283,-11.9712));
        Poles.SetValue(39,27,gp_Pnt(46.6796,8.93942,-11.7865));
        Poles.SetValue(39,28,gp_Pnt(46.6257,12.8059,-11.425));
        Poles.SetValue(39,29,gp_Pnt(46.5243,16.6605,-10.8643));
        Poles.SetValue(39,30,gp_Pnt(46.3615,20.4856,-10.0971));
        Poles.SetValue(39,31,gp_Pnt(46.2089,24.304,-9.28958));
        Poles.SetValue(39,32,gp_Pnt(46.0224,27.5938,-8.42989));
        Poles.SetValue(39,33,gp_Pnt(45.9077,30.415,-7.785));
        Poles.SetValue(39,34,gp_Pnt(45.8414,32.3032,-7.38582));
        Poles.SetValue(39,35,gp_Pnt(45.7555,34.647,-6.87832));
        Poles.SetValue(39,36,gp_Pnt(45.6756,37.4791,-6.33131));
        Poles.SetValue(39,37,gp_Pnt(45.5588,40.2637,-5.71115));
        Poles.SetValue(39,38,gp_Pnt(45.4983,43.0898,-5.31195));
        Poles.SetValue(39,39,gp_Pnt(45.4876,46.9076,-5.26074));
        Poles.SetValue(39,40,gp_Pnt(45.5315,51.7279,-5.25925));
        Poles.SetValue(39,41,gp_Pnt(45.5802,57.5155,-4.81316));
        Poles.SetValue(39,42,gp_Pnt(45.6332,63.3069,-4.55574));
        Poles.SetValue(39,43,gp_Pnt(45.6962,71.0259,-4.09811));
        Poles.SetValue(39,44,gp_Pnt(45.8254,84.5537,-3.37827));
        Poles.SetValue(39,45,gp_Pnt(45.9243,103.783,-2.3236));
        Poles.SetValue(39,46,gp_Pnt(46.0379,126.89,-1.12458));
        Poles.SetValue(39,47,gp_Pnt(46.1137,142.301,-0.395727));
        Poles.SetValue(39,48,gp_Pnt(46.1465,150.001,0.000996613));
        Poles.SetValue(40,1,gp_Pnt(51.9175,-150,-0.000207971));
        Poles.SetValue(40,2,gp_Pnt(51.8891,-142.268,-0.241315));
        Poles.SetValue(40,3,gp_Pnt(51.8282,-126.796,-0.673804));
        Poles.SetValue(40,4,gp_Pnt(51.7199,-103.588,-1.45857));
        Poles.SetValue(40,5,gp_Pnt(51.6194,-84.2705,-2.10744));
        Poles.SetValue(40,6,gp_Pnt(51.5011,-70.7153,-2.68959));
        Poles.SetValue(40,7,gp_Pnt(51.438,-62.979,-3.06884));
        Poles.SetValue(40,8,gp_Pnt(51.3917,-57.1815,-3.4528));
        Poles.SetValue(40,9,gp_Pnt(51.333,-51.3737,-3.73088));
        Poles.SetValue(40,10,gp_Pnt(51.2926,-46.5422,-4.00316));
        Poles.SetValue(40,11,gp_Pnt(51.2652,-42.6804,-4.15327));
        Poles.SetValue(40,12,gp_Pnt(51.2481,-39.786,-4.14827));
        Poles.SetValue(40,13,gp_Pnt(51.2355,-36.9009,-4.17909));
        Poles.SetValue(40,14,gp_Pnt(51.269,-34.0518,-4.48805));
        Poles.SetValue(40,15,gp_Pnt(51.2999,-31.6825,-4.8317));
        Poles.SetValue(40,16,gp_Pnt(51.3423,-29.7917,-5.18954));
        Poles.SetValue(40,17,gp_Pnt(51.4157,-26.9599,-5.75758));
        Poles.SetValue(40,18,gp_Pnt(51.4915,-23.638,-6.34409));
        Poles.SetValue(40,19,gp_Pnt(51.6134,-19.8436,-7.01661));
        Poles.SetValue(40,20,gp_Pnt(51.7563,-16.0473,-7.67254));
        Poles.SetValue(40,21,gp_Pnt(51.9147,-12.2466,-8.28467));
        Poles.SetValue(40,22,gp_Pnt(52.0172,-8.41953,-8.69997));
        Poles.SetValue(40,23,gp_Pnt(52.0633,-5.54232,-8.92052));
        Poles.SetValue(40,24,gp_Pnt(52.1076,-1.69853,-9.12971));
        Poles.SetValue(40,25,gp_Pnt(52.1149,2.14658,-9.19114));
        Poles.SetValue(40,26,gp_Pnt(52.0573,5.98803,-9.04481));
        Poles.SetValue(40,27,gp_Pnt(52.0106,8.86368,-8.89058));
        Poles.SetValue(40,28,gp_Pnt(51.8867,12.6814,-8.51757));
        Poles.SetValue(40,29,gp_Pnt(51.7526,16.4859,-8.05187));
        Poles.SetValue(40,30,gp_Pnt(51.6058,20.2741,-7.46264));
        Poles.SetValue(40,31,gp_Pnt(51.4828,24.0629,-6.88014));
        Poles.SetValue(40,32,gp_Pnt(51.4123,27.3832,-6.35298));
        Poles.SetValue(40,33,gp_Pnt(51.3431,30.2167,-5.90237));
        Poles.SetValue(40,34,gp_Pnt(51.2972,32.1041,-5.6001));
        Poles.SetValue(40,35,gp_Pnt(51.2645,34.472,-5.32012));
        Poles.SetValue(40,36,gp_Pnt(51.2449,37.3238,-5.1352));
        Poles.SetValue(40,37,gp_Pnt(51.2601,40.2033,-5.10074));
        Poles.SetValue(40,38,gp_Pnt(51.2747,43.0862,-5.12741));
        Poles.SetValue(40,39,gp_Pnt(51.2989,46.9359,-5.04137));
        Poles.SetValue(40,40,gp_Pnt(51.3275,51.7504,-4.70088));
        Poles.SetValue(40,41,gp_Pnt(51.3841,57.5438,-4.49279));
        Poles.SetValue(40,42,gp_Pnt(51.4284,63.3289,-4.20573));
        Poles.SetValue(40,43,gp_Pnt(51.4853,71.0399,-3.80369));
        Poles.SetValue(40,44,gp_Pnt(51.5952,84.5486,-3.19745));
        Poles.SetValue(40,45,gp_Pnt(51.696,103.788,-2.1844));
        Poles.SetValue(40,46,gp_Pnt(51.8075,126.892,-1.07015));
        Poles.SetValue(40,47,gp_Pnt(51.8814,142.3,-0.382549));
        Poles.SetValue(40,48,gp_Pnt(51.9141,150,0.00156463));
        Poles.SetValue(41,1,gp_Pnt(57.6856,-150,-0.00138605));
        Poles.SetValue(41,2,gp_Pnt(57.6563,-142.267,-0.207898));
        Poles.SetValue(41,3,gp_Pnt(57.5975,-126.803,-0.672181));
        Poles.SetValue(41,4,gp_Pnt(57.4914,-103.604,-1.36423));
        Poles.SetValue(41,5,gp_Pnt(57.3865,-84.2807,-1.92972));
        Poles.SetValue(41,6,gp_Pnt(57.2896,-70.7489,-2.50829));
        Poles.SetValue(41,7,gp_Pnt(57.2325,-63.0194,-2.86727));
        Poles.SetValue(41,8,gp_Pnt(57.1871,-57.2225,-3.14507));
        Poles.SetValue(41,9,gp_Pnt(57.1423,-51.4292,-3.42927));
        Poles.SetValue(41,10,gp_Pnt(57.1017,-46.5994,-3.67864));
        Poles.SetValue(41,11,gp_Pnt(57.0621,-42.7292,-3.85432));
        Poles.SetValue(41,12,gp_Pnt(57.03,-39.8239,-4.01388));
        Poles.SetValue(41,13,gp_Pnt(56.9862,-36.9115,-4.15465));
        Poles.SetValue(41,14,gp_Pnt(56.9332,-33.9994,-4.17982));
        Poles.SetValue(41,15,gp_Pnt(56.8867,-31.5805,-4.17145));
        Poles.SetValue(41,16,gp_Pnt(56.8715,-29.6623,-4.20861));
        Poles.SetValue(41,17,gp_Pnt(56.8491,-26.7863,-4.25366));
        Poles.SetValue(41,18,gp_Pnt(56.889,-23.4691,-4.52661));
        Poles.SetValue(41,19,gp_Pnt(56.9549,-19.6804,-4.85027));
        Poles.SetValue(41,20,gp_Pnt(57.0475,-15.8997,-5.24921));
        Poles.SetValue(41,21,gp_Pnt(57.1486,-12.1132,-5.61949));
        Poles.SetValue(41,22,gp_Pnt(57.2089,-8.31776,-5.89532));
        Poles.SetValue(41,23,gp_Pnt(57.2368,-5.4661,-6.04286));
        Poles.SetValue(41,24,gp_Pnt(57.2712,-1.66308,-6.20009));
        Poles.SetValue(41,25,gp_Pnt(57.2835,2.13802,-6.25618));
        Poles.SetValue(41,26,gp_Pnt(57.2397,5.93802,-6.16615));
        Poles.SetValue(41,27,gp_Pnt(57.2064,8.78585,-6.08208));
        Poles.SetValue(41,28,gp_Pnt(57.1543,12.5793,-5.9372));
        Poles.SetValue(41,29,gp_Pnt(57.0517,16.3584,-5.62801));
        Poles.SetValue(41,30,gp_Pnt(56.9666,20.1329,-5.34449));
        Poles.SetValue(41,31,gp_Pnt(56.9077,23.9136,-5.10547));
        Poles.SetValue(41,32,gp_Pnt(56.8624,27.2205,-4.90474));
        Poles.SetValue(41,33,gp_Pnt(56.894,30.0927,-4.90413));
        Poles.SetValue(41,34,gp_Pnt(56.9172,32.0097,-4.91734));
        Poles.SetValue(41,35,gp_Pnt(56.9647,34.4197,-4.99635));
        Poles.SetValue(41,36,gp_Pnt(57.0094,37.3184,-4.96512));
        Poles.SetValue(41,37,gp_Pnt(57.0441,40.2167,-4.87499));
        Poles.SetValue(41,38,gp_Pnt(57.074,43.1143,-4.7391));
        Poles.SetValue(41,39,gp_Pnt(57.107,46.9716,-4.56627));
        Poles.SetValue(41,40,gp_Pnt(57.1445,51.7898,-4.39661));
        Poles.SetValue(41,41,gp_Pnt(57.1814,57.5667,-4.1162));
        Poles.SetValue(41,42,gp_Pnt(57.2214,63.3465,-3.84863));
        Poles.SetValue(41,43,gp_Pnt(57.2769,71.057,-3.56972));
        Poles.SetValue(41,44,gp_Pnt(57.3669,84.5447,-2.9687));
        Poles.SetValue(41,45,gp_Pnt(57.468,103.793,-2.06019));
        Poles.SetValue(41,46,gp_Pnt(57.5787,126.894,-1.02905));
        Poles.SetValue(41,47,gp_Pnt(57.6483,142.299,-0.34422));
        Poles.SetValue(41,48,gp_Pnt(57.682,150,-0.00209716));
        Poles.SetValue(42,1,gp_Pnt(63.4538,-150,0.0025406));
        Poles.SetValue(42,2,gp_Pnt(63.4247,-142.269,-0.215374));
        Poles.SetValue(42,3,gp_Pnt(63.3664,-126.808,-0.616228));
        Poles.SetValue(42,4,gp_Pnt(63.2638,-103.618,-1.25801));
        Poles.SetValue(42,5,gp_Pnt(63.16,-84.297,-1.79523));
        Poles.SetValue(42,6,gp_Pnt(63.0783,-70.7815,-2.3401));
        Poles.SetValue(42,7,gp_Pnt(63.027,-63.0578,-2.63615));
        Poles.SetValue(42,8,gp_Pnt(62.9884,-57.2671,-2.89533));
        Poles.SetValue(42,9,gp_Pnt(62.9474,-51.4766,-3.14791));
        Poles.SetValue(42,10,gp_Pnt(62.9083,-46.6481,-3.34003));
        Poles.SetValue(42,11,gp_Pnt(62.8727,-42.7819,-3.50126));
        Poles.SetValue(42,12,gp_Pnt(62.844,-39.8816,-3.60833));
        Poles.SetValue(42,13,gp_Pnt(62.8104,-36.9784,-3.72009));
        Poles.SetValue(42,14,gp_Pnt(62.778,-34.08,-3.87178));
        Poles.SetValue(42,15,gp_Pnt(62.7431,-31.6622,-3.99896));
        Poles.SetValue(42,16,gp_Pnt(62.716,-29.7336,-4.07291));
        Poles.SetValue(42,17,gp_Pnt(62.6769,-26.8425,-4.18869));
        Poles.SetValue(42,18,gp_Pnt(62.6426,-23.4886,-4.19616));
        Poles.SetValue(42,19,gp_Pnt(62.6334,-19.6721,-4.26579));
        Poles.SetValue(42,20,gp_Pnt(62.6295,-15.8635,-4.27213));
        Poles.SetValue(42,21,gp_Pnt(62.646,-12.069,-4.36676));
        Poles.SetValue(42,22,gp_Pnt(62.6617,-8.28186,-4.45874));
        Poles.SetValue(42,23,gp_Pnt(62.6691,-5.44261,-4.50132));
        Poles.SetValue(42,24,gp_Pnt(62.6758,-1.65987,-4.58196));
        Poles.SetValue(42,25,gp_Pnt(62.6653,2.12054,-4.61217));
        Poles.SetValue(42,26,gp_Pnt(62.6597,5.90097,-4.63357));
        Poles.SetValue(42,27,gp_Pnt(62.6549,8.73683,-4.65964));
        Poles.SetValue(42,28,gp_Pnt(62.6418,12.5178,-4.65205));
        Poles.SetValue(42,29,gp_Pnt(62.6327,16.3026,-4.65055));
        Poles.SetValue(42,30,gp_Pnt(62.6435,20.0986,-4.70703));
        Poles.SetValue(42,31,gp_Pnt(62.662,23.9034,-4.72568));
        Poles.SetValue(42,32,gp_Pnt(62.7054,27.2507,-4.79821));
        Poles.SetValue(42,33,gp_Pnt(62.7415,30.1344,-4.72137));
        Poles.SetValue(42,34,gp_Pnt(62.7653,32.0573,-4.65936));
        Poles.SetValue(42,35,gp_Pnt(62.7973,34.4696,-4.55886));
        Poles.SetValue(42,36,gp_Pnt(62.8292,37.3615,-4.47524));
        Poles.SetValue(42,37,gp_Pnt(62.8611,40.2567,-4.4017));
        Poles.SetValue(42,38,gp_Pnt(62.885,43.1468,-4.31334));
        Poles.SetValue(42,39,gp_Pnt(62.9162,47.0013,-4.18874));
        Poles.SetValue(42,40,gp_Pnt(62.9483,51.8154,-4.00002));
        Poles.SetValue(42,41,gp_Pnt(62.9852,57.5918,-3.80305));
        Poles.SetValue(42,42,gp_Pnt(63.0192,63.3675,-3.57283));
        Poles.SetValue(42,43,gp_Pnt(63.0676,71.0714,-3.31891));
        Poles.SetValue(42,44,gp_Pnt(63.1421,84.547,-2.75086));
        Poles.SetValue(42,45,gp_Pnt(63.2424,103.797,-1.92907));
        Poles.SetValue(42,46,gp_Pnt(63.3482,126.895,-0.959019));
        Poles.SetValue(42,47,gp_Pnt(63.4166,142.297,-0.27787));
        Poles.SetValue(42,48,gp_Pnt(63.4498,150,0.00445876));
        Poles.SetValue(43,1,gp_Pnt(71.145,-150,-0.000857612));
        Poles.SetValue(43,2,gp_Pnt(71.1162,-142.27,-0.152808));
        Poles.SetValue(43,3,gp_Pnt(71.0596,-126.815,-0.587576));
        Poles.SetValue(43,4,gp_Pnt(70.9617,-103.636,-1.09496));
        Poles.SetValue(43,5,gp_Pnt(70.8648,-84.3248,-1.63803));
        Poles.SetValue(43,6,gp_Pnt(70.796,-70.8215,-2.12446));
        Poles.SetValue(43,7,gp_Pnt(70.7543,-63.1072,-2.37388));
        Poles.SetValue(43,8,gp_Pnt(70.7215,-57.3211,-2.55159));
        Poles.SetValue(43,9,gp_Pnt(70.6876,-51.5353,-2.75534));
        Poles.SetValue(43,10,gp_Pnt(70.6596,-46.716,-2.93875));
        Poles.SetValue(43,11,gp_Pnt(70.638,-42.8622,-3.09314));
        Poles.SetValue(43,12,gp_Pnt(70.6204,-39.9716,-3.17812));
        Poles.SetValue(43,13,gp_Pnt(70.6043,-37.0819,-3.28977));
        Poles.SetValue(43,14,gp_Pnt(70.5873,-34.1932,-3.36653));
        Poles.SetValue(43,15,gp_Pnt(70.5748,-31.7872,-3.44513));
        Poles.SetValue(43,16,gp_Pnt(70.5669,-29.8663,-3.51303));
        Poles.SetValue(43,17,gp_Pnt(70.5542,-26.9849,-3.60722));
        Poles.SetValue(43,18,gp_Pnt(70.5428,-23.6313,-3.75743));
        Poles.SetValue(43,19,gp_Pnt(70.5216,-19.7991,-3.8651));
        Poles.SetValue(43,20,gp_Pnt(70.5074,-15.9743,-4.01435));
        Poles.SetValue(43,21,gp_Pnt(70.4895,-12.1568,-4.07719));
        Poles.SetValue(43,22,gp_Pnt(70.4779,-8.34932,-4.14211));
        Poles.SetValue(43,23,gp_Pnt(70.4743,-5.49851,-4.2144));
        Poles.SetValue(43,24,gp_Pnt(70.4711,-1.70019,-4.26983));
        Poles.SetValue(43,25,gp_Pnt(70.4772,2.09575,-4.33461));
        Poles.SetValue(43,26,gp_Pnt(70.4818,5.8932,-4.36337));
        Poles.SetValue(43,27,gp_Pnt(70.4863,8.74296,-4.35931));
        Poles.SetValue(43,28,gp_Pnt(70.4965,12.5464,-4.34509));
        Poles.SetValue(43,29,gp_Pnt(70.5111,16.3559,-4.34157));
        Poles.SetValue(43,30,gp_Pnt(70.5238,20.1732,-4.25483));
        Poles.SetValue(43,31,gp_Pnt(70.5444,23.9982,-4.20505));
        Poles.SetValue(43,32,gp_Pnt(70.5565,27.3468,-4.1081));
        Poles.SetValue(43,33,gp_Pnt(70.5753,30.2247,-4.08653));
        Poles.SetValue(43,34,gp_Pnt(70.5877,32.1436,-4.07523));
        Poles.SetValue(43,35,gp_Pnt(70.6023,34.545,-4.05073));
        Poles.SetValue(43,36,gp_Pnt(70.6195,37.4276,-3.98741));
        Poles.SetValue(43,37,gp_Pnt(70.6334,40.3106,-3.90872));
        Poles.SetValue(43,38,gp_Pnt(70.6494,43.1951,-3.83));
        Poles.SetValue(43,39,gp_Pnt(70.6689,47.0414,-3.71793));
        Poles.SetValue(43,40,gp_Pnt(70.6945,51.851,-3.59864));
        Poles.SetValue(43,41,gp_Pnt(70.7214,57.6214,-3.39071));
        Poles.SetValue(43,42,gp_Pnt(70.7489,63.3924,-3.20505));
        Poles.SetValue(43,43,gp_Pnt(70.7842,71.0857,-2.88826));
        Poles.SetValue(43,44,gp_Pnt(70.8464,84.5547,-2.43588));
        Poles.SetValue(43,45,gp_Pnt(70.9463,103.804,-1.8405));
        Poles.SetValue(43,46,gp_Pnt(71.0396,126.896,-0.842595));
        Poles.SetValue(43,47,gp_Pnt(71.1097,142.298,-0.284851));
        Poles.SetValue(43,48,gp_Pnt(71.1403,150,0.000935964));
        Poles.SetValue(44,1,gp_Pnt(84.6054,-150,-0.00110981));
        Poles.SetValue(44,2,gp_Pnt(84.578,-142.276,-0.19879));
        Poles.SetValue(44,3,gp_Pnt(84.5262,-126.827,-0.428048));
        Poles.SetValue(44,4,gp_Pnt(84.4365,-103.667,-0.830318));
        Poles.SetValue(44,5,gp_Pnt(84.36,-84.3785,-1.33467));
        Poles.SetValue(44,6,gp_Pnt(84.3031,-70.89,-1.70138));
        Poles.SetValue(44,7,gp_Pnt(84.275,-63.1861,-1.89652));
        Poles.SetValue(44,8,gp_Pnt(84.2584,-57.4138,-2.07504));
        Poles.SetValue(44,9,gp_Pnt(84.248,-51.6477,-2.24438));
        Poles.SetValue(44,10,gp_Pnt(84.2459,-46.8473,-2.35057));
        Poles.SetValue(44,11,gp_Pnt(84.2519,-43.0105,-2.40101));
        Poles.SetValue(44,12,gp_Pnt(84.2621,-40.138,-2.47508));
        Poles.SetValue(44,13,gp_Pnt(84.2775,-37.2661,-2.50855));
        Poles.SetValue(44,14,gp_Pnt(84.2986,-34.4008,-2.62094));
        Poles.SetValue(44,15,gp_Pnt(84.3172,-32.0111,-2.66065));
        Poles.SetValue(44,16,gp_Pnt(84.3284,-30.0979,-2.70139));
        Poles.SetValue(44,17,gp_Pnt(84.3451,-27.2279,-2.76856));
        Poles.SetValue(44,18,gp_Pnt(84.3501,-23.8723,-2.78848));
        Poles.SetValue(44,19,gp_Pnt(84.3568,-20.039,-2.88096));
        Poles.SetValue(44,20,gp_Pnt(84.3439,-16.1977,-2.86279));
        Poles.SetValue(44,21,gp_Pnt(84.3401,-12.3571,-2.99929));
        Poles.SetValue(44,22,gp_Pnt(84.3364,-8.51254,-3.10404));
        Poles.SetValue(44,23,gp_Pnt(84.3307,-5.62646,-3.13212));
        Poles.SetValue(44,24,gp_Pnt(84.3261,-1.77822,-3.18231));
        Poles.SetValue(44,25,gp_Pnt(84.3219,2.0692,-3.18879));
        Poles.SetValue(44,26,gp_Pnt(84.3241,5.91518,-3.18419));
        Poles.SetValue(44,27,gp_Pnt(84.3283,8.7991,-3.19732));
        Poles.SetValue(44,28,gp_Pnt(84.3322,12.6435,-3.19362));
        Poles.SetValue(44,29,gp_Pnt(84.3367,16.4878,-3.15729));
        Poles.SetValue(44,30,gp_Pnt(84.3449,20.3298,-3.212));
        Poles.SetValue(44,31,gp_Pnt(84.3382,24.1652,-3.18681));
        Poles.SetValue(44,32,gp_Pnt(84.3328,27.5176,-3.24238));
        Poles.SetValue(44,33,gp_Pnt(84.3137,30.3832,-3.1762));
        Poles.SetValue(44,34,gp_Pnt(84.3012,32.2934,-3.13458));
        Poles.SetValue(44,35,gp_Pnt(84.2835,34.6788,-3.05639));
        Poles.SetValue(44,36,gp_Pnt(84.2688,37.5436,-3.03341));
        Poles.SetValue(44,37,gp_Pnt(84.2567,40.4108,-2.97846));
        Poles.SetValue(44,38,gp_Pnt(84.2493,43.2807,-2.94203));
        Poles.SetValue(44,39,gp_Pnt(84.2426,47.1105,-2.85403));
        Poles.SetValue(44,40,gp_Pnt(84.2423,51.9032,-2.73437));
        Poles.SetValue(44,41,gp_Pnt(84.2552,57.6633,-2.65697));
        Poles.SetValue(44,42,gp_Pnt(84.2707,63.4255,-2.51041));
        Poles.SetValue(44,43,gp_Pnt(84.2947,71.1123,-2.33047));
        Poles.SetValue(44,44,gp_Pnt(84.3464,84.5727,-1.97042));
        Poles.SetValue(44,45,gp_Pnt(84.4215,103.814,-1.51302));
        Poles.SetValue(44,46,gp_Pnt(84.5086,126.896,-0.626681));
        Poles.SetValue(44,47,gp_Pnt(84.5693,142.301,-0.243958));
        Poles.SetValue(44,48,gp_Pnt(84.5995,150,-0.000554027));
        Poles.SetValue(45,1,gp_Pnt(103.838,-149.999,0.000693631));
        Poles.SetValue(45,2,gp_Pnt(103.817,-142.283,-0.134708));
        Poles.SetValue(45,3,gp_Pnt(103.771,-126.851,-0.393149));
        Poles.SetValue(45,4,gp_Pnt(103.704,-103.715,-0.554331));
        Poles.SetValue(45,5,gp_Pnt(103.653,-84.4538,-0.85582));
        Poles.SetValue(45,6,gp_Pnt(103.619,-70.9776,-1.13828));
        Poles.SetValue(45,7,gp_Pnt(103.6,-63.2834,-1.26826));
        Poles.SetValue(45,8,gp_Pnt(103.585,-57.5128,-1.32722));
        Poles.SetValue(45,9,gp_Pnt(103.569,-51.7417,-1.39145));
        Poles.SetValue(45,10,gp_Pnt(103.554,-46.933,-1.47713));
        Poles.SetValue(45,11,gp_Pnt(103.538,-43.0832,-1.56351));
        Poles.SetValue(45,12,gp_Pnt(103.524,-40.1968,-1.61145));
        Poles.SetValue(45,13,gp_Pnt(103.505,-37.3056,-1.67968));
        Poles.SetValue(45,14,gp_Pnt(103.487,-34.4181,-1.71097));
        Poles.SetValue(45,15,gp_Pnt(103.468,-32.0079,-1.76122));
        Poles.SetValue(45,16,gp_Pnt(103.458,-30.085,-1.80205));
        Poles.SetValue(45,17,gp_Pnt(103.443,-27.2014,-1.86016));
        Poles.SetValue(45,18,gp_Pnt(103.436,-23.8458,-1.96204));
        Poles.SetValue(45,19,gp_Pnt(103.426,-20.0087,-2.01108));
        Poles.SetValue(45,20,gp_Pnt(103.432,-16.1784,-2.11951));
        Poles.SetValue(45,21,gp_Pnt(103.429,-12.3464,-2.09847));
        Poles.SetValue(45,22,gp_Pnt(103.426,-8.51499,-2.09843));
        Poles.SetValue(45,23,gp_Pnt(103.427,-5.64258,-2.13298));
        Poles.SetValue(45,24,gp_Pnt(103.428,-1.81239,-2.15552));
        Poles.SetValue(45,25,gp_Pnt(103.429,2.01779,-2.2127));
        Poles.SetValue(45,26,gp_Pnt(103.425,5.84787,-2.24947));
        Poles.SetValue(45,27,gp_Pnt(103.42,8.7205,-2.23377));
        Poles.SetValue(45,28,gp_Pnt(103.419,12.5505,-2.2506));
        Poles.SetValue(45,29,gp_Pnt(103.418,16.3787,-2.27936));
        Poles.SetValue(45,30,gp_Pnt(103.409,20.2066,-2.15004));
        Poles.SetValue(45,31,gp_Pnt(103.421,24.0411,-2.1335));
        Poles.SetValue(45,32,gp_Pnt(103.43,27.3967,-2.05519));
        Poles.SetValue(45,33,gp_Pnt(103.449,30.2805,-2.04635));
        Poles.SetValue(45,34,gp_Pnt(103.463,32.2036,-2.0388));
        Poles.SetValue(45,35,gp_Pnt(103.484,34.6131,-2.03358));
        Poles.SetValue(45,36,gp_Pnt(103.502,37.4987,-2.0062));
        Poles.SetValue(45,37,gp_Pnt(103.523,40.3884,-1.97852));
        Poles.SetValue(45,38,gp_Pnt(103.537,43.2734,-1.95553));
        Poles.SetValue(45,39,gp_Pnt(103.557,47.1221,-1.93252));
        Poles.SetValue(45,40,gp_Pnt(103.574,51.9281,-1.89403));
        Poles.SetValue(45,41,gp_Pnt(103.584,57.6907,-1.74768));
        Poles.SetValue(45,42,gp_Pnt(103.596,63.4551,-1.66735));
        Poles.SetValue(45,43,gp_Pnt(103.612,71.1425,-1.54309));
        Poles.SetValue(45,44,gp_Pnt(103.644,84.5989,-1.46411));
        Poles.SetValue(45,45,gp_Pnt(103.687,103.818,-0.980467));
        Poles.SetValue(45,46,gp_Pnt(103.757,126.905,-0.536908));
        Poles.SetValue(45,47,gp_Pnt(103.805,142.3,-0.161313));
        Poles.SetValue(45,48,gp_Pnt(103.83,150,-0.000163518));
        Poles.SetValue(46,1,gp_Pnt(126.927,-150.004,-0.000100752));
        Poles.SetValue(46,2,gp_Pnt(126.912,-142.299,-0.130631));
        Poles.SetValue(46,3,gp_Pnt(126.883,-126.887,-0.24799));
        Poles.SetValue(46,4,gp_Pnt(126.843,-103.777,-0.260883));
        Poles.SetValue(46,5,gp_Pnt(126.816,-84.5309,-0.411687));
        Poles.SetValue(46,6,gp_Pnt(126.804,-71.0707,-0.543712));
        Poles.SetValue(46,7,gp_Pnt(126.796,-63.376,-0.610079));
        Poles.SetValue(46,8,gp_Pnt(126.79,-57.6067,-0.646868));
        Poles.SetValue(46,9,gp_Pnt(126.783,-51.839,-0.65842));
        Poles.SetValue(46,10,gp_Pnt(126.779,-47.033,-0.68082));
        Poles.SetValue(46,11,gp_Pnt(126.778,-43.1892,-0.711667));
        Poles.SetValue(46,12,gp_Pnt(126.779,-40.3077,-0.742656));
        Poles.SetValue(46,13,gp_Pnt(126.782,-37.4269,-0.771362));
        Poles.SetValue(46,14,gp_Pnt(126.785,-34.5477,-0.808585));
        Poles.SetValue(46,15,gp_Pnt(126.79,-32.1486,-0.836321));
        Poles.SetValue(46,16,gp_Pnt(126.79,-30.2277,-0.847354));
        Poles.SetValue(46,17,gp_Pnt(126.791,-27.3461,-0.865188));
        Poles.SetValue(46,18,gp_Pnt(126.786,-23.9805,-0.850589));
        Poles.SetValue(46,19,gp_Pnt(126.785,-20.137,-0.872177));
        Poles.SetValue(46,20,gp_Pnt(126.774,-16.2901,-0.841412));
        Poles.SetValue(46,21,gp_Pnt(126.771,-12.4455,-0.888617));
        Poles.SetValue(46,22,gp_Pnt(126.77,-8.6011,-0.924689));
        Poles.SetValue(46,23,gp_Pnt(126.769,-5.71714,-0.928362));
        Poles.SetValue(46,24,gp_Pnt(126.767,-1.87221,-0.950812));
        Poles.SetValue(46,25,gp_Pnt(126.766,1.97273,-0.93774));
        Poles.SetValue(46,26,gp_Pnt(126.768,5.81772,-0.926189));
        Poles.SetValue(46,27,gp_Pnt(126.773,8.70129,-0.947427));
        Poles.SetValue(46,28,gp_Pnt(126.775,12.5459,-0.942477));
        Poles.SetValue(46,29,gp_Pnt(126.78,16.3917,-0.92351));
        Poles.SetValue(46,30,gp_Pnt(126.795,20.2388,-1.01431));
        Poles.SetValue(46,31,gp_Pnt(126.796,24.0826,-1.01183));
        Poles.SetValue(46,32,gp_Pnt(126.802,27.4469,-1.0509));
        Poles.SetValue(46,33,gp_Pnt(126.799,30.3276,-1.04049));
        Poles.SetValue(46,34,gp_Pnt(126.797,32.2478,-1.03441));
        Poles.SetValue(46,35,gp_Pnt(126.793,34.647,-1.02114));
        Poles.SetValue(46,36,gp_Pnt(126.79,37.5265,-1.01193));
        Poles.SetValue(46,37,gp_Pnt(126.787,40.4072,-0.994831));
        Poles.SetValue(46,38,gp_Pnt(126.786,43.2878,-0.961342));
        Poles.SetValue(46,39,gp_Pnt(126.783,47.1296,-0.902547));
        Poles.SetValue(46,40,gp_Pnt(126.782,51.9326,-0.817673));
        Poles.SetValue(46,41,gp_Pnt(126.787,57.6992,-0.788109));
        Poles.SetValue(46,42,gp_Pnt(126.789,63.4643,-0.707706));
        Poles.SetValue(46,43,gp_Pnt(126.797,71.1523,-0.758598));
        Poles.SetValue(46,44,gp_Pnt(126.808,84.6041,-0.552723));
        Poles.SetValue(46,45,gp_Pnt(126.827,103.836,-0.468498));
        Poles.SetValue(46,46,gp_Pnt(126.872,126.912,-0.205591));
        Poles.SetValue(46,47,gp_Pnt(126.901,142.306,-0.0896176));
        Poles.SetValue(46,48,gp_Pnt(126.919,150,-0.00186706));
        Poles.SetValue(47,1,gp_Pnt(142.331,-150.022,-0.0011824));
        Poles.SetValue(47,2,gp_Pnt(142.32,-142.319,-0.0845723));
        Poles.SetValue(47,3,gp_Pnt(142.296,-126.912,-0.0348247));
        Poles.SetValue(47,4,gp_Pnt(142.281,-103.817,-0.216625));
        Poles.SetValue(47,5,gp_Pnt(142.273,-84.5809,-0.194036));
        Poles.SetValue(47,6,gp_Pnt(142.264,-71.117,-0.0962207));
        Poles.SetValue(47,7,gp_Pnt(142.262,-63.4269,-0.109676));
        Poles.SetValue(47,8,gp_Pnt(142.261,-57.6591,-0.134362));
        Poles.SetValue(47,9,gp_Pnt(142.261,-51.8912,-0.17447));
        Poles.SetValue(47,10,gp_Pnt(142.261,-47.0855,-0.198201));
        Poles.SetValue(47,11,gp_Pnt(142.261,-43.2412,-0.213413));
        Poles.SetValue(47,12,gp_Pnt(142.261,-40.3577,-0.219481));
        Poles.SetValue(47,13,gp_Pnt(142.261,-37.474,-0.231785));
        Poles.SetValue(47,14,gp_Pnt(142.261,-34.5899,-0.238239));
        Poles.SetValue(47,15,gp_Pnt(142.261,-32.1863,-0.245761));
        Poles.SetValue(47,16,gp_Pnt(142.261,-30.264,-0.25701));
        Poles.SetValue(47,17,gp_Pnt(142.262,-27.3805,-0.273362));
        Poles.SetValue(47,18,gp_Pnt(142.265,-24.0175,-0.308423));
        Poles.SetValue(47,19,gp_Pnt(142.266,-20.1723,-0.320574));
        Poles.SetValue(47,20,gp_Pnt(142.269,-16.3277,-0.347549));
        Poles.SetValue(47,21,gp_Pnt(142.268,-12.4815,-0.333554));
        Poles.SetValue(47,22,gp_Pnt(142.267,-8.63539,-0.333702));
        Poles.SetValue(47,23,gp_Pnt(142.267,-5.75114,-0.350197));
        Poles.SetValue(47,24,gp_Pnt(142.267,-1.9053,-0.361855));
        Poles.SetValue(47,25,gp_Pnt(142.269,1.94054,-0.382183));
        Poles.SetValue(47,26,gp_Pnt(142.27,5.78645,-0.393577));
        Poles.SetValue(47,27,gp_Pnt(142.27,8.67102,-0.385519));
        Poles.SetValue(47,28,gp_Pnt(142.271,12.5173,-0.39336));
        Poles.SetValue(47,29,gp_Pnt(142.272,16.3631,-0.412844));
        Poles.SetValue(47,30,gp_Pnt(142.268,20.2079,-0.378051));
        Poles.SetValue(47,31,gp_Pnt(142.268,24.0533,-0.383864));
        Poles.SetValue(47,32,gp_Pnt(142.265,27.4171,-0.365288));
        Poles.SetValue(47,33,gp_Pnt(142.265,30.301,-0.36621));
        Poles.SetValue(47,34,gp_Pnt(142.265,32.2237,-0.366061));
        Poles.SetValue(47,35,gp_Pnt(142.266,34.6274,-0.366875));
        Poles.SetValue(47,36,gp_Pnt(142.265,37.5114,-0.360017));
        Poles.SetValue(47,37,gp_Pnt(142.265,40.395,-0.352827));
        Poles.SetValue(47,38,gp_Pnt(142.264,43.2786,-0.337981));
        Poles.SetValue(47,39,gp_Pnt(142.264,47.1233,-0.322015));
        Poles.SetValue(47,40,gp_Pnt(142.263,51.9292,-0.307379));
        Poles.SetValue(47,41,gp_Pnt(142.263,57.6957,-0.274754));
        Poles.SetValue(47,42,gp_Pnt(142.265,63.4634,-0.312079));
        Poles.SetValue(47,43,gp_Pnt(142.267,71.1535,-0.274627));
        Poles.SetValue(47,44,gp_Pnt(142.268,84.6121,-0.229297));
        Poles.SetValue(47,45,gp_Pnt(142.275,103.837,-0.124819));
        Poles.SetValue(47,46,gp_Pnt(142.29,126.918,-0.114093));
        Poles.SetValue(47,47,gp_Pnt(142.315,142.308,0.0555229));
        Poles.SetValue(47,48,gp_Pnt(142.328,150.004,0.00172523));
        Poles.SetValue(48,1,gp_Pnt(150.034,-150.032,0.000374255));
        Poles.SetValue(48,2,gp_Pnt(150.024,-142.329,-0.0018713));
        Poles.SetValue(48,3,gp_Pnt(150.004,-126.925,0.00327151));
        Poles.SetValue(48,4,gp_Pnt(149.999,-103.837,-0.00371772));
        Poles.SetValue(48,5,gp_Pnt(150,-84.6045,0.00227291));
        Poles.SetValue(48,6,gp_Pnt(150,-71.1444,-0.00370124));
        Poles.SetValue(48,7,gp_Pnt(150,-63.4533,0.00501253));
        Poles.SetValue(48,8,gp_Pnt(150,-57.6854,-0.0023337));
        Poles.SetValue(48,9,gp_Pnt(150,-51.9177,-0.00081928));
        Poles.SetValue(48,10,gp_Pnt(150.001,-47.1109,-0.000992593));
        Poles.SetValue(48,11,gp_Pnt(150.001,-43.2652,0.000772656));
        Poles.SetValue(48,12,gp_Pnt(150.001,-40.381,-3.16637e-006));
        Poles.SetValue(48,13,gp_Pnt(150.001,-37.4966,0.00148961));
        Poles.SetValue(48,14,gp_Pnt(150.001,-34.6123,0.00149543));
        Poles.SetValue(48,15,gp_Pnt(150.001,-32.2086,-0.000131478));
        Poles.SetValue(48,16,gp_Pnt(150.001,-30.2856,-0.000312929));
        Poles.SetValue(48,17,gp_Pnt(150.001,-27.4012,-0.00119273));
        Poles.SetValue(48,18,gp_Pnt(150.002,-24.036,0.000913256));
        Poles.SetValue(48,19,gp_Pnt(150.002,-20.19,-0.000202949));
        Poles.SetValue(48,20,gp_Pnt(150.003,-16.3441,0.0036406));
        Poles.SetValue(48,21,gp_Pnt(150.003,-12.4982,0.0019362));
        Poles.SetValue(48,22,gp_Pnt(150.004,-8.65203,0.000622563));
        Poles.SetValue(48,23,gp_Pnt(150.005,-5.7673,0.00229496));
        Poles.SetValue(48,24,gp_Pnt(150.005,-1.92098,-7.68902e-005));
        Poles.SetValue(48,25,gp_Pnt(150.005,1.92543,0.000370792));
        Poles.SetValue(48,26,gp_Pnt(150.005,5.77187,0.00239266));
        Poles.SetValue(48,27,gp_Pnt(150.004,8.65667,0.000574651));
        Poles.SetValue(48,28,gp_Pnt(150.004,12.503,0.00222324));
        Poles.SetValue(48,29,gp_Pnt(150.003,16.3492,0.00598073));
        Poles.SetValue(48,30,gp_Pnt(150.002,20.1954,0.000135085));
        Poles.SetValue(48,31,gp_Pnt(150.002,24.0417,0.00238843));
        Poles.SetValue(48,32,gp_Pnt(150.002,27.4072,-0.000189144));
        Poles.SetValue(48,33,gp_Pnt(150.001,30.2919,0.000229755));
        Poles.SetValue(48,34,gp_Pnt(150.001,32.2151,0.000156563));
        Poles.SetValue(48,35,gp_Pnt(150.001,34.6191,-0.00112558));
        Poles.SetValue(48,36,gp_Pnt(150.001,37.5038,-0.000249663));
        Poles.SetValue(48,37,gp_Pnt(150.001,40.3885,0.000316503));
        Poles.SetValue(48,38,gp_Pnt(150.001,43.273,-0.00132504));
        Poles.SetValue(48,39,gp_Pnt(150.001,47.119,-0.00179323));
        Poles.SetValue(48,40,gp_Pnt(150,51.9261,0.00186415));
        Poles.SetValue(48,41,gp_Pnt(150,57.6944,0.000887863));
        Poles.SetValue(48,42,gp_Pnt(150,63.4625,-0.00455467));
        Poles.SetValue(48,43,gp_Pnt(150,71.1535,0.00329757));
        Poles.SetValue(48,44,gp_Pnt(150,84.613,-0.00125356));
        Poles.SetValue(48,45,gp_Pnt(150,103.842,0.000250841));
        Poles.SetValue(48,46,gp_Pnt(150,126.921,-0.00110007));
        Poles.SetValue(48,47,gp_Pnt(150.022,142.311,0.00127182));
        Poles.SetValue(48,48,gp_Pnt(150.034,150.006,9.4637e-005));




        TColStd_Array1OfReal UKnots(1,46);
        UKnots.SetValue(1,0);
        UKnots.SetValue(2,0.0769231);
        UKnots.SetValue(3,0.153846);
        UKnots.SetValue(4,0.230769);
        UKnots.SetValue(5,0.269231);
        UKnots.SetValue(6,0.288462);
        UKnots.SetValue(7,0.307692);
        UKnots.SetValue(8,0.326923);
        UKnots.SetValue(9,0.346154);
        UKnots.SetValue(10,0.365385);
        UKnots.SetValue(11,0.375);
        UKnots.SetValue(12,0.384615);
        UKnots.SetValue(13,0.394231);
        UKnots.SetValue(14,0.403846);
        UKnots.SetValue(15,0.413462);
        UKnots.SetValue(16,0.423077);
        UKnots.SetValue(17,0.432692);
        UKnots.SetValue(18,0.442308);
        UKnots.SetValue(19,0.461538);
        UKnots.SetValue(20,0.480769);
        UKnots.SetValue(21,0.485577);
        UKnots.SetValue(22,0.490385);
        UKnots.SetValue(23,0.495192);
        UKnots.SetValue(24,0.504808);
        UKnots.SetValue(25,0.509615);
        UKnots.SetValue(26,0.514423);
        UKnots.SetValue(27,0.519231);
        UKnots.SetValue(28,0.538462);
        UKnots.SetValue(29,0.557692);
        UKnots.SetValue(30,0.567308);
        UKnots.SetValue(31,0.576923);
        UKnots.SetValue(32,0.586538);
        UKnots.SetValue(33,0.596154);
        UKnots.SetValue(34,0.605769);
        UKnots.SetValue(35,0.615385);
        UKnots.SetValue(36,0.625);
        UKnots.SetValue(37,0.634615);
        UKnots.SetValue(38,0.653846);
        UKnots.SetValue(39,0.673077);
        UKnots.SetValue(40,0.692308);
        UKnots.SetValue(41,0.711538);
        UKnots.SetValue(42,0.730769);
        UKnots.SetValue(43,0.769231);
        UKnots.SetValue(44,0.846154);
        UKnots.SetValue(45,0.923077);
        UKnots.SetValue(46,1);

        TColStd_Array1OfReal VKnots(1,46);
        VKnots.SetValue(1,0);
        VKnots.SetValue(2,0.0769231);
        VKnots.SetValue(3,0.153846);
        VKnots.SetValue(4,0.230769);
        VKnots.SetValue(5,0.269231);
        VKnots.SetValue(6,0.288462);
        VKnots.SetValue(7,0.307692);
        VKnots.SetValue(8,0.326923);
        VKnots.SetValue(9,0.346154);
        VKnots.SetValue(10,0.355769);
        VKnots.SetValue(11,0.365385);
        VKnots.SetValue(12,0.375);
        VKnots.SetValue(13,0.384615);
        VKnots.SetValue(14,0.394231);
        VKnots.SetValue(15,0.399038);
        VKnots.SetValue(16,0.403846);
        VKnots.SetValue(17,0.423077);
        VKnots.SetValue(18,0.432692);
        VKnots.SetValue(19,0.442308);
        VKnots.SetValue(20,0.461538);
        VKnots.SetValue(21,0.471154);
        VKnots.SetValue(22,0.480769);
        VKnots.SetValue(23,0.490385);
        VKnots.SetValue(24,0.509615);
        VKnots.SetValue(25,0.519231);
        VKnots.SetValue(26,0.528846);
        VKnots.SetValue(27,0.538462);
        VKnots.SetValue(28,0.557692);
        VKnots.SetValue(29,0.567308);
        VKnots.SetValue(30,0.576923);
        VKnots.SetValue(31,0.596154);
        VKnots.SetValue(32,0.600962);
        VKnots.SetValue(33,0.605769);
        VKnots.SetValue(34,0.615385);
        VKnots.SetValue(35,0.625);
        VKnots.SetValue(36,0.634615);
        VKnots.SetValue(37,0.644231);
        VKnots.SetValue(38,0.653846);
        VKnots.SetValue(39,0.673077);
        VKnots.SetValue(40,0.692308);
        VKnots.SetValue(41,0.711538);
        VKnots.SetValue(42,0.730769);
        VKnots.SetValue(43,0.769231);
        VKnots.SetValue(44,0.846154);
        VKnots.SetValue(45,0.923077);
        VKnots.SetValue(46,1);

        TColStd_Array1OfInteger UMults(1,46);
        UMults.SetValue(1,4);
        UMults.SetValue(2,1);
        UMults.SetValue(3,1);
        UMults.SetValue(4,1);
        UMults.SetValue(5,1);
        UMults.SetValue(6,1);
        UMults.SetValue(7,1);
        UMults.SetValue(8,1);
        UMults.SetValue(9,1);
        UMults.SetValue(10,1);
        UMults.SetValue(11,1);
        UMults.SetValue(12,1);
        UMults.SetValue(13,1);
        UMults.SetValue(14,1);
        UMults.SetValue(15,1);
        UMults.SetValue(16,1);
        UMults.SetValue(17,1);
        UMults.SetValue(18,1);
        UMults.SetValue(19,1);
        UMults.SetValue(20,1);
        UMults.SetValue(21,1);
        UMults.SetValue(22,1);
        UMults.SetValue(23,1);
        UMults.SetValue(24,1);
        UMults.SetValue(25,1);
        UMults.SetValue(26,1);
        UMults.SetValue(27,1);
        UMults.SetValue(28,1);
        UMults.SetValue(29,1);
        UMults.SetValue(30,1);
        UMults.SetValue(31,1);
        UMults.SetValue(32,1);
        UMults.SetValue(33,1);
        UMults.SetValue(34,1);
        UMults.SetValue(35,1);
        UMults.SetValue(36,1);
        UMults.SetValue(37,1);
        UMults.SetValue(38,1);
        UMults.SetValue(39,1);
        UMults.SetValue(40,1);
        UMults.SetValue(41,1);
        UMults.SetValue(42,1);
        UMults.SetValue(43,1);
        UMults.SetValue(44,1);
        UMults.SetValue(45,1);
        UMults.SetValue(46,4);

        TColStd_Array1OfInteger  VMults(1,46);
        VMults.SetValue(1,4);
        VMults.SetValue(2,1);
        VMults.SetValue(3,1);
        VMults.SetValue(4,1);
        VMults.SetValue(5,1);
        VMults.SetValue(6,1);
        VMults.SetValue(7,1);
        VMults.SetValue(8,1);
        VMults.SetValue(9,1);
        VMults.SetValue(10,1);
        VMults.SetValue(11,1);
        VMults.SetValue(12,1);
        VMults.SetValue(13,1);
        VMults.SetValue(14,1);
        VMults.SetValue(15,1);
        VMults.SetValue(16,1);
        VMults.SetValue(17,1);
        VMults.SetValue(18,1);
        VMults.SetValue(19,1);
        VMults.SetValue(20,1);
        VMults.SetValue(21,1);
        VMults.SetValue(22,1);
        VMults.SetValue(23,1);
        VMults.SetValue(24,1);
        VMults.SetValue(25,1);
        VMults.SetValue(26,1);
        VMults.SetValue(27,1);
        VMults.SetValue(28,1);
        VMults.SetValue(29,1);
        VMults.SetValue(30,1);
        VMults.SetValue(31,1);
        VMults.SetValue(32,1);
        VMults.SetValue(33,1);
        VMults.SetValue(34,1);
        VMults.SetValue(35,1);
        VMults.SetValue(36,1);
        VMults.SetValue(37,1);
        VMults.SetValue(38,1);
        VMults.SetValue(39,1);
        VMults.SetValue(40,1);
        VMults.SetValue(41,1);
        VMults.SetValue(42,1);
        VMults.SetValue(43,1);
        VMults.SetValue(44,1);
        VMults.SetValue(45,1);
        VMults.SetValue(46,4);

        // Creating the BSpline Surface
        Handle(Geom_BSplineSurface) Surface = new Geom_BSplineSurface(
            Poles,        // const TColgp_Array2OfPnt &    Poles,
            UKnots,       // const TColStd_Array1OfReal &   UKnots,
            VKnots,       // const TColStd_Array1OfReal &   VKnots,
            UMults,       // const TColStd_Array1OfInteger &   UMults,
            VMults,       // const TColStd_Array1OfInteger &   VMults,
            3,            // const Standard_Integer   UDegree,
            3             // const Standard_Integer   VDegree,
            // const Standard_Boolean   UPeriodic = Standard_False,
            // const Standard_Boolean   VPeriodic = Standard_False*/
        );





        BRepBuilderAPI_MakeFace  Face(Surface);

        return new TopoShapePy(new TopoShape(Face.Face()));
    } PY_CATCH;
}


static PyObject * createPlane(PyObject *self, PyObject *args)
{

    double z_level;

    //const char* Name;
    if (! PyArg_ParseTuple(args, "d", &z_level))
        return NULL;


    PY_TRY
    {

        gp_Pnt aPlanePnt(0,0,z_level);
        gp_Dir aPlaneDir(0,0,1);
        Handle_Geom_Plane aPlane = new Geom_Plane(aPlanePnt, aPlaneDir);
        BRepBuilderAPI_MakeFace  Face(aPlane);

        return new TopoShapePy(new TopoShape(Face.Face()));
    } PY_CATCH;
}

static PyObject * createBox(PyObject *self, PyObject *args)
{
    double X, Y, Z , L, H, W ;

    //const char* Name;
    if (! PyArg_ParseTuple(args, "dddddd", &X, &Y, &Z , &L, &H, &W ))
        return NULL;


    PY_TRY
    {
        // Build a box using the dimension and position attributes
        BRepPrimAPI_MakeBox mkBox( gp_Pnt( X, Y, Z ), L, H, W );

        TopoDS_Shape ResultShape = mkBox.Shape();

        return new TopoShapePy(new TopoShape(ResultShape));
    } PY_CATCH;
}

static PyObject * useMesh(PyObject *self, PyObject *args)
{
    MeshPy   *pcObject;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!; Need exatly one Mesh object", &(MeshPy::Type), &pcObj))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    pcObject = (MeshPy*)pcObj;

    PY_TRY
    {

        const MeshObject *aMeshObject = pcObject->getMeshObjectPtr();
        const MeshKernel& m = aMeshObject->getKernel();
        //MeshAlgos::boolean(&_cMesh,&m,&_cMesh,0);
        MeshKernel copy = m;
        MeshCore::MeshAlgorithm algo(copy);
        std::list< std::vector <unsigned long> > BoundariesIndex;
        std::list< std::vector <unsigned long> > ::iterator bin_it;
        std::list< std::vector <Base::Vector3f> > BoundariesPoints;
        algo.GetMeshBorders(BoundariesIndex);
        algo.GetMeshBorders(BoundariesPoints);
        Base::BoundBox3f BoundBox = copy.GetBoundBox();


        // Count of edges
        m.CountEdges();
        // Count of vertices
        m.CountPoints();
        // Count of Triangles
        m.CountFacets();

        // Neighbour triangles
        unsigned long idx1,idx2,idx3,idx=0;
        m.GetFacetNeighbours(idx,idx1,idx2,idx3);

        // points of a triangle
        m.GetFacetPoints(idx,idx1,idx2,idx3);
        MeshCore::MeshPoint p = m.GetPoint(idx1);
        //float x = p[0];
        //float y = p[1];
        //float z = p[2];

        // topological stuff, works only on a non const copy....
        //MeshCore::MeshTopoAlgorithm  TopAlgs(m);

        // e.g. Iterators
        MeshCore::MeshFacetIterator It(m);
        while (It.More())
        {
            ++It;
        }

        // most of the algoristhms are under src/Mod/Mesh/App/Core!

    } PY_CATCH;

    Py_Return;
}
//static PyObject * MyApprox(PyObject *self, PyObject *args)
//{
//  MeshPy   *pcObject;
//  PyObject *pcObj;
//  double tolerance;
//  if (!PyArg_ParseTuple(args, "O!d; Usage:- MyApprox(meshobject, tolerance)", &(MeshPy::Type), &pcObj, &tolerance))     // convert args: Python->C
//    return NULL;                             // NULL triggers exception
//
//  pcObject = (MeshPy*)pcObj;
//
//  PY_TRY {
//    const MeshObject *o = pcObject->getMesh();
// const MeshKernel &m = o->getKernel();
//    //MeshAlgos::boolean(&_cMesh,&m,&_cMesh,0);
// //MeshKernel copy = m;
//
// std::vector<double> Control;
// std::vector<double> KntU;
// std::vector<double> KntV;
// int OrdU;
// int OrdV;
// try {
//  Approximate approx((MeshKernel &)m,Control,KntU,KntV,OrdU,OrdV,tolerance);
// }
// catch(const char *errstr) { std::cerr << errstr << std::endl; }
// int maxCntrlU = KntU.size() - OrdU;
// int maxCntrlV = KntU.size() - OrdV;
// //Load Control Pnts
// TColgp_Array2OfPnt Poles(1,maxCntrlU,1,maxCntrlV);
// for(int u = 0; u < maxCntrlU; u++)
// {
//  for(int v = 0; v < maxCntrlV; v++)
//   Poles.SetValue(u+1,v+1,gp_Pnt(Control[(u*3)+(3*maxCntrlU*v)],Control[(u*3)+(3*maxCntrlU*v)+1],
//   Control[(u*3)+(3*maxCntrlU*v)+2]));
// }
// //Load U-Knot Vector
// TColStd_Array1OfReal UKnots(1,KntU.size() - 6);
// TColStd_Array1OfInteger UMults(1,KntU.size() - 6);
// UKnots.SetValue(1,KntU[0]);
// for(unsigned int i = 1, j = 1; i < KntU.size(); i++)
// {
//  if(KntU[i] == KntU[i-1])
//   continue;
//  else
//  {
//   UKnots.SetValue(j+1,KntU[i]);
//   j++;
//  }
//
// }
// UMults.SetValue(1,4);
// UMults.SetValue(KntU.size() - 6,4);
// for(unsigned int i = 1; i < KntU.size() - 7; i++)
//  UMults.SetValue(i+1,1);
// //Load V-Knot Vector
// TColStd_Array1OfReal VKnots(1,KntU.size() - 6);
// TColStd_Array1OfInteger VMults(1,KntU.size() - 6);
// VKnots.SetValue(1,KntV[0]);
// for(unsigned int i = 1, j = 1; i < KntV.size(); i++)
// {
//  if(KntV[i] == KntV[i-1])
//   continue;
//  else
//  {
//   VKnots.SetValue(j+1,KntV[i]);
//   j++;
//  }
//
// }
// VMults.SetValue(1,4);
// VMults.SetValue(KntV.size() - 6,4);
// for(unsigned int i = 1; i < KntV.size() - 7; i++)
//  VMults.SetValue(i+1,1);
//
// Handle(Geom_BSplineSurface) Surface = new Geom_BSplineSurface(
//                                      Poles,        // const TColgp_Array2OfPnt &    Poles,
//                                     UKnots,       // const TColStd_Array1OfReal &   UKnots,
//                                     VKnots,       // const TColStd_Array1OfReal &   VKnots,
//                                     UMults,       // const TColStd_Array1OfInteger &   UMults,
//                                     VMults,       // const TColStd_Array1OfInteger &   VMults,
//                                     3,            // const Standard_Integer   UDegree,
//                                     3             // const Standard_Integer   VDegree,
//                                                   // const Standard_Boolean   UPeriodic = Standard_False,
//                                                   // const Standard_Boolean   VPeriodic = Standard_False*/
//                           );
//
//
//
//
//
//    BRepBuilderAPI_MakeFace  Face(Surface);
//
//    return new TopoShapePyOld(Face.Face());
//  } PY_CATCH;
//
//  Py_Return;
//}
static PyObject * openDYNA(PyObject *self, PyObject *args)
{
    const char* filename;
    if (! PyArg_ParseTuple(args, "s;Usage:- openDYNA(filename)", &filename))
        return NULL;
    PY_TRY
    {
        MeshCore::MeshKernel mesh;
        ReadDyna parse(mesh,filename);
        MeshObject aObject(mesh);

        return aObject.getPyObject();
    }
    PY_CATCH;

    Py_Return;

}


static PyObject * offset_mesh(PyObject *self, PyObject *args)
{
    double offset;

    MeshPy   *pcObject;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!d; Need exatly one Mesh object", &(MeshPy::Type), &pcObj, &offset))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    pcObject = (MeshPy*)pcObj;
    Base::Builder3D log3d;

    PY_TRY
    {

        MeshObject *o = pcObject->getMeshObjectPtr();
        MeshKernel &mesh = o->getKernel();

        //Base::Vector3f Point[3];
        Base::Vector3f current_pnt;


        //const MeshCore::MeshFacetArray& Facets = mesh.GetFacets();
        //const MeshCore::MeshPointArray& Points = mesh.GetPoints();

        MeshCore::MeshPointIterator p_it(mesh);
        MeshCore::MeshFacetIterator f_it(mesh);
        MeshCore::MeshRefPointToFacets rf2pt(mesh);
        MeshCore::MeshGeomFacet t_face;

        //int NumOfPoints = mesh.CountPoints();

        Base::Vector3f normal,local_normal;
        //float fArea = 0.0f;



        for (unsigned long i=0; i<mesh.CountPoints(); i++)
        {
            // Satz von Dreiecken zu jedem Punkt
            const std::set<unsigned long>& faceSet = rf2pt[i];
            float fArea = 0.0;
            normal.Set(0.0,0.0,0.0);


            // Iteriere über die Dreiecke zu jedem Punkt
            for (std::set<unsigned long>::const_iterator it = faceSet.begin(); it != faceSet.end(); ++it)
            {
                // Einmal derefernzieren, um an das MeshFacet zu kommen und dem Kernel uebergeben, dass er ein MeshGeomFacet liefert
                t_face = mesh.GetFacet(*it);
                // Flaecheninhalt aufsummieren
                float local_Area = t_face.Area();
                local_normal = t_face.GetNormal();
                if (local_normal.z < 0)
                {
                    local_normal = local_normal * (-1);
                }

                fArea = fArea + local_Area;
                normal = normal + local_normal;

            }

            normal.Normalize();
            log3d.addSingleArrow(mesh.GetPoint(i),mesh.GetPoint(i) + (normal*float(offset)));
            mesh.MovePoint(i,(normal*float(offset)));

        }

        log3d.saveToFile("c:/test.iv");
        MeshObject aObject(mesh);
        return new MeshPy(&aObject);
    }
    PY_CATCH;

    Py_Return;



    /*for(p_it.Begin();!(p_it.EndReached()); ++p_it)
    {
     cout << "Erste Schleife" <<endl;
     for(f_it.Begin(); !(f_it.EndReached()); ++f_it)
     {
      cout << "Zweite Schleife" <<endl;
      int pos = f_it.Position();
      t_face = mesh.GetFacet(f_it.Position());

      for (int i = 0; i < 3; ++i)
      {
       cout << "dritte Schleife" <<endl;
       if(*p_it == t_face._aclPoints[i])
       {
        a += t_face.Area();
        normal = t_face.Area()*t_face.GetNormal();
       }
      }
      normal = normal/a;
      n_vect.push_back(normal);
      log3d.addPoint(normal);
     }
    }*/
}


//static PyObject * mesh_build(PyObject *self, PyObject *args)
//{
// PyObject *pcObj;
//
// if (!PyArg_ParseTuple(args, "O!; Need exatly one CAD object",&(TopoShapePyOld::Type), &pcObj))     // convert args: Python->C
//  return NULL;                             // NULL triggers exception
//
//
// TopoShapePyOld *pcShape = static_cast<TopoShapePyOld*>(pcObj); //Surface wird übergeben
//
// TopExp_Explorer Ex;
// Ex.Init(pcShape->getShape(),TopAbs_FACE);  // initialisiere cad-geometrie (trimmed surface)
//
// Base::Builder3D m_log3d;
//
// // surface types
// TopoDS_Face atopo_surface,atopo_surface2;
// BRepAdaptor_Surface adaptor_surface;
// GeomAdaptor_Surface geom_adapterSurface;
// Handle_Geom_Surface geom_surf;
// GeomAbs_SurfaceType type;
// gp_Pln plane;
//
// TopoDS_Shape cad = pcShape->getShape();
//
// // point types
// std::vector<Base::Vector3f> point_list;
// gp_Pnt tmp_pnt;
// Base::Vector3f pnt;
// gp_Dir pl_vec;
// gp_Lin lin;
//
//
// // bounding box
// Bnd_Box BBox;
// Standard_Real XMin1, YMin1, ZMin1, XMax1, YMax1, ZMax1;
//
// IntCurvesFace_ShapeIntersector shp_int;
// BRepClass3d_SolidClassifier    check;
//
//
// double u_min,v_min,u_max,v_max;
// double u_range, v_range;
// double N;
// int n1,n2;
// bool b;
//
// if (!Ex.More()) return false;
//
//    for (;Ex.More();Ex.Next())
//    {
//  // übergebe die einzelnen patches
//  atopo_surface = TopoDS::Face (Ex.Current());
//  adaptor_surface.Initialize(atopo_surface);
//
//  type = adaptor_surface.GetType();
//
//  if(type == GeomAbs_Plane)
//  {
//   check.Load(atopo_surface);
//
//   plane = adaptor_surface.Plane();
//   pl_vec = (plane.Axis()).Direction();
//
//   /*geom_adapterSurface = adaptor_surface.Surface();
//   geom_surf = geom_adapterSurface.Surface();*/
//
//   u_min = adaptor_surface.FirstUParameter();
//   u_max = adaptor_surface.LastUParameter();
//
//   v_min = adaptor_surface.FirstVParameter();
//   v_max = adaptor_surface.LastVParameter();
//
//   u_range = u_max - u_min;
//   v_range = v_max - v_min;
//
//   BRepBndLib::Add(atopo_surface, BBox);
//   BBox.SetGap(0.0);
//   BBox.Get(XMin1, YMin1, ZMin1, XMax1, YMax1, ZMax1);
//
//   N  = (XMax1 - XMin1)*(YMax1 - YMin1)/1e+4;
//   n1 = sqrt(N);//(XMax1 - XMin1)*sqrt(N)/(YMax1 - YMin1);
//   n2 = sqrt(N);//(YMax1 - YMin1)*sqrt(N)/(XMax1 - XMin1);
//
//   if(n1<2)
//    n1=2;
//   if(n2<2)
//    n2=2;
//
//   BBox.SetVoid();
//
//   for(int i=0; i<n1; ++i)
//   {
//    for(int j=0; j<n2; ++j)
//    {
//     adaptor_surface.D0(u_min + i*u_range/(n1-1) ,v_min + j*v_range/(n2-1) ,tmp_pnt);
//     check.Perform(tmp_pnt, 0.1);
//     b = check.IsOnAFace();
//
//     if(b==true)
//     {
//      pnt.x = tmp_pnt.X();
//      pnt.y = tmp_pnt.Y();
//      pnt.z = tmp_pnt.Z();
//
//      point_list.push_back(pnt);
//      m_log3d.addSinglePoint(pnt);
//     }
//     else
//     {
//      lin.SetLocation(tmp_pnt);
//      lin.SetDirection(pl_vec);
//
//      shp_int.Load(cad, 0.1);
//      shp_int.PerformNearest(lin, -RealLast(), +RealLast());
//
//      if(shp_int.IsDone())
//      {
//       tmp_pnt = shp_int.Pnt(1);
//
//       pnt.x = tmp_pnt.X();
//       pnt.y = tmp_pnt.Y();
//       pnt.z = tmp_pnt.Z();
//
//       point_list.push_back(pnt);
//       m_log3d.addSinglePoint(pnt,2,0,0,0);
//      }
//     }
//    }
//   }
//   break;
//  }
// }
//
//
// m_log3d.saveToFile("c:/test_trim.iv");
//
//
// PY_TRY
// {
//  MeshCore::MeshKernel mesh;
//    MeshCore::MeshBuilder builder(mesh);
//  builder.Initialize(point_list.size()-3);
//  Base::Vector3f Points[3];
//
//  for(unsigned int i = 0; i < point_list.size()-2; ++i)
//  {
//   for(unsigned int j = 0; j < 3; j++)
//    Points[j] = point_list[i+j];
//
//
//   MeshCore::MeshGeomFacet Face(Points[0],Points[1],Points[2]);
//   Face.CalcNormal();
//   builder.AddFacet(Face);
//
//  }
//  builder.Finish();
//
//  return new MeshPy(mesh);
//
//
//
// }PY_CATCH;
//
// Py_Return;
//}

//static PyObject * best_fit(PyObject *self, PyObject *args)
//{
// MeshPy   *pcObject;
// MeshPy   *pcObject2;
// PyObject *pcObj;
// PyObject *pcObj2;
//
//
// Base::Builder3D log3d;
// Base::Vector3f pnt(0.0,0.0,0.0);
//
// if (!PyArg_ParseTuple(args, "O!O!; Need two Mesh objects and one toposhape", &(MeshPy::Type), &pcObj, &(MeshPy::Type), &pcObj2))     // convert args: Python->C
//  return NULL;                             // NULL triggers exception
//
// pcObject  = (MeshPy*)pcObj;
// pcObject2 = (MeshPy*)pcObj2;
//
//
// PY_TRY
// {
//  MeshCore::MeshKernel mesh  = pcObject->getMesh();  // Input Mesh
//  MeshCore::MeshKernel mesh2 = pcObject2->getMesh(); // Mesh from CAD
//
//  MeshCore::MeshEigensystem pca(mesh);
//  pca.Evaluate();
//
//  MeshCore::MeshEigensystem pca2(mesh2);
//  pca2.Evaluate();
//
//
//
//  Base::Matrix4D T1 =  pca.Transform();
//  Base::Matrix4D T2 =  pca2.Transform();
//
//     T2[0][3] = 0.0;
//  T2[1][3] = 0.0;
//  T2[2][3] = 0.0;
//
//  Base::Matrix4D C;
//  C.unity();
//
//  T2.inverse();
//  mesh.Transform(T2*T1);
//
//  /*
//  const MeshCore::MeshPointArray& rPoints = mesh.GetPoints();
//  Base::Vector3f vec(0.0,0.0,0.0);
//  unsigned long c=0;
//
//
//  for(MeshCore::MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it)
//  {
//   vec = *it;
//   //vec = T*vec;
//   //T.transform(vec,T);
//   mesh.SetPoint(c,vec);
//   ++c;
//  }
//  */
//
//
//  log3d.addSinglePoint(pnt);
//  log3d.saveToFile("c:/origin.iv");
//  return new MeshPy(mesh);
//
// }PY_CATCH;
//
// Py_Return;
//}



static PyObject * best_fit_complete(PyObject *self, PyObject *args)
{
    MeshPy   *pcObject;
    PyObject *pcObj;
    PyObject *pcObj2;

    if (!PyArg_ParseTuple(args, "O!O!; Need one Mesh objects and one toposhape", &(MeshPy::Type), &pcObj, &(TopoShapePy::Type), &pcObj2))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY
    {
        GProp_GProps prop;
        GProp_PrincipalProps pprop;
        gp_Pnt orig;

        pcObject  = (MeshPy*)pcObj;
        TopoShapePy *pcShape = static_cast<TopoShapePy*>(pcObj2); //Shape wird übergeben
        TopoDS_Shape cad           = pcShape->getTopoShapePtr()->_Shape;  // Input CAD
        MeshCore::MeshKernel mesh  = pcObject->getMeshObjectPtr()->getKernel();  // Input Mesh

//        best_fit befi(&mesh,&cad);
        //befi.Perform();

        //MeshObject anObject(*(befi.m_Mesh));

        //return new MeshPy(&anObject);

    }PY_CATCH;

    Py_Return;
}

#include "WireExplorer.h"
#include <GeomAPI_Interpolate.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include "BRepAdaptor_CompCurve2.h"
#include "SpringbackCorrection.h"
static PyObject * best_fit_test(PyObject *self, PyObject *args)
{

    PyObject *pcObj;


    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY
    {

        TopoShapePy *pcShape = static_cast<TopoShapePy*>(pcObj); //Shape wird übergeben
        TopoDS_Shape aShape = pcShape->getTopoShapePtr()->_Shape;
        TopExp_Explorer anExplorer;
        TopExp_Explorer aFaceExplorer;

        //SpringbackCorrection aShapeTriangulator(pcShape->getShape());
        //aShapeTriangulator.Init();
        std::vector<TopoDS_Edge> anEdgeList;
        TopoDS_Face first,second;
        anEdgeList.clear();
        bool firstrun=true;
        bool finished=false;
        for (anExplorer.Init(aShape,TopAbs_FACE);anExplorer.More();anExplorer.Next())
        {
            if (finished) break;
            for (aFaceExplorer.Init(anExplorer.Current(),TopAbs_EDGE);aFaceExplorer.More();aFaceExplorer.Next())
            {
                if (!firstrun && finished==false)
                {
                    for (unsigned int i=0;i<anEdgeList.size();++i)
                    {
                        if (anEdgeList[i].IsSame(TopoDS::Edge(aFaceExplorer.Current())))
                        {
                            second = TopoDS::Face(anExplorer.Current());
                            finished=true;
                            break;
                        }
                    }
                }
                if (firstrun)
                {
                    anEdgeList.push_back(TopoDS::Edge(aFaceExplorer.Current()));
                    first = TopoDS::Face(anExplorer.Current());
                }
            }
            firstrun=false;
        }
        //Uniformes Grid erzeugen und verschieben
        BRepAdaptor_Surface afirstFaceAdaptor(first);

        BRepAdaptor_Surface asecondFaceAdaptor(second);
        Standard_Real lUf,lVf,fUf,fVf,lUs,lVs,fUs,fVs,schrittweiteUf,schrittweiteVf,schrittweiteUs,schrittweiteVs;
        lUf = afirstFaceAdaptor.LastUParameter();
        fUf = afirstFaceAdaptor.FirstUParameter();
        lVf = afirstFaceAdaptor.LastVParameter();
        fVf = afirstFaceAdaptor.FirstVParameter();
        lUs = asecondFaceAdaptor.LastUParameter();
        fUs = asecondFaceAdaptor.FirstUParameter();
        lVs = asecondFaceAdaptor.LastVParameter();
        fVs = asecondFaceAdaptor.FirstVParameter();
        schrittweiteUf = fabs(lUf-fUf)/20.0;
        schrittweiteVf = fabs(lVf-fVf)/20.0;
        schrittweiteUs = fabs(lUs-fUs)/20.0;
        schrittweiteVs = fabs(lVs-fVs)/20.0;
        TColgp_Array2OfPnt Input(1,21,1,21);
        for (int j=0;j<=20;++j)
        {
            for (int k=0;k<=20;++k)
            {
                gp_Pnt currentPoint;
                gp_Vec UVec,VVec,Normal;
                afirstFaceAdaptor.D1((fUf+j*schrittweiteUf),(fVf+k*schrittweiteVf),currentPoint,UVec,VVec);
                UVec.Cross(VVec);
                Normal = UVec;
                Normal.Normalize();
                Normal.Multiply(20.0);
                gp_Pnt OffsetPoint;
                OffsetPoint.SetXYZ(currentPoint.XYZ()+Normal.XYZ());
                Input.SetValue(j+1,k+1,OffsetPoint);
            }
        }
        GeomAPI_PointsToBSplineSurface *Approx_Surface = new GeomAPI_PointsToBSplineSurface(Input, 3, 8, GeomAbs_C1,0.1);
        Handle(Geom_BSplineSurface) Final_Approx = Approx_Surface->Surface () ;
        //Jetzt die Wires vom ursprünglichen Face offsettieren.
        TopExp_Explorer asecondFaceExplorer;
        TopoDS_Wire aFaceWire;
        for (asecondFaceExplorer.Init(first,TopAbs_WIRE);asecondFaceExplorer.More();asecondFaceExplorer.Next())
        {
            aFaceWire = TopoDS::Wire(asecondFaceExplorer.Current());
        }
        WireExplorer awireexplorer(aFaceWire);
        //Punkte auf der Wire erzeugen und dann diese Punkte als Input in den Delaynay reinschieben
        BRepAdaptor_CompCurve2 aWireAdapter(aFaceWire);
        Standard_Real first_p,last_p,delta_u;
        last_p = aWireAdapter.LastParameter();
        first_p = aWireAdapter.FirstParameter();
        delta_u = fabs(last_p-first_p)/50;
        std::vector<Base::Vector3f> mesh_input;
        mesh_input.clear();
        for (int k=0;k<=50;++k)
        {
            gp_Pnt currentPoint = aWireAdapter.Value(first_p+(k*delta_u));
            Base::Vector3f aVector;
            aVector.x = float(currentPoint.X());
            aVector.y = float(currentPoint.Y());
            aVector.z = float(currentPoint.Z());
            mesh_input.push_back(aVector);
        }
        MeshCore::MeshKernel ameshkernel;
        MeshCore::MeshBuilder aBuilder(ameshkernel);
        std::vector<Base::Vector3f> mesh_output;
        //MeshCore::MeshPolygonTriangulation aTriangulator;
        //aTriangulator.SetPolygon(mesh_input);
        Base::Matrix4D matrix;
        //Base::Vector3f cPlaneNormal = aTriangulator.TransformToFitPlane(matrix);
        //aTriangulator.ComputeQualityDelaunay(20,mesh_output);
        std::vector<MeshCore::MeshGeomFacet> geomfacets;// = aTriangulator.GetTriangles();
        aBuilder.Initialize(100);


        for ( std::vector<MeshCore::MeshGeomFacet>::iterator kt = geomfacets.begin(); kt != geomfacets.end(); ++kt )
        {
            MeshCore::MeshGeomFacet atempfacet(matrix*(kt->_aclPoints[0]),matrix*(kt->_aclPoints[1]),matrix*(kt->_aclPoints[2]));
            atempfacet.CalcNormal();
            aBuilder.AddFacet(atempfacet);
        }
        aBuilder.Finish();
        MeshObject anObject(ameshkernel);
        return new MeshPy(&anObject);


        TopoDS_Wire newWire;
        for (awireexplorer.Init();awireexplorer.More();awireexplorer.Next())
        {
            TopoDS_Edge currentEdge = TopoDS::Edge(awireexplorer.Current());
            Standard_Real uf,ul,schrittweite_edge;
            Handle_Geom2d_Curve aPCurve = BRep_Tool::CurveOnSurface(currentEdge,first,uf,ul);
            schrittweite_edge = fabs(uf-ul)/10;
            std::vector<gp_Pnt> offsetPoints;
            offsetPoints.clear();
            for (int i=0;i<=10;++i)
            {
                gp_Pnt2d a2dPoint;
                aPCurve->D0(uf+i*schrittweite_edge,a2dPoint);
                //Koordinaten von der Surface holen
                gp_Pnt point;
                gp_Vec vecu,vecv,normal;
                afirstFaceAdaptor.D1(a2dPoint.X(),a2dPoint.Y(),point,vecu,vecv);
                vecu.Cross(vecv);
                normal = vecu;
                normal.Normalize();
                normal.Multiply(20.0);
                gp_Pnt OffsetPoint;
                OffsetPoint.SetXYZ(point.XYZ()+normal.XYZ());
                offsetPoints.push_back(OffsetPoint);
            }
            Handle(TColgp_HArray1OfPnt) MasterOffsetPoints = new TColgp_HArray1OfPnt(1, offsetPoints.size());
            for (unsigned int t=0;t<offsetPoints.size();++t)
            {
                MasterOffsetPoints->SetValue(t+1,offsetPoints[t]);
            }
            GeomAPI_Interpolate aBSplineInterp(MasterOffsetPoints, Standard_False, Precision::Confusion());
            aBSplineInterp.Perform();
            Handle_Geom_BSplineCurve currentOffsetCurve(aBSplineInterp.Curve());
        }
        BRepBuilderAPI_MakeFace Face(Final_Approx);

        //return new TopoShapePyOld(Face.Face());


    }PY_CATCH;

    Py_Return;
}

static PyObject * shape2orig(PyObject *self, PyObject *args)
{
    PyObject *pcObj;

    if (!PyArg_ParseTuple(args, "O!; Need one toposhape", &(TopoShapePy::Type), &pcObj))  // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY
    {
        GProp_GProps prop;
        GProp_PrincipalProps pprop;
        gp_Pnt orig;

        TopoShapePy *pcShape = static_cast<TopoShapePy*>(pcObj); //shape wird übergeben
        TopoDS_Shape cad           = pcShape->getTopoShapePtr()->_Shape;  // Input CAD

//        best_fit befi(cad);
        // befi.ShapeFit_Coarse();

        //     return new TopoShapePyOld(*(befi.m_Cad));

    }PY_CATCH;

    Py_Return;
}




static PyObject * spring_back(PyObject *self, PyObject *args)
{
    MeshPy   *pcObject;
    PyObject *pcObj;
    PyObject *pcObj2;

    if (!PyArg_ParseTuple(args, "O!O!; Need one Mesh objects and one toposhape", &(MeshPy::Type), &pcObj, &(TopoShapePy::Type), &pcObj2))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY
    {
        GProp_GProps prop;
        GProp_PrincipalProps pprop;
        gp_Pnt orig;

        pcObject  = (MeshPy*)pcObj;
        TopoShapePy *pcShape = static_cast<TopoShapePy*>(pcObj2); //Shape wird übergeben
        TopoDS_Shape cad              = pcShape->getTopoShapePtr()->_Shape;            // Input CAD
        MeshObject* anObject          = pcObject->getMeshObjectPtr();            // Input Mesh
        MeshCore::MeshKernel mesh     = anObject->getKernel();

        time_t seconds1, seconds2;
        seconds1 = time(NULL);

        /*-----------------------------------------------*/
        SpringbackCorrection aShapeTri(cad, mesh);
        aShapeTri.Perform(60,1);
        /*-----------------------------------------------*/

        seconds2 = time(NULL);
        cout << "laufzeit: " << seconds2-seconds1 << " sec" << endl;

        //anObject->setKernel(aShapeTri.m_Mesh);

        return new MeshPy(anObject);

    }PY_CATCH;

    Py_Return;
}







static PyObject * tess_shape(PyObject *self, PyObject *args)
{
    PyObject *pcObj;
    float aDeflection;
    //PyObject *pcObj2;
    if (!PyArg_ParseTuple(args, "O!f", &(TopoShapePy::Type), &pcObj, &aDeflection))    // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY
    {

        TopoShapePy *pcShape = static_cast<TopoShapePy*>(pcObj); //shape wird übergeben
        TopoDS_Shape cad        = pcShape->getTopoShapePtr()->_Shape;  // Input CAD

        //best_fit befi(cad);
        //befi.ShapeFit_Coarse();

        MeshCore::MeshKernel mesh;
        best_fit::Tesselate_Shape(cad, mesh, float(0.1));
        MeshObject* anObject = new MeshObject(mesh);
        return new MeshPy(anObject);

    }PY_CATCH;

    Py_Return;
}




















//static PyObject * trafo_mesh(PyObject *self, PyObject *args)
//{
// MeshPy   *pcObject;
//
//
// Base::Vector3f pnt;
//
// if (!PyArg_ParseTuple(args, "O!; Need one Mesh objects", &(MeshPy::Type), &pcObj))     // convert args: Python->C
//  return NULL;                             // NULL triggers exception
//
// pcObject  = (MeshPy*)pcObj;
//
// PY_TRY
// {
//        gp_Trsf trf;
//        trf.SetTranslation(trafo);
//
//  BRepBuilderAPI_Transform trsf(tsf);
//        trsf.Perform(cad);
//        TopoDS_Shape ResultShape = trsf.Shape();
//
//   return new MeshPy(mesh);
//
// }PY_CATCH;
//
// Py_Return;
//}
//
//static PyObject * trafo_cad(PyObject *self, PyObject *args)
//{
// MeshPy   *pcObject;
//
// Base::Vector3f pnt;
//
// if (!PyArg_ParseTuple(args, "O!; Need one TopoShape objects", &(MeshPy::Type), &pcObj))     // convert args: Python->C
//  return NULL;                             // NULL triggers exception
//
// pcObject  = (MeshPy*)pcObj;
//
// PY_TRY
// {
//        gp_Trsf trf;
//        trf.SetTranslation(trafo);
//
//  BRepBuilderAPI_Transform trsf(tsf);
//        trsf.Perform(cad);
//        TopoDS_Shape ResultShape = trsf.Shape();
//
//   return new MeshPy(mesh);
//
// }PY_CATCH;
//
// Py_Return;
//}

static PyObject * fit_iter(PyObject *self, PyObject *args)
{
    MeshPy   *pcObject;
    PyObject *pcObj;
    PyObject *pcObj2;

    if (!PyArg_ParseTuple(args, "O!O!; Need exatly one Mesh object", &(MeshPy::Type), &pcObj, &(TopoShapePy::Type), &pcObj2))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    TopoShapePy *pcShape = static_cast<TopoShapePy*>(pcObj2); //Surface wird übergeben
    TopoDS_Shape cad = pcShape->getTopoShapePtr()->_Shape;

    TopExp_Explorer Ex;
    Ex.Init(cad,TopAbs_FACE);  // initialisiere cad-geometrie (trimmed surface)

    pcObject = (MeshPy*)pcObj;

    Base::Builder3D log3d;
    gp_Dir pl_vec;
    gp_Lin lin;
    gp_Pnt pnt;
    Base::Vector3f tmp_pnt;
    IntCurvesFace_ShapeIntersector shp_int;

    std::vector< std::vector<double> > R(3, std::vector<double>(3,0.0)); // Rotationsmatrix
    double err = 1001;

    TopoDS_Face atopo_surface;

    MeshCore::MeshKernel mesh = pcObject->getMeshObjectPtr()->getKernel();

    PY_TRY
    {
        //Base::Vector3f Point[3];
        Base::Vector3f current_pnt;




        //const MeshCore::MeshFacetArray& Facets = mesh.GetFacets();
        //const MeshCore::MeshPointArray& Points = mesh.GetPoints();

        MeshCore::MeshPointIterator p_it(mesh);
        MeshCore::MeshFacetIterator f_it(mesh);
        MeshCore::MeshRefPointToFacets rf2pt(mesh);
        MeshCore::MeshGeomFacet t_face;

        int NumOfPoints = mesh.CountPoints();

        //int NumOfPoints = mesh.CountPoints();

        Base::Vector3f normal,local_normal;
        //float fArea = 0.0f;

        while (err > 1000)
        {

            err = 0.0;
            for (unsigned long i=0; i<mesh.CountPoints(); i++)
            {
                // Satz von Dreiecken zu jedem Punkt
                const std::set<unsigned long>& faceSet = rf2pt[i];
                float fArea = 0.0;
                normal.Set(0.0,0.0,0.0);


                // Iteriere über die Dreiecke zu jedem Punkt
                for (std::set<unsigned long>::const_iterator it = faceSet.begin(); it != faceSet.end(); ++it)
                {
                    // Einmal derefernzieren, um an das MeshFacet zu kommen und dem Kernel uebergeben, dass er ein MeshGeomFacet liefert
                    t_face = mesh.GetFacet(*it);
                    // Flaecheninhalt aufsummieren
                    float local_Area = t_face.Area();
                    local_normal = t_face.GetNormal();
                    if (local_normal.z < 0)
                    {
                        local_normal = local_normal * (-1);
                    }

                    fArea = fArea + local_Area;
                    normal = normal + local_normal;

                }

                pnt.SetX((mesh.GetPoint(i)).x);
                pnt.SetY((mesh.GetPoint(i)).y);
                pnt.SetZ((mesh.GetPoint(i)).z);

                lin.SetLocation(pnt);

                pl_vec.SetX(normal.x);
                pl_vec.SetY(normal.y);
                pl_vec.SetZ(normal.z);

                lin.SetDirection(pl_vec);

                shp_int.Load(cad, 0.1);
                shp_int.PerformNearest(lin,-RealLast(),+RealLast());

                if (shp_int.IsDone())
                {
                    err += sqrt((pnt.X() - shp_int.Pnt(1).X())*(pnt.X() - shp_int.Pnt(1).X())*
                                (pnt.Y() - shp_int.Pnt(1).Y())*(pnt.Y() - shp_int.Pnt(1).Y())*
                                (pnt.Z() - shp_int.Pnt(1).Z())*(pnt.Z() - shp_int.Pnt(1).Z()));


                    pnt = shp_int.Pnt(1);

                    tmp_pnt.x = float(pnt.X());
                    tmp_pnt.y = float(pnt.Y());
                    tmp_pnt.z = float(pnt.Z());

                    //point_list.push_back(pnt);
                    log3d.addSinglePoint(tmp_pnt,2,0,0,0);
                }
            }


            R[0][0] =  cos(PI);
            R[0][1] = -sin(PI);
            R[1][0] =  sin(PI);
            R[1][1] =  cos(PI);
            R[2][2] =  1;

            MeshCore::MeshPoint mpnt;

            for (int i=0; i<NumOfPoints; ++i)
            {
                mpnt = mesh.GetPoint(i);

                tmp_pnt.x = float(R[0][0]*mpnt.x + R[0][1]*mpnt.y + R[0][2]*mpnt.z);
                tmp_pnt.y = float(R[1][0]*mpnt.x + R[1][1]*mpnt.y + R[1][2]*mpnt.z);
                tmp_pnt.z = float(R[2][0]*mpnt.x + R[2][1]*mpnt.y + R[2][2]*mpnt.z);

                mesh.SetPoint(i,tmp_pnt);
            }

            break;

        } /*end while*/

        //// translation fit
        //// z-translation
        //while(traf_step > TOL)
        //{
        // traf += fl*traf_step;

        // err = 0;
        // for(int i=0; i<NumOfPoints; ++i)
        // {
        //  pnt_tmp[i][0] = pnt[i][0];
        //  pnt_tmp[i][1] = pnt[i][1];
        //  pnt_tmp[i][2] = pnt[i][2] + traf;

        //  err += weights[i]*sqrt( (pnt_tmp[i][0] - pnt_ref[i][0])*(pnt_tmp[i][0] - pnt_ref[i][0]) +
        //                             (pnt_tmp[i][1] - pnt_ref[i][1])*(pnt_tmp[i][1] - pnt_ref[i][1]) +
        //        (pnt_tmp[i][2] - pnt_ref[i][2])*(pnt_tmp[i][2] - pnt_ref[i][2]) );
        // }

        // traf -= fl*traf_step;
        //
        // if(err < err_tmp)
        // {
        //  err_tmp = err;
        //  traf += fl*traf_step;
        // }
        // else
        // {
        //  if(fl == -1)
        //  {
        //   traf_step /= 2;
        //   fl = 1;
        //  }
        //  else
        //      fl = -1;
        // }
        //}

        //for(int i=0; i<NumOfPoints; ++i)
        // pnt[i][2] += traf;
        //
        //
        //traf = 0;
        //traf_step = 1;
        //// end z-translation
    }PY_CATCH;

    log3d.saveToFile("c:/test_trim2.iv");
    MeshObject aObject(mesh);
    return new MeshPy(&aObject);
    Py_Return;
}


//PyDoc_STRVAR(open_doc,
//"open(string) -- Not implemented for this Module so far.");
//
//PyDoc_STRVAR(inst_doc,
//"insert(string, string) -- Not implemented for this Module so far.");
//
//PyDoc_STRVAR(loft_doc,
//"Creates a TopoShape with a test BSPLINE");
//
//PyDoc_STRVAR(useMesh,"useMesh(MeshObject) -- Shows the usage of Mesh objects from the Mesh Module.");


/* registration table  */
struct PyMethodDef Cam_methods[] =
{
    {"open"   , open,   Py_NEWARGS, "open(string) -- Not implemented for this Module so far."
    },
    {"insert" , insert, Py_NEWARGS, "insert(string, string) -- Not implemented for this Module so far."},
    {"read"   , read,  1},
    {"createTestBSPLINE"   , createTestBSPLINE,  Py_NEWARGS, "Creates a TopoShape with a test BSPLINE"},
    {"createTestApproximate" , createTestApproximate, 1},
    //  {"makeToolPath", makeToolPath, 1},
    {"offset", offset, 1},
// {"offset_mesh", offset_mesh, 1},
    {"tesselateShape",tesselateShape,1},
    {"tess_shape",tess_shape,1},
// {"cut", cut, 1},
    {"createPlane" , createPlane, 1},
    {"best_fit_test", best_fit_test,1},
    {"best_fit_complete", best_fit_complete,1},
    {"best_fit_coarse", best_fit_coarse ,1},
    {"createBox" , createBox, 1},
    {"spring_back", spring_back, 1},
    {"useMesh" , useMesh, Py_NEWARGS, "useMesh(MeshObject) -- Shows the usage of Mesh objects from the Mesh Module." },
    //{"MyApprox" , MyApprox, Py_NEWARGS, "MyApprox(MeshObject) -- My test approximate." },
    {"openDYNA" , openDYNA, Py_NEWARGS, "Open up a DYNA file, triangulate it, and returns a mesh"},
    {NULL     , NULL      }        /* end of table marker */
};

