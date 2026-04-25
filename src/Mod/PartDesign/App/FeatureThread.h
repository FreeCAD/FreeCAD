// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "FeatureSketchBased.h"

namespace PartDesign {

class PartDesignExport Thread: public ProfileBased
{

public:
    Thread();

protected:
    void onChanged(const App::Property* prop) override;

private:
    void addThreadType();
};

}