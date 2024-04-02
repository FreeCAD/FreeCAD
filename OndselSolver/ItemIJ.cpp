#include "ItemIJ.h"

using namespace MbD;

MbD::ItemIJ::ItemIJ()
{
}

MbD::ItemIJ::ItemIJ(const char* str) : Item(str)
{
}

MbD::ItemIJ::ItemIJ(EndFrmsptr frmi, EndFrmsptr frmj) : frmI(frmi), frmJ(frmj)
{
}

void MbD::ItemIJ::connectsItoJ(EndFrmsptr frmi, EndFrmsptr frmj)
{
	frmI = frmi;
	frmJ = frmj;
}
