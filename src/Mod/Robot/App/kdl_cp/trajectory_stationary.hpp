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
 *		$Id: trajectory_stationary.h 22 2004-09-21 08:58:54Z eaertbellocal $
 *		$Name:  $
 ****************************************************************************/

#ifndef TRAJECTORY_STATIONARY_H
#define TRAJECTORY_STATIONARY_H

#include "trajectory.hpp"


namespace KDL {
  /**
   * Implements a "trajectory" of a stationary position
   * for an amount of time.
   * @ingroup Motion
   */
	class Trajectory_Stationary : public Trajectory
	  {
		double duration;
		Frame pos;
	public:
		Trajectory_Stationary(double _duration,const Frame& _pos):
		  duration(_duration),pos(_pos) {}
		virtual double Duration() const {
			return duration;
		}
		virtual Frame Pos(double time) const {
			return pos;
		}
		virtual Twist Vel(double time) const {
			return Twist::Zero();
		}
		virtual Twist Acc(double time) const {
			return Twist::Zero();
		}
		virtual void Write(std::ostream& os) const;

		virtual Trajectory* Clone() const {
			return new Trajectory_Stationary(duration,pos);
		}
		virtual ~Trajectory_Stationary() {}
	};


}

#endif
