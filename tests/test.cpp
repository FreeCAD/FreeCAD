#include "pch.h"
#include <CADSystem.h>
#include <ASMTAssembly.h>
#include <GESpMatParPvPrecise.h>
#include <MomentOfInertiaSolver.h>

using namespace MbD;

TEST(OndselSolver, TestName) {
	EXPECT_EQ(1, 1);
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runOndselSinglePendulum) {
	//testing::internal::CaptureStdout();
	auto cadSystem = std::make_shared<CADSystem>();
	cadSystem->runOndselSinglePendulum();
	//std::string output = testing::internal::GetCapturedStdout();
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runPreDragBackhoe1) {
	auto assembly = ASMTAssembly::assemblyFromFile(std::string(TEST_DATA_PATH) + "/runPreDragBackhoe1.asmt");
	assembly->runDraggingLog(std::string(TEST_DATA_PATH) + "/draggingBackhoe1.log");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runPreDragBackhoe2) {
	auto assembly = ASMTAssembly::assemblyFromFile(std::string(TEST_DATA_PATH) + "/runPreDragBackhoe2.asmt");
	assembly->runDraggingLog(std::string(TEST_DATA_PATH) + "/draggingBackhoe2.log");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runPreDragBackhoe3) {
	auto assembly = ASMTAssembly::assemblyFromFile(std::string(TEST_DATA_PATH) + "/runPreDragBackhoe3.asmt");
	assembly->runDraggingLog(std::string(TEST_DATA_PATH) + "/draggingBackhoe3.log");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, pistonAllowZRotation) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/pistonAllowZRotation.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, RevRevJt) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/RevRevJt.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, RevCylJt) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/RevCylJt.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, CylSphJt) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/CylSphJt.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, SphSphJt) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/SphSphJt.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, Gears) {
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/Gears.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, anglejoint) {
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/anglejoint.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, constvel) {
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/constvel.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, rackscrew) {
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/rackscrew.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, planarbug) {
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/planarbug.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, cirpendu2) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/cirpendu2.asmt");	//Under constrained. Testing ICKine.
	EXPECT_TRUE(true);
}
TEST(OndselSolver, quasikine) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/quasikine.asmt");	//Under constrained. Testing ICKine.
	EXPECT_TRUE(true);
}
TEST(OndselSolver, piston) {
	ASMTAssembly::readWriteFile(std::string(TEST_DATA_PATH) + "/piston.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runSinglePendulumSuperSimplified) {
	ASMTAssembly::runSinglePendulumSuperSimplified();	//Mass is missing
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runSinglePendulumSuperSimplified2) {
	ASMTAssembly::runSinglePendulumSuperSimplified2();	//DOF has infinite acceleration due to zero mass and inertias
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runSinglePendulumSimplified) {
	ASMTAssembly::runSinglePendulumSimplified();
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runSinglePendulum) {
	ASMTAssembly::runSinglePendulum();
	EXPECT_TRUE(true);
}
TEST(OndselSolver, piston2) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/piston.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, 00backhoe) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/00backhoe.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, circular) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/circular.asmt");	//Needs checking
	EXPECT_TRUE(true);
}
TEST(OndselSolver, engine1) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/engine1.asmt");	//Needs checking
	EXPECT_TRUE(true);
}
TEST(OndselSolver, fourbar) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/fourbar.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, fourbot) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/fourbot.asmt");	//Very large but works
	EXPECT_TRUE(true);
}
TEST(OndselSolver, wobpump) {
	ASMTAssembly::runFile(std::string(TEST_DATA_PATH) + "/wobpump.asmt");
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runOndselDoublePendulum) {
	auto cadSystem = std::make_shared<CADSystem>();
	cadSystem->runOndselDoublePendulum();
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runOndselPiston) {
	auto cadSystem = std::make_shared<CADSystem>();
	cadSystem->runOndselPiston();		//For debugging
	EXPECT_TRUE(true);
}
TEST(OndselSolver, runPiston) {
	auto cadSystem = std::make_shared<CADSystem>();
	cadSystem->runPiston();
	EXPECT_TRUE(true);
}
TEST(OndselSolver, GESpMatParPvPrecise) {
	GESpMatParPvPrecise::runSpMat();
	EXPECT_TRUE(true);
}
TEST(OndselSolver, MomentOfInertiaSolver) {
	MomentOfInertiaSolver::example1();
	EXPECT_TRUE(true);
}
TEST(OndselSolver, sharedptrTest) {
	auto assm = ASMTAssembly::With();

	std::shared_ptr<ASMTAssembly> assm1 = assm;	//New shared_ptr to old object. Reference count incremented.
	assert(assm == assm1);
	assert(assm.get() == assm1.get());
	assert(&assm != &assm1);
	assert(assm->constantGravity == assm1->constantGravity);
	assert(&(assm->constantGravity) == &(assm1->constantGravity));

	auto assm2 = std::make_shared<ASMTAssembly>(*assm);	//New shared_ptr to new object. Member variables copy old member variables
	assert(assm != assm2);
	assert(assm.get() != assm2.get());
	assert(&assm != &assm2);
	assert(assm->constantGravity == assm2->constantGravity);	//constantGravity is same object pointed to
	assert(&(assm->constantGravity) != &(assm2->constantGravity)); //Different shared_ptrs of same reference counter
	EXPECT_TRUE(true);
}

