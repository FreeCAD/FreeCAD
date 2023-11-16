#include "MBDynMarker.h"
#include "MBDynStructural.h"
#include "ASMTMarker.h"
#include "ASMTPart.h"
#include "ASMTAssembly.h"
#include "MBDynBody.h"
#include "ASMTSpatialContainer.h"

using namespace MbD;

void MbD::MBDynMarker::parseMBDyn(std::vector<std::string>& args)
{
	rPmP = std::make_shared<FullColumn<double>>(3);
	aAPm = FullMatrixDouble::identitysptr(3);
	if (args.empty()) return;
	auto str = args.at(0); //Must copy string
	if (str.find("reference") != std::string::npos) {
		auto strucNode = std::static_pointer_cast<MBDynStructural>(nodeAt(nodeStr));
		auto rOPO = strucNode->rOfO;
		auto aAOP = strucNode->aAOf;
		auto rOmO = readPosition(args);
		auto aAOm = readOrientation(args);
		rPmP = aAOP->transposeTimesFullColumn(rOmO->minusFullColumn(rOPO));
		aAPm = aAOP->transposeTimesFullMatrix(aAOm);
	}
	else if (str.find("offset") != std::string::npos) {
		rPmP = readPosition(args);
	}
	else {
		rPmP = readPosition(args);
		aAPm = readOrientation(args);
	}
}

void MbD::MBDynMarker::parseMBDynClamp(std::vector<std::string>& args)
{
	//rOmO = rOPO + aAOP*rPmP
	//aAOm = aAOP * aAPm
	auto rOmO = std::make_shared<FullColumn<double>>(3);
	auto aAOm = FullMatrixDouble::identitysptr(3);
	auto rOPO = readPosition(args);
	auto aAOP = readOrientation(args);
	rPmP = aAOP->transposeTimesFullColumn(rOmO->minusFullColumn(rOPO));
	aAPm = aAOP->transposeTimesFullMatrix(aAOm);
}

void MbD::MBDynMarker::createASMT()
{
	auto asmtAsm = asmtAssembly();
	if (nodeStr == "Assembly") {
		auto mkr = std::make_shared<ASMTMarker>();
		asmtItem = mkr;
		mkr->setName(asmtAsm->generateUniqueMarkerName());
		mkr->setPosition3D(rPmP);
		mkr->setRotationMatrix(aAPm);
		asmtAsm->addMarker(mkr);
	}
	else {
		auto asmtPart = asmtAsm->partPartialNamed(nodeStr);
		auto mkr = std::make_shared<ASMTMarker>();
		asmtItem = mkr;
		mkr->setName(asmtPart->generateUniqueMarkerName());
		mkr->setPosition3D(rPmP);
		mkr->setRotationMatrix(aAPm);
		asmtPart->addMarker(mkr);
	}
}
