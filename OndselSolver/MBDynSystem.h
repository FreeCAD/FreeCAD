/***************************************************************************
 *   Copyright (c) 2023 Ondsel, Inc.                                       *
 *                                                                         *
 *   This file is part of OndselSolver.                                    *
 *                                                                         *
 *   See LICENSE file for details about copyright.                         *
 ***************************************************************************/

#pragma once
#include <memory>
#include <string>

#include "MBDynItem.h"

namespace MbD {
    class MBDynData;
    class MBDynInitialValue;
    class MBDynControlData;
    class MBDynNode;
    class MBDynElement;
    class MBDynLabel;
    class MBDynReference;
    class MBDynDrive;
    class MBDynGravity;

    class MBDynSystem : public MBDynItem
    {
    public:
        static void runFile(const char* chars);
        static void eraseComments(std::vector<std::string>& lines);
        static std::vector<std::string> collectStatements(std::vector<std::string>& lines);
        void initialize() override;
        void parseMBDyn(std::vector<std::string>& lines) override;
        void parseMBDynData(std::vector<std::string>& lines);
        void parseMBDynNodes(std::vector<std::string>& lines);
        void parseMBDynElements(std::vector<std::string>& lines);
        void parseMBDynVariables(std::vector<std::string>& lines);
        void parseMBDynLabels(std::vector<std::string>& lines);
        void parseMBDynReferences(std::vector<std::string>& lines);
        std::shared_ptr<std::vector<std::shared_ptr<MBDynNode>>> mbdynNodes() override;
        std::shared_ptr<std::vector<std::shared_ptr<MBDynBody>>> mbdynBodies() override;
        std::shared_ptr<std::vector<std::shared_ptr<MBDynJoint>>> mbdynJoints() override;
        std::shared_ptr<std::vector<std::shared_ptr<MBDynDrive>>> mbdynDrives() override;
        std::shared_ptr<std::map<std::string, Symsptr>> mbdynVariables() override;
        std::shared_ptr<std::map<std::string, std::shared_ptr<MBDynReference>>> mbdynReferences() override;
        void createASMT() override;
        std::shared_ptr<MBDynNode> nodeAt(std::string nodeName) override;
        int nodeidAt(std::string nodeName) override;
        std::shared_ptr<MBDynBody> bodyWithNode(std::string nodeName) override;
        std::shared_ptr<ASMTAssembly> asmtAssembly() override;
        std::vector<std::string> nodeNames() override;

        void runKINEMATIC();
        void outputFiles();
        void setFilename(std::string filename);
        void readDataBlock(std::vector<std::string>& lines);
        void readInitialValueBlock(std::vector<std::string>& lines);
        void readControlDataBlock(std::vector<std::string>& lines);
        void readLabels(std::vector<std::string>& lines);
        void readVariables(std::vector<std::string>& lines);
        void readReferences(std::vector<std::string>& lines);
        void readNodesBlock(std::vector<std::string>& lines);
        void readElementsBlock(std::vector<std::string>& lines);

        std::string filename = "";
        std::string data;
        std::shared_ptr<MBDynInitialValue> initialValue;
        std::shared_ptr<MBDynControlData> controlData;
        std::shared_ptr<std::vector<std::shared_ptr<MBDynNode>>> nodes;
        std::shared_ptr<std::vector<std::shared_ptr<MBDynBody>>> bodies;
        std::shared_ptr<std::vector<std::shared_ptr<MBDynJoint>>> joints;
        std::shared_ptr<std::vector<std::shared_ptr<MBDynDrive>>> drives;
        std::shared_ptr<MBDynGravity> gravity;
        std::shared_ptr<std::map<std::string, Symsptr>> variables;
        std::shared_ptr<std::map<std::string, int>> labels;
        std::shared_ptr<std::map<std::string, std::shared_ptr<MBDynReference>>> references;

    };
}
