#pragma once
#include <memory>

#include "Item.h"

namespace MbD {
    class Constraint : public Item
    {
    public:
        Constraint() : Item() {
            iG = -1;
            aG = 0.0;
            lam = 0.0;
        }
        void setOwner(std::shared_ptr<Item> x) {
            owner = x;
        }
        std::shared_ptr<Item> getOwner() {
            return owner.lock();
        }
        //iG aG lam mu lamDeriv owner 
        int iG;
        double aG;  //Constraint function
        double lam; //Lambda is Lagrange Multiplier
        std::weak_ptr<Item> owner; //A Joint or PartFrame owns the constraint
    };
}

