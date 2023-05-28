#include "AtPointConstraintIJ.h"
#include "DispCompIecJecO.h"

using namespace MbD;

std::shared_ptr<AtPointConstraintIJ> MbD::AtPointConstraintIJ::Create(EndFrmcptr frmi, EndFrmcptr frmj, int axisi)
{
	auto item = std::make_shared<AtPointConstraintIJ>(frmi, frmj, axisi);
	item->initialize();
	return item;
}

AtPointConstraintIJ::AtPointConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisi) :
	ConstraintIJ(frmi, frmj), axis(axisi)
{
}

void AtPointConstraintIJ::initialize()
{
}

void MbD::AtPointConstraintIJ::initializeLocally()
{
	riIeJeO->initializeLocally();
}

void MbD::AtPointConstraintIJ::initializeGlobally()
{
	riIeJeO->initializeGlobally();
}

void AtPointConstraintIJ::initriIeJeO()
{
	riIeJeO = std::make_shared<DispCompIecJecO>();
}

void MbD::AtPointConstraintIJ::postInput()
{
}

void MbD::AtPointConstraintIJ::calcPostDynCorrectorIteration()
{
}
