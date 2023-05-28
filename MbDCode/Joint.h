#pragma once
#include <memory>
#include <vector>

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
        void postInput() override;
        void addConstraint(std::shared_ptr<Constraint> con);

        EndFrmcptr frmI;
        EndFrmcptr frmJ;
        std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> constraints;

    };
}

