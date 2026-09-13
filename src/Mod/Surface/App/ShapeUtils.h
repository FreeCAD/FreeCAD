// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/App/TopoShape.h>

namespace App
{
class DocumentObject;
}

namespace Surface
{

Part::TopoShape getTopoShapeInGlobalCoordinates(const App::DocumentObject* obj);

}
