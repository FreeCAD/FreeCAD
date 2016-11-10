/***************************************************************************
  tag: Erwin Aertbelien  Mon Jan 10 16:38:39 CET 2005  path_line.h

                        path_line.h -  description
                           -------------------
    begin                : Mon January 10 2005
    copyright            : (C) 2005 Erwin Aertbelien
    email                : erwin.aertbelien@mech.kuleuven.ac.be

 ***************************************************************************
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Lesser General Public            *
 *   License as published by the Free Software Foundation; either          *
 *   version 2.1 of the License, or (at your option) any later version.    *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 59 Temple Place,                                    *
 *   Suite 330, Boston, MA  02111-1307  USA                                *
 *                                                                         *
 ***************************************************************************/


/*****************************************************************************
*   ALTERNATIVE FOR trajectory_line.h/cpp
 *  \author
 *  	Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version
 *		ORO_Geometry V0.2
 *
 *	\par History
 *		- $log$
 *
 *	\par Release
 *		$Id: path_line.h,v 1.1.1.1.2.3 2003/07/24 13:26:15 psoetens Exp $
 *		$Name:  $
 ****************************************************************************/

#ifndef KDL_MOTION_PATH_LINE_H
#define KDL_MOTION_PATH_LINE_H

#include "path.hpp"
#include "rotational_interpolation.hpp"


namespace KDL {


	/**
	 * A path representing a line from A to B.
	 * @ingroup Motion
	 */
class Path_Line : public Path
	{
		// Orientatie gedeelte
		RotationalInterpolation* orient;

		// Lineair gedeelte
		Vector V_base_start;
		Vector V_base_end;
		Vector V_start_end;

		double eqradius;  // equivalent radius

		// verdeling baanlengte over pos/rot
		double pathlength;
		double scalelin;
		double scalerot;

		bool aggregate;
	public:
		/**
		 * Constructs a Line Path
		 * F_base_start and F_base_end give the begin and end frame wrt the base
		 * orient gives the method of rotation interpolation
		 * eqradius : equivalent radius :
		 *		serves to compare rotations and translations.
		 *		the "amount of motion"(pos,vel,acc) of the rotation is taken
		 *      to be the amount motion of a point at distance eqradius from the
		 *      rotation axis.
         *
         * Eqradius is introduced because it is unavoidable that you have to compare rotations and translations :
         * e.g. : You can have motions that only contain rotation, and motions that only contain translations.
         * The motion planning goes as follows :
         *  - translation is planned with the given parameters
         *  - rotation is planned planned with the parameters calculated with eqradius.
         *  - The longest of the previous two remains unchanged,
         *    the shortest in duration is scaled to take as long as the longest.
         * This guarantees that the geometric path in 6D space remains independent of the motion profile parameters.
         *
         * RotationalInterpolation_SingleAxis() has the advantage that it is independent
         * of the frame in which you express your path.
         * Other implementations for RotationalInterpolations COULD be
         *    (not implemented) (yet) :
         *    1) quaternion interpolation : but this is more difficult for the human to interprete
         *    2) 3-axis interpolation : express the orientation of the frame in e.g.
         *       euler zyx angles alfa,beta, gamma  and interpolate these parameters.
         *       But this is dependent of the frame you choose as a reference and
         *       their can occur representation singularities.
		 */
		Path_Line(const Frame& F_base_start,
			const Frame& F_base_end,
			RotationalInterpolation* orient,
			double eqradius,
			bool _aggregate=true);
		Path_Line(const Frame& F_base_start,
			const Twist& twist_in_base,
			RotationalInterpolation* orient,
			double eqradius,
			bool _aggregate=true);
		double LengthToS(double length);
		virtual double PathLength();
		virtual Frame Pos(double s) const;
		virtual Twist Vel(double s,double sd) const ;
		virtual Twist Acc(double s,double sd,double sdd) const;
		virtual void Write(std::ostream& os);
		virtual Path* Clone();

		/**
		 * gets an identifier indicating the type of this Path object
		 */
		virtual IdentifierType getIdentifier() const {
			return ID_LINE;
		}
		virtual ~Path_Line();
	};


}


#endif
