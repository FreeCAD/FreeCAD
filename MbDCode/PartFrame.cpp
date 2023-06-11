#include<algorithm>

#include "PartFrame.h"
#include "Part.h"
#include "EulerConstraint.h"
#include "AbsConstraint.h"
#include "MarkerFrame.h"
#include "EulerParameters.h"
#include "EulerParametersDot.h"
#include "CREATE.h"
#include "RedundantConstraint.h"

using namespace MbD;

PartFrame::PartFrame()
{
}
PartFrame::PartFrame(const char* str) : CartesianFrame(str)
{
}
void PartFrame::initialize()
{
	aGeu = CREATE<EulerConstraint>::With();
	aGeu->setOwner(this);
	aGabs = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	markerFrames = std::make_shared<std::vector<std::shared_ptr<MarkerFrame>>>();
}

void PartFrame::initializeLocally()
{
	markerFramesDo([](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->initializeLocally(); });
	aGeu->initializeLocally();
	aGabsDo([](std::shared_ptr<Constraint> aGab) { aGab->initializeLocally(); });
}

void PartFrame::initializeGlobally()
{
	markerFramesDo([](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->initializeGlobally(); });
	aGeu->initializeGlobally();
	aGabsDo([](std::shared_ptr<Constraint> aGab) { aGab->initializeGlobally(); });
}
void PartFrame::setqX(FColDsptr x) {
	qX->copyFrom(x);
}

FColDsptr PartFrame::getqX() {
	return qX;
}

void PartFrame::setqE(FColDsptr x) {
	qE->copyFrom(x);
}

FColDsptr PartFrame::getqE() {
	return qE;
}
void PartFrame::setqXdot(FColDsptr x) {
	//qXdot->copy(x);
}

FColDsptr PartFrame::getqXdot() {
	//return qXdot;
	return std::make_shared<FullColumn<double>>(3);
}

void PartFrame::setomeOpO(FColDsptr omeOpO) {
	//qEdot = EulerParametersDot<double>::FromqEOpAndOmegaOpO(qE, omeOpO);
}

FColDsptr PartFrame::getomeOpO() {
	return qE;
}

void PartFrame::setPart(Part* x) {
	part = x;
}

Part* PartFrame::getPart() {
	return part;
}

void PartFrame::addMarkerFrame(std::shared_ptr<MarkerFrame> markerFrame)
{
	markerFrame->setPartFrame(this);
	markerFrames->push_back(markerFrame);
}

EndFrmcptr PartFrame::endFrame(std::string name)
{
	auto match = std::find_if(markerFrames->begin(), markerFrames->end(), [&](auto& mkr) {return mkr->getName() == name; });
	return (*match)->endFrames->at(0);
}

void MbD::PartFrame::aGabsDo(const std::function<void(std::shared_ptr<Constraint>)>& f)
{
	std::for_each(aGabs->begin(), aGabs->end(), f);
}

void MbD::PartFrame::markerFramesDo(const std::function<void(std::shared_ptr<MarkerFrame>)>& f)
{
	std::for_each(markerFrames->begin(), markerFrames->end(), f);
}

void MbD::PartFrame::removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos)
{
	if (std::find(redundantEqnNos->begin(), redundantEqnNos->end(), aGeu->iG) != redundantEqnNos->end()) {
		auto redunCon = CREATE<RedundantConstraint>::With();
		redunCon->constraint = aGeu;
		aGeu = redunCon;
	}
	for (size_t i = 0; i < aGabs->size(); i++)
	{
		auto& constraint = aGabs->at(i);
		if (std::find(redundantEqnNos->begin(), redundantEqnNos->end(), constraint->iG) != redundantEqnNos->end()) {
			auto redunCon = CREATE<RedundantConstraint>::With();
			redunCon->constraint = constraint;
			aGabs->at(i) = redunCon;
		}
	}
}

void MbD::PartFrame::reactivateRedundantConstraints()
{
	if (aGeu->isRedundant()) aGeu = std::dynamic_pointer_cast<RedundantConstraint>(aGeu)->constraint;
	for (size_t i = 0; i < aGabs->size(); i++)
	{
		auto& con = aGabs->at(i);
		if (con->isRedundant()) {
			aGabs->at(i) = std::static_pointer_cast<RedundantConstraint>(con)->constraint;
		}
	}
}

void MbD::PartFrame::constraintsReport()
{
	auto redunCons = std::make_shared<std::vector<std::shared_ptr<Constraint>>>();
	aGabsDo([&](std::shared_ptr<Constraint> con) {
		if (con->isRedundant()) {
			redunCons->push_back(con);
		}
		});
	if (aGeu->isRedundant()) redunCons->push_back(aGeu);
	if (redunCons->size() > 0) {
		std::string str = "MbD: " + part->classname() + std::string(" ") + part->getName() + " has the following constraint(s) removed: ";
		this->logString(str);
		std::for_each(redunCons->begin(), redunCons->end(), [&](auto& con) {
			str = "MbD: " + std::string("    ") + std::string(typeid(*con).name());
			this->logString(str);
			});
	}
}

void MbD::PartFrame::prePosIC()
{
	iqX = -1;
	iqE = -1;
	Item::prePosIC();
	markerFramesDo([](std::shared_ptr<MarkerFrame> markerFrm) { markerFrm->prePosIC(); });
	aGeu->prePosIC();
	aGabsDo([](std::shared_ptr<Constraint> aGab) { aGab->prePosIC(); });
}

FColDsptr MbD::PartFrame::rOpO()
{
	return qX;
}

FMatDsptr MbD::PartFrame::aAOp()
{
	return qE->aA;
}

FColFMatDsptr MbD::PartFrame::pAOppE()
{
	return qE->pApE;
}

void MbD::PartFrame::fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints)
{
	aGeu->fillEssenConstraints(aGeu, essenConstraints);
	aGabsDo([&](std::shared_ptr<Constraint> con) { con->fillEssenConstraints(con, essenConstraints); });
}

void MbD::PartFrame::fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints)
{
}

void MbD::PartFrame::fillqsu(FColDsptr col)
{
	col->atiputFullColumn(iqX, qX);
	col->atiputFullColumn(iqE, qE);
	markerFramesDo([&](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->fillqsu(col); });
}

void MbD::PartFrame::fillqsuWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat)
{
	markerFramesDo([&](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->fillqsuWeights(diagMat); });
}

void MbD::PartFrame::fillqsulam(FColDsptr col)
{
	col->atiputFullColumn(iqX, qX);
	col->atiputFullColumn(iqE, qE);
	markerFramesDo([&](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->fillqsulam(col); });
	aGeu->fillqsulam(col);
	aGabsDo([&](std::shared_ptr<Constraint> con) { con->fillqsulam(col); });
}

void MbD::PartFrame::useEquationNumbers()
{
	markerFramesDo([](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->useEquationNumbers(); });
	aGeu->useEquationNumbers();
	aGabsDo([](std::shared_ptr<Constraint> con) { con->useEquationNumbers(); });
}

void MbD::PartFrame::setqsu(FColDsptr col)
{
	qX->equalFullColumnAt(col, iqX);
	qE->equalFullColumnAt(col, iqE);
	markerFramesDo([&](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->setqsu(col); });
	aGeu->setqsu(col);
	aGabsDo([&](std::shared_ptr<Constraint> con) { con->setqsu(col); });
}

void MbD::PartFrame::setqsulam(FColDsptr col)
{
	qX->equalFullColumnAt(col, iqX);
	qE->equalFullColumnAt(col, iqE);
	markerFramesDo([&](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->setqsulam(col); });
	aGeu->setqsulam(col);
	aGabsDo([&](std::shared_ptr<Constraint> con) { con->setqsulam(col); });
}

void MbD::PartFrame::postPosICIteration()
{
	Item::postPosICIteration();
	markerFramesDo([](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->postPosICIteration(); });
	aGeu->postPosICIteration();
	aGabsDo([](std::shared_ptr<Constraint> con) { con->postPosICIteration(); });
}

void MbD::PartFrame::fillPosICError(FColDsptr col)
{
	markerFramesDo([&](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->fillPosICError(col); });
	aGeu->fillPosICError(col);
	aGabsDo([&](std::shared_ptr<Constraint> con) { con->fillPosICError(col); });
}

void MbD::PartFrame::fillPosICJacob(SpMatDsptr mat)
{
	markerFramesDo([&](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->fillPosICJacob(mat); });
	aGeu->fillPosICJacob(mat);
	aGabsDo([&](std::shared_ptr<Constraint> con) { con->fillPosICJacob(mat); });
}

void MbD::PartFrame::postPosIC()
{
	markerFramesDo([](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->postPosIC(); });
	aGeu->postPosIC();
	aGabsDo([](std::shared_ptr<Constraint> con) { con->postPosIC(); });
}

void MbD::PartFrame::outputStates()
{
	std::stringstream ss;
	ss << "qX = ";
	qX->printOn(ss);
	ss << std::endl;
	ss << "qE = ";
	qE->printOn(ss);
	auto str = ss.str();
	this->logString(str);
	aGeu->outputStates();
	aGabsDo([](std::shared_ptr<Constraint> con) { con->outputStates(); });
}

void MbD::PartFrame::preDyn()
{
	markerFramesDo([](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->preDyn(); });
	aGeu->preDyn();
	aGabsDo([](std::shared_ptr<Constraint> aGab) { aGab->preDyn(); });
}

void PartFrame::asFixed()
{
	for (int i = 0; i < 6; i++) {		
		auto con = CREATE<AbsConstraint>::With(i);
		con->setOwner(this);
		aGabs->push_back(con);
	}
}

void MbD::PartFrame::postInput()
{
	//qXddot = std::make_shared<FullColumn<double>>(3, 0.0);
	//qEddot = std::make_shared<FullColumn<double>>(4, 0.0);
	Item::postInput();
	markerFramesDo([](std::shared_ptr<MarkerFrame> markerFrame) { markerFrame->postInput(); });
	aGeu->postInput();
	aGabsDo([](std::shared_ptr<Constraint> aGab) { aGab->postInput(); });
}

void MbD::PartFrame::calcPostDynCorrectorIteration()
{
	qE->calcABC();
	qE->calcpApE();
	//qEdot->calcAdotBdotCdot();
	//qEdot->calcpAdotpE();
}
