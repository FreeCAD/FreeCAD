#pragma once
#include <memory>
#include <vector>

//#include "typedef.h"
#include "Item.h"
#include "EndFramec.h"
#include "Constraint.h"

namespace MbD {
    //class EndFramec;
    class Constraint;
    //using EndFrmcptr = std::shared_ptr<EndFramec>;

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

        EndFrmcptr frmI;
        EndFrmcptr frmJ;
        std::shared_ptr<std::vector<std::shared_ptr<Constraint>>> constraints;

    };
}

