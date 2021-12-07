/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
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

//Mesh Stuff
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/TopoAlgorithm.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/MeshPy.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/Core/Elements.h>
#include <Mod/Mesh/App/Core/Grid.h>


//FreeCAD Stuff
#include <Base/Builder3D.h>

//OCC Stuff
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <GCPnts_QuasiUniformAbscissa.hxx>
#include <Adaptor2d_Curve2d.hxx>
#include <Adaptor3d_HSurface.hxx>
#include <BRep_Tool.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TColStd_HArray1OfBoolean.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <BRepAlgo_Section.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Handle_Geom_Plane.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <Handle_TColStd_HArray1OfBoolean.hxx>
#include <BSplCLib.hxx>
#include <BRepBuilderAPI_NurbsConvert.hxx>

//Own Stuff
#include "cutting_tools.h"
#include "best_fit.h"
#include "edgesort.h"
#include "WireExplorer.h"
#include "BRepAdaptor_CompCurve2.h"
//#include "MeshInterface.h"







cutting_tools::cutting_tools(TopoDS_Shape aShape, float pitch)
        : m_Shape(aShape),
        m_aMeshAlgo(NULL),
        m_CAD_Mesh_Grid(NULL),
        m_pitch(pitch)
{
    m_ordered_cuts.clear();
    m_all_offset_cuts_high.clear();
    m_all_offset_cuts_low.clear();
    m_face_bboxes.clear();
    m_cad = false;
    m_CAD_Mesh.Clear();
    m_FaceWireMap.clear();
    m_MachiningOrder.clear();
    getShapeBB();
    fillFaceBBoxes();
    classifyShape();
    //checkFlatLevel();
    initializeMeshStuff();
    //BRepBuilderAPI_Sewing aSewer;
    //aSewer.Add(m_Shape);
    //aSewer.
    //Everything should be initialised now

}



cutting_tools::cutting_tools(TopoDS_Shape aShape)
        :m_Shape(aShape),m_aMeshAlgo(NULL),m_CAD_Mesh_Grid(NULL),m_cad(false),m_pitch(0.0)
{
    m_ordered_cuts.clear();
    m_all_offset_cuts_high.clear();
    m_all_offset_cuts_low.clear();
    m_face_bboxes.clear();
    m_CAD_Mesh.Clear();
    m_FaceWireMap.clear();
    m_MachiningOrder.clear();
    getShapeBB();
    fillFaceBBoxes();
    classifyShape();
    //checkFlatLevel();
    initializeMeshStuff();



}


cutting_tools::~cutting_tools()
{
    delete m_aMeshAlgo;
    delete m_CAD_Mesh_Grid;
}


bool cutting_tools::SetMachiningOrder(const TopoDS_Face &aFace, float x,float y,float z)
{
    std::pair<Base::Vector3f,TopoDS_Face> aTempPair;
    Base::Vector3f aPoint(x,y,z);
    aTempPair.first = aPoint;
    aTempPair.second = aFace;
    m_MachiningOrder.push_back(aTempPair);
    return true;
}


bool cutting_tools::fillFaceWireMap()
{
    std::vector<std::pair<Base::Vector3f,TopoDS_Face> >::iterator MOrderIt;
    for (MOrderIt = m_MachiningOrder.begin();MOrderIt != m_MachiningOrder.end(); ++MOrderIt)
    {
        //Here we take the flat areas and put them in a map where we can easily search for the biggest, smallest Wire
        std::pair<TopoDS_Face,std::map<Base::BoundBox3f,TopoDS_Wire,BoundBox3f_Less> >aTempPair;
        aTempPair.second.clear();
        std::pair<Base::BoundBox3f,TopoDS_Wire> aTempBBoxPair;
        aTempPair.first = MOrderIt->second;
        //Jetzt durch das Face gehen und die Wires rausfiltern
        TopExp_Explorer Explore_Face;
        Explore_Face.Init(MOrderIt->second,TopAbs_WIRE);
        //If there is no Wire -> return
        if (!Explore_Face.More()) return false;

        //Jetzt alle Wires in die map schieben
        for (Explore_Face.ReInit();Explore_Face.More();Explore_Face.Next())
        {
            aTempBBoxPair.first = getWireBBox(TopoDS::Wire(Explore_Face.Current()));
            aTempBBoxPair.second = TopoDS::Wire(Explore_Face.Current());
            aTempPair.second.insert(aTempBBoxPair);
        }
        m_FaceWireMap.insert(aTempPair);
    }
    return true;
}




bool cutting_tools::getShapeBB()
{
    //Die Cascade-Bounding Box funktioniert nicht richtig
    //Es wird dort wohl ne BoundingBox um dasKontrollnetz gelegt
    //Deshalb wird jetzt kurz das Shape tesseliert und dann die Bounding Box direkt ausgelesen
    best_fit::Tesselate_Shape(m_Shape,m_CAD_Mesh,float(0.1));
    Base::BoundBox3f aBoundBox = m_CAD_Mesh.GetBoundBox();
    m_maxlevel = aBoundBox.MaxZ;
    m_minlevel = aBoundBox.MinZ;

 //   //Hier testen wir noch ein paar OpenMesh Funktionen
 //   typedef OpenMesh::DefaultTraits MyTraits;
 //   typedef OpenMesh::TriMesh_ArrayKernelT<MyTraits>  MyMesh;

	//////// Mesh::Interface erbt von MyMesh , so dass das Objekt 'mesh' direkt in den OpenMesh-Funktionen genutzt werden kann
 //  Mesh::Interface<MyMesh> mesh(m_CAD_Mesh);
	////// proceed with mesh, e.g. refining
 //  MyMesh::EdgeIter e_it,e_end(mesh.edges_end());
 //  MyMesh::EdgeHandle eh;
 //  MyMesh::Scalar dist;
 //  MyMesh::Point startPoint,midPoint,endPoint;

 //  for ( e_it = mesh.edges_begin() ; e_it != e_end ; ++e_it )
 //  {
 //       eh = e_it.handle();
 //       dist = mesh.calc_edge_length(eh);  //get the length of the current edge
 //       if(dist>2.6) //Split the Edge now
 //       {
 //        
 //           //Get the Start and EndPoints of the Edge
 //          
 //       }

 //   }
 ////// throw away all tagged edges
 ////mesh.garbage_collection(); //Function from OpenMesh
 //mesh.release(m_CAD_Mesh);    //Function from MeshInterface to convert the Input Kernel back


   
    /* Hier ist die alte OCC BoundingBox Funktion
    Bnd_Box currentBBox;
    Standard_Real XMin, YMin, ZMin, XMax, YMax, ZMax;
    BRepBndLib::Add(m_shape, currentBBox );
       currentBBox.SetGap(1000.0);
       currentBBox.Get(XMin, YMin, ZMin, XMax, YMax, ZMax);
    */
    return true;
}


//bool cutting_tools::fillFaceMeshMap()
//{
// if(m_CAD_Mesh.size()>





bool cutting_tools::fillFaceBBoxes()
{
    TopoDS_Face atopo_surface;
    TopExp_Explorer Explorer;
    MeshCore::MeshKernel aFaceMesh;
    Base::BoundBox3f aBoundBox;
    Explorer.Init(m_Shape,TopAbs_FACE);
    for (;Explorer.More();Explorer.Next())
    {
        aFaceMesh.Clear();
        aBoundBox.Flush();
        atopo_surface = TopoDS::Face (Explorer.Current());
        best_fit::Tesselate_Face(atopo_surface,aFaceMesh,float(0.1));
        aBoundBox = aFaceMesh.GetBoundBox();
        aBoundBox.Enlarge(2.0);
        std::pair<TopoDS_Face,Base::BoundBox3f> tempPair;
        tempPair.first = atopo_surface;
        tempPair.second = aBoundBox;
        m_face_bboxes.push_back(tempPair);
    }
    return true;
}
//Hier ist die alte Version um die Bounding Box zu bestimmen
//      aAdaptor_Surface.Initialize(atopo_surface);
//      Standard_Real FirstUParameter, LastUParameter,FirstVParameter,LastVParameter;
//      gp_Pnt aSurfacePoint;
//   FirstUParameter = aAdaptor_Surface.FirstUParameter();
//      LastUParameter  = aAdaptor_Surface.LastUParameter();
//      FirstVParameter = aAdaptor_Surface.FirstVParameter();
//      LastVParameter = aAdaptor_Surface.LastVParameter();
//float urange = LastUParameter - FirstUParameter;
//float vrange = LastVParameter - FirstVParameter;
////Jetzt ein 10x10 Grid pro Face machen und so die BBox bestimmen
////Zunächst mal die BoundingBox auf Null setzen
//Bnd_Box currentBBox;
//
//for(int i=0;i<10;++i)
//{
// for(int j=0;j<10;++j)
// {
//  aAdaptor_Surface.D0(FirstUParameter+((urange/10)*i),FirstVParameter+((vrange/10)*j),aSurfacePoint);
//  currentBBox.Update(aSurfacePoint.X(),aSurfacePoint.Y(),aSurfacePoint.Z());
// }
//}
////Jetzt noch nen Rand drumherum bauen
//currentBBox.SetGap(0.5);
////Jetzt die aktuelle BoundingBox mit dem aktuellen Face in einen vector pushen
//std::pair<TopoDS_Face,Bnd_Box> tempPair;
//tempPair.first = atopo_surface;
//tempPair.second = currentBBox;
//m_face_bboxes.push_back(tempPair);
////aVertex = BRepBuilderAPI_MakeVertex(aSurfacePoint);
////BRepBndLib::Add(aVertex, currentBBox );
//currentBBox.SetVoid();


bool cutting_tools::checkPointinFaceBB(const gp_Pnt &aPnt,const Base::BoundBox3f &aBndBox)
{
    if ((aPnt.X()>aBndBox.MinX) && (aPnt.X()<aBndBox.MaxX) && (aPnt.Y()>aBndBox.MinY) && (aPnt.Y()<aBndBox.MaxY) && (aPnt.Z()>aBndBox.MinZ) && (aPnt.Z()<aBndBox.MaxZ))
        return true;

    return false;
}


bool cutting_tools::initializeMeshStuff()
{
    m_CAD_Mesh_Grid = new MeshCore::MeshFacetGrid(m_CAD_Mesh);
    m_aMeshAlgo = new MeshCore::MeshAlgorithm(m_CAD_Mesh);
    return true;
}


bool cutting_tools::arrangecuts_ZLEVEL()
{
    //We have to fill the required maps first
    fillFaceWireMap();
    //Zunächst wieder checken ob CAD oder nicht
    if (m_cad==false)
    {
        //Cast um die Nachkommastellen wegzuschneiden
        int cutnumber = (int)fabs((m_maxlevel-m_minlevel)/m_pitch);
        //m_pitch leicht korrigieren um wirklich auf die letzte Ebene zu kommen
        m_pitch = fabs(m_maxlevel-m_minlevel)/cutnumber;
        //Jetzt die Schnitte machen. Die höchste Ebene fällt weg, da hier noch kein Blech gedrückt wird
        float z_level,z_level_corrected;
        TopoDS_Shape aCutShape;
        for (int i=1;i<=cutnumber;++i)
        {
            //Jetzt schneiden (die oberste Ebene auslassen)
            z_level = m_maxlevel-(i*m_pitch);
            z_level_corrected = z_level;
            cut(z_level,m_minlevel,aCutShape,z_level_corrected);
            //cut_Mesh(z_level,m_minlevel,result,z_level_corrected);
            //Jetzt die resultierende Wire in einen Vector pushen
            std::pair<float,TopoDS_Shape> tempPair;
            tempPair.first = z_level_corrected;
            tempPair.second = aCutShape;
            m_ordered_cuts.push_back(tempPair);
        }
        return true;
    }
    //Wenn wir mehrere Faces haben oder eine CAD-Geometrie vorhanden ist
    else
    {
        //Über die MachiningOrder wird jetzt die Cutting-Folge festgelegt
        std::vector<std::pair<Base::Vector3f,TopoDS_Face> >::iterator MOrderIt;
        if (m_MachiningOrder.size()<2) return false; //Did not select at least two Levels
        //Now take two levels and perform the Cutting Stuff
        for (MOrderIt = m_MachiningOrder.begin();MOrderIt != m_MachiningOrder.end(); ++MOrderIt)
        {
            float temp_max = MOrderIt->first.z;
            //the check if MOrderIt+1 != end is performed at the bottom of the function
            float temp_min = (MOrderIt+1)->first.z;
            //set the direction flags
            if (temp_max> temp_min)
                m_direction = true;
            else m_direction = false;
            //Now we cut from temp_max to temp_min, without the flat areas at the top and bottom

            int cutnumber = (int)fabs((temp_max-temp_min)/m_UserSettings.level_distance);
            //m_pitch correction to really reach temp_min
            m_UserSettings.level_distance = fabs(temp_max-temp_min)/cutnumber;

            float z_level,z_level_corrected;
            TopoDS_Shape aCutShape;
            //Now lets cut and push the highest and lowest level also into the results vector
            std::pair<float,TopoDS_Shape> tempPair;
            //Highest Level push_back (only the proper Wire)

            tempPair.first = MOrderIt->first.z;
            //get the Wire with the smallest Bounding Box if we go from top to bottom for the highest area
            tempPair.second = m_FaceWireMap.find(MOrderIt->second)->second.begin()->second;

            m_ordered_cuts.push_back(tempPair);
            for (int i=1;i<cutnumber;++i)
            {
                if (m_direction)
                    z_level = temp_max-(i*m_UserSettings.level_distance);
                else
                    z_level = temp_max+(i*m_UserSettings.level_distance);
                z_level_corrected = z_level;
                cut(z_level,temp_min, aCutShape,z_level_corrected);
                if (z_level_corrected != z_level)
                    std::cout << "Somehow we couldn't cut" << std::endl;
                //Jetzt nur das gewünschte Resultat in den vector schieben (von oben nach unten große usw.)
                Edgesort aCuttingShapeSorter(aCutShape);
                tempPair.first = z_level_corrected;
                if (m_direction)
                    tempPair.second = aCuttingShapeSorter.GetDesiredCutShape(2);//With an Index !=1 we get the biggest one
                else
                    tempPair.second = aCuttingShapeSorter.GetDesiredCutShape(1);
                m_ordered_cuts.push_back(tempPair);
            }
            //Now push the lowest level into the ordered_cuts_vector.
            //if there is no more area to cut take the biggest wire of the bottom face
            //otherwise take the biggest wire but continue
            if (MOrderIt+2 == m_MachiningOrder.end())
            {
                tempPair.first = (MOrderIt+1)->first.z;
                tempPair.second = m_FaceWireMap.find((MOrderIt+1)->second)->second.rbegin()->second;
                m_ordered_cuts.push_back(tempPair);
                break; //No more combination to find
            }
            else
            {
                tempPair.first = (MOrderIt+1)->first.z;
                tempPair.second = m_FaceWireMap.find((MOrderIt+1)->second)->second.rbegin()->second;
                m_ordered_cuts.push_back(tempPair);
            }
        }
    }
    return true;
}








//}
//      std::map<float,std::map<Base::BoundBox3f,TopoDS_Wire,BoundBox3f_Less> >::iterator zl_wire_it;
//      //Wir holen uns jetzt den nächsten Z-Level raus. Wir müssen was kleineres
//      //wie den höchsten Wert nehmen sonst gibt er immer den höchsten Wert aus
//      zl_wire_it = m_zl_wire_combination.upper_bound(temp_max-0.1);
//      if (zl_wire_it->first == temp_max)
//      {
//          cout << "Tja, es gibt wohl nur eine flache Area";
//          temp_min = m_minlevel;
//      }
//      //Wenn es mehrere flache Bereiche gibt muss ich nochmal weitermachen
//      else
//      {
//          temp_min = zl_wire_it->first;
//          cout << "Mehrere Areas erkannt";
//      }
//      //Jetzt schnippeln von temp_max bis temp_min
//      int cutnumber = (int)fabs((temp_max-temp_min)/m_pitch);
//      //m_pitch leicht korrigieren um wirklich auf die letzte Ebene zu kommen
//      m_pitch = fabs(temp_max-temp_min)/cutnumber;
//      //Jetzt die Schnitte machen. Die höchste Ebene fällt weg, da hier noch kein Blech gedrückt wird
//      float z_level,z_level_corrected;
//      TopoDS_Shape aCutShape;
//      //Jetzt schneiden (die oberste Ebene auslassen)
//      for (int i=1;i<=20;++i)
//      {
//          z_level = temp_max-(i*m_pitch);
//          z_level_corrected = z_level;
//          //cut_Mesh(z_level,m_minlevel,result,z_level_corrected);
//          //Jetzt die resultierenden Points in den vector schieben
//          //std::pair<float,std::list<std::vector<Base::Vector3f> > > tempPair;
//          //tempPair.first = z_level_corrected;
//          //tempPair.second = result;
//          //m_ordered_cuts.push_back(tempPair);
//          cut(z_level,temp_min, aCutShape,z_level_corrected);
//          //Jetzt die gefüllte Wire in den vector schieben
//          std::pair<float,TopoDS_Shape> tempPair;
//          tempPair.first = z_level_corrected;
//          tempPair.second = aCutShape;
//          m_ordered_cuts.push_back(tempPair);
//      }
//      return true;
//  }
//    return false;
//}



//bool cutting_tools::checkFlatLevel()
//{
//    //Falls keine CAD-Geometrie da ist, gleich wieder rausspringen
//
//    if (m_cad==false) return false;
//
//    TopoDS_Face atopo_surface;
//    BRepAdaptor_Surface aAdaptor_Surface;
//    TopExp_Explorer Explorer;
//
//
//    for (Explorer.Init(m_Shape,TopAbs_FACE);Explorer.More();Explorer.Next())
//    {
//        atopo_surface = TopoDS::Face (Explorer.Current());
//        aAdaptor_Surface.Initialize(atopo_surface);
//        Standard_Real FirstUParameter, LastUParameter,FirstVParameter,LastVParameter;
//        gp_Pnt first,second,third;
//        gp_Vec first_u,first_v,second_u,second_v,third_u,third_v, Norm_first,Norm_second,Norm_third,Norm_average;
//        double u_middle,v_middle;
//        /*
//        Generate three random point on the surface to get the surface normal and decide whether it's a
//        planar face or not
//        */
//        FirstUParameter = aAdaptor_Surface.FirstUParameter();
//        LastUParameter  = aAdaptor_Surface.LastUParameter();
//        FirstVParameter = aAdaptor_Surface.FirstVParameter();
//        LastVParameter = aAdaptor_Surface.LastVParameter();
//        u_middle = sqrt((FirstUParameter - LastUParameter)*(FirstUParameter - LastUParameter))/2;
//        v_middle = sqrt((FirstVParameter - LastVParameter)*(FirstVParameter - LastVParameter))/2;
//        aAdaptor_Surface.D1(sqrt((FirstUParameter-u_middle)*(FirstUParameter-u_middle))/2,sqrt((FirstVParameter-v_middle)*(FirstVParameter-v_middle))/2,first,first_u,first_v);
//        aAdaptor_Surface.D1(sqrt((u_middle)*(u_middle))/2,sqrt((v_middle)*(v_middle))/2,second,second_u,second_v);
//        aAdaptor_Surface.D1(sqrt((FirstUParameter+u_middle)*(FirstUParameter+u_middle))/2,sqrt((FirstVParameter+v_middle)*(FirstVParameter+v_middle))/2,third,third_u,third_v);
//        //Get Surface normal as Cross-Product between two Vectors
//        Norm_first = first_u.Crossed(first_v);
//        Norm_first.Normalize();
//        Norm_second = second_u.Crossed(second_v);
//        Norm_second.Normalize();
//        Norm_third = third_u.Crossed(third_v);
//        Norm_third.Normalize();
//        //Evaluate average normal vector
//        Norm_average.SetX((Norm_first.X()+Norm_second.X()+Norm_third.X())/3);
//        Norm_average.SetY((Norm_first.Y()+Norm_second.Y()+Norm_third.Y())/3);
//        Norm_average.SetZ((Norm_first.Z()+Norm_second.Z()+Norm_third.Z())/3);
//        Norm_average.Normalize();
//        gp_Vec z_normal(0,0,1);
//        gp_Vec z_normal_opposite(0,0,-1);
//        if (Norm_average.IsEqual(z_normal,0.01,0.01) || Norm_average.IsEqual(z_normal_opposite,0.01,0.01))
//        {
//            cout << "Einen flachen Bereich gefunden";
//            std::pair<float,std::map<Base::BoundBox3f,TopoDS_Wire,BoundBox3f_Less> >aTempPair;
//            aTempPair.second.clear();
//            std::pair<Base::BoundBox3f,TopoDS_Wire> aTempBBoxPair;
//
//            //Z-Wert vom flachen Bereich in ein temporäres pair pushen
//            aTempPair.first = ((first.Z()+second.Z()+third.Z())/3);
//            //Jetzt durch das Face gehen und die Wires rausfiltern
//            TopExp_Explorer Explore_Face;
//            Explore_Face.Init(atopo_surface,TopAbs_WIRE);
//            //If there is no Wire -> return
//            if (!Explore_Face.More()) return false;
//
//            //Jetzt alle Wires in die map schieben
//            for (Explore_Face.ReInit();Explore_Face.More();Explore_Face.Next())
//            {
//                aTempBBoxPair.first = getWireBBox(TopoDS::Wire(Explore_Face.Current()));
//                aTempBBoxPair.second = TopoDS::Wire(Explore_Face.Current());
//                aTempPair.second.insert(aTempBBoxPair);
//                //aTempPair.first ist ja noch auf dem gleichen Z-Wert wie vorher, deswegen muss da nichts abgepasst werden
//            }
//            m_zl_wire_combination.insert(aTempPair);
//        }
//    }
//
//    return true;
//}

//bool cutting_tools::projectWireToSurface(const TopoDS_Wire &aWire,const TopoDS_Shape &aShape,std::vector<projectPointContainer> &aContainer);
//{
// //make your wire looks like a curve to other algorithm and generate Points to offset the curve
// aContainer.clear();
// BRepAdaptor_CompCurve2 wireAdaptor(aWire);
// GCPnts_QuasiUniformDeflection aProp(wireAdaptor,0.01);
// int numberofpoints = aProp.NbPoints();
// Standard_Real Umin,Vmin,lowestdistance;
// TopoDS_Face atopo_surface,atopo_surface_shortest;
// Handle_Geom_Surface geom_surface;
//
// //Now project the points to the surface and get surface normal.
// for (int i=1;i<=numberofpoints;++i)
// {
//  lowestdistance=200;
//  //Aktuellen Punkt holen
//  gp_Pnt currentPoint = aProp.Value(i);
//  projectPointContainer aTempContainer;
//  //checken auf welches Face wir projezieren könnnen
//  for(m_face_bb_it = m_face_bboxes.begin();m_face_bb_it!=m_face_bboxes.end();++m_face_bb_it)
//  {
//   //Wenn der aktuelle Punkt in der BBox enthalten ist, dann machen wir mit der Projection weiter
//   if(checkPointinFaceBB(aProp.Value(i),m_face_bb_it->second))
//   {
//    atopo_surface = m_face_bb_it->first;
//    geom_surface = BRep_Tool::Surface(atopo_surface);
//    GeomAPI_ProjectPointOnSurf aPPS(currentPoint,geom_surface,0.001);
//    //Wenn nichts projeziert werden kann, gehts gleich weiter zum nächsten Face bzw. der nächsten BBox
//    if (aPPS.NbPoints() == 0) continue;
//    //Jetzt muss das aktuelle Face gespeichert werden, da es eventuell das face ist, welches am nächsten ist
//    double length = aPPS.LowerDistance();
//    if(lowestdistance>length)
//    {
//     lowestdistance=length;
//     atopo_surface_shortest = atopo_surface;
//     aPPS.LowerDistanceParameters (Umin,Vmin);
//    }
//   }
//  }
//  gp_Vec Uvec,Vvec,normalVec;
//  geom_surface = BRep_Tool::Surface(atopo_surface_shortest);
//  //Das Face welches am nächsten ist in der temp-struct speichern
//  aTempContainer.face = atopo_surface_shortest;
//  geom_surface->D1(Umin,Vmin,aTempContainer.point,Uvec,Vvec);
//  //Jetzt den Normalenvector auf die Fläche ausrechnen
//  normalVec = Uvec;
//  normalVec.Cross(Vvec);
//  normalVec.Normalize();
//  //Jetzt ist die Normale berechnet und auch normalisiert
//  //Jetzt noch checken ob die Normale auch wirklich wie alle anderen auf die gleiche Seite zeigt.
//  //dazu nur checken ob der Z-Wert der Normale größer Null ist (dann im 1.und 2. Quadranten)
//  if(normalVec.Z()<0) normalVec.Multiply(-1.0);
//  //Mal kurz den Winkel zur Grund-Ebene ausrechnen
//  aTempContainer.normalvector = normalVec;
//  aContainer.push_back(aTempContainer);
//
// }
// return true;
//}


TopoDS_Wire cutting_tools::ordercutShape(const TopoDS_Shape &aShape)
{
    //Bisher funktioniert das Ganze nur für Schnitte welche nur einmal rundherum laufen und noch nicht für Inseln
    TopExp_Explorer exploreShape;
    exploreShape.Init(aShape,TopAbs_EDGE);
    int k=0;
    for (; exploreShape.More(); exploreShape.Next()) //erstmal schauen wieviele Edges wir haben
    {
        k++;
    }
    //Jetzt die Edges alle in eine Wire packen (mit Add) und schauen ob ein Fehler kommt.
    //Wenn ja, dann die nächste Edge. Solange bis alle Edges drin sind.

    if (k<1)
    {
        exploreShape.ReInit();
        BRepBuilderAPI_MakeWire mkWire(TopoDS::Edge(exploreShape.Current()));
        return mkWire.Wire();
    }
    else //Nur wenn mehr als eine Edge vorhanden ist
    {
        BRepBuilderAPI_MakeWire mkWire; //WireContainer aufbauen
        std::vector<edge_container> listofedge_tmp,listofedge;
        std::vector<edge_container>::iterator it_edge;
        edge_container a_edge_container; //Definiert in der stuff.h
        listofedge.clear();
        listofedge_tmp.clear();
        exploreShape.ReInit();
        //Edge-Liste füllen
        for (; exploreShape.More(); exploreShape.Next())
        {
            a_edge_container.edge = TopoDS::Edge(exploreShape.Current());
            TopoDS_Vertex V1,V2;
            TopExp::Vertices(a_edge_container.edge,V1,V2);
            a_edge_container.firstPoint = BRep_Tool::Pnt(V1);;
            a_edge_container.lastPoint = BRep_Tool::Pnt(V2);
            listofedge.push_back(a_edge_container);
        }

        gp_Pnt lastpointoflastedge,firstpointoflastedge;
        while (listofedge.empty() == false )
        {
            listofedge_tmp.clear();
            for (it_edge = listofedge.begin();it_edge!=listofedge.end();++it_edge)
            {
                mkWire.Add((*it_edge).edge);
                if (mkWire.IsDone())
                {
                    lastpointoflastedge = (*it_edge).lastPoint;
                    firstpointoflastedge = (*it_edge).firstPoint;
                }
                else
                {
                    //Abstände ausrechnen
                    double abstand1=sqrt((*it_edge).firstPoint.SquareDistance(lastpointoflastedge));
                    double abstand2=sqrt((*it_edge).lastPoint.SquareDistance(lastpointoflastedge));
                    double abstand3=sqrt((*it_edge).firstPoint.SquareDistance(firstpointoflastedge));
                    double abstand4=sqrt((*it_edge).lastPoint.SquareDistance(firstpointoflastedge));
                    if (abstand1<0.5)
                    {
                        //Neue Edge erzeugen welche vom letzten Endpunkt zum aktuellen Startpunkt geht
                        BRepBuilderAPI_MakeEdge newedge(lastpointoflastedge,(*it_edge).firstPoint);
                        mkWire.Add(newedge);
                        mkWire.Add((*it_edge).edge);
                        lastpointoflastedge = (*it_edge).lastPoint;
                        firstpointoflastedge = (*it_edge).firstPoint;
                    }
                    else if (abstand2<0.5)
                    {
                        //Neue Edge erzeugen welche vom letzten Endpunkt zum aktuellen Startpunkt geht
                        BRepBuilderAPI_MakeEdge newedge(lastpointoflastedge,(*it_edge).lastPoint);
                        mkWire.Add(newedge);
                        mkWire.Add((*it_edge).edge);
                        lastpointoflastedge = (*it_edge).lastPoint;
                        firstpointoflastedge = (*it_edge).firstPoint;

                    }

                    else if (abstand3<0.5)
                    {
                        //Neue Edge erzeugen welche vom letzten Endpunkt zum aktuellen Startpunkt geht
                        BRepBuilderAPI_MakeEdge newedge(firstpointoflastedge,(*it_edge).firstPoint);
                        mkWire.Add(newedge);
                        mkWire.Add((*it_edge).edge);
                        lastpointoflastedge = (*it_edge).lastPoint;
                        firstpointoflastedge = (*it_edge).firstPoint;
                    }
                    else if (abstand4<0.5)
                    {
                        //Neue Edge erzeugen welche vom letzten Endpunkt zum aktuellen Startpunkt geht
                        BRepBuilderAPI_MakeEdge newedge(firstpointoflastedge,(*it_edge).lastPoint);
                        mkWire.Add(newedge);
                        mkWire.Add((*it_edge).edge);
                        lastpointoflastedge = (*it_edge).lastPoint;
                        firstpointoflastedge = (*it_edge).firstPoint;
                    }
                    else
                    {
                        listofedge_tmp.push_back(*it_edge);
                    }
                }
            }
            listofedge = listofedge_tmp;
        }
        return mkWire.Wire();
    }

}


//bool cutting_tools::OffsetWires_Standard(float radius) //Version wo nur in X,Y-Ebene verschoben wird
//{
// Base::Builder3D build;
// std::ofstream outfile;
// outfile.open("c:/atest.out");
//
//
// //Die ordered_cuts sind ein Vector wo für jede Ebene ein Pair existiert
// for(m_ordered_cuts_it = m_ordered_cuts.begin();m_ordered_cuts_it!=m_ordered_cuts.end();++m_ordered_cuts_it)
// {
//  float current_z_level = m_ordered_cuts_it->first;
//  std::vector<gp_Pnt> finalPoints;
//  finalPoints.clear();
//  Standard_Real Umin,Vmin,lowestdistance;
//  TopoDS_Face atopo_surface,atopo_surface_shortest;
//  Handle_Geom_Surface geom_surface;
//  int i=0;
//  for (;avector_it!=aPolyline_it->end();++avector_it)
//  {
//   i++;
//   lowestdistance=200;
//   //Aktuellen Punkt holen
//   gp_Pnt currentPoint(avector_it->x,avector_it->y,avector_it->z);
//   gp_Pnt nearest_Point;
//   //checken auf welches Face wir projezieren könnnen
//   for(m_face_bb_it = m_face_bboxes.begin();m_face_bb_it!=m_face_bboxes.end();++m_face_bb_it)
//   {
//    //Wenn der aktuelle Punkt in der BBox enthalten ist, dann machen wir mit der Projection weiter
//    if(checkPointinFaceBB(currentPoint,m_face_bb_it->second))
//    {
//     atopo_surface = m_face_bb_it->first;
//     geom_surface = BRep_Tool::Surface(atopo_surface);
//     GeomAPI_ProjectPointOnSurf aPPS(currentPoint,geom_surface,0.001);
//     //Wenn nichts projeziert werden kann, gehts gleich weiter zum nächsten Face bzw. der nächsten BBox
//     if (aPPS.NbPoints() == 0) continue;
//     //Jetzt muss das aktuelle Face gespeichert werden, da es eventuell das face ist, welches am nächsten ist
//     double length = aPPS.LowerDistance();
//     if(lowestdistance>length)
//     {
//      lowestdistance=length;
//      atopo_surface_shortest = atopo_surface;
//      //aPPS.LowerDistanceParameters (Umin,Vmin);
//      nearest_Point = aPPS.NearestPoint();
//     }
//    }
//   }
//   //Für eine saubere Projection auf der aktuellen Ebene wird jetzt der Richtungsvector
//   //aus dem projezierten Punkt und dem Ursprungspunkt gebildet
//   //und dieser dann hergenommen um damit nochmal in dessen Richtung zu projezieren
//   gp_Vec aVec(currentPoint,nearest_Point);
//   aVec.Normalize();
//   aVec.SetZ(0.0);
//   gp_Dir aDir(aVec);
//   gp_Lin aLine(currentPoint,aDir);
//   IntCurvesFace_ShapeIntersector aFaceIntSect;
//   aFaceIntSect.Load(m_Shape, 0.001);
//   aFaceIntSect.PerformNearest(aLine,-RealLast(), +RealLast());
//   //Jetzt holen wir uns auf der Fläche den U und V Wert um dann D1 und D2 bestimmen zu können
//   gp_Pnt projectedPoint,OffsetPoint;
//   gp_Pnt testpoint,testpoint2;
//   float abstand,abstand_old;
//   int number = aFaceIntSect.NbPnt();
//   abstand_old = 100;
//   for(int k=0;k<number;++k)
//   {
//    testpoint = aFaceIntSect.Pnt(k+1);
//    abstand = testpoint.Distance(currentPoint);
//    if(abstand<abstand_old)
//    {
//     Umin = aFaceIntSect.UParameter(k+1);
//     Vmin = aFaceIntSect.VParameter(k+1);
//     atopo_surface_shortest = aFaceIntSect.Face(k+1);
//     abstand_old = abstand;
//    }
//   }
//   gp_Vec Uvec,Vvec,normalVec,projPointVec,z_normale;
//   geom_surface = BRep_Tool::Surface(atopo_surface_shortest);
//   geom_surface->D1(Umin,Vmin,projectedPoint,Uvec,Vvec);
//   normalVec = Uvec;
//   normalVec.Cross(Vvec);
//   normalVec.Normalize();
//   //Jetzt ist die Surface-Normale berechnet und auch normalisiert
//   //Jetzt noch checken ob die Normale auch wirklich wie alle anderen auf die gleiche Seite zeigt.
//   //dazu nur checken ob der Z-Wert der Normale größer Null ist (dann im 1.und 2. Quadranten)
//   if(normalVec.Z()<0) normalVec.Multiply(-1.0);
//   //Jetzt die Normale auf die Radiuslänge verlängern
//   normalVec.Multiply(radius);
//   //Jetzt die Z-Komponente auf 0 setzen
//   normalVec.SetZ(0.0);
//   //float abstand = currentPoint.Distance(projectedPoint);
//   //if(abstand>0.2)
//   //{cout<<"error"<<endl;}
//   projPointVec.SetXYZ(projectedPoint.XYZ());
//   OffsetPoint.SetXYZ((projPointVec + normalVec).XYZ());
//   OffsetPoint.SetZ(projectedPoint.Z()+radius);//Den Radius noch dazu addieren
//   //Aktuellen OffsetPoint setzen
//   finalPoints.push_back(OffsetPoint);
//
//   //Base::Vector3f offsetPoint,projectPoint;
//   //offsetPoint.x=OffsetPoint.X();offsetPoint.y=OffsetPoint.Y();offsetPoint.z=OffsetPoint.Z();
//   //projectPoint.x=projectedPoint.X();projectPoint.y=projectedPoint.Y();projectPoint.z=projectedPoint.Z();
//   //build.addSingleArrow(projectPoint,offsetPoint);
//   build.addSinglePoint(OffsetPoint.X(),OffsetPoint.Y(),OffsetPoint.Z());
//   outfile << currentPoint.X() <<","<<currentPoint.Y()<<","<<currentPoint.Z()<<","<< projectedPoint.X() <<","<<projectedPoint.Y()<<","<<projectedPoint.Z()<<","<< OffsetPoint.X() <<","<<OffsetPoint.Y()<<","<<OffsetPoint.Z()<<","<<normalVec.X() <<","<<normalVec.Y()<<","<<normalVec.Z()<< std::endl;
//  }
//
//
// //  outfile << projectedPoint.X() <<","<<projectedPoint.Y()<<","<<projectedPoint.Z()<<std::endl;
//   //Jetzt die aktuelle Kurve als BSpline interpolieren
//  //check for intersections due to wrong offset points
//  checkPointIntersection(finalPoints);
//  std::vector<gp_Pnt> finalPointscorrected;
//  finalPointscorrected.clear();
//  checkPointDistance(finalPoints,finalPointscorrected);
//  Handle(TColgp_HArray1OfPnt) finalOffsetPoints = new TColgp_HArray1OfPnt(1, finalPointscorrected.size());
//  for(int t=0;t<finalPointscorrected.size();++t)
//  {
//   finalOffsetPoints->SetValue(t+1,finalPointscorrected[t]);
//  }
//  GeomAPI_Interpolate aNoPeriodInterpolate(finalOffsetPoints, Standard_False, Precision::Confusion());
//  aNoPeriodInterpolate.Perform();
//  Handle_Geom_BSplineCurve aCurve(aNoPeriodInterpolate.Curve());
//   //check results
//  if (!aNoPeriodInterpolate.IsDone()) return false;
//  m_all_offset_cuts_high.push_back(aCurve);
// }
//
//
//
//
//
// build.saveToFile("c:/output.iv");
// outfile.close();
//
//
//
//
//
//return true;
//
//}

//bool cutting_tools::checkPointDistance(std::vector<gp_Pnt> &finalPoints,std::vector<gp_Pnt> &output)
//{
// std::vector<gp_Pnt>::iterator aPntIt,atempit;
// output.clear();
// double square_precision = Precision::Confusion()*Precision::Confusion();
// int i;
// for(aPntIt=finalPoints.begin();aPntIt!=finalPoints.end();++aPntIt)
// {
//  atempit=aPntIt;
//  output.push_back(*aPntIt);
//  i=1;
//  while(((*(aPntIt+i)).SquareDistance(*aPntIt))< square_precision && (aPntIt+i)!=finalPoints.end())
//  {
//   i++;
//   atempit++;
//  }
//  aPntIt = atempit;
//
//
//
// }
//
// return true;
//}

//bool cutting_tools::checkPointIntersection(std::vector<projectPointContainer> &finalPoints)
//{
// //Hier wird gecheckt ob die Punkte wirklich alle in der richtigen Reihenfolge vorliegen
// std::vector<projectPointContainer>::iterator aPntIt;
// double distance,distance_old;
// projectPointContainer nearestPointStruct;
// int k;
// for(unsigned int j=0;j<finalPoints.size();++j)
// {
//  distance_old = 100;
//  for(int i=1;i<30;++i)
//  {
//   //Wenn wir schon fast am Ende sind oder schon bald, dann rausspringen
//   if((j+i)>=finalPoints.size()) break;
//
//   distance = (finalPoints[j+i].point).SquareDistance(finalPoints[j].point);
//   if(distance<distance_old)
//   {
//    //Speichern wo wir den nächsten Punkt gefunden haben
//    k=i;
//    distance_old = distance;
//   }
//  }
//  //Jetzt checken ob der nächste Punkt bereits der gesuchte ist
//  if(k==1)
//  {
//   continue;
//  }
//  else
//  {
//   //Jetzt den Punkteaustausch vornehmen
//   nearestPointStruct = finalPoints[j+k];
//   finalPoints[j+k] = finalPoints[j+1];
//   finalPoints[j+1] = nearestPointStruct;
//  }
// }
//
//
//
// return true;
//}
//
//



Handle_Geom_BSplineCurve cutting_tools::InterpolateOrderedPoints(Handle(TColgp_HArray1OfPnt) InterpolationPoints, const bool direction)
{
    //Handle_TColStd_HArray1OfBoolean TangentFlags = new TColStd_HArray1OfBoolean(1,InterpolationPoints->Length());
    //for (int j=1;j<=TangentFlags->Length();++j)
    //    TangentFlags->SetValue(j,true);

    GeomAPI_Interpolate aBSplineInterpolation(InterpolationPoints, Standard_False, Precision::Confusion());
    //aBSplineInterpolation.Load(Tangents,TangentFlags);
    aBSplineInterpolation.Perform();
    Handle_Geom_BSplineCurve aBSplineCurve(aBSplineInterpolation.Curve());
    //check results
    //if (!aBSplineInterpolation.IsDone()) cout << "error" << endl;

    return aBSplineCurve;
}


bool cutting_tools::CheckEdgeTangency(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2)
{
    //Get the vertex which is equal to both edges
    TopoDS_Vertex CommonVertex;
    TopExp::CommonVertex(edge1,edge2,CommonVertex);
    if (CommonVertex.IsNull())
    {
        cout << "Not possible to calculate Tangency" << endl;
        return false;
    }
    gp_Pnt CommonPoint = BRep_Tool::Pnt(CommonVertex);
    BRepAdaptor_Curve aCurve1(edge1);
    BRepAdaptor_Curve aCurve2(edge2);
    gp_Vec Tangent1,Tangent2;
    gp_Pnt P;
    if (aCurve1.Value(aCurve1.FirstParameter()).IsEqual(CommonPoint,0.1))
        aCurve1.D1(aCurve1.FirstParameter(),P,Tangent1);
    else if (aCurve1.Value(aCurve1.LastParameter()).IsEqual(CommonPoint,0.1))
        aCurve1.D1(aCurve1.LastParameter(),P,Tangent1);
    if (aCurve2.Value(aCurve2.FirstParameter()).IsEqual(CommonPoint,0.1))
        aCurve2.D1(aCurve1.FirstParameter(),P,Tangent2);
    else if (aCurve2.Value(aCurve2.LastParameter()).IsEqual(CommonPoint,0.1))
        aCurve2.D1(aCurve1.LastParameter(),P,Tangent2);

    //Now we calculate the angle between the two Tangents and if the angle is below 10° then we say its continuous
    double angle = Tangent1.Angle(Tangent2);
    //Angle must be between +/-5°
    if (angle<(5.0/180.0*D_PI) || angle>(175.0/180.0*D_PI))
        return true;
    else
        return false;
}

bool cutting_tools::CheckforLastPoint(const gp_Pnt& lastPoint, int &start_index,int &start_array,const std::vector<std::vector<std::pair<gp_Pnt,double> > >& MasterPointsStorage)
{
    float dist,distold = FLT_MAX;
    for (unsigned int t=0;t<MasterPointsStorage.size();++t)
    {
        for (unsigned int k=0;k<MasterPointsStorage[t].size();k++)
        {
            dist = float(MasterPointsStorage[t][k].first.SquareDistance(lastPoint));
            if (dist<distold)
            {
                start_index = k;
                start_array = t;
                distold = dist;
            }
        }
    }
    return true;
}

bool cutting_tools::CheckforLastPoint(const gp_Pnt& lastPoint, int &start_index,int &start_array,const std::vector<std::vector<gp_Pnt> >& SlavePointsStorage)
{
    float dist,distold = FLT_MAX;
    for (unsigned int t=0;t<SlavePointsStorage.size();++t)
    {
        for (unsigned int k=0;k<SlavePointsStorage[t].size();k++)
        {
            dist = float(SlavePointsStorage[t][k].SquareDistance(lastPoint));
            if (dist<distold)
            {
                start_index = k;
                start_array = t;
                distold = dist;
            }
        }
    }
    return true;
}

bool cutting_tools::CheckforLastPoint(  const gp_Pnt& lastPoint,
                                        int &start_index_master,
                                        int &start_array_master,
                                        int &start_index_slave,
                                        int &start_array_slave,
                                        const std::vector<std::vector<std::pair<gp_Pnt,double> > >& MasterPointsStorage,
                                        const std::vector<std::vector<gp_Pnt> >& SlavePointsStorage
                                     )
{
    float dist,distold = FLT_MAX;
    for (unsigned int t=0;t<MasterPointsStorage.size();++t)
    {
        for (unsigned int k=0;k<MasterPointsStorage[t].size();k++)
        {
            dist = float(MasterPointsStorage[t][k].first.SquareDistance(lastPoint));
            if (dist<distold)
            {
                start_index_master = k;
                start_array_master = t;
                distold = dist;
            }
        }
    }
    distold = FLT_MAX;
    for (unsigned int t=0;t<SlavePointsStorage.size();++t)
    {
        for (unsigned int k=0;k<SlavePointsStorage[t].size();k++)
        {
            dist = float(SlavePointsStorage[t][k].SquareDistance(lastPoint));
            if (dist<distold)
            {
                start_index_slave = k;
                start_array_slave = t;
                distold = dist;
            }
        }
    }
    return true;
}
bool cutting_tools::OffsetWires_Standard() //Version wo nur in X,Y-Ebene verschoben wird
{
    //for debugging issues
    std::ofstream anoutput1,anoutput2;
    anoutput1.open("c:/master_output.txt");
    anoutput2.open("c:/slave_output.txt");
    std::vector<std::pair<float,TopoDS_Shape> >::iterator current_flat_level;
    gp_Pnt lastPoint(0.0,0.0,0.0); //Initialize the first Point to the Origin
    current_flat_level = m_ordered_cuts.begin();
    bool slave_is_wire= false; //Necessary if the slave is a wire while the master is not...
    //Nicht beim höchsten Anfangen, da wir den nicht mit dem Master fahren wollen
    for (m_ordered_cuts_it = m_ordered_cuts.begin()+1;m_ordered_cuts_it!=m_ordered_cuts.end();++m_ordered_cuts_it)
    {
        std::vector<std::pair<gp_Pnt,double> > MasterPointContainer;
        std::vector<std::vector<std::pair<gp_Pnt,double> > > MasterPointsStorage;
        std::vector<gp_Pnt> SlavePointContainer;
        std::vector<std::vector<gp_Pnt> > SlavePointsStorage;
        std::vector<std::vector<gp_Pnt> >::iterator anIterator1;
        std::vector<std::vector<std::pair<gp_Pnt,double> > >::iterator anIterator2;
        MasterPointsStorage.clear();
        SlavePointsStorage.clear();
        MasterPointContainer.clear();
        SlavePointContainer.clear();
        //Now we have to select which strategy to choose
        //if the current levels is bigger,the same, or less then the previous one
        if (m_ordered_cuts_it->first<(m_ordered_cuts_it-1)->first)
        {
            //Master is calculated as Usual, Slave follows the Master
            //Check if current Level has got a Wire
            if (m_ordered_cuts_it->second.ShapeType() == TopAbs_WIRE)
            {
                WireExplorer aWireExplorer(TopoDS::Wire(m_ordered_cuts_it->second));
                for (aWireExplorer.Init();aWireExplorer.More();aWireExplorer.Next())
                {
                    BRepAdaptor_Curve curveAdaptor(aWireExplorer.Current());
                    GCPnts_QuasiUniformAbscissa aProp(curveAdaptor,100);
                    for (int i=1;i<=aProp.NbPoints();++i)
                    {
                        std::pair<gp_Pnt,double> aTempPair;
                        //Check the direction
                        if (aWireExplorer.Current().Orientation() != TopAbs_REVERSED)
                            curveAdaptor.D0(aProp.Parameter(i),aTempPair.first);
                        else curveAdaptor.D0(aProp.Parameter(aProp.NbPoints()-i+1),aTempPair.first);
                        //aTempPair.first.SetZ(aTempPair.first.Z() + m_UserSettings.master_radius);
						aTempPair.first.SetZ(aTempPair.first.Z() );
                        aTempPair.second = 0.0; //Initialize of Angle
                        //checken ob der neue Punkt zu nahe am alten ist. Wenn ja, dann kein push_back
                        if (MasterPointContainer.size()>0 && (MasterPointContainer.rbegin()->first.SquareDistance(aTempPair.first)>(Precision::Confusion()*Precision::Confusion())))
                        {
                            MasterPointContainer.push_back(aTempPair);
                            anoutput1 << aTempPair.first.X() <<","<< aTempPair.first.Y() <<","<< aTempPair.first.Z()<<std::endl;
                            aTempPair.first.SetZ(aTempPair.first.Z() - m_UserSettings.master_radius - m_UserSettings.slave_radius - m_UserSettings.sheet_thickness);
                            SlavePointContainer.push_back(aTempPair.first);
                            anoutput2 << aTempPair.first.X() <<","<< aTempPair.first.Y() <<","<< aTempPair.first.Z()<<std::endl;
                        }
                        else if (MasterPointContainer.empty())
                        {
                            MasterPointContainer.push_back(aTempPair);
                            anoutput1 << aTempPair.first.X() <<","<< aTempPair.first.Y() <<","<< aTempPair.first.Z()<<std::endl;
                            aTempPair.first.SetZ(aTempPair.first.Z() - m_UserSettings.master_radius - m_UserSettings.slave_radius - m_UserSettings.sheet_thickness);
                            SlavePointContainer.push_back(aTempPair.first);
                            anoutput2 << aTempPair.first.X() <<","<< aTempPair.first.Y() <<","<< aTempPair.first.Z()<<std::endl;
                        }
                    }
                    //Now Interpolate the Points only if we have a non-continuous edge or if we are finished
                    //with all edges
                    bool tangency = true;
                    //If there are more Edges in the wire
                    if (aWireExplorer.MoreEdge())
                    {
                        tangency = CheckEdgeTangency(aWireExplorer.Current(),aWireExplorer.NextEdge());
                        if (!tangency)
                        {
                            //Store all the PointClouds in a StorageVector to arrange the Robot-Movement afterwards
                            MasterPointsStorage.push_back(MasterPointContainer);
                            SlavePointsStorage.push_back(SlavePointContainer);
                            MasterPointContainer.clear();
                            SlavePointContainer.clear();
                        }
                        else continue;
                    }
                    else
                    {
                        MasterPointsStorage.push_back(MasterPointContainer);
                        SlavePointsStorage.push_back(SlavePointContainer);
                        MasterPointContainer.clear();
                        SlavePointContainer.clear();
                    }
                }
                //Now check the point-cloud with the shortest distance to "lastPoint"
                int start_index = 0,start_array=0;
                CheckforLastPoint(lastPoint,start_index,start_array,MasterPointsStorage);
                //Now Interpolate the PointClouds...Cloud by Cloud
                //First we have to divide the first PointCloud as it is for sure that we start
                //Somewhere in the middle of it. We will then insert the points at the current start
                if (MasterPointsStorage.size() == 1) //If we have only one PointCloud
                {
                    //To make sure that the start and end point are the same we have to use a flag
                    bool first = true;
                    for (unsigned int i=start_index;i<MasterPointsStorage.begin()->size();i++)
                    {
                        //We skip the Endpoint as it may be the same point as the start point
                        if (i+1 == MasterPointsStorage.begin()->size())
                        {
                            i=-1;
                            first = false;
                            continue;
                        }
                        MasterPointContainer.push_back(MasterPointsStorage[0][i]);
                        SlavePointContainer.push_back(SlavePointsStorage[0][i]);
                        if (!first && i == start_index)
                            break;
                    }
                    //Now lets interpolate the Point Cloud
                    Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointContainer.size());
                    Handle(TColgp_HArray1OfPnt) InterpolationPointsSlave = new TColgp_HArray1OfPnt(1, SlavePointContainer.size());
                    for (unsigned int t=0;t<MasterPointContainer.size();++t)
                    {
                        InterpolationPointsMaster->SetValue(t+1,MasterPointContainer[t].first);
                        InterpolationPointsSlave->SetValue(t+1,SlavePointContainer[t]);
                    }
                    m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                    m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPointsSlave,true));
                    //Store the last point
                    lastPoint = MasterPointContainer.rbegin()->first;
                }
                else
                {
                    anIterator1 = SlavePointsStorage.begin();
                    anIterator2 = MasterPointsStorage.begin();
                    MasterPointContainer.clear();
                    SlavePointContainer.clear();
                    for (int i=0;i<=start_index;i++)
                    {
                        MasterPointContainer.push_back(MasterPointsStorage[start_array][i]);
                        SlavePointContainer.push_back(SlavePointsStorage[start_array][i]);
                    }
                    MasterPointsStorage.insert((anIterator2+start_array+1),MasterPointContainer);
                    SlavePointsStorage.insert((anIterator1+start_array+1),SlavePointContainer);
                    MasterPointContainer.clear();
                    SlavePointContainer.clear();
                    //Reinitialize the Iterators
                    anIterator1 = SlavePointsStorage.begin();
                    anIterator2 = MasterPointsStorage.begin();
                    for (unsigned int i=start_index;i<MasterPointsStorage[start_array].size();i++)
                    {
                        MasterPointContainer.push_back(MasterPointsStorage[start_array][i]);
                        SlavePointContainer.push_back(SlavePointsStorage[start_array][i]);
                    }
                    MasterPointsStorage.insert((anIterator2+start_array+2),MasterPointContainer);
                    SlavePointsStorage.insert((anIterator1+start_array+2),SlavePointContainer);
                    //Reinitialize the Iterators
                    anIterator1 = SlavePointsStorage.begin();
                    anIterator2 = MasterPointsStorage.begin();
                    //Delete the Original PointCloud
                    MasterPointsStorage.erase(anIterator2+start_array);
                    SlavePointsStorage.erase(anIterator1+start_array);
                    MasterPointContainer.clear();
                    SlavePointContainer.clear();
                    //Now lets interpolate the Point Clouds
                    //Start at start_array+1 as the insert operations used us to do it like that
                    for (unsigned int j=start_array+1;j<MasterPointsStorage.size();++j)
                    {
                        Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointsStorage[j].size());
                        Handle(TColgp_HArray1OfPnt) InterpolationPointsSlave = new TColgp_HArray1OfPnt(1, SlavePointsStorage[j].size());
                        for (unsigned int t=0;t<MasterPointsStorage[j].size();++t)
                        {
                            InterpolationPointsMaster->SetValue(t+1,MasterPointsStorage[j][t].first);
                            InterpolationPointsSlave->SetValue(t+1,SlavePointsStorage[j][t]);
                        }
                        m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                        m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPointsSlave,true));
                        if (j+1==MasterPointsStorage.size())
                        {
                            j=-1;
                            continue;
                        }
                        if (j+1==start_array+1)
                            break;

                    }
                    lastPoint = MasterPointsStorage[start_array].rbegin()->first;
                }
            }
            else //If the current Curve is no Wire (Mode <)
            {
                Edgesort aCutShapeSorter(m_ordered_cuts_it->second);
                for (aCutShapeSorter.Init();aCutShapeSorter.More();aCutShapeSorter.Next())
                {
                    //Get the PCurve and the GeomSurface
                    Handle_Geom2d_Curve a2DCurve;
                    Handle_Geom_Surface aSurface;
                    TopLoc_Location aLoc;
                    TopoDS_Edge anEdge;
                    double first2,last2;
                    bool reversed = false;
                    BRep_Tool::CurveOnSurface(aCutShapeSorter.Current(),a2DCurve,aSurface,aLoc,first2,last2);
                    //Jetzt noch die resultierende Surface und die Curve sauber drehen
                    //(vielleicht wurde ja das TopoDS_Face irgendwie gedreht oder die TopoDS_Edge)
                    if (aCutShapeSorter.Current().Orientation() == TopAbs_REVERSED)
                        reversed = true;

                    BRepAdaptor_Curve aCurveAdaptor(aCutShapeSorter.Current());
                    GCPnts_QuasiUniformAbscissa aPointGenerator(aCurveAdaptor,200);
                    int PointSize = aPointGenerator.NbPoints();
                    //Now get the surface normal to the generated points
                    for (int i=1;i<=PointSize;++i)
                    {
                        std::pair<gp_Pnt,double> PointContactPair;
                        gp_Pnt2d a2dParaPoint;
                        gp_Pnt aSurfacePoint;
                        TopoDS_Face aFace;
                        gp_Vec Uvec,Vvec,normalVec;
                        //If the curve is reversed we also have to reverse the point direction
                        if (reversed) a2DCurve->D0(aPointGenerator.Parameter(PointSize-i+1),a2dParaPoint);
                        else a2DCurve->D0(aPointGenerator.Parameter(i),a2dParaPoint);
                        GeomAdaptor_Surface aGeom_Adaptor(aSurface);
                        int t = aGeom_Adaptor.GetType();
                        aGeom_Adaptor.D1(a2dParaPoint.X(),a2dParaPoint.Y(),aSurfacePoint,Uvec,Vvec);
                        //Jetzt den Normalenvector auf die Fläche ausrechnen
                        normalVec = Uvec;
                        normalVec.Cross(Vvec);
                        normalVec.Normalize();
                        //Jetzt ist die Normale berechnet und auch normalisiert
                        //Jetzt noch checken ob die Normale auch wirklich auf die saubere Seite zeigt
                        //dazu nur checken ob der Z-Wert der Normale größer Null ist (dann im 1.und 2. Quadranten)
                        if (normalVec.Z()<0) normalVec.Multiply(-1.0);

                        //Mal kurz den Winkel zur Grund-Ebene ausrechnen
                        gp_Vec planeVec(normalVec.X(),normalVec.Y(),0.0);
                        //Den Winkel
                        PointContactPair.second = normalVec.Angle(planeVec);
                        gp_Vec NormalVecSlave = normalVec;
                        gp_Pnt SlavePoint;
                        //Jetzt die Z-Komponente auf 0 setzen
                        //normalVec.SetZ(0.0);
                        normalVec.Normalize();
                        //Jetzt die Normale mit folgender Formel multiplizieren für den Master
                        //double multiply = m_UserSettings.master_radius*(1-sin(PointContactPair.second))/cos(PointContactPair.second);
                        double multiply = m_UserSettings.master_radius;
                        normalVec.Multiply(multiply);
                        //und hier für den Slave
                        NormalVecSlave.Normalize();
                        multiply = m_UserSettings.sheet_thickness+m_UserSettings.slave_radius;
                        NormalVecSlave.Multiply(multiply);
                        //Jetzt die Richtung umdrehen
                        NormalVecSlave.Multiply(-1.0);
                        //Jetzt den OffsetPunkt berechnen
                        PointContactPair.first.SetXYZ(aSurfacePoint.XYZ());
						//PointContactPair.first.SetXYZ(aSurfacePoint.XYZ() + normalVec.XYZ());
                        SlavePoint.SetXYZ(aSurfacePoint.XYZ() + NormalVecSlave.XYZ());
                        //PointContactPair.first.SetZ(PointContactPair.first.Z() + m_UserSettings.master_radius);
                        //Damit wir keine Punkte bekommen die zu nahe beieinander liegen
                        //Den letzten hinzugefügten Punkt suchen
                        if (MasterPointContainer.size()>0 && (MasterPointContainer.rbegin()->first.SquareDistance(PointContactPair.first)>0.001))
                        {
                            MasterPointContainer.push_back(PointContactPair);
                            SlavePointContainer.push_back(SlavePoint);
                            anoutput1 << PointContactPair.first.X() <<","<< PointContactPair.first.Y() <<","<< PointContactPair.first.Z()<<std::endl;
                            anoutput2 << SlavePoint.X() <<","<< SlavePoint.Y() <<","<< SlavePoint.Z()<<std::endl;
                        }
                        else if (MasterPointContainer.empty())
                        {
                            MasterPointContainer.push_back(PointContactPair);
                            SlavePointContainer.push_back(SlavePoint);
                            anoutput1 << PointContactPair.first.X() <<","<< PointContactPair.first.Y() <<","<< PointContactPair.first.Z()<<std::endl;
                            anoutput2 << SlavePoint.X() <<","<< SlavePoint.Y() <<","<< SlavePoint.Z()<<std::endl;
                        }
                    }
                }
                //Before we Output anything we check if the lowest Coordinate can still touch the "currently" highest flat area and if yes, we have to adapt the Slave toolpath
                for (unsigned int k=0;k<SlavePointContainer.size();++k)
                {
                    if ((SlavePointContainer[k].Z()+m_UserSettings.slave_radius)>(current_flat_level->first-m_UserSettings.sheet_thickness))
                    {
                        slave_is_wire = true;
                        TopoDS_Wire aWire = TopoDS::Wire(current_flat_level->second);
                        BRepAdaptor_CompCurve2 wireAdaptor(aWire);
                        GCPnts_QuasiUniformAbscissa aProp(wireAdaptor,1000);
                        SlavePointContainer.clear();
                        for (int i=1;i<=aProp.NbPoints();++i)
                        {
                            gp_Pnt SlaveOffsetPoint;
                            wireAdaptor.D0(aProp.Parameter(i),SlaveOffsetPoint);
                            SlaveOffsetPoint.SetZ(SlaveOffsetPoint.Z() - m_UserSettings.slave_radius - m_UserSettings.sheet_thickness);
                            //checken ob der neue Punkt zu nahe am alten ist. Wenn ja, dann kein push_back
                            if (SlavePointContainer.size()>0 && (SlavePointContainer.rbegin()->SquareDistance(SlaveOffsetPoint)>(0.001)))
                            {
                                SlavePointContainer.push_back(SlaveOffsetPoint);
                                anoutput2 << SlaveOffsetPoint.X() <<","<< SlaveOffsetPoint.Y() <<","<< SlaveOffsetPoint.Z()<<std::endl;
                            }
                            else if (SlavePointContainer.empty())
                            {
                                SlavePointContainer.push_back(SlaveOffsetPoint);
                                anoutput2 << SlaveOffsetPoint.X() <<","<< SlaveOffsetPoint.Y() <<","<< SlaveOffsetPoint.Z()<<std::endl;
                            }
                        }
                        break;
                    }
                }
                MasterPointsStorage.clear();
                SlavePointsStorage.clear();
                MasterPointsStorage.push_back(MasterPointContainer);
                SlavePointsStorage.push_back(SlavePointContainer);
                int start_index_master = 0,start_array_master=0,start_index_slave=0,start_array_slave=0;
                CheckforLastPoint(lastPoint,start_index_master,start_array_master,start_index_slave,start_array_slave,MasterPointsStorage,SlavePointsStorage);
                //If the Slave is a Wire 'slave_is_wire ==true' then we have to take care of two independent point clouds
                if (!slave_is_wire)
                {
                    //First we have to divide the first PointCloud as it is for sure that we start
                    //Somewhere in the middle of it. We will then insert the points at the current start
                    MasterPointContainer.clear();
                    SlavePointContainer.clear();
                    bool first = true;
                    for (unsigned int i=start_index_master;i<MasterPointsStorage.begin()->size();i++)
                    {
                        //We skip the Endpoint as it may be the same point as the start point
                        if (i+1 == MasterPointsStorage.begin()->size())
                        {
                            first = false;
                            i=-1;
                            continue;
                        }
                        MasterPointContainer.push_back(MasterPointsStorage[0][i]);
                        SlavePointContainer.push_back(SlavePointsStorage[0][i]);
                        if (!first && i==start_index_master)
                            break;
                    }
                }
                else //if slave is a wire
                {
                    //Reset the flag
                    slave_is_wire = false;
                    //Now check the point-cloud with the shortest distance to "lastPoint"
                    //As the Master and the slave have different number of points we have to do it two times now
                    //First we have to divide the first PointCloud as it is for sure that we start
                    //Somewhere in the middle of it. We will then insert the points at the current start
                    MasterPointContainer.clear();
                    SlavePointContainer.clear();
                    bool first = true;
                    for (unsigned int i=start_index_master;i<MasterPointsStorage.begin()->size();i++)
                    {
                        //We skip the Endpoint as it may be the same point as the start point
                        if (i+1 == MasterPointsStorage.begin()->size())
                        {
                            first = false;
                            i=-1;
                            continue;
                        }
                        MasterPointContainer.push_back(MasterPointsStorage[0][i]);
                        if (!first && i== start_index_master)
                            break;
                    }
                    first = true;
                    for (unsigned int i=start_index_slave;i<SlavePointsStorage.begin()->size();i++)
                    {
                        //We skip the Endpoint as it may be the same point as the start point
                        if (i+1 == SlavePointsStorage.begin()->size())
                        {
                            first = false;
                            i=-1;
                            continue;
                        }
                        SlavePointContainer.push_back(SlavePointsStorage[0][i]);
                        if (!first && i== start_index_slave)
                            break;
                    }
                }
                //Now lets interpolate the Point Cloud
                Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointContainer.size());
                Handle(TColgp_HArray1OfPnt) InterpolationPointsSlave = new TColgp_HArray1OfPnt(1, SlavePointContainer.size());
                for (unsigned int t=0;t<MasterPointContainer.size();++t)
                {
                    InterpolationPointsMaster->SetValue(t+1,MasterPointContainer[t].first);
                }
                for (unsigned int t=0;t<SlavePointContainer.size();++t)
                {
                    InterpolationPointsSlave->SetValue(t+1,SlavePointContainer[t]);
                }

                /*CheckPoints(InterpolationPointsMaster);*/
                /*CheckPoints(InterpolationPointsSlave);*/

                m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPointsSlave,true));
                //Store the last point
                lastPoint = MasterPointContainer.rbegin()->first;
                //SlaveTool Path finished
            }//Current Curve < last curve is finished here
        }
        if (fabs(m_ordered_cuts_it->first-(m_ordered_cuts_it-1)->first)<=0.01)
        {
            //we only set the new flat level wire here
            //no Toolpath is calculated
            current_flat_level = m_ordered_cuts_it;
        }//end of current == last
        if (m_ordered_cuts_it->first>(m_ordered_cuts_it-1)->first)
        {
            //The Slave Tool is now the Master Tool and therefore we only have to exchange < case
            //Check if current Level has got a Wire
            if (m_ordered_cuts_it->second.ShapeType() == TopAbs_WIRE)
            {
                WireExplorer aWireExplorer(TopoDS::Wire(m_ordered_cuts_it->second));
                for (aWireExplorer.Init();aWireExplorer.More();aWireExplorer.Next())
                {
                    BRepAdaptor_Curve curveAdaptor(aWireExplorer.Current());
                    GCPnts_QuasiUniformAbscissa aProp(curveAdaptor,100);
                    for (int i=1;i<=aProp.NbPoints();++i)
                    {
                        std::pair<gp_Pnt,double> aTempPair;
                        //Check the direction
                        if (aWireExplorer.Current().Orientation() != TopAbs_REVERSED)
                            curveAdaptor.D0(aProp.Parameter(i),aTempPair.first);
                        else curveAdaptor.D0(aProp.Parameter(aProp.NbPoints()-i+1),aTempPair.first);
                        aTempPair.first.SetZ(aTempPair.first.Z() -m_UserSettings.slave_radius -m_UserSettings.sheet_thickness);
                        aTempPair.second = 0.0; //Initialize of Angle
                        //checken ob der neue Punkt zu nahe am alten ist. Wenn ja, dann kein push_back
                        if (MasterPointContainer.size()>0 && (MasterPointContainer.rbegin()->first.SquareDistance(aTempPair.first)>(Precision::Confusion()*Precision::Confusion())))
                        {
                            MasterPointContainer.push_back(aTempPair);
                            anoutput1 << aTempPair.first.X() <<","<< aTempPair.first.Y() <<","<< aTempPair.first.Z()<<std::endl;
                            aTempPair.first.SetZ(aTempPair.first.Z() + m_UserSettings.master_radius + m_UserSettings.slave_radius + m_UserSettings.sheet_thickness);
                            SlavePointContainer.push_back(aTempPair.first);
                            anoutput2 << aTempPair.first.X() <<","<< aTempPair.first.Y() <<","<< aTempPair.first.Z()<<std::endl;
                        }
                        else if (MasterPointContainer.empty())
                        {
                            MasterPointContainer.push_back(aTempPair);
                            anoutput1 << aTempPair.first.X() <<","<< aTempPair.first.Y() <<","<< aTempPair.first.Z()<<std::endl;
                            aTempPair.first.SetZ(aTempPair.first.Z() + m_UserSettings.master_radius + m_UserSettings.slave_radius + m_UserSettings.sheet_thickness);
                            SlavePointContainer.push_back(aTempPair.first);
                            anoutput2 << aTempPair.first.X() <<","<< aTempPair.first.Y() <<","<< aTempPair.first.Z()<<std::endl;
                        }
                    }
                    //Now Interpolate the Points only if we have a non-continuous edge or if we are finished
                    //with all edges
                    bool tangency = true;
                    //If there are more Edges in the wire
                    if (aWireExplorer.MoreEdge())
                    {
                        tangency = CheckEdgeTangency(aWireExplorer.Current(),aWireExplorer.NextEdge());
                        if (!tangency)
                        {
                            //Store all the PointClouds in a StorageVector to arrange the Robot-Movement afterwards
                            MasterPointsStorage.push_back(MasterPointContainer);
                            SlavePointsStorage.push_back(SlavePointContainer);
                            MasterPointContainer.clear();
                            SlavePointContainer.clear();
                        }
                        else continue;
                    }
                    else
                    {
                        MasterPointsStorage.push_back(MasterPointContainer);
                        SlavePointsStorage.push_back(SlavePointContainer);
                        MasterPointContainer.clear();
                        SlavePointContainer.clear();
                    }
                }
                //Now check the point-cloud with the shortest distance to "lastPoint"
                int start_index = 0,start_array=0;
                CheckforLastPoint(lastPoint,start_index,start_array,MasterPointsStorage);
                //Now Interpolate the PointClouds...Cloud by Cloud
                //First we have to divide the first PointCloud as it is for sure that we start
                //Somewhere in the middle of it. We will then insert the points at the current start
                if (MasterPointsStorage.size() == 1) //If we have only one PointCloud
                {
                    bool first = true;
                    for (unsigned int i=start_index;i<MasterPointsStorage.begin()->size();i++)
                    {
                        //We skip the Endpoint as it may be the same point as the start point
                        if (i+1 == MasterPointsStorage.begin()->size())
                        {
                            i=-1;
                            first = false;
                            continue;
                        }
                        MasterPointContainer.push_back(MasterPointsStorage[0][i]);
                        SlavePointContainer.push_back(SlavePointsStorage[0][i]);
                        if (!first && i == start_index)
                            break;
                    }
                    //Now lets interpolate the Point Cloud
                    Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointContainer.size());
                    Handle(TColgp_HArray1OfPnt) InterpolationPointsSlave = new TColgp_HArray1OfPnt(1, SlavePointContainer.size());
                    for (unsigned int t=0;t<MasterPointContainer.size();++t)
                    {
                        InterpolationPointsMaster->SetValue(t+1,MasterPointContainer[t].first);
                        InterpolationPointsSlave->SetValue(t+1,SlavePointContainer[t]);
                    }
                    m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                    m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPointsSlave,true));
                    //Store the last point
                    lastPoint = MasterPointContainer.rbegin()->first;
                }
                else
                {
                    anIterator1 = SlavePointsStorage.begin();
                    anIterator2 = MasterPointsStorage.begin();
                    MasterPointContainer.clear();
                    SlavePointContainer.clear();
                    for (int i=0;i<=start_index;i++)
                    {
                        MasterPointContainer.push_back(MasterPointsStorage[start_array][i]);
                        SlavePointContainer.push_back(SlavePointsStorage[start_array][i]);
                    }
                    MasterPointsStorage.insert((anIterator2+start_array+1),MasterPointContainer);
                    SlavePointsStorage.insert((anIterator1+start_array+1),SlavePointContainer);
                    MasterPointContainer.clear();
                    SlavePointContainer.clear();
                    //Reinitialize the Iterators
                    anIterator1 = SlavePointsStorage.begin();
                    anIterator2 = MasterPointsStorage.begin();
                    for (unsigned int i=start_index;i<MasterPointsStorage[start_array].size();i++)
                    {
                        MasterPointContainer.push_back(MasterPointsStorage[start_array][i]);
                        SlavePointContainer.push_back(SlavePointsStorage[start_array][i]);
                    }
                    MasterPointsStorage.insert((anIterator2+start_array+2),MasterPointContainer);
                    SlavePointsStorage.insert((anIterator1+start_array+2),SlavePointContainer);
                    //Reinitialize the Iterators
                    anIterator1 = SlavePointsStorage.begin();
                    anIterator2 = MasterPointsStorage.begin();
                    //Delete the Original PointCloud
                    MasterPointsStorage.erase(anIterator2+start_array);
                    SlavePointsStorage.erase(anIterator1+start_array);
                    MasterPointContainer.clear();
                    SlavePointContainer.clear();
                    //Now lets interpolate the Point Clouds
                    //Start at start_array+1 as the insert operations used us to do it like that
                    for (unsigned int j=start_array+1;j<MasterPointsStorage.size();++j)
                    {
                        Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointsStorage[j].size());
                        Handle(TColgp_HArray1OfPnt) InterpolationPointsSlave = new TColgp_HArray1OfPnt(1, SlavePointsStorage[j].size());
                        for (unsigned int t=0;t<MasterPointsStorage[j].size();++t)
                        {
                            InterpolationPointsMaster->SetValue(t+1,MasterPointsStorage[j][t].first);
                            InterpolationPointsSlave->SetValue(t+1,SlavePointsStorage[j][t]);
                        }
                        m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                        m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPointsSlave,true));
                        if (j+1==MasterPointsStorage.size())
                        {
                            j=-1;
                            continue;
                        }
                        if (j+1==start_array+1)
                            break;
                    }
                    lastPoint = MasterPointsStorage[start_array].rbegin()->first;
                }
            }
            else //If the current Curve is no Wire (Mode >)
            {
                Edgesort aCutShapeSorter(m_ordered_cuts_it->second);
                for (aCutShapeSorter.Init();aCutShapeSorter.More();aCutShapeSorter.Next())
                {
                    //Get the PCurve and the GeomSurface
                    Handle_Geom2d_Curve a2DCurve;
                    Handle_Geom_Surface aSurface;
                    TopLoc_Location aLoc;
                    TopoDS_Edge anEdge;
                    double first2,last2;
                    bool reversed = false;
                    BRep_Tool::CurveOnSurface(aCutShapeSorter.Current(),a2DCurve,aSurface,aLoc,first2,last2);
                    //Jetzt noch die resultierende Surface und die Curve sauber drehen
                    //(vielleicht wurde ja das TopoDS_Face irgendwie gedreht oder die TopoDS_Edge)
                    if (aCutShapeSorter.Current().Orientation() == TopAbs_REVERSED)
                        reversed = true;

                    BRepAdaptor_Curve aCurveAdaptor(aCutShapeSorter.Current());
                    GCPnts_QuasiUniformAbscissa aPointGenerator(aCurveAdaptor,200);
                    int PointSize = aPointGenerator.NbPoints();
                    //Now get the surface normal to the generated points
                    for (int i=1;i<=PointSize;++i)
                    {
                        std::pair<gp_Pnt,double> PointContactPair;
                        gp_Pnt2d a2dParaPoint;
                        gp_Pnt aSurfacePoint;
                        TopoDS_Face aFace;
                        gp_Vec Uvec,Vvec,normalVec;
                        //If the curve is reversed we also have to reverse the point direction
                        if (reversed) a2DCurve->D0(aPointGenerator.Parameter(PointSize-i+1),a2dParaPoint);
                        else a2DCurve->D0(aPointGenerator.Parameter(i),a2dParaPoint);
                        GeomAdaptor_Surface aGeom_Adaptor(aSurface);
                        int t = aGeom_Adaptor.GetType();
                        aGeom_Adaptor.D1(a2dParaPoint.X(),a2dParaPoint.Y(),aSurfacePoint,Uvec,Vvec);
                        //Jetzt den Normalenvector auf die Fläche ausrechnen
                        normalVec = Uvec;
                        normalVec.Cross(Vvec);
                        normalVec.Normalize();
                        //Jetzt ist die Normale berechnet und auch normalisiert
                        //Jetzt noch checken ob die Normale auch wirklich auf die saubere Seite zeigt
                        //dazu nur checken ob der Z-Wert der Normale größer Null ist (dann im 1.und 2. Quadranten)
                        if (normalVec.Z()<0) normalVec.Multiply(-1.0);

                        //Mal kurz den Winkel zur Grund-Ebene ausrechnen
                        gp_Vec planeVec(normalVec.X(),normalVec.Y(),0.0);
                        //Den Winkel
                        PointContactPair.second = normalVec.Angle(planeVec);
                        gp_Vec NormalVecSlave = normalVec;
                        gp_Pnt SlavePoint;
                        //Jetzt die Z-Komponente auf 0 setzen
                        //normalVec.SetZ(0.0);
                        normalVec.Normalize();
                        //Jetzt die Normale mit folgender Formel multiplizieren für den Master
                        //double multiply = (m_UserSettings.slave_radius*(1-sin(PointContactPair.second))/cos(PointContactPair.second))+m_UserSettings.sheet_thickness;
                        double multiply = m_UserSettings.slave_radius + m_UserSettings.sheet_thickness;
                        //As the Master is now the Slave we have to multiply it also by -1.0
                        normalVec.Multiply(-multiply);
                        //und hier für den Slave
                        NormalVecSlave.Normalize();
                        multiply = m_UserSettings.master_radius;
                        NormalVecSlave.Multiply(multiply);
                        //Jetzt den OffsetPunkt berechnen
                        PointContactPair.first.SetXYZ(aSurfacePoint.XYZ() + normalVec.XYZ());
                        SlavePoint.SetXYZ(aSurfacePoint.XYZ() + NormalVecSlave.XYZ());
                        //PointContactPair.first.SetZ(PointContactPair.first.Z() - m_UserSettings.slave_radius);
                        //Damit wir keine Punkte bekommen die zu nahe beieinander liegen
                        //Den letzten hinzugefügten Punkt suchen
                        if (MasterPointContainer.size()>0 && (MasterPointContainer.rbegin()->first.SquareDistance(PointContactPair.first)>(Precision::Confusion()*Precision::Confusion())))
                        {
                            MasterPointContainer.push_back(PointContactPair);
                            SlavePointContainer.push_back(SlavePoint);
                            anoutput1 << PointContactPair.first.X() <<","<< PointContactPair.first.Y() <<","<< PointContactPair.first.Z()<<std::endl;
                            anoutput2 << SlavePoint.X() <<","<< SlavePoint.Y() <<","<< SlavePoint.Z()<<std::endl;
                        }
                        else if (MasterPointContainer.empty())
                        {
                            MasterPointContainer.push_back(PointContactPair);
                            SlavePointContainer.push_back(SlavePoint);
                            anoutput1 << PointContactPair.first.X() <<","<< PointContactPair.first.Y() <<","<< PointContactPair.first.Z()<<std::endl;
                            anoutput2 << SlavePoint.X() <<","<< SlavePoint.Y() <<","<< SlavePoint.Z()<<std::endl;
                        }
                    }
                }
                //Before we Output anything we check if the lowest Coordinate can still touch the "currently" highest flat area and if yes, we have to adapt the Slave toolpath
                for (unsigned int k=0;k<SlavePointContainer.size();++k)
                {
                    if ((SlavePointContainer[k].Z()-m_UserSettings.master_radius)<(current_flat_level->first))
                    {
                        slave_is_wire = true;
                        TopoDS_Wire aWire = TopoDS::Wire(current_flat_level->second);
                        BRepAdaptor_CompCurve2 wireAdaptor(aWire);
                        GCPnts_QuasiUniformAbscissa aProp(wireAdaptor,1000);
                        SlavePointContainer.clear();
                        for (int i=1;i<=aProp.NbPoints();++i)
                        {
                            gp_Pnt SlaveOffsetPoint;
                            wireAdaptor.D0(aProp.Parameter(i),SlaveOffsetPoint);
                            SlaveOffsetPoint.SetZ(SlaveOffsetPoint.Z() + m_UserSettings.master_radius);
                            //checken ob der neue Punkt zu nahe am alten ist. Wenn ja, dann kein push_back
                            if (SlavePointContainer.size()>0 && (SlavePointContainer.rbegin()->SquareDistance(SlaveOffsetPoint)>(0.001)))
                            {
                                SlavePointContainer.push_back(SlaveOffsetPoint);
                                anoutput2 << SlaveOffsetPoint.X() <<","<< SlaveOffsetPoint.Y() <<","<< SlaveOffsetPoint.Z()<<std::endl;
                            }
                            else if (SlavePointContainer.empty())
                            {
                                SlavePointContainer.push_back(SlaveOffsetPoint);
                                anoutput2 << SlaveOffsetPoint.X() <<","<< SlaveOffsetPoint.Y() <<","<< SlaveOffsetPoint.Z()<<std::endl;
                            }
                        }
                        break;
                    }
                }
                MasterPointsStorage.push_back(MasterPointContainer);
                SlavePointsStorage.push_back(SlavePointContainer);
                int start_index_master = 0,start_array_master=0,start_index_slave=0,start_array_slave=0;
                CheckforLastPoint(lastPoint,start_index_master,start_array_master,start_index_slave,start_array_slave,MasterPointsStorage,SlavePointsStorage);
                //If the Slave is a Wire 'slave_is_wire ==true' then we have to take care of two independent point clouds
                if (!slave_is_wire)
                {
                    //First we have to divide the first PointCloud as it is for sure that we start
                    //Somewhere in the middle of it. We will then insert the points at the current start
                    MasterPointContainer.clear();
                    SlavePointContainer.clear();
                    bool first =  true;
                    for (unsigned int i=start_index_master;i<MasterPointsStorage.begin()->size();i++)
                    {
                        if (i+1 == MasterPointsStorage.begin()->size())
                        {
                            i=-1;
                            first = false;
                            continue;
                        }
                        MasterPointContainer.push_back(MasterPointsStorage[0][i]);
                        SlavePointContainer.push_back(SlavePointsStorage[0][i]);
                        if (!first && i == start_index_master)
                            break;
                    }

                }
                else //if slave is a wire
                {
                    //Now check the point-cloud with the shortest distance to "lastPoint"
                    //As the Master and the slave have different number of points we have to do it two times now
                    //First we have to divide the first PointCloud as it is for sure that we start
                    //Somewhere in the middle of it. We will then insert the points at the current start
                    MasterPointContainer.clear();
                    SlavePointContainer.clear();
                    bool first = true;
                    for (unsigned int i=start_index_master;i<MasterPointsStorage.begin()->size();i++)
                    {
                        //We skip the Endpoint as it may be the same point as the start point
                        if (i+1 == MasterPointsStorage.begin()->size())
                        {
                            i=-1;
                            first = false;
                            continue;
                        }
                        MasterPointContainer.push_back(MasterPointsStorage[0][i]);
                        if (!first && i== start_index_master)
                            break;
                    }
                    for (unsigned int i=start_index_slave;i<SlavePointsStorage.begin()->size();i++)
                    {
                        //We skip the Endpoint as it may be the same point as the start point
                        if (i+1 == SlavePointsStorage.begin()->size())
                        {
                            i=-1;
                            first = false;
                            continue;
                        }
                        SlavePointContainer.push_back(SlavePointsStorage[0][i]);
                        if (!first && i == start_index_slave)
                            break;
                    }
                }
                //Now lets interpolate the Point Cloud
                Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointContainer.size());
                Handle(TColgp_HArray1OfPnt) InterpolationPointsSlave = new TColgp_HArray1OfPnt(1, SlavePointContainer.size());
                for (unsigned int t=0;t<MasterPointContainer.size();++t)
                {
                    InterpolationPointsMaster->SetValue(t+1,MasterPointContainer[t].first);
                }
                for (unsigned int t=0;t<SlavePointContainer.size();++t)
                {
                    InterpolationPointsSlave->SetValue(t+1,SlavePointContainer[t]);
                }

                /*CheckPoints(InterpolationPointsMaster);*/
                /*CheckPoints(InterpolationPointsSlave);*/

                m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPointsSlave,true));
                //Store the last point
                lastPoint = MasterPointContainer.rbegin()->first;
            }//Current Curve > last curve is finished here
        }
    }
    return true;
}


bool cutting_tools::CheckPoints(Handle(TColgp_HArray1OfPnt) PointArray)
{
    Standard_Integer ii ;
    Standard_Real tolerance_squared = Precision::Confusion() * Precision::Confusion(),
                                      distance_squared ;
    Standard_Boolean result = Standard_True ;
    for (ii = PointArray->Lower() ; result && ii < PointArray->Upper() ; ii++)
    {
        distance_squared = PointArray->Value(ii).SquareDistance(PointArray->Value(ii+1)) ;
        if (distance_squared < tolerance_squared)
        {
            return false;
        }

    }
    return true;

}
Base::BoundBox3f cutting_tools::getWireBBox(TopoDS_Wire aWire)
{
    //Fill Bounding Boxes with Wires
    //Therefore we have to evaluate some points on our wire and feed the BBox Algorithm
    Base::BoundBox3f currentBox;
    BRepAdaptor_CompCurve2 wireAdaptor(aWire);
    GCPnts_QuasiUniformDeflection aProp(wireAdaptor,0.1);
    Base::Vector3f aPoint;
    currentBox.Flush();
    for (int j=1;j<=aProp.NbPoints();++j)
    {
        aPoint.x = float(aProp.Value(j).X());
        aPoint.y = float(aProp.Value(j).Y());
        aPoint.z = float(aProp.Value(j).Z());
        currentBox.Add(aPoint);
    }
    return currentBox;
}

TopoDS_Shape cutting_tools::getProperCut(TopoDS_Shape& aShape)
{
    //A cutting Shape is aShape
    //check direction to decide which Topology to hold and which to delete
    if (m_direction)//From top to bottom
    {

    }
    TopoDS_Shape aReturnShape;
    return aReturnShape;
}

bool cutting_tools::calculateAccurateSlaveZLevel(std::vector<std::pair<gp_Pnt,double> >& OffsetPoints, double current_z_level, double &slave_z_level, double &average_sheet_thickness, double &average_angle, bool &cutpos)
{
    //Mittelwert von allen Normalenwinkeln und damit dann den Mittelwert der Blechdicke bilden

    bool direction,area;
    //Zunächst checken in welchem Bereich vom Teil wir uns aufhalten
    if (current_z_level < m_MachiningOrder[0].first.z && current_z_level > m_MachiningOrder[1].first.z)
    {
        //Wir sind von oben nach unten im ersten Teil
        direction = true;
        area = true;
    }
    else if (m_MachiningOrder.size() > 2 && current_z_level < m_MachiningOrder[1].first.z)
    {
        //Wir sind von oben nach unten im zweiten Teil
        direction = true;
        area = false;
    }
    else if (m_MachiningOrder.size() > 2 && current_z_level < m_MachiningOrder[2].first.z && current_z_level > m_MachiningOrder[1].first.z)
    {
        //Wir sind von unten nach oben im 2. Teil
        direction = false;
        area = false;

    }
    else
    {
        cout << "Konnte keine Zuordnung finden" << endl;
    }

    average_angle = 0.0;
    double slave_z_leveltop,slave_z_levelbottom;

    for (unsigned int i=0;i<OffsetPoints.size();++i)
    {
        average_angle = average_angle + OffsetPoints[i].second;
    }

    average_angle = average_angle/OffsetPoints.size();
    //Average Sheet thickness based on sin-law
    average_sheet_thickness = m_UserSettings.sheet_thickness * sin ((PI/2)-average_angle);


    slave_z_leveltop = current_z_level + \
                       (m_UserSettings.master_radius*(1-sin(average_angle))) - \
                       (sin(average_angle)*(average_sheet_thickness+m_UserSettings.slave_radius)) + \
                       ((average_sheet_thickness+m_UserSettings.slave_radius)/cos(average_angle));

    slave_z_levelbottom = current_z_level - ( m_UserSettings.master_radius + \
                          m_UserSettings.slave_radius + \
                          average_sheet_thickness) * \
                          sin(average_angle) - \
                          m_UserSettings.master_radius;

    if (direction && area && slave_z_leveltop < (m_MachiningOrder[0].first.z - m_UserSettings.sheet_thickness))
    {
        slave_z_level = slave_z_leveltop;
        cutpos = true;
        return true;
    }
    else if (direction && area && slave_z_leveltop > (m_MachiningOrder[0].first.z-m_UserSettings.sheet_thickness))
    {
        if ((slave_z_levelbottom + m_UserSettings.slave_radius) < (m_MachiningOrder[0].first.z - m_UserSettings.sheet_thickness))
        {
            slave_z_level = slave_z_levelbottom;
            cutpos = false;
            return true;
        }
        else
        {
            slave_z_level = m_MachiningOrder[0].first.z;
            return true;
        }
    }
    //Die oberste Ebene ist hier abgehakt
    if (direction && !area && slave_z_leveltop < (m_MachiningOrder[1].first.z - m_UserSettings.sheet_thickness))
    {
        slave_z_level = slave_z_leveltop;
        cutpos = true;
        return true;
    }
    else if (direction && !area && slave_z_leveltop > (m_MachiningOrder[1].first.z-m_UserSettings.sheet_thickness))
    {
        if ((slave_z_levelbottom + m_UserSettings.slave_radius) < (m_MachiningOrder[1].first.z - m_UserSettings.sheet_thickness))
        {
            slave_z_level = slave_z_levelbottom;
            cutpos = false;
            return true;
        }
        else
        {
            slave_z_level = m_MachiningOrder[1].first.z;
            return true;
        }
    }
    //Jetzt noch die Geschichte wo wir wieder nach oben fahren
    if (!direction && !area && slave_z_leveltop < (m_MachiningOrder[1].first.z - m_UserSettings.sheet_thickness))
    {
        slave_z_level = slave_z_leveltop;
        cutpos = true;
        return true;
    }
    else if (direction && !area && slave_z_leveltop > (m_MachiningOrder[2].first.z-m_UserSettings.sheet_thickness))
    {
        if ((slave_z_levelbottom + m_UserSettings.slave_radius) < (m_MachiningOrder[2].first.z - m_UserSettings.sheet_thickness))
        {
            slave_z_level = slave_z_levelbottom;
            cutpos = false;
            return true;
        }
        else
        {
            slave_z_level = m_MachiningOrder[2].first.z;
            return true;
        }
    }

    return  true;
}

//bool cutting_tools::OffsetWires() //Hier ist die alte Version
//{
// for(m_it = m_all_cuts.begin();m_it<m_all_cuts.end();++m_it)
// {
//  // make your wire looks like a curve to other algorithm and generate Points to offset the curve
//  BRepAdaptor_CompCurve2 wireAdaptor((*m_it).second);
//  GCPnts_QuasiUniformDeflection aProp(wireAdaptor,0.001);
//  int numberofpoints = aProp.NbPoints();
//  Handle(TColgp_HArray1OfPnt) finalOffsetPoints = new TColgp_HArray1OfPnt(1, numberofpoints);
//  //Now project the points to the surface. As we might have many faces just project to every face and take the min distance
//  for (int i=1;i<=numberofpoints;++i)
//  {
//   TopExp_Explorer ExShape;
//   ExShape.Init(m_Shape,TopAbs_FACE);
//   Standard_Real Umin,Vmin;
//   double distance_old,distance;
//   Handle_Geom_Surface nearest_surface;
//   distance_old = 200.0;
//   for (; ExShape.More(); ExShape.Next())
//   {
//    const TopoDS_Face &atopo_surface =  TopoDS::Face (ExShape.Current());
//    Handle_Geom_Surface geom_surface = BRep_Tool::Surface(atopo_surface);
//    gp_Pnt currentPoint = aProp.Value(i);
//    GeomAPI_ProjectPointOnSurf aPPS(currentPoint,geom_surface,0.001);
//    if (! aPPS.IsDone())
//    {
//     return false;
//    }
//    distance = aPPS.LowerDistance();
//    if(distance<distance_old)
//    {
//     distance_old = distance;
//     aPPS.LowerDistanceParameters (Umin,Vmin);
//     nearest_surface = geom_surface;
//    }
//   }
//   gp_Pnt projectedPoint,OffsetPoint;
//   gp_Vec Uvec,Vvec,normalVec,projPointVec;
//   nearest_surface->D1(Umin,Vmin,projectedPoint,Uvec,Vvec);
//   normalVec = Uvec;
//   normalVec.Cross(Vvec);
//   normalVec.Normalize(); //Jetzt ist die Normale berechnet und auch normalisiert
//   //zeroPoint.SetCoord(0.0,0.0,0.0); //
//   projPointVec.SetXYZ(projectedPoint.XYZ());
//   OffsetPoint.SetXYZ((projPointVec + (normalVec*(-21))).XYZ());
//   finalOffsetPoints->SetValue(i,OffsetPoint); //Aktuellen OffsetPoint setzen
//  }
//  Standard_Boolean isPeriodic = Standard_False;
//  GeomAPI_Interpolate aNoPeriodInterpolate(finalOffsetPoints, isPeriodic, Precision::Confusion());
//  aNoPeriodInterpolate.Perform();
//  Handle_Geom_BSplineCurve aCurve(aNoPeriodInterpolate.Curve());
//
//  // check results
//  if (!aNoPeriodInterpolate.IsDone()) return false;
//  m_all_offset_cuts_high.push_back(aCurve);
// }
//}

gp_Dir cutting_tools::getPerpendicularVec(gp_Vec& anInput)
{
    double x,y;
    gp_Dir Output;
    x = anInput.X();
    y = anInput.Y();
    if (x != 0.0 && y != 0.0)
    {
        Output.SetCoord(1.0,(-x/y),0.0);
    }
    else if (x == 0 && y == 0)
    {
        Output.SetCoord(1.0,1.0,0.0);
    }
    else if (x == 0 && y != 0)
    {
        Output.SetCoord(1.0,0.0,0.0);
    }
    else if (x != 0 && y == 0)
    {
        Output.SetCoord(0.0,1.0,0.0);
    }
    return Output;
}



bool cutting_tools::OffsetWires_Spiral()
{
    std::ofstream anoutput1;
    anoutput1.open("c:/spiral.txt");
    Base::Builder3D log;
    std::vector<std::pair<float,TopoDS_Shape> >::iterator current_flat_level;
    current_flat_level = m_ordered_cuts.begin();
    SpiralHelper lastPoint;
    lastPoint.SurfacePoint.SetCoord(0.0,0.0,0.0);
    bool slave_done= false; //Necessary if the slave is already output
    bool just_started = true; //
    gp_Vec direction_vector(0.0,0.0,1.0),last_direction_vector(0.0,0.0,1.0);//Just to initialize them
    bool direction = true; //for the Robot, this tells the algo that we have to switch the direction
    //Nicht beim höchsten Anfangen, da wir den nicht mit dem Master fahren wollen
    for (m_ordered_cuts_it = m_ordered_cuts.begin()+1;m_ordered_cuts_it!=m_ordered_cuts.end();++m_ordered_cuts_it)
    {
        std::vector<SpiralHelper> OffsetSpiralPoints,TempSpiralPoints;
        std::vector<gp_Pnt> SlavePointContainer;
        OffsetSpiralPoints.clear();
        TempSpiralPoints.clear();

        //Now we have to select which strategy to choose
        //if the current levels is bigger,the same, or less then the previous one
        if (m_ordered_cuts_it->first<(m_ordered_cuts_it-1)->first)
        {
            //Master is calculated as Usual, Slave stays at the currently highest level
            //Check if Last Level has got a Wire as we go from the last to the current with our Spiral
            SpiralHelper aSpiralStruct;
            double CurveLength = 0.0;
            if ((m_ordered_cuts_it-1)->second.ShapeType() == TopAbs_WIRE)
            {
                WireExplorer aWireExplorer(TopoDS::Wire((m_ordered_cuts_it-1)->second));
                for (aWireExplorer.Init();aWireExplorer.More();aWireExplorer.Next())
                {
                    CurveLength = CurveLength + GetEdgeLength(aWireExplorer.Current());
                    BRepAdaptor_Curve curveAdaptor(aWireExplorer.Current());
                    //Adjust the point amount based on the curve length make every 0.3mm a point
                    //GCPnts_QuasiUniformAbscissa aProp(curveAdaptor,int(GetEdgeLength(aWireExplorer.Current())/0.4));
                    GCPnts_QuasiUniformAbscissa aProp(curveAdaptor,1000);
                    for (int i=1;i<=aProp.NbPoints();++i)
                    {
                        //Check the direction
                        if (aWireExplorer.Current().Orientation() != TopAbs_REVERSED)
                            curveAdaptor.D1(aProp.Parameter(i),aSpiralStruct.SurfacePoint,aSpiralStruct.LineD1);
                        else curveAdaptor.D1(aProp.Parameter(aProp.NbPoints()-i+1),aSpiralStruct.SurfacePoint,aSpiralStruct.LineD1);

                        aSpiralStruct.SurfaceNormal.SetCoord(0.0,0.0,1.0);
                        TempSpiralPoints.push_back(aSpiralStruct);
                    }
                }
            }
            else
            {
                CurveLength = 0.0;
                Edgesort aCutShapeSorter((m_ordered_cuts_it-1)->second);
                for (aCutShapeSorter.Init();aCutShapeSorter.More();aCutShapeSorter.Next())
                {
                    CurveLength = CurveLength + GetEdgeLength(aCutShapeSorter.Current());
                    //Get the PCurve and the GeomSurface
                    Handle_Geom2d_Curve a2DCurve;
                    Handle_Geom_Surface aSurface;
                    TopLoc_Location aLoc;
                    double first2,last2;
                    bool reversed = false;
                    BRep_Tool::CurveOnSurface(aCutShapeSorter.Current(),a2DCurve,aSurface,aLoc,first2,last2);
                    //Jetzt noch die resultierende Surface und die Curve sauber drehen
                    //(vielleicht wurde ja das TopoDS_Face irgendwie gedreht oder die TopoDS_Edge)
                    if (aCutShapeSorter.Current().Orientation() == TopAbs_REVERSED)
                        reversed = true;

                    BRepAdaptor_Curve aCurveAdaptor(aCutShapeSorter.Current());
                    GCPnts_QuasiUniformAbscissa aPointGenerator(aCurveAdaptor,1000);
                    int PointSize = aPointGenerator.NbPoints();
                    //Now get the surface normal to the generated points
                    for (int i=1;i<=PointSize;++i)
                    {
                        gp_Pnt2d a2dParaPoint;
                        gp_Pnt aSurfacePoint;
                        TopoDS_Face aFace;
                        gp_Vec Uvec,Vvec,normalVec;
                        //If the curve is reversed we also have to reverse the point direction
                        if (reversed)
                        {
                            a2DCurve->D0(aPointGenerator.Parameter(PointSize-i+1),a2dParaPoint);
                            aCurveAdaptor.D1(aPointGenerator.Parameter(PointSize-i+1),aSurfacePoint,aSpiralStruct.LineD1);
                        }
                        else
                        {
                            a2DCurve->D0(aPointGenerator.Parameter(i),a2dParaPoint);
                            aCurveAdaptor.D1(aPointGenerator.Parameter(i),aSurfacePoint,aSpiralStruct.LineD1);
                        }
                        GeomAdaptor_Surface aGeom_Adaptor(aSurface);
                        aGeom_Adaptor.D1(a2dParaPoint.X(),a2dParaPoint.Y(),aSurfacePoint,Uvec,Vvec);
                        //Jetzt den Normalenvector auf die Fläche ausrechnen
                        normalVec = Uvec;
                        normalVec.Cross(Vvec);
                        normalVec.Normalize();
                        //Jetzt ist die Normale berechnet und auch normalisiert
                        //Jetzt noch checken ob die Normale auch wirklich auf die saubere Seite zeigt
                        //dazu nur checken ob der Z-Wert der Normale größer Null ist (dann im 1.und 2. Quadranten)
                        if (normalVec.Z()<0)
                            normalVec.Multiply(-1.0);

                        //Mal kurz den Winkel zur Grund-Ebene ausrechnen
                        gp_Vec planeVec(normalVec.X(),normalVec.Y(),0.0);
                        //Den Winkel speichern
                        float angle = float(normalVec.Angle(planeVec));
                        aSpiralStruct.SurfaceNormal = normalVec;
                        aSpiralStruct.SurfacePoint = aSurfacePoint;
                        TempSpiralPoints.push_back(aSpiralStruct);
                    }
                }
            }

            //Now we have to find the shortest distance to the lastPoint of the previous spiral.
            //At the beginning the lastPoint is the Origin
            //This represents our startPoint. If we just started, then we skip this point

            unsigned int start_index = 0,adapted_start_index=0;
            float dist,distold = FLT_MAX;
            for (unsigned int t=0;t<TempSpiralPoints.size();t++)
            {
                dist = float(TempSpiralPoints[t].SurfacePoint.SquareDistance(lastPoint.SurfacePoint));
                if (dist<distold)
                {
                    start_index = t;
                    distold = dist;
                }
            }

            //now we know where to start at our PointCloud, it's the index t

            //Calculate the Slave Toolpath for the current flat area
            if (!slave_done)//if we did not calculate the slave toolpath for the current flat area
            {
                const TopoDS_Wire &aWire = TopoDS::Wire(current_flat_level->second);
                BRepAdaptor_CompCurve2 wireAdaptor(aWire);
                GCPnts_QuasiUniformAbscissa aProp(wireAdaptor,1000);
                SlavePointContainer.clear();
                gp_Pnt SlaveOffsetPoint;
                for (int i=1;i<=aProp.NbPoints();++i)
                {
                    wireAdaptor.D0(aProp.Parameter(i),SlaveOffsetPoint);
                    SlaveOffsetPoint.SetZ(SlaveOffsetPoint.Z() - m_UserSettings.sheet_thickness - m_UserSettings.slave_radius);
                    //checken ob der neue Punkt zu nahe am alten ist. Wenn ja, dann kein push_back
                    if (SlavePointContainer.size()>0 && (SlavePointContainer.rbegin()->SquareDistance(SlaveOffsetPoint)>(Precision::Confusion()*Precision::Confusion())))
                        SlavePointContainer.push_back(SlaveOffsetPoint);
                    else if (SlavePointContainer.empty())
                        SlavePointContainer.push_back(SlaveOffsetPoint);
                }
                //Now Interpolate the Slave with reordered points based on the last point
                int index = 0;
                distold = FLT_MAX;
                for (unsigned int t=0;t<SlavePointContainer.size();t++)
                {
                    dist = float(SlavePointContainer[t].SquareDistance(lastPoint.SurfacePoint));
                    if (dist<distold)
                    {
                        index = t;
                        distold = dist;
                    }
                }
                Handle(TColgp_HArray1OfPnt) InterpolationPoints = new TColgp_HArray1OfPnt(1, SlavePointContainer.size());
                adapted_start_index = start_index+1;
                for (unsigned int t=index+1;t<SlavePointContainer.size();++t)
                {
                    InterpolationPoints->SetValue(t+1,SlavePointContainer[t]);
                    if (t==index)
                        break;
                    if (t+1==SlavePointContainer.size())
                        t=-1;
                }

                m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPoints,true));
                slave_done = true;
            } //Slave done

            gp_Pnt PreviousPoint = TempSpiralPoints[start_index].SurfacePoint;
            //check the current direction we would go
            gp_Vec help_Vec(TempSpiralPoints[start_index].SurfacePoint.Coord());
            if (!start_index+1>=TempSpiralPoints.size())
                direction_vector = TempSpiralPoints[start_index+1].SurfacePoint.Coord();
            else
                direction_vector = TempSpiralPoints[1].SurfacePoint.Coord();//As Start and End-Point are the same
            direction_vector.Subtract(help_Vec);
            //direction_vector.SetZ(0.0);
            direction_vector.Normalize();
            direction_vector.Multiply(1.0); //Zum testen bauen wir keinen Offset ein
            //switch the Spiral-direction if the clockwise flag is checked and the
            //angle between the last and current round is more then 90°
            double angle = direction_vector.Angle(last_direction_vector);
            if (m_UserSettings.clockwise && angle<(D_PI*0.5))
                direction = true; //We are already in the wright direction
            else if (m_UserSettings.clockwise && angle >(D_PI*0.5))
                direction = false; //we have to switch the direction
            else if (!m_UserSettings.clockwise && angle<(D_PI*0.5))
                direction = false;
            else if (!m_UserSettings.clockwise && angle>(D_PI*0.5))
                direction = true;


            IntCurvesFace_ShapeIntersector anIntersector;
            anIntersector.Load(m_Shape,0.01);

            //Insert the first point into the TempSpiralPoints if we have just started
            std::vector<SpiralHelper> TempSpiralPointsFinal;
            TempSpiralPointsFinal.clear();
            if (!just_started)
                TempSpiralPointsFinal.push_back(lastPoint);
            else
                just_started = false;//The first point is now done

            if (direction && ((start_index+1)<TempSpiralPoints.size()))
                adapted_start_index = start_index+1;
            else if (direction && ((start_index+1)>=TempSpiralPoints.size()))
                adapted_start_index = 1;//Not 0 because the last one is already added
            else
            {
                if (start_index==0)
                    adapted_start_index = TempSpiralPoints.size()-2; //Skip the last Point as its equal to the first one
                else
                    adapted_start_index = start_index-1;
            }
            std::cout<<"Angle ="<< angle<< " Direction = "<< direction << "Start_Index: "<< adapted_start_index <<","<<start_index<<std::endl;
            double distance=0.0;
            for (unsigned int j=adapted_start_index;j<TempSpiralPoints.size();++j)
            {
                distance = distance + PreviousPoint.Distance(TempSpiralPoints[j].SurfacePoint);
                double delta_z = distance * m_UserSettings.level_distance / CurveLength;
                //we have to store the currentPoint for the distance calculation
                //before we change something at its coordinates
                PreviousPoint = TempSpiralPoints[j].SurfacePoint;

                TempSpiralPoints[j].SurfacePoint.SetZ(TempSpiralPoints[j].SurfacePoint.Z()-delta_z);
                //before we shoot we check if the z-level is not exactly the same as the next round
                //if (fabs(current_z_value-m_UserSettings.level_distance-TempSpiralPoints[j].SurfacePoint.Z())<=0.04)
                //    break; //We have nearly reached the next z-level, therefore we stop here.
                //Now we have to shoot to get the real Point we want
                //Therefore we have to generate the Direction Vector where to Shoot
                gp_Dir Shooting_Direction = getPerpendicularVec(TempSpiralPoints[j].LineD1);
                gp_Lin aLine(TempSpiralPoints[j].SurfacePoint,Shooting_Direction);
                anIntersector.Perform(aLine,-RealLast(),RealLast());
                if (anIntersector.NbPnt()<=1) //Just to debug
                    anIntersector.Perform(aLine,-RealLast(),RealLast());
                //Now we set the real Surface Point
                //How many Points did we get??
                int current_index;
                int points = anIntersector.NbPnt();
                if (points>0)
                {
                    float shortestDistance, shortestDistanceOld = FLT_MAX;
                    for (int g=1;g<=points;g++)
                    {
                        const gp_Pnt& TestPoint = anIntersector.Pnt(g);
                        shortestDistance = float(TestPoint.SquareDistance(TempSpiralPoints[j].SurfacePoint));
                        if (shortestDistance<shortestDistanceOld)
                        {
                            current_index = g;
                            shortestDistanceOld = shortestDistance;
                        }
                    }
                    //Now we check how far the shortest Distance is. If its more then 14mm then we jump to the
                    //next Point

                    if (shortestDistanceOld>156)//The point is more then 14mm away (square distance)
                        continue;
                    TempSpiralPoints[j].SurfacePoint = anIntersector.Pnt(current_index);
                }
                else //We have to try a mesh intersection as the Nurb Intersection does not seem to work
                {
                    cout << "Big Probleme";
                    continue;
                }
                //Now get the Proper Normal at this point
                BRepAdaptor_Surface aFaceAdaptor(anIntersector.Face(current_index));
                gp_Pnt P;
                gp_Vec U_Vec,V_Vec;
                aFaceAdaptor.D1(anIntersector.UParameter(current_index),anIntersector.VParameter(current_index),P,U_Vec,V_Vec);
                U_Vec.Cross(V_Vec);
                U_Vec.Normalize();
                if (U_Vec.Z() < 0) U_Vec.Multiply(-1.0);

                TempSpiralPoints[j].SurfaceNormal = U_Vec;
                TempSpiralPointsFinal.push_back(TempSpiralPoints[j]);
                //If we reached the end before we processed all points, then we start at the beginning.
                if (direction)
                {
                    if (j==start_index)
                        break; //Now we have completed all Points
                    else if (j+1==TempSpiralPoints.size())
                    {
                        j=-1;    //-1 because at the for we ++ the variable directly
                        continue;
                    }

                }
                else
                {
                    if (j==start_index)
                        break; //Now we have completed all Points
                    else if (j-1<0)
                    {
                        j=TempSpiralPoints.size()-3;
                        continue;
                    }
                    //We switch to the end and skip the last point
                    //as it's the same as the point at j=0;
                    j=j-2;//As the for puts +1 for each step
                }
            }
            //Offset for Master and Slave
            OffsetSpiralPoints = OffsetSpiral(TempSpiralPointsFinal);
            //Now store the lastPoint of the currentSpiral as we need it for the next one
            lastPoint = *(TempSpiralPointsFinal.rbegin());
            gp_Vec temp_vector((OffsetSpiralPoints.rbegin()+5)->SurfacePoint.Coord());
            last_direction_vector = OffsetSpiralPoints.rbegin()->SurfacePoint.Coord();
            last_direction_vector.Subtract(temp_vector);
            //last_direction_vector.SetZ(0.0);
            last_direction_vector.Normalize();
            last_direction_vector.Multiply(10.0);
            Handle(TColgp_HArray1OfPnt) InterpolationPoints = new TColgp_HArray1OfPnt(1,OffsetSpiralPoints.size());
            TColgp_Array1OfVec Tangents(1,OffsetSpiralPoints.size());
            for (unsigned int t=0;t<OffsetSpiralPoints.size();++t)
            {
                InterpolationPoints->SetValue(t+1,OffsetSpiralPoints[t].SurfacePoint);
            }
            bool check = CheckPoints(InterpolationPoints);
            //Here we interpolate. If direction == true this means that the rotation is like the initial rotation
            m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPoints,true));
        }
        if (m_ordered_cuts_it->first==(m_ordered_cuts_it-1)->first)
        {
            //we only set the new flat level wire here
            //no Toolpath is calculated
            current_flat_level = m_ordered_cuts_it;
            slave_done = false; //This is to generate the next flat level for the slave
            just_started = true; //We start a new section
        }//end of current == last
        //Now lets take the case when we move up again
        if (m_ordered_cuts_it->first>(m_ordered_cuts_it-1)->first)
        {
            //Slave is calculated as Usual, Master stays at the currently highest level
            //Check if Last Level has got a Wire as we go from the last to the current with our Spiral
            SpiralHelper aSpiralStruct;
            double CurveLength = 0.0;
            if ((m_ordered_cuts_it-1)->second.ShapeType() == TopAbs_WIRE)
            {
                WireExplorer aWireExplorer(TopoDS::Wire((m_ordered_cuts_it-1)->second));
                for (aWireExplorer.Init();aWireExplorer.More();aWireExplorer.Next())
                {
                    CurveLength = CurveLength + GetEdgeLength(aWireExplorer.Current());
                    BRepAdaptor_Curve curveAdaptor(aWireExplorer.Current());
                    //Adjust the point amount based on the curve length make every 0.3mm a point
                    //GCPnts_QuasiUniformAbscissa aProp(curveAdaptor,int(GetEdgeLength(aWireExplorer.Current())/0.4));
                    GCPnts_QuasiUniformAbscissa aProp(curveAdaptor,1000);
                    for (int i=1;i<=aProp.NbPoints();++i)
                    {
                        //Check the direction
                        if (aWireExplorer.Current().Orientation() != TopAbs_REVERSED)
                            curveAdaptor.D1(aProp.Parameter(i),aSpiralStruct.SurfacePoint,aSpiralStruct.LineD1);
                        else curveAdaptor.D1(aProp.Parameter(aProp.NbPoints()-i+1),aSpiralStruct.SurfacePoint,aSpiralStruct.LineD1);

                        aSpiralStruct.SurfaceNormal.SetCoord(0.0,0.0,1.0);
                        TempSpiralPoints.push_back(aSpiralStruct);
                    }
                }
            }
            else
            {
                CurveLength = 0.0;
                Edgesort aCutShapeSorter((m_ordered_cuts_it-1)->second);
                for (aCutShapeSorter.Init();aCutShapeSorter.More();aCutShapeSorter.Next())
                {
                    CurveLength = CurveLength + GetEdgeLength(aCutShapeSorter.Current());
                    //Get the PCurve and the GeomSurface
                    Handle_Geom2d_Curve a2DCurve;
                    Handle_Geom_Surface aSurface;
                    TopLoc_Location aLoc;
                    double first2,last2;
                    bool reversed = false;
                    BRep_Tool::CurveOnSurface(aCutShapeSorter.Current(),a2DCurve,aSurface,aLoc,first2,last2);
                    //Jetzt noch die resultierende Surface und die Curve sauber drehen
                    //(vielleicht wurde ja das TopoDS_Face irgendwie gedreht oder die TopoDS_Edge)
                    if (aCutShapeSorter.Current().Orientation() == TopAbs_REVERSED)
                        reversed = true;

                    BRepAdaptor_Curve aCurveAdaptor(aCutShapeSorter.Current());
                    GCPnts_QuasiUniformAbscissa aPointGenerator(aCurveAdaptor,1000);
                    int PointSize = aPointGenerator.NbPoints();
                    //Now get the surface normal to the generated points
                    for (int i=1;i<=PointSize;++i)
                    {
                        gp_Pnt2d a2dParaPoint;
                        gp_Pnt aSurfacePoint;
                        TopoDS_Face aFace;
                        gp_Vec Uvec,Vvec,normalVec;
                        //If the curve is reversed we also have to reverse the point direction
                        if (reversed)
                        {
                            a2DCurve->D0(aPointGenerator.Parameter(PointSize-i+1),a2dParaPoint);
                            aCurveAdaptor.D1(aPointGenerator.Parameter(PointSize-i+1),aSurfacePoint,aSpiralStruct.LineD1);
                        }
                        else
                        {
                            a2DCurve->D0(aPointGenerator.Parameter(i),a2dParaPoint);
                            aCurveAdaptor.D1(aPointGenerator.Parameter(i),aSurfacePoint,aSpiralStruct.LineD1);
                        }
                        GeomAdaptor_Surface aGeom_Adaptor(aSurface);
                        aGeom_Adaptor.D1(a2dParaPoint.X(),a2dParaPoint.Y(),aSurfacePoint,Uvec,Vvec);
                        //Jetzt den Normalenvector auf die Fläche ausrechnen
                        normalVec = Uvec;
                        normalVec.Cross(Vvec);
                        normalVec.Normalize();
                        //Jetzt ist die Normale berechnet und auch normalisiert
                        //Jetzt noch checken ob die Normale auch wirklich auf die saubere Seite zeigt
                        //dazu nur checken ob der Z-Wert der Normale größer Null ist (dann im 1.und 2. Quadranten)
                        if (normalVec.Z()<0)
                            normalVec.Multiply(-1.0);

                        //Mal kurz den Winkel zur Grund-Ebene ausrechnen
                        gp_Vec planeVec(normalVec.X(),normalVec.Y(),0.0);
                        //Den Winkel speichern
                        float angle = float(normalVec.Angle(planeVec));
                        aSpiralStruct.SurfaceNormal = normalVec;
                        aSpiralStruct.SurfacePoint = aSurfacePoint;
                        TempSpiralPoints.push_back(aSpiralStruct);
                    }
                }
            }

            //Now we have to find the shortest distance to the lastPoint of the previous spiral.
            //At the beginning the lastPoint is the Origin
            //This represents our startPoint. If we just started, then we skip this point

            int start_index = 0,adapted_start_index=0;
            float dist,distold = FLT_MAX;
            for (unsigned int t=0;t<TempSpiralPoints.size();t++)
            {
                dist = float(TempSpiralPoints[t].SurfacePoint.SquareDistance(lastPoint.SurfacePoint));
                if (dist<distold)
                {
                    start_index = t;
                    distold = dist;
                }
            }

            //now we know where to start at our PointCloud, it's the index t

            //Calculate the Slave Toolpath for the current flat area
            if (!slave_done)//if we did not calculate the slave toolpath for the current flat area
            {
                const TopoDS_Wire &aWire = TopoDS::Wire(current_flat_level->second);
                BRepAdaptor_CompCurve2 wireAdaptor(aWire);
                GCPnts_QuasiUniformAbscissa aProp(wireAdaptor,1000);
                SlavePointContainer.clear();
                gp_Pnt SlaveOffsetPoint;
                for (int i=1;i<=aProp.NbPoints();++i)
                {
                    wireAdaptor.D0(aProp.Parameter(i),SlaveOffsetPoint);
                    SlaveOffsetPoint.SetZ(SlaveOffsetPoint.Z() + m_UserSettings.master_radius);
                    //checken ob der neue Punkt zu nahe am alten ist. Wenn ja, dann kein push_back
                    if (SlavePointContainer.size()>0 && (SlavePointContainer.rbegin()->SquareDistance(SlaveOffsetPoint)>(Precision::Confusion()*Precision::Confusion())))
                        SlavePointContainer.push_back(SlaveOffsetPoint);
                    else if (SlavePointContainer.empty())
                        SlavePointContainer.push_back(SlaveOffsetPoint);
                }

                //Now Interpolate the Slave with reordered points based on the last point
                Handle(TColgp_HArray1OfPnt) InterpolationPoints = new TColgp_HArray1OfPnt(1, SlavePointContainer.size());
                adapted_start_index = start_index+1;
                for (int t=adapted_start_index;t<SlavePointContainer.size();++t)
                {
                    InterpolationPoints->SetValue(t+1,SlavePointContainer[t]);
                    if (t==start_index)
                        break;
                    if (t+1==SlavePointContainer.size())
                        t=-1;
                }

                m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPoints,true));
                slave_done = true;
            } //Slave done

            gp_Pnt PreviousPoint = TempSpiralPoints[start_index].SurfacePoint;
            //check the current direction we would go
            gp_Vec help_Vec(TempSpiralPoints[start_index].SurfacePoint.Coord());
            direction_vector = TempSpiralPoints[start_index+1].SurfacePoint.Coord();
            direction_vector.Subtract(help_Vec);
            //direction_vector.SetZ(0.0);
            direction_vector.Normalize();
            direction_vector.Multiply(10.0);
            //switch the Spiral-direction if the clockwise flag is checked and the
            //angle between the last and current round is more then 90°
            double angle = direction_vector.Angle(last_direction_vector);

            if (m_UserSettings.clockwise && angle<(D_PI*0.5))
                direction = true; //We are already in the wright direction
            else if (m_UserSettings.clockwise && angle >(D_PI*0.5))
                direction = false; //we have to switch the direction
            else if (!m_UserSettings.clockwise && angle<(D_PI*0.5))
                direction = false;
            else if (!m_UserSettings.clockwise && angle>(D_PI*0.5))
                direction = true;


            IntCurvesFace_ShapeIntersector anIntersector;
            anIntersector.Load(m_Shape,0.01);

            //Insert the first point into the TempSpiralPoints if we have just started
            std::vector<SpiralHelper> TempSpiralPointsFinal;
            TempSpiralPointsFinal.clear();
            gp_Pnt origin(0.0,0.0,0.0);
            if (!just_started)
                TempSpiralPointsFinal.push_back(lastPoint);
            else
                just_started=false;

            if (direction)
                adapted_start_index = start_index+1;
            else
            {
                if (start_index==0)
                    adapted_start_index = TempSpiralPoints.size()-2; //Skip the last Point as its equal to the first one
                else
                    adapted_start_index = start_index-1;
            }
            std::cout<<"Angle ="<< angle<< " Direction = "<< direction << "Start_Index: "<< adapted_start_index <<","<<start_index<<std::endl;
            double distance=0.0;
            for (int j=adapted_start_index;j<TempSpiralPoints.size();++j)
            {
                distance = distance + PreviousPoint.Distance(TempSpiralPoints[j].SurfacePoint);
                double delta_z = distance * m_UserSettings.level_distance / CurveLength;
                //we have to store the currentPoint for the distance calculation
                //before we change something at its coordinates
                PreviousPoint = TempSpiralPoints[j].SurfacePoint;

                TempSpiralPoints[j].SurfacePoint.SetZ(TempSpiralPoints[j].SurfacePoint.Z()+delta_z);
                //before we shoot we check if the z-level is not exactly the same as the next round
                //if (fabs(current_z_value-m_UserSettings.level_distance-TempSpiralPoints[j].SurfacePoint.Z())<=0.04)
                //    break; //We have nearly reached the next z-level, therefore we stop here.
                //Now we have to shoot to get the real Point we want
                //Therefore we have to generate the Direction Vector where to Shoot
                gp_Dir Shooting_Direction = getPerpendicularVec(TempSpiralPoints[j].LineD1);
                gp_Lin aLine(TempSpiralPoints[j].SurfacePoint,Shooting_Direction);
                anIntersector.Perform(aLine,-RealLast(),RealLast());
                if (anIntersector.NbPnt()<=1) //Just to debug
                    anIntersector.Perform(aLine,-RealLast(),RealLast());
                //Now we set the real Surface Point
                //How many Points did we get??
                int current_index;
                int points = anIntersector.NbPnt();
                if (points>0)
                {
                    float shortestDistance, shortestDistanceOld = FLT_MAX;
                    for (int g=1;g<=points;g++)
                    {
                        const gp_Pnt& TestPoint = anIntersector.Pnt(g);
                        shortestDistance = float(TestPoint.SquareDistance(TempSpiralPoints[j].SurfacePoint));
                        if (shortestDistance<shortestDistanceOld)
                        {
                            current_index = g;
                            shortestDistanceOld = shortestDistance;
                        }
                    }
                    //Now we check how far the shortest Distance is. If its more than 5mm then we jump to the
                    //next Point

                    if (shortestDistanceOld>50 )
                        continue;
                    TempSpiralPoints[j].SurfacePoint = anIntersector.Pnt(current_index);
                }
                else //We have to try a mesh intersection as the Nurb Intersection does not seem to work
                {
                    cout << "Big Probleme";
                    continue;
                }
                //Now get the Proper Normal at this point
                BRepAdaptor_Surface aFaceAdaptor(anIntersector.Face(current_index));
                gp_Pnt P;
                gp_Vec U_Vec,V_Vec;
                aFaceAdaptor.D1(anIntersector.UParameter(current_index),anIntersector.VParameter(current_index),P,U_Vec,V_Vec);
                U_Vec.Cross(V_Vec);
                U_Vec.Normalize();
                if (U_Vec.Z() < 0) U_Vec.Multiply(-1.0);

                TempSpiralPoints[j].SurfaceNormal = U_Vec;
                TempSpiralPointsFinal.push_back(TempSpiralPoints[j]);
                //If we reached the end before we processed all points, then we start at the beginning.
                if (direction)
                {
                    if (j==start_index)
                        break; //Now we have completed all Points
                    else if (j+1==TempSpiralPoints.size())
                    {
                        j=-1;    //-1 because at the for we ++ the variable directly
                        continue;
                    }

                }
                else
                {
                    if (j==start_index)
                        break; //Now we have completed all Points
                    else if (j-1<0)
                    {
                        j=TempSpiralPoints.size()-3;
                        continue;
                    }
                    //We switch to the end and skip the last point
                    //as it's the same as the point at j=0;
                    j=j-2;//As the for puts +1 for each step
                }
            }
            //Offset for the slave
            OffsetSpiralPoints = OffsetSpiral(TempSpiralPointsFinal,false);
            //Now store the lastPoint of the currentSpiral as we need it for the next one
            lastPoint = *(TempSpiralPointsFinal.rbegin());
            gp_Vec temp_vector((OffsetSpiralPoints.rbegin()+5)->SurfacePoint.Coord());
            last_direction_vector = OffsetSpiralPoints.rbegin()->SurfacePoint.Coord();
            last_direction_vector.Subtract(temp_vector);
            //last_direction_vector.SetZ(0.0);
            last_direction_vector.Normalize();
            last_direction_vector.Multiply(10.0);
            Handle(TColgp_HArray1OfPnt) InterpolationPoints = new TColgp_HArray1OfPnt(1,OffsetSpiralPoints.size());
            TColgp_Array1OfVec Tangents(1,OffsetSpiralPoints.size());
            for (unsigned int t=0;t<OffsetSpiralPoints.size();++t)
            {
                InterpolationPoints->SetValue(t+1,OffsetSpiralPoints[t].SurfacePoint);
            }
            bool check = CheckPoints(InterpolationPoints);
            //Here we interpolate. If direction == true this means that the rotation is like the initial rotation
            m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPoints,true));
        }
    }
    anoutput1.close();
    log.saveToFile("C:/normals.iv");
    return true;
}


std::vector<SpiralHelper> cutting_tools::OffsetSpiral(const std::vector<SpiralHelper>& SpiralPoints,bool master_or_slave)
{
    std::vector<SpiralHelper> OffsetPoints;
    SpiralHelper OffsetPoint;
    OffsetPoints.clear();
    //Offset the Points now

    for (unsigned int i=0;i<SpiralPoints.size();++i)
    {
        OffsetPoint = SpiralPoints[i];
        if (master_or_slave)
        {
            OffsetPoint.SurfaceNormal.Multiply(m_UserSettings.master_radius);
            OffsetPoint.SurfacePoint.SetXYZ(OffsetPoint.SurfacePoint.XYZ() + OffsetPoint.SurfaceNormal.XYZ());
        }
        else
        {
            OffsetPoint.SurfaceNormal.Multiply(m_UserSettings.slave_radius+m_UserSettings.sheet_thickness);
            OffsetPoint.SurfacePoint.SetXYZ(OffsetPoint.SurfacePoint.XYZ() - OffsetPoint.SurfaceNormal.XYZ());
        }
        if (OffsetPoints.size()>0 && (OffsetPoints.rbegin()->SurfacePoint.SquareDistance(OffsetPoint.SurfacePoint)>(Precision::Confusion()*Precision::Confusion())) && (fabs(OffsetPoints.rbegin()->SurfacePoint.Z()-OffsetPoint.SurfacePoint.Z())< (m_UserSettings.level_distance/10.0)))
        {
            OffsetPoints.push_back(OffsetPoint);
        }
        else if (OffsetPoints.empty())
        {
            OffsetPoints.push_back(OffsetPoint);
        }
        else if ((i+1==SpiralPoints.size()) && (OffsetPoints.rbegin()->SurfacePoint.SquareDistance(OffsetPoint.SurfacePoint)<(Precision::Confusion()*Precision::Confusion())))
        {
            //This part is necessary as the last point of the current spiral would otherwise not be offset
            OffsetPoints.pop_back();//Delete the two currently last points
            OffsetPoints.pop_back();
            OffsetPoints.push_back(OffsetPoint);//Add the last point
        }
    }
    return OffsetPoints;
}
std::vector<float> cutting_tools::getFlatAreas()
{
    std::vector<float> FlatAreas;
    FlatAreas.clear();
    for (unsigned int i=0;i<m_MachiningOrder.size();++i)
        FlatAreas.push_back(m_MachiningOrder[i].first.z);
    return FlatAreas;
}


bool cutting_tools::OffsetWires_FeatureBased()
{
    std::vector<std::pair<float,TopoDS_Shape> >::iterator current_flat_level;
    gp_Pnt lastPoint(0.0,0.0,0.0); //Initialize the first Point to the Origin
    current_flat_level = m_ordered_cuts.begin();
    bool slave_done= false; //Necessary if the slave is already put out
    //Nicht beim höchsten Anfangen, da wir den nicht mit dem Master fahren wollen
    for (m_ordered_cuts_it = m_ordered_cuts.begin()+1;m_ordered_cuts_it!=m_ordered_cuts.end();++m_ordered_cuts_it)
    {
        std::vector<std::pair<gp_Pnt,double> > MasterPointContainer;
        std::vector<std::vector<std::pair<gp_Pnt,double> > > MasterPointsStorage;
        std::vector<gp_Pnt> SlavePointContainer;
        std::vector<std::vector<gp_Pnt> > SlavePointsStorage;
        std::vector<std::vector<gp_Pnt> >::iterator anIterator1;
        std::vector<std::vector<std::pair<gp_Pnt,double> > >::iterator anIterator2;
        MasterPointsStorage.clear();
        SlavePointsStorage.clear();
        MasterPointContainer.clear();
        SlavePointContainer.clear();
        //Now we have to select which strategy to choose
        //if the current levels is bigger,the same, or less then the previous one
        if (m_ordered_cuts_it->first<(m_ordered_cuts_it-1)->first)
        {
            //Master is calculated as Usual, Slave stays at the currently highest level
            //Check if current Level has got a Wire
            if (m_ordered_cuts_it->second.ShapeType() == TopAbs_WIRE)
            {
                WireExplorer aWireExplorer(TopoDS::Wire(m_ordered_cuts_it->second));
                for (aWireExplorer.Init();aWireExplorer.More();aWireExplorer.Next())
                {
                    BRepAdaptor_Curve curveAdaptor(aWireExplorer.Current());
                    GCPnts_QuasiUniformAbscissa aProp(curveAdaptor,100);
                    for (int i=1;i<=aProp.NbPoints();++i)
                    {
                        std::pair<gp_Pnt,double> aTempPair;
                        //Check the direction
                        if (aWireExplorer.Current().Orientation() != TopAbs_REVERSED)
                            curveAdaptor.D0(aProp.Parameter(i),aTempPair.first);
                        else curveAdaptor.D0(aProp.Parameter(aProp.NbPoints()-i+1),aTempPair.first);
                        aTempPair.first.SetZ(aTempPair.first.Z() + m_UserSettings.master_radius);
                        aTempPair.second = 0.0; //Initialize of Angle
                        //checken ob der neue Punkt zu nahe am alten ist. Wenn ja, dann kein push_back
                        if (MasterPointContainer.size()>0 && (MasterPointContainer.rbegin()->first.SquareDistance(aTempPair.first)>(Precision::Confusion()*Precision::Confusion())))
                        {
                            MasterPointContainer.push_back(aTempPair);
                        }
                        else if (MasterPointContainer.empty())
                        {
                            MasterPointContainer.push_back(aTempPair);
                        }
                    }
                    //Now Interpolate the Points only if we have a non-continuous edge or if we are finished
                    //with all edges
                    bool tangency = true;
                    //If there are more Edges in the wire
                    if (aWireExplorer.MoreEdge())
                    {
                        tangency = CheckEdgeTangency(aWireExplorer.Current(),aWireExplorer.NextEdge());
                        if (!tangency)
                        {
                            //Store all the PointClouds in a StorageVector to arrange the Robot-Movement afterwards
                            MasterPointsStorage.push_back(MasterPointContainer);
                            MasterPointContainer.clear();
                        }
                        else continue;
                    }
                    else
                    {
                        MasterPointsStorage.push_back(MasterPointContainer);
                        MasterPointContainer.clear();
                    }
                }
                //Now check the point-cloud with the shortest distance to "lastPoint"
                int start_index = 0,start_array=0;
                CheckforLastPoint(lastPoint,start_index,start_array,MasterPointsStorage);
                //Now Interpolate the PointClouds...Cloud by Cloud
                //First we have to divide the first PointCloud as it is for sure that we start
                //Somewhere in the middle of it. We will then insert the points at the current start
                if (MasterPointsStorage.size() == 1) //If we have only one PointCloud
                {
                    MasterPointContainer.clear();
                    bool first = true;
                    for (unsigned int i=start_index;i<MasterPointsStorage.begin()->size();i++)
                    {
                        if (i+1 == MasterPointsStorage.begin()->size())
                        {
                            first = false;
                            i=-1;
                            continue;
                        }
                        MasterPointContainer.push_back(MasterPointsStorage[0][i]);
                        if (!first && i == start_index)
                            break;
                    }
                    //Now lets interpolate the Point Cloud
                    Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointContainer.size());
                    for (unsigned int t=0;t<MasterPointContainer.size();++t)
                    {
                        InterpolationPointsMaster->SetValue(t+1,MasterPointContainer[t].first);
                    }
                    m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                    //Store the last point
                    lastPoint = MasterPointContainer.rbegin()->first;
                }
                else //If we have more than one PointCloud
                {
                    anIterator2 = MasterPointsStorage.begin();
                    MasterPointContainer.clear();
                    for (int i=0;i<=start_index;i++)
                    {
                        MasterPointContainer.push_back(MasterPointsStorage[start_array][i]);
                    }
                    MasterPointsStorage.insert((anIterator2+start_array+1),MasterPointContainer);
                    MasterPointContainer.clear();
                    //Reinitialize the Iterators
                    anIterator2 = MasterPointsStorage.begin();
                    for (unsigned int i=start_index;i<MasterPointsStorage[start_array].size();i++)
                    {
                        MasterPointContainer.push_back(MasterPointsStorage[start_array][i]);
                    }
                    MasterPointsStorage.insert((anIterator2+start_array+2),MasterPointContainer);
                    //Reinitialize the Iterators
                    anIterator2 = MasterPointsStorage.begin();
                    //Delete the Original PointCloud
                    MasterPointsStorage.erase(anIterator2+start_array);
                    MasterPointContainer.clear();
                    //Now lets interpolate the Point Clouds
                    //Start at start_array+1 as the insert operations used us to do it like that
                    for (unsigned int j=start_array+1;j<MasterPointsStorage.size();++j)
                    {
                        Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointsStorage[j].size());
                        for (unsigned int t=0;t<MasterPointsStorage[j].size();++t)
                        {
                            InterpolationPointsMaster->SetValue(t+1,MasterPointsStorage[j][t].first);
                        }
                        m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                        if (j+1==MasterPointsStorage.size())
                        {
                            j=-1;
                            continue;
                        }
                        if (j+1==start_array+1)
                            break;
                    }
                    lastPoint = MasterPointsStorage[start_array].rbegin()->first;
                }
            }
            else //If the current Curve is no Wire
            {
                Edgesort aCutShapeSorter(m_ordered_cuts_it->second);
                for (aCutShapeSorter.Init();aCutShapeSorter.More();aCutShapeSorter.Next())
                {
                    //Get the PCurve and the GeomSurface
                    Handle_Geom2d_Curve a2DCurve;
                    Handle_Geom_Surface aSurface;
                    TopLoc_Location aLoc;
                    TopoDS_Edge anEdge;
                    double first2,last2;
                    bool reversed = false;
                    BRep_Tool::CurveOnSurface(aCutShapeSorter.Current(),a2DCurve,aSurface,aLoc,first2,last2);
                    //Jetzt noch die resultierende Surface und die Curve sauber drehen
                    //(vielleicht wurde ja das TopoDS_Face irgendwie gedreht oder die TopoDS_Edge)
                    if (aCutShapeSorter.Current().Orientation() == TopAbs_REVERSED)
                        reversed = true;

                    BRepAdaptor_Curve aCurveAdaptor(aCutShapeSorter.Current());
                    GCPnts_QuasiUniformAbscissa aPointGenerator(aCurveAdaptor,200);
                    int PointSize = aPointGenerator.NbPoints();
                    //Now get the surface normal to the generated points
                    for (int i=1;i<=PointSize;++i)
                    {
                        std::pair<gp_Pnt,double> PointContactPair;
                        gp_Pnt2d a2dParaPoint;
                        gp_Pnt aSurfacePoint;
                        TopoDS_Face aFace;
                        gp_Vec Uvec,Vvec,normalVec;
                        //If the curve is reversed we also have to reverse the point direction
                        if (reversed) a2DCurve->D0(aPointGenerator.Parameter(PointSize-i+1),a2dParaPoint);
                        else a2DCurve->D0(aPointGenerator.Parameter(i),a2dParaPoint);
                        GeomAdaptor_Surface aGeom_Adaptor(aSurface);
                        int t = aGeom_Adaptor.GetType();
                        aGeom_Adaptor.D1(a2dParaPoint.X(),a2dParaPoint.Y(),aSurfacePoint,Uvec,Vvec);
                        //Jetzt den Normalenvector auf die Fläche ausrechnen
                        normalVec = Uvec;
                        normalVec.Cross(Vvec);
                        normalVec.Normalize();
                        //Jetzt ist die Normale berechnet und auch normalisiert
                        //Jetzt noch checken ob die Normale auch wirklich auf die saubere Seite zeigt
                        //dazu nur checken ob der Z-Wert der Normale größer Null ist (dann im 1.und 2. Quadranten)
                        if (normalVec.Z()<0) normalVec.Multiply(-1.0);

                        //Mal kurz den Winkel zur Grund-Ebene ausrechnen
                        gp_Vec planeVec(normalVec.X(),normalVec.Y(),0.0);
                        //Den Winkel
                        PointContactPair.second = normalVec.Angle(planeVec);
                        gp_Vec NormalVecSlave = normalVec;
                        //Jetzt die Z-Komponente auf 0 setzen
                        //normalVec.SetZ(0.0);
                        normalVec.Normalize();
                        //Jetzt die Normale mit folgender Formel multiplizieren für den Master
                        //double multiply = m_UserSettings.master_radius*(1-sin(PointContactPair.second))/cos(PointContactPair.second);
                        double multiply = m_UserSettings.master_radius;
                        normalVec.Multiply(multiply);
                        //Jetzt den OffsetPunkt berechnen
                        PointContactPair.first.SetXYZ(aSurfacePoint.XYZ() + normalVec.XYZ());
                        //Damit wir keine Punkte bekommen die zu nahe beieinander liegen
                        //Den letzten hinzugefügten Punkt suchen
                        if (MasterPointContainer.size()>0 && (MasterPointContainer.rbegin()->first.SquareDistance(PointContactPair.first)>0.001))
                        {
                            MasterPointContainer.push_back(PointContactPair);
                        }
                        else if (MasterPointContainer.empty())
                        {
                            MasterPointContainer.push_back(PointContactPair);
                        }
                    }
                }
                MasterPointsStorage.push_back(MasterPointContainer);
                int start_index_master = 0,start_array_master=0;
                CheckforLastPoint(lastPoint,start_index_master,start_array_master,MasterPointsStorage);
                //First we have to divide the first PointCloud as it is for sure that we start
                //Somewhere in the middle of it. We will then insert the points at the current start
                MasterPointContainer.clear();
                bool first = true;
                for (unsigned int i=start_index_master;i<MasterPointsStorage.begin()->size();i++)
                {
                    if (i+1 == MasterPointsStorage.begin()->size())
                    {
                        first = false;
                        i=-1;
                        continue;
                    }
                    MasterPointContainer.push_back(MasterPointsStorage[0][i]);
                    if (!first && i == start_index_master)
                        break;
                }
                //Now lets interpolate the Point Cloud
                Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointContainer.size());
                for (unsigned int t=0;t<MasterPointContainer.size();++t)
                    InterpolationPointsMaster->SetValue(t+1,MasterPointContainer[t].first);

                m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                //Store the last point
                lastPoint = MasterPointContainer.rbegin()->first;
                //SlaveTool Path finished
            }//end calculation of the master if its not a wire
            //Calculate the Slave Toolpath
            if (!slave_done)//if we did not calculate the slave toolpath for the current flat area
            {
                TopoDS_Wire aWire = TopoDS::Wire(current_flat_level->second);
                BRepAdaptor_CompCurve2 wireAdaptor(aWire);
                GCPnts_QuasiUniformAbscissa aProp(wireAdaptor,1000);
                SlavePointContainer.clear();
                SlavePointsStorage.clear();
                for (int i=1;i<=aProp.NbPoints();++i)
                {
                    gp_Pnt SlaveOffsetPoint;
                    wireAdaptor.D0(aProp.Parameter(i),SlaveOffsetPoint);
                    SlaveOffsetPoint.SetZ(SlaveOffsetPoint.Z() - m_UserSettings.sheet_thickness - m_UserSettings.slave_radius);
                    //checken ob der neue Punkt zu nahe am alten ist. Wenn ja, dann kein push_back
                    if (SlavePointContainer.size()>0 && (SlavePointContainer.rbegin()->SquareDistance(SlaveOffsetPoint)>(Precision::Confusion()*Precision::Confusion())))
                        SlavePointContainer.push_back(SlaveOffsetPoint);
                    else if (SlavePointContainer.empty())
                        SlavePointContainer.push_back(SlaveOffsetPoint);
                }
                SlavePointsStorage.push_back(SlavePointContainer);
                int start_index_slave = 0,start_array_slave=0;
                CheckforLastPoint(lastPoint,start_index_slave,start_array_slave,SlavePointsStorage);
                //First we have to divide the first PointCloud as it is for sure that we start
                //Somewhere in the middle of it. We will then insert the points at the current start
                SlavePointContainer.clear();
                bool first = true;
                for (unsigned int i=start_index_slave;i<SlavePointsStorage.begin()->size();i++)
                {
                    if (i+1 == SlavePointsStorage.begin()->size())
                    {
                        first = false;
                        i=-1;
                        continue;
                    }
                    SlavePointContainer.push_back(SlavePointsStorage[0][i]);
                    if (!first && i == start_index_slave)
                        break;
                }
                //Now Interpolate the Slave, therefore we also have to take the curves normal directions
                Handle(TColgp_HArray1OfPnt) InterpolationPoints = new TColgp_HArray1OfPnt(1, SlavePointContainer.size());
                for (unsigned int t=0;t<SlavePointContainer.size();++t)
                    InterpolationPoints->SetValue(t+1,SlavePointContainer[t]);

                m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPoints,true));
                slave_done = true;
            } //Slave done
        }//Current Curve < last curve is finished here
        if (m_ordered_cuts_it->first==(m_ordered_cuts_it-1)->first)
        {
            //we only set the new flat level wire here
            //no Toolpath is calculated
            current_flat_level = m_ordered_cuts_it;
            slave_done = false; //This is to generate the next flat level for the slave
        }//end of current == last
        if (m_ordered_cuts_it->first>(m_ordered_cuts_it-1)->first)
        {
            //The Slave Tool is now the Master Tool and therefore we only have to exchange < case
            if (m_ordered_cuts_it->second.ShapeType() == TopAbs_WIRE)
            {
                WireExplorer aWireExplorer(TopoDS::Wire(m_ordered_cuts_it->second));
                for (aWireExplorer.Init();aWireExplorer.More();aWireExplorer.Next())
                {
                    BRepAdaptor_Curve curveAdaptor(aWireExplorer.Current());
                    GCPnts_QuasiUniformAbscissa aProp(curveAdaptor,100);
                    for (int i=1;i<=aProp.NbPoints();++i)
                    {
                        std::pair<gp_Pnt,double> aTempPair;
                        //Check the direction
                        if (aWireExplorer.Current().Orientation() != TopAbs_REVERSED)
                            curveAdaptor.D0(aProp.Parameter(i),aTempPair.first);
                        else curveAdaptor.D0(aProp.Parameter(aProp.NbPoints()-i+1),aTempPair.first);
                        aTempPair.first.SetZ(aTempPair.first.Z() - m_UserSettings.slave_radius - m_UserSettings.sheet_thickness);
                        aTempPair.second = 0.0; //Initialize of Angle
                        //checken ob der neue Punkt zu nahe am alten ist. Wenn ja, dann kein push_back
                        if (MasterPointContainer.size()>0 && (MasterPointContainer.rbegin()->first.SquareDistance(aTempPair.first)>(Precision::Confusion()*Precision::Confusion())))
                        {
                            MasterPointContainer.push_back(aTempPair);
                        }
                        else if (MasterPointContainer.empty())
                        {
                            MasterPointContainer.push_back(aTempPair);
                        }
                    }
                    //Now Interpolate the Points only if we have a non-continuous edge or if we are finished
                    //with all edges
                    bool tangency = true;
                    //If there are more Edges in the wire
                    if (aWireExplorer.MoreEdge())
                    {
                        tangency = CheckEdgeTangency(aWireExplorer.Current(),aWireExplorer.NextEdge());
                        if (!tangency)
                        {
                            //Store all the PointClouds in a StorageVector to arrange the Robot-Movement afterwards
                            MasterPointsStorage.push_back(MasterPointContainer);
                            MasterPointContainer.clear();
                        }
                        else continue;
                    }
                    else
                    {
                        MasterPointsStorage.push_back(MasterPointContainer);
                        MasterPointContainer.clear();
                    }
                }
                //Now check the point-cloud with the shortest distance to "lastPoint"
                int start_index = 0,start_array=0;
                CheckforLastPoint(lastPoint,start_index,start_array,MasterPointsStorage);
                //Now Interpolate the PointClouds...Cloud by Cloud
                //First we have to divide the first PointCloud as it is for sure that we start
                //Somewhere in the middle of it. We will then insert the points at the current start
                if (MasterPointsStorage.size() == 1) //If we have only one PointCloud
                {
                    MasterPointContainer.clear();
                    bool first = true;
                    for (unsigned int i=start_index;i<MasterPointsStorage.begin()->size();i++)
                    {
                        if (i+1 == MasterPointsStorage.begin()->size())
                        {
                            first = false;
                            i=-1;
                            continue;
                        }
                        MasterPointContainer.push_back(MasterPointsStorage[0][i]);
                        if (!first && i == start_index)
                            break;
                    }
                    //Now lets interpolate the Point Cloud
                    Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointContainer.size());
                    for (unsigned int t=0;t<MasterPointContainer.size();++t)
                    {
                        InterpolationPointsMaster->SetValue(t+1,MasterPointContainer[t].first);
                    }
                    m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                    //Store the last point
                    lastPoint = MasterPointContainer.rbegin()->first;
                }
                else //If we have more than one PointCloud
                {
                    anIterator2 = MasterPointsStorage.begin();
                    MasterPointContainer.clear();
                    for (int i=0;i<=start_index;i++)
                    {
                        MasterPointContainer.push_back(MasterPointsStorage[start_array][i]);
                    }
                    MasterPointsStorage.insert((anIterator2+start_array+1),MasterPointContainer);
                    MasterPointContainer.clear();
                    //Reinitialize the Iterators
                    anIterator2 = MasterPointsStorage.begin();
                    for (unsigned int i=start_index;i<MasterPointsStorage[start_array].size();i++)
                    {
                        MasterPointContainer.push_back(MasterPointsStorage[start_array][i]);
                    }
                    MasterPointsStorage.insert((anIterator2+start_array+2),MasterPointContainer);
                    //Reinitialize the Iterators
                    anIterator2 = MasterPointsStorage.begin();
                    //Delete the Original PointCloud
                    MasterPointsStorage.erase(anIterator2+start_array);
                    MasterPointContainer.clear();
                    //Now lets interpolate the Point Clouds
                    //Start at start_array+1 as the insert operations used us to do it like that
                    for (unsigned int j=start_array+1;j<MasterPointsStorage.size();++j)
                    {
                        Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointsStorage[j].size());
                        for (unsigned int t=0;t<MasterPointsStorage[j].size();++t)
                        {
                            InterpolationPointsMaster->SetValue(t+1,MasterPointsStorage[j][t].first);
                        }
                        m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                        if (j+1==MasterPointsStorage.size())
                        {
                            j=-1;
                            continue;
                        }
                        if (j+1==start_array+1)
                            break;
                    }
                    lastPoint = MasterPointsStorage[start_array].rbegin()->first;
                }
            }
            else //If the current Curve is no Wire
            {
                Edgesort aCutShapeSorter(m_ordered_cuts_it->second);
                for (aCutShapeSorter.Init();aCutShapeSorter.More();aCutShapeSorter.Next())
                {
                    //Get the PCurve and the GeomSurface
                    Handle_Geom2d_Curve a2DCurve;
                    Handle_Geom_Surface aSurface;
                    TopLoc_Location aLoc;
                    TopoDS_Edge anEdge;
                    double first2,last2;
                    bool reversed = false;
                    BRep_Tool::CurveOnSurface(aCutShapeSorter.Current(),a2DCurve,aSurface,aLoc,first2,last2);
                    //Jetzt noch die resultierende Surface und die Curve sauber drehen
                    //(vielleicht wurde ja das TopoDS_Face irgendwie gedreht oder die TopoDS_Edge)
                    if (aCutShapeSorter.Current().Orientation() == TopAbs_REVERSED)
                        reversed = true;

                    BRepAdaptor_Curve aCurveAdaptor(aCutShapeSorter.Current());
                    GCPnts_QuasiUniformAbscissa aPointGenerator(aCurveAdaptor,200);
                    int PointSize = aPointGenerator.NbPoints();
                    //Now get the surface normal to the generated points
                    for (int i=1;i<=PointSize;++i)
                    {
                        std::pair<gp_Pnt,double> PointContactPair;
                        gp_Pnt2d a2dParaPoint;
                        gp_Pnt aSurfacePoint;
                        TopoDS_Face aFace;
                        gp_Vec Uvec,Vvec,normalVec;
                        //If the curve is reversed we also have to reverse the point direction
                        if (reversed) a2DCurve->D0(aPointGenerator.Parameter(PointSize-i+1),a2dParaPoint);
                        else a2DCurve->D0(aPointGenerator.Parameter(i),a2dParaPoint);
                        GeomAdaptor_Surface aGeom_Adaptor(aSurface);
                        int t = aGeom_Adaptor.GetType();
                        aGeom_Adaptor.D1(a2dParaPoint.X(),a2dParaPoint.Y(),aSurfacePoint,Uvec,Vvec);
                        //Jetzt den Normalenvector auf die Fläche ausrechnen
                        normalVec = Uvec;
                        normalVec.Cross(Vvec);
                        normalVec.Normalize();
                        //Jetzt ist die Normale berechnet und auch normalisiert
                        //Jetzt noch checken ob die Normale auch wirklich auf die saubere Seite zeigt
                        //dazu nur checken ob der Z-Wert der Normale größer Null ist (dann im 1.und 2. Quadranten)
                        if (normalVec.Z()<0) normalVec.Multiply(-1.0);

                        //Mal kurz den Winkel zur Grund-Ebene ausrechnen
                        gp_Vec planeVec(normalVec.X(),normalVec.Y(),0.0);
                        //Den Winkel
                        PointContactPair.second = normalVec.Angle(planeVec);
                        gp_Vec NormalVecSlave = normalVec;
                        //Jetzt die Z-Komponente auf 0 setzen
                        //normalVec.SetZ(0.0);
                        normalVec.Normalize();
                        //Jetzt die Normale mit folgender Formel multiplizieren für den Master
                        //double multiply = m_UserSettings.master_radius*(1-sin(PointContactPair.second))/cos(PointContactPair.second);
                        double multiply = m_UserSettings.slave_radius + m_UserSettings.sheet_thickness;
                        normalVec.Multiply(multiply);
                        normalVec.Multiply(-1.0); //As the master is the slave
                        //Jetzt den OffsetPunkt berechnen
                        PointContactPair.first.SetXYZ(aSurfacePoint.XYZ() + normalVec.XYZ());
                        //Damit wir keine Punkte bekommen die zu nahe beieinander liegen
                        //Den letzten hinzugefügten Punkt suchen
                        if (MasterPointContainer.size()>0 && (MasterPointContainer.rbegin()->first.SquareDistance(PointContactPair.first)>0.001))
                        {
                            MasterPointContainer.push_back(PointContactPair);
                        }
                        else if (MasterPointContainer.empty())
                        {
                            MasterPointContainer.push_back(PointContactPair);
                        }
                    }
                }
                MasterPointsStorage.push_back(MasterPointContainer);
                int start_index_master = 0,start_array_master=0;
                CheckforLastPoint(lastPoint,start_index_master,start_array_master,MasterPointsStorage);
                //First we have to divide the first PointCloud as it is for sure that we start
                //Somewhere in the middle of it. We will then insert the points at the current start
                MasterPointContainer.clear();
                bool first = true;
                for (unsigned int i=start_index_master;i<MasterPointsStorage.begin()->size();i++)
                {
                    if (i+1 == MasterPointsStorage.begin()->size())
                    {
                        first = false;
                        i=-1;
                        continue;
                    }
                    MasterPointContainer.push_back(MasterPointsStorage[0][i]);
                    if (!first && i == start_index_master)
                        break;
                }
                //Now lets interpolate the Point Cloud
                Handle(TColgp_HArray1OfPnt) InterpolationPointsMaster = new TColgp_HArray1OfPnt(1, MasterPointContainer.size());
                for (unsigned int t=0;t<MasterPointContainer.size();++t)
                    InterpolationPointsMaster->SetValue(t+1,MasterPointContainer[t].first);

                m_all_offset_cuts_low.push_back(InterpolateOrderedPoints(InterpolationPointsMaster,true));
                //Store the last point
                lastPoint = MasterPointContainer.rbegin()->first;
                //SlaveTool Path finished
            }//end calculation of the master if its not a wire
            //Calculate the Slave Toolpath
            if (!slave_done)//if we did not calculate the slave toolpath for the current flat area
            {
                TopoDS_Wire aWire = TopoDS::Wire(current_flat_level->second);
                BRepAdaptor_CompCurve2 wireAdaptor(aWire);
                GCPnts_QuasiUniformAbscissa aProp(wireAdaptor,1000);
                SlavePointContainer.clear();
                SlavePointsStorage.clear();
                for (int i=1;i<=aProp.NbPoints();++i)
                {
                    gp_Pnt SlaveOffsetPoint;
                    wireAdaptor.D0(aProp.Parameter(i),SlaveOffsetPoint);
                    SlaveOffsetPoint.SetZ(SlaveOffsetPoint.Z() + m_UserSettings.master_radius);
                    //checken ob der neue Punkt zu nahe am alten ist. Wenn ja, dann kein push_back
                    if (SlavePointContainer.size()>0 && (SlavePointContainer.rbegin()->SquareDistance(SlaveOffsetPoint)>(Precision::Confusion()*Precision::Confusion())))
                    {
                        SlavePointContainer.push_back(SlaveOffsetPoint);
                    }
                    else if (SlavePointContainer.empty())
                    {
                        SlavePointContainer.push_back(SlaveOffsetPoint);
                    }
                }
                SlavePointsStorage.push_back(SlavePointContainer);
                int start_index_slave = 0,start_array_slave=0;
                CheckforLastPoint(lastPoint,start_index_slave,start_array_slave,SlavePointsStorage);
                //First we have to divide the first PointCloud as it is for sure that we start
                //Somewhere in the middle of it. We will then insert the points at the current start
                SlavePointContainer.clear();
                bool first = true;
                for (unsigned int i=start_index_slave;i<SlavePointsStorage.begin()->size();i++)
                {
                    if (i+1 == SlavePointsStorage.begin()->size())
                    {
                        first = false;
                        i=-1;
                        continue;
                    }
                    SlavePointContainer.push_back(SlavePointsStorage[0][i]);
                    if (!first && i == start_index_slave)
                        break;
                }
                //Now Interpolate the Slave, therefore we also have to take the curves normal directions
                Handle(TColgp_HArray1OfPnt) InterpolationPoints = new TColgp_HArray1OfPnt(1, SlavePointContainer.size());
                for (unsigned int t=0;t<SlavePointContainer.size();++t)
                    InterpolationPoints->SetValue(t+1,SlavePointContainer[t]);

                m_all_offset_cuts_high.push_back(InterpolateOrderedPoints(InterpolationPoints,true));
                slave_done = true;
            } //Slave done
        }//Current Curve > Last Curve
    }//Main for Loop which goes through all the curves

    return true;
}

bool cutting_tools::cut_Mesh(float z_level, float min_level, std::list<std::vector<Base::Vector3f> >&result, float &z_level_corrected)
{
    //std::ofstream outfile;
    //outfile.open("c:/mesh_cut.out");
    Base::Vector3f z_level_plane,normal;
    z_level_plane.z=z_level;
    normal.x=0;
    normal.y=0;
    normal.z=1.0;
    bool cutok;
    //Die Richtung für die Korrektur wird hier festgelegt
    bool direction=true;
    float factor = 0.0;
    do
    {
        cutok = true;
        m_aMeshAlgo->CutWithPlane(z_level_plane,normal,*m_CAD_Mesh_Grid,result);
        //std::list<std::vector<Base::Vector3f> >::iterator it;
        //std::vector<Base::Vector3f>::iterator vector_it;
        //checken ob wirklich ein Schnitt zustande gekommen ist
        if (result.size()==0)
        {
            cutok = false;
            //Jedes Mal ein wenig mehr Abstand für die Korrektur einfügen
            factor = factor+float(0.05);
            if (factor>=1) factor = float(0.95);
            //Wenn wir das erste Mal eine Korrektur machen müssen gehts zunächst mal mit Minus rein
            if (direction)
            {
                z_level_plane.z = (z_level-(m_pitch*factor));
                z_level_corrected = z_level_plane.z;
                direction=false;
                continue;
            }
            else
            {
                z_level_plane.z = (z_level+(m_pitch*factor));
                z_level_corrected = z_level_plane.z;
                direction=true;
                continue;
            }
        }
    }
    while (cutok==false);
    //for(vector_it=(*(result.begin())).begin();vector_it<(*(result.begin())).end();++vector_it)
    //outfile << (*vector_it).x <<","<<(*vector_it).y <<","<<(*vector_it).z<< std::endl;
    //outfile.close();
    return true;
}






bool cutting_tools::cut(float z_level, float min_level, TopoDS_Shape &aCutShape, float &z_level_corrected)
{
    gp_Pnt aPlanePnt(0,0,z_level);
    gp_Dir aPlaneDir(0,0,1);
    bool cutok;
    //Die Richtung für die Korrektur wird hier festgelegt
    bool correction=true;
    float factor = 0.0;
    do
    {
        cutok = true;
        Handle_Geom_Plane aPlane = new Geom_Plane(aPlanePnt, aPlaneDir);
        BRepBuilderAPI_MakeFace Face(aPlane);
        BRepAlgo_Section mkCut(m_Shape, Face.Face(),Standard_False);
        mkCut.Approximation (Standard_True);
        mkCut.ComputePCurveOn1(Standard_True);
        mkCut.Build();
        //Den neuen Algorithmus checken
        //Edgesort aSorter(mkCut.Shape());
        //aSorter.Init();

        //Jetzt checken ob auch wirlich edges vorhanden sind
        TopExp_Explorer exploreShape;
        exploreShape.Init(mkCut.Shape(),TopAbs_EDGE);
        //Wenn keine Edge vorhanden ist
        if (!exploreShape.More())
        {
            cutok = false;
            //Jedes Mal ein wenig mehr Abstand für die Korrektur einfügen
            factor = factor+float(0.05);
            if (factor>=1) factor = float(0.95);
            //Wenn wir das erste Mal eine Korrektur machen müssen gehts zunächst mal mit Minus rein
            if (correction)
            {
                aPlanePnt.SetZ(z_level-(m_pitch*factor));
                z_level_corrected = float(aPlanePnt.Z());
                correction=false;
                continue;
            }
            else
            {
                aPlanePnt.SetZ(z_level+(m_pitch*factor));
                z_level_corrected = float(aPlanePnt.Z());
                correction=true;
                continue;
            }
        }
        //Das Shape, welches per Referenz übergeben wird jetzt mit dem geordneten Schnitt füllen
        aCutShape = mkCut.Shape();

    }
    while (cutok==false);

    return true;
}





bool cutting_tools::classifyShape()
{
    TopExp_Explorer Explorer;
    Explorer.Init(m_Shape,TopAbs_FACE);
    if (!Explorer.More()) return false;
    //checken wieviele verschiedene Faces wir haben
    int k=0;
    for (; Explorer.More(); Explorer.Next())
    {
        k++;
    }
    std::cout <<"We have " << k << "Faces" << std::endl;
    //Wenn mehr als ein Face vorhanden, dann eine Membervariable setzen
    if (k>1) m_cad = true;
    return true;
}



/* Hier ging das alte cut los




 bool cutok=true;
    //Falls wir nur ein Face haben und keine flachen Bereiche
    if (m_all_cuts.empty())
    {
        //Schnitte über die Bounding Box bestimmen
        Bnd_Box currentBBox;

        Standard_Real XMin, YMin, ZMin, XMax, YMax, ZMax;
        BRepBndLib::Add(m_Shape, currentBBox );
        currentBBox.SetGap(0.0);
        currentBBox.Get(XMin, YMin, ZMin, XMax, YMax, ZMax);
        double maxlevel=Max(ZMax,ZMin);
        double minlevel=Min(ZMax,ZMin);
        int cutnumber = fabs((maxlevel-minlevel)/m_pitch);//Cast um die Nachkommastellen wegzuschneiden
        m_pitch = fabs(maxlevel-minlevel)/cutnumber;//m_pitch leicht korrigieren um wirklich auf die letzte Ebene zu kommen

  //Aktuell wird die letzte Ebene bei selbst approxmierten Flächen nicht als Bahnkurve betrachtet
  //Auch die erste Ebene fällt komplett weg, da unwichtig. Lediglich für die untere Maschine ist die Bahn sinnvoll falls überhaupt noch flache Bereiche vorhanden sind
        for (int i=0;i<cutnumber;++i)
  {
            //Jetzt schneiden (die oberste Ebene auslassen)
            double z_level = maxlevel-(i*m_pitch);
            gp_Pnt aPlanePnt(0,0,z_level);
            gp_Dir aPlaneDir(0,0,1);

   do
   {
    cutok = true;
    Handle_Geom_Plane aPlane = new Geom_Plane(aPlanePnt, aPlaneDir);
    BRepBuilderAPI_MakeFace Face(aPlane);
    BRepAlgo_Section mkCut(m_Shape, Face.Face(),Standard_False);
    mkCut.Approximation (Standard_True);
    mkCut.Build();
    //Jetzt checken ob auch wirlich edges vorhanden sind
    TopExp_Explorer exploreShape;
    exploreShape.Init(mkCut.Shape(),TopAbs_EDGE);
    //Wenn keine Edge vorhanden ist
    if(!exploreShape.More())
    {
     cutok = false;
     aPlanePnt.SetZ(z_level-(m_pitch/5));
     if(aPlanePnt.Z()<minlevel) aPlanePnt.SetZ(minlevel+m_pitch/5);
     continue;
    }
    //Weil der Punkt sich ja geändert haben könnte
    m_zl_wire_combination.first = aPlanePnt.Z();
    m_zl_wire_combination.second = ordercutShape(mkCut.Shape());
    //Geordnete Edges in den All_Cuts-Vector stecken
    m_all_cuts.push_back(m_zl_wire_combination);
   }while (cutok==false);
  }
        return true;
    }
    //Bei mehreren flachen Bereichen
    else
    {
        std::vector<float> InitialPlaneLevels;
  std::vector<std::pair<float,TopoDS_Wire> > atemp_storage;
  atemp_storage.clear();
        std::vector<float>::iterator temp_it;
        for (m_it= m_all_cuts.begin();m_it<m_all_cuts.end();++m_it)
        {
            //Die Wires filtern und z.B. bei zwei Wires auf der obersten Ebene nur die innere nehmen
   Bnd_Box currentBBox,BBox2;
   Standard_Real X1Min, Y1Min, Z1Min, X1Max, Y1Max, Z1Max,X2Min, Y2Min, Z2Min, X2Max, Y2Max, Z2Max;
   BRepBndLib::Add((*m_it).second, currentBBox );
   currentBBox.SetGap(0.0);
   currentBBox.Get(X1Min, Y1Min, Z1Min, X1Max, Y1Max, Z1Max);
   if((*(m_it+1)).first == (*m_it).first) //Wenn die beiden Wires auf der gleichen Ebene liegen.....
   {
    BRepBndLib::Add((*(m_it+1)).second,BBox2);
    BBox2.SetGap(0.0);
    BBox2.Get(X2Min,Y2Min,Z2Min,X2Max,Y2Max,Z2Max);
   }
   //Jetzt checken welche kleiner ist
   if(X1Min<X2Min && X1Max>X2Max && Y1Min < Y2Min && Y1Max>Y2Max) //1 ist größer
   {
    if(m_it == m_all_cuts.begin())//Wenn wir auf der obersten Ebene sind....
    {
     atemp_storage.push_back(*(m_it+1));
     m_it++;//Wir überspringen damit das nächste //Damit haben wir nur noch ein Problem falls wir mehr als zwei Wires auf einer ebene haben.
    }
    else
    {
     atemp_storage.push_back(*(m_it));
    }
   }
   //Mal schauen ob der Wert schon in der Liste vorhanden ist
            temp_it = std::find(InitialPlaneLevels.begin(),InitialPlaneLevels.end(),(*m_it).first);
            if (temp_it == InitialPlaneLevels.end())
            {
                InitialPlaneLevels.push_back((*m_it).first);
            }
        }
  //Jetzt die flachen Bereiche der Höhe nach sortieren
  std::sort(InitialPlaneLevels.begin(),InitialPlaneLevels.end(),FloatHuge);

  //Die Schnitte müssen jetzt zwischen die flachen Stücke einsortiert werden
  for (temp_it=InitialPlaneLevels.begin();temp_it<InitialPlaneLevels.end();++temp_it)
        {
            //Debug cout << "Bereich" <<endl;
   double maxlevel=*temp_it;//Maximaler aktueller Wert
   if(temp_it+1==InitialPlaneLevels.end()) continue;
   double minlevel=*(temp_it+1);
   int cutnumber = fabs((maxlevel-minlevel)/m_pitch);//Cast um die Nachkommastellen wegzuschneiden
   m_pitch = fabs(maxlevel-minlevel)/cutnumber;

   for (int i=1;i<cutnumber;++i)
   {
    //Jetzt schneiden (die oberste Ebene auslassen und die unterste, da dort wieder wires kommen)
    double z_level = maxlevel-(i*m_pitch);
    gp_Pnt aPlanePnt(0,0,z_level);
    gp_Dir aPlaneDir(0,0,1);
    cutok=true;
    do
    {
     Handle_Geom_Plane aPlane = new Geom_Plane(aPlanePnt, aPlaneDir);
     BRepBuilderAPI_MakeFace Face(aPlane);
     BRepAlgo_Section mkCut(m_Shape, Face.Face(),Standard_False);
     try
     {
      mkCut.Approximation (Standard_True);
      mkCut.Build();
     }
     catch(...)
     {
      cutok = false;
      aPlanePnt.SetZ(z_level-(m_pitch/5));
      continue;
     }

     //Weil der Punkt sich ja geändert haben könnte
     m_zl_wire_combination.first = aPlanePnt.Z();
     m_zl_wire_combination.second = ordercutShape(mkCut.Shape());
     //Geordnete Edges in den All_Cuts-Vector stecken
     m_all_cuts.push_back(m_zl_wire_combination);
    }while (cutok=false);

   }
  }
 }
}
*/

//TopoDS_Compound  cutting_tools::getCutShape()
//{
//  BRep_Builder builder;
//  TopoDS_Compound Comp;
//  builder.MakeCompound(Comp);
////  for(m_it = m_ordered_wires.begin();m_it < m_ordered_wires.end();++m_it)
////  {
////   builder.Add(Comp,(*m_it).second);
////  }
//  return Comp;
//
//}




double cutting_tools::GetEdgeLength(const TopoDS_Edge& anEdge)
{
    GProp_GProps lProps;
    BRepGProp::LinearProperties(anEdge,lProps);
    double length = lProps.Mass();
    return length;
}
