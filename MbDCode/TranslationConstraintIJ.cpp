#include "TranslationConstraintIJ.h"
#include "CREATE.h"

using namespace MbD;

TranslationConstraintIJ::TranslationConstraintIJ(EndFrmsptr frmi, EndFrmsptr frmj, int axisi) :
    ConstraintIJ(frmi, frmj), axisI(axisi)
{
}

void TranslationConstraintIJ::initialize()
{
    ConstraintIJ::initialize();
    initriIeJeIe();
}

void TranslationConstraintIJ::initializeLocally()
{
    riIeJeIe->initializeLocally();
}

void TranslationConstraintIJ::initializeGlobally()
{
    riIeJeIe->initializeGlobally();
}

void TranslationConstraintIJ::initriIeJeIe()
{
    riIeJeIe = CREATE<DispCompIecJecKec>::With(frmI, frmJ, frmI, axisI);
}

void TranslationConstraintIJ::postInput()
{
    riIeJeIe->postInput();
    Constraint::postInput();
}

void TranslationConstraintIJ::calcPostDynCorrectorIteration()
{
    aG = riIeJeIe->value() - aConstant;
}

void TranslationConstraintIJ::prePosIC()
{
    riIeJeIe->prePosIC();
    Constraint::prePosIC();
}

ConstraintType TranslationConstraintIJ::type()
{
    return displacement;
}

void TranslationConstraintIJ::postPosICIteration()
{
    riIeJeIe->postPosICIteration();
    Item::postPosICIteration();
}

void TranslationConstraintIJ::preVelIC()
{
    riIeJeIe->preVelIC();
    Item::preVelIC();
}

void MbD::TranslationConstraintIJ::simUpdateAll()
{
    riIeJeIe->simUpdateAll();
    Constraint::simUpdateAll();
}

void TranslationConstraintIJ::preAccIC()
{
    riIeJeIe->preAccIC();
    Constraint::preAccIC();
}
