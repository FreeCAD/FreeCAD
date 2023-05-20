#include "TranslationConstraintIJ.h"

using namespace MbD;

std::shared_ptr<TranslationConstraintIJ> MbD::TranslationConstraintIJ::Create(EndFrmcptr frmi, EndFrmcptr frmj, int axisk)
{
    auto item = std::make_shared<TranslationConstraintIJ>(frmi, frmj, axisk);
    item->initialize();
    return item;
}

TranslationConstraintIJ::TranslationConstraintIJ(EndFrmcptr frmi, EndFrmcptr frmj, int axisk) :
    ConstraintIJ(frmi, frmj), axisK(axisk)
{
}

void TranslationConstraintIJ::initialize()
{
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
    riIeJeIe = std::make_shared<DispCompIecJecKec>();
}

void MbD::TranslationConstraintIJ::postInput()
{
}
