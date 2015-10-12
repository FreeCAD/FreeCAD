/*****************************************************************************
 *  \author
 *  	Erwin Aertbelien, Div. PMA, Dep. of Mech. Eng., K.U.Leuven
 *
 *  \version
 *		LRL V0.2
 *
 *	\par History
 *		- $log$
 *
 *	\par Release
 *		$Id: trajectory_composite.h 22 2004-09-21 08:58:54Z eaertbellocal $
 *		$Name:  $
 ****************************************************************************/


#ifndef TRAJECTORY_COMPOSITE_H
#define TRAJECTORY_COMPOSITE_H

#include "trajectory.hpp"
#include "path_composite.hpp"
#include <vector>



namespace KDL {
  /**
   * Trajectory_Composite implements a trajectory that is composed
   * of underlying trajectoria.  Call Add to add a trajectory
   * @ingroup Motion
   */
class Trajectory_Composite: public Trajectory
	{
		typedef std::vector<Trajectory*> VectorTraj;
		typedef std::vector<double>         VectorDouble;
		VectorTraj vt;      // contains the element Trajectories
		VectorDouble  vd;      // contains end time for each Trajectory
		double duration;    // total duration of the composed
				    // Trajectory

	public:
		Trajectory_Composite();
		// Constructs an empty composite

		virtual double Duration() const;
		virtual Frame Pos(double time) const;
		virtual Twist Vel(double time) const;
		virtual Twist Acc(double time) const;

		virtual void Add(Trajectory* elem);
		// Adds trajectory <elem> to the end of the sequence.

		virtual void Destroy();
		virtual void Write(std::ostream& os) const;
		virtual Trajectory* Clone() const;

		virtual ~Trajectory_Composite();
	};


}

#endif
