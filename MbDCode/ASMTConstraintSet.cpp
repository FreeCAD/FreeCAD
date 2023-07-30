#include "ASMTConstraintSet.h"
#include "ASMTAssembly.h"
#include "ASMTMarker.h"

using namespace MbD;

void MbD::ASMTConstraintSet::createMbD(std::shared_ptr<System> mbdSys, std::shared_ptr<Units> mbdUnits)
{
	//self dataSeries : OrderedCollection new.
	//self discontinuities : OrderedCollection new.
	auto mbdJt = this->mbdClassNew();
	mbdObject = mbdJt;
	mbdJt->name = fullName("");
	auto mrkI = std::static_pointer_cast<EndFramec>(root()->markerAt(markerI)->mbdObject);
	auto mrkJ = std::static_pointer_cast<EndFramec>(root()->markerAt(markerJ)->mbdObject);
	mbdJt->connectsItoJ(mrkI, mrkJ);
	mbdSys->addJoint(mbdJt);
}

std::shared_ptr<Joint> MbD::ASMTConstraintSet::mbdClassNew()
{
	assert(false);
	return std::shared_ptr<Joint>();
}

void MbD::ASMTConstraintSet::updateFromMbD()
{
	//"
	//MbD returns aFIeO and aTIeO.
	//GEO needs aFImO and aTImO.
	//For Motion rImIeO is not zero and is changing.
	//aFImO = aFIeO.
	//aTImO = aTIeO + (rImIeO cross : aFIeO).
	//"
	auto mbdUnts = mbdUnits();
	auto mbdJoint = std::static_pointer_cast<Joint>(mbdObject);
	auto aFIeO = mbdJoint->aFX()->times(mbdUnts->force);
	auto aTIeO = mbdJoint->aTX()->times(mbdUnts->torque);
	auto rImIeO = mbdJoint->frmI->rmeO()->times(mbdUnts->length);
	auto aFIO = aFIeO;
	auto aTIO = aTIeO->plusFullColumn(rImIeO->cross(aFIeO));
	fxs->push_back(aFIO->at(0));
	fys->push_back(aFIO->at(1));
	fzs->push_back(aFIO->at(2));
	txs->push_back(aTIO->at(0));
	tys->push_back(aTIO->at(1));
	tzs->push_back(aTIO->at(2));
}

void MbD::ASMTConstraintSet::compareResults(AnalysisType type)
{
	auto mbdUnts = mbdUnits();
	auto factor = 1.0e-6;
	auto forceTol = mbdUnts->force * factor;
	auto torqueTol = mbdUnts->torque * factor;
	auto i = fxs->size() - 1;
	//assert(Numeric::equaltol(fxs->at(i), infxs->at(i), forceTol));
	//assert(Numeric::equaltol(fys->at(i), infys->at(i), forceTol));
	//assert(Numeric::equaltol(fzs->at(i), infzs->at(i), forceTol));
	//assert(Numeric::equaltol(txs->at(i), intxs->at(i), torqueTol));
	//assert(Numeric::equaltol(tys->at(i), intys->at(i), torqueTol));
	//assert(Numeric::equaltol(tzs->at(i), intzs->at(i), torqueTol));
}
