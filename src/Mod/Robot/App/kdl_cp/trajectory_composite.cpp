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
 *		$Id: trajectory_composite.cpp 22 2004-09-21 08:58:54Z eaertbellocal $
 *		$Name:  $
 ****************************************************************************/

#include "trajectory_composite.hpp"
#include "path_composite.hpp"

namespace KDL {

    using namespace std;


    Trajectory_Composite::Trajectory_Composite():duration(0.0)
    {
    }

    double Trajectory_Composite::Duration() const{
        return duration;
    }

    Frame Trajectory_Composite::Pos(double time) const {
        // not optimal, could be done in log(#elem)
        // or one could buffer the last segment and start looking from there.
        unsigned int i;
        double previoustime;
        Trajectory* traj;
        if (time < 0) {
            return vt[0]->Pos(0);
        }
        previoustime = 0;
        for (i=0;i<vt.size();i++) {
            if (time < vd[i]) {
                return vt[i]->Pos(time-previoustime);
            }
            previoustime = vd[i];
        }
        traj = vt[vt.size()-1];
        return traj->Pos(traj->Duration());
    }


    Twist Trajectory_Composite::Vel(double time) const {
        // not optimal, could be done in log(#elem)
        unsigned int i;
        Trajectory* traj;
        double previoustime;
        if (time < 0) {
            return vt[0]->Vel(0);
        }
        previoustime = 0;
        for (i=0;i<vt.size();i++) {
            if (time < vd[i]) {
                return vt[i]->Vel(time-previoustime);
            }
            previoustime = vd[i];
        }
        traj = vt[vt.size()-1];
        return traj->Vel(traj->Duration());
    }

    Twist Trajectory_Composite::Acc(double time) const {
        // not optimal, could be done in log(#elem)
    	unsigned int i;
        Trajectory* traj;
        double previoustime;
        if (time < 0) {
            return vt[0]->Acc(0);
        }
        previoustime = 0;
        for (i=0;i<vt.size();i++) {
		if (time < vd[i]) {
			return vt[i]->Acc(time-previoustime);
		}
		previoustime = vd[i];
        }
        traj = vt[vt.size()-1];
        return traj->Acc(traj->Duration());
    }

    void Trajectory_Composite::Add(Trajectory* elem) {
        vt.insert(vt.end(),elem);
        duration += elem->Duration();
        vd.insert(vd.end(),duration);
    }

    void Trajectory_Composite::Destroy() {
        VectorTraj::iterator it;
        for (it=vt.begin();it!=vt.end();it++) {
            delete *it;
        }
        vt.erase(vt.begin(),vt.end());
        vd.erase(vd.begin(),vd.end());
    }

    Trajectory_Composite::~Trajectory_Composite() {
        Destroy();
    }


    void Trajectory_Composite::Write(ostream& os) const {
        os << "COMPOSITE[ " << vt.size() << endl;
        unsigned int i;
        for (i=0;i<vt.size();i++) {
            vt[i]->Write(os);
        }
        os << "]" << endl;
    }

    Trajectory* Trajectory_Composite::Clone() const{
        Trajectory_Composite* comp = new Trajectory_Composite();
        for (unsigned int i = 0; i < vt.size(); ++i) {
            comp->Add(vt[i]->Clone());
        }
        return comp;
    }

}


