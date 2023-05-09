#include "KinematicIeJe.h"

using namespace MbD;

MbD::KinematicIeJe::KinematicIeJe()
{
}

MbD::KinematicIeJe::KinematicIeJe(std::shared_ptr<EndFramec> frmi, std::shared_ptr<EndFramec> frmj) : frmI(frmi), frmJ(frmj)
{
}
