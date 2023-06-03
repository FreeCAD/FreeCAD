#include<algorithm>

#include "Part.h"
#include "PartFrame.h"
#include "EulerConstraint.h"
#include "AbsConstraint.h"
#include "MarkerFrame.h"
#include "EulerParameters.h"
#include "EulerParametersDot.h"
#include "CREATE.h"

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
	aGabs = std::make_shared<std::vector<std::shared_ptr<AbsConstraint>>>();
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

void MbD::PartFrame::aGabsDo(const std::function<void(std::shared_ptr<AbsConstraint>)>& f)
{
	std::for_each(aGabs->begin(), aGabs->end(), f);
}

void MbD::PartFrame::markerFramesDo(const std::function<void(std::shared_ptr<MarkerFrame>)>& f)
{
	std::for_each(markerFrames->begin(), markerFrames->end(), f);
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

void PartFrame::asFixed()
{
	for (size_t i = 0; i < 6; i++) {
		auto con = std::make_shared<AbsConstraint>(i);
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
