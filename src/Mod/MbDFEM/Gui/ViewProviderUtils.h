// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/MbDFEM/MbDFEMGlobal.h>

class SoDetail;
class SoFullPath;

namespace App
{
class DocumentObject;
}

namespace Gui
{
class ViewProviderDocumentObject;
}

namespace MbDFEMGui
{

MbDFEMGuiExport void setOriginInTreeVisible(App::DocumentObject* object, bool visible);
MbDFEMGuiExport void hideOriginInTree(App::DocumentObject* object);
MbDFEMGuiExport App::DocumentObject* getOriginObject(App::DocumentObject* object);
MbDFEMGuiExport bool delegateSubobjectDetailPath(const Gui::ViewProviderDocumentObject* parent,
                                                 const char* subname,
                                                 SoFullPath* path,
                                                 bool append,
                                                 SoDetail*& det);

}  // namespace MbDFEMGui
