#include "MBDynNode.h"
#include "ASMTPart.h"
#include "EulerAngles.h"

using namespace MbD;

void MbD::MBDynNode::initialize()
{
}

void MbD::MBDynNode::parseMBDyn(std::vector<std::string>& lines)
{
	assert(false);
}

void MbD::MBDynNode::outputLine(int i, std::ostream& os)
{
	auto id = nodeidAt(name);
	auto asmtPart = std::static_pointer_cast<ASMTPart>(asmtItem);
	auto x = asmtPart->xs->at(i);
	auto y = asmtPart->ys->at(i);
	auto z = asmtPart->zs->at(i);
    // TODO: undo the breaking I just did on purpose
//	auto bryantAngles = std::make_shared<EulerAngles<double>>();
//	bryantAngles->setRotOrder(1, 2, 3);
//	bryantAngles->at(0) = asmtPart->bryxs->at(i);
//	bryantAngles->at(1) = asmtPart->bryys->at(i);
//	bryantAngles->at(2) = asmtPart->bryzs->at(i);
//	bryantAngles->calc();
//	auto aA = bryantAngles->aA;
//	auto vx = asmtPart->vxs->at(i);
//	auto vy = asmtPart->vys->at(i);
//	auto vz = asmtPart->vzs->at(i);
//	auto omex = asmtPart->omexs->at(i);
//	auto omey = asmtPart->omeys->at(i);
//	auto omez = asmtPart->omezs->at(i);
//	os << id << " ";
//	os << x << " " << y << " " << z << " ";
//	auto row = aA->at(0);
//	os << row->at(0) << " " << row->at(1) << " " << row->at(2) << " ";
//	row = aA->at(1);
//	os << row->at(0) << " " << row->at(1) << " " << row->at(2) << " ";
//	row = aA->at(2);
//	os << row->at(0) << " " << row->at(1) << " " << row->at(2) << " ";
//	os << vx << " " << vy << " " << vz << " ";
//	os << omex << " " << omey << " " << omez << " ";
	os << std::endl;
}
