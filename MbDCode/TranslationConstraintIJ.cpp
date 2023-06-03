#include "TranslationConstraintIJ.h"
#include "CREATE.h"

using namespace MbD;

TranslationConstraintIJ::TranslationConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, size_t axisi) :
    ConstraintIJ(frmi, frmj), axisI(axisi)
{
}

void TranslationConstraintIJ::initialize()
{
    ConstraintIJ::initialize();
    initriIeJeIe();
}

void MbD::TranslationConstraintIJ::initializeLocally()
{
    riIeJeIe->initializeLocally();
}

void MbD::TranslationConstraintIJ::initializeGlobally()
{
    riIeJeIe->initializeGlobally();
}

void MbD::TranslationConstraintIJ::initriIeJeIe()
{
    riIeJeIe = CREATE<DispCompIecJecKec>::With(frmI, frmJ, frmI, axisI);
}

void MbD::TranslationConstraintIJ::postInput()
{
    riIeJeIe->postInput();
    Constraint::postInput();
}

void MbD::TranslationConstraintIJ::calcPostDynCorrectorIteration()
{
    aG = riIeJeIe->value() - aConstant;
}

void MbD::TranslationConstraintIJ::prePosIC()
{
    riIeJeIe->prePosIC();
    Constraint::prePosIC();
}

MbD::ConstraintType MbD::TranslationConstraintIJ::type()
{
    return MbD::displacement;
}

void MbD::TranslationConstraintIJ::postPosICIteration()
{
    riIeJeIe->postPosICIteration();
    Item::postPosICIteration();
}
