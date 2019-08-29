/***************************************************************************
 *   Copyright (c) 2007                                                    *
 *   Human Rezaijafari <H.Rezai@web.de>                                    *
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

#define curvTOL  30.0  // gibt maximalen Krümmungsradius an ab welchem eine Unterteilung der Kurve erfolgt
#define TolDist  1.0   // entspricht der Samplingschrittweite der Kurvenpunkte für den Roboter-Output 

/* Konstruktor mit zwei Bahnfolgen (master tool & supporting die) als Input */
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
   
	if(m_pretension > 0) beam = true;  // flag für "write_output_***" generierung
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

    /* Set q to the initial-Z-level in the Simulation: -5mm - Slave-Radius-Spring-Pretensionbelow the sheet upper level which is located at Z=0 */
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


    return GCPnts_AbscissaPoint::Length(curve,sParam,eParam);   // genauigkeitssteuerung über parameter TOL nach eParam 
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

/* Hier wird die absolute Geschwindigkeitsfunktion definiert und liefert die Geschwindigkeit zur Zeiteingabe <t>. 
Die Funktion gliedert sich in drei Abschnitte mit den Parametergrenzen <m_t0>, <m_t1>, <m_t2>, <m_T> welche z.B.
mittels path_simulate::ParameterCalculation() ermittelt werden kann. 
Die Start- und Endgeschwindigkeit müssen vorher in <m_v[0]> und <m_v[2]> bestimmt werden.
Die maximale Geschwindigkeit welche zwischen <m_t1> und <m_t2> erreicht werden soll, entspricht hier <m_v[1]> */
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

/* Diese Funktion liefert den zurückgelegten Weg zur Zeiteingabe <t> und entspricht 
dem Integral der Funktion GetVelocity(t)*/
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

/*Parameterberechnung der Geschwindigkeitsfunktion für eine gerade Strecke der Länge <S1>*/
bool path_simulate::ParameterCalculation_Line(double S1)
{
    if (S1 == 0.0)  // hier gibts nichts zu tun
    {
        m_T = m_t0;
        return true;
    }

	m_a = m_amax;

	m_v[0] = 0.0;                  // Startgeschwindigkeit wird auf Null gesetzt
	m_v[1] = sqrt(m_a*S1/2.0);     // Geschwindigkeit die notwendig ist damit der Weg <S1> zur Zeit <m_T> erreicht wird
	m_v[2] = 0.0;                  // Endgeschwindigkeit wird auf Null gesetzt

	while(m_v[1] > m_vmax)         // maximale Geschwindigkeit darf nicht überschritten werden
	{
		m_a /= 2;                  // Versuchs erneut mit halber Beschleunigung
		m_v[1] = sqrt(m_a*S1/2.0); // Geschwindigkeit die notwendig ist damit der Weg <S1> zur Zeit <m_T> erreicht wird
	}

	// Jetzt lassen sich die Zeitgrenzen berechnen
    m_t1 = 2*m_v[1]/m_a + m_t0;
    m_t2 = m_t1;
    m_T = 2*m_t1 - m_t0;

    return true;
}

/*Parameterberechnung der Geschwindigkeitsfunktion (definiert in path_simulate::GetVelocity())
für einen Kurvenabschnitt der Länge <S1>. Aufruf muss stets vor der Funktion path_simulate::GetVelocity() erfolgen*/
bool path_simulate::ParameterCalculation_Curve(double S1)
{
	// Berechnung der Zeitgrenzen
    m_t1 = m_t0 + 2*(abs(m_v[1]-m_v[0]))/m_a;
    m_t2 = m_t1;
    m_T  = m_t1 + 2*(abs(m_v[1]-m_v[2]))/m_a;

    double tmp, v_tmp;
    tmp = GetDistance(m_T);  // liefert den Weg zurück, der unter den gegebenen Parametereinstellungen,
	                         // zum hoch- und runterbeschleunigen, midestens notwendig ist 

    if (tmp <= S1) // d.h. der Weg reicht aus
    {
        m_t2 = m_t1 + (S1 - tmp)/m_v[1];  // zwischen <m_t1> und <m_t2> wird die Kurve mit der konstanten 
		                                  // Geschwindigkeit <m_v[1]> durchlaufen
    }
    else // Weg reicht nicht aus -> Parameterkorrektur                            
    {
		// Berechne Geschwindigkeit die mindestenns notwendig ist damit der Weg <S1> zur Zeit <m_T> erreicht wird 
        m_v[1] = sqrt((m_a*S1 + m_v[0]*m_v[0] + m_v[2]*m_v[2])/2.0);  
		m_t1 = m_t0 + 2*(abs(m_v[1]-m_v[0]))/m_a;
        m_t2 = m_t1;
		
		// hier wird evtl. eine Korrektur notwendig
		if(m_v[1] > m_vmax)
		{
			m_v[1] = m_vmax;
			
			// ab hier wieder analog zu oben
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
				
				while(tmp > S1) // hier wird die Geschwindigkeit <m_v[1]> solange in Richtung <m_v[2]> vekleinert bis
					            // der Weg schließlich ausreicht
				{
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

    m_T = m_t2 + 2*(abs(m_v[1]-m_v[2]))/m_a;  // Endzeit lässt sich jetzt berechnen

    return true;
}

/* setzt die Outputvektoren und den Beschleunigungsparameter <m_a> zurück. Die Startzeit <m_t0> wird aktualisiert*/
bool path_simulate::UpdateParam()
{
    m_Output.clear();
	m_Output2.clear();
    m_Output_time.clear();
    m_Output_time2.clear();

    m_t0 = m_T;     // Endzeit des letzten Durchlaufs wird zur neuen Startzeit
	m_a  = m_amax;
   
    return true;
}

/* Hilfsfunktion für die Zustellung. Rückgabewert legt fest ob zuerst in z- oder in xy-Richtung zugestellt wird*/
bool path_simulate::CheckConnect()
{
    gp_Pnt tmp;

    // ab dem 2. lauf
    if (m_it1 != m_BSplineTop.begin() || m_it2 != m_BSplineBottom.begin())
    {
        m_StartPnts1.clear(); 
        m_StartPnts2.clear(); 

        // Berechne neue Verbindungspunkte für die Zustellung - MASTER -
		m_it1--;

		(*m_it1)->D0((*m_it1)->LastParameter(),tmp);  // Speichert Endpunkt der vorigen Master-Kurve in <tmp>      		
		m_StartPnts1.push_back(tmp);                  // Pushe Endpunkt der vorigen Master-Kurve
		
		m_it1++; 
		
		(*m_it1)->D0((*m_it1)->FirstParameter(),tmp); // Speichert Startpunkt der vorigen Master-Kurve in <tmp>        
		m_StartPnts1.push_back(tmp);                  // Pushe Startpunkt der aktuellen Master-Kurve
		               
        if (m_single == false) // Falls beidseitig gefahren wird, mache dasselbe, wie für den Slave (s.o.)
        {
            // Berechne neue Verbindungspunkte für die Zustellung - SLAVE -
            m_it2--; 
			
			(*m_it2)->D0((*m_it2)->LastParameter(),tmp); // Speichert Startpunkt der vorigen Slave-Kurve in <tmp> 
			m_StartPnts2.push_back(tmp);				 // Pushe Endpunkt der vorigen Slave-Kurve
            
			m_it2++;            
			  
            (*m_it2)->D0((*m_it2)->FirstParameter(),tmp); // Speichert Startpunkt der aktuellen Slave-Kurve in <tmp>             
			m_StartPnts2.push_back(tmp);                  // Pushe Startpunkt der aktuellen Slave-Kurve
        }
    }
    else
    {
        return true;  // Erste zustellung immer gleich (in negativer z-Richtung)
    }

    if (m_StartPnts1[0].Z() - m_StartPnts1[1].Z() >= 0.0) return true;  // Zustellung in negativer z-Richtung
    else                                                  return false; // Zustellung in positiver z-Richtung
}

/* Hilfsfunktion für die Zustellung. Rückgabewert legt fest ob zuerst in z- oder in xy-Richtung zugestellt wird*/
bool path_simulate::CheckConnect(bool tool)
{
    gp_Pnt tmp;

    // ab dem 2. lauf
    if (m_it1 != m_BSplineTop.begin() || m_it2 != m_BSplineBottom.begin())
    {
        if (m_Feat == true)  // Für den Feature-Basierten Fall werden die Zustellungen von Master und Slave seperat behandelt	
		{
			 if (!tool)
			 {
				 m_StartPnts1.clear();
				 
				 m_it1--;
				 
				 (*m_it1)->D0((*m_it1)->LastParameter(),tmp);
				 m_StartPnts1.push_back(tmp);  // Startpunkt
				 
				 m_it1++;
				 
				 (*m_it1)->D0((*m_it1)->FirstParameter(),tmp);
				 m_StartPnts1.push_back(tmp);  // Zielpunkt

				 if(m_StartPnts1[0].Z() - m_StartPnts1[1].Z() >= 0.0) return true;
				 else                                                 return false;
			}
			else
			{
				 m_StartPnts2.clear();
				 
				 m_it2--;
				 (*m_it2)->D0((*m_it2)->LastParameter(),tmp);
				 m_it2++;
				 m_StartPnts2.push_back(tmp);  // Startpunkt

				 (*m_it2)->D0((*m_it2)->FirstParameter(),tmp);
				 m_StartPnts2.push_back(tmp);  // Zielpunkt

				 if(m_StartPnts2[0].Z() - m_StartPnts2[1].Z() >= 0.0) return true;
				 else                                                 return false;
			}
		}

		// Ab hier: Berechnung der Zustellungsvektoren für die synchrone Zustellung von Master und Slave
		m_StartPnts1.clear();
		m_StartPnts2.clear();

		// Berechne neue Verbindungspunkte für die Zustellung - MASTER
		m_it1--;
		
		(*m_it1)->D0((*m_it1)->LastParameter(),tmp);
		m_StartPnts1.push_back(tmp);  // Startpunkt
		
		m_it1++;
		
		(*m_it1)->D0((*m_it1)->FirstParameter(),tmp);
		m_StartPnts1.push_back(tmp);  // Zielpunkt

		if (m_single == false)
		{
			// Berechne neue Verbindungspunkte für die Zustellung - SLAVE
			m_it2--;
			
			(*m_it2)->D0((*m_it2)->LastParameter(),tmp);
			m_StartPnts2.push_back(tmp);  // Startpunkt
			
			m_it2++;
			
			(*m_it2)->D0((*m_it2)->FirstParameter(),tmp);
			m_StartPnts2.push_back(tmp);  // Zielpunkt
		}
    }
    else
    {
        return true;  // erste Zustellung immer gleich
    }

    if (m_StartPnts1[0].Z() - m_StartPnts1[1].Z() >= 0.0) return true;
    else                                                  return false;
}

/* Füllt die Outputvektoren für die erste Zustellung. Der Eingabeparameter <brob> legt den Ausgabetyp fest*/
bool path_simulate::ConnectPaths_xy(bool brob)
{
    int N;
    double t = m_t0;
      
    std::vector<Base::Vector3d> tmp2;
	std::vector<double> d;
	Base::Vector3d tmp;
	
	gp_Pnt tmpPnt, pnt1, pnt2, p;
	gp_Vec vec_t(m_StartPnts1[0], m_StartPnts1[1]);
	
	if( 1e-3 > vec_t.Magnitude())  // keine Zustellung erforderlich
		return true;

    if (m_single == false)
    {
        gp_Vec vec_1(m_StartPnts1[0], m_StartPnts1[1]); 
        gp_Vec vec_2(m_StartPnts2[0], m_StartPnts2[1]);

        gp_Vec2d vec_11, // Speichert Master-Zustellung in XY-Richtung
			     vec_21; // Speichert Master-Zustellung in  Z-Richtung

        vec_11.SetX(vec_1.X());  
        vec_11.SetY(vec_1.Y());

        if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())  // erster lauf -> xy zustellung (Slave)
        {
            vec_21.SetX(vec_2.X());
            vec_21.SetY(vec_2.Y());
        }
        else
        {
            vec_21.SetX(0.0);
            vec_21.SetY(vec_2.Z());  // slave zustellung in z-Richtung (ab 2.Lauf)
        }

        // Simulationsoutput
        if (brob == false)
        {
			// ***** MASTER ******
            
			ParameterCalculation_Line(vec_11.Magnitude());

            if (vec_11.Magnitude() != 0)
                vec_11.Normalize();

            N = std::max(2, int(ceil((m_T - m_t0)/m_step))); // Anzahl der zu erzeugenden Outputwerte
            m_del_t = (m_T - m_t0)/N;                   // Zeitschrittweite

            for (int i=0; i<N; ++i)
            {
				// Erzeuge XY-Outputvektor
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

            N = std::max(2, int(ceil((m_T - m_t0)/m_step)));  // Anzahl der zu erzeugenden Outputwerte
            m_del_t = (m_T - m_t0)/N;                    // Zeitschrittweite

            for (int i=0; i<N; ++i)
            {
                if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())
                {
					// Erzeuge XY-Outputvektor
                    tmp.x = vec_21.X()*GetVelocity(t);
                    tmp.y = vec_21.Y()*GetVelocity(t);
                    tmp.z = 0.0;
                }
                else
                {
					// Erzeuge Z-Outputvektor
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

			// Nullvektor
            tmp.x = 0.0;
            tmp.y = 0.0;
            tmp.z = 0.0;

            tmp2.push_back(tmp);

            m_Output.push_back(tmp2);
            m_Output2.push_back(tmp2);
        }
        else  // Roboter Output für den beidseitigen Fall
        {
            bool con = false;

            // hier wird die Anzahl <N> der Punkte für die Diskretisierung der Zustellungslinie berechnet
            if (vec_11.Magnitude() > vec_21.Magnitude()) N = std::max(2, int(ceil(vec_21.Magnitude()/TolDist)));
            else     				                     N = std::max(2, int(ceil(vec_11.Magnitude()/TolDist)));

            if (vec_11.Magnitude() == 0.0 && vec_21.Magnitude() == 0.0) N=0;
			if (!m_conn) con = true;

            // Erster Punkt wird stets weggelassen
            if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())
            {
				// Startpunkt Master
                tmp.x = m_StartPnts1[0].X();
                tmp.y = m_StartPnts1[0].Y();
                tmp.z = m_StartPnts1[0].Z();
                
				m_Output_robo1.push_back(tmp);
                RoboFlag_Master.push_back(0);

				// Startpunkt Slave
                tmp.x = m_StartPnts2[0].X();
                tmp.y = m_StartPnts2[0].Y();
                tmp.z = m_StartPnts2[0].Z();
                
				m_Output_robo2.push_back(tmp);
                RoboFlag_Slave.push_back(0);
            }

            // Erzeuge Output - MASTER
            for (int i=1; i<N; ++i)
            {
                tmp.x = m_StartPnts1[0].X() + (double(i)*vec_11.X())/(double(N)-1.0);
                tmp.y = m_StartPnts1[0].Y() + (double(i)*vec_11.Y())/(double(N)-1.0);
                tmp.z = m_StartPnts1[con].Z();

                m_Output_robo1.push_back(tmp);
                RoboFlag_Master.push_back(0);
            }

            // // Erzeuge Output - SLAVE
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
				// Erzeuge XY-Outputvektor
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

			// Nullvektor 
            tmp.x = 0.0;
            tmp.y = 0.0;
            tmp.z = 0.0;

            tmp2.push_back(tmp);
            m_Output.push_back(tmp2);
            tmp2.clear();
        }
        else
        {
            N = (int) vec_11.Magnitude();  // Anzahl der zu erzeugenden Outputwerte

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

/* Füllt die Outputvektoren für die zweite Zustellung. Der Eingabeparameter <brob> legt den Ausgabetyp fest*/
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

            N = std::max(2, int(ceil((m_T - m_t0)/m_step)));  // Anzahl der zu erzeugenden Outputwerte
            m_del_t = (m_T - m_t0)/N;                    // Zeitschrittweite

            for (int i=0; i<N; ++i)
            {
				// Erzeuge Z-Outputvektor
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

            N = std::max(2, int(ceil((m_T - m_t0)/m_step)));  // Anzahl der zu erzeugenden Outputwerte
            m_del_t = (m_T - m_t0)/N;                    // Zeitschrittweite

            for (int i=0; i<N; ++i)
            {
                if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())
                {
					// Erzeuge Z-Outputvektor im allerersten Schritt
                    tmp.x = 0.0;
                    tmp.y = 0.0;
                    tmp.z = vec_12.Y()*GetVelocity(t);
                }
                else // Slave Zustellung in XY-Richtung
                {
					// Erzeuge XY-Outputvektor
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
        else  // Roboteroutput
        {
            if (vec_11.Magnitude() > vec_12.Magnitude()) std::max(2, N = int(ceil(vec_12.Magnitude()/TolDist)));  // Anzahl der zu erzeugenden Outputwerte
            else                                         std::max(2, N = int(ceil(vec_11.Magnitude()/TolDist)));  // Anzahl der zu erzeugenden Outputwerte
    
            if (vec_11.Magnitude() == 0.0 && vec_12.Magnitude() == 0.0) N=1;

            for (int i=1; i<N; ++i)
            {
				/*MASTER*/

				// Erzeuge Outputvektor für die Zustellung in Z-Richtung
                tmp.x = m_StartPnts1[m_conn].X();
                tmp.y = m_StartPnts1[m_conn].Y();
                tmp.z = m_StartPnts1[0].Z() + (double(i)*vec_11.Y())/double(N-1);

                m_Output_robo1.push_back(tmp);
                RoboFlag_Master.push_back(0);



				/*SLAVE*/

                if (m_it1 == m_BSplineTop.begin() && m_it2 == m_BSplineBottom.begin())
                {   
					// Erzeuge Outputvektor für die Zustellung in Z-Richtung (nur im allerersten Schritt)
                    tmp.x = m_StartPnts2[1].X();
                    tmp.y = m_StartPnts2[1].Y();
                    tmp.z = m_StartPnts2[0].Z() + (double(i)*vec_12.Y())/double(N-1);
                }
                else
                {
					// Erzeuge Outputvektor für die Zustellung in XY-Richtung
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

            N = std::max(2, int(ceil((m_T - m_t0)/m_step)));  // Anzahl der zu erzeugenden Outputwerte
            m_del_t = (m_T - m_t0)/N;                    // Zeitschrittweite

            for (int i=0; i<N; ++i)
            {
                m_Output_time.push_back(t);

				// Erzeuge Z-Outputvektor
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

			// Nullvektor
            tmp.x = 0.0;
            tmp.y = 0.0;
            tmp.z = 0.0;

            tmp2.push_back(tmp);
            m_Output.push_back(tmp2);
            tmp2.clear();
        }
        else // Roboteroutput
        {
            N =(int) vec_12.Magnitude();  // Anzahl der zu erzeugenden Outputwerte

            for (int i=0; i<N; ++i)
            {
				// Erzeuge Outputvektor für die Zustellung in XY-Richtung
                tmp.x = m_StartPnts1[0].X() + (i*vec_12.X())/N;
                tmp.y = m_StartPnts1[0].Y() + (i*vec_12.Y())/N;
                tmp.z = m_StartPnts1[0].Z();

                m_Output_robo1.push_back(tmp);
            }
        }
    }

    return true;
}

/* Füllt die Outputvektoren für die Zustellung für den Feature-Basierten Fall. 
Der Eingabeparameter <brob> legt den Ausgabetyp fest*/
bool path_simulate::ConnectPaths_Feat(bool tool,  // Tool           (Master, Slave)
                                      bool brob,  // Ausgabetyp     (Roboter, Simulation)
                                      bool c_typ) // Zustellungsart (in zwei bzw. drei Schritten)
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

    dir = CheckConnect(tool);  // setze Starpunkte neu

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

    if (c_typ)  // Zustellung in 2 Schritten
    {
        ind = 2;
        vec_tmp[0].SetCoord(0.0, 0.0, abs(ConnPnts[1].Z()-ConnPnts[0].Z()));
        vec_tmp[1].SetCoord(ConnPnts[1].X()-ConnPnts[0].X(), ConnPnts[1].Y()-ConnPnts[0].Y(), 0.0);

        if (dir) // tool  (Master/Slave) muss runter fahren
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
        else   // tool  (Master/Slave) muss hoch fahren
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
    else // Zustellung in 3 Schritten
    {
        ind = 3;

        vec_tmp[0].SetCoord(0.0, 0.0, rad);
        vec_tmp[1].SetCoord(ConnPnts[1].X()-ConnPnts[0].X(), ConnPnts[1].Y()-ConnPnts[0].Y(), 0.0);
        vec_tmp[2].SetCoord(0.0, 0.0, abs(ConnPnts[1].Z()-ConnPnts[0].Z()) + rad);

        if (dir) // tool  (Master/Slave) muss runter fahren
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
        else   // tool  (Master/Slave) muss hoch fahren
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

    if (brob) // Roboteroutput
	{   
		for (int i=0; i<ind; ++i)
		{
			N = std::max(2, int(ceil(vec[i].Magnitude()/TolDist)));  // Anzahl der zu erzeugenden Outputwerte

			for(int j=1; j<N; ++j)
			{
				tmp.x = ConnPnts[0].X() + (double(j)*vec[i].X())/(double(N)-1.0);
				tmp.y = ConnPnts[0].Y() + (double(j)*vec[i].Y())/(double(N)-1.0);
				tmp.z = ConnPnts[0].Z() + (double(j)*vec[i].Z())/(double(N)-1.0);;

				if(!tool) m_Output_robo1.push_back(tmp);  // Master
				else      m_Output_robo2.push_back(tmp);  // Slave
			}

			ConnPnts[0].SetCoord(tmp.x , tmp.y, tmp.z); // Setze Startpunkt = Endpunkt für die nächste Iteration
		}

		return true;
	}
	
	if(ConnPnts[0].Distance(ConnPnts[1]) < 1e-3) return true;  // keine Zustellung bei Spiralbahnen !!!

    for (int i=0; i<ind; ++i)
    {
        t = m_t0;
        ParameterCalculation_Line(vec[i].Magnitude());
        if (vec[i].Magnitude() != 0.0) vec[i].Normalize();
        else continue;

        N = std::max(2,(int) ceil((m_T - m_t0)/m_step)); // Anzahl der zu erzeugenden Outputwerte
        m_del_t = (m_T - m_t0)/N;                   // Zeitschrittweite

        for (int j=0; j<N; ++j)
        {
            Times.push_back(t);
            vel = GetVelocity(t);

            tmp.x = vec[i].X()*vel;
            tmp.y = vec[i].Y()*vel;
            tmp.z = vec[i].Z()*vel;

            tmp2.push_back(tmp);
            Out.push_back(tmp2);  // Fülle temporären Output-Vektor (wird weiter unten zugewiesen)

            t += m_del_t;
			tmp2.clear();
        }

        m_t0 = m_T; // Endzeit des letzten Durchlaufs wird zur neuen Startzeit
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

/* Hier wird unter Berücksichtigung der Krümmungstoleranz <curvTOL> eine Unterteilung der Kurve vorgenommen.
Die Bereichsgrenzen werden im Rückgabevektor zurückgeliefert. Der Ausgabevektor ist leer wenn die maximale Krümmung
den Toleranzwert nicht überschreitet und somit auch keine Unterteilung notwendig ist */ 
std::vector<std::vector<double> > path_simulate::CompBounds(bool tool,std::vector<double> knots)
{
	m_curMax = 0.0; // setze maximale Krümmung initial auf Null	
	double cr_bound = 1/curvTOL;
	double cr_last;
	gp_Vec dtmp1, dtmp2;
	gp_Pnt dtmp0;
    GeomAdaptor_Curve curve;
	std::vector<double> single_bound;
    std::vector<double> bounds;
    std::vector<std::vector<double> > CriticalBounds;

    // lade aktuelle Kurve
    if (!tool)  curve.Load(*m_it1);
    else        curve.Load(*m_it2);

    double fParam = curve.FirstParameter(), // Erster Kurvenparameter
           lParam = curve.LastParameter(),  // Letzter Kurvenparameter
     	   period = lParam - fParam;        // Länge des Parameterbereichs
 

	int n  = knots.size();  // Länge des Knotenvektors
	bool b = false;

	// Hier erfolgt die Berechnung der maximalen Krümmung <m_curMax>
	// Die Parameter der Bereichsgrenzen an denen die Kurvenkrümmung dem Toleranzwert <cr_bound> 
	// entspricht werden in den Vektor <bounds> für die weitere Nachbearbeitung gefüllt
    for (int i=0; i<n; ++i)
    {
		curve.D2(knots[i], dtmp0, dtmp1, dtmp2); // Da es sich um eine kubische B-Spline Kurve handelt,
		                                         // kann die maximale Krümmung nur an den Knotenpunkten angenomen werden
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
		
		cr_last = dtmp2.Magnitude(); // Krümmung entspricht hier dem Betrag der zweiten Ableitung
			
		if(m_curMax < cr_last)  // Speichert maximale Krümmung
			m_curMax = cr_last;
	}

	if(period < 2*m_boundTol || bounds.size() == 0)
		return CriticalBounds;

	if(b) bounds.push_back(lParam - m_boundTol);  // Hier muss evtl. noch die letzte Grenze eingefügt werden

	n = (int) bounds.size()/2;  // <bounds> hat stets gerade Länge
	for(int i=0; i<n; i++)      // Fasse Bereiche welche einen geringeren Abstand als <m_boundTol> haben zusammen
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

/* Hier wird die Vorarbeit für die Funktion Gen_Path() geleistet.
Die Vektoren <m_length>, <m_velocity>, <m_accel> werden für die aktuelle Kurve gefüllt.*/ 
bool path_simulate::CompPath(bool tool) // tool = 0  -> Master
                                        // tool = 1  -> Slave
{
	m_boundTol = pow(m_vmax, 2.0)/m_amax; // Toleranzbereich vor kritischen Bereichen (notwendig zum Hochbeschleunigen)
	
	double cur     = 1.0/curvTOL,         // Krümmungstoleranz für die Kurvenunterteilung
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
   
	double t0 = m_t0; // Übergibt aktuelle Startzeit

	int num = Detect_FeatCurve(tool);  // Liefert Anzahl der zu fahrenden Kurven (i.d.R. num = 1) 
	
	for(int a=0; a<num; a++)  // Schleife über die zusammenhängenden Kurven
	{
		// lade aktuelle kurven
	    if (!tool)  curve.Load(*m_it1);
		else        curve.Load(*m_it2);

		// setze Parameter neu
		fParam = curve.FirstParameter();
		lParam = curve.LastParameter();
		period = lParam - fParam;

		// übergibt Starparameter der aktuellen Kurve
		m_StartParam[tool] = fParam;
		start = m_StartParam[tool];
		
		// *** Berechnung des Knotenvektors ***
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

		// *** Ende der Berechnung des Knotenvektors ***


		std::vector<std::vector<double> > CriticalBounds = 
			                                CompBounds(tool, knot_vec); // Berechnet auf Basis der aktuellen Kurve, 
																	    // die kritischen Bereiche im Parameterraum und 
																	    // berechnet gleichzeitig die maximale Kurvenkrümmung <m_curMax>
		
		m_vmid = std::min(m_vmax,sqrt(m_amax/m_curMax)); // Legt Geschwindigkeit fest mit der allen kritischen Bereiche     
		                                            // abgefahren werden

Newtry: // Falls die generierten Weglängen nicht ausreichen, dann wird ein neuer Versuch mit halbem <m_vmid> gestartet
		
		v[0] = 0.0;  // starte jede Kurve mit v = 0
		int m = 0;

		for (unsigned int i=0; i<CriticalBounds.size(); ++i)  // Schleife über einzelne Unterteilungsbereiche
		{
			d2 = 0.0;
			pos = m_Knots->Value(m);

			while(pos < m_StartParam[tool])
			{
				m++;
				pos = m_Knots->Value(m);
			}
			
			/*------------------- Gerader Bereich ---------------------*/
			
			while(pos < CriticalBounds[i][0]) // Berechnung der maximalen Krümmung dieses geraden Bereichs
			{
				curve.D2(pos, pnt0, pnt1, pnt2);
				cur_tmp = pnt2.Magnitude();   // Krümmung am aktuellen Punkt
				
				if(d2 < cur_tmp) 
					d2 = cur_tmp;             // Speichert maximale Krümmung dieses geraden Bereichs

				m++;
				pos = m_Knots->Value(m);
			}

			tetha = 0.6 + 0.25*sqrt(d2/m_curMax);         // Setzt tetha-parameter -> (0.6 < tetha < 0.85)
			velo = std::min(m_vmax, tetha*(sqrt(m_amax/d2)));  // Setzt maximale Geschwindigkeit <velo>
			m_a = m_amax - d2*velo*velo;                  // wenn <velo> zu groß gewählt wurde, kann <m_a> evtl. negativ werden

			// Korrektur
			while(m_a <= 0.0)
			{
				velo = velo/2.0;
				m_a = m_amax - d2*velo*velo;
			}

			if(velo < m_vmid)
				m_vmid = velo;

			v[1] = velo;

			/*-------------- Korrektur der Geschwindigkeiten (falls der Weg zu lang) -------------*/

			len = CriticalBounds[i][0] - m_StartParam[tool];              // Länge des i. geraden Abschnitts
			len_1 = (pow(v[1] - v[0],2.0) + pow(v[1] - m_vmid,2.0))/m_a;  // notwendige Länge

			if(len < pow(v[0] - m_vmid,2.0)/m_a)  // für diesen Fall ist keine Korrektur möglich
			{
				l_vec.clear();
				v_vec.clear();
				a_vec.clear();

				m_StartParam[tool] = start; // setze Startparameter zurück
			    m_vmid = m_vmid/2;          // halbiere kritische Durchlaufgeschwindigkeit
				goto Newtry;
			}

			while(len < len_1)
			{
				v[1] = v[0] + (v[1] - v[0])/2.0;
				len_1 = (pow(v[1] - v[0],2.0) + pow(v[1] - m_vmid,2.0))/m_a; 
			}

			v[2] = m_vmid;

			/*---Korrekturende---*/	

			// Fülle Vektoren
			l_vec.push_back(len);  // Länge
			v_vec.push_back(v);    // Geschwindigkeiten
			a_vec.push_back(m_a);  // Beschleunigung
	        
			m_StartParam[tool] += len;   // setzt Startparameter neu

			/*------------------------ gekrümmter Bereich ------------------------*/

			len = CriticalBounds[i][1] - CriticalBounds[i][0];  // Bogenlänge des gekrümmten Bereichs
			m_StartParam[tool] += len;                          // setzt Startparameter neu
			v[0] = v[2];                                        // Endgeschwindigkeit wird zur Startgeschwindigkeit
			
			l_vec.push_back(len);
		}

		d2 = 0;	
		
		// Korrigiere aktuellen Knotenparameter
		while(pos < m_StartParam[tool])
		{
			m++;
			pos = m_Knots->Value(m);
		}

		pos = m_Knots->Value(m);
	    
		// Berechnung der maximalen Krümmung für den letzten Abschnitt
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
		velo = std::min(m_vmax, tetha*(sqrt(m_amax/d2))); // vgl. oben
			
		v[1] = velo;
		v[2] = 0.0;
		m_a = m_amax - d2*velo*velo; // wenn velo zu groß gewählt wurde, kann m_a negativ werden

		// Korrektur
		while(m_a <= 0.0)
		{
			velo = velo/2.0;
			v[1] = velo;
			m_a = m_amax - cur*velo*velo;
		}

		d2 = 0;

		len = lParam - m_StartParam[tool] + start;          // Länge des letzten geraden Abschnitts
		len_1 = (pow(v[1] - v[0],2.0) + pow(v[1],2.0))/m_a; // notwendige Länge

		if(len < pow(v[0],2.0)/m_a) // hier keine Korrektur möglich
		{
			l_vec.clear();
			v_vec.clear();
			a_vec.clear();

			m_StartParam[tool] = start; // setze Startparameter zurück
			m_vmid = m_vmid/2;          // halbiere kritische Durchlaufgeschwindigkeit und versuchs erneut
			goto Newtry;
		}

		while(len < len_1)
		{
			v[1] = v[0] + (v[1] - v[0])/2;
			len_1 = (pow(v[1] - v[0],2.0) + pow(v[1],2.0))/m_a; 
		}

		// Fülle Vektoren
		l_vec.push_back(len);  // Länge
		v_vec.push_back(v);    // Geschwindigkeiten
		a_vec.push_back(m_a);  // Beschleunignung
		
		// Fülle hier erst die Ausgabevektoren (einmal pro Kurve)
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

		// lade aktuelle kurven
	    if (!tool)  m_it1++;
		else        m_it2++;

		m_StartParam[tool] = start;
		
		l_vec.clear();
		v_vec.clear();
		a_vec.clear();
		
		curve.Delete();
		CriticalBounds.clear();
	}

	// Setze Iterator und Startparameter wieder zurück
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

/* Integriert erst nach der Trapezregel die Outputwerte nach der Zeit und liefert den Fehlervektor als Verbindung von Start- und Endpunkt.
Abschließend erfolgt die Korrektur der Outputvektoren um eben diesen Fehlervektor */
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

	// Hier erfolgt die numerische Integration des Outputvektors nach der Trapezregel und der
	// entstehende Fehlervektor wird in <vec> abgelegt
    if (tool==false) // Master
    {
		N = m_Output.size();
        for (int i=1; i<N; ++i)
        {
            Sum[0] += (m_Output[i][0].x + m_Output[i-1][0].x)*(m_Output_time[i] - m_Output_time[i-1]) / 2.0;
            Sum[1] += (m_Output[i][0].y + m_Output[i-1][0].y)*(m_Output_time[i] - m_Output_time[i-1]) / 2.0;
            Sum[2] += (m_Output[i][0].z + m_Output[i-1][0].z)*(m_Output_time[i] - m_Output_time[i-1]) / 2.0;
        }

		if(m_StartPnts1[0].Distance(m_StartPnts1[1]) > 1e-3)  // falls Kurve NICHT geschlossen
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

		if(m_StartPnts2[0].Distance(m_StartPnts2[1]) > 1e-3)  // falls Kurve NICHT geschlossen
		{
			vec.SetCoord(Sum[0],Sum[1],Sum[2]);
			vec.SetCoord(m_StartPnts2[1].X() - m_StartPnts2[0].X() - vec.X(),
						 m_StartPnts2[1].Y() - m_StartPnts2[0].Y() - vec.Y(),
						 m_StartPnts2[1].Z() - m_StartPnts2[0].Z() - vec.Z());
		}
		else
			vec.SetCoord(-Sum[0],-Sum[1],-Sum[2]);
    }

    ParameterCalculation_Line(vec.Magnitude());  // Berechnung der Zeitgrenzen

    N = (int) ceil((m_T - m_t0)/m_step);  // Anzahl der zu erzeugenden Outputwerte
    m_del_t = (m_T - m_t0)/N;             // Zeitschrittweite

    if (N==1)  // Nur ein Outputvektor
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

        if(tool==false)  m_Output.push_back(tmp2);  // füllt Master-Output
        else             m_Output2.push_back(tmp2); // füllt Slave-Output

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

			// Ausgabevektor zum Zeitpunkt <t>
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

	// Nullvektor
	tmp.x = 0.0;
    tmp.y = 0.0;
    tmp.z = 0.0;
    
	tmp2.push_back(tmp);
	
	// Übergebe Nullvektor zur Endzeit <m_T>
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
// Hier wird der eigentliche Output für den Simulationsprozess generiert unter der Berücksichtigung 
// der in CompPath() berechneten Vektoren m_velocity, m_accel, m_length
bool path_simulate::Gen_Path()
{
	int n,m;	
	double length, lam, t_start, t_ma, t_sl, t_tmp; 

	t_start = m_t0;                           // setze Startzeit
	m_StartPnts1[0] = (*m_it1)->StartPoint(); // setze Startparameter

	if(m_single == false)
	{
		m_StartPnts2[0] = (*m_it2)->StartPoint();

/*------------------------------ Berechne Durchlaufzeit für das Master-Tool --------------------------------*/

		n = m_velocity_ma.size();
		for(int i=0; i<n; i++)                    // Schleife über die (zusammenhängenden) Kurven 
		{
			m = m_velocity_ma[i].size();       
			for(int j=0; j<m; j++)                // Schleife über die einzelnen Unterteilungsgebiete 
			{
				m_v[0] = m_velocity_ma[i][j][0];  // Startgeschwindigkeit des i-ten Abschnitts
				m_v[1] = m_velocity_ma[i][j][1];  // maximale Geschwindigkeit des i-ten Abschnitts
				m_v[2] = m_velocity_ma[i][j][2];  // Endgeschwindigkeit des i-ten Abschnitts

				m_a = m_accel_ma[i][j];

				length = m_length_ma[i][2*j];      // Länge des unkritischen Abschnitts 
												   // Bem: Jede Kurve beginnt und endet in einem unkritischen Abschnitt
				ParameterCalculation_Curve(length);   // Hier wird die Endzeit m_T berechnet

				if(j != m-1)
				{
					length = m_length_ma[i][2*j+1];// Länge des kritischen Abschnitts 
					m_T = m_T + length/m_v[2];     // Berechnet neue Endzeit (kritische Abschnitte werden mit konstanter
												   // Geschwindigkeit m_v[2] durchlaufen)
				}

				m_t0 = m_T;                        // Endzeit wird zur Startzeit
			}
		}

		t_ma = m_T - t_start;                  // Entspricht Durchlaufzeit für den Master
		m_t0 = t_start;                        // Setzt Startzeit zurück

/*-------------------------- Berechnung der Durchlaufzeit für das Slave-Tool -------------------------------*/

		n = m_velocity_sl.size();
		for(int i=0; i<n; i++)                    // Schleife über die (zusammenhängenden) Kurven 
		{
			m = m_velocity_sl[i].size();       
			for(int j=0; j<m; j++)                // Schleife über die einzelnen Unterteilungsgebiete 
			{
				m_v[0] = m_velocity_sl[i][j][0];  // Startgeschwindigkeit des i-ten Abschnitts
				m_v[1] = m_velocity_sl[i][j][1];  // maximale Geschwindigkeit des i-ten Abschnitts
				m_v[2] = m_velocity_sl[i][j][2];  // Endgeschwindigkeit des i-ten Abschnitts

				m_a = m_accel_sl[i][j];

				length = m_length_sl[i][2*j];      // Länge des unkritischen Abschnitts 
												   // Bem: Jede Kurve beginnt und endet in einem unkritischen Abschnitt
				ParameterCalculation_Curve(length);// Hier wird u.a. die Endzeit <m_T> berechnet

				if(j != m-1)
				{
					length = m_length_sl[i][2*j+1];// Länge des kritischen Abschnitts 
					m_T = m_T + length/m_v[2];     // Berechnet neue Endzeit (kritische Abschnitte werden mit der konstanten
												   // Geschwindigkeit <m_v[2]> durchlaufen)
				}

				m_t0 = m_T;                        // Endzeit wird zur Startzeit
			}
		}
									   										   
		t_sl = m_T - t_start;				   // Entspricht Durchlaufzeit für den Slave
		m_t0 = t_start;						   // Setzt Startzeit zurück
						
/*----------------------- Synchronisierung von Master und Slave -----------------------------*/

// Idee: Skaliere die Geschwindigkeiten und Beschleunigungen derjenigen Bahn mit der kürzeren Durchlaufzeit,
// so dass die Durchlaufzeiten für Master und Slave übereinstimmen

		
		if(t_ma <= t_sl)					   
		{
			// Ab hier: Korrektur für den Master

			lam = t_ma/t_sl;  // Skalierungsfaktor 0 < lam <= 1
			
			// Zunächst werden alle Geschwindigkeiten runterskaliert
			n = m_velocity_ma.size();
			for(int i=0; i<n; i++)
			{
				m = m_velocity_ma[i].size();
				for(int j=0; j<m; j++)
				{
					// linearen Beziehung zwischen den Geschwindigkeiten und der Durchlaufzeit
					m_velocity_ma[i][j][0] = lam*m_velocity_ma[i][j][0];
					m_velocity_ma[i][j][1] = lam*m_velocity_ma[i][j][1];
					m_velocity_ma[i][j][2] = lam*m_velocity_ma[i][j][2];
				}
			}

			// Skalierung der Beschleunigungen
			n = m_accel_ma.size();
			for(int i=0; i<n; i++)
			{
				m = m_accel_ma[i].size();
				for(int j=0; j<m; j++)
					m_accel_ma[i][j] = lam*lam*m_accel_ma[i][j];  // Die Beschleunigungen müssen mit dem Quadrat des Skalierungsfaktors
			}                                                     // multipliziert werden (quadratische Abhängigkeit zur Durchlaufzeit)
		}
		else
		{
			// Ab hier: Korrektur für den Slave

			lam = t_sl/t_ma;  // Skalierungsfaktor 0 < lam <= 1
			
			// Zunächst werden alle Geschwindigkeiten runterskaliert
			n = m_velocity_sl.size();
			for(int i=0; i<n; i++)
			{
				m = m_velocity_sl[i].size();
				for(int j=0; j<m; j++)
				{
					// vgl. Master
					m_velocity_sl[i][j][0] = lam*m_velocity_sl[i][j][0];
					m_velocity_sl[i][j][1] = lam*m_velocity_sl[i][j][1];
					m_velocity_sl[i][j][2] = lam*m_velocity_sl[i][j][2];
				}
			}

			// Skalierung der Beschleunigungen
			n = m_accel_sl.size();
			for(int i=0; i<n; i++)
			{
				m = m_accel_sl[i].size();
				for(int j=0; j<m; j++)
					m_accel_sl[i][j] = lam*lam*m_accel_sl[i][j];  // Die Beschleunigungen müssen mit dem Quadrat des Skalierungsfaktors
			}                                                     // multipliziert werden
		}
	}

/*------------------------------- Generierung der Outputvektoren ------------------------------------*/
	
/*MASTER*/	
	
	bool l;
	int  q,p;

	n = m_length_ma.size();
	for(int i=0; i<n; i++)  // Schleife geht über die Anzahl der zu fahrenden Kurven, vgl. path_simulate::CompPath()
	{
		if(i!=0)
			m_it1++;

		l = false;  // Legt fest ob wir uns gerade in einem kritischen Abschnitt befinden
		q = 0;      // Zu Beginn jeder Kurve auf Null
	    p = 0;      // Zu Beginn jeder Kurve auf Null

		m_StartParam[0] = (*m_it1)->FirstParameter(); // Setzt Startparameter der aktuellen Kurve
		
		m = m_length_ma[i].size();
		for(int j=0; j<m; j++)
		{
			if(l==false)  // wird in jedem zweiten Schritt aufgerufen
			{
				m_a = m_accel_ma[i][p];
				
				m_v[0] = m_velocity_ma[i][q][0];
				m_v[1] = m_velocity_ma[i][q][1];
				m_v[2] = m_velocity_ma[i][q][2];
				
				p++;
				q++;
			}

			MakePathSingle(0,m_length_ma[i][j],l,0);  // Hier werden die Outputvektoren erzeugt
			
			// Update der Variablen 
			m_StartParam[0] += m_length_ma[i][j];
			m_t0 = m_T;
			l = !l;
		}
	}

	m_t0 = t_start;

/*ENDE MASTER*/

	
/*SLAVE*/
	
	// Analog zum Master (s.o.)
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

			// vgl. Master
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

				MakePathSingle(0,m_length_sl[i][j],l,1);  // hier wird der output für den slave generiert
				m_StartParam[1] += m_length_sl[i][j];
				m_t0 = m_T;
				l = !l;
			}
		}
	}

/*ENDE SLAVE*/


/*WEG-KORREKTUR*/
	
	t_tmp = m_t0;

	m_StartPnts1[1] = (*m_it1)->EndPoint(); // Setzt Endpunkt, notwendig für die folgende Wegkorrektur
	Correction(0);                          // Wegkorrektur für den Master

	if(m_single == false)
	{
		m_t0 = t_tmp;
		m_StartPnts2[1] = (*m_it2)->EndPoint(); // Setzt Endpunkt, notwendig für die folgende Wegkorrektur
		Correction(1);                          // Wegkorrektur für den Slave
	}

	m_t0 = t_start;

/*ENDE WEG-KORREKTUR*/


	m_velocity_ma.clear();
	m_velocity_sl.clear();
	m_length_ma.clear();
	m_length_sl.clear();
	m_accel_ma.clear();
	m_accel_sl.clear();
	
	return true;
}

/* Hier werden die Simulations-Outputvektoren für die jeweiligen Kurvenabschnitte erzeugt */
bool path_simulate::MakePathSingle(bool   brob,   // Beschreibt Ausgabeart (Roboter, Simulation) 
								   double length, // Bogenlänge des betrachteten Abschnitts
								   bool   part,   // Gibt an ob wir einen kritischen Abschnitt betrachten
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

		for (int i=1; i<N; ++i)  // niemals den ersten punkt mitnehmen
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

    if (part==true) m_T = m_t0 + length/m_v[2]; // kritische Bereiche werden mit konstanter Geschwindigkeit durchlaufen
    else            ParameterCalculation_Curve(length);
    
	N = std::max(2, (int)((m_T-m_t0)/m_step));

    if (N>=100000) 
        N = 99999;  // maximale Anzahl möglicher Outputwerte

	m_del_t = (m_T-m_t0)/double(N);

    std::vector< std::vector<Base::Vector3d> > D0;
    std::vector< std::vector<Base::Vector3d> > D1;

    double t = m_t0;

	//anAdaptorCurve.D0(m_StartParam[tool],tmp);
	//pnt2.Set(tmp.X(),tmp.Y(),tmp.Z());

	if (part == true)  // kritischer Abschnitt
    {
        for (int i=0; i<N; ++i)  // Hauptschleife
        {
            if (!tool) m_Output_time.push_back(t);
            else       m_Output_time2.push_back(t);

            d = m_v[2]*(t-m_t0);
			param = m_StartParam[tool] + d; // Kurvenparameter entspricht zurückgelegtem Weg 
			                                // für den Fall einer Bogenlängenparametrisierung.			
			                                // Bei beliebiger Parametrisierung: 
			                                // param = FindParamAt(anAdaptorCurve, d, m_StartParam[tool]);

			// Berechnet alles bis zur zweiten Ableitung
			if      ( param > lastParam  ){ anAdaptorCurve.D2(param - period, tmp, dtmp1, dtmp2);}
            else if ( param < firstParam ){ anAdaptorCurve.D2(param + period, tmp, dtmp1, dtmp2);}
			else                          { anAdaptorCurve.D2(param,          tmp, dtmp1, dtmp2);}

			//m_times_tmp.push_back(t);
		    //m_velo_tmp.push_back(m_v[2]);

			// Ausgabevektor zum Zeitpunkt <t>
			tmp2.x = dtmp1.X()*m_v[2];
            tmp2.y = dtmp1.Y()*m_v[2];
            tmp2.z = dtmp1.Z()*m_v[2];

            tmp3.push_back(tmp2);
            
			if (!tool) m_Output.push_back(tmp3);
            else       m_Output2.push_back(tmp3);
            
			tmp3.clear();
			
			/*
			if (tool == false)  // zeichnet den tatsächlichen Weg nach
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

    // unkritischer Abschnitt
    for (int i=0; i<N; ++i)
    {
        if (!tool) m_Output_time.push_back(t);
        else       m_Output_time2.push_back(t);
        
		d = GetDistance(t);
		param = m_StartParam[tool] + d;     // Kurvenparameter entspricht zurückgelegtem Weg 
			                                // für den Fall einer Bogenlängenparametrisierung.			
			                                // Bei beliebiger Parametrisierung: 
			                                // param = FindParamAt(anAdaptorCurve, d, m_StartParam[tool]);

		// Berechnet alles bis zur zweiten Ableitung
        if      ( param > lastParam  ){ anAdaptorCurve.D2(param - period, tmp, dtmp1, dtmp2);}
        else if ( param < firstParam ){ anAdaptorCurve.D2(param + period, tmp, dtmp1, dtmp2);}
		else                          { anAdaptorCurve.D2(param,          tmp, dtmp1, dtmp2);}

		velo = GetVelocity(t);  // Berechnet die Geschwindigkeit <velo> des Tools zum Zeitpunkt <t>
		                        // bzgl. den Parametern m_t0, m_t1, m_t2, m_T, m_a, m_v[i], i =1,2,3 

		// Ausgabevektor zum Zeitpunkt <t>
        tmp2.x = dtmp1.X()*velo;
        tmp2.y = dtmp1.Y()*velo;
        tmp2.z = dtmp1.Z()*velo;

        tmp3.push_back(tmp2);
       
		if (!tool) m_Output.push_back(tmp3);
        else       m_Output2.push_back(tmp3);

		tmp3.clear();
		
		/*
		if (tool == false)  // zeichnet den tatsächlichen Weg nach
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

/* Hauptroutine zur Generierung des Roboteroutputs für den normalen beidseitigen Fall */
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

	// Outputgenerierung über alle Kurven
    for (m_it1 = m_BSplineTop.begin(); m_it1 != m_BSplineTop.end(); ++m_it1)
    {
        m_StartParam[0] = ((*m_it1)->FirstParameter());
        if (m_single == false) m_StartParam[1] = ((*m_it2)->FirstParameter());

        /*------ 1.ZUSTELLUNG ------*/
        
		m_conn = CheckConnect();
        
		if (m_conn)   ConnectPaths_xy(1);
        else          ConnectPaths_z(1);

        UpdateParam();
        

		/*------ 2.ZUSTELLUNG ------*/
        
		if (m_conn)   ConnectPaths_z(1);
        else          ConnectPaths_xy(1);

        UpdateParam();

       
		/*------ KURVE ------*/

		MakePathSingle(1,0,0,0);  // Master
        MakePathSingle(1,0,0,1);  // Slave
        UpdateParam();

        if (m_single==false && (m_it1 != (m_BSplineTop.end()-1)))
            ++m_it2;

    }

	int c = 1;

	/*--- Schreibe Roboter-Output ---*/

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

/* Wird nur zu Beginn der Ausschreibung in path_simulate::WriteOurputDouble() aufgerufen und passt die Ausgabevektoren
<m_Output_time>, <m_Output_time2> so aneinander an, dass die Endzeiten übereinstimmen*/ 
bool path_simulate::TimeCorrection()
{
	int N;
	Base::Vector3d vec(0.0,0.0,0.0);
    std::vector<Base::Vector3d> vecc;
	
	vecc.push_back(vec);
    
    if (m_single == false)  // Eine Zeitkorrektur macht nur für diesen Fall auch Sinn
    {
        if(m_Output_time.size() == 0 || m_Output_time2.size() == 0)  // Sonderbehandlung für diesen Fall
		{
			if(m_Output_time.size() > m_Output_time2.size())
			{
				m_Output_time2 = m_Output_time;
				N = m_Output_time2.size();
				m_Output2.resize(N);
				
				for (int i=0; i<N; ++i) // Füllt leeren Output mit Nullvektoren (Werkzeug hat zu warten)
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
				
				for (int i=0; i<N; ++i) // Füllt leeren Output mit Nullvektoren (-> Werkzeug hat zu warten)
				{
					m_Output[i] = vecc;
				}

				return true;
			}

			return true;  //gibt true zurück wenn beide Outputvektoren <m_Output_time> und <m_Output_time2> leer sein sollten
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
    else // falls <m_single> = true (d.h nur Master -> keine Zeitkorrektur erforderlich!!!)
    {
        return false;
    }

    return true;
}

/* Hauptroutine zur Generierung des Simulationsoutputs für den feature-basierten und spiral-basierten beidseitigen Fall */
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

    c[0] = 1;    // Start Index der Master Kurven
    c[1] = 2001; // Start Index der Slave  Kurven

    int i = 0;

    while (m_it1 != m_BSplineTop.end() && m_it2 != m_BSplineBottom.end())
    {
        tool = StartingTool();  // bestimmt welches tool warten muss:
								// tool == true  : Roboter = Slave  & NC = Master
								// tool == fasle : Roboter = Master & NC = Slave
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
       
        m_StartParam[0] = ((*m_it1)->FirstParameter()); // setze neue Startparameter (in unserem Fall immer 0)
        
		if (m_single == false) 
			m_StartParam[1] = ((*m_it2)->FirstParameter());
 
		// die erste Zustellung vor dem Kontakt mit dem Blech wird hier seperat gehandelt
        if (i==0)
        {
            /*------ ZUSTELLUNG 1 ------*/
            ConnectPaths_xy(0);
            WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam);
            UpdateParam();

            /*------ ZUSTELLUNG 2 ------*/
            ConnectPaths_z(0);
            WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam);
            UpdateParam();
        }

        while (true)
        {
            if (*it_1 != (*curves_1).end()-1)
            {
				/* ------ Kurve ------*/
                CompPath(0); 														 // Berechne Parameter für den Master
				CompPath(1); 														 // Berechne Parameter für den Slave
				Gen_Path();  														 // Erzeuge Output für aktuelle Kurve
				WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Schreibe Output
                UpdateParam();

				/*------ Zustellung ------*/
                (*it_1)++;                     										 // Gehe zur nächsten Kurve
                ConnectPaths_Feat(tool, 0, 1); 										 // Erzeuge Output für Zustellung
				WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Schreibe Output
                UpdateParam();

                if (*it_1 == (*curves_1).end()-1 && *it_2 == (*curves_2).end()-1)    // Letzter Schritt
                {
					/* ------ Kurve ------*/
					CompPath(0);  // Berechne Parameter für den Master
					CompPath(1);  // Berechne Parameter für den Slave
					Gen_Path();	  // Erzeuge Output für aktuelle Kurve
					WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Schreibe Output
					UpdateParam();
                    break;
                }

                (**it_1)->D0((**it_1)->FirstParameter(),pnt0); // übergebe aktuellen Startpunkt

                
                if ((pnt0.Z() > (flatAreas[i+1] + rad[tool] - 1e-1)) &&  // Erreicht der MASTER den nächsten ebenen 
                    (pnt0.Z() < (flatAreas[i+1] + rad[tool] + 1e-1)))    // Bereich, muss auf den SLAVE gewartet werden
                {
					if(!spiral)  // Bei Spiralbahnen erfolgt die Zustellung sofort
					{
						/* ------ Kurve ------*/
						CompPath(0);  // Berechne Parameter für den Master
						CompPath(1);  // Berechne Parameter für den Slave
						Gen_Path();	  // Erzeuge Output für aktuelle Kurve
						WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Schreibe Output
						UpdateParam();
					}
					break;
                }
            }
            else
            {
                if (*it_2 == (*curves_2).end()-1)  // letzter Schritt
                {
					/* ------ Kurve ------*/
                    CompPath(0);  // Berechne Parameter für den Master
					CompPath(1);  // Berechne Parameter für den Slave
					Gen_Path();	  // Erzeuge Output für aktuelle Kurve
					WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Schreibe Output
					UpdateParam();
                    break;
                }
                else break;
            }
        }

        if (m_it1 == m_BSplineTop.end()-1 && m_it2 == m_BSplineBottom.end()-1) // Fertig !!!
            break;

		/* ------ Zustellung ------*/
		if(!spiral) 
		{
			(*it_1)++;						// gehe zur nächsten Kurve
			ConnectPaths_Feat(tool, 0, 1);	// Erzeuge Output für die Master-Zustellung
		}
		
		(*it_2)++;															 // gehe zur nächsten Kurve										 
		ConnectPaths_Feat(!tool, 0, 0);								         // Erzeuge Output für die Slave-Zustellung
		WriteOutputDouble(anOutputFile[0],anOutputFile[1],c[0],c[1],0,beam); // Schreibe Output
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

/* Hauptroutine zur Generierung des Roboteroutputs für den feature-basierten und spiral-basierten beidseitigen Fall */
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
        tool = StartingTool();  // Bestimmt welches tool warten muss

        // Setze Startparameter (in unserem Fall unnötig da immer 0)
        m_StartParam[0] = ((*m_it1)->FirstParameter());
        if (m_single == false) m_StartParam[1] = ((*m_it2)->FirstParameter());

        // Erste Zustellung wird seperat gehandelt
        if (i==0)
        {
            ConnectPaths_xy(1); /* 1. ZUSTELLUNG */
            ConnectPaths_z(1);  /* 2. ZUSTELLUNG */
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

        // Hauptschleife
        while (true)
        {
            MakePathSingle(1,0,0,tool);  // Bahngenerierung des kontinuierlich fahrenden tools für aktuelle kurve

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

// Hilfsfunktion für den Featurebasierten Teil:
// Bestimmt welches Tool wartet, während das andere läuft
bool path_simulate::StartingTool()
{
    double z0,z1;
    gp_Pnt pnt0,pnt1;

    if(m_it1 != m_BSplineTop.end()-1)
    {
        // z-Abstand zur nächsten Bahn - MASTER
        (*m_it1)->D0((*m_it1)->FirstParameter(),pnt0); m_it1++;
        (*m_it1)->D0((*m_it1)->FirstParameter(),pnt1); m_it1--;
       
        z0 = abs(pnt0.Z() - pnt1.Z());
    }

    else
        z0 = 1e+3;

    if(m_it2 != m_BSplineBottom.end()-1)
    {
        // z-Abstand zur nächsten Bahn - SLAVE
        (*m_it2)->D0((*m_it2)->FirstParameter(),pnt0); m_it2++;
        (*m_it2)->D0((*m_it2)->FirstParameter(),pnt1); m_it2--;
        
        z1 = abs(pnt0.Z() - pnt1.Z());
    }
    else z1 = 1e+3;

    if(z0<z1)  return false;  // Slave muss warten
    else       return true;   // Master muss warten
}

/* Hier wird geschaut, ob es sich um eine geschlossene Kurve handelt bzw. wieviele Kurven abgefahren werden müssen
um wieder am Anfangspunkt der Kurve anzukommen. Die Anzahl der zu fahrenden Kurven wird als integer zurückgegeben */
int path_simulate::Detect_FeatCurve(bool tool)
{
    gp_Pnt pt0,pt1;
    int num = 1;

    if(!tool)
	{
		pt0 = (*m_it1)->StartPoint(); // Startpunkt der aktuellen Master-Kurve
		pt1 = (*m_it1)->EndPoint();   // Endpunkt der aktuellen Master-Kurve
		
		while(pt0.Distance(pt1) > 1e-3)  // Mache weiter solange Start und Endpunkt nicht passen
		{
			if(m_it1 == m_BSplineTop.end()-1)  // Stoppe wenn bei der letzten Kurve angekommen
			{
				for(int i=1; i<num; i++)
					m_it1--;

				num = 1;
				pt1 = pt0; // Damit die äußere while-Schleife verlassen wird
			}
			else
			{
				num++;
				m_it1++;
				pt1 = (*m_it1)->EndPoint();
			}
		}

		// Zurück zum Status quo
		for(int i=1; i<num; i++)
			m_it1--;
	}
	else
	{	
		pt0 = (*m_it2)->StartPoint(); // Startpunkt der aktuellen Slave-Kurve
		pt1 = (*m_it2)->EndPoint();   // Endpunkt der aktuellen Slave-Kurve
		
		while(pt0.Distance(pt1) > 1e-3) // Mache weiter solange Start und Endpunkt nicht passen
		{
			if(m_it2 == m_BSplineBottom.end()-1) // Stoppe wenn bei der letzten Kurve angekommen
			{
				for(int i=1; i<num; i++)
					m_it2--;

				num = 1;
				pt1 = pt0;  // Damit die while-Schleife verlassen wird
			}
			else
			{			
				num++;
				m_it2++;
				pt1 = (*m_it2)->EndPoint();
			}
		}

		// Zurück zum Status quo
		for(int i=1; i<num; i++)
			m_it2--;
	}

	return num;
}

/* Hauptroutine zur Generierung des Simulationsoutputs für den normalen beidseitigen Fall */
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

    for (m_it1 = m_BSplineTop.begin(); m_it1 < m_BSplineTop.end(); ++m_it1)  // Schleife über alle Kurven 
    {
        m_StartParam[0] = ((*m_it1)->FirstParameter());     // speichert Startparameterwert der aktuellen Master-Kurve
       
		if (m_single == false)
			m_StartParam[1] = ((*m_it2)->FirstParameter()); // speichert Startparameterwert der aktuellen Slave-Kurve
      

		/*Zustellung Start*/

        m_conn = CheckConnect(); // Rückgabewert = 1 bei negativer z-Richtungszustellung
                         		 // Rückgabewert = 0 bei positiver z-Richtungszustellung 
       
		//  negative z-Richtung: 1. XY --> 2. Z              
		//  positive z-Richtung: 1. Z  --> 2. XY    
 
		// *** 1. ***
		if (m_conn)   ConnectPaths_xy(0);
        else          ConnectPaths_z(0);

        if (m_single == false) WriteOutputDouble(anOutputFile,anOutputFile2,c1,c2,0,beam);
        else                   WriteOutputSingle(anOutputFile,c1,0,0,beam);

        UpdateParam();

		// *** 2. ***
        if (m_conn)   ConnectPaths_z(0);
        else          ConnectPaths_xy(0);

		// Schreibe Output
        if (m_single == false) WriteOutputDouble(anOutputFile,anOutputFile2,c1,c2,0,beam);
        else                   WriteOutputSingle(anOutputFile,c1,0,0,beam);

        UpdateParam();

        /*Zustellung Ende*
		
		
		/*Kurve Start*/

        CompPath(0); // Berechnung der Parameter für den Master
        
		if (m_single == false)
			CompPath(1); // Berechnung der Parameter für den Slave

		Gen_Path();  // Erzeugung der Outputvektoren

		// Schreibe Outputvektoren
        if (m_single == false) WriteOutputDouble(anOutputFile,anOutputFile2,c1,c2,0,beam);
        else                   WriteOutputSingle(anOutputFile,c1,0,0,beam);

        UpdateParam();
       
		if (m_single==false && (m_it1 != (m_BSplineTop.end()-1)))
            ++m_it2;

		/*Kurve Ende*/
    }

	//m_log.saveToFile("c:/Master-Path.iv");

	/*
	ofstream anOutputvelocity;
    anOutputvelocity.open("output_velocity.k");
    anOutputvelocity.precision(7);

	for(int i=0; i<(int)m_times_tmp.size(); ++i) // Schreibe absolute Geschwindigkeitswerte aus
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

/* Schreibt den Output für den feature- und spiral-basierten beidseitigen Fall */
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
// Schreibt alle Zeitwerte der Ausgabevektoren aus
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

/* Schreibt Output für den normalen einseitigen Fall */
bool path_simulate::WriteOutputSingle(ofstream &anOutputFile, int &c, bool brob, bool tool, bool beamfl)
{
    std::vector< std::vector<Base::Vector3d> > Out_val;
    std::vector<double> Out_time;
    std::pair<float,float> times;
    int n;
    int ind;

    int pid;

    if (brob == true)  // Schreibe Roboter-Output
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
        throw Base::RuntimeError("Outputlängen passen nicht zusammen");

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

/* Schreibt Output für den normalen beidseitigen Fall */
bool path_simulate::WriteOutputDouble(ofstream &anOutputFile, ofstream &anOutputFile2, int &c1, int &c2, bool brob, bool beamfl)
{
    std::pair<float,float> times;
    int pid1 = 2; // Master
	int pid2 = 3; // Slave
	int pid3 = 4; // Platte

    if (brob == false) // Simulations-Output (brob == true -> roboter-output)
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
            m_PathTimes_Master.push_back(times);       // fülle vektor für curve-times

            
			
			
			// SLAVE-X

            anOutputFile2 << "*BOUNDARY_PRESCRIBED_MOTION_RIGID" << std::endl;
            anOutputFile2 << "$#     pid       dof       vad      lcid        sf       vid     death     birth" << std::endl;
            anOutputFile2 << pid2 << ",1,0," << c2 <<  ",1.000000, ," << m_Output_time2[n2-1] << ","  << m_Output_time2[0] << std::endl;
			
			if (beamfl) // wenn auf true, dann füge neuen part mit ein
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
            
			if (beamfl) // wenn auf true, dann füge neuen part mit ein
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
    else  // Schreibe Roboter-Output
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
