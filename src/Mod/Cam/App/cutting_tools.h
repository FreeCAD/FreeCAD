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
/**\file
\brief Here you can find the header file for all the cutting stuff.

This is the place where all the prototypes are declared and the members are defined
*/
#ifndef Cutting_Tools
#define Cutting_Tools


#include <Handle_Geom_BSplineCurve.hxx>
#include <TColgp_Array1OfVec.hxx>
#include <Handle_TColgp_HArray1OfPnt.hxx>
#include <Base/BoundBox.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <map>

#include "stuff.h"

namespace MeshCore {
class MeshAlgorithm;
class MeshFacetGrid;
}

/**\brief A Container to transfer the GUI settings

This struct can be used to transfer the settings of the CAM-Workbench GUI to other functions if required.
It provides members for all fields of the GUI settings window.
*/
struct CuttingToolsSettings
{
    /**This represents the maximum allowed angle for the springback functions*/
    float limit_angle; 
    /**This represents the minimum CAD-Radius of the forming shape. This is necessary for the springback
    to avoid the generation of radii which are below that value*/
    float cad_radius; 
    /**This represents the radius of the Master Tool*/
    float master_radius; 
    /**This represents the radius of the Slave Tool*/
    float slave_radius;
    /**This represents the cutting distance between two levels (the pitch)*/
    float level_distance;
    /**This represents the springback correction factor used for the mirror part*/
    float correction_factor;
    /**This represents the sheet thickness*/
    float sheet_thickness;
    /**This represents the maximum velocity for the simulation output*/
    float max_Vel;
    /**This represents the maximum acceleration for the simulation output*/
    float max_Acc;
    /**This represents the pretension of the spring if used during simulation in mm*/
    int spring_pretension;
    /**This represents the Y-Offset value for the robot output*/
    float y_offset_robot;
    /**This represents the X-Offset value for the robot output*/
    float x_offset_robot;
    /**This represents the error of the approximated NURB surface*/ 
    float error_tolerance;
    
    /**This value is necessary to tell some special functions if we move zig/zag or clockwise/counterclockwise
    without changing direction after each step*/
    bool clockwise;
};

/**\brief A Container used for the Spiral-Toolpath Generation*/
struct SpiralHelper
{
    /**This represents the SurfacePoint of the CAD-Surface*/
    gp_Pnt SurfacePoint;
    /**This represents the direction vector between two following points*/
    gp_Vec LineD1;
    /**This represents the SurfaceNormal at the SurfacePoint*/
    gp_Vec SurfaceNormal;
};

struct BoundBox3f_Less
{
    bool operator()(const Base::BoundBox3f& _Left, const Base::BoundBox3f& _Right) const
    {
        if (_Left.IsInBox(_Right)) return false;

        return true;
    }
};

struct Face_Less
{
    bool operator()(const TopoDS_Face& _Left, const TopoDS_Face& _Right) const
    {
        return(_Left.HashCode(IntegerLast())<_Right.HashCode(IntegerLast()));
    }
};




/**\brief This class is the main class for the cutting Algorithms. 

Its idea is to provide the basic functionality for cutting CAD surfaces. In this class you can also find 
functions useful for generating spiral and feature based toolpaths
*/
class CamExport cutting_tools
{
public:
	/**\brief The standard constructor 
	
	\param aShape A TopoDS_Shape 
	*/ 
    cutting_tools(TopoDS_Shape aShape);

    /**\brief The second standard constructor

    This one gets a vertical step down value as well
	\param aShape A TopoDS_Shape 
    \param pitch A vertical step down in[mm]
	*/ 
    cutting_tools(TopoDS_Shape aShape, float pitch);

    /**\brief The standard destructor*/
    ~cutting_tools();

    TopoDS_Wire ordercutShape(const TopoDS_Shape &aShape);
    double GetEdgeLength(const TopoDS_Edge& anEdge);
    bool OffsetWires_Standard();
    bool OffsetWires_FeatureBased();
    bool OffsetWires_Spiral();
    //Die Abfolge der flachen Bereiche wird hier festgelegt(der Input kommt von der GUI)
    bool SetMachiningOrder(const TopoDS_Face &aFace, float x,float y,float z);
    /*
    Dient zum checken wieviele Faces wir haben und hier wird auch gleich ein vector gefüllt
    wo alle flachen bereiche drin sind
    */

	/*! \brief Hier finden wir eine tolle Funktion */ 
    bool arrangecuts_ZLEVEL();
    //bool checkPointIntersection(std::vector<projectPointContainer> &finalPoints);
    bool calculateAccurateSlaveZLevel(std::vector<std::pair<gp_Pnt,double> >&OffsetPoints, double current_z_level, double &slave_z_level, double &average_sheet_thickness,double &average_angle, bool &cutpos);
    //bool checkPointDistance(std::vector<gp_Pnt> &finalPoints,std::vector<gp_Pnt> &output);
    bool initializeMeshStuff();
    bool arrangecuts_SPIRAL();
    bool arrangecuts_FEATURE_BASED();
    inline const std::vector<Handle_Geom_BSplineCurve>* getOutputhigh()
    {
        return &m_all_offset_cuts_high;
    }
    inline const std::vector<Handle_Geom_BSplineCurve>* getOutputlow()
    {
        return &m_all_offset_cuts_low;
    }
    inline std::vector<std::pair<float,TopoDS_Shape> > getCutShape()
    {
        return m_ordered_cuts;
    }

    std::vector<float> getFlatAreas();
    CuttingToolsSettings m_UserSettings;

private:
    //typedef std::list<std::vector<Base::Vector3f> > Polylines;
    bool CheckEdgeTangency(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2);
    bool CheckPoints(Handle(TColgp_HArray1OfPnt) PointArray);
    bool getShapeBB();
    bool fillFaceWireMap();
    bool CheckforLastPoint(const gp_Pnt& lastPoint,int &start_index,int &start_array,const std::vector<std::vector<std::pair<gp_Pnt,double> > >& MasterPointsStorage);
    bool CheckforLastPoint(const gp_Pnt& lastPoint,int &start_index,int &start_array,const std::vector<std::vector<gp_Pnt> >& SlavePointsStorage);
    bool CheckforLastPoint(const gp_Pnt& lastPoint, int &start_index_master,int &start_array_master,int &start_index_slave,int &start_array_slave,const std::vector<std::vector<std::pair<gp_Pnt,double> > >& MasterPointsStorage, const std::vector<std::vector<gp_Pnt> >& SlavePointsStorage);
    TopoDS_Shape getProperCut(TopoDS_Shape& aShape);
    Handle_Geom_BSplineCurve InterpolateOrderedPoints(Handle(TColgp_HArray1OfPnt) InterpolationPoints,const bool direction);
    //bool projectWireToSurface(const TopoDS_Wire &aWire,const TopoDS_Shape &aShape,std::vector<projectPointContainer> &aContainer);

    bool fillFaceBBoxes();
    Base::BoundBox3f getWireBBox(TopoDS_Wire aWire);
    bool checkPointinFaceBB(const gp_Pnt &aPnt,const Base::BoundBox3f &aBndBox);
    bool classifyShape();
    //bool GenFlatLevelBSpline(
    //bool checkFlatLevel();
    bool cut(float z_level, float min_level, TopoDS_Shape &aCutShape,float &z_level_corrected);
    bool cut_Mesh(float z_level, float min_level, std::list<std::vector<Base::Vector3f> > &result,float &z_level_corrected);
    std::vector<SpiralHelper> OffsetSpiral(const std::vector<SpiralHelper>& SpiralPoints,bool master_or_slave=true);
    gp_Dir getPerpendicularVec(gp_Vec& anInput);
    std::vector<std::pair<float,TopoDS_Shape> > m_ordered_cuts;
    std::vector<std::pair<TopoDS_Face,Base::BoundBox3f> > m_face_bboxes;
    std::vector<std::pair<TopoDS_Face,Base::BoundBox3f> >::iterator m_face_bb_it;

    std::vector<Handle_Geom_BSplineCurve> m_all_offset_cuts_high,m_all_offset_cuts_low;
    //std::multimap<float,TopoDS_Wire> m_zl_wire_combination;
    std::map<TopoDS_Face,std::map<Base::BoundBox3f,TopoDS_Wire,BoundBox3f_Less>,Face_Less > m_FaceWireMap;
    std::vector<std::pair<float,TopoDS_Shape> >::iterator m_ordered_cuts_it;

    //Member zum checken ob CAD oder nicht
    bool m_cad;
    TopoDS_Shape m_Shape;
    MeshCore::MeshKernel m_CAD_Mesh;
    MeshCore::MeshAlgorithm * m_aMeshAlgo;
    MeshCore::MeshFacetGrid * m_CAD_Mesh_Grid;
    bool m_mirrortobothsides;


    //Zustellungswert
    float m_pitch;
    //Der höchste und niedrigste Z-Wert vom Shape
    float m_minlevel,m_maxlevel;
    //Der Radius der Werkzeuge
    float m_radius,m_radius_slave;
    //Blechdicke
    float m_sheet_thickness;
    bool m_direction; ////If we cut from top to bottom (true) or from bottom to top (false)
    std::vector<std::pair<Base::Vector3f,TopoDS_Face> > m_MachiningOrder;








};

#endif




