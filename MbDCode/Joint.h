#pragma once
#include <memory>
#include <vector>

#include "Item.h"
#include "EndFramec.h"
#include "Constraint.h"

namespace MbD {
    class EndFramec;
    class Constraint;

    class Joint : public Item
    {
        //frmI frmJ constraints friction 
    public:
        Joint();
        Joint(const char* str);
        void initialize();
        virtual void connectsItoJ(std::shared_ptr<EndFramec> frmI, std::shared_ptr<EndFramec> frmJ);
        void initializeLocally();
        void initializeGlobally();

        std::shared_ptr<EndFramec> frmI;
        std::shared_ptr<EndFramec> frmJ;
        std::unique_ptr<std::vector<std::shared_ptr<Constraint>>> constraints;

    };
}

