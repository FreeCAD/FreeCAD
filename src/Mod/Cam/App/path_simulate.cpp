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

#include "PreCompiled.h"
#include "path_simulate.h"

#include <GeomAPI_ProjectPointOnCurve.hxx>

#include <TColgp_Array1OfPnt.hxx>
#include <Base/Exception.h>
#include <Base/Builder3D.h>
#include <GCPnts_AbscissaPoint.hxx>
#include <Geom_BSplineCurve.hxx>

#define curvTOL  30.0  // indicates the maximum radius of curvature from which the curve is subdivided
#define TolDist  1.0   // corresponds to the sampling increment of the curve points for the robot output

/* Constructor with two path sequences (master tool & supporting die) as input */
path_simulate::path_simulate(const std::vector<Handle_Geom_BSplineCurve> &BSplineTop,
                             const std::vector<Handle_Geom_BSplineCurve> &BSplineBottom,
                             struct CuttingToolsSettings& set)
        :m_BSplineTop(BSplineTop),m_BSplineBottom(BSplineBottom),m_set(set),
        m_t0(0.0),m_step(1e-3),m_clip(90000)
{
	
	m_pretension = m_set.spring_pretension;
	m_blech		 = m_set.sheet_thickness;
	m_amax		 = m_set.max_Acc;
    m_vmax       = m_set.max_Vel;
    
	m_single = false;
   
	if(m_pretension > 0) beam = true;  // flag for "write_output_***" generation
	else           		 beam = false;

    //Initialize the Iterators
    m_it1 = m_BSplineTop.begin();
    m_it2 = m_BSplineBottom.begin();

    //Initialise some vars
    gp_Pnt p(0,0,0);
    gp_Pnt q(0,0,0);

    /*------------------------------Generate the first movement of the Master------------*/

    /* Fill p with the starting point of the first Master curve*/
    (*m_it1)->D0((*m_it1)->FirstParameter(),p);

    /* Set q to the initial-Z-level in the Simulation: 2mm + Master-Radius over the sheet which is located at Z=0 */
    q.SetZ(2.0 + set.master_radius);
	
	//First we clear the vectors
    m_StartPnts1.clear();
    m_StartPnts2.clear();

    //Now we insert the start points for the Master movement
    m_StartPnts1.push_back(q);
    m_StartPnts1.push_back(p);

	//Inserts the start parameter
    m_StartParam.push_back((*m_it1)->FirstParameter());

    /*Master finished here*/

    /*-----------------------------Generate the first movement for the Slave--------------*/

    /* Fill p with the starting point of the first Slave curve*/
    (*m_it2)->D0((*m_it2)->FirstParameter(),p);

    // Set q to the initial-Z-level in the Simulation:
    // -5mm - Slave-Radius-Spring-Pretensionbelow the sheet upper level which is located at Z=0
    q.SetZ(-5.0 - set.slave_radius - m_pretension);

    /*Now we insert the start points for the Slave movement*/
    m_StartPnts2.push_back(q);
    m_StartPnts2.push_back(p);

	//Inserts the start parameter
	m_StartParam.push_back((*m_it2)->FirstParameter());

    /*Slave finished here*/
}

path_simulate::~path_simulate()
{
}

/*
double path_simulate::GetLength(GeomAdaptor_Curve& curve, const Standard_Real startParameter,const Standard_Real endParameter)
{
    Standard_Real firstParameter = curve.FirstParameter();
    Standard_Real lastParameter  = curve.LastParameter();


    Standard_Real sParam = Min(startParameter,endParameter);
    Standard_Real eParam = Max(startParameter,endParameter);

    //Standard_Real length = 0.0;

    if ( eParam > lastParameter )
    {
        //get the first part of the length
        Standard_Real l1 = GetLength(curve,firstParameter,eParam-lastParameter);
        Standard_Real l2 = GetLength(curve,sParam,lastParameter);
        return l1 + l2;
    }
    if ( sParam < firstParameter )
    {
        Standard_Real l1 = GetLength(curve,lastParameter-fabs(sParam),lastParameter);
        Standard_Real l2 = GetLength(curve,firstParameter,eParam);
        return l1 + l2;
    }

    // Accuracy control via parameter TOL according to eParam
    return GCPnts_AbscissaPoint::Length(curve,sParam,eParam);
}
*/

/*
double path_simulate::FindParamAt(GeomAdaptor_Curve& curve, double dist, double startParam)
{
    double param;

    //compute the parameter of the next point
    GCPnts_AbscissaPoint absc(curve, dist, startParam);
    if ( absc.IsDone() )
    {
        //the parameter is computed
        param = absc.Parameter();
    }

    return param;
}
*/

/* The absolute speed function is defined here and provides the speed for time input <t>.
The function is divided into three sections with the parameter limits <m_t0>, <m_t1>, <m_t2>, <m_T>
which e.g. can be determined using path_simulate::ParameterCalculation()
The start and end speed must be determined beforehand in <mv[0]> and <m_v[2]>
The maximum speed that is to be achieved between <m_t1> and <m_t2> corresponds here to <m_v[1]> */
double path_simulate::GetVelocity(double t)
{
    double vel;
    double c[2];

    c[0] = m_a/2.0;

    if (t>=m_t0 && t<=m_t1)
    {
        if (t==m_t0 || (m_v[1] - m_v[0]) == 0.0)
        {
            vel = m_v[0];
        }
        else
        {
			c[1] = PI*m_a / (m_v[1] - m_v[0]);
            vel = c[0]*(sin(c[1]*(t-m_t0) - PI)/c[1] + (t-m_t0)) + m_v[0];
        }
    }
    else if (t>m_t1  && t<=m_t2)
    {
        vel = m_v[1];
    }
    else if (t>m_t2 && t<=m_T)
    {
        if (t==m_T)
        {
            vel = m_v[1] - c[0]*(t-m_t2);
        }
        else
        {
			c[1] = PI*m_a / (m_v[1] - m_v[2]);
            vel = m_v[1] - c[0]*(sin(c[1]*(t-m_t2) - PI)/c[1] + (t-m_t2));
        }
    }
    else
        throw Base::RuntimeError("time input not inside [t0, T]");

    return vel;
}

/* This function supplies the distance covered to enter the time <t> and
   corresponds to the integral of the GetVelocity(t) function */
double path_simulate::GetDistance(double t)
{
    double d;
    double c[2];

    c[0] = m_a/2.0;

    if (t>=m_t0 && t<m_t1)
    {
        c[1] = PI*m_a / (m_v[1] - m_v[0]);

		if(t==m_t0){d = 0.0;}
		else{       d = -(c[0]/pow(c[1],2.0))*cos(c[1]*(t-m_t0) - PI) + 
			              c[0]*pow((t-m_t0),2.0)/2 - c[0]/(c[1]*c[1]) + 
						  m_v[0]*(t-m_t0);}
    }
    else if (t>=m_t1  && t<=m_t2)
    {
        d = c[0]*pow((m_t1-m_t0),2.0)/2 + 
			m_v[0]*(m_t1-m_t0) + 
			m_v[1]*(t-m_t1);
    }
    else if (t>m_t2 && t<=m_T)
    {
        c[1] = PI*m_a / (m_v[1] - m_v[2]);

        if (t==m_T)
        {
            d = m_v[0]*(m_t1-m_t0) + c[0]*pow((m_t1-m_t0),2.0)/2 +
				m_v[1]*(m_t2-m_t1) - c[0]*pow((t-m_t2),2.0)/2 + 
				m_v[1]*(t-m_t2);
        }
		else
		{
			d =  m_v[0]*(m_t1-m_t0) + c[0]*pow((m_t1-m_t0),2.0)/2 +
				 m_v[1]*(m_t2-m_t1) + (c[0]/pow(c[1],2.0))*cos(c[1]*(t-m_t2) - PI) - 
				                       c[0]*pow((t-m_t2),2.0)/2 + 
									   c[0]/pow(c[1],2.0) + 
									   m_v[1]*(t-m_t2);
		}
    }
    else
    {
        throw Base::RuntimeError("time input not inside [t0,T]");
    }

    return d;
}

/*
double path_simulate::GetWireLength(TopoDS_Wire &aWire)
{
 GProp_GProps lProps;
 BRepGProp::LinearProperties(aWire,lProps);
 double length = lProps.Mass();
 return length;
}
*/

/*Parameter calculation of the speed function for a straight line of length <S1> */
bool path_simulate::ParameterCalculation_Line(double S1)
{
    if (S1 == 0.0)  // there's nothing to be done here
    {
        m_T = m_t0;
        return true;
    }

	m_a = m_amax;

	m_v[0] = 0.0;                  // Start speed is set to zero
	m_v[1] = sqrt(m_a*S1/2.0);     // Speed that is necessary so that the path <S1> at the time <m_T> is reached
	m_v[2] = 0.0;                  // Final speed is set to zero

	while(m_v[1] > m_vmax)         // maximum speed must not be exceeded
	{
		m_a /= 2;                  // Retry at half acceleration
		m_v[1] = sqrt(m_a*S1/2.0); // Speed that is necessary so that the path <S1> at the time <m_T> is reached
	}

    // Now the time limits can be calculated
    m_t1 = 2*m_v[1]/m_a + m_t0;
    m_t2 = m_t1;
    m_T = 2*m_t1 - m_t0;

    return true;
}

/* Parameter calculation of the speed function (defined in path_simulate::GetVelocity())
for a curve section of length <S1>. The call must always be made before the path_simulate::GetVelocity() function */
bool path_simulate::ParameterCalculation_Curve(double S1)
{
	// Calculating the time limits
    m_t1 = m_t0 + 2*(abs(m_v[1]-m_v[0]))/m_a;
    m_t2 = m_t1;
    m_T  = m_t1 + 2*(abs(m_v[1]-m_v[2]))/m_a;

    double tmp, v_tmp;
    tmp = GetDistance(m_T);  // returns the path that, under the given parameter settings,
                             // to accelerate up and down, at least is necessary

    if (tmp <= S1) // i.e. the path is sufficient
    {
        m_t2 = m_t1 + (S1 - tmp)/m_v[1];  // between <m_t1> and <m_t2> the curve is traversed
                                          // at the constant speed <m_v[1]>
    }
    else // Path is not sufficient -> parameter correction
    {
        // Calculate the speed that is at least necessary so that the path <S1> at time <m_T> is reached
        m_v[1] = sqrt((m_a*S1 + m_v[0]*m_v[0] + m_v[2]*m_v[2])/2.0);  
		m_t1 = m_t0 + 2*(abs(m_v[1]-m_v[0]))/m_a;
        m_t2 = m_t1;

        // a correction may be required here
		if(m_v[1] > m_vmax)
		{
			m_v[1] = m_vmax;

            // from here on again as above
			m_t1 = m_t0 + 2*(abs(m_v[1]-m_v[0]))/m_a;
			m_t2 = m_t1;
			m_T  = m_t1 + 2*(abs(m_v[1]-m_v[2]))/m_a;

			tmp = GetDistance(m_T);

			if (tmp <= S1)
			{
				m_t2 = m_t1 + (S1 - tmp)/m_v[1];
			}
			else
			{
				v_tmp = (m_vmax - std::min(m_v[1],m_v[2]))/2.0;

                while(tmp > S1) // here the speed <mv[1]> is reduced in the direction <mv[2]>
                {               // until the path is sufficient
					m_v[1] = std::min(m_v[1],m_v[2]) + v_tmp;

					m_t1 = m_t0 + 2*(abs(m_v[1]-m_v[0]))/m_a;
				    m_t2 = m_t1;
				    m_T  = m_t1 + 2*(abs(m_v[1]-m_v[2]))/m_a;

				    tmp = GetDistance(m_T);
				
					v_tmp /= 2.0;
				}
			}
		}
    }

    m_T = m_t2 + 2*(abs(m_v[1]-m_v[2]))/m_a;  // End time can now be calculated

    return true;
}

// resets the output vectors and the acceleration parameter <m_a>. The start time <m_t0> is updated
bool path_simulate::UpdateParam()
{
    m_Output.clear();
	m_Output2.clear();
    m_Output_time.clear();
    m_Output_time2.clear();

    m_t0 = m_T;     // The end time of the last run will become the new start time
	m_a  = m_amax;
   
    return true;
}

/* Auxiliary function for the delivery. Return value defines whether delivery is first made in the z or xy direction*/
bool path_simulate::CheckConnect()
{
    gp_Pnt tmp;

    // from the 2nd run
    if (m_it1 != m_BSplineTop.begin() || m_it2 != m_BSplineBottom.begin())
    {
        m_StartPnts1.clear(); 
        m_StartPnts2.clear(); 

        // Calculate new connecting points for the delivery - MASTER -
		m_it1--;

		(*m_it1)->D0((*m_it1)->LastParameter(),tmp);  // Saves the end point of the previous Master curve in <tmp>
		m_StartPnts1.push_back(tmp);                  // Push end point of previous Master curve
		
		m_it1++; 
		
		(*m_it1)->D0((*m_it1)->FirstParameter(),tmp); // Saves the starting point of the previous Mlave curve in <tmp>
		m_StartPnts1.push_back(tmp);                  // Push start point of the current Master curve
		               
        if (m_single == false) // If both sides are driven, do the same as for the slave (see above)
        {
            // Calculate new connecting points for the delivery - SLAVE -
            m_it2--; 
			
			(*m_it2)->D0((*m_it2)->LastParameter(),tmp); // Saves the starting point of the previous Slave curve in <tmp>
			m_StartPnts2.push_back(tmp);                 // Push end point of previous Slave curve
            
			m_it2++;            
			  
            (*m_it2)->D0((*m_it2)->FirstParameter(),tmp); // Speichert Startpunkt der aktuellen Slave-Kurve in <tmp>             
			m_StartPnts2.push_back(tmp);                  // Push start point of the current Slave curve
        }
    }
    else
    {
        return true;  // The first delivery is always the same (in the negative z-direction)
    }

    if (m_StartPnts1[0].Z() - m_StartPnts1[1].Z() >= 0.0) return true;  // Infeed in the negative z-direction
    else                                                  return false; // Infeed in the positive z-direction
}

/* Auxiliary function for the delivery. Return value defines whether delivery is first made in the z or xy direction*/
bool path_simulate::CheckConnect(bool tool)
{
    gp_Pnt tmp;

    // from the 2nd run
    if (m_it1 != m_BSplineTop.begin() || m_it2 != m_BSplineBottom.begin())
    {
        if (m_Feat == true)  // For the feature-based case, the deliveries from Master and Slave are treated separately
		{
			 if (!tool)
			 {
				 m_StartPnts1.clear();
				 
				 m_it1--;
				 
				 (*m_it1)->D0((*m_it1)->LastParameter(),tmp);
				 m_StartPnts1.push_back(tmp);  // Start point
				 
				 m_it1++;
				 
				 (*m_it1)->D0((*m_it1)->FirstParameter(),tmp);
				 m_StartPnts1.push_back(tmp);  // Target point

				 if(m_StartPnts1[0].Z() - m_StartPnts1[1].Z() >= 0.0) return true;
				 else                                                 return false;
			}
			else
			{
				 m_StartPnts2.clear();
				 
				 m_it2--;
				 (*m_it2)->D0((*m_it2)->LastParameter(),tmp);
				 m_it2++;
				 m_StartPnts2.push_back(tmp);  // Start point

				 (*m_it2)->D0((*m_it2)->FirstParameter(),tmp);
				 m_StartPnts2.push_back(tmp);  // Target point

				 if(m_StartPnts2[0].Z() - m_StartPnts2[1].Z() >= 0.0) return true;
				 else                                                 return false;
			}
		}

        // From here: Calculation of the infeed vectors for the synchronous infeed of Master and Slave
		m_StartPnts1.clear();
		m_StartPnts2.clear();

		// Calculate new connecting points for the delivery - MASTER
		m_it1--;
		
		(*m_it1)->D0((*m_it1)->LastParameter(),tmp);
        m_StartPnts1.push_back(tmp);  // Start point

		m_it1++;

		(*m_it1)->D0((*m_it1)->FirstParameter(),tmp);
        m_StartPnts1.push_back(tmp);  // Target point

		if (m_single == false)
		{
			// Calculate new connecting points for the delivery - SLAVE
			m_it2--;
			
			(*m_it2)->D0((*m_it2)->LastParameter(),tmp);
			m_StartPnts2.push_back(tmp);  // Start point
			
			m_it2++;
			
			(*m_it2)->D0((*m_it2)->FirstParameter(),tmp);
			m_StartPnts2.push_back(tmp);  // Target point
		}
    }
    else
    {
        return true;  // The first delivery is always the same
    }

    if (m_StartPnts1[0].Z() - m_StartPnts1[1].Z() >= 0.0) return true;
    else                                                  return false;
}

/* Fills the output vectors for the first delivery. The input parameter <brob> defines the type of output*/
bool path_simulate::ConnectPaths_xy(bool brob)
{
    int N;
    double t = m_t0;
      
    std::vector<Base::Vector3d> tmp2;
	std::vector<double> d;
	Base::Vector3d tmp;
	
	gp_Pnt tmpPnt, pnt1, pnt2, p;
	gp_Vec vec_t(m_StartPnts1[0], m_StartPnts1[1]);
	
	if( 1e-3 > vec_t.Magnitude())  // no delivery necessary
		return true;

    if (m_single == false)
    {
        gp_Vec vec_1(m_StartPnts1[0], m_StartPnts1[1]); 
        gp_Vec vec_2(m_StartPnts2[0], m_StartPnts2[1]);

        gp_Vec2d vec_11, // Saves Master infeed in the XY-direction
			     vec_21; // Saves Master infeed in the  Z-direction

        vec_11.SetX(vec_1.X());  
        vec_11.SetY(vec_1.Y());

        if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())  // first run -> xy delivery (Slave)
        {
            vec_21.SetX(vec_2.X());
            vec_21.SetY(vec_2.Y());
        }
        else
        {
            vec_21.SetX(0.0);
            vec_21.SetY(vec_2.Z());  // slave delivery in the z-direction (from 2nd run)
        }

        // Simulation output
        if (brob == false)
        {
            // ***** MASTER ******
            
			ParameterCalculation_Line(vec_11.Magnitude());

            if (vec_11.Magnitude() != 0)
                vec_11.Normalize();

            N = std::max(2, int(ceil((m_T - m_t0)/m_step))); // Number of output values to be generated
            m_del_t = (m_T - m_t0)/N;                   // Time increment

            for (int i=0; i<N; ++i)
            {
                // Generate XY output vector
                tmp.x = vec_11.X()*GetVelocity(t);
                tmp.y = vec_11.Y()*GetVelocity(t);
                tmp.z = 0.0;

                tmp2.push_back(tmp);
				
                m_Output.push_back(tmp2);
				m_Output_time.push_back(t);
                
				t += m_del_t;
				tmp2.clear();
			}
			 
			m_Output_time.push_back(m_T);

			// ***** SLAVE ******
			t = m_t0;
			ParameterCalculation_Line(vec_21.Magnitude());

            if (vec_21.Magnitude() != 0)
                  vec_21.Normalize();

            N = std::max(2, int(ceil((m_T - m_t0)/m_step)));  // Number of output values to be generated
            m_del_t = (m_T - m_t0)/N;                    // Time increment

            for (int i=0; i<N; ++i)
            {
                if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())
                {
                    // Generate XY output vector
                    tmp.x = vec_21.X()*GetVelocity(t);
                    tmp.y = vec_21.Y()*GetVelocity(t);
                    tmp.z = 0.0;
                }
                else
                {
                    // Generate Z output vector
                    tmp.x = 0.0;
                    tmp.y = 0.0;
                    tmp.z = vec_21.Y()*GetVelocity(t);
                }

                tmp2.push_back(tmp);
				m_Output_time2.push_back(t);
                m_Output2.push_back(tmp2);
                

				t += m_del_t;
				tmp2.clear();
			}
               
            m_Output_time2.push_back(m_T);

            // Null vector
            tmp.x = 0.0;
            tmp.y = 0.0;
            tmp.z = 0.0;

            tmp2.push_back(tmp);

            m_Output.push_back(tmp2);
            m_Output2.push_back(tmp2);
        }
        else  // Robot output for the case on both sides
        {
            bool con = false;

            // calculate the number <N> of the points for the discretization of the infeed line
            if (vec_11.Magnitude() > vec_21.Magnitude()) N = std::max(2, int(ceil(vec_21.Magnitude()/TolDist)));
            else                                         N = std::max(2, int(ceil(vec_11.Magnitude()/TolDist)));

            if (vec_11.Magnitude() == 0.0 && vec_21.Magnitude() == 0.0) N=0;
			if (!m_conn) con = true;

            // The first point is always omitted
            if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())
            {
                // Start point Master
                tmp.x = m_StartPnts1[0].X();
                tmp.y = m_StartPnts1[0].Y();
                tmp.z = m_StartPnts1[0].Z();
                
				m_Output_robo1.push_back(tmp);
                RoboFlag_Master.push_back(0);

                // Start point Slave
                tmp.x = m_StartPnts2[0].X();
                tmp.y = m_StartPnts2[0].Y();
                tmp.z = m_StartPnts2[0].Z();

				m_Output_robo2.push_back(tmp);
                RoboFlag_Slave.push_back(0);
            }

            // Generate Output - MASTER
            for (int i=1; i<N; ++i)
            {
                tmp.x = m_StartPnts1[0].X() + (double(i)*vec_11.X())/(double(N)-1.0);
                tmp.y = m_StartPnts1[0].Y() + (double(i)*vec_11.Y())/(double(N)-1.0);
                tmp.z = m_StartPnts1[con].Z();

                m_Output_robo1.push_back(tmp);
                RoboFlag_Master.push_back(0);
            }

            // // Generate Output - SLAVE
            for (int i=1; i<N; ++i)
            {
                if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())
                {
                    tmp.x = m_StartPnts2[0].X() + (double(i)*vec_21.X())/(double(N)-1.0);
                    tmp.y = m_StartPnts2[0].Y() + (double(i)*vec_21.Y())/(double(N)-1.0);
                    tmp.z = m_StartPnts2[0].Z();
                }
                else
                {
                    tmp.x = m_StartPnts2[con].X();
                    tmp.y = m_StartPnts2[con].Y();
                    tmp.z = m_StartPnts2[0].Z() + (double(i)*vec_21.Y())/(double(N)-1.0);
                }
                
				m_Output_robo2.push_back(tmp);
                RoboFlag_Slave.push_back(0);
            }
        }
    }
    else
    {
		gp_Vec2d vec_11,vec_12;
        gp_Vec vec_1(m_StartPnts1[0], m_StartPnts1[1]);

        vec_11.SetX(vec_1.X());
        vec_11.SetY(vec_1.Y());

        if(brob == false)
        {
            ParameterCalculation_Line(vec_11.Magnitude());

            if(vec_11.Magnitude() != 0.0) 
				vec_11.Normalize();

			N = std::max(2, (int)((m_T-m_t0)/m_step));
            m_del_t = (m_T-m_t0)/double(N);

            for(int i=0; i<N; ++i)
            {
                // Generate XY output vector
                tmp.x = vec_11.X()*GetVelocity(t);
                tmp.y = vec_11.Y()*GetVelocity(t);
                tmp.z = 0.0;

                tmp2.push_back(tmp);

				m_Output_time.push_back(t);
                m_Output.push_back(tmp2);
                
                t += m_del_t;
				tmp2.clear();
            }

            m_Output_time.push_back(m_T);

            // Null vector 
            tmp.x = 0.0;
            tmp.y = 0.0;
            tmp.z = 0.0;

            tmp2.push_back(tmp);
            m_Output.push_back(tmp2);
            tmp2.clear();
        }
        else
        {
            N = (int) vec_11.Magnitude();  // Number of output values to be generated

            for (int i=0; i<N; ++i)
            {
                tmp.x = m_StartPnts1[0].X() + (i*vec_11.X())/N;
                tmp.y = m_StartPnts1[0].Y() + (i*vec_11.Y())/N;
                tmp.z = m_StartPnts1[0].Z();

                m_Output_robo1.push_back(tmp);
            }
        }
    }
    
	return true;
}

/* Fills out the output vectors for the second delivery. The input parameter <brob> defines the output type*/
bool path_simulate::ConnectPaths_z(bool brob)
{
    int N;
    double t = m_t0;

    Base::Vector3d tmp;
    std::vector<double> d;
    std::vector<Base::Vector3d> tmp2;

	gp_Vec vec_t(m_StartPnts1[0], m_StartPnts1[1]);
	if( 1e-3 > vec_t.Magnitude())
		return true;

    if (m_single == false)
    {
        gp_Vec vec_1(m_StartPnts1[0], m_StartPnts1[1]);
        gp_Vec vec_2(m_StartPnts2[0], m_StartPnts2[1]);

        gp_Vec2d vec_11,vec_12;

        vec_11.SetX(0.0);
        vec_11.SetY(vec_1.Z());

        if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())
        {
            vec_12.SetX(0.0);
            vec_12.SetY(vec_2.Z());
        }
        else
        {
            vec_12.SetX(vec_2.X());
            vec_12.SetY(vec_2.Y());
        }

		if (brob == false)
        {
			// ***** MASTER ******            
			ParameterCalculation_Line(vec_11.Magnitude());

            if (vec_11.Magnitude() != 0)
                vec_11.Normalize();

            N = std::max(2, int(ceil((m_T - m_t0)/m_step)));  // Number of output values to be generated
            m_del_t = (m_T - m_t0)/N;                    // Time increment

            for (int i=0; i<N; ++i)
            {
                // Generate Z output vector
                tmp.x = 0.0;
                tmp.y = 0.0;
                tmp.z = vec_11.Y()*GetVelocity(t); 

                tmp2.push_back(tmp);
				
                m_Output.push_back(tmp2);
				m_Output_time.push_back(t);
                
				t += m_del_t;
				tmp2.clear();
			}
			 
			m_Output_time.push_back(m_T);

            // ***** SLAVE ******
			t = m_t0;
			ParameterCalculation_Line(vec_12.Magnitude());

            if (vec_12.Magnitude() != 0)
                  vec_12.Normalize();

            N = std::max(2, int(ceil((m_T - m_t0)/m_step)));  // Number of output values to be generated
            m_del_t = (m_T - m_t0)/N;                    // Time increment

            for (int i=0; i<N; ++i)
            {
                if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())
                {
                    // Generate Z output vector in the very first step
                    tmp.x = 0.0;
                    tmp.y = 0.0;
                    tmp.z = vec_12.Y()*GetVelocity(t);
                }
                else // Slave infeed in XY direction
                {
                    // Generate XY output vector
                    tmp.x = vec_12.X()*GetVelocity(t);
                    tmp.y = vec_12.Y()*GetVelocity(t);
                    tmp.z = 0.0;
                }

                tmp2.push_back(tmp);
				m_Output_time2.push_back(t);
                m_Output2.push_back(tmp2);
                

				t += m_del_t;
				tmp2.clear();
			}
               
            m_Output_time2.push_back(m_T);

            tmp.x = 0.0;
            tmp.y = 0.0;
            tmp.z = 0.0;

            tmp2.push_back(tmp);

            m_Output.push_back(tmp2);
            m_Output2.push_back(tmp2);
        }
        else  // Robot output
        {
            if (vec_11.Magnitude() > vec_12.Magnitude()) std::max(2, N = int(ceil(vec_12.Magnitude()/TolDist)));  // Number of output values to be generated
            else                                         std::max(2, N = int(ceil(vec_11.Magnitude()/TolDist)));  // Number of output values to be generated
    
            if (vec_11.Magnitude() == 0.0 && vec_12.Magnitude() == 0.0) N=1;

            for (int i=1; i<N; ++i)
            {
                /*MASTER*/

                // Generate output vector for delivery in the Z-direction
                tmp.x = m_StartPnts1[m_conn].X();
                tmp.y = m_StartPnts1[m_conn].Y();
                tmp.z = m_StartPnts1[0].Z() + (double(i)*vec_11.Y())/double(N-1);

                m_Output_robo1.push_back(tmp);
                RoboFlag_Master.push_back(0);


                /*SLAVE*/

                if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())
                {   
                    // Generate output vector for delivery in the Z-direction (only in the very first step)
                    tmp.x = m_StartPnts2[1].X();
                    tmp.y = m_StartPnts2[1].Y();
                    tmp.z = m_StartPnts2[0].Z() + (double(i)*vec_12.Y())/double(N-1);
                }
                else
                {
                    // Generate output vector for delivery in the XY-direction
                    tmp.x = m_StartPnts2[0].X() + (double(i)*vec_12.X())/double(N-1);
                    tmp.y = m_StartPnts2[0].Y() + (double(i)*vec_12.Y())/double(N-1);
                    tmp.z = m_StartPnts2[m_conn].Z();
                }

                m_Output_robo2.push_back(tmp);
                RoboFlag_Slave.push_back(0);
            }
        }
    }
    else
    {
        gp_Vec vec_1(m_StartPnts1[0], m_StartPnts1[1]);
        gp_Vec2d vec_12;

        vec_12.SetX(0.0);
        vec_12.SetY(vec_1.Z());

        if (brob == false)
        {
            ParameterCalculation_Line(vec_12.Magnitude());
			
			if (vec_12.Magnitude() != 0)
                  vec_12.Normalize();

            N = std::max(2, int(ceil((m_T - m_t0)/m_step)));  // Number of output values to be generated
            m_del_t = (m_T - m_t0)/N;                    // Time increment

            for (int i=0; i<N; ++i)
            {
                m_Output_time.push_back(t);

                // Generate Z output vector
                tmp.x = 0.0;
                tmp.y = 0.0;
                tmp.z = vec_12.Y()*GetVelocity(t);

                tmp2.push_back(tmp);
                m_Output.push_back(tmp2);
                
                t += m_del_t;
				tmp2.clear();
            }

            t = m_T;

            m_Output_time.push_back(t);

			// Null vector
            tmp.x = 0.0;
            tmp.y = 0.0;
            tmp.z = 0.0;

            tmp2.push_back(tmp);
            m_Output.push_back(tmp2);
            tmp2.clear();
        }
        else // Robot output
        {
            N =(int) vec_12.Magnitude();  // Number of output values to be generated

            for (int i=0; i<N; ++i)
            {
                // Generate output vector for delivery in the XY-direction
                tmp.x = m_StartPnts1[0].X() + (i*vec_12.X())/N;
                tmp.y = m_StartPnts1[0].Y() + (i*vec_12.Y())/N;
                tmp.z = m_StartPnts1[0].Z();

                m_Output_robo1.push_back(tmp);
            }
        }
    }

    return true;
}

/* Fills in the output vectors for the delivery for the feature-based case. 
The input parameter <brob> defines the type of output*/
bool path_simulate::ConnectPaths_Feat(bool tool,  // Tool           (Master, Slave)
                                      bool brob,  // Output type    (Robot, Simulation)
                                      bool c_typ) // Delivery type  (in two or three steps)
{
    int N, ind;
    double rad, t;
    bool dir;

    std::vector<double> Times;
    std::vector<gp_Pnt> ConnPnts;
    std::vector< std::vector<Base::Vector3d> > Out;
    gp_Vec vec[3], vec_tmp[3];

    Base::Vector3d tmp;
    std::vector<double> d;
    std::vector<Base::Vector3d> tmp2;
    double vel;

    dir = CheckConnect(tool);  // set new starting points

    if (!tool)
    {
        ConnPnts = m_StartPnts1;
        rad = m_set.master_radius;
    }
    else
    {
        ConnPnts = m_StartPnts2;
        rad = m_set.slave_radius;
    }

    if (c_typ)  // Delivery in 2 steps
    {
        ind = 2;
        vec_tmp[0].SetCoord(0.0, 0.0, abs(ConnPnts[1].Z()-ConnPnts[0].Z()));
        vec_tmp[1].SetCoord(ConnPnts[1].X()-ConnPnts[0].X(), ConnPnts[1].Y()-ConnPnts[0].Y(), 0.0);

        if (dir) // tool  (Master/Slave) must go down
        {
            if (!tool)
            {
                vec[0] =  vec_tmp[1];    // tool = Master
                vec[1] = -vec_tmp[0];
            }
            else
            {
                vec[0] = -vec_tmp[0];    // tool = Slave
                vec[1] =  vec_tmp[1];
            }
        }
        else   // tool  (Master/Slave) must go up
        {
            if (!tool)
            {
                vec[0] = vec_tmp[0];    // tool = Master
                vec[1] = vec_tmp[1];
            }
            else
            {
                vec[0] = vec_tmp[1];    // tool = Slave
                vec[1] = vec_tmp[0];
            }
        }
    }
    else // Delivery in 3 steps
    {
        ind = 3;

        vec_tmp[0].SetCoord(0.0, 0.0, rad);
        vec_tmp[1].SetCoord(ConnPnts[1].X()-ConnPnts[0].X(), ConnPnts[1].Y()-ConnPnts[0].Y(), 0.0);
        vec_tmp[2].SetCoord(0.0, 0.0, abs(ConnPnts[1].Z()-ConnPnts[0].Z()) + rad);

        if (dir) // tool  (Master/Slave) must go down
        {
            if (!tool)
            {
                vec[0] =  vec_tmp[0];    // tool = Master
                vec[1] =  vec_tmp[1];
                vec[2] = -vec_tmp[2];
            }
            else
            {
                vec[0] = -vec_tmp[2];    // tool = Slave
                vec[1] =  vec_tmp[1];
                vec[2] =  vec_tmp[0];
            }
        }
        else   // tool  (Master/Slave) must go up
        {
            if (!tool)
            {
                vec[0] =  vec_tmp[2];    // tool = Master
                vec[1] =  vec_tmp[1];
                vec[2] = -vec_tmp[0];
            }
            else
            {
                vec[0] = -vec_tmp[0];    // tool = Slave
                vec[1] =  vec_tmp[1];
                vec[2] =  vec_tmp[2];
            }
        }
    }

    if (brob) // Robot output
	{   
		for (int i=0; i<ind; ++i)
		{
			N = std::max(2, int(ceil(vec[i].Magnitude()/TolDist)));  // Number of output values to be generated

			for(int j=1; j<N; ++j)
			{
				tmp.x = ConnPnts[0].X() + (double(j)*vec[i].X())/(double(N)-1.0);
				tmp.y = ConnPnts[0].Y() + (double(j)*vec[i].Y())/(double(N)-1.0);
				tmp.z = ConnPnts[0].Z() + (double(j)*vec[i].Z())/(double(N)-1.0);;

				if(!tool) m_Output_robo1.push_back(tmp);  // Master
				else      m_Output_robo2.push_back(tmp);  // Slave
			}

			ConnPnts[0].SetCoord(tmp.x , tmp.y, tmp.z); // Set start point = end point for the next iteration
		}

		return true;
	}
	
	if(ConnPnts[0].Distance(ConnPnts[1]) < 1e-3) return true;  // no delivery on spiral tracks!!!

    for (int i=0; i<ind; ++i)
    {
        t = m_t0;
        ParameterCalculation_Line(vec[i].Magnitude());
        if (vec[i].Magnitude() != 0.0) vec[i].Normalize();
        else continue;

        N = std::max(2,(int) ceil((m_T - m_t0)/m_step)); // Number of output values to be generated
        m_del_t = (m_T - m_t0)/N;                   // Time increment

        for (int j=0; j<N; ++j)
        {
            Times.push_back(t);
            vel = GetVelocity(t);

            tmp.x = vec[i].X()*vel;
            tmp.y = vec[i].Y()*vel;
            tmp.z = vec[i].Z()*vel;

            tmp2.push_back(tmp);
            Out.push_back(tmp2);  // Fill Temporary Output Vector (will be assigned below)

            t += m_del_t;
			tmp2.clear();
        }

        m_t0 = m_T; // The end time of the last run will become the new start time
    }

    Times.push_back(m_T);

    tmp.x = 0.0;
    tmp.y = 0.0;
    tmp.z = 0.0;

    tmp2.clear();
    tmp2.push_back(tmp);
    Out.push_back(tmp2);

    if(!tool)
    {
        m_Output      = Out;
        m_Output_time = Times;
    }
    else
    {
        m_Output2      = Out;
        m_Output_time2 = Times;
    }

    return true;
}

/* The curve is subdivided here, taking into account the <curvTOL> curvature tolerance.
The range limits are returned in the return vector. The output vector is empty when the maximum curvature
does not exceed the tolerance value and therefore no subdivision is necessary */ 
std::vector<std::vector<double> > path_simulate::CompBounds(bool tool,std::vector<double> knots)
{
    m_curMax = 0.0; // set maximum curvature initially to zero
	double cr_bound = 1/curvTOL;
	double cr_last;
	gp_Vec dtmp1, dtmp2;
	gp_Pnt dtmp0;
    GeomAdaptor_Curve curve;
	std::vector<double> single_bound;
    std::vector<double> bounds;
    std::vector<std::vector<double> > CriticalBounds;

    // load current curve
    if (!tool)  curve.Load(*m_it1);
    else        curve.Load(*m_it2);

    double fParam = curve.FirstParameter(), // First curve parameter
           lParam = curve.LastParameter(),  // Last curve parameter
           period = lParam - fParam;        // Length of the parameter range


	int n  = knots.size();  // Length of the knot vector
	bool b = false;

    // The maximum curvature <m_curMax> is calculated here
    // The parameters of the range limits at which the curvature of the curve corresponds to the tolerance
    // value <cr_bound> corresponds to are filled into the vector <bounds> for further post-processing
    for (int i=0; i<n; ++i)
    {
        curve.D2(knots[i], dtmp0, dtmp1, dtmp2); // Since it is a cubic B-spline curve,
                                                 // the maximum curvature can only be assumed at the knot points
		if(dtmp2.Magnitude() >= cr_bound && !b)
		{
			if(knots[i] >= m_boundTol && knots[i] < lParam - m_boundTol)
			{
				if(cr_last < cr_bound)
				{
					bounds.push_back(knots[i-1] + (knots[i] - knots[i-1])*
					                              (cr_last - cr_bound)/
						     					  (cr_last - dtmp2.Magnitude()));
				}
				else
				{
					bounds.push_back(fParam + m_boundTol);
				}
				b = true;
			}
		}
		
		if(dtmp2.Magnitude() < cr_bound && b)
		{
			if(knots[i] <= lParam - m_boundTol)
			{
				bounds.push_back(knots[i-1] + (knots[i] - knots[i-1])*
					                          (cr_bound - cr_last)/
											  (dtmp2.Magnitude() - cr_last));
			}
			else
			{
				bounds.push_back(lParam - m_boundTol);
				
			}
			b = false;
		}
		
		cr_last = dtmp2.Magnitude(); // The curvature here corresponds to the amount of the second derivative

		if(m_curMax < cr_last)  // Stores maximum curvature
			m_curMax = cr_last;
	}

	if(period < 2*m_boundTol || bounds.size() == 0)
		return CriticalBounds;

	if(b) bounds.push_back(lParam - m_boundTol);  // The last limit may have to be added here

	n = (int) bounds.size()/2;  // <bounds> is always of even length
	for(int i=0; i<n; i++)      // Combine areas with a smaller distance than <m_boundTol>
	{
		single_bound.push_back(bounds[2*i]);
		
		while(i<n-1 && bounds[2*i+2] - bounds[2*i+1] < m_boundTol)
			i++;
		
		single_bound.push_back(bounds[2*i+1]);
        CriticalBounds.push_back(single_bound);
        single_bound.clear();
    }

    return CriticalBounds;
}

/* This is where the preparatory work for the Gen_Path () function is done.
The vectors <m_length>, <m_velocity>, <m_accel> are filled for the current curve. */
bool path_simulate::CompPath(bool tool) // tool = 0  -> Master
                                        // tool = 1  -> Slave
{
	m_boundTol = pow(m_vmax, 2.0)/m_amax; // Tolerance range in front of critical areas (necessary for high acceleration)
	
	double cur     = 1.0/curvTOL,         // Curvature tolerance for curve division
	       pos     = 0.0, 
	       cur_tmp = 0.0;

	int    nb_knots;

	std::vector<std::vector<double> > v_vec;
	std::vector<double> v(3), l_vec, a_vec;
	
	std::vector<Base::Vector3d> Pnt1Vec;
    
	GeomAdaptor_Curve curve;

    gp_Pnt pnt0;
    gp_Vec pnt, pnt1, pnt2, vec;
    Base::Vector3d Pnt1;
    
    double start,
		   fParam,
           lParam,
		   period;

	double d2, velo, tetha, 
		   len, len_1;

    double t0 = m_t0; // Returns the current start time

    int num = Detect_FeatCurve(tool);  // Returns the number of curves to be driven (usually num = 1)

    for(int a=0; a<num; a++)  // Loop over the connected curves
	{
        // load current curves
        if (!tool)  curve.Load(*m_it1);
		else        curve.Load(*m_it2);

        // reset parameters
		fParam = curve.FirstParameter();
		lParam = curve.LastParameter();
		period = lParam - fParam;

        // transfers the start parameters of the current curve
		m_StartParam[tool] = fParam;
		start = m_StartParam[tool];

        // *** Calculating the Knot vectors ***
		if (!tool)
		{
			nb_knots = (*m_it1)->NbKnots();
			m_Knots  = new TColStd_Array1OfReal(0,nb_knots-1);
		   (*m_it1)->Knots(*m_Knots);
		}
		else
		{
			nb_knots = (*m_it2)->NbKnots();
			m_Knots  = new TColStd_Array1OfReal(0,nb_knots-1);
		   (*m_it2)->Knots(*m_Knots);
		}
		
		std::vector<double> knot_vec(m_Knots->Length());
		for(int i=0; i<m_Knots->Length(); i++)
			knot_vec[i] = m_Knots->Value(i);

        // *** End of the calculation of the Knot vectors ***


        std::vector<std::vector<double> > CriticalBounds = 
                                            CompBounds(tool, knot_vec); // Calculated on the basis of the current curve,
                                                                        // the critical areas in the parameter space and
                                                                        // simultaneously calculates the maximum curvature <m_curMax>

        m_vmid = std::min(m_vmax,sqrt(m_amax/m_curMax)); // Defines the speed with which all critical areas are traveled

Newtry: // If the generated path lengths are insufficient, a new attempt is started with half the <m_vmid>

        v[0] = 0.0;  // start every curve with v = 0
		int m = 0;

        for (unsigned int i=0; i<CriticalBounds.size(); ++i)  // Loop over individual subdivision areas
		{
			d2 = 0.0;
			pos = m_Knots->Value(m);

			while(pos < m_StartParam[tool])
			{
				m++;
				pos = m_Knots->Value(m);
			}
			
			/*------------------- Straight section ---------------------*/
			
			while(pos < CriticalBounds[i][0]) // Calculate the maximum curvature of this straight section
			{
				curve.D2(pos, pnt0, pnt1, pnt2);
				cur_tmp = pnt2.Magnitude();   // Curvature at the current point
				
				if(d2 < cur_tmp) 
					d2 = cur_tmp;             // Stores maximum curvature dieses geraden Bereichs

				m++;
				pos = m_Knots->Value(m);
			}

            tetha = 0.6 + 0.25*sqrt(d2/m_curMax);         // Set tetha parameter -> (0.6 < tetha < 0.85)
            velo = std::min(m_vmax, tetha*(sqrt(m_amax/d2)));  // Sets maximum speed <velo>
            m_a = m_amax - d2*velo*velo;                  // if <velo> was chosen too large, <m_a> can possibly become negative

			// Correction
			while(m_a <= 0.0)
			{
				velo = velo/2.0;
				m_a = m_amax - d2*velo*velo;
			}

			if(velo < m_vmid)
				m_vmid = velo;

			v[1] = velo;

            /*-------------- Correction of the speeds (if the path is too long) -------------*/

            len = CriticalBounds[i][0] - m_StartParam[tool];              // Length of the i. straight section
            len_1 = (pow(v[1] - v[0],2.0) + pow(v[1] - m_vmid,2.0))/m_a;  // necessary length

            if(len < pow(v[0] - m_vmid,2.0)/m_a)  // no correction is possible in this case
			{
				l_vec.clear();
				v_vec.clear();
				a_vec.clear();

                m_StartParam[tool] = start; // reset start parameters
                m_vmid = m_vmid/2;          // halve critical throughput speed
				goto Newtry;
			}

			while(len < len_1)
			{
				v[1] = v[0] + (v[1] - v[0])/2.0;
				len_1 = (pow(v[1] - v[0],2.0) + pow(v[1] - m_vmid,2.0))/m_a; 
			}

			v[2] = m_vmid;

            /*---Correction end---*/

            // fill Vectors
            l_vec.push_back(len);  // Length
            v_vec.push_back(v);    // Speeds
            a_vec.push_back(m_a);  // Acceleration

            m_StartParam[tool] += len;   // Sets new start parameters

            /*------------------------ curved area ------------------------*/

            len = CriticalBounds[i][1] - CriticalBounds[i][0];  // Arc length of the curved area
            m_StartParam[tool] += len;                          // Sets new start parameters
            v[0] = v[2];                                        // End speed becomes the start speed

			l_vec.push_back(len);
		}

		d2 = 0;	

        // Correct the current knot parameter
		while(pos < m_StartParam[tool])
		{
			m++;
			pos = m_Knots->Value(m);
		}

		pos = m_Knots->Value(m);

        // Calculation of the maximum curvature for the last section
		while(pos < lParam)
		{
			curve.D2(pos, pnt0, pnt1, pnt2);

			Pnt1.x = pnt2.X();
			Pnt1.y = pnt2.Y();
			Pnt1.z = pnt2.Z();

			if(d2 < Pnt1.Length()) d2 = Pnt1.Length();

			m++;
			pos = m_Knots->Value(m);
		}

        tetha = 0.6 + 0.25*sqrt(d2/m_curMax);        // 0.6 < tetha < 0.85
        velo = std::min(m_vmax, tetha*(sqrt(m_amax/d2))); // see above

		v[1] = velo;
		v[2] = 0.0;
        m_a = m_amax - d2*velo*velo; // if velo is too large, m_a can become negative

        // Correction
		while(m_a <= 0.0)
		{
			velo = velo/2.0;
			v[1] = velo;
			m_a = m_amax - cur*velo*velo;
		}

		d2 = 0;

        len = lParam - m_StartParam[tool] + start;          // Length of the last straight section
        len_1 = (pow(v[1] - v[0],2.0) + pow(v[1],2.0))/m_a; // required length

        if(len < pow(v[0],2.0)/m_a) // no correction possible here
		{
			l_vec.clear();
			v_vec.clear();
			a_vec.clear();

            m_StartParam[tool] = start; // reset start parameters
            m_vmid = m_vmid/2;          // halve critical speed and try again
			goto Newtry;
		}

		while(len < len_1)
		{
			v[1] = v[0] + (v[1] - v[0])/2;
			len_1 = (pow(v[1] - v[0],2.0) + pow(v[1],2.0))/m_a; 
		}

        // fill Vectors
        l_vec.push_back(len);  // Length
        v_vec.push_back(v);    // Speeds
        a_vec.push_back(m_a);  // Acceleration

        // First fill the output vectors here (once per curve)
		if(tool)
		{	
			m_length_sl.push_back(l_vec);
			m_velocity_sl.push_back(v_vec);
			m_accel_sl.push_back(a_vec);
		}
		else
		{
			m_length_ma.push_back(l_vec);
			m_velocity_ma.push_back(v_vec);
			m_accel_ma.push_back(a_vec);
		}

        // load current curves
	    if (!tool)  m_it1++;
		else        m_it2++;

		m_StartParam[tool] = start;
		
		l_vec.clear();
		v_vec.clear();
		a_vec.clear();
		
		curve.Delete();
		CriticalBounds.clear();
    }

    // Reset iterator and start parameters
	if (!tool)
	{ 
		for(int i=0; i<num; i++){m_it1--;}
		m_StartParam[tool] = (*m_it1)->FirstParameter();
		
	}
	else
	{ 
		for(int i=0; i<num; i++){m_it2--;}
		m_StartParam[tool] = (*m_it2)->FirstParameter();
	}

    return true;
}

/* Only integrates the output values according to the time according to the trapezoid rule and delivers the error vector as a connection between the start and end point.
Finally, the output vectors are corrected by precisely this error vector */
bool path_simulate::Correction(bool tool)
{
    int N;
    gp_Vec vec;
	gp_Pnt pnt;
    Base::Vector3d tmp;
    std::vector<Base::Vector3d> tmp2;

	double Sum[3];

	Sum[0] = 0.0;
    Sum[1] = 0.0;
    Sum[2] = 0.0;

	// Here the numerical integration of the output vector takes place according to the
    // trapezoidal rule and the resulting error vector is stored in <vec>
    if (tool==false) // Master
    {
		N = m_Output.size();
        for (int i=1; i<N; ++i)
        {
            Sum[0] += (m_Output[i][0].x + m_Output[i-1][0].x)*(m_Output_time[i] - m_Output_time[i-1]) / 2.0;
            Sum[1] += (m_Output[i][0].y + m_Output[i-1][0].y)*(m_Output_time[i] - m_Output_time[i-1]) / 2.0;
            Sum[2] += (m_Output[i][0].z + m_Output[i-1][0].z)*(m_Output_time[i] - m_Output_time[i-1]) / 2.0;
        }

        if(m_StartPnts1[0].Distance(m_StartPnts1[1]) > 1e-3)  // if curve is NOT closed
		{
			vec.SetCoord(Sum[0],Sum[1],Sum[2]);
			vec.SetCoord(m_StartPnts1[1].X() - m_StartPnts1[0].X() - vec.X(),
						 m_StartPnts1[1].Y() - m_StartPnts1[0].Y() - vec.Y(),
						 m_StartPnts1[1].Z() - m_StartPnts1[0].Z() - vec.Z());
		}
		else
			vec.SetCoord(-Sum[0],-Sum[1],-Sum[2]);
    }
    else  // Slave
    {
		N = m_Output2.size();
        for (int i=1; i<N; ++i)
        {
            Sum[0] += (m_Output2[i][0].x + m_Output2[i-1][0].x)*(m_Output_time2[i] - m_Output_time2[i-1]) / 2.0;
            Sum[1] += (m_Output2[i][0].y + m_Output2[i-1][0].y)*(m_Output_time2[i] - m_Output_time2[i-1]) / 2.0;
            Sum[2] += (m_Output2[i][0].z + m_Output2[i-1][0].z)*(m_Output_time2[i] - m_Output_time2[i-1]) / 2.0;
        }

		if(m_StartPnts2[0].Distance(m_StartPnts2[1]) > 1e-3)  // if curve is NOT closed
		{
			vec.SetCoord(Sum[0],Sum[1],Sum[2]);
			vec.SetCoord(m_StartPnts2[1].X() - m_StartPnts2[0].X() - vec.X(),
						 m_StartPnts2[1].Y() - m_StartPnts2[0].Y() - vec.Y(),
						 m_StartPnts2[1].Z() - m_StartPnts2[0].Z() - vec.Z());
		}
		else
			vec.SetCoord(-Sum[0],-Sum[1],-Sum[2]);
    }

    ParameterCalculation_Line(vec.Magnitude());  // Calculating the time limits

    N = (int) ceil((m_T - m_t0)/m_step);  // Number of output values to be generated
    m_del_t = (m_T - m_t0)/N;             // Time increment

    if (N==1)  // One output vector only
    {
        m_T = m_t0 + 2e-3;

        if (tool==false) // Master
        {
            m_Output_time.push_back(m_t0);
            m_Output_time.push_back(m_t0 + 1e-3);
        }
        else // Slave
        {
            m_Output_time2.push_back(m_t0);
            m_Output_time2.push_back(m_t0 + 1e-3);
        }

        tmp.x = 0.0;
        tmp.y = 0.0;
        tmp.z = 0.0;

        tmp2.push_back(tmp);

        if(tool==false)  m_Output.push_back(tmp2);  // fills Master-Output
        else             m_Output2.push_back(tmp2); // fills Slave-Output

        tmp2.clear();

        tmp.x = (vec.X()/vec.Magnitude())*(vec.X()/1e-3);
        tmp.y = (vec.Y()/vec.Magnitude())*(vec.Y()/1e-3);
        tmp.z = (vec.Z()/vec.Magnitude())*(vec.Z()/1e-3);

        tmp2.push_back(tmp);

        if (tool==false) m_Output.push_back(tmp2);
        else             m_Output2.push_back(tmp2);

        tmp2.clear();
    }
    else
    {
        double vel,t = m_t0;
        if (vec.Magnitude() != 0) vec.Normalize();

        for (int i=0; i<N; ++i)
        {
            if (tool==false) m_Output_time.push_back(t);
            else             m_Output_time2.push_back(t);

            // MASTER
			vel = GetVelocity(t);

			// Output vector at the point in time <t>
			tmp.x = vec.X()*vel;
            tmp.y = vec.Y()*vel;
            tmp.z = vec.Z()*vel;

            tmp2.push_back(tmp);
          
			if (tool==false) m_Output.push_back(tmp2);
            else             m_Output2.push_back(tmp2);

            t += m_del_t;
            tmp2.clear();
        }
    }

	// Null vector
	tmp.x = 0.0;
    tmp.y = 0.0;
    tmp.z = 0.0;
    
	tmp2.push_back(tmp);
	
	// Pass null vector at end time <m_T>
	if (tool==false) 
	{
		m_Output.push_back(tmp2);
		m_Output_time.push_back(m_T);
	}
    else
	{
		m_Output2.push_back(tmp2);
		m_Output_time2.push_back(m_T);
	}

    return true;
}
// This is where the actual output for the simulation process is generated, taking into account
// the vectors calculated in CompPath() m_velocity, m_accel, m_length
bool path_simulate::Gen_Path()
{
	int n,m;
	double length, lam, t_start, t_ma, t_sl, t_tmp;

	t_start = m_t0;                           // Set start time
	m_StartPnts1[0] = (*m_it1)->StartPoint(); // Set Start parameters

	if(m_single == false)
	{
		m_StartPnts2[0] = (*m_it2)->StartPoint();

/*------------------------------ Calculate lead time for the Master tool --------------------------------*/

		n = m_velocity_ma.size();
		for(int i=0; i<n; i++)                    // Loop over the (connected) curves
		{
			m = m_velocity_ma[i].size();
			for(int j=0; j<m; j++)                // Loop over each subdivision area
			{
				m_v[0] = m_velocity_ma[i][j][0];  // Starting speed of the i-th section
				m_v[1] = m_velocity_ma[i][j][1];  // Maximum speed of the i-th segment
				m_v[2] = m_velocity_ma[i][j][2];  // Final speed of the ith section

				m_a = m_accel_ma[i][j];

				length = m_length_ma[i][2*j];      // Length of the non-critical section
                                                   // Note: Every curve starts and ends in a non-critical section
				ParameterCalculation_Curve(length);   // This is where the end time m_T is calculated

				if(j != m-1)
				{
					length = m_length_ma[i][2*j+1];// Length of the critical section
					m_T = m_T + length/m_v[2];     // Calculates new end time (critical sections are run through at
                                                   // constant speed m_v[2])
				}

				m_t0 = m_T;                        // The end time becomes the start time
			}
		}

		t_ma = m_T - t_start;                  // Corresponds to the lead time for the Master
		m_t0 = t_start;                        // Resets the start time

/*-------------------------- Calculate lead time for the Slave tool -------------------------------*/

		n = m_velocity_sl.size();
		for(int i=0; i<n; i++)                    // Loop over the (connected) curves
		{
			m = m_velocity_sl[i].size();
			for(int j=0; j<m; j++)                // Loop over each subdivision area
			{
				m_v[0] = m_velocity_sl[i][j][0];  // Starting speed of the i-th section
				m_v[1] = m_velocity_sl[i][j][1];  // Maximum speed of the i-th segment
				m_v[2] = m_velocity_sl[i][j][2];  // Final speed of the ith section

				m_a = m_accel_sl[i][j];

				length = m_length_sl[i][2*j];      // Length of the non-critical section
                                                   // Note: Every curve starts and ends in a non-critical section
				ParameterCalculation_Curve(length);// Among other things, the end time <m_T> is calculated here

				if(j != m-1)
				{
					length = m_length_sl[i][2*j+1];// Length of the critical section
					m_T = m_T + length/m_v[2];     // Calculates new end time (critical sections are run through at
                }                                  // the constant speed <m_v[2]>)

				m_t0 = m_T;                        // End time becomes start time
			}
		}

		t_sl = m_T - t_start;                      // Corresponds to the processing time for the Slave
		m_t0 = t_start;                            // Resets the start time

/*----------------------- Synchronization of Master and Slave -----------------------------*/

// Idea: scale the speeds and accelerations of the path with the shorter processing time,
// so that the processing times for master and slave match


		if(t_ma <= t_sl)
		{
            // From here on: Correction for the master

			lam = t_ma/t_sl;  // Scaling factor 0 < lam <= 1

            // First of all, all speeds are scaled down
			n = m_velocity_ma.size();
			for(int i=0; i<n; i++)
			{
				m = m_velocity_ma[i].size();
				for(int j=0; j<m; j++)
				{
                    // linear relationship between the speeds and the processing time
					m_velocity_ma[i][j][0] = lam*m_velocity_ma[i][j][0];
					m_velocity_ma[i][j][1] = lam*m_velocity_ma[i][j][1];
					m_velocity_ma[i][j][2] = lam*m_velocity_ma[i][j][2];
				}
			}

            // Scaling the accelerations
			n = m_accel_ma.size();
			for(int i=0; i<n; i++)
			{
				m = m_accel_ma[i].size();
				for(int j=0; j<m; j++)
					m_accel_ma[i][j] = lam*lam*m_accel_ma[i][j];  // The accelerations must be squared with the scaling factor
            }                                                     // be multiplied (quadratic dependency on processing time)
		}
		else
		{
            // From here on: correction for the slave

			lam = t_sl/t_ma;  // Scaling factor 0 < lam <= 1

            // First of all, all speeds are scaled down
			n = m_velocity_sl.size();
			for(int i=0; i<n; i++)
			{
				m = m_velocity_sl[i].size();
				for(int j=0; j<m; j++)
				{
                    // see Master
					m_velocity_sl[i][j][0] = lam*m_velocity_sl[i][j][0];
					m_velocity_sl[i][j][1] = lam*m_velocity_sl[i][j][1];
					m_velocity_sl[i][j][2] = lam*m_velocity_sl[i][j][2];
				}
			}

			// Scaling the accelerations
			n = m_accel_sl.size();
			for(int i=0; i<n; i++)
			{
				m = m_accel_sl[i].size();
				for(int j=0; j<m; j++)
					m_accel_sl[i][j] = lam*lam*m_accel_sl[i][j];  // The accelerations have to be multiplied by the
			}                                                     // square of the scaling factor
		}
	}

/*------------------------------- Generate the output vectors ------------------------------------*/

/*MASTER*/

	bool l;
	int  q,p;

    n = m_length_ma.size();
    for(int i=0; i<n; i++)  // The loop goes over the number of curves to be driven
                            // see path_simulate::CompPath()
    {
		if(i!=0)
			m_it1++;

        l = false;  // Determines whether we are currently in a critical section
        q = 0;      // At the beginning of every curve to zero
        p = 0;      // At the beginning of every curve to zero

		m_StartParam[0] = (*m_it1)->FirstParameter(); // Sets the start parameters of the current curve
		
		m = m_length_ma[i].size();
		for(int j=0; j<m; j++)
		{
			if(l==false)  // is called in every second step
			{
				m_a = m_accel_ma[i][p];
				
				m_v[0] = m_velocity_ma[i][q][0];
				m_v[1] = m_velocity_ma[i][q][1];
				m_v[2] = m_velocity_ma[i][q][2];
				
				p++;
				q++;
			}

			MakePathSingle(0,m_length_ma[i][j],l,0);  // The output vectors are created here
			
			// Update of variables 
			m_StartParam[0] += m_length_ma[i][j];
			m_t0 = m_T;
			l = !l;
		}
	}

	m_t0 = t_start;

/*END MASTER*/


/*SLAVE*/

    // Similar to the master (see above)
	if(m_single == false)
	{
		n = m_length_sl.size();
		for(int i=0; i<n; i++)
		{
			if(i!=0)
				m_it2++;
			
			l = false;
			q = 0;
			p = 0;
			m_StartParam[1] = (*m_it2)->FirstParameter();
			m = m_length_sl[i].size();

            // see Master
			for(int j=0; j<m; j++)
			{
				if(l==false)
				{
					m_a = m_accel_sl[i][p];
					
					m_v[0] = m_velocity_sl[i][q][0];
					m_v[1] = m_velocity_sl[i][q][1];
					m_v[2] = m_velocity_sl[i][q][2];
					
					p++;
					q++;
				}

				MakePathSingle(0,m_length_sl[i][j],l,1);  // the output for the slave is generated here
				m_StartParam[1] += m_length_sl[i][j];
				m_t0 = m_T;
				l = !l;
			}
		}
	}

/*END SLAVE*/


/*PATH CORRECTION*/

	t_tmp = m_t0;

	m_StartPnts1[1] = (*m_it1)->EndPoint(); // Sets the end point, necessary for the following path correction
	Correction(0);                          // Path correction for the Master

	if(m_single == false)
	{
		m_t0 = t_tmp;
		m_StartPnts2[1] = (*m_it2)->EndPoint(); // Sets the end point, necessary for the following path correction
		Correction(1);                          // Path correction for the Slave
	}

	m_t0 = t_start;

/*END PATH CORRECTION*/


	m_velocity_ma.clear();
	m_velocity_sl.clear();
	m_length_ma.clear();
	m_length_sl.clear();
	m_accel_ma.clear();
	m_accel_sl.clear();
	
	return true;
}

/* The simulation output vectors for the respective curve sections are generated here */
bool path_simulate::MakePathSingle(bool   brob,   // Describes output type (Robot, Simulation)
                                   double length, // Arc length of the considered section
                                   bool   part,   // Specifies whether we are looking at a critical section
                                   bool   tool)   // Tool (Master, Slave)
{
    GeomAdaptor_Curve anAdaptorCurve;
    double firstParam, lastParam, param, period, d, velo;
    int N;

    if(tool == false) anAdaptorCurve.Load(*m_it1);  // Master
    else              anAdaptorCurve.Load(*m_it2);  // Slave

    firstParam = anAdaptorCurve.FirstParameter();
    lastParam  = anAdaptorCurve.LastParameter();
    period     = lastParam - firstParam;

	gp_Vec                      dtmp1, dtmp2;
	gp_Pnt                      tmp;
    Base::Vector3d              tmp2;
    std::vector<Base::Vector3d> tmp3;

	// Base::Vector3f pnt1,pnt2;

	if(brob)
	{
		N = std::max(2, (int) ceil(period / double(TolDist)));

		for (int i=1; i<N; ++i)  // never take the first point with you
		{
			if (m_StartParam[tool] + double(i)*period/(double(N)-1.0) > lastParam)
			{
				anAdaptorCurve.D0(m_StartParam[tool] + double(i)*period/(double(N)-1.0) - period, tmp);
			}
			else if (m_StartParam[tool] + double(i)*period/(double(N)-1.0) < firstParam )
			{
				anAdaptorCurve.D0(m_StartParam[tool] + double(i)*period/(double(N)-1.0) + period, tmp);
			}
			else
			{
				anAdaptorCurve.D0(m_StartParam[tool] + double(i)*period/(double(N)-1.0), tmp);
			}

			tmp2.x = tmp.X();
			tmp2.y = tmp.Y();
			tmp2.z = tmp.Z();

			if (!tool)
			{
				m_Output_robo1.push_back(tmp2);

				if (i==1)
				{
					RoboFlag_Master.pop_back();
					RoboFlag_Master.push_back(1);
				}

				RoboFlag_Master.push_back(0);
			}
			else
			{
				m_Output_robo2.push_back(tmp2);

				if (i==1)
				{
					RoboFlag_Slave.pop_back();
					RoboFlag_Slave.push_back(1);
				}

				RoboFlag_Slave.push_back(0);
			}
		}

		return true;
	}

    if (part==true) m_T = m_t0 + length/m_v[2]; // critical areas are traversed at a constant speed
    else            ParameterCalculation_Curve(length);
    
	N = std::max(2, (int)((m_T-m_t0)/m_step));

    if (N>=100000) 
        N = 99999;  // maximum number of possible output values

	m_del_t = (m_T-m_t0)/double(N);

    std::vector< std::vector<Base::Vector3d> > D0;
    std::vector< std::vector<Base::Vector3d> > D1;

    double t = m_t0;

	//anAdaptorCurve.D0(m_StartParam[tool],tmp);
	//pnt2.Set(tmp.X(),tmp.Y(),tmp.Z());

	if (part == true)  // critical section
    {
        for (int i=0; i<N; ++i)  // Main loop
        {
            if (!tool) m_Output_time.push_back(t);
            else       m_Output_time2.push_back(t);

            d = m_v[2]*(t-m_t0);
            param = m_StartParam[tool] + d; // Curve parameter corresponds to the distance covered
                                            // in the case of arc length parameterization.
                                            // With any parameterization:
                                            // param = FindParamAt(anAdaptorCurve, d, m_StartParam[tool]);

            // Calculates everything up to the second derivative
			if      ( param > lastParam  ){ anAdaptorCurve.D2(param - period, tmp, dtmp1, dtmp2);}
            else if ( param < firstParam ){ anAdaptorCurve.D2(param + period, tmp, dtmp1, dtmp2);}
			else                          { anAdaptorCurve.D2(param,          tmp, dtmp1, dtmp2);}

			//m_times_tmp.push_back(t);
		    //m_velo_tmp.push_back(m_v[2]);

            // Output vector at the point in time <t>
			tmp2.x = dtmp1.X()*m_v[2];
            tmp2.y = dtmp1.Y()*m_v[2];
            tmp2.z = dtmp1.Z()*m_v[2];

            tmp3.push_back(tmp2);
            
			if (!tool) m_Output.push_back(tmp3);
            else       m_Output2.push_back(tmp3);
            
			tmp3.clear();
			
			/*
            if (tool == false)  // traces the actual path
			{
				pnt1 = pnt2;
				
				pnt2.x = pnt2.x + dtmp1.X()*m_v[2]*m_del_t;
				pnt2.y = pnt2.y + dtmp1.Y()*m_v[2]*m_del_t;
				pnt2.z = pnt2.z + dtmp1.Z()*m_v[2]*m_del_t;
				
				if(i==0)
					m_log.addSingleArrow(pnt1,pnt2,2,1,1,1);
				else
					m_log.addSingleArrow(pnt1,pnt2,2,1,0,0);
			}
			*/
            
			t += m_del_t;
        }
        return true;
    }

    // uncritical section
    for (int i=0; i<N; ++i)
    {
        if (!tool) m_Output_time.push_back(t);
        else       m_Output_time2.push_back(t);

		d = GetDistance(t);
        param = m_StartParam[tool] + d;     // Curve parameter corresponds to the distance covered
                                            // in the case of arc length parameterization.
                                            // With any parameterization:
                                            // param = FindParamAt(anAdaptorCurve, d, m_StartParam[tool]);

        // Calculates everything up to the second derivative
        if      ( param > lastParam  ){ anAdaptorCurve.D2(param - period, tmp, dtmp1, dtmp2);}
        else if ( param < firstParam ){ anAdaptorCurve.D2(param + period, tmp, dtmp1, dtmp2);}
		else                          { anAdaptorCurve.D2(param,          tmp, dtmp1, dtmp2);}

        velo = GetVelocity(t);  // Calculates the speed <velo> of the tool at the point in time <t>
                                // with respect to the parameters m_t0, m_t1, m_t2, m_T, m_a, m_v[i], i =1,2,3

        // Output vector at the point in time <t>
        tmp2.x = dtmp1.X()*velo;
        tmp2.y = dtmp1.Y()*velo;
        tmp2.z = dtmp1.Z()*velo;

        tmp3.push_back(tmp2);
       
		if (!tool) m_Output.push_back(tmp3);
        else       m_Output2.push_back(tmp3);

		tmp3.clear();
		
		/
        if (tool == false)  // traces the actual path
		{
			pnt1 = pnt2;
			
			pnt2.x = pnt2.x + dtmp1.X()*velo*m_del_t;
			pnt2.y = pnt2.y + dtmp1.Y()*velo*m_del_t;
			pnt2.z = pnt2.z + dtmp1.Z()*velo*m_del_t;
			
			if(i==0)
				m_log.addSingleArrow(pnt1,pnt2,2,1,1,1);
			else
				m_log.addSingleArrow(pnt1,pnt2,2,1,0,0);
		}
		*/

		//m_times_tmp.push_back(t);
		//m_velo_tmp.push_back(velo);

        t += m_del_t;
    }

	return true;
}

/* Main routine for generating the robot output for the normal two-sided case */
bool path_simulate::MakePathRobot()
{
    ofstream anOutputFile;
    ofstream anOutputFile2;

    anOutputFile.open("output_master.k");
    anOutputFile.precision(7);
	
	anOutputFile  << "none" << std::endl;

    if (m_single == false)
    {
        anOutputFile2.open("output_slave.k");
        anOutputFile2.precision(7);
		
		anOutputFile2 << "none" << std::endl;
    }

	// Output generation across all curves
    for (m_it1 = m_BSplineTop.begin(); m_it1 != m_BSplineTop.end(); ++m_it1)
    {
        m_StartParam[0] = ((*m_it1)->FirstParameter());
        if (m_single == false) m_StartParam[1] = ((*m_it2)->FirstParameter());

        /*------ 1.DELIVERY ------*/

		m_conn = CheckConnect();

		if (m_conn)   ConnectPaths_xy(1);
        else          ConnectPaths_z(1);

        UpdateParam();


        /*------ 2.DELIVERY ------*/

		if (m_conn)   ConnectPaths_z(1);
        else          ConnectPaths_xy(1);

        UpdateParam();


        /*------ CURVE ------*/

        MakePathSingle(1,0,0,0);  // Master
        MakePathSingle(1,0,0,1);  // Slave
        UpdateParam();

        if (m_single==false && (m_it1 != (m_BSplineTop.end()-1)))
            ++m_it2;

    }

	int c = 1;

    /*--- Write robot output ---*/

	if (m_single==false)
	{
		WriteOutputDouble(anOutputFile,anOutputFile2,c,c,1,beam);
		anOutputFile2 << "*END" << endl;
		anOutputFile2.close();
	}
	else
	{
		WriteOutputSingle(anOutputFile,c,1,0,beam);
	}
    
	anOutputFile  << "*END" << endl;
    anOutputFile.close();

    return true;
}

// Is only called at the beginning of the writing in path_simulate::WriteOurputDouble()
// and fits the output vectors <m_Output_time>, <m_Output_time2> in such a way that the end times coincide
bool path_simulate::TimeCorrection()
{
	int N;
	Base::Vector3d vec(0.0,0.0,0.0);
    std::vector<Base::Vector3d> vecc;
	
	vecc.push_back(vec);
    
    if (m_single == false)  // A time correction makes sense only in this case
    {
        if(m_Output_time.size() == 0 || m_Output_time2.size() == 0)  // Special treatment for this case
		{
			if(m_Output_time.size() > m_Output_time2.size())
			{
				m_Output_time2 = m_Output_time;
				N = m_Output_time2.size();
				m_Output2.resize(N);
				
				for (int i=0; i<N; ++i) // Fills empty output with null vectors (tool has to wait)
				{
					m_Output2[i] = vecc; 
				}

				return true;
			}
			else if(m_Output_time2.size() > m_Output_time.size())
			{
				m_Output_time = m_Output_time2;
				N = m_Output_time.size();
				m_Output.resize(N);
				
				for (int i=0; i<N; ++i) // Fills empty output with null vectors (-> tool has to wait)
				{
					m_Output[i] = vecc;
				}

				return true;
			}

			return true;  //returns true if both output vectors <m_Output_time> and <m_Output_time2> should be empty
		}

        if (m_Output_time[m_Output_time.size()-1] < m_Output_time2[m_Output_time2.size()-1])
        {
            m_T = m_Output_time2[m_Output_time2.size()-1];

            N = std::max(1,int(ceil((m_Output_time2[m_Output_time2.size()-1] - m_Output_time[m_Output_time.size()-1])/ m_step)));
            m_del_t = (m_Output_time2[m_Output_time2.size()-1] - m_Output_time[m_Output_time.size()-1])/double(N);

            int ind = m_Output_time.size()-1;
            double time = 0.0;

            for (int i=1; i<N; ++i)
            {
                time += m_del_t;
                m_Output_time.push_back(m_Output_time[ind] + time);
                m_Output.push_back(vecc);
            }

            m_Output_time.push_back(m_Output_time2[m_Output_time2.size()-1]);
            m_Output.push_back(vecc);
        }
        else if (m_Output_time[m_Output_time.size()-1] > m_Output_time2[m_Output_time2.size()-1])
        {
            m_T = m_Output_time[m_Output_time.size()-1];

            N = std::max(1,int(ceil((m_Output_time[m_Output_time.size()-1] - m_Output_time2[m_Output_time2.size()-1])/ m_step)));
            m_del_t = (m_Output_time[m_Output_time.size()-1] - m_Output_time2[m_Output_time2.size()-1])/double(N);

            int ind = m_Output_time2.size()-1;
            double time = 0.0;

            for (int i=1; i<N; ++i)
            {
                time += m_del_t;
                m_Output_time2.push_back(m_Output_time2[ind] + time);
                m_Output2.push_back(vecc);
            }

            m_Output_time2.push_back(m_Output_time[m_Output_time.size()-1]);
            m_Output2.push_back(vecc);
        }
    }
    else // if <m_single> = true (i.e. only master -> no time correction required!!!)
    {
        return false;
    }

    return true;
}

/* Main routine for generating the simulation output for the feature-based and spiral-based two-sided case */
bool path_simulate::MakePathSimulate_Feat(const std::vector<float> &flatAreas, bool spiral)
{
    m_Feat = true;
	
	double rad[2];
	bool   tool;
	int    c[2];
    
    gp_Pnt pnt0, pnt1;

    std::vector<Handle_Geom_BSplineCurve>::iterator *it_1,*it_2;
    std::vector<Handle_Geom_BSplineCurve> *curves_1, *curves_2;

    ofstream anOutputFile[2];

    anOutputFile[0].open("output_master.k");
    anOutputFile[1].open("output_slave.k");
	anOutputFile[0].precision(7);
    anOutputFile[1].precision(7);

    m_it1 = m_BSplineTop.begin();
    m_it2 = m_BSplineBottom.begin();

    rad[0] =  m_set.master_radius;
    rad[1] = - m_set.slave_radius - m_set.sheet_thickness;

    c[0] = 1;    // Start index of the Master curves
    c[1] = 2001; // Start index of the Slave curves

    int i = 0;

    while (m_it1 != m_BSplineTop.end() && m_it2 != m_BSplineBottom.end())
    {
        tool = StartingTool();  // determines which tool must wait:
                                // tool == true  : Robot = Slave  & NC = Master
                                // tool == false : Robot = Master & NC = Slave
		if (!tool)
		{
			it_1 = &m_it1;
			it_2 = &m_it2;
			curves_1 = &m_BSplineTop;
			curves_2 = &m_BSplineBottom;
		}
		else
		{
			it_1 = &m_it2;
			it_2 = &m_it1;
			curves_1 = &m_BSplineBottom;
			curves_2 = &m_BSplineTop;
		}

        m_StartParam[0] = ((*m_it1)->FirstParameter()); // set new start parameters (in our case always 0)

		if (m_single == false) 
			m_StartParam[1] = ((*m_it2)->FirstParameter());

        // the first delivery before contact with the sheet metal is handled separately here
        if (i==0)
        {
            /*------ DELIVERY 1 ------*/
            ConnectPaths_xy(0);
            WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam);
            UpdateParam();

            /*------ DELIVERY 2 ------*/
            ConnectPaths_z(0);
            WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam);
            UpdateParam();
        }

        while (true)
        {
            if (*it_1 != (*curves_1).end()-1)
            {
                /* ------ Curve ------*/
                CompPath(0);                                                         // Calculate parameters for the Master
                CompPath(1);                                                         // Calculate parameters for the Slave
                Gen_Path();                                                          // Generate output for current Curve
                WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Write output
                UpdateParam();

                /*------ Delivery ------*/
                (*it_1)++;                                                           // Go to the next curve
                ConnectPaths_Feat(tool, 0, 1);                                       // Generate output for delivery
                WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Write output
                UpdateParam();

                if (*it_1 == (*curves_1).end()-1 && *it_2 == (*curves_2).end()-1)    // last step
                {
                    /* ------ Curve ------*/
                    CompPath(0);  // Calculate parameters for the Master
                    CompPath(1);  // Calculate parameters for the Slave
                    Gen_Path();	  // Generate output for current Curve
                    WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Write output
                    UpdateParam();
                    break;
                }

                (**it_1)->D0((**it_1)->FirstParameter(),pnt0); // pass current starting point


                if ((pnt0.Z() > (flatAreas[i+1] + rad[tool] - 1e-1)) &&  // When the MASTER reaches the next level
                    (pnt0.Z() < (flatAreas[i+1] + rad[tool] + 1e-1)))    // Area, the SLAVE must be waited for
                {
                    if(!spiral)  // In the case of spiral tracks, delivery takes place immediately
                    {
                        /* ------ Curve ------*/
                        CompPath(0);  // Calculate parameters for the Master
                        CompPath(1);  // Calculate parameters for the Slave
                        Gen_Path();	  // Generate output for current Curve
                        WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Write output
                        UpdateParam();
                    }
                    break;
                }
            }
            else
            {
                if (*it_2 == (*curves_2).end()-1)  // last step
                {
                    /* ------ Curve ------*/
                    CompPath(0);  // Calculate parameters for the Master
                    CompPath(1);  // Calculate parameters for the Slave
                    Gen_Path();	  // Generate output for current Curve
                    WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Write output
                    UpdateParam();
                    break;
                }
                else break;
            }
        }

        if (m_it1 == m_BSplineTop.end()-1 && m_it2 == m_BSplineBottom.end()-1) // Fertig !!!
            break;

        /* ------ Delivery ------*/
		if(!spiral) 
		{
            (*it_1)++;                      // Go to the next curve
            ConnectPaths_Feat(tool, 0, 1);  // Generate output for the Master-delivery
		}

        (*it_2)++;                                                           // Go to the next curve
        ConnectPaths_Feat(!tool, 0, 0);                                      // Generate output for the Slave-devlivery
        WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Write output
        UpdateParam();

        ++i;
    }

    anOutputFile[0] << "*END" << endl;
    anOutputFile[1] << "*END" << endl;
    anOutputFile[0].close();
    anOutputFile[1].close();

	//WriteTimes();

    return true;
}

/* Main routine for generating the robot output for the feature-based and spiral-based two-sided case */
bool path_simulate::MakePathRobot_Feat(const std::vector<float> &flatAreas)
{
    m_Feat = true;
    int f = 0, run;
    bool tool;
    double rad[2];
    gp_Pnt pnt0, pnt1;

    std::vector<Handle_Geom_BSplineCurve>::iterator *it_1,*it_2;
    std::vector<Handle_Geom_BSplineCurve> *curves_1, *curves_2;

    ofstream anOutputFile[2];

	std::stringstream master_file, slave_file;

    m_it1 = m_BSplineTop.begin();
    m_it2 = m_BSplineBottom.begin();

    rad[0] =  m_set.master_radius;
    rad[1] = - m_set.slave_radius - m_set.sheet_thickness;

    int i = 0;

    while (m_it1 != m_BSplineTop.end() && m_it2 != m_BSplineBottom.end())
    {
        tool = StartingTool();  // Determines which tool must wait

        // Set start parameter (in our case unnecessary because always 0)
        m_StartParam[0] = ((*m_it1)->FirstParameter());
        if (m_single == false) m_StartParam[1] = ((*m_it2)->FirstParameter());

        // The first delivery is handled separately
        if (i==0)
        {
            ConnectPaths_xy(1); /* 1. Delivery */
            ConnectPaths_z(1);  /* 2. Delivery */
        }

        if (!tool)
        {
            it_1 = &m_it1;
            it_2 = &m_it2;
            curves_1 = &m_BSplineTop;
            curves_2 = &m_BSplineBottom;
        }
        else
        {
            it_1 = &m_it2;
            it_2 = &m_it1;
            curves_1 = &m_BSplineBottom;
            curves_2 = &m_BSplineTop;
        }

        // Main loop
        while (true)
        {
            MakePathSingle(1,0,0,tool);  // Path generation of the continuously moving tool for the current curve

            // MASTER
            if (*it_1 != (*curves_1).end()-1)
            {
                (*it_1)++;
                (**it_1)->D0((**it_1)->FirstParameter(),pnt0);

                if (*it_1 == (*curves_1).end()-1 && *it_2 == (*curves_2).end()-1)
                {
                    ConnectPaths_Feat(tool, 1, 1);
                    MakePathSingle(1,0,0,tool);
                    break;
                }

                if ((pnt0.Z() > (flatAreas[i+1] + rad[tool] - 1e-1)) &&
                    (pnt0.Z() < (flatAreas[i+1] + rad[tool] + 1e-1)))
                {

                    run = Detect_FeatCurve(tool);

                    for (int i=0; i<run; ++i)
                    {
                        ConnectPaths_Feat(tool, 1, 1);
                        MakePathSingle(1,0,0,tool);
                        ++(*it_1);
                    }

                    break;
                }
            }
            else
            {
                if (*it_2 == (*curves_2).end()-1)
                {
                    ConnectPaths_Feat(tool, 1, 1);
                    MakePathSingle(1,0,0,tool);
                    break;
                }
                else break;
            }

            ConnectPaths_Feat(tool, 1, 1);
        }

        (**it_2)->D0((**it_2)->FirstParameter(),pnt0);

        while ((pnt0.Z() > (flatAreas[i] + rad[!tool] - 1e-1)) && 
			   (pnt0.Z() < (flatAreas[i] + rad[!tool] + 1e-1)))
        {
            MakePathSingle(1,0,0,!tool);

            if (*it_2 != (*curves_2).end()-1)
                (*it_2)++;
            else
                break;

            (**it_2)->D0((**it_2)->FirstParameter(),pnt0);
        }

        master_file << "output_master" << f << ".k";
        slave_file  << "output_slave"  << f << ".k";
        ++f;

        std::cout << master_file.str() << "  ,  " << slave_file << std::endl;

        anOutputFile[0].open((master_file.str()).c_str());
        anOutputFile[1].open((slave_file.str()).c_str());

        if (!tool)
        {
            anOutputFile[0] << "none" << std::endl;
            anOutputFile[1] << "continuous" << std::endl;
        }
        else
        {
            anOutputFile[0] << "continuous" << std::endl;
            anOutputFile[1] << "none" << std::endl;
        }

        WriteOutput_Feat(anOutputFile[0], anOutputFile[1],f,1);

        master_file.str("");
        master_file.clear();
        slave_file.str("");
        slave_file.clear();

        m_Output_robo1.clear();
        m_Output_robo2.clear();

        if (*it_1 == (*curves_1).end()-1 && *it_2 == (*curves_2).end()-1)
            break;

        ConnectPaths_Feat(0, 1, 1);
        ConnectPaths_Feat(1, 1, 0);
        ++i;
    }

    anOutputFile[0] << "*END" << endl;
	anOutputFile[1] << "*END" << endl;
    anOutputFile[0].close();
    anOutputFile[1].close();

    return true;
}

// Help function for the feature-based part:
// Determines which tool waits while the other is running
bool path_simulate::StartingTool()
{
    double z0,z1;
    gp_Pnt pnt0,pnt1;

    if(m_it1 != m_BSplineTop.end()-1)
    {
        // z-distance to the next path - MASTER
        (*m_it1)->D0((*m_it1)->FirstParameter(),pnt0); m_it1++;
        (*m_it1)->D0((*m_it1)->FirstParameter(),pnt1); m_it1--;
       
        z0 = abs(pnt0.Z() - pnt1.Z());
    }

    else
        z0 = 1e+3;

    if(m_it2 != m_BSplineBottom.end()-1)
    {
        // z-distance to the next path - SLAVE
        (*m_it2)->D0((*m_it2)->FirstParameter(),pnt0); m_it2++;
        (*m_it2)->D0((*m_it2)->FirstParameter(),pnt1); m_it2--;
        
        z1 = abs(pnt0.Z() - pnt1.Z());
    }
    else z1 = 1e+3;

    if(z0<z1)  return false;  // Slave has to wait
    else       return true;   // Master has to wait
}

/* Here a check is made to see whether it is a closed curve or how many curves have to be traversed in order
to arrive at the starting point of the curve again. The number of curves to be traversed is returned as an integer */
int path_simulate::Detect_FeatCurve(bool tool)
{
    gp_Pnt pt0,pt1;
    int num = 1;

    if(!tool)
	{
		pt0 = (*m_it1)->StartPoint(); // The starting point of the current master curve
		pt1 = (*m_it1)->EndPoint();   // The end point of the current master curve
		
		while(pt0.Distance(pt1) > 1e-3)  // Keep going as long as the start and end point don't match
		{
			if(m_it1 == m_BSplineTop.end()-1)  // Stop when you reach the last corner
			{
				for(int i=1; i<num; i++)
					m_it1--;

				num = 1;
				pt1 = pt0; // So that the outer while loop is exited
			}
			else
			{
				num++;
				m_it1++;
				pt1 = (*m_it1)->EndPoint();
			}
		}

		// Return to the status quo
		for(int i=1; i<num; i++)
			m_it1--;
	}
	else
	{	
		pt0 = (*m_it2)->StartPoint(); // The starting point of the current slave curve
		pt1 = (*m_it2)->EndPoint();   // The end point of the current slave curve
		
		while(pt0.Distance(pt1) > 1e-3) // Keep going as long as the start and end point don't match
		{
			if(m_it2 == m_BSplineBottom.end()-1) // Stop when you reach the last corner
			{
				for(int i=1; i<num; i++)
					m_it2--;

				num = 1;
				pt1 = pt0;  // So that the while loop is exited
			}
			else
			{
				num++;
				m_it2++;
				pt1 = (*m_it2)->EndPoint();
			}
		}

		// Return to the status quo
		for(int i=1; i<num; i++)
			m_it2--;
	}

	return num;
}

/* Main routine for generating the simulation output for the normal two-sided case */
bool path_simulate::MakePathSimulate()
{
    int c1 = 1,
		c2 = 2001;

    ofstream anOutputFile, anOutputFile2;

    anOutputFile.open("output_master.k");
    anOutputFile.precision(7);

    if (m_single == false)
    {
        anOutputFile2.open("output_slave.k");
        anOutputFile2.precision(7);
    }

    for (m_it1 = m_BSplineTop.begin(); m_it1 < m_BSplineTop.end(); ++m_it1)  // Loop over all curves
    {
        m_StartParam[0] = ((*m_it1)->FirstParameter());     // saves start parameter values of the current master curve

		if (m_single == false)
            m_StartParam[1] = ((*m_it2)->FirstParameter()); // saves start parameter values of the current slave curve


        /*Delivery start*/

        m_conn = CheckConnect(); // Return value = 1 in the case of a negative z-direction infeed
                                 // Return value = 0 in the case of a positive z-direction feed

        //  negative z-direction: 1. XY --> 2. Z
        //  positive z-direction: 1. Z  --> 2. XY

		// *** 1. ***
		if (m_conn)   ConnectPaths_xy(0);
        else          ConnectPaths_z(0);

        if (m_single == false) WriteOutputDouble(anOutputFile,anOutputFile2,c1,c2,0,beam);
        else                   WriteOutputSingle(anOutputFile,c1,0,0,beam);

        UpdateParam();

		// *** 2. ***
        if (m_conn)   ConnectPaths_z(0);
        else          ConnectPaths_xy(0);

		// Write output
        if (m_single == false) WriteOutputDouble(anOutputFile,anOutputFile2,c1,c2,0,beam);
        else                   WriteOutputSingle(anOutputFile,c1,0,0,beam);

        UpdateParam();

        /*Delivery end*


		/*Curve start*/

        CompPath(0); // Calculate the parameters for the Master
        
		if (m_single == false)
			CompPath(1); // Calculate the parameters for the Slave

		Gen_Path();  // Generation of the output vectors

		// Write output vectors
        if (m_single == false) WriteOutputDouble(anOutputFile,anOutputFile2,c1,c2,0,beam);
        else                   WriteOutputSingle(anOutputFile,c1,0,0,beam);

        UpdateParam();
       
		if (m_single==false && (m_it1 != (m_BSplineTop.end()-1)))
            ++m_it2;

        /*Curve Ende*/
    }

	//m_log.saveToFile("c:/Master-Path.iv");

	/*
	ofstream anOutputvelocity;
    anOutputvelocity.open("output_velocity.k");
    anOutputvelocity.precision(7);

	for(int i=0; i<(int)m_times_tmp.size(); ++i) // Write out absolute speed values
		anOutputvelocity << m_times_tmp[i] << ", " << m_velo_tmp[i] << endl;
	
	anOutputvelocity.close();
	*/

    anOutputFile  << "*END" << endl;
    anOutputFile.close();
	

	if (m_single == false)
    {
		anOutputFile2 << "*END" << endl;
        anOutputFile2.close();
    }
    
    // WriteTimes();
    return true;
}

/* Writes the output for the feature-based and spiral-based bilateral case */
bool path_simulate::WriteOutput_Feat(ofstream &anOutputFile, ofstream &anOutputFile2, int &c, bool brob)
{
    if (m_single == false)
    {
        if (brob == false)
        {
            int n = m_Output.size();

            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << "2,1,0," << c <<  ",1.000000, ," << m_Output_time[n-1] << ","  << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c << std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile << m_Output_time[i] - m_Output_time[0]<<"," << m_Output[i][0].x << std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][0].x<< std::endl;

            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << "3,1,0," << c <<  ",1.000000, ," << m_Output_time[n-1] << ","  << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c+1 << std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile << m_Output_time[i] - m_Output_time[0]<<"," << m_Output[i][1].x << std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][1].x<< std::endl;

            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << "2,2,0," << c+1 <<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c+2 << std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile <<  m_Output_time[i] - m_Output_time[0]<< "," << m_Output[i][0].y<< std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][0].y<< std::endl;


            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << "3,2,0," << c+1 <<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c+3 << std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile <<  m_Output_time[i] - m_Output_time[0]<< "," << m_Output[i][1].y<< std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][1].y<< std::endl;


            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << "2,3,0," << c+2 <<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c+4 <<  std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile <<  m_Output_time[i] - m_Output_time[0]<<"," << m_Output[i][0].z<< std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][0].z<< std::endl;


            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << "3,3,0," << c+2 <<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c+5 <<  std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile <<  m_Output_time[i] - m_Output_time[0]<<"," << m_Output[i][1].z<< std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][1].z<< std::endl;

            c = c+6;
        }
        else
        {
            int n1 = m_Output_robo1.size();
            for (int i=0; i<n1; ++i)
                anOutputFile  << m_Output_robo1[i].x + m_set.x_offset_robot << "," <<  m_Output_robo1[i].y + m_set.y_offset_robot << "," << m_Output_robo1[i].z  << endl;

            anOutputFile.close();

            int n2 = m_Output_robo2.size();
            for (int i=0; i<n2; ++i)
                anOutputFile2 << m_Output_robo2[i].x + m_set.x_offset_robot << "," <<  m_Output_robo2[i].y + m_set.y_offset_robot << "," << m_Output_robo2[i].z  << endl;

            anOutputFile2.close();
        }
    }
    else
    {
        if (brob == false)
        {

            int n = m_Output.size();

            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << "2,1,0," << c <<  ",1.000000, ," << m_Output_time[n-1] << ","  << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c << std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile << m_Output_time[i] - m_Output_time[0]<<"," << m_Output[i][0].x << std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][0].x<< std::endl;

            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << "2,2,0," << c+1 <<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c+1 << std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile <<  m_Output_time[i] - m_Output_time[0]<< "," << m_Output[i][0].y<< std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][0].y<< std::endl;

            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << "2,3,0," << c+2 <<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c+4 <<  std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile <<  m_Output_time[i] - m_Output_time[0]<<"," << m_Output[i][0].z<< std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][0].z<< std::endl;

            c = c+3;
        }
        else
        {
            int n = m_Output_robo1.size();

            for (int i=0; i<n; ++i)
                anOutputFile << m_Output_robo1[i].x << "," <<  m_Output_robo1[i].y << "," << m_Output_robo1[i].z << ",";

            anOutputFile << std::endl;
        }
    }

    return true;
}

/*
// Writes out all time values of the output vectors
bool path_simulate::WriteTimes()
{
    ofstream anOutputFile;

    anOutputFile.open("CurveTimes.k");
    anOutputFile.precision(7);

    for (unsigned int i=0; i< m_PathTimes_Master.size(); ++i)
    {
        anOutputFile << m_PathTimes_Master[i].first << " " << m_PathTimes_Master[i].second << std::endl;
    }

    for (unsigned int i=0; i< m_PathTimes_Slave.size(); ++i)
    {
        anOutputFile << m_PathTimes_Slave[i].first << " " << m_PathTimes_Slave[i].second << std::endl;
    }

    anOutputFile.close();

    return true;
}
*/

/* Writes output for normal one-sided case */
bool path_simulate::WriteOutputSingle(ofstream &anOutputFile, int &c, bool brob, bool tool, bool beamfl)
{
    std::vector< std::vector<Base::Vector3d> > Out_val;
    std::vector<double> Out_time;
    std::pair<float,float> times;
    int n;
    int ind;

    int pid;

    if (brob == true)  // Write robot output
	{
		std::vector<Base::Vector3d> Out_rob;

		if (!tool) Out_rob  = m_Output_robo1;
		else       Out_rob  = m_Output_robo2;

		n = Out_rob.size();

		for (int i=0; i<n; ++i)
			anOutputFile << Out_rob[i].x + m_set.x_offset_robot << "," <<  Out_rob[i].y + m_set.y_offset_robot << "," << Out_rob[i].z << "," << std::endl;

		anOutputFile << std::endl;
		return true;
	}

	if (!tool)
    {
        Out_val  = m_Output;
        Out_time = m_Output_time;
        ind = 2;
    }
    else
    {
        Out_val  = m_Output2;
        Out_time = m_Output_time2;
        ind = 3;
    }

    if (beamfl && tool) pid = ind+1;    // pid: 2 - Master
    else                pid = ind;      //      3 - Slave
                                        //      4 - Plate (x-,y-movement)

    n = Out_val.size();

    if (n != Out_time.size())
        throw Base::RuntimeError("Output lengths do not match");

    if (n>1)
    {
        anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
        anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
        anOutputFile << ind << ",1,0," << c <<  ",1.000000, ," << Out_time[n-1] << ","  << Out_time[0] << std::endl;
        

		if (beamfl && tool)
        {
            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
			anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
			anOutputFile << pid << ",1,0," << c <<  ",1.000000, ," << Out_time[n-1] << ","  << Out_time[0] << std::endl;
        }
		
		anOutputFile << "*DEFINE_CURVE" << std::endl << c << std::endl;

        for (int i=0; i<n; ++i)
        {
            anOutputFile << Out_time[i] - Out_time[0]<<"," << Out_val[i][0].x << std::endl;
        }

        anOutputFile << Out_time[n-1] - Out_time[0] + 0.1 << "," << Out_val[n-1][0].x << std::endl;

        times.first  = (float) Out_time[0];
        times.second = (float) Out_time[n-1];

        if (!tool) m_PathTimes_Master.push_back(times);
        else      m_PathTimes_Slave.push_back(times);

        anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
        anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
        anOutputFile << ind << ",2,0," << c+1 <<  ",1.000000, ," << Out_time[n-1] << "," << Out_time[0] << std::endl;
        

		if (beamfl && tool)
        {
            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
			anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
			anOutputFile << pid << ",2,0," << c+1 <<  ",1.000000, ," << Out_time[n-1] << "," << Out_time[0] << std::endl;
        }
		
		anOutputFile << "*DEFINE_CURVE" << std::endl << c+1 << std::endl;

        for (int i=0; i<n; ++i)
        {
            anOutputFile <<  Out_time[i] - Out_time[0]<< "," << Out_val[i][0].y << std::endl;
        }

        anOutputFile << Out_time[n-1] - Out_time[0] + 0.1 << "," << Out_val[n-1][0].y << std::endl;

        times.first  = (float) Out_time[0];
        times.second = (float) Out_time[n-1];

        if (!tool) m_PathTimes_Master.push_back(times);
        else      m_PathTimes_Slave.push_back(times);

        if (beamfl && tool)
        {
			anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << pid << ",3,0," << c+2 <<  ",1.000000, ," << Out_time[n-1] << "," << Out_time[0] << std::endl;
        }
		else
		{
			anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
			anOutputFile << "$#     ind       dof       vad      lcid        sf       vid     death     birth" << std::endl;
			anOutputFile << ind << ",3,0," << c+2 <<  ",1.000000, ," << Out_time[n-1] << "," << Out_time[0] << std::endl;
		}

        anOutputFile << "*DEFINE_CURVE" << std::endl << c+2 <<  std::endl;


        for (int i=0; i<n; ++i)
        {
            anOutputFile <<  Out_time[i] - Out_time[0]<<"," << Out_val[i][0].z<< std::endl;
        }

        anOutputFile << Out_time[n-1] - Out_time[0] + 0.1 << "," << Out_val[n-1][0].z<< std::endl;

        times.first  = (float) Out_time[0];
        times.second = (float) Out_time[n-1];

        if (!tool) m_PathTimes_Master.push_back(times);
        else       m_PathTimes_Slave.push_back(times);

        c = c+3;
    }

    return true;
}

/* Writes output for the normal two-sided case */
bool path_simulate::WriteOutputDouble(ofstream &anOutputFile, ofstream &anOutputFile2, int &c1, int &c2, bool brob, bool beamfl)
{
    std::pair<float,float> times;
    int pid1 = 2; // Master
	int pid2 = 3; // Slave
	int pid3 = 4; // Plate

    if (brob == false) // Simulations-Output (brob == true -> robot output)
    {
		TimeCorrection();

        int n = m_Output.size();   // Master
        int n2 = m_Output2.size(); // Slave

        if (n>1)
        {

            // MASTER-X

            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << pid1 << ",1,0," << c1 <<  ",1.000000, ," << m_Output_time[n-1] << ","  << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c1 << std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile << m_Output_time[i] - m_Output_time[0]<<"," << m_Output[i][0].x << std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][0].x << std::endl;

            times.first  = (float) m_Output_time[0];
            times.second = (float) m_Output_time[n-1];
            m_PathTimes_Master.push_back(times);       // fill in vector for curve times




			// SLAVE-X

            anOutputFile2 << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile2 << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile2 << pid2 << ",1,0," << c2 <<  ",1.000000, ," << m_Output_time2[n2-1] << ","  << m_Output_time2[0] << std::endl;

			if (beamfl) // if true, then insert a new part
            {
                anOutputFile2 << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
                anOutputFile2 << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
                anOutputFile2 << pid3 << ",1,0," << c2<<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;
            }

			anOutputFile2 << "*DEFINE_CURVE" << std::endl << c2 << std::endl;

            for (int i=0; i<n2; ++i)
            {
                anOutputFile2 << m_Output_time2[i] - m_Output_time2[0]<<"," << m_Output2[i][0].x << std::endl;
            }

            anOutputFile2 << m_Output_time2[n2-1] - m_Output_time2[0] + 0.1 << "," << m_Output2[n2-1][0].x << std::endl;

            times.first  = (float) m_Output_time2[0];
            times.second = (float) m_Output_time2[n2-1];
            m_PathTimes_Slave.push_back(times);





			// MASTER-Y

            anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile << pid1 << ",2,0," << c1+1 <<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c1+1 << std::endl;

            for (int i=0; i<n; ++i)
            {
                anOutputFile <<  m_Output_time[i] - m_Output_time[0]<< "," << m_Output[i][0].y << std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][0].y << std::endl;

            times.first  = (float) m_Output_time[0];
            times.second = (float) m_Output_time[n-1];
            m_PathTimes_Master.push_back(times);




			// SLAVE-Y

            anOutputFile2 << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile2 << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile2 << pid2 << ",2,0," << c2+1 <<  ",1.000000, ," << m_Output_time2[n2-1] << ","  << m_Output_time2[0] << std::endl;
            
			if (beamfl) // if true, then insert a new part
            {
                anOutputFile2 << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
                anOutputFile2 << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
                anOutputFile2 << pid3 << ",2,0," << c2+1 <<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;
            }
			
			anOutputFile2 << "*DEFINE_CURVE" << std::endl << c2+1 << std::endl;

            for (int i=0; i<n2; ++i)
            {
                anOutputFile2 << m_Output_time2[i] - m_Output_time2[0]<<"," << m_Output2[i][0].y << std::endl;
            }

            anOutputFile2 << m_Output_time2[n2-1] - m_Output_time2[0] + 0.1 << "," << m_Output2[n2-1][0].y << std::endl;

            times.first  = (float) m_Output_time2[0];
            times.second = (float) m_Output_time2[n2-1];
            m_PathTimes_Slave.push_back(times);





			// MASTER-Z
			anOutputFile << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
			anOutputFile << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
			anOutputFile << pid1 << ",3,0," << c1+2 <<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;
            anOutputFile << "*DEFINE_CURVE" << std::endl << c1+2 <<  std::endl;


            for (int i=0; i<n; ++i)
            {
                anOutputFile <<  m_Output_time[i] - m_Output_time[0]<<"," << m_Output[i][0].z<< std::endl;
            }

            anOutputFile << m_Output_time[n-1] - m_Output_time[0] + 0.1 << "," << m_Output[n-1][0].z<< std::endl;

            times.first  = (float) m_Output_time[0];
            times.second = (float) m_Output_time[n-1];
            m_PathTimes_Master.push_back(times);



			// SLAVE-Z

			if (beamfl)
            {
                anOutputFile2 << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
                anOutputFile2 << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
                anOutputFile2 << pid3 << ",3,0," << c2+2 <<  ",1.000000, ," << m_Output_time[n-1] << "," << m_Output_time[0] << std::endl;

            }
			else
			{
				anOutputFile2 << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
				anOutputFile2 << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
				anOutputFile2 << pid2 << ",3,0," << c2+2 <<  ",1.000000, ," << m_Output_time2[n2-1] << ","  << m_Output_time2[0] << std::endl;
			}

            anOutputFile2 << "*DEFINE_CURVE" << std::endl << c2+2 <<  std::endl;

            for (int i=0; i<n2; ++i)
            {
                anOutputFile2 << m_Output_time2[i] - m_Output_time2[0]<<"," << m_Output2[i][0].z << std::endl;
            }

            anOutputFile2 << m_Output_time2[n2-1] - m_Output_time2[0] + 0.1 << "," << m_Output2[n2-1][0].z << std::endl;

            times.first  = (float) m_Output_time2[0];
            times.second = (float) m_Output_time2[n2-1];
            m_PathTimes_Slave.push_back(times);

            c1 += 3;
            c2 += 3;
        }
    }
    else  // Write robot output
    {
            int n1 = m_Output_robo1.size();
            for (int i=0; i<n1; ++i)
                anOutputFile  << m_Output_robo1[i].x + m_set.x_offset_robot << "," <<  m_Output_robo1[i].y + m_set.y_offset_robot << "," << m_Output_robo1[i].z << "," << RoboFlag_Master[i] << endl;

            int n2 = m_Output_robo2.size();
            for (int i=0; i<n2; ++i)
                anOutputFile2 << m_Output_robo2[i].x + m_set.x_offset_robot << "," <<  m_Output_robo2[i].y + m_set.y_offset_robot << "," << m_Output_robo2[i].z << "," << RoboFlag_Slave[i] << endl;
    }

    return true;
}
