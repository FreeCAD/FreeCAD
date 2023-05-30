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
        void fillEssenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> essenConstraints) override;
        virtual void fillDispConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> dispConstraints);
        virtual void fillPerpenConstraints(std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> perpenConstraints);

        EndFrmcptr frmI;
        EndFrmcptr frmJ;
        std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> constraints;

    };
}

