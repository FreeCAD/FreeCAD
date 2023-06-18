#pragma once
#include <memory>
#include <vector>
#include <functional>

#include "Item.h"
#include "EndFramec.h"

namespace MbD {
    class Constraint;

    class Joint : public Item
    {
        //frmI frmJ constraints friction 
    public:
        Joint();
        Joint(const char* str);
        void initialize();
        virtual void connectsItoJ(EndFrmcptr frmI, EndFrmcptr frmJ);
        void initializeLocally() override;
        void initializeGlobally() override;
        void constraintsDo(const std::function <void(std::shared_ptr<Constraint>)>& f);
        void postInput() override;
        void addConstraint(std::shared_ptr<Constraint> con);
        void prePosIC() override;
        void prePosKine() override;
        void fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints) override;
        virtual void fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints);
        virtual void fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints);
        void fillRedundantConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> redunConstraints) override;
        void fillConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> allConstraints) override;
        void fillqsulam(FColDsptr col) override;
        void fillqsudot(FColDsptr col) override;
        void fillqsudotWeights(std::shared_ptr<DiagonalMatrix<double>> diagMat) override;
        void useEquationNumbers() override;
        void setqsulam(FColDsptr col) override;
        void postPosICIteration() override;
        void fillPosICError(FColDsptr col) override;
        void fillPosICJacob(SpMatDsptr mat) override;
        void removeRedundantConstraints(std::shared_ptr<std::vector<int>> redundantEqnNos) override;
        void reactivateRedundantConstraints() override;
        void constraintsReport() override;
        void postPosIC() override;
        void outputStates() override;
        void preDyn() override;
        void fillPosKineError(FColDsptr col) override;
        void fillPosKineJacob(SpMatDsptr mat) override;
        void preVelIC() override;

        EndFrmcptr frmI;
        EndFrmcptr frmJ;
        std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> constraints;

    };
}

