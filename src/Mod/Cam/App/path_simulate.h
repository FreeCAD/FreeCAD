/***************************************************************************
 *   Copyright (c) 2007 Human Rezaijafari <H.Rezai@web.de>                 *
 *                                                                         *
 *   This file is part of the FreeCAD CAM development system.              *
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

#ifndef Path_Simulate_h
#define Path_Simulate_h


#include <Base/Vector3D.h>
#include <Base/Builder3D.h>
#include <TColStd_Array1OfReal.hxx>
#include "cutting_tools.h"
#include <sstream>

//#include <Base/Builder3D.h>

/*! \brief The main class for the path_simulate routine

    As it's input parameters it takes one respectively two vectors of B-Spline
    Curves (describing the Tool-Paths for the IBU-simulation), two values
    a_max and v_max of type double, which stands for the maximum allowable
    acceleration and velocity of the tool-movement and one value m_step of
    type double, specifying the step-length of the time-output-vector.

    As output, it gives one respectively two output-files for the simulation
    process containing a two-dimensional vector of time vs. velocity which
    describes the tool-movement.
*/

class CamExport path_simulate
{
public:

    /** @brief Constructor.

        @param BSplineTop    vector of B-Spline-Curves describing the tool-
                             paths of the master-tool
        @param BSplineBottom vector of B-Spline-Curves describing the tool-
                             paths of the slave-tool
        @param set           a struct which also includes the parameters
                             a_max and v_max
    */
    path_simulate(const std::vector<Handle_Geom_BSplineCurve>& BSplineTop,
                  const std::vector<Handle_Geom_BSplineCurve>& BSplineBottom,
                  struct CuttingToolsSettings& set);
    ~path_simulate();

    /** @brief Computes all parameters of the velocity-function for the tool
               movement along a straight line.

        @param wirelength length of the straight line
    */
    bool ParameterCalculation_Line(double wirelength);

    /** @brief Computes all parameters of the velocity-function for the tool
               movement along a curve-segment.

        @param wirelength arc-length of the curve-segment
    */
    bool ParameterCalculation_Curve(double wirelength);

    /** @brief Computes output for the connection-part in xy-direction

        @param outputstyle false: simulation, true: robot
    */
    bool ConnectPaths_xy(bool outputstyle);

    /** @brief Computes output for the connection-part in z-direction

        @param outputstyle false: simulation, true: robot
    */
    bool ConnectPaths_z(bool outputstyle);

    /** @brief Computes output for the connection-part in all directions for
               the feature-based-strategy

        @param tool            false: master true: slave
        @param robo            defines outputstyle (false: simulation
                               true: robot)
        @param connection_type false: in 3-steps true: in 2-steps
    */
    bool ConnectPaths_Feat(bool tool, bool robo, bool connection_type);

    /** @brief Updates actual parameter sets */
    bool UpdateParam();

    /** @brief Writes output-vectors to file for the feature-based-strategy

        @param  anOutputFile_1 output-file for the master
        @param  anOutputFile_2 output-file for the slave
        @param  c              startindex of the curve-id
        @param  outputstyle    false: simulation, true: robot
    */
    bool WriteOutput_Feat(ofstream &anOutputFile_1, ofstream &anOutputFile_2, int &c, bool outputstyle);

    ///** @brief Write start- and endtime for one tool-path-run (master & slave) */
    // bool WriteTimes();

    /** @brief Writes output-vectors to file

        @param  anOutputFile output-file
        @param  c
        @param  outputstyle  false: simulation,  true: robot 
        @param  tool         false: master, true: slave
        @param beamfl        specifies an additional outputvalue which
                             determines the waiting-status of the tool
                             movement
        \todo undocumented parameter c
    */
    bool WriteOutputSingle(ofstream &anOutputFile, int &c, bool outputstyle, bool tool, bool beamfl);

    /** @brief Writes output-vectors to file

        @param anOutputFile_1 output-file for the master
        @param nOutputFile_2  output-file for the slave
        @param c1             startindex of the curve-id for the master
        @param c2             startindex of the curve-id for the slave
        @param outputstyle    false: simulation, true: robot 
        @param beamfl         specifies an additional outputvalue using
                              spring-pretension
    */
    bool WriteOutputDouble(ofstream &anOutputFile_1,ofstream &nOutputFile_2, int &c1, int &c2, bool outputstyle,bool beamfl);
    
    /** @brief Main routine for creating the simulation output for a single
               curve, including the synchronisation of master and slave <br>
               CompPath() must have been called before using this function
    */
    bool Gen_Path();
    
    /** @brief Creates outputvectors for the simulation outputfor a single
               curve

        @param outputstyle false: simulation, true: robot
        @param length      defines length of the path
	@param part        tells us that we are generating output for a
                           critical part of the curve or not
        @param curveType   
        \todo undocumented parameter curveType
    */ 
    bool MakePathSingle(bool outputstyle, double length, bool part, bool curveType);

    /** @brief Main function of the output-generation for the simulation
               using the standard-strategy
    */
    bool MakePathSimulate();

    /** @brief Main function of the output-generation for the simulation
               using the feature-based or spiral-based strategy

        @param flatAreas index-vector specifying the flat areas 
        @param spiral    specifies the strategy (true: spiral-based,
                         false: feature-based)
    */
    bool MakePathSimulate_Feat(const std::vector<float> &flatAreas, bool spiral);

    /** @brief Main function of the output-generation for the robot using the
               standard-strategy*/
    bool MakePathRobot();

    /** @brief Main function of the output-generation for the robot using the
               feature-based-strategy
    */
    bool MakePathRobot_Feat(const std::vector<float> &flatAreas);

    /** @brief computes path-correction after a tool-path-run
 
        @param b false: master, true: slave
    */
    bool Correction(bool b);

    /** @brief Adds additional time-values to the output-vector of the
               waiting tool for synchronisation
    */
    bool TimeCorrection();

    /** @brief Returns the next curve-index for the feature-based-strategy */ 
    int  Detect_FeatCurve(bool tool);

private:
    /** @brief  curve-vector for the master-tool*/
    std::vector<Handle_Geom_BSplineCurve> m_BSplineTop;
    /** @brief curve-vector for the slave-tool*/
    std::vector<Handle_Geom_BSplineCurve> m_BSplineBottom;
    /** @brief actual start-point for the connection-part*/
    std::vector<gp_Pnt> m_StartPnts1;
    /** @brief actual end-point for the connection-part*/
    std::vector<gp_Pnt> m_StartPnts2;

    // Base::Builder3D m_log;
    
    /** @brief output-vector for the master including velocity-values for the
               simulation-process */
    std::vector< std::vector<Base::Vector3d> > m_Output;
    /** @brief output-vector for the slave including velocity-values for the
               simulation-process*/
    std::vector< std::vector<Base::Vector3d> > m_Output2;
    /** @brief output-vector for the master including a point-set for the
               robot*/
    std::vector<Base::Vector3d> m_Output_robo1;
    /** @brief output-vector for the slave including a point-set for the
               robot*/
    std::vector<Base::Vector3d> m_Output_robo2;
    /** @brief additional output-vector of the master-tool for the robot
               output including values 0 and 1 */
    std::vector<int> RoboFlag_Master;
    /** @brief additional output-vector of the slave-tool for the robot
               output including values 0 and 1 */
    std::vector<int> RoboFlag_Slave;
    /** @brief output-vector for the master including a time-set conform to
               m_Output respectively m_Output_robo1*/
    std::vector<double> m_Output_time;
    /** @brief output-vector for the slave including a time-set conform to
               m_Output2 respectively m_Output_robo2*/
    std::vector<double> m_Output_time2;
    /** @brief timestep for the simulation-output*/
    double m_step;
    /** @brief pointing to the knot-vector of the current B-spline Curve*/
    TColStd_Array1OfReal *m_Knots;
    /** @brief maximum curvature of current curve*/
    double m_curMax;
    /** @brief external setting-parameters*/
    CuttingToolsSettings m_set;

    /** @brief iterator for the master-curves*/
    std::vector<Handle_Geom_BSplineCurve>::iterator m_it1;   /* iterator über inner-paths */
    /** @brief iterator for the slave-curves*/
    std::vector<Handle_Geom_BSplineCurve>::iterator m_it2;   /* iterator über outer-paths */
    /** @brief sheet-thickness */
    double m_blech;
    /** @brief spring-pretension*/
    double m_pretension;
    /** @brief maximum number of output-values per file*/
    int  m_clip;
    /** @brief flag specifies if spring-pretension is used or not*/
    bool beam;
    /** @brief flag specifying the used forming-strategy*/
    bool m_single;
    /** @brief flag specifying moving-direction (clockwise vs. anticlockwise)
    */
    bool m_dir;
    /** @brief vector in which the lengths of the separated curve-segments for
               the master-tool are stored*/
    std::vector<std::vector<double> > m_length_ma;
    /** @brief vector in which the lengths of the separated curve-segments for
               the slave-tool are stored*/
    std::vector<std::vector<double> > m_length_sl;
    /** @brief vector of acceleration-values regarding to the separated curve
               segments for the master-tool*/
    std::vector<std::vector<double> > m_accel_ma;
    /** @brief vector of acceleration-values regarding to the separated curve
               segments for the slave-tool*/
    std::vector<std::vector<double> > m_accel_sl;
	/** @brief Matrix of three velocity-values regarding to the curves and curve-segments for the master-tool*/
    std::vector<std::vector<std::vector<double> > > m_velocity_ma;
    /** @brief Matrix of three velocity-values regarding to the curves and
               curve-segments for the slave-tool*/
    std::vector<std::vector<std::vector<double> > > m_velocity_sl;
    /** @brief maximum allowable resulting velocity of the tool*/
    double m_vmax;
    /** @brief maximum allowable resulting acceleration of the tool*/
    double m_amax;
    /** @brief pathtolerance which is set (in subject to m_vmax and m_amax)
               before and after a critical region*/
    double m_boundTol;
    /** @brief acceleration-parameter used in GetVelocity() and GetDistance()
    */
    double m_a;
    /** @brief three-dimensional vector containing start-, maximum- and end-
               velocity used in GetVelocity() and GetDistance()*/
    double m_v[3];
    /** @brief parameter of the velocity-function */
    double m_vmid;
    /** @brief initial start-parameter for the current master- and slave-
               curves*/
    std::vector<double> m_StartParam;
    /** @brief timeparameter of the velocity-function*/
    double m_t0;
    /** @brief timeparameter of the velocity-function*/
    double m_t1;
    /** @brief timeparameter of the velocity-function*/
    double m_t2;
    /** @brief timeparameter of the velocity-function*/
    double m_T;
    /** @brief timeparameter of the velocity-function*/
    double m_del_t;  /* t_0 - starttime, T - endtime, del_t - timestep */
    /** @brief returns absolute velocity at the time-value t */
    double GetVelocity(double time);
    /** @brief returns arc-length of the current tool-path at the time-value
               t*/
    double GetDistance(double t);
    /** @brief determines connection-strategy for both tools*/
    bool CheckConnect();
    /** @brief determines connection-strategy for the specified tool

        @param tool false: master, true: slave
    */
    bool CheckConnect(bool tool);
    /** @brief determines critical bounds of the current curve for the
               specified tool

        @param tool false: master, true: slave
        @param knots 
        \todo undocumented parameter knots
    */
    std::vector<std::vector<double> > CompBounds(bool tool, std::vector<double> knots);
    /** @brief Generates output for the current tool-path*/
    bool CompPath(bool tool);
    /** @brief determines which tool should wait (feature-based-stategy only)*/
    bool StartingTool();
    /** @brief vector containing start- and end-times for the master-curves*/
    std::vector<std::pair<float,float> > m_PathTimes_Master;
    /** @brief vector containing start- and end-times for the slave-curves*/
    std::vector<std::pair<float,float> > m_PathTimes_Slave;
    
	// std::vector<double> m_times_tmp;
	// std::vector<double> m_velo_tmp;
    
    /** @brief flag specifying current connection-type*/
    bool m_conn;
    /** @brief flag specifying if a feature-based-strategy is used*/
    bool m_Feat;
};


#endif //Path_Simulate_h




