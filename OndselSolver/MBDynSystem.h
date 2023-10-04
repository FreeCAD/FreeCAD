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
    class MBDynNodes;
    class MBDynElements;
    class MBDynVariables;
    class MBDynLabels;
    class MBDynReferences;

    class MBDynSystem : public MBDynItem
    {
    public:
        static void runFile(const char* chars);
        static void eraseComments(std::vector<std::string>& lines);
        static std::vector<std::string> collectStatements(std::vector<std::string>& lines);
        void initialize() override;
        void parseMBDyn(std::vector<std::string>& lines) override;
        std::shared_ptr<MBDynVariables> mbdynVariables() override;

        void runKINEMATIC();
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
        std::shared_ptr<MBDynData> dataBlk;
        std::shared_ptr<MBDynInitialValue> initialValueBlk;
        std::shared_ptr<MBDynControlData> controlDataBlk;
        std::shared_ptr<MBDynNodes> nodesBlk;
        std::shared_ptr<MBDynElements> elementsBlk;
        std::shared_ptr<MBDynVariables> variables;
        std::shared_ptr<MBDynLabels> labels;
        std::shared_ptr<MBDynReferences> references;

    };
}
